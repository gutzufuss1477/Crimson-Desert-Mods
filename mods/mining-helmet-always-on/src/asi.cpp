#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <intrin.h>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include "capture_state.hpp"
#include "render_state.hpp"
#include "hash_core.hpp"
#include "build_guards.hpp"
#include "vendor/minhook/include/MinHook.h"

using namespace mh125;
namespace {
uintptr_t base{};
HMODULE module{};
std::atomic<bool> ready{false};
// Observer code is retained for local regression coverage. The candidate
// starts automatically; F8 only retries or resumes the native transition.
#ifdef MH125_TEST_HOST
bool diagnosticOnly=false;
#else
constexpr bool diagnosticOnly=false;
#endif
std::atomic<unsigned> captureCount{0}, fxCount{0}, tickCount{0};
std::atomic<uint64_t> cleanupAt{0};
std::atomic<bool> workPending{false};
std::atomic<bool> renderWorkPending{false};
std::atomic_flag draining=ATOMIC_FLAG_INIT;
State state;
RenderState renderState;
std::atomic<unsigned> renderTicks{0},renderSamples{0},renderApplies{0};
SRWLOCK stateLock = SRWLOCK_INIT;
SRWLOCK logLock = SRWLOCK_INIT;
constexpr size_t LogCapacity=2048, LineSize=1024;
char logQueue[LogCapacity][LineSize]{};
size_t logRead{}, logWrite{}, logCount{};
unsigned logDropped{};
FILE* logFile{};
uint64_t logBytes{};
wchar_t statusPath[MAX_PATH]{};
using EnqueueFn = uint32_t* (__fastcall*)(void*, uint32_t*, const void*, uintptr_t);
using EffectFn = uint32_t* (__fastcall*)(void*, uint32_t*, const void*, const void*);
// Disassembly uses XMM1 (vaddss); Ghidra's inferred integer parameter is incorrect.
using UpdateFn = void (__fastcall*)(void*, float);
EnqueueFn enqueueOriginal{};
EffectFn effectOriginal{};
UpdateFn updateOriginal{};
UpdateFn renderOriginal{};
using GeometryFn=bool (__fastcall*)(void*,uintptr_t,float*,float*,float*,float*,float*,float*,uint32_t*,uint8_t*);
using ApplyGeometryFn=void (__fastcall*)(uintptr_t,const float*,const float*,const float*,float,float,uint32_t);
GeometryFn getGeometry{};
ApplyGeometryFn applyGeometry{};
struct alignas(16) NativeRef {
    uintptr_t vtable{}, actor{};
    uint8_t acquired{}, mode{};
    uint8_t padding[6]{};
    uintptr_t extra{};
};
static_assert(sizeof(NativeRef)==0x20);
static_assert(offsetof(NativeRef,acquired)==0x10);
using LookupFn=NativeRef* (__fastcall*)(uintptr_t,NativeRef*,uint32_t,uintptr_t);
using PlayerFn=NativeRef* (__fastcall*)(uintptr_t,NativeRef*,uintptr_t,uintptr_t);
using ReleaseFn=void (__fastcall*)(NativeRef*);
LookupFn lookupActor{};
PlayerFn lookupPlayer{};
ReleaseFn releaseRef{};
struct ActorLease {
    NativeRef ref{};
    ~ActorLease() { if(ref.acquired) releaseRef(&ref); }
    ActorLease()=default;
    ActorLease(const ActorLease&)=delete;
    ActorLease& operator=(const ActorLease&)=delete;
};


bool readBytes(const void* source, void* dest, size_t n) {
    if (!source) return false;
    __try { std::memcpy(dest,source,n); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
template<class T> bool read(uintptr_t p, T& value) { return readBytes(reinterpret_cast<void*>(p),&value,sizeof(value)); }
void log(const char* kind, const char* format="", ...) {
    char details[768]{};
    va_list args; va_start(args,format); vsnprintf_s(details,sizeof(details),_TRUNCATE,format,args); va_end(args);
    AcquireSRWLockExclusive(&logLock);
    if (logCount < LogCapacity) {
        sprintf_s(logQueue[logWrite], "{\"ms\":%llu,\"thread\":%lu,\"kind\":\"%s\"%s%s}\n",
            GetTickCount64(),GetCurrentThreadId(),kind,details[0]?",":"",details);
        logWrite=(logWrite+1)%LogCapacity; ++logCount;
    } else ++logDropped;
    ReleaseSRWLockExclusive(&logLock);
}
void flushLog() {
    char line[LineSize];
    for(;;) {
        AcquireSRWLockExclusive(&logLock);
        if (!logCount) { ReleaseSRWLockExclusive(&logLock); break; }
        std::memcpy(line,logQueue[logRead],LineSize); logRead=(logRead+1)%LogCapacity; --logCount;
        ReleaseSRWLockExclusive(&logLock);
        if (logFile && logBytes<16*1024*1024) logBytes+=std::fwrite(line,1,std::strlen(line),logFile);
    }
    if(logFile) std::fflush(logFile);
}
bool identity(void* component, Identity& id) {
    id.component=reinterpret_cast<uintptr_t>(component);
    uintptr_t vt{}, components{}, current{};
    return read(id.component,vt) && vt==base+0x55b7898 &&
        read(id.component+8,id.actor) && id.actor &&
        read(id.actor+0x60,id.actorId) && id.actorId &&
        read(id.actor+0x90,id.ownerId) &&
        read(id.actor+0x68,components) && components &&
        read(components+0x30,current) && current==id.component;
}
bool naturalCaller(uintptr_t address, uint32_t kind) {
    const auto rva=address-base;
    return (kind==On && (rva==0x61e189 || rva==0x61e5e9)) ||
           (kind==Off && rva==0x620ca8);
}
uint32_t* __fastcall enqueueHook(void* self, uint32_t* result, const void* eventPtr, uintptr_t fourth) {
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    Event snapshot{}; Identity id{}; uint32_t kind{};
    const bool capture=ready.load(std::memory_order_acquire) &&
        readBytes(eventPtr,&kind,4) && naturalCaller(caller,kind) &&
        identity(self,id) && readBytes(eventPtr,snapshot.data(),snapshot.size());
    uint32_t* ret=enqueueOriginal(self,result,eventPtr,fourth);
    if(capture) {
        uint32_t code=0xffffffff; readBytes(result,&code,4);
        bool saved=false;
        if(code==0) {
            AcquireSRWLockExclusive(&stateLock);
            saved=state.capture(id,snapshot,GetTickCount64());
            ReleaseSRWLockExclusive(&stateLock);
            if(saved) captureCount.fetch_add(1,std::memory_order_relaxed);
        }
        log("native_event","\"event\":%u,\"actor\":%u,\"owner\":%u,\"component\":\"%llx\",\"caller_rva\":\"%llx\",\"result\":%u,\"saved\":%s,\"source\":%u,\"source2\":%u,\"game_time\":%llu",
            kind,id.actorId,id.ownerId,id.component,caller-base,code,saved?"true":"false",
            field<uint32_t>(snapshot,0x10),field<uint32_t>(snapshot,0x14),field<uint64_t>(snapshot,0x18));
    }
    return ret;
}
uint32_t* __fastcall effectHook(void* self, uint32_t* result, const void* handler, const void* context) {
    uint32_t eventId{}; uint64_t time{}; uintptr_t handlerVt{}; Identity id{};
    uint16_t effectIndex=0xffff; uint8_t control=0xff;
    const bool relevant=ready.load(std::memory_order_acquire) &&
        readBytes(context,&eventId,4) && (eventId==On || eventId==Off) && identity(self,id) &&
        read(reinterpret_cast<uintptr_t>(handler),handlerVt) && handlerVt==base+0x57c3118;
    unsigned syntheticSerial=0;
    if(relevant) {
        read(reinterpret_cast<uintptr_t>(handler)+0x6c,effectIndex);
        read(reinterpret_cast<uintptr_t>(handler)+0xd0,control);
        read(reinterpret_cast<uintptr_t>(context)+0x18,time);
        AcquireSRWLockShared(&stateLock);
        if(auto* t=state.find(id); t && t->sentEvent==eventId && t->sentTime==time) syntheticSerial=t->sentSerial;
        ReleaseSRWLockShared(&stateLock);
    }
    auto* ret=effectOriginal(self,result,handler,context);
    if(relevant) {
        uint32_t code=0xffffffff; readBytes(result,&code,4);
        fxCount.fetch_add(1,std::memory_order_relaxed);
        log("effect_switch","\"event\":%u,\"actor\":%u,\"effect_index\":%u,\"control\":%u,\"result\":%u,\"synthetic_serial\":%u,\"game_time\":%llu",
            eventId,id.actorId,effectIndex,control,code,syntheticSerial,time);
    }
    return ret;
}
bool gameTime(uint64_t& value) {
#ifdef MH125_TEST_HOST
    value=123456789;
    return true;
#else
    uintptr_t tlsArray=__readgsqword(0x58), tls{};
    uint8_t special{}; uint64_t delta{};
    if(!read(tlsArray,tls) || !tls || !read(tls+0x1ec,special) || !read(base+0x6d69438,delta)) return false;
    auto now=reinterpret_cast<uint64_t(__fastcall*)()>(base+0x1416b80);
    value=now()+(special?0:delta);
    return true;
#endif
}
bool actorManager(uintptr_t& local) {
#ifdef MH125_TEST_HOST
    local=1; return true;
#else
    uintptr_t manager{};
    return read(base+0x6d69190,manager) && manager && read(manager+0x30,local) && local;
#endif
}
void dispatchOne(Dispatch& dispatch) {
    uintptr_t manager{};
    if(!actorManager(manager)) { log("replay_refused","\"reason\":\"manager_unavailable\",\"serial\":%u",dispatch.serial); return; }
    ActorLease player;
    // Use the same local-player resolver as the original FindMine producer.
    // The +0x50 context used by the queue is a different actor (not the event source).
    lookupPlayer(manager,&player.ref,0,0);
    uint32_t playerId{};
    if(!player.ref.acquired || !player.ref.actor || !read(player.ref.actor+0x60,playerId) ||
       playerId!=field<uint32_t>(dispatch.event,0x10) || playerId!=field<uint32_t>(dispatch.event,0x14)) {
        log("replay_refused","\"reason\":\"player_unavailable_or_changed\",\"actor\":%u,\"expected_source\":%u,\"actual_source\":%u,\"serial\":%u",
            dispatch.id.actorId,field<uint32_t>(dispatch.event,0x10),playerId,dispatch.serial); return;
    }
    ActorLease target;
    lookupActor(manager,&target.ref,dispatch.id.actorId,0);
    uintptr_t components{}, component{}; Identity current{};
    if(!target.ref.acquired || !target.ref.actor ||
       !read(target.ref.actor+0x68,components) || !components || !read(components+0x30,component) ||
       !identity(reinterpret_cast<void*>(component),current) || !(current==dispatch.id)) {
        log("replay_refused","\"reason\":\"target_unavailable_or_identity_changed\",\"actor\":%u,\"serial\":%u",dispatch.id.actorId,dispatch.serial); return;
    }
    uint64_t time{};
    if(!gameTime(time)) { log("replay_refused","\"reason\":\"game_tls_unavailable\",\"serial\":%u",dispatch.serial); return; }
    set(dispatch.event,0x18,time);
    AcquireSRWLockExclusive(&stateLock);
    bool stillCurrent=state.current(dispatch);
    if(stillCurrent) state.stamp(dispatch,time);
    ReleaseSRWLockExclusive(&stateLock);
    if(!stillCurrent) { log("replay_refused","\"reason\":\"newer_command_or_native_event\",\"serial\":%u",dispatch.serial); return; }
    // The actor is newly resolved and held by a native lease for the entire call.
    uint32_t result=0xffffffff;
    enqueueOriginal(reinterpret_cast<void*>(component),&result,dispatch.event.data(),0);
    AcquireSRWLockExclusive(&stateLock);
    state.result(dispatch,result);
    ReleaseSRWLockExclusive(&stateLock);
    log("replay","\"serial\":%u,\"event\":%u,\"actor\":%u,\"result\":%u,\"game_time\":%llu",
        dispatch.serial,dispatch.kind,dispatch.id.actorId,result,time);
}
#include "full_mode.hpp"
#include "ore_only.hpp"
void __fastcall updateHook(void* self, float dt) {
    // Any observed live gimmick update drives the queue; static ore need not tick.
    updateOriginal(self,dt);
    if(!ready.load(std::memory_order_acquire)) return;
    tickCount.fetch_add(1,std::memory_order_relaxed);
    lastGimmickUpdate.store(GetTickCount64(),std::memory_order_relaxed);
    if(!workPending.load(std::memory_order_acquire) ||
         draining.test_and_set(std::memory_order_acquire)) return;
    for(unsigned i=0;i<8;++i) {
        Dispatch dispatch{};
        AcquireSRWLockExclusive(&stateLock);
        bool send=state.next(GetTickCount64(),dispatch);
        workPending.store(state.hasPending(),std::memory_order_release);
        ReleaseSRWLockExclusive(&stateLock);
        if(!send) break;
        dispatchOne(dispatch);
    }
    draining.clear(std::memory_order_release);
}
void __fastcall renderHook(void* self,float dt) {
    renderOriginal(self,dt);
    if(!ready.load(std::memory_order_acquire)) return;
    renderTicks.fetch_add(1,std::memory_order_relaxed);
    if(diagnosticOnly) return;
    uintptr_t manager{};
    if(!actorManager(manager)) return;
    ActorLease player;
    lookupPlayer(manager,&player.ref,0,0);
    if(!player.ref.acquired || !player.ref.actor) return;
    uintptr_t vtable{}; uint32_t playerId{};
    if(!read(reinterpret_cast<uintptr_t>(self),vtable) || vtable!=base+0x558d140 ||
       !read(player.ref.actor+0x60,playerId)) return;
    // Apply from the verified native render callback with a freshly held player.
    // This path works even when the native getter's entry detour is not reached.
    if(oreOnly.load() && ourOreContext(reinterpret_cast<uintptr_t>(self),oreIndex.load())) {
        renderOwnedOre(reinterpret_cast<uintptr_t>(self),player.ref.actor);
        return;
    }
    if(oreOnly.load() && helmetOverride.load() && oreRenderContextRejected.fetch_add(1)==0)
        log("ore_render_context_rejected","\"component\":\"%llx\",\"expected_component\":\"%llx\",\"player\":\"%llx\",\"expected_player\":\"%llx\"",
            reinterpret_cast<uintptr_t>(self),oreComponent.load(),player.ref.actor,overridePlayer.load());
    DetectGeometry geometry{};
    const bool nativeGeometry=getGeometry(self,player.ref.actor,geometry.position,geometry.rotation,
        geometry.look,geometry.up,&geometry.angle,&geometry.radius,&geometry.hat,&geometry.type);
    const auto now=GetTickCount64();
    AcquireSRWLockExclusive(&stateLock);
    bool nativeFindMine=false;
    for(const auto& t:state.targets) if(t.occupied && t.nativeOn && field<uint32_t>(t.on,0x10)==playerId) { nativeFindMine=true; break; }
    const bool first=nativeGeometry && renderState.observe(geometry,playerId,now,nativeFindMine);
    ReleaseSRWLockExclusive(&stateLock);
    if(nativeGeometry && nativeFindMine && geometry.valid()) renderSamples.fetch_add(1,std::memory_order_relaxed);
    if(first) log("render_baseline","\"player\":%u,\"hat\":%u,\"type\":%u,\"angle\":%.6g,\"radius\":%.6g,\"x\":%.6g,\"y\":%.6g,\"z\":%.6g",
        playerId,geometry.hat,geometry.type,geometry.angle,geometry.radius,geometry.position[0],geometry.position[1],geometry.position[2]);
    if(nativeGeometry && nativeFindMine && geometry.valid()) captureMode(self,player.ref.actor,playerId);
}

struct Slot { uintptr_t offset, expected; void* replacement; bool installed{}; };
Slot slots[]={{0x55b7898+0xf8,0x8dcfa0,reinterpret_cast<void*>(updateHook)},
              {0x55b7898+0x390,0x8ea530,reinterpret_cast<void*>(effectHook)},
              {0x55b7898+0x740,0x8e59d0,reinterpret_cast<void*>(enqueueHook)},
              {0x558d140+0x100,0x61ca80,reinterpret_cast<void*>(renderHook)},
              {0x558d140+0xf8,0x61c1c0,reinterpret_cast<void*>(modeUpdateHook)},
              {0x5954dc0+0xa8,0x224c5f0,reinterpret_cast<void*>(equipHook)},
              {0x5573418,0x3f41240,reinterpret_cast<void*>(frameHook)},
              {0x5d727b0,0x3f41240,reinterpret_cast<void*>(frameHook)}};
bool exchangeSlot(Slot& slot, bool install) {
    auto** address=reinterpret_cast<void**>(base+slot.offset);
    void* original=reinterpret_cast<void*>(base+slot.expected);
    DWORD old{};
    if(!VirtualProtect(address,sizeof(void*),PAGE_READWRITE,&old)) return false;
    void* previous=InterlockedCompareExchangePointer(address,install?slot.replacement:original,install?original:slot.replacement);
    DWORD discarded{}; bool restored=VirtualProtect(address,sizeof(void*),old,&discarded)!=FALSE;
    if(!restored) log("page_protection_restore_failed","\"rva\":\"%llx\"",slot.offset);
    return previous==(install?original:slot.replacement);
}
bool installHooks() {
    for(const auto& guard : Guards) {
        uint8_t bytes[16]{};
        if(!readBytes(reinterpret_cast<void*>(base+guard.rva),bytes,sizeof(bytes)) || std::memcmp(bytes,guard.bytes,sizeof(bytes))) {
            log("refused","\"reason\":\"memory_bytes_changed\",\"rva\":\"%llx\"",guard.rva); return false;
        }
    }
    for(const auto& slot : slots) {
        uintptr_t current{};
        if(!read(base+slot.offset,current) || current!=base+slot.expected) {
            log("refused","\"reason\":\"vtable_changed\",\"rva\":\"%llx\"",slot.offset); return false;
        }
    }
    enqueueOriginal=reinterpret_cast<EnqueueFn>(base+0x8e59d0);
    effectOriginal=reinterpret_cast<EffectFn>(base+0x8ea530);
    updateOriginal=reinterpret_cast<UpdateFn>(base+0x8dcfa0);
    lookupActor=reinterpret_cast<LookupFn>(base+0x8ae940);
    lookupPlayer=reinterpret_cast<PlayerFn>(base+0x8b4570);
    releaseRef=reinterpret_cast<ReleaseFn>(base+0x1434880);
    renderOriginal=reinterpret_cast<UpdateFn>(base+0x61ca80);
    getGeometry=reinterpret_cast<GeometryFn>(base+0x624300);
    applyGeometry=reinterpret_cast<ApplyGeometryFn>(base+0x624930);
    modeInfo=reinterpret_cast<InfoFn>(base+0x4a2890);
    itemInfo=reinterpret_cast<InfoFn>(base+0x38ab60);
    modeOwner=reinterpret_cast<OwnerFn>(base+0x17ee020);
    modeOn=reinterpret_cast<ModeOnFn>(base+0x621ca0);
    modeOff=reinterpret_cast<ModeOffFn>(base+0x6227a0);
    equipOriginal=reinterpret_cast<EquipFn>(base+0x224c5f0);
    modeUpdateOriginal=reinterpret_cast<UpdateFn>(base+0x61c1c0);
    startEffectOriginal=reinterpret_cast<StartEffectFn>(base+0x61d8c0);
    oreScanOriginal=reinterpret_cast<OreScanFn>(base+0x623530);
    nearbyCacheOriginal=reinterpret_cast<NearbyCacheFn>(base+0x9af360);
    nearbyQueryOriginal=reinterpret_cast<NearbyQueryFn>(base+0x9abde0);
    rotateVector=reinterpret_cast<RotateVectorFn>(base+0x394790);
    frameOriginal=reinterpret_cast<FrameFn>(base+0x3f41240);
    lockOwner=reinterpret_cast<LockOwnerFn>(base+0x393650);
    effectEntry=reinterpret_cast<EffectEntryFn>(base+0x61dcd0);
    createSequence=reinterpret_cast<CreateSequenceFn>(base+0x34daea0);
    findSequence=reinterpret_cast<FindSequenceFn>(base+0x34dc130);
    attachSequence=reinterpret_cast<AttachSequenceFn>(base+0x476290);
    stopSequence=reinterpret_cast<StopSequenceFn>(base+0x34dbca0);
    MH_STATUS hookStatus=MH_Initialize();
    if(hookStatus==MH_OK) hookStatus=MH_CreateHook(reinterpret_cast<void*>(base+0x624300),
        reinterpret_cast<void*>(oreGeometryHook),reinterpret_cast<void**>(&getGeometry));
    if(hookStatus==MH_OK) hookStatus=MH_CreateHook(reinterpret_cast<void*>(base+0x624930),
        reinterpret_cast<void*>(oreApplyGeometryHook),reinterpret_cast<void**>(&applyGeometry));
    if(hookStatus==MH_OK) hookStatus=MH_CreateHook(reinterpret_cast<void*>(base+0x61d8c0),
        reinterpret_cast<void*>(startEffectHook),reinterpret_cast<void**>(&startEffectOriginal));
    if(hookStatus==MH_OK) hookStatus=MH_CreateHook(reinterpret_cast<void*>(base+0x623530),
        reinterpret_cast<void*>(oreScanHook),reinterpret_cast<void**>(&oreScanOriginal));
    if(hookStatus==MH_OK) hookStatus=MH_CreateHook(reinterpret_cast<void*>(base+0x9af360),
        reinterpret_cast<void*>(nearbyCacheHook),reinterpret_cast<void**>(&nearbyCacheOriginal));
    if(hookStatus==MH_OK) hookStatus=MH_CreateHook(reinterpret_cast<void*>(base+0x9abde0),
        reinterpret_cast<void*>(nearbyQueryHook),reinterpret_cast<void**>(&nearbyQueryOriginal));
    if(hookStatus==MH_OK) hookStatus=MH_EnableHook(MH_ALL_HOOKS);
    if(hookStatus!=MH_OK) {
        MH_Uninitialize();
        log("refused","\"reason\":\"ore_detour_install_failed\",\"status\":%d",static_cast<int>(hookStatus)); return false;
    }
    for(auto& slot : slots) {
        if(!exchangeSlot(slot,true)) {
            for(auto& undo : slots) if(undo.installed) exchangeSlot(undo,false);
            MH_Uninitialize();
            log("refused","\"reason\":\"vtable_install_failed\""); return false;
        }
        slot.installed=true;
    }
    return true;
}
uint8_t installedGeometryBytes[16]{},installedApplyGeometryBytes[16]{},installedSequenceBytes[16]{},installedScanBytes[16]{};
uint8_t installedCacheBytes[16]{},installedQueryBytes[16]{};
bool detourSnapshot=false;
void geometryHookStatus() {
    if(base && ready.load()) {
        uint8_t geometryBytes[16]{},applyGeometryBytes[16]{},sequenceBytes[16]{},scanBytes[16]{};
        const bool got=readBytes(reinterpret_cast<void*>(base+0x624300),geometryBytes,16) &&
            readBytes(reinterpret_cast<void*>(base+0x624930),applyGeometryBytes,16) &&
            readBytes(reinterpret_cast<void*>(base+0x61d8c0),sequenceBytes,16) &&
            readBytes(reinterpret_cast<void*>(base+0x623530),scanBytes,16);
        log("detour_integrity","\"snapshot\":%s,\"readable\":%s,\"geometry_unchanged\":%s,\"final_geometry_unchanged\":%s,\"sequence_unchanged\":%s,\"scan_unchanged\":%s",
            detourSnapshot?"true":"false",got?"true":"false",
            got && detourSnapshot && !std::memcmp(geometryBytes,installedGeometryBytes,16)?"true":"false",
            got && detourSnapshot && !std::memcmp(applyGeometryBytes,installedApplyGeometryBytes,16)?"true":"false",
            got && detourSnapshot && !std::memcmp(sequenceBytes,installedSequenceBytes,16)?"true":"false",
            got && detourSnapshot && !std::memcmp(scanBytes,installedScanBytes,16)?"true":"false");
        uint8_t cacheBytes[16]{},queryBytes[16]{};
        const bool gotCandidates=readBytes(reinterpret_cast<void*>(base+0x9af360),cacheBytes,16) &&
            readBytes(reinterpret_cast<void*>(base+0x9abde0),queryBytes,16);
        log("candidate_detour_integrity","\"readable\":%s,\"cache_unchanged\":%s,\"query_unchanged\":%s",
            gotCandidates?"true":"false",
            gotCandidates && detourSnapshot && !std::memcmp(cacheBytes,installedCacheBytes,16)?"true":"false",
            gotCandidates && detourSnapshot && !std::memcmp(queryBytes,installedQueryBytes,16)?"true":"false");
    }
    log("geometry_observer","\"passive\":%s,\"entries\":%u,\"native_success\":%u,\"native_failure\":%u,\"context_rejected\":%u,\"player_rejected\":%u,\"sequence_entries\":%u",
        diagnosticOnly?"true":"false",geometryEntries.load(),geometryNativeSuccess.load(),geometryNativeFailure.load(),
        geometryContextRejected.load(),geometryPlayerRejected.load(),sequenceEntries.load());
}
void status(const char* phase) {
    geometryHookStatus();
    log("ore_render_status","\"entries\":%u,\"applies\":%u,\"failures\":%u,\"context_rejected\":%u,\"final_entries\":%u,\"final_overrides\":%u",oreRenderEntries.load(),oreRenderApplies.load(),oreRenderFailures.load(),oreRenderContextRejected.load(),oreFinalGeometryEntries.load(),oreFinalGeometryOverrides.load());
    log("ore_status","\"automatic\":%s,\"geometry_fallbacks\":%u,\"geometry_failures\":%u,\"startup_sequences_omitted\":%u",oreAutomatic.load()?"true":"false",oreGeometryCalls.load(),oreGeometryFailures.load(),oreSequenceSkips.load());
    log("frame_status","\"frames\":%u,\"verified_engine_frames\":%u",frameTicks.load(),verifiedFrameTicks.load());
    unsigned eligible{}, owned{}, used{}, on{}, off{}, overflow{};
    bool renderReady{},renderActive{};
    AcquireSRWLockShared(&stateLock);
    auto now=GetTickCount64(); eligible=state.eligibleCount(now); owned=state.syntheticCount(); overflow=state.overflow;
    renderReady=renderState.available(now); renderActive=renderState.applied;
    for(const auto& t:state.targets) if(t.occupied) { ++used; if(t.nativeOn)++on; if(t.haveOff)++off; }
    ReleaseSRWLockShared(&stateLock);
    AcquireSRWLockShared(&stateLock);
    const auto fullMode=modeTest;
    ReleaseSRWLockShared(&stateLock);
    log("mode_status","\"baseline_ready\":%s,\"mode_key\":%u,\"owned\":%s,\"desired\":%s,\"mode_updates\":%u,\"starts\":%u,\"stops\":%u,\"helmet_checks\":%u,\"helmet_bypasses\":%u",fullMode.available(now)?"true":"false",fullMode.baseline.key,fullMode.owned?"true":"false",fullMode.desired?"true":"false",modeUpdates.load(),modeStarts.load(),modeStops.load(),helmetChecks.load(),helmetBypasses.load());
    log("helmet_option_status","\"checks\":%u,\"bypasses\":%u",helmetOptionChecks.load(),helmetOptionBypasses.load());
    log("ore_range_status","\"scan_overrides\":%u,\"radius\":%.6g,\"angle\":%.6g",oreScanOverrides.load(),OreRange,OreAngleDegrees);
    log("ore_candidate_status","\"cache_calls\":%u,\"query_overrides\":%u",oreCacheCalls.load(),oreQueryOverrides.load());
    log("status","\"phase\":\"%s\",\"targets\":%u,\"native_on\":%u,\"off_captured\":%u,\"eligible\":%u,\"synthetic_on\":%u,\"captures\":%u,\"effects\":%u,\"ticks\":%u,\"target_overflow\":%u,\"render_ready\":%s,\"render_active\":%s,\"render_ticks\":%u,\"render_samples\":%u,\"render_applies\":%u",
        phase,used,on,off,eligible,owned,captureCount.load(),fxCount.load(),tickCount.load(),overflow,
        renderReady?"true":"false",renderActive?"true":"false",renderTicks.load(),renderSamples.load(),renderApplies.load());
    FILE* f{};
    if(!_wfopen_s(&f,statusPath,L"wb") && f) {
        std::fprintf(f,"Version: 1.0.0\nStatus: %s\nFull mode baseline: %s\nMode key: %u\nFull mode active: %s\nMode requested: %s\nMode updates: %u\nRender updates: %u\nMode starts: %u\nMode stops: %u\nMining helmet checks: %u\nMining helmet overrides: %u\nNative events: %u\nEffectSwitch calls: %u\nGame update calls: %u\n\nOre mode starts automatically; F8 = retry/ON, F9 = OFF, F7 = status; physical mining helmet yields to original mode.\nThe blue ore marker uses the game's normal visibility distance.\n",
            phase,fullMode.available(now)?"yes":"no",fullMode.baseline.key,fullMode.owned?"yes":"no",fullMode.desired?"yes":"no",modeUpdates.load(),renderTicks.load(),modeStarts.load(),modeStops.load(),helmetChecks.load(),helmetBypasses.load(),captureCount.load(),fxCount.load(),tickCount.load());
        std::fclose(f);
    }
}
bool foregroundGame() {
    DWORD pid{}; GetWindowThreadProcessId(GetForegroundWindow(),&pid); return pid==GetCurrentProcessId();
}
bool queueCommand(uint32_t eventId,unsigned& selected,unsigned& serial) {
    if(diagnosticOnly) {
        selected=0; serial=0;
        log("diagnostic_marker","\"key\":\"%s\",\"native_mode_requested\":false",eventId==On?"F8":"F9");
        return true;
    }
    return queueOreCommand(eventId,selected,serial);
}

void command(uint32_t eventId) {
    unsigned selected{},serial{};
    if(diagnosticOnly) {
        queueCommand(eventId,selected,serial); status("passive_marker"); Beep(700,80); return;
    }
    if(!queueCommand(eventId,selected,serial)) {
        log("command_refused","\"reason\":\"missing_or_expired_mode_baseline_or_already_active\"");
        status("mode_baseline_missing_or_already_active"); Beep(350,120); return;
    }
    log("command","\"serial\":%u,\"event\":%u,\"selected\":%u",serial,eventId,selected);
    status(selected?"command_pending":"no_eligible_target");
    Beep(selected?1000:350,120);
}
DWORD WINAPI worker(void*) {
    wchar_t local[MAX_PATH]{}, directory[MAX_PATH]{}, path[MAX_PATH]{};
    wchar_t exePath[MAX_PATH]{}; GetModuleFileNameW(nullptr,exePath,MAX_PATH);
    auto* name=wcsrchr(exePath,L'\\');
    const bool gameProcess=name && _wcsicmp(name+1,L"CrimsonDesert.exe")==0;
    if(!GetEnvironmentVariableW(L"LOCALAPPDATA",local,MAX_PATH)) return 0;
    swprintf_s(directory,L"%s\\MiningHelmetAlwaysOn",local); CreateDirectoryW(directory,nullptr);
    swprintf_s(directory,L"%s\\MiningHelmetAlwaysOn\\MH125",local); CreateDirectoryW(directory,nullptr);
    if(!gameProcess) {
        swprintf_s(directory,L"%s\\MiningHelmetAlwaysOn\\MH125\\smoke",local); CreateDirectoryW(directory,nullptr);
    }
    SYSTEMTIME t{}; GetLocalTime(&t);
    swprintf_s(path,L"%s\\test-%04u%02u%02u-%02u%02u%02u-%lu.jsonl",directory,t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,GetCurrentProcessId());
    _wfopen_s(&logFile,path,L"wb"); if(!logFile) return 0;
    swprintf_s(statusPath,L"%s\\latest-status.txt",directory);
    log("start","\"version\":\"1.0.0\",\"pid\":%lu,\"expected_sha256\":\"%s\"",GetCurrentProcessId(),ExpectedSha);
    if(!gameProcess) { log("refused","\"reason\":\"wrong_process\""); status("wrong_process"); flushLog(); return 0; }
    // Pin module: callbacks must remain resident even if an external manager requests unload.
    HMODULE pinned{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&worker),&pinned)) { log("refused","\"reason\":\"pin_failed\""); flushLog(); return 0; }
    char digest[65]{}; mh067::HashDiagnostics diagnostic{};
    if(!mh067::Sha256File(exePath,digest,&diagnostic) || std::strcmp(digest,ExpectedSha)) {
        log("refused","\"reason\":\"exe_hash_mismatch_or_read_failure\",\"actual\":\"%s\"",digest);
        status("version_refused"); flushLog(); return 0;
    }
    base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    if(!installHooks()) { status("hooks_refused"); flushLog(); return 0; }
    detourSnapshot=readBytes(reinterpret_cast<void*>(base+0x624300),installedGeometryBytes,16) &&
        readBytes(reinterpret_cast<void*>(base+0x624930),installedApplyGeometryBytes,16) &&
        readBytes(reinterpret_cast<void*>(base+0x61d8c0),installedSequenceBytes,16) &&
        readBytes(reinterpret_cast<void*>(base+0x623530),installedScanBytes,16) &&
        readBytes(reinterpret_cast<void*>(base+0x9af360),installedCacheBytes,16) &&
        readBytes(reinterpret_cast<void*>(base+0x9abde0),installedQueryBytes,16);
    ready.store(true,std::memory_order_release);
    log("ready","\"mode\":\"automatic_mining_helmet_ore_glow\",\"on_key\":119,\"off_key\":120");
    status("ready"); flushLog();
    bool previous[3]{}; const int keys[]={VK_F7,VK_F8,VK_F9}; uint64_t nextStatus=GetTickCount64()+5000;
    for(;;) {
        const bool focused=foregroundGame();
        for(size_t i=0;i<3;++i) {
            bool down=(GetAsyncKeyState(keys[i])&0x8000)!=0;
            if(focused && down && !previous[i]) {
                if(i==0) { status("manual_status"); Beep(700,80); } else command(i==1?On:Off);
            }
            previous[i]=down;
        }
        if(GetTickCount64()>=nextStatus) { status("running"); nextStatus=GetTickCount64()+5000; }
        const uint64_t due=cleanupAt.load();
        if(due && GetTickCount64()>=due) { log("automatic_cleanup"); command(Off); }
        flushLog(); Sleep(25);
    }
}
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {
        module=instance; DisableThreadLibraryCalls(instance);
        if(HANDLE thread=CreateThread(nullptr,0,worker,nullptr,0,nullptr)) CloseHandle(thread);
    }
    return TRUE;
}
