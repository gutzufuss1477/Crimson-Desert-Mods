// MH127.1 reached 0x14047e472 while the current animation state was null.
// An existing player object is not sufficient evidence that gameplay is ready.
std::atomic<uint64_t> lastGimmickUpdate{0};
extern std::atomic<unsigned> verifiedFrameTicks;
struct ModeReadySnapshot {
    uintptr_t model{},animation{},currentState{},controller{},movement{};
};
bool inspectModeReadiness(uintptr_t owner,ModeReadySnapshot& result) {
    uintptr_t components{},controllerRoot{},controllerOwner{};
    uint8_t visible{},movementState{},animationState{};
    // 0x14047e2f0 (called by BOTH modeOn and modeOff) traverses these fields.
    return read(owner+0x94,visible) && visible && read(owner+0x68,components) && components &&
        read(components+0x40,result.model) && result.model &&
        read(components+0x90,result.movement) && result.movement &&
        read(result.movement+0x220,movementState) &&
        read(result.model+0x88,result.animation) && result.animation &&
        read(result.animation+0x10,result.currentState) && result.currentState &&
        read(result.currentState+0xf5,animationState) &&
        read(result.model+0x120,result.controller) && result.controller &&
        read(result.controller,controllerRoot) && controllerRoot &&
        read(controllerRoot+8,controllerOwner) && controllerOwner==owner;
}
struct ModeReadiness {
    uintptr_t owner{},model{};
    uint64_t since{};
    unsigned firstTick{};
    bool observe(uintptr_t nextOwner,uintptr_t nextModel,bool valid,uint64_t now,uint64_t lastUpdate,unsigned ticks) {
        if(!valid || !lastUpdate || lastUpdate>now || now-lastUpdate>500) { *this={}; return false; }
        if(owner!=nextOwner || model!=nextModel || ticks<firstTick) {
            owner=nextOwner; model=nextModel; since=now; firstTick=ticks; return false;
        }
        return now>=since && now-since>=2000 && ticks-firstTick>=30;
    }
} modeReadiness;
bool nativeModeReady(uintptr_t owner) {
    ModeReadySnapshot snapshot;
    const bool valid=inspectModeReadiness(owner,snapshot);
    const auto now=GetTickCount64();
    // Readiness is driven by verified engine frames, including locations where
    // all nearby mining objects are static. The null-animation guard remains.
    const bool allowed=modeReadiness.observe(owner,snapshot.model,valid,now,now,verifiedFrameTicks.load());
    if(!allowed) {
        static uint64_t nextLog{};
        if(now>=nextLog) {
            log("mode_waiting_for_gameplay","\"native_context_ready\":%s,\"current_animation_state\":\"%llx\",\"verified_frames\":%u,\"gimmick_updates\":%u",
                valid?"true":"false",snapshot.currentState,verifiedFrameTicks.load(),tickCount.load());
            nextLog=now+5000;
        }
    }
    return allowed;
}
