#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <intrin.h>
#include <atomic>
#include <array>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cmath>
#include <share.h>
#include "../material/hash_core.hpp"
#include "../material/build_guards.hpp"
#include "range_values.hpp"
#include "range_config.hpp"
#include "group_range.hpp"
#include "../vendor/minhook/include/MinHook.h"

// Standalone build. No FullMode/ore_only/direct_bridge implementation linked.
namespace auto34 {
constexpr char Version[]="1.2.0";
constexpr uint32_t On=1417341355,Off=652747463,MineKey=1001881;
constexpr uint16_t MineIndex=2345;
constexpr size_t MaxTargets=2048;
// Keep our existing mining effects through brief departures from the scan cache.
// The shader continues to fade them at the user-configured visual cutoff.
constexpr uint64_t MiningReentryGraceMs=90000;
// Each unknown generic actor gets one initial ON attempt and at most seven
// additional attempts, exclusively when approaching it significantly.
constexpr unsigned MaxUnconfirmedProbes=8;
constexpr float ReprobeDistanceRatioSquared=0.64f;
uintptr_t base{},imageEnd{};
std::atomic<bool> ready{},enabled{true};
std::atomic_flag pumping=ATOMIC_FLAG_INIT;
std::atomic<unsigned> scans{},onCalls{},offCalls{},failures{},materialFrames{},miningCalls{},confirmed{};
std::atomic<unsigned> tracked{},owned{},phase{},nativeYields{},nativeResumes{};
std::atomic<bool> nativeYielded{false};
std::atomic<float> glowStrength{0.5f};
std::atomic<float> visualRange{60},scanRange{60},materialRangeWritten{},farthestCandidate{},farthestMine{},lastQueryRadius{};
std::atomic<bool> radialPath{true};
std::atomic<unsigned> scopedQueries{},extendedQueries{},farMiningTargets{},targetCapacityMisses{};
std::atomic<unsigned> heldMiningEffects{},miningReentries{},heldExpirations{};
std::atomic<unsigned> approachingRetries{},lateMiningIdentifications{};
std::atomic<unsigned> handlerOnBeyond30{},handlerOffBeyond30{},handlerOffCalls{};
SRWLOCK targetLock=SRWLOCK_INIT,logLock=SRWLOCK_INIT;
FILE* logFile{};
wchar_t statusPath[MAX_PATH]{};
ini37::Watcher ini;

bool readRaw(uintptr_t p,void* out,size_t n) {
    if(!p) return false;
    __try { std::memcpy(out,reinterpret_cast<void*>(p),n); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool writeMaterial(uintptr_t p,const void* in,size_t n) {
    if(!p) return false;
    __try { std::memcpy(reinterpret_cast<void*>(p),in,n); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
template<class T> bool read(uintptr_t p,T& v) { return readRaw(p,&v,sizeof(v)); }
template<class T> T value(uintptr_t p,T fallback={}) { T v=fallback; read(p,v); return v; }
void log(const char* kind,const char* format="",...) {
    char detail[800]{};
    va_list args; va_start(args,format); vsnprintf_s(detail,sizeof(detail),_TRUNCATE,format,args); va_end(args);
    AcquireSRWLockExclusive(&logLock);
    if(logFile) { std::fprintf(logFile,"{\"ms\":%llu,\"kind\":\"%s\"%s%s}\n",GetTickCount64(),kind,detail[0]?",":"",detail); std::fflush(logFile); }
    ReleaseSRWLockExclusive(&logLock);
}
struct alignas(16) Ref { uintptr_t vt{},actor{}; uint8_t acquired{},mode{},padding[6]{}; uintptr_t extra{}; };
static_assert(sizeof(Ref)==0x20);
using LookupFn=Ref*(__fastcall*)(uintptr_t,Ref*,uint32_t,uintptr_t);
using PlayerFn=Ref*(__fastcall*)(uintptr_t,Ref*,uintptr_t,uintptr_t);
using ReleaseFn=void(__fastcall*)(Ref*);
using EventInitFn=void*(__fastcall*)(void*);
using EnqueueFn=uint32_t*(__fastcall*)(void*,uint32_t*,const void*,uintptr_t);
using EffectFn=uint32_t*(__fastcall*)(void*,uint32_t*,const void*,const void*);
using FrameFn=uintptr_t(__fastcall*)(uintptr_t);
using NearbyFn=void(__fastcall*)(uintptr_t,uintptr_t);
using QueryFn=void(__fastcall*)(uintptr_t,const float*,float,float,void*);
using InfoFn=uintptr_t(__fastcall*)(const uint16_t*);
using RotateFn=void(__fastcall*)(float*,const float*);
using EyeFn=float*(__fastcall*)(uintptr_t,float*);
using OrientationFn=void(__fastcall*)(float*,const float*,float*,int,int,int);
LookupFn lookupActor{}; PlayerFn lookupPlayer{},modeOwner{}; ReleaseFn releaseRef{};
EnqueueFn enqueueOriginal{}; EffectFn effectOriginal{}; FrameFn frameOriginal{};
EventInitFn eventInit{}; NearbyFn nearbyCache{}; InfoFn modeInfo{}; RotateFn rotateVector{};
EyeFn eyePosition{}; OrientationFn orientation{};
QueryFn queryOriginal{};
struct QueryContext { uintptr_t manager{}; float radius{}; };
thread_local QueryContext queryContext{};
struct QueryScope {
    QueryContext previous;
    explicit QueryScope(QueryContext current):previous(queryContext) { queryContext=current; }
    ~QueryScope() { queryContext=previous; }
    QueryScope(const QueryScope&)=delete;
    QueryScope& operator=(const QueryScope&)=delete;
};
bool validRange(float v,float minimum=30) { return std::isfinite(v) && v>=minimum && v<=120; }
void __fastcall queryHook(uintptr_t manager,const float* position,float actorRadius,float radius,void* output) {
    const auto context=queryContext;
    // Scope only this ASI's cache request, on this thread. Nested and native
    // requests keep their original arguments; no global range value is patched.
    QueryScope nested({});
    if(ready.load() && context.manager==manager && manager && validRange(context.radius) && std::isfinite(radius) && radius>=0) {
        ++scopedQueries;
        if(radius<context.radius) { radius=context.radius; ++extendedQueries; }
        lastQueryRadius=radius;
    }
    queryOriginal(manager,position,actorRadius,radius,output);
}
bool configure(const char* command,float radius=0) {
    if(!std::strcmp(command,"off")) { enabled=false; return true; }
    if(!std::strcmp(command,"on")) { enabled=true; return true; }
    if(!std::strcmp(command,"native")) { visualRange=20; scanRange=30; radialPath=false; return true; }
    if(!std::strcmp(command,"visual") && validRange(radius,5)) { visualRange=radius; radialPath=true; return true; }
    if(!std::strcmp(command,"scan") && validRange(radius)) { scanRange=radius; return true; }
    if(!std::strcmp(command,"range") && validRange(radius,ini37::MinimumRange)) {
        visualRange=radius; scanRange=radius<30?30:radius; radialPath=true; return true;
    }
    return false;
}
void reloadIni() {
    static ini37::Result lastReport=ini37::Result::Unchanged;
    const auto result=ini.poll();
    if(result==ini37::Result::Unchanged) return;
    if(result==ini37::Result::Applied) {
        configure("range",ini.range);
        glowStrength=ini.strength;
        log("ini_applied","\"range\":%.3f,\"glow_strength\":%.2f,\"search_radius\":%.3f,\"reloads\":%u",ini.range,ini.strength,scanRange.load(),ini.reloads);
    } else if(result!=lastReport) {
        log("ini_ignored","\"reason\":\"%s\"",ini37::describe(result));
    }
    lastReport=result;
}
constexpr ByteGuard ExtraGuards[]={
    {0x502ce0,{0x48,0x89,0x5c,0x24,0x10,0x57,0x48,0x83,0xec,0x30,0x48,0x8b,0x81,0x88,0x00,0x00}},
    {0x141dce0,{0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x70,0x10,0x48,0x89,0x78,0x18,0x55}}
};
struct Lease { Ref ref{}; ~Lease() { if(ref.acquired) releaseRef(&ref); } };
struct Identity {
    uintptr_t actor{},component{}; uint32_t id{},owner{};
    bool operator==(const Identity&) const=default;
};
struct Target {
    Identity who{};
    uint32_t source{};
    uint64_t seen{},sent{},retry{};
    float distance2{},lastProbedDistance2{};
    uint8_t probes{};
    bool dispatched{},mine{},ours{},held{};
};
std::array<Target,MaxTargets> targets{};
struct Session { uintptr_t root{},manager{},player{}; uint32_t id{}; uint64_t since{},scanAt{}; float position[3]{}; bool hasPosition{}; } session;
mh34::View materialView{};
mh34::Ownership materialOwnership;

bool identify(uintptr_t component,Identity& who) {
    uintptr_t vt{},parts{};
    who.component=component;
    return read(component,vt) && vt==base+0x55b7898 && read(component+8,who.actor) && who.actor &&
        read(who.actor+0x60,who.id) && who.id && read(who.actor+0x90,who.owner) &&
        read(who.actor+0x68,parts) && parts && value<uintptr_t>(parts+0x30)==component;
}
bool sameActor(uintptr_t mgr,const Identity& expected,Lease& lease) {
    lookupActor(mgr,&lease.ref,expected.id,0);
    if(!lease.ref.acquired || lease.ref.actor!=expected.actor) return false;
    Identity current{};
    return identify(expected.component,current) && current==expected;
}
bool nativeInactive(uintptr_t player) {
    Lease owner; modeOwner(player,&owner.ref,0,0);
    if(!owner.ref.acquired || !owner.ref.actor) return false;
    const auto parts=value<uintptr_t>(owner.ref.actor+0x68);
    const auto mode=parts?value<uintptr_t>(parts+0x178):0;
    uint16_t a{},b{};
    return mode && value<uintptr_t>(mode)==base+0x558d140 &&
        read(mode+0x30,a) && read(mode+0x48,b) && a==0xffff && b==0xffff;
}
Target* find(const Identity& who) {
    for(auto& t:targets) if(t.who==who) return &t;
    return nullptr;
}
bool gameTime(uint64_t& time) {
#ifdef AUTO34_TEST_HOST
    time=123456; return true;
#else
    uintptr_t tls{}; uint8_t special{}; uint64_t delta{};
    if(!read(__readgsqword(0x58),tls) || !tls || !read(tls+0x1ec,special) || !read(base+0x6d69438,delta)) return false;
    time=reinterpret_cast<uint64_t(__fastcall*)()>(base+0x1416b80)()+(special?0:delta); return true;
#endif
}
bool send(uintptr_t mgr,const Target& t,uint32_t kind) {
    Lease target; if(!sameActor(mgr,t.who,target)) return false;
    alignas(16) std::array<uint8_t,0xe8> event{};
    eventInit(event.data()+0x30);
    uint64_t time{}; if(!gameTime(time) || !t.source) return false;
    std::memcpy(event.data(),&kind,4);
    std::memcpy(event.data()+0x10,&t.source,4); std::memcpy(event.data()+0x14,&t.source,4);
    std::memcpy(event.data()+0x18,&time,8);
    uint32_t result=0xffffffff;
    enqueueOriginal(reinterpret_cast<void*>(t.who.component),&result,event.data(),0);
    if(kind==On) ++onCalls; else ++offCalls;
    if(result) ++failures;
    return result==0;
}
bool mineRecordAlive(const Identity& who) {
    const auto parts=value<uintptr_t>(who.actor+0x68);
    const auto effects=parts?value<uintptr_t>(parts+0x60):0;
    if(!effects || value<uintptr_t>(effects+8)!=who.actor) return false;
    const auto records=value<uintptr_t>(effects+0x90); const auto n=value<uint32_t>(effects+0x98);
    if(!records || n>1024) return false;
    for(uint32_t i=0;i<n;++i) {
        const auto p=records+static_cast<uintptr_t>(i)*0x250;
        if(value<uint32_t>(p)!=MineKey) continue;
        const auto refs=value<uintptr_t>(p+0x1f8); const auto count=value<uint32_t>(p+0x200);
        if(!refs || count>128) continue;
        for(uint32_t j=0;j<count;++j) {
            const auto ref=value<uintptr_t>(refs+static_cast<uintptr_t>(j)*0x18+0x10);
            const auto object=ref?value<uintptr_t>(ref+8):0;
            if(object && value<uint8_t>(object+0x15,1)==0) return true;
        }
    }
    return false;
}
uint32_t* __fastcall effectHook(void* self,uint32_t* result,const void* handler,const void* event) {
    const auto h=reinterpret_cast<uintptr_t>(handler),e=reinterpret_cast<uintptr_t>(event);
    const auto returned=effectOriginal(self,result,handler,event);
    Identity who{};
    if(ready.load() && value<uintptr_t>(h)==base+0x57c3118 && value<uint16_t>(h+0x6c,0xffff)==MineIndex &&
       identify(reinterpret_cast<uintptr_t>(self),who)) {
        ++miningCalls;
        AcquireSRWLockExclusive(&targetLock);
        if(auto* t=find(who)) {
            if(!t->mine && t->probes>1) ++lateMiningIdentifications;
            t->mine=true;
            const auto kind=value<uint32_t>(e);
            if(kind==On && t->distance2>30*30) ++handlerOnBeyond30;
            if(kind==Off) {
                ++handlerOffCalls;
                if(t->distance2>30*30) ++handlerOffBeyond30;
                t->dispatched=false; t->ours=false; t->retry=GetTickCount64()+300;
            }
        }
        ReleaseSRWLockExclusive(&targetLock);
    }
    return returned;
}
void clearTargets(uintptr_t mgr,bool sendOff) {
    for(size_t i=0;i<targets.size();++i) {
        Target old{};
        AcquireSRWLockExclusive(&targetLock); old=targets[i]; targets[i]={}; ReleaseSRWLockExclusive(&targetLock);
        if(mgr && sendOff && old.who.id && old.ours) send(mgr,old,Off);
    }
    tracked=0; owned=0; confirmed=0; farMiningTargets=0; farthestMine=0; heldMiningEffects=0;
}
void suspend(uintptr_t mgr,bool native,mh34::Read reader=readRaw,mh34::Write writer=writeMaterial) {
    if(native) {
        // Leave the native mode entirely alone while it is active. Crucially,
        // retain our last-written values and original snapshot: the native
        // handoff may leave some of our material values in place.
        nativeYielded=true;
    } else {
        // Restore only bytes that STILL match our own last write. Any values
        // changed by the lantern/another mod remain under their ownership.
        materialOwnership.release(reader,writer);
        nativeYielded=false;
    }
    clearTargets(mgr,!native); materialView={}; materialRangeWritten=0;
}
bool normalize(float* v) {
    const float n=v[0]*v[0]+v[1]*v[1]+v[2]*v[2];
    if(!std::isfinite(n) || n<0.00001f) return false;
    for(unsigned i=0;i<3;++i) v[i]/=std::sqrt(n);
    return true;
}
bool cameraGeometry(uintptr_t player,float* direction,float* worldUp,float* p,float* look,float* up) {
    if(!normalize(direction) || !normalize(worldUp)) return false;
    alignas(16) float rotation[4]{},position[4]{};
    if(eyePosition(player,position)!=position) return false;
    for(unsigned i=0;i<3;++i) if(!std::isfinite(position[i]) || std::fabs(position[i])>1e8f) return false;
    // This is the exact camera branch of native 0x624300, without reading or
    // changing an active SpecialMode slot. The native helper mutates worldUp,
    // so both camera inputs above are local copies.
    orientation(rotation,direction,worldUp,3,2,-1);
    float norm{}; for(float f:rotation) norm+=f*f;
    if(!std::isfinite(norm) || norm<0.5f || norm>1.5f) return false;
    rotateVector(look,rotation); rotateVector(up,rotation);
    for(auto v:{look,up}) {
        if(!normalize(v)) return false;
    }
    std::memcpy(p,position,12);
    return true;
}
bool geometry(uintptr_t root,uintptr_t player,float* p,float* look,float* up) {
    const auto parts=value<uintptr_t>(player+0x68),model=parts?value<uintptr_t>(parts+0x40):0;
    const auto vt=model?value<uintptr_t>(model):0,metadata=value<uintptr_t>(player+0x88);
    const auto type=metadata?value<uint8_t>(metadata+1):0;
    if(vt<base || vt>=imageEnd || (type!=1 && type!=3 && type!=4 && type!=5 && type!=6 && type!=8 && type!=9)) return false;
    const uint16_t index=12; const auto info=modeInfo(&index);
    if(!info || value<uint32_t>(info)!=10 || value<uint8_t>(info+0x1c8)!=1 || value<uint8_t>(info+0x200)!=1 ||
       !readRaw(info+0x1ec,look,12) || !readRaw(info+0x1e0,up,12)) return false;
    const auto holder=value<uintptr_t>(root+0xd8),camera=holder?value<uintptr_t>(holder+8):0;
    alignas(16) float direction[4]{},worldUp[4]{};
    return camera && readRaw(camera+0xd4,direction,12) && readRaw(base+0x5545e50,worldUp,12) &&
        cameraGeometry(player,direction,worldUp,p,look,up);
}
void markTargetSeen(Target& target,uint64_t now,float distance2) {
    if(target.held) { ++miningReentries; target.held=false; }
    target.seen=now; target.distance2=distance2;
}
void scan(uintptr_t root,uintptr_t mgr,uintptr_t player,uint32_t source,uint64_t now) {
    const auto holder=value<uintptr_t>(root+0xc8),nearby=holder?value<uintptr_t>(holder):0;
    if(!nearby) return;
    const float requested=scanRange.load(),limit=requested<45?45:requested;
    { QueryScope scope({nearby,requested}); nearbyCache(nearby,player); }
    const auto entries=value<uintptr_t>(nearby+0x308); const auto n=value<uint32_t>(nearby+0x310);
    if(!value<uint8_t>(nearby+0x318) || !entries || n>65536) return;
    ++scans; float farthest{};
    for(uint32_t i=0;i<n;++i) {
        const auto entry=entries+static_cast<uintptr_t>(i)*0x28;
        float distance2{};
        if(!read(entry+0x20,distance2) || !std::isfinite(distance2) || distance2<0 || distance2>limit*limit) continue;
        const auto actor=value<uintptr_t>(entry+8); const auto id=actor?value<uint32_t>(actor+0x60):0;
        if(!id || id==source) continue;
        Lease lease; lookupActor(mgr,&lease.ref,id,0);
        if(!lease.ref.acquired || lease.ref.actor!=actor) continue;
        const auto parts=value<uintptr_t>(actor+0x68),component=parts?value<uintptr_t>(parts+0x30):0;
        Identity who{}; if(!identify(component,who)) continue;
        AcquireSRWLockExclusive(&targetLock);
        auto* t=find(who);
        if(!t) for(auto& slot:targets) if(!slot.who.id) { slot.who=who; slot.source=source; t=&slot; break; }
        if(t) markTargetSeen(*t,now,distance2);
        else ++targetCapacityMisses;
        if(distance2>farthest) farthest=distance2;
        ReleaseSRWLockExclusive(&targetLock);
    }
    farthestCandidate=std::sqrt(farthest);
}
void updateTargets(uintptr_t mgr,uint64_t now) {
    unsigned budget=16,all{},mine{},active{},farCount{},heldCount{}; float farthest{};
    for(size_t i=0;i<targets.size();++i) {
        Target t{}; AcquireSRWLockShared(&targetLock); t=targets[i]; ReleaseSRWLockShared(&targetLock);
        if(!t.who.id) continue;
        if(now-t.seen>2000) {
            if(t.mine && t.ours && now-t.seen<=MiningReentryGraceMs) {
                // A mine's emitter can remain alive beyond the cache radius.
                // Do not send our OFF as soon as that object leaves the cache:
                // otherwise the shader fades outward but requires close-range
                // reacquisition when the player reverses direction.
                ++heldCount;
                if(!t.held) {
                    AcquireSRWLockExclusive(&targetLock);
                    if(targets[i].who==t.who && targets[i].seen==t.seen) targets[i].held=true;
                    ReleaseSRWLockExclusive(&targetLock);
                }
                continue;
            }
            if(!budget) continue;
            --budget;
            if(t.ours) send(mgr,t,Off);
            if(t.held) ++heldExpirations;
            AcquireSRWLockExclusive(&targetLock); if(targets[i].who==t.who) targets[i]={}; ReleaseSRWLockExclusive(&targetLock);
            continue;
        }
        ++all; if(t.mine) ++mine; if(t.ours) ++active;
        if(t.mine) { if(t.distance2>30*30) ++farCount; if(t.distance2>farthest) farthest=t.distance2; }
        if(!budget || now<t.retry) continue;
        // Initial native OnDetectMode visits nearby generic objects just like
        // the helmet. Only objects whose handler selects the Mining effect
        // are subsequently maintained. Unmatched candidates are not spammed.
        // The queue can acknowledge ON before a distant actor is fully
        // prepared; treating that ACK as permanent would prevent activation
        // even after the player comes closer. Retry a bounded number of times
        // only when the actor moves into a significantly closer distance band.
        if(t.dispatched && !t.mine &&
           (t.probes>=MaxUnconfirmedProbes || t.lastProbedDistance2<=0 ||
            !(t.distance2<t.lastProbedDistance2*ReprobeDistanceRatioSquared))) continue;
        if(t.dispatched && t.mine && now-t.sent<2000) continue;
        Lease lease; if(!sameActor(mgr,t.who,lease)) continue;
        if(t.dispatched && t.mine && mineRecordAlive(t.who)) continue;
        --budget;
        const bool retryUnknown=t.dispatched && !t.mine;
        const bool ok=send(mgr,t,On);
        AcquireSRWLockExclusive(&targetLock);
        if(targets[i].who==t.who) {
            targets[i].dispatched=ok; targets[i].ours=ok || targets[i].ours;
            targets[i].sent=now; targets[i].retry=now+(ok?2000:5000);
            if(ok && !targets[i].mine) {
                if(targets[i].probes<MaxUnconfirmedProbes) ++targets[i].probes;
                targets[i].lastProbedDistance2=targets[i].distance2;
                if(retryUnknown) ++approachingRetries;
            }
        }
        ReleaseSRWLockExclusive(&targetLock);
    }
    tracked=all; confirmed=mine; owned=active; farMiningTargets=farCount; farthestMine=std::sqrt(farthest);
    heldMiningEffects=heldCount;
}
void tick() {
    const auto now=GetTickCount64(),root=value<uintptr_t>(base+0x6d69190),mgr=root?value<uintptr_t>(root+0x30):0;
    if(!root || !mgr) { suspend(0,false); session={}; phase=1; return; }
    Lease player; lookupPlayer(mgr,&player.ref,0,0);
    const auto id=player.ref.actor?value<uint32_t>(player.ref.actor+0x60):0;
    if(!player.ref.acquired || !id) { suspend(mgr,false); session={}; phase=1; return; }
    if(!nativeInactive(player.ref.actor)) {
        if(!nativeYielded.load() && (materialOwnership.owned || owned.load())) {
            suspend(mgr,true);
            ++nativeYields;
            log("native_yield","\"material_lease\":%s",materialOwnership.owned?"true":"false");
        }
        session={}; phase=2; return;
    }
    if(!enabled.load()) { suspend(mgr,false); session={}; phase=3; return; }
    if(session.root!=root || session.manager!=mgr || session.player!=player.ref.actor || session.id!=id) {
        // Cleanup uses each recorded source ID, not the newly selected player.
        // This also finishes the deferred native handoff, after the native
        // SpecialMode is actually inactive and before a new material apply.
        const bool resuming=nativeYielded.load();
        suspend(mgr,false);
        if(resuming) { ++nativeResumes; log("native_resume","\"material_lease_released\":true"); }
        session={root,mgr,player.ref.actor,id,now}; phase=4;
        log("context","\"player\":%u",id); return;
    }
    if(now-session.since<1500) { phase=4; return; }
    float p[3]{},look[3]{},up[3]{};
    if(!geometry(root,player.ref.actor,p,look,up)) { materialOwnership.release(readRaw,writeMaterial); phase=5; return; }
    if(session.hasPosition) {
        float d{}; for(unsigned i=0;i<3;++i) d+=(p[i]-session.position[i])*(p[i]-session.position[i]);
        if(d>60*60) { suspend(mgr,false); session.since=now; session.scanAt=0; session.hasPosition=false; phase=4; return; }
    }
    std::memcpy(session.position,p,12); session.hasPosition=true;
    if(now>=session.scanAt) { scan(root,mgr,player.ref.actor,id,now); session.scanAt=now+500; }
    updateTargets(mgr,now);
    if(!mh34::valid(readRaw,materialView) && !mh34::resolve(readRaw,root,base,materialView)) { phase=6; return; }
    const float radius=visualRange.load();
    const bool radial=radialPath.load();
    if(materialOwnership.apply(readRaw,writeMaterial,materialView,range36::values(p,look,up,radius,radial,glowStrength.load()))) { ++materialFrames; materialRangeWritten=radial?radius*10:30; phase=7; }
    else phase=8;
}
uintptr_t __fastcall frameHook(uintptr_t self) {
    const auto result=frameOriginal(self);
    if(!ready.load() || !(result&0xff) || value<DWORD>(base+0x6a652d4)!=GetCurrentThreadId() || pumping.test_and_set()) return result;
    tick(); pumping.clear(); return result;
}
struct Slot { uintptr_t rva,expected; void* hook; bool installed{}; };
Slot slots[]={ {0x55b7c28,0x8ea530,reinterpret_cast<void*>(effectHook)}, {0x5573418,0x3f41240,reinterpret_cast<void*>(frameHook)} };
bool exchange(Slot& slot,bool install) {
    auto** p=reinterpret_cast<void**>(base+slot.rva); DWORD old{},ignored{};
    if(!VirtualProtect(p,8,PAGE_READWRITE,&old)) return false;
    auto* original=reinterpret_cast<void*>(base+slot.expected);
    const auto before=InterlockedCompareExchangePointer(p,install?slot.hook:original,install?original:slot.hook);
    const bool changed=before==(install?original:slot.hook);
    if(changed) slot.installed=install; // Also track changes if protection restoration fails.
    const bool restored=VirtualProtect(p,8,old,&ignored)!=FALSE;
    return restored && changed;
}
bool install() {
    constexpr uintptr_t used[]={0x8ea530,0x8e59d0,0x3f41240,0x8ae940,0x8b4570,0x1434880,0x17ee020,
        0x1416b80,0x20efba0,0x9af360,0x4a2890,0x394790,0x502ce0,0x141dce0,0x9abde0};
    for(auto rva:used) {
        bool good{}; uint8_t bytes[16]{};
        for(const auto& g:Guards) if(g.rva==rva) { good=readRaw(base+rva,bytes,16) && !std::memcmp(bytes,g.bytes,16); break; }
        for(const auto& g:ExtraGuards) if(g.rva==rva) { good=readRaw(base+rva,bytes,16) && !std::memcmp(bytes,g.bytes,16); break; }
        if(!good) { log("refused","\"rva\":\"%llx\"",rva); return false; }
    }
    for(const auto& s:slots) if(value<uintptr_t>(base+s.rva)!=base+s.expected) { log("slot_conflict"); return false; }
    lookupActor=reinterpret_cast<LookupFn>(base+0x8ae940); lookupPlayer=reinterpret_cast<PlayerFn>(base+0x8b4570);
    modeOwner=reinterpret_cast<PlayerFn>(base+0x17ee020); releaseRef=reinterpret_cast<ReleaseFn>(base+0x1434880);
    enqueueOriginal=reinterpret_cast<EnqueueFn>(base+0x8e59d0); effectOriginal=reinterpret_cast<EffectFn>(base+0x8ea530);
    frameOriginal=reinterpret_cast<FrameFn>(base+0x3f41240); eventInit=reinterpret_cast<EventInitFn>(base+0x20efba0);
    nearbyCache=reinterpret_cast<NearbyFn>(base+0x9af360); modeInfo=reinterpret_cast<InfoFn>(base+0x4a2890);
    rotateVector=reinterpret_cast<RotateFn>(base+0x394790);
    eyePosition=reinterpret_cast<EyeFn>(base+0x502ce0); orientation=reinterpret_cast<OrientationFn>(base+0x141dce0);
    const auto initialized=MH_Initialize();
    if(initialized!=MH_OK && initialized!=MH_ERROR_ALREADY_INITIALIZED) { log("query_hook_init_failed"); return false; }
    auto* queryAddress=reinterpret_cast<void*>(base+0x9abde0);
    if(MH_CreateHook(queryAddress,reinterpret_cast<void*>(queryHook),reinterpret_cast<void**>(&queryOriginal))!=MH_OK) { log("query_hook_create_failed"); return false; }
    auto* groupAddress=reinterpret_cast<void*>(base+ore_range::ReaderRva);
    if(!ore_range::guard(base,readRaw) ||
       MH_CreateHook(groupAddress,reinterpret_cast<void*>(ore_range::onRead),reinterpret_cast<void**>(&ore_range::original))!=MH_OK) {
        MH_RemoveHook(queryAddress); log("group_hook_refused"); return false;
    }
    for(auto& s:slots) {
        if(!exchange(s,true)) {
            for(auto& old:slots) if(old.installed) exchange(old,false);
            MH_RemoveHook(groupAddress); MH_RemoveHook(queryAddress); return false;
        }
    }
    if(MH_QueueEnableHook(queryAddress)!=MH_OK || MH_QueueEnableHook(groupAddress)!=MH_OK || MH_ApplyQueued()!=MH_OK) {
        MH_DisableHook(groupAddress); MH_DisableHook(queryAddress);
        MH_RemoveHook(groupAddress); MH_RemoveHook(queryAddress);
        for(auto& old:slots) if(old.installed) exchange(old,false);
        log("hooks_enable_failed"); return false;
    }
    return true;
}
void status(const char* state) {
    FILE* f=_wfsopen(statusPath,L"wb",_SH_DENYNO); if(!f) return;
    std::fprintf(f,"Version: %s\nPID: %lu\nStatus: %s\nReady: %s\nEnabled: %s\nPhase: %u\n"
        "Nearby scans: %u\nTracked candidates: %u\nConfirmed mining targets: %u\nOwned event targets: %u\n"
        "ON calls: %u\nOFF calls: %u\nRejected events: %u\nMining handler calls: %u\nMaterial frames: %u\n"
        "Player SpecialMode writes: none\nGameplay predicate overrides: none\nHotkey or native helmet capture required: no\n"
        "0 startup; 1 no player; 2 native mode; 3 disabled; 4 settling; 5 geometry; 6 material unavailable; 7 active; 8 material yielded\n",
        Version,GetCurrentProcessId(),state,ready.load()?"yes":"no",enabled.load()?"yes":"no",phase.load(),scans.load(),tracked.load(),
        confirmed.load(),owned.load(),onCalls.load(),offCalls.load(),failures.load(),miningCalls.load(),materialFrames.load());
    std::fprintf(f,"Native handoffs: %u\nNative resumptions: %u\nDeferred material lease: %s\n",
        nativeYields.load(),nativeResumes.load(),nativeYielded.load()?"yes":"no");
    std::fprintf(f,"Requested visual radius: %.1f\nRequested search radius: %.1f\nLast written material radius: %.1f\n"
        "Scoped search calls: %u\nExtended search calls: %u\nLast search radius: %.1f\n"
        "Farthest candidate: %.1f\nFarthest mining target: %.1f\nMining targets beyond 30: %u\nTarget capacity misses: %u\n",
        visualRange.load(),scanRange.load(),materialRangeWritten.load(),scopedQueries.load(),extendedQueries.load(),lastQueryRadius.load(),
        farthestCandidate.load(),farthestMine.load(),farMiningTargets.load(),targetCapacityMisses.load());
    std::fprintf(f,"Approaching-object ON retries: %u\nLate mining confirmations after retries: %u\nMaximum unconfirmed probes per actor: %u\n",
        approachingRetries.load(),lateMiningIdentifications.load(),MaxUnconfirmedProbes);
    std::fprintf(f,"Retained mining effects outside scan: %u\nReturn events while retained: %u\nExpired retained effects: %u\nRetention grace milliseconds: %llu\n",
        heldMiningEffects.load(),miningReentries.load(),heldExpirations.load(),static_cast<unsigned long long>(MiningReentryGraceMs));
    std::fprintf(f,"Mining ON handler calls last scanned distance >30: %u\nMining OFF handler calls last scanned distance >30: %u\nMining OFF handler calls total: %u\n",
        handlerOnBeyond30.load(),handlerOffBeyond30.load(),handlerOffCalls.load());
    std::fprintf(f,"Shader distance path: %s\nGlow strength: %.2f\n",radialPath.load()?"adjustable radial (hat=0, angle=0)":"native fixed 20 (hat=1)",glowStrength.load());
    std::fprintf(f,"INI file: Mining_Helmet_Always_On.ini (next to ASI)\nINI status: %s\nINI range: %.3f\nINI GlowStrength: %.2f\nINI successful reads: %u\n",
        ini37::describe(ini.status),ini.range,ini.strength,ini.reloads);
    std::fprintf(f,"Ore group records extended: %u\nOre group records refused: %u\nOre object capacity: 128\n",
        ore_range::changed.load(),ore_range::refused.load());
    std::fclose(f);
}
DWORD WINAPI worker(void*) {
    wchar_t local[MAX_PATH]{},dir[MAX_PATH]{},path[MAX_PATH]{},exe[MAX_PATH]{};
    if(!GetEnvironmentVariableW(L"LOCALAPPDATA",local,MAX_PATH)) return 0;
    swprintf_s(dir,L"%s\\MiningHelmetAlwaysOn",local); CreateDirectoryW(dir,nullptr);
    swprintf_s(dir,L"%s\\MiningHelmetAlwaysOn\\MH120",local); CreateDirectoryW(dir,nullptr);
    GetModuleFileNameW(nullptr,exe,MAX_PATH); const auto name=wcsrchr(exe,L'\\');
    const bool game=name && !_wcsicmp(name+1,L"CrimsonDesert.exe");
    if(!game) { swprintf_s(dir,L"%s\\MiningHelmetAlwaysOn\\MH120\\smoke",local); CreateDirectoryW(dir,nullptr); }
    swprintf_s(path,L"%s\\mining-%lu.jsonl",dir,GetCurrentProcessId()); logFile=_wfsopen(path,L"wb",_SH_DENYNO);
    swprintf_s(statusPath,L"%s\\latest-status.txt",dir);
    if(!game) { status("wrong_process"); log("wrong_process"); return 0; }
    HMODULE pinned{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&worker),&pinned)) { status("pin_failed"); return 0; }
    char hash[65]{};
    if(!mh067::Sha256File(exe,hash) || std::strcmp(hash,ExpectedSha)) { status("version_refused"); log("version_refused"); return 0; }
    if(!ini.fromModule(pinned)) { status("ini_path_failed"); return 0; }
    reloadIni();
    base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    const auto nt=base+value<uint32_t>(base+0x3c); imageEnd=base+value<uint32_t>(nt+0x50);
    if(!install()) { status("hooks_refused"); return 0; }
    ready=true; log("ready","\"automatic\":true,\"player_mode_writes\":0");
    for(;;) {
        reloadIni();
        status("running"); Sleep(1000);
    }
}
}
#ifndef AUTO34_TEST_HOST
BOOL APIENTRY DllMain(HMODULE module,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) { DisableThreadLibraryCalls(module); auto t=CreateThread(nullptr,0,auto34::worker,nullptr,0,nullptr); if(t) CloseHandle(t); }
    return TRUE;
}
#endif
