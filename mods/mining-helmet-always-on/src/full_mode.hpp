// Included inside asi.cpp's namespace, after native-reference and logging helpers.
using InfoFn=uintptr_t (__fastcall*)(const uint16_t*);
using OwnerFn=NativeRef* (__fastcall*)(uintptr_t,NativeRef*,uintptr_t,uintptr_t);
using ModeOnFn=uint32_t* (__fastcall*)(uintptr_t,uint32_t*,uintptr_t,const uint32_t*,const uint64_t*);
using ModeOffFn=uint32_t* (__fastcall*)(uintptr_t,uint32_t*,uintptr_t,const uint32_t*);
using EquipFn=uint8_t (__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);
InfoFn modeInfo{},itemInfo{};
OwnerFn modeOwner{};
ModeOnFn modeOn{};
ModeOffFn modeOff{};
EquipFn equipOriginal{};
UpdateFn modeUpdateOriginal{};
struct ModeBaseline {
    uintptr_t component{},owner{},player{};
    uint32_t ownerId{},playerId{},key{};
    uint16_t index{0xffff};
    uint8_t slot{};
    uint64_t capturedAt{};
};
struct ModeTest {
    ModeBaseline baseline{};
    bool captured{},desired{},owned{};
    uint64_t activationStamp{},nextWatch{};
    unsigned serial{};
    bool available(uint64_t now) const { return captured && now-baseline.capturedAt<=120000; }
} modeTest;
std::atomic<bool> modeWork{false},helmetOverride{false};
std::atomic<uintptr_t> overridePlayer{0};
std::atomic<uint32_t> overridePlayerId{0};
std::atomic<unsigned> helmetChecks{0},helmetBypasses{0},modeUpdates{0},modeStarts{0},modeStops{0};
std::atomic<unsigned> helmetOptionChecks{0},helmetOptionBypasses{0};
// The shipping behavior is always-on.  F9 remains a test/debug opt-out and F8
// can re-enable it without restarting the game.
std::atomic<bool> oreOnly{true},oreAutomatic{true};
std::atomic<uintptr_t> oreComponent{0};
std::atomic<uint16_t> oreIndex{0xffff};
thread_local uintptr_t oreStartingComponent{};
thread_local uint16_t oreStartingIndex{0xffff};
void armAutomaticOreMode();
std::atomic<uint64_t> nextCaptureDiagnostic{0};
#ifdef MH125_TEST_HOST
DWORD testRequiredModeThread=GetCurrentThreadId();
#endif
bool modeThreadAllowed() {
    DWORD required{};
#ifdef MH125_TEST_HOST
    required=testRequiredModeThread;
#else
    // Native effect creation 0x152E0B8B0 rejects any other thread with -1.
    // Its SpecialMode caller dereferences the failed lookup without a null check.
    if(!read(base+0x6a652d4,required)) return false;
#endif
    const DWORD actual=GetCurrentThreadId();
    if(required && actual==required) return true;
    static std::atomic<uint64_t> nextLog{0};
    const auto now=GetTickCount64(); auto due=nextLog.load();
    if(now>=due && nextLog.compare_exchange_strong(due,now+5000))
        log("mode_thread_deferred","\"required_thread\":%lu,\"actual_thread\":%lu",required,actual);
    return false;
}
#include "mode_effect.hpp"
#include "mode_readiness.hpp"
// Read the same equipped-item array as native 0x14c8edf80, under the
// already verified actor lock. Resolve only equipped entries, not the whole table.
bool physicalMiningHelmet(uintptr_t player,bool& equipped) {
    equipped=false;
    BorrowedOwnerLock lock;
    if(!lock.acquire(player)) return false;
    uintptr_t components{},equipment{},bag{},entries{}; uint32_t count{};
    if(!read(player+0x68,components) || !components ||
       !read(components+0x38,equipment) || !equipment ||
       !read(equipment+0x90,bag) || !bag || !read(bag+8,entries) ||
       !read(bag+0x10,count) || count>4096 || (count && !entries)) return false;
    for(uint32_t i=0;i<count;++i) {
        uint16_t index{}; uint32_t key{};
        if(!read(entries+static_cast<uintptr_t>(i)*0xd0+8,index)) return false;
        if(index==0xffff) continue;
        const auto info=itemInfo(&index);
        if(!info || !read(info,key)) return false;
        if(key==1000143) { equipped=true; return true; }
    }
    return true;
}
void captureDiagnostic(const char* reason,uint32_t key=0,uint32_t source=0,uint64_t link=0) {
    const auto now=GetTickCount64(); auto due=nextCaptureDiagnostic.load();
    if(now>=due && nextCaptureDiagnostic.compare_exchange_strong(due,now+5000))
        log("mode_baseline_rejected","\"reason\":\"%s\",\"key\":%u,\"source\":%u,\"action_link\":\"%llx\"",reason,key,source,link);
}
void yieldOreToOriginalHelmet() {
    if(!oreOnly.load()) return;
    AcquireSRWLockExclusive(&stateLock);
    if(modeTest.owned || modeTest.desired) {
        helmetOverride=false;
        modeTest.desired=false; ++modeTest.serial;
        modeWork=modeTest.owned;
        log("ore_yield_to_original_helmet","\"cleanup_pending\":%s",modeTest.owned?"true":"false");
    }
    ReleaseSRWLockExclusive(&stateLock);
}

uint8_t __fastcall equipHook(uintptr_t self,uintptr_t context,uintptr_t third,uintptr_t fourth) {
    const uint8_t original=equipOriginal(self,context,third,fourth);
    if(diagnosticOnly) return original;
    if(!ready.load(std::memory_order_acquire) || !helmetOverride.load(std::memory_order_acquire)) return original;
    uintptr_t vt{},contextVt{},actor{}; uint32_t actorId{},key{}; uint16_t index{}; int16_t variant{};
    // Both RTTI-verified contexts store the operator/player at +0x828.
    // 0x61dfd0 uses the derived context for the per-target FindMine On condition;
    // accepting only the base context keeps the mode alive but rejects every ore event.
    if(!read(self,vt) || vt!=base+0x5954dc0 || !read(context,contextVt) ||
       (contextVt!=base+0x5574488 && contextVt!=base+0x558d650) ||
       !read(context+0x828,actor) || !actor || actor!=overridePlayer.load() ||
       !read(actor+0x60,actorId) || actorId!=overridePlayerId.load() ||
       !read(self+0x18,index) || index==0xffff || !read(self+0x1a,variant) || variant!=-1) return original;
    const uintptr_t info=itemInfo(&index);
    if(!info || !read(info,key) || key!=1000143) return original;
    helmetChecks.fetch_add(1,std::memory_order_relaxed);
    const bool optionContext=contextVt==base+0x558d650;
    if(optionContext) helmetOptionChecks.fetch_add(1,std::memory_order_relaxed);
    // The original predicate recognizes the real helmet. Stop owning its mode
    // on the next verified frame; never invoke engine transitions on this worker.
    if(original==0 && oreOnly.load()) { yieldOreToOriginalHelmet(); return original; }
    if(original==1 && helmetOverride.load(std::memory_order_acquire)) {
        const unsigned count=helmetBypasses.fetch_add(1,std::memory_order_relaxed)+1;
        if(count<=3) log("helmet_check_override","\"player\":%u,\"item\":%u,\"original\":1,\"returned\":0",actorId,key);
        if(optionContext && helmetOptionBypasses.fetch_add(1,std::memory_order_relaxed)<3)
            log("helmet_option_override","\"player\":%u,\"item\":%u,\"context_rva\":\"558d650\",\"original\":1,\"returned\":0",actorId,key);
        return 0; // Native condition result: 0 = satisfied, 1 = false, 2 = inapplicable.
    }
    return original;
}
void __fastcall modeUpdateHook(void* self,float dt) {
    modeUpdateOriginal(self,dt);
    if(ready.load(std::memory_order_acquire)) modeUpdates.fetch_add(1,std::memory_order_relaxed);
}
bool modeComponent(uintptr_t owner,uintptr_t& component,uint32_t& ownerId) {
    uintptr_t components{},vt{},actualOwner{},description{}; uint8_t type{},active{};
    // Same owner-type and live-state requirements as the native activation receiver.
    return read(owner+0x88,description) && description && read(description+1,type) && type==1 &&
        read(owner+0x96,active) && active && read(owner+0x60,ownerId) && ownerId && read(owner+0x68,components) && components &&
        read(components+0x178,component) && component && read(component,vt) && vt==base+0x558d140 &&
        read(component+8,actualOwner) && actualOwner==owner;
}
void captureMode(void* self,uintptr_t player,uint32_t playerId) {
    AcquireSRWLockShared(&stateLock);
    const bool busy=modeTest.desired || modeTest.owned;
    ReleaseSRWLockShared(&stateLock);
    if(busy) return;
    ActorLease owner; modeOwner(player,&owner.ref,0,0);
    uintptr_t component{}; uint32_t ownerId{};
    if(!owner.ref.acquired || !owner.ref.actor || !modeComponent(owner.ref.actor,component,ownerId) ||
       component!=reinterpret_cast<uintptr_t>(self)) { captureDiagnostic("owner_mapping_mismatch"); return; }
    ModeBaseline found{}; unsigned count=0;
    for(uint8_t slot=0;slot<2;++slot) {
        uint16_t index=0xffff; uint32_t key{},source{}; uint8_t type{},infoSlot{}; uint64_t link{};
        if(!read(component+0x30+slot*0x18,index) || index==0xffff) continue;
        const uintptr_t info=modeInfo(&index);
        if(!info || !read(info,key) || (key!=10 && key!=103 && key!=105) ||
           !read(info+0x1c8,type) || type!=1 || !read(info+0x11,infoSlot) || infoSlot!=slot) continue;
        if(!read(component+0x38+slot*0x18,link) || !read(component+0x40+slot*0x18,source)) continue;
        if(link!=0 || source!=playerId) { captureDiagnostic("action_link_or_source_not_replayable",key,source,link); continue; }
        ++count;
        found={component,owner.ref.actor,player,ownerId,playerId,key,index,slot,GetTickCount64()};
    }
    if(count!=1) { captureDiagnostic("no_unique_supported_mining_mode"); return; }
    AcquireSRWLockExclusive(&stateLock);
    const bool first=!modeTest.captured || modeTest.baseline.key!=found.key || modeTest.baseline.playerId!=playerId;
    if(!modeTest.desired && !modeTest.owned) { modeTest.baseline=found; modeTest.captured=true; }
    ReleaseSRWLockExclusive(&stateLock);
    if(first) log("mode_baseline","\"key\":%u,\"index\":%u,\"slot\":%u,\"player\":%u,\"owner\":%u,\"action_link\":0",found.key,found.index,found.slot,playerId,ownerId);
}
bool queueModeCommand(uint32_t eventId,unsigned& selected,unsigned& serial) {
    AcquireSRWLockExclusive(&stateLock);
    if(eventId==On && (!modeTest.available(GetTickCount64()) || modeTest.owned || modeTest.desired)) {
        ReleaseSRWLockExclusive(&stateLock); return false;
    }
    modeTest.desired=eventId==On;
    serial=++modeTest.serial;
    selected=(modeTest.desired || modeTest.owned)?1:0;
    modeWork.store(selected!=0,std::memory_order_release);
    if(eventId==Off) helmetOverride.store(false,std::memory_order_release);
    ReleaseSRWLockExclusive(&stateLock);
    cleanupAt.store(eventId==On && !oreOnly.load()?GetTickCount64()+45000:0);
    return true;
}
void releaseModeOwnership(const char* reason) {
    helmetOverride=false;
    // Keep the user's persistent enable state across player/world replacement.
    // armAutomaticOreMode waits for empty native slots and for the physical
    // mining helmet to be removed, so this does not fight another mode or B.
    AcquireSRWLockExclusive(&stateLock);
    modeTest.owned=false; modeTest.desired=false;
    modeWork=false;
    ReleaseSRWLockExclusive(&stateLock);
    cleanupAt=0;
    log("mode_released","\"reason\":\"%s\"",reason);
}
void pumpMode() {
    if(diagnosticOnly) return;
    // Called only after the launcher's frame callback; verify engine identity too.
    if(!modeThreadAllowed()) return;
    AcquireSRWLockShared(&stateLock);
    const auto request=modeTest;
    ReleaseSRWLockShared(&stateLock);
    uintptr_t manager{};
    if(!actorManager(manager)) { helmetOverride=false; return; }
    ActorLease player;
    lookupPlayer(manager,&player.ref,0,0);
    uint32_t currentPlayer{};
    if(!player.ref.acquired || !player.ref.actor || !read(player.ref.actor+0x60,currentPlayer)) { helmetOverride=false; return; }
    const bool changed=player.ref.actor!=request.baseline.player || currentPlayer!=request.baseline.playerId;
    ActorLease previousPlayer;
    uintptr_t source=player.ref.actor;
    if(changed) {
        helmetOverride=false;
        if(!request.owned) { releaseModeOwnership("player_changed_before_start"); return; }
        lookupActor(manager,&previousPlayer.ref,request.baseline.playerId,0);
        uint32_t oldId{};
        if(!previousPlayer.ref.acquired || previousPlayer.ref.actor!=request.baseline.player ||
           !read(previousPlayer.ref.actor+0x60,oldId) || oldId!=request.baseline.playerId) {
            releaseModeOwnership("old_player_unloaded"); return;
        }
        source=previousPlayer.ref.actor;
    }
    ActorLease owner; modeOwner(source,&owner.ref,0,0);
    uintptr_t component{}; uint32_t ownerId{};
    if(!owner.ref.acquired || owner.ref.actor!=request.baseline.owner ||
       !modeComponent(owner.ref.actor,component,ownerId) || component!=request.baseline.component || ownerId!=request.baseline.ownerId) {
        releaseModeOwnership("owner_or_component_changed"); return;
    }
    uint16_t slotsNow[2]{}; uint64_t stamp{};
    if(!read(component+0x30,slotsNow[0]) || !read(component+0x48,slotsNow[1]) || !read(component+0x18,stamp)) {
        releaseModeOwnership("mode_state_unreadable"); return;
    }
    if(request.owned && (slotsNow[request.baseline.slot]!=request.baseline.index || stamp!=request.activationStamp)) {
        releaseModeOwnership("native_mode_changed"); return;
    }
    const bool want=request.desired && !changed && (oreOnly.load() || request.available(GetTickCount64()));
    // A running mode must not depend on nearby gimmicks ticking. Static mines
    // can leave that counter unchanged for minutes during ordinary gameplay.
    // Actual animation/model invalidation still suspends the override.
    ModeReadySnapshot live;
    if(!inspectModeReadiness(owner.ref.actor,live)) { modeReadiness={}; helmetOverride=false; return; }
    if((!request.owned || !want) && !nativeModeReady(owner.ref.actor)) return;
    if(request.owned && want) {
        AcquireSRWLockShared(&stateLock);
        if(modeTest.desired && modeTest.serial==request.serial) helmetOverride=true;
        ReleaseSRWLockShared(&stateLock);
    }
    if(!request.owned && want) {
        if(slotsNow[0]!=0xffff || slotsNow[1]!=0xffff) { releaseModeOwnership("original_mode_still_active"); return; }
        if(oreOnly.load()) {
            bool equipped{};
            if(!physicalMiningHelmet(source,equipped)) return;
            // Avoid starting and immediately stopping the native lamp/ore state.
            // Leave the original helmet's B toggle completely in its own hands.
            if(equipped) { releaseModeOwnership("physical_helmet_before_start"); return; }
        }
        const auto info=modeInfo(&request.baseline.index); uint32_t key{}; uint8_t slot{};
        if(!info || !read(info,key) || key!=request.baseline.key || !read(info+0x11,slot) || slot!=request.baseline.slot) {
            releaseModeOwnership("mode_table_changed"); return;
        }
        if(oreOnly.load()) {
            uint32_t sequence{};
            if(key!=10 || !read(component+0x6c+slot*4,sequence) || sequence!=0xffffffff) {
                releaseModeOwnership("ore_start_sequence_still_present"); return;
            }
        }
        AcquireSRWLockShared(&stateLock);
        const bool current=modeTest.serial==request.serial && modeTest.desired;
        ReleaseSRWLockShared(&stateLock);
        if(!current) return;
        overridePlayer=source; overridePlayerId=request.baseline.playerId;
        oreComponent=component; oreIndex=request.baseline.index;
        helmetOverride=true;
        PreparedEffect prepared;
        if(!oreOnly.load() && !prepareModeEffect(component,owner.ref.actor,ownerId,request.baseline.index,slot,info,prepared)) {
            releaseModeOwnership("effect_preparation_failed"); return;
        }
        AcquireSRWLockShared(&stateLock);
        const bool stillWanted=modeTest.serial==request.serial && modeTest.desired;
        ReleaseSRWLockShared(&stateLock);
        if(!stillWanted) {
            helmetOverride=false;
            if(!oreOnly.load()) abandonPreparedEffect(component,slot,prepared);
            releaseModeOwnership("cancelled_during_effect_preparation"); return;
        }
        uint32_t result=0xffffffff; const uint64_t noActionLink=0;
        if(oreOnly.load()) { oreStartingComponent=component; oreStartingIndex=request.baseline.index; }
        modeOn(component,&result,source,&key,&noActionLink);
        oreStartingComponent=0; oreStartingIndex=0xffff;
        if(prepared.created) restorePreparedRefresh(component,slot,prepared);
        uint16_t after=0xffff; uint64_t afterStamp{};
        const bool applied=read(component+0x30+slot*0x18,after) && after==request.baseline.index && read(component+0x18,afterStamp);
        AcquireSRWLockExclusive(&stateLock);
        // Even if F9 arrived during activation, retain ownership so it gets cleaned up.
        modeTest.owned=applied; modeTest.activationStamp=afterStamp; modeTest.nextWatch=0;
        if(!applied) modeTest.desired=false;
        if(!modeTest.desired || !applied) helmetOverride=false;
        modeWork=modeTest.owned || modeTest.desired;
        ReleaseSRWLockExclusive(&stateLock);
        if(!applied && !oreOnly.load()) abandonPreparedEffect(component,slot,prepared);
        if(applied) modeStarts.fetch_add(1,std::memory_order_relaxed);
        log("mode_start","\"serial\":%u,\"key\":%u,\"result\":%u,\"slot_confirmed\":%s,\"stamp\":%llu",request.serial,key,result,applied?"true":"false",afterStamp);
    } else if(request.owned && !want) {
        helmetOverride=false;
        uint32_t result=0xffffffff;
        modeOff(component,&result,source,&request.baseline.key);
        uint16_t after{}; const bool cleared=read(component+0x30+request.baseline.slot*0x18,after) && after==0xffff;
        modeStops.fetch_add(1,std::memory_order_relaxed);
        log("mode_stop","\"serial\":%u,\"result\":%u,\"slot_cleared\":%s",request.serial,result,cleared?"true":"false");
        releaseModeOwnership(cleared?"test_stopped":"native_stop_not_confirmed");
    } else if(request.owned && GetTickCount64()>=request.nextWatch) {
        log("mode_watch","\"key\":%u,\"slot_confirmed\":true,\"mode_updates\":%u,\"render_ticks\":%u,\"helmet_checks\":%u,\"helmet_bypasses\":%u",request.baseline.key,modeUpdates.load(),renderTicks.load(),helmetChecks.load(),helmetBypasses.load());
        AcquireSRWLockExclusive(&stateLock);
        modeTest.nextWatch=GetTickCount64()+1000;
        ReleaseSRWLockExclusive(&stateLock);
    } else if(!request.owned && !want) releaseModeOwnership("request_expired_or_cancelled");
}
using FrameFn=uintptr_t (__fastcall*)(uintptr_t);
FrameFn frameOriginal{};
std::atomic<unsigned> frameTicks{0},verifiedFrameTicks{0};
std::atomic_flag modeDraining=ATOMIC_FLAG_INIT;
uintptr_t __fastcall frameHook(uintptr_t self) {
    const uintptr_t result=frameOriginal(self);
    if(!ready.load(std::memory_order_acquire) || !(result&0xff)) return result;
    frameTicks.fetch_add(1,std::memory_order_relaxed);
    if(!modeThreadAllowed()) return result;
    if(verifiedFrameTicks.fetch_add(1,std::memory_order_relaxed)==0)
        log("mode_frame_ready","\"verified_engine_thread\":%lu",GetCurrentThreadId());
    if(diagnosticOnly) return result;
    if(oreOnly.load() && oreAutomatic.load()) armAutomaticOreMode();
    if(modeWork.load(std::memory_order_acquire) && !modeDraining.test_and_set(std::memory_order_acquire)) {
        pumpMode();
        modeDraining.clear(std::memory_order_release);
    }
    return result;
}
