#define MH125_TEST_HOST
#include "asi.cpp"
#include <cstdlib>
namespace {
std::array<uint8_t,0x100> playerMem{},otherPlayer{},ownerMem{},itemMem{};
std::array<uint8_t,0x200> componentMem{},componentsMem{};
std::array<uint8_t,0x240> infoMem{};
std::array<uint8_t,0x840> contextMem{};
std::array<uint8_t,0x30> equipMem{};
std::array<uint8_t,8> ownerDescription{};
std::array<uint8_t,8> ownerLockMem{},nameRefMem{},entryArray{},controllerMem{};
std::array<uint8_t,0x140> modelMem{};
std::array<uint8_t,0x230> movementMem{};
std::array<uint8_t,0x20> animationMem{},controllerRoot{};
std::array<uint8_t,0x100> animationState{};
std::array<uint8_t,0x30> lockVt{},entryMem{};
std::array<uint8_t,0x180> modelVt{};
std::array<uint8_t,0x300> sequenceMem{};
char sequenceName[]="baseseq/gamesystemfx/effect/cd_common_searcharound_start";
unsigned locks{},unlocks{},creates{},attaches{},sequenceStops{};
bool lockHeld=false,createFails=false,lookupFails=false,startFails=false,cancelDuringCreate=false,frameReturned=false;
uintptr_t frameResult=0x1234567801;
unsigned acquired{},released{},starts{},stops{};
uint8_t conditionResult=1;
bool playerPresent=true,ownerPresent=true,switchedPlayer=false,cancelDuringStart=false;
uintptr_t addr(auto& a) { return reinterpret_cast<uintptr_t>(a.data()); }
void put(auto& a,size_t off,auto value) { std::memcpy(a.data()+off,&value,sizeof(value)); }
void check(bool yes,const char* msg) { if(!yes) { std::fprintf(stderr,"FAIL: %s\n",msg); std::exit(1); } }
NativeRef* lease(NativeRef* ref,uintptr_t actor,bool present) {
    *ref={}; if(present) { ref->actor=actor; ref->acquired=1; ++acquired; } return ref;
}
NativeRef* __fastcall playerLookup(uintptr_t,NativeRef* ref,uintptr_t,uintptr_t) { return lease(ref,switchedPlayer?addr(otherPlayer):addr(playerMem),playerPresent); }
NativeRef* __fastcall actorLookup(uintptr_t,NativeRef* ref,uint32_t id,uintptr_t) { return lease(ref,addr(playerMem),id==17); }
NativeRef* __fastcall ownerLookup(uintptr_t player,NativeRef* ref,uintptr_t,uintptr_t) { return lease(ref,addr(ownerMem),ownerPresent && player==addr(playerMem)); }
void __fastcall release(NativeRef* ref) { check(ref->acquired==1,"release acquired only"); ref->acquired=0; ++released; }
uintptr_t __fastcall getMode(const uint16_t* index) { return *index==7?addr(infoMem):0; }
uintptr_t __fastcall getItem(const uint16_t* index) { return *index==9?addr(itemMem):0; }
uint8_t __fastcall evalEquip(uintptr_t,uintptr_t,uintptr_t,uintptr_t) { return conditionResult; }
void __fastcall tick(void*,float) {}
uintptr_t __fastcall frame(uintptr_t self) {
    check(self==999,"frame self forwarded"); frameReturned=true; return frameResult;
}
NativeLockRef* __fastcall lockFake(NativeLockRef* ref,uintptr_t object,uint8_t addRef) {
    check(!lockHeld && acquired-released==2 && object==addr(ownerLockMem) && addRef==0,"borrow native lock while owner lease held");
    lockHeld=true; ++locks; ref->object=object; ref->ownsReference=addRef; return ref;
}
void __fastcall unlockFake(uintptr_t object,uintptr_t mode) {
    check(lockHeld && object==addr(ownerLockMem) && mode==0,"native unlock arguments"); lockHeld=false; ++unlocks;
}
uintptr_t __fastcall entryFake(uintptr_t component) {
    check(lockHeld && component==addr(componentMem),"entry pool access under owner lock"); return addr(entryMem);
}
void __fastcall transformFake(uintptr_t model,void* out) {
    check(!lockHeld && model==addr(modelMem),"current model transform outside owner lock");
    std::memset(out,0x5a,48);
}
uint32_t __fastcall createFake(uintptr_t manager,const char* name,const void* transform,uintptr_t entry,uint8_t a,uint8_t b) {
    check(frameReturned && !lockHeld && acquired-released==2 && modeThreadAllowed(),"effect creation after original frame on verified thread with leases");
    check(manager==2 && name==sequenceName && static_cast<const uint8_t*>(transform)[47]==0x5a && entry==addr(entryMem) && !a && !b,"native create arguments");
    ++creates;
    if(cancelDuringCreate) { unsigned selected{},serial{}; queueModeCommand(Off,selected,serial); }
    return createFails?0xffffffff:123;
}
uintptr_t __fastcall findFake(uintptr_t manager,uint32_t id) {
    check(manager==2 && id==123,"never lookup invalid sequence ID"); return lookupFails?0:addr(sequenceMem);
}
void __fastcall attachFake(uintptr_t target,const void* data) {
    check(!lockHeld && target==addr(sequenceMem)+0x150,"never attach callback to null+0x150");
    const auto& closure=*static_cast<const SequenceClosure*>(data);
    check(closure.component==addr(componentMem) && closure.callback==base+0x6250b0 && !closure.extra && !closure.flags,"native completion closure layout"); ++attaches;
}
uintptr_t __fastcall stopSequenceFake(uintptr_t manager,uint32_t id,uint8_t flags) {
    check(!lockHeld && manager==2 && id==123 && !flags,"failure cleanup outside owner lock"); ++sequenceStops; return 1;
}
uint32_t* __fastcall startMode(uintptr_t self,uint32_t* result,uintptr_t player,const uint32_t* key,const uint64_t* link) {
    check(acquired-released==2,"source and owner references held during activation");
    check(self==addr(componentMem) && player==addr(playerMem) && *key==10 && *link==0,"full native activation arguments");
    check(equipHook(addr(equipMem),addr(contextMem),0,0)==0,"mining helmet condition passes during activation");
    uint32_t effectId{}; read(self+0x6c,effectId);
    check(frameReturned && !lockHeld && effectId==123,"checked sequence installed before full activation");
    put(componentMem,0xf0,uint8_t{1}); put(entryMem,0x18,uint8_t{1}); // Original helper's reuse branch.
    if(startFails) { *result=0xffffffff; return result; }
    ++starts; put(componentMem,0x30,uint16_t{7}); put(componentMem,0x40,uint32_t{17});
    put(componentMem,0x18,uint64_t{1000+starts});
    if(cancelDuringStart) { unsigned selected{},serial{}; queueModeCommand(Off,selected,serial); }
    *result=0; return result;
}
uint32_t* __fastcall stopMode(uintptr_t self,uint32_t* result,uintptr_t player,const uint32_t* key) {
    check(acquired-released>=2,"source and owner references held during stop");
    check(self==addr(componentMem) && player==addr(playerMem) && *key==10,"full native stop arguments");
    check(!helmetOverride,"helmet condition restored before native teardown");
    ++stops; put(componentMem,0x30,uint16_t{0xffff}); put(componentMem,0x40,uint32_t{0});
    put(componentMem,0x18,uint64_t{2000+stops}); *result=0; return result;
}
void setup() {
    oreOnly=false; oreAutomatic=false;
    modeTest={}; modeWork=false; workPending=false; helmetOverride=false; renderWorkPending=false;
    base=0x140000000; ready=true; playerPresent=ownerPresent=true; switchedPlayer=cancelDuringStart=false;
    put(playerMem,0x60,uint32_t{17}); put(otherPlayer,0x60,uint32_t{18});
    put(ownerMem,0x60,uint32_t{1}); put(ownerMem,0x68,addr(componentsMem));
    put(ownerDescription,1,uint8_t{1}); put(ownerMem,0x88,addr(ownerDescription)); put(ownerMem,0x96,uint8_t{1});
    put(componentsMem,0x178,addr(componentMem)); put(componentMem,0,base+0x558d140);
    put(componentMem,8,addr(ownerMem)); put(componentMem,0x30,uint16_t{7}); put(componentMem,0x48,uint16_t{0xffff});
    put(componentMem,0x38,uint64_t{0}); put(componentMem,0x40,uint32_t{17});
    put(infoMem,0,uint32_t{10}); put(infoMem,0x11,uint8_t{0}); put(infoMem,0x1c8,uint8_t{1});
    put(itemMem,0,uint32_t{1000143}); put(equipMem,0,base+0x5954dc0);
    put(equipMem,0x18,uint16_t{9}); put(equipMem,0x1a,int16_t{-1});
    put(contextMem,0,base+0x5574488); put(contextMem,0x828,addr(playerMem));
    lookupPlayer=playerLookup; lookupActor=actorLookup; modeOwner=ownerLookup; releaseRef=release;
    modeInfo=getMode; itemInfo=getItem; modeOn=startMode; modeOff=stopMode; equipOriginal=evalEquip;
    updateOriginal=tick; modeUpdateOriginal=tick;
    frameOriginal=frame; frameResult=0x1234567801; frameReturned=false;
    createFails=lookupFails=startFails=cancelDuringCreate=false;
    testRequiredModeThread=GetCurrentThreadId();
    put(ownerMem,8,addr(ownerLockMem)); put(ownerLockMem,0,addr(lockVt));
    put(lockVt,0x20,reinterpret_cast<uintptr_t>(unlockFake));
    put(componentsMem,0x40,addr(modelMem)); put(modelMem,0,addr(modelVt));
    put(ownerMem,0x94,uint8_t{1}); put(componentsMem,0x90,addr(movementMem));
    put(modelMem,0x88,addr(animationMem)); put(animationMem,0x10,addr(animationState));
    put(modelMem,0x120,addr(controllerMem)); put(controllerMem,0,addr(controllerRoot));
    put(controllerRoot,8,addr(ownerMem));
    tickCount=40; verifiedFrameTicks=40; lastGimmickUpdate=GetTickCount64();
    modeReadiness={addr(ownerMem),addr(modelMem),GetTickCount64()-2001,0};
    put(modelVt,0x178,reinterpret_cast<uintptr_t>(transformFake));
    put(infoMem,0x18,addr(nameRefMem)); put(nameRefMem,0,reinterpret_cast<uintptr_t>(sequenceName));
    put(componentMem,0x6c,uint32_t{0xffffffff}); put(componentMem,0xc8,addr(entryArray));
    put(componentMem,0xd0,uint32_t{1}); put(entryArray,0,addr(entryMem));
    put(entryMem,0x10,uint32_t{0xffffffff});
    put(entryMem,0x18,uint8_t{0}); put(componentMem,0xf0,uint8_t{0});
    lockOwner=lockFake; effectEntry=entryFake; createSequence=createFake;
    findSequence=findFake; attachSequence=attachFake; stopSequence=stopSequenceFake;
    captureMode(componentMem.data(),addr(playerMem),17);
    check(modeTest.captured && modeTest.baseline.key==10,"capture exact original mode and owner");
    put(componentMem,0x30,uint16_t{0xffff});
}
void pulse() { lastGimmickUpdate=GetTickCount64(); check(frameHook(999)==frameResult,"entire original frame return preserved"); }
void on() { unsigned selected{},serial{}; check(queueCommand(On,selected,serial) && selected==1,"queue full-mode ON"); }
void off() { unsigned selected{},serial{}; check(queueCommand(Off,selected,serial),"queue full-mode OFF"); }
}
int main() {
    diagnosticOnly=false;
    setup();
    check(equipHook(addr(equipMem),addr(contextMem),0,0)==1,"original helmet check unchanged before F8");
    on(); check(starts==0,"keyboard command makes no engine call");
    updateHook(reinterpret_cast<void*>(999),0.016f);
    check(starts==0 && creates==0 && modeWork,"gimmick callbacks no longer drive full-mode activation");
    frameResult=0x1234567800; pulse();
    check(starts==0 && creates==0 && modeWork,"terminating frame makes no activation call"); frameResult=0x1234567801;
    testRequiredModeThread=GetCurrentThreadId()+1; pulse();
    check(starts==0 && modeWork && !helmetOverride && acquired==released,"wrong engine thread cannot call native activation or enable helmet override");
    testRequiredModeThread=0; pulse();
    check(starts==0 && !helmetOverride,"uninitialized engine thread cannot start mode");
    testRequiredModeThread=GetCurrentThreadId(); pulse();
    check(starts==1 && modeTest.owned && helmetOverride,"F8 starts full mode without further original callbacks");
    check(componentMem[0xf0]==0 && entryMem[0x18]==0,"prepared fresh sequence retains native fresh-start flags");
    pulse(); check(starts==1,"no repeated activation");
    unsigned selected{},serial{};
    check(!queueCommand(On,selected,serial),"duplicate F8 rejected");
    diagnosticOnly=true;
    for(uint8_t value=0;value<3;++value) {
        conditionResult=value;
        check(equipHook(addr(equipMem),addr(contextMem),0,0)==value,"passive observer preserves every original equipment result even with stale override state");
    }
    diagnosticOnly=false; conditionResult=1;
    // The target-event dispatcher constructs CompareTargetParameterWithOperator,
    // unlike the plain context used to keep the overall mode active.
    put(contextMem,0,base+0x558d650);
    check(equipHook(addr(equipMem),addr(contextMem),0,0)==0,"ore target-event helmet condition passes in derived operator context");
    put(contextMem,0x828,addr(otherPlayer));
    check(equipHook(addr(equipMem),addr(contextMem),0,0)==1,"derived context for another actor stays unchanged");
    put(contextMem,0x828,addr(playerMem));
    put(itemMem,0,uint32_t{123});
    check(equipHook(addr(equipMem),addr(contextMem),0,0)==1,"derived context for another item stays unchanged");
    put(itemMem,0,uint32_t{1000143});
    conditionResult=2;
    check(equipHook(addr(equipMem),addr(contextMem),0,0)==2,"derived inapplicable result is preserved");
    conditionResult=1; helmetOverride=false;
    check(equipHook(addr(equipMem),addr(contextMem),0,0)==1,"derived context cannot bypass helmet outside owned override window");
    helmetOverride=true; put(contextMem,0,base+0x5574488);
    conditionResult=2; check(equipHook(addr(equipMem),addr(contextMem),0,0)==2,"inapplicable condition is preserved"); conditionResult=1;
    put(itemMem,0,uint32_t{123}); check(equipHook(addr(equipMem),addr(contextMem),0,0)==1,"other item unaffected"); put(itemMem,0,uint32_t{1000143});
    put(contextMem,0x828,addr(otherPlayer)); check(equipHook(addr(equipMem),addr(contextMem),0,0)==1,"other actor unaffected"); put(contextMem,0x828,addr(playerMem));
    put(contextMem,0,uintptr_t{123}); check(equipHook(addr(equipMem),addr(contextMem),0,0)==1,"other context type unaffected"); put(contextMem,0,base+0x5574488);
    put(equipMem,0x1a,int16_t{2}); check(equipHook(addr(equipMem),addr(contextMem),0,0)==1,"specific item variant unaffected"); put(equipMem,0x1a,int16_t{-1});
    off(); testRequiredModeThread=GetCurrentThreadId()+1; pulse();
    check(stops==0 && modeTest.owned && modeWork && !helmetOverride,"wrong thread defers native teardown without losing cleanup");
    testRequiredModeThread=GetCurrentThreadId(); pulse(); check(stops==1 && !modeTest.owned && !modeWork,"F9 clears full mode without target events");
    check(equipHook(addr(equipMem),addr(contextMem),0,0)==1,"original helmet requirement restored");

    setup(); put(componentMem,0x48,uint16_t{4}); on(); pulse();
    check(starts==1 && !modeWork,"active native mode prevents replacement");
    setup(); put(ownerMem,0x60,uint32_t{2}); on(); pulse();
    check(starts==1 && !modeWork,"changed owner identity rejects activation");
    setup(); on(); switchedPlayer=true; pulse();
    check(starts==1 && !modeWork,"changed player rejects activation");
    setup(); on(); pulse();
    put(componentMem,0x18,uint64_t{99999}); pulse();
    check(stops==1 && !modeTest.owned && !helmetOverride,"native mode replacement relinquishes ownership without stopping it");
    setup(); on(); pulse(); switchedPlayer=true; pulse();
    check(!modeWork && stops==2,"player switch tears down owned mode with freshly held old source");
    setup(); on(); pulse(); modeTest.baseline.capturedAt=GetTickCount64()-120001; pulse();
    check(!modeWork && stops==3,"expired baseline ends owned mode");
    setup(); cancelDuringStart=true; on(); pulse();
    check(modeTest.owned && modeWork && !modeTest.desired && !helmetOverride,"F9 arriving during start retains cleanup ownership");
    pulse(); check(stops==4 && !modeWork,"concurrent cancellation is cleaned on next update");
    setup(); on(); pulse(); off(); playerPresent=false; pulse();
    check(modeTest.owned && modeWork && !helmetOverride,"missing player defers cleanup");
    playerPresent=true; pulse(); check(stops==5 && !modeWork,"cleanup resumes with player");
    setup(); modeTest.captured=false; put(componentMem,0x30,uint16_t{7}); put(componentMem,0x38,uint64_t{123});
    captureMode(componentMem.data(),addr(playerMem),17);
    check(!modeTest.captured,"mode with external action pointer is not captured for replay");
    check(acquired==released,"all actor references released");
    const unsigned before=starts, beforeAttach=attaches;
    setup(); createFails=true; on(); pulse();
    check(starts==before && attaches==beforeAttach && !modeWork && !helmetOverride,"failed native creation cannot reach unchecked mode startup");
    setup(); lookupFails=true; on(); pulse();
    check(starts==before && attaches==beforeAttach && !modeWork && sequenceStops==1,"null sequence lookup never reaches callback attachment and requests native cleanup");
    setup(); cancelDuringCreate=true; on(); pulse();
    check(starts==before && !modeWork && !helmetOverride && sequenceStops==2,"F9 during preparation abandons sequence before mode activation");
    setup(); startFails=true; on(); pulse();
    check(starts==before && !modeWork && !helmetOverride && sequenceStops==3,"unconfirmed activation releases prepared effect");
    check(componentMem[0xf0]==0,"failed activation cannot request a render refresh with inactive mode index");
    const unsigned beforeCreate=creates;
    setup(); put(componentMem,0x6c,uint32_t{123}); put(entryMem,8,uint32_t{1});
    put(entryMem,0xc,uint16_t{7}); put(entryMem,0x10,uint32_t{123}); on(); pulse();
    check(starts==before+1 && creates==beforeCreate && modeTest.owned,"valid matching sequence reused without duplicate creation"); off(); pulse();
    setup(); put(componentMem,0x6c,uint32_t{123}); put(entryMem,0x10,uint32_t{123});
    put(entryMem,0xc,uint16_t{99}); on(); pulse();
    check(starts==before+1 && creates==beforeCreate && !modeWork,"foreign sequence cannot be reused");
    check(acquired==released && locks==unlocks && !lockHeld,"all actor leases and borrowed native locks balanced");
    std::puts("PASS: frame dispatch and thread identity, checked sequence preparation/failure/reuse/cleanup, full-mode capture/start/stop, isolated helmet condition, identities, expiry, cancellation, reference and lock lifetime");
}
