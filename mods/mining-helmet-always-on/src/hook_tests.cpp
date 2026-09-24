#define MH125_TEST_HOST
#include "asi.cpp"
#include <cstdlib>
#include <cmath>
namespace {
unsigned enqueues{}, updates{}, effects{};
unsigned acquires{}, releases{}, playerLookups{}, actorLookups{};
uintptr_t fakeActor{}, fakePlayer{};
bool targetAvailable=true, playerAvailable=true;
bool nativeGeometryAvailable=true;
unsigned geometryWrites{};
uint32_t lastHat{};
float lastRadius{},lastAngle{};
Event lastSent{};
float lastDt{};
void check(bool yes,const char* msg) {
    if(!yes) { std::fprintf(stderr,"FAIL: %s\n",msg); std::exit(1); }
}
uint32_t* __fastcall fakeEnqueue(void*,uint32_t* result,const void* event,uintptr_t) {
    check(acquires-releases==2,"player and target native leases held during enqueue");
    ++enqueues; std::memcpy(lastSent.data(),event,EventSize); *result=0; return result;
}
uint32_t* __fastcall fakeEffect(void*,uint32_t* result,const void*,const void*) {
    ++effects; *result=0; return result;
}
void __fastcall fakeUpdate(void*,float dt) { ++updates; lastDt=dt; }
NativeRef* __fastcall fakeLookup(uintptr_t,NativeRef* ref,uint32_t,uintptr_t) {
    ++actorLookups; *ref={};
    if(targetAvailable) { ref->actor=fakeActor; ref->acquired=1; ++acquires; }
    return ref;
}
NativeRef* __fastcall fakePlayerLookup(uintptr_t,NativeRef* ref,uintptr_t,uintptr_t) {
    ++playerLookups; *ref={};
    if(playerAvailable) { ref->actor=fakePlayer; ref->acquired=1; ++acquires; }
    return ref;
}
void __fastcall fakeRelease(NativeRef* ref) { check(ref->acquired!=0,"release only acquired ref"); ref->acquired=0; ++releases; }
void request(uint32_t kind) { workPending.store(state.request(kind,GetTickCount64())!=0); }
bool __fastcall fakeGeometry(void*,uintptr_t,float* pos,float* rotation,float* look,float* up,float* angle,float* radius,uint32_t* hat,uint8_t* type) {
    if(!nativeGeometryAvailable) return false;
    pos[0]=10; pos[1]=20; pos[2]=30; rotation[3]=1;
    look[2]=1; up[1]=1; *angle=0.5f; *radius=2000; *hat=1; *type=1;
    return true;
}
void __fastcall fakeApplyGeometry(uintptr_t,const float*,const float*,const float*,float angle,float radius,uint32_t hat) {
    check(acquires-releases==1,"player lease held during render parameter write");
    ++geometryWrites; lastHat=hat; lastRadius=radius; lastAngle=angle;
}
}
int main() {
    diagnosticOnly=false;
    // Fake objects only. No game is running and no executable game function is called.
    std::array<uint8_t,0x100> actor{}, components{}, component{}, handler{}, player{};
    auto put=[](auto& a,size_t off,auto v) { std::memcpy(a.data()+off,&v,sizeof(v)); };
    base=0x140000000;
    put(component,0,base+0x55b7898); put(component,8,reinterpret_cast<uintptr_t>(actor.data()));
    put(actor,0x60,uint32_t{300}); put(actor,0x90,uint32_t{17});
    put(actor,0x68,reinterpret_cast<uintptr_t>(components.data()));
    put(components,0x30,reinterpret_cast<uintptr_t>(component.data()));
    put(player,0x60,uint32_t{17}); fakePlayer=reinterpret_cast<uintptr_t>(player.data());
    fakeActor=reinterpret_cast<uintptr_t>(actor.data());
    Identity id{}; check(identity(component.data(),id),"live identity validation");
    Event on{},off{}; set(on,0,On); set(off,0,Off);
    for(auto* e : {&on,&off}) { set(*e,0x10,uint32_t{17}); set(*e,0x14,uint32_t{17}); }
    auto now=GetTickCount64();
    state.capture(id,on,now); state.capture(id,off,now);
    enqueueOriginal=fakeEnqueue; updateOriginal=fakeUpdate; effectOriginal=fakeEffect;
    lookupActor=fakeLookup; lookupPlayer=fakePlayerLookup; releaseRef=fakeRelease;
    ready.store(true);
    request(On);
    // Dispatcher callback belongs to a DIFFERENT object; the static target never ticks.
    updateHook(reinterpret_cast<void*>(999),0.016f);
    check(enqueues==1 && updates==1 && std::fabs(lastDt-0.016f)<0.000001f,"on replay and float dt pass-through");
    check(field<uint32_t>(lastSent,0)==On && field<uint64_t>(lastSent,0x18)==123456789,"event and timestamp");
    check(field<uint32_t>(lastSent,0x10)==17,"player context preserved");
    check(acquires==2 && releases==2,"both references released after successful enqueue");
    updateHook(component.data(),0.017f);
    check(enqueues==1 && updates==2,"no repeated per-frame event");
    put(handler,0,base+0x57c3118); put(handler,0x6c,uint16_t{123}); put(handler,0xd0,uint8_t{1});
    uint32_t result=99; effectHook(component.data(),&result,handler.data(),lastSent.data());
    check(effects==1 && result==0 && fxCount.load()==1,"effect callback and result preserved");
    request(Off); updateHook(component.data(),0.016f);
    check(enqueues==2 && field<uint32_t>(lastSent,0)==Off && state.syntheticCount()==0,"off replay clears ownership");
    put(player,0x60,uint32_t{18}); request(On); updateHook(component.data(),0.016f);
    check(enqueues==2 && updates==4,"changed player/context refuses replay but still updates game");
    check(acquires==releases,"player lease released on changed-player refusal");
    put(player,0x60,uint32_t{17}); request(On);
    put(actor,0x60,uint32_t{301}); updateHook(component.data(),0.016f);
    check(enqueues==2 && updates==5,"reused address with changed actor rejected");
    check(acquires==releases,"both leases released on changed-target refusal");
    put(actor,0x60,uint32_t{300}); targetAvailable=false; request(On); updateHook(component.data(),0.016f);
    check(enqueues==2 && acquires==releases,"unloaded target rejected and player lease released");
    targetAvailable=true; playerAvailable=false; request(On); updateHook(component.data(),0.016f);
    check(enqueues==2 && acquires==releases,"absent player rejected without leaking leases");
    playerAvailable=true;
    check(naturalCaller(base+0x61e5e9,On) && naturalCaller(base+0x620ca8,Off) &&
          !naturalCaller(base+0x61e5e9,Off),"exact producer callsite filter");
    void* memory=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    check(memory!=nullptr,"fake vtable allocation");
    const uintptr_t savedBase=base; base=reinterpret_cast<uintptr_t>(memory);
    auto** slot=static_cast<void**>(memory); *slot=reinterpret_cast<void*>(base+0x100);
    Slot testSlot{0,0x100,reinterpret_cast<void*>(fakeUpdate)};
    check(exchangeSlot(testSlot,true) && *slot==testSlot.replacement,"atomic slot installation");
    check(!exchangeSlot(testSlot,true),"slot conflict rejected");
    check(exchangeSlot(testSlot,false) && *slot==reinterpret_cast<void*>(base+0x100),"slot rollback");
    VirtualFree(memory,0,MEM_RELEASE); base=savedBase;
    std::puts("PASS: native event dispatcher, held references, identity checks, ABI and vtables");
}
