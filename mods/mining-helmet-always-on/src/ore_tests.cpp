#define MH125_TEST_HOST
#include "asi.cpp"
#include <cstdlib>
#include <thread>
namespace {
std::array<uint8_t,0x100> player{},description{},animationState{};
std::array<uint8_t,0x140> model{};
std::array<uint8_t,0x230> movement{};
std::array<uint8_t,0x20> animation{},controllerRoot{};
std::array<uint8_t,8> controller{};
std::array<uint8_t,0x200> components{},component{},modelTable{};
std::array<uint8_t,0x240> info{};
std::array<uint8_t,0x30> equipCondition{},miningItem{};
std::array<uint8_t,0x840> equipContext{};
std::array<uint8_t,0xa0> equipmentComponent{};
std::array<uint8_t,0x20> equipmentBag{},playerLock{},playerLockTable{};
std::array<uint8_t,0xd0> equippedEntry{};
unsigned equipmentLocks{},equipmentUnlocks{};
float scannedRadius{};
uintptr_t scannedComponent{},scannedPlayer{};
void* scannedOutput{};
std::array<uint8_t,0x328> nearbyManager{};
std::array<float,3> queryPosition{10,20,30};
float queriedRange{},queriedActorRadius{};
uintptr_t queriedManager{};
const float* queriedPosition{};
void* queriedOutput{};
uint32_t queryResult{};
float nestedRange{},threadRange{};
bool nestedCacheTest=false,threadCacheTest=false,cancelQueryTest=false;
unsigned cacheDepth{};
NearbyQueryFn queryEntry=nearbyQueryHook;
uint8_t equipmentResult=1;
unsigned acquired{},released{},starts{},stops{},originalEffects{},originalGeometry{};
unsigned geometryWrites{},originalRenders{};
float renderedX{},renderedRadius{};
unsigned finalGeometryWrites{};
uintptr_t finalGeometryComponent{};
float finalGeometryRadius{},finalGeometryAngle{};
uint32_t finalGeometryHat{};
bool playerPresent=true,nativeGeometry=false;
bool cancelDuringStart=false;
float localX=100;
int16_t tileX{},tileZ{};
uintptr_t addr(auto& a) { return reinterpret_cast<uintptr_t>(a.data()); }
void put(auto& a,size_t offset,auto value) { std::memcpy(a.data()+offset,&value,sizeof(value)); }
void check(bool yes,const char* why) { if(!yes) { std::fprintf(stderr,"FAIL: %s\n",why); std::exit(1); } }
NativeRef* lease(NativeRef* ref,bool present) {
    *ref={}; if(present) { ref->actor=addr(player); ref->acquired=1; ++acquired; } return ref;
}
NativeRef* __fastcall lookup(uintptr_t,NativeRef* ref,uintptr_t,uintptr_t) { return lease(ref,playerPresent); }
void __fastcall release(NativeRef* ref) { ++released; ref->acquired=0; }
uintptr_t __fastcall infoLookup(const uint16_t* index) { return *index==12?addr(info):0; }
uintptr_t __fastcall itemLookup(const uint16_t* index) { return *index==9?addr(miningItem):0; }
uint8_t __fastcall equipment(uintptr_t,uintptr_t,uintptr_t,uintptr_t) { return equipmentResult; }
NativeLockRef* __fastcall equipmentLock(NativeLockRef* ref,uintptr_t object,uint8_t flag) {
    check(object==addr(playerLock) && flag==0,"preflight borrows the player's native lock");
    ref->object=object; ++equipmentLocks; return ref;
}
void __fastcall equipmentUnlock(uintptr_t object,uintptr_t flag) {
    check(object==addr(playerLock) && flag==0,"preflight releases the borrowed lock"); ++equipmentUnlocks;
}
__declspec(noinline) void __fastcall scan(uintptr_t componentArg,uintptr_t playerArg,float radius,void* output) {
    scannedComponent=componentArg; scannedPlayer=playerArg; scannedRadius=radius; scannedOutput=output;
}
__declspec(noinline) void __fastcall query(uintptr_t manager,const float* pos,float actorRadius,float range,void* output) {
    queriedManager=manager; queriedPosition=pos; queriedActorRadius=actorRadius;
    queriedRange=range; queriedOutput=output;
    *static_cast<uint32_t*>(output)=0x1234abcd;
}
__declspec(noinline) void __fastcall cache(uintptr_t manager,uintptr_t) {
    ++cacheDepth;
    if(cacheDepth==1 && nestedCacheTest) {
        nearbyCacheHook(manager,999);
        nestedRange=queriedRange;
    }
    if(cacheDepth==1 && threadCacheTest) {
        std::thread t([manager] { queryEntry(manager,queryPosition.data(),1.5f,30,&queryResult); threadRange=queriedRange; });
        t.join();
    }
    if(cancelQueryTest) helmetOverride=false;
    queryEntry(manager,queryPosition.data(),1.5f,30,&queryResult);
    --cacheDepth;
}
uintptr_t __fastcall frame(uintptr_t) { return 1; }
void __fastcall transform(uintptr_t self,void* output) {
    check(self==addr(model),"live player model supplies transform");
    TiledModelTransform value{{1,1,1},{0,0,0,1},{localX,20,30},tileX,tileZ,0}; std::memcpy(output,&value,sizeof(value));
}
void __fastcall rotate(float*,const float* quaternion) { check(quaternion[3]==1,"native quaternion layout"); }
void __fastcall renderWithoutGetter(void*,float) { ++originalRenders; }
void __fastcall shaderGeometry(uintptr_t self,const float* pos,const float* look,const float* up,float angle,float radius,uint32_t hat) {
    check(acquired-released==1,"direct renderer holds a fresh native player reference");
    check(self==addr(component) && look[2]==1 && up[1]==1 && angle==360 && hat==1,"native geometry setter ABI and wider angle arguments");
    ++geometryWrites; renderedX=pos[0]; renderedRadius=radius;
}
__declspec(noinline) void __fastcall finalGeometry(uintptr_t self,const float*,const float*,const float*,float angle,float radius,uint32_t hat) {
    ++finalGeometryWrites; finalGeometryComponent=self; finalGeometryAngle=angle; finalGeometryRadius=radius; finalGeometryHat=hat;
}
__declspec(noinline) void __fastcall originalStart(uintptr_t,const uint16_t*) { ++originalEffects; }
__declspec(noinline) bool __fastcall geometry(void*,uintptr_t,float* position,float*,float*,float*,float*,float*,uint32_t*,uint8_t*) {
    ++originalGeometry; if(nativeGeometry) position[0]=777; return nativeGeometry;
}
uint32_t* __fastcall start(uintptr_t self,uint32_t* result,uintptr_t source,const uint32_t* key,const uint64_t*) {
    check(acquired-released==2 && source==addr(player) && self==addr(component) && *key==10,"automatic start holds fresh references");
    const uint16_t index=12;
    if(cancelDuringStart) { unsigned selected{},serial{}; queueOreCommand(Off,selected,serial); }
    const auto before=originalEffects; startEffectHook(self,&index);
    check(originalEffects==before,"automatic mode suppresses startup sequence");
    put(component,0x30,index); put(component,0x18,uint64_t{1000+ ++starts}); *result=0; return result;
}
uint32_t* __fastcall stop(uintptr_t, uint32_t* result,uintptr_t,const uint32_t*) {
    ++stops; put(component,0x30,uint16_t{0xffff}); *result=0; return result;
}
void setup() {
    diagnosticOnly=false;
    base=0x140000000; ready=true; oreOnly=true; oreAutomatic=true;
    modeTest={}; modeWork=false; oreArm={}; helmetOverride=false;
    put(player,0x60,uint32_t{17}); put(player,0x68,addr(components));
    put(player,0x88,addr(description)); put(description,1,uint8_t{1}); put(player,0x96,uint8_t{1});
    put(components,0x178,addr(component)); put(component,0,base+0x558d140); put(component,8,addr(player));
    put(component,0x30,uint16_t{0xffff}); put(component,0x48,uint16_t{0xffff});
    put(component,0x6c,uint32_t{0xffffffff});
    put(components,0x40,addr(model)); put(model,0,addr(modelTable));
    put(player,0x94,uint8_t{1}); put(components,0x90,addr(movement));
    put(model,0x88,addr(animation)); put(animation,0x10,addr(animationState));
    put(model,0x120,addr(controller)); put(controller,0,addr(controllerRoot)); put(controllerRoot,8,addr(player));
    put(modelTable,0x178,reinterpret_cast<uintptr_t>(transform));
    put(info,0,uint32_t{10}); put(info,0x11,uint8_t{0}); put(info,0x1c8,uint8_t{1});
    put(info,0x1e4,1.0f); put(info,0x1f4,1.0f); put(info,0x1f8,30.0f); put(info,0x1fc,30.0f);
    modeInfo=infoLookup; lookupPlayer=lookup; modeOwner=lookup; releaseRef=release;
    modeOn=start; modeOff=stop; frameOriginal=frame; getGeometry=geometry;
    startEffectOriginal=originalStart; rotateVector=rotate; testRequiredModeThread=GetCurrentThreadId();
    renderOriginal=renderWithoutGetter; applyGeometry=shaderGeometry;
    put(equipCondition,0,base+0x5954dc0); put(equipCondition,0x18,uint16_t{9}); put(equipCondition,0x1a,int16_t{-1});
    put(equipContext,0,base+0x5574488); put(equipContext,0x828,addr(player)); put(miningItem,0,uint32_t{1000143});
    itemInfo=itemLookup; equipOriginal=equipment;
    put(player,8,addr(playerLock)); put(playerLock,0,addr(playerLockTable));
    put(playerLockTable,0x20,reinterpret_cast<uintptr_t>(equipmentUnlock)); lockOwner=equipmentLock;
    put(components,0x38,addr(equipmentComponent)); put(equipmentComponent,0x90,addr(equipmentBag));
    put(equipmentBag,8,addr(equippedEntry)); put(equipmentBag,0x10,uint32_t{0});
    oreScanOriginal=scan;
    nearbyCacheOriginal=cache; nearbyQueryOriginal=query;
}
bool sample(GeometryFn fn,DetectGeometry& g,uintptr_t self=0,uintptr_t source=0) {
    return fn(reinterpret_cast<void*>(self?self:addr(component)),source?source:addr(player),
        g.position,g.rotation,g.look,g.up,&g.angle,&g.radius,&g.hat,&g.type);
}
void stableFrame() {
    const auto now=GetTickCount64();
    oreArm.since=now-2001; oreArm.nextTry=0;
    tickCount=40; lastGimmickUpdate=now; verifiedFrameTicks=40; modeReadiness={addr(player),addr(model),now-2001,0};
    frameHook(123);
}
}
int main() {
    TiledModelTransform logged{{1,1,1},{0,0,0,1},{-610.536f,587.891f,-940.349f},-10,-3,0};
    float absolute[3]{};
    check(logged.worldPosition(absolute) && std::fabs(absolute[0]+10610.536f)<0.002f &&
        absolute[1]==587.891f && std::fabs(absolute[2]+3940.349f)<0.002f,"reproduce live X/Z tile-offset regression; Y is not tiled");
    logged.tileX=3; logged.tileZ=-2; logged.localPosition[0]=5; logged.localPosition[2]=7;
    check(logged.worldPosition(absolute) && absolute[0]==3005 && absolute[2]==-1993,"mixed-sign tiles remain independent");
    logged.localPosition[0]=std::nanf("");
    check(!logged.worldPosition(absolute),"non-finite local position is rejected");
    check(oreAutomatic,"load-time automatic activation enabled by default");
    ModeReadiness policy;
    check(!policy.observe(1,2,true,1000,1000,1),"first live context alone is insufficient");
    check(!policy.observe(1,2,true,3001,3001,20),"time without enough updates is insufficient");
    check(policy.observe(1,2,true,3002,3002,31),"stable context and live updates allow transition");
    check(!policy.observe(1,2,true,4000,3002,50),"stale updates invalidate readiness");
    check(!policy.observe(1,2,false,4100,4100,55),"incomplete animation state invalidates readiness");
    unsigned selected{},serial{};
    setup();
    oreAutomatic=false;
    diagnosticOnly=true;
    queueCommand(On,selected,serial); stableFrame();
    check(!oreAutomatic && !modeWork && starts==0,"passive F8 never requests a native mode");
    oreAutomatic=true; armAutomaticOreMode();
    modeWork=true; modeTest.desired=true; pumpMode(); frameHook(123);
    check(starts==0,"passive policy guards direct arming, pump and frame paths");
    DetectGeometry passive{}; passive.position[0]=321;
    const auto passiveBefore=passive;
    check(!sample(oreGeometryHook,passive) && std::memcmp(&passive,&passiveBefore,sizeof(passive))==0,
        "passive native failure leaves all output storage untouched");
    nativeGeometry=true;
    check(sample(oreGeometryHook,passive) && passive.position[0]==777,"passive forwards native success and output");
    nativeGeometry=false;
    const uint16_t passiveIndex=12; const auto effectsBefore=originalEffects;
    startEffectHook(addr(component),&passiveIndex);
    check(originalEffects==effectsBefore+1 && oreSequenceSkips==0,"passive never suppresses original startup sequence");
    check(geometryEntries==2 && geometryNativeSuccess==1 && geometryNativeFailure==1,"observer distinguishes native success and failure");
    setup(); stableFrame();
    check(starts==0 && !modeWork,"loading alone cannot arm a native mode");
    frameHook(123);
    check(starts==0 && !modeWork,"wait for stable loaded player");
    put(animation,0x10,uintptr_t{0}); stableFrame();
    check(starts==0 && !modeWork,"exact MH127.1 null animation state blocks activation before mutation");
    put(animation,0x10,addr(animationState));
    stableFrame(); check(starts==1 && modeTest.owned && helmetOverride && cleanupAt==0,"always-on mode arms after gameplay readiness without F8");
    DetectGeometry g{};
    check(sample(oreGeometryHook,g) && g.position[0]==100 && g.hat==1 && g.type==1 && g.radius==250 && g.angle==360,"live geometry uses wider range and full cone trial");
    lastGimmickUpdate=GetTickCount64()-60000;
    const auto runningStarts=starts;
    frameHook(123);
    check(helmetOverride && modeTest.owned && modeTest.desired && starts==runningStarts,"stationary world objects cannot disable an already running mode");
    oreScanHook(addr(component),addr(player),30,&g);
    check(scannedRadius==250 && scannedOutput==&g,"actor scan range matches shader range and preserves output destination");
    oreScanHook(addr(component),addr(player),350,&g); check(scannedRadius==350,"larger native scan radius is not reduced");
    oreScanHook(999,addr(player),30,&g); check(scannedRadius==30,"foreign component scan unchanged");
    oreScanHook(addr(component),999,30,&g); check(scannedRadius==30,"foreign player scan unchanged");
    diagnosticOnly=true; oreScanHook(addr(component),addr(player),30,&g);
    check(scannedRadius==30,"observer scan unchanged"); diagnosticOnly=false;
    nearbyQueryHook(addr(nearbyManager),queryPosition.data(),1.5f,30,&queryResult);
    check(queriedRange==30,"standalone world queries retain original range even while ore mode is active");
    nearbyCacheHook(addr(nearbyManager),addr(player));
    check(queriedRange==250 && queriedActorRadius==1.5f && queriedManager==addr(nearbyManager) &&
        queriedPosition==queryPosition.data() && queriedOutput==&queryResult && queryResult==0x1234abcd,
        "owned upstream candidate query widens radius while retaining other arguments and native output");
    check(!oreCacheContext.manager,"candidate scope ends with producer call");
    nearbyCacheHook(addr(nearbyManager),999); check(queriedRange==30,"foreign player cache stays native");
    nestedCacheTest=true; nearbyCacheHook(addr(nearbyManager),addr(player)); nestedCacheTest=false;
    check(nestedRange==30 && queriedRange==250,"nested foreign cache cannot inherit override and outer scope resumes");
    threadCacheTest=true; nearbyCacheHook(addr(nearbyManager),addr(player)); threadCacheTest=false;
    check(threadRange==30 && queriedRange==250,"other thread cannot inherit candidate override");
    cancelQueryTest=true; nearbyCacheHook(addr(nearbyManager),addr(player)); cancelQueryTest=false;
    check(queriedRange==30,"cancellation between producer entry and query leaves native range"); helmetOverride=true;
    diagnosticOnly=true; nearbyCacheHook(addr(nearbyManager),addr(player)); diagnosticOnly=false;
    check(queriedRange==30,"observer cache stays native");
    put(component,0x30,uint16_t{13}); nearbyCacheHook(addr(nearbyManager),addr(player));
    check(queriedRange==30,"replaced mode cannot widen upstream query"); put(component,0x30,uint16_t{12});
    {
        OreCacheScope scoped({addr(nearbyManager),addr(player),addr(component)});
        nearbyQueryHook(addr(nearbyManager)+1,queryPosition.data(),1.5f,30,&queryResult);
        check(queriedRange==30,"different manager within producer scope stays native");
        nearbyQueryHook(addr(nearbyManager),queryPosition.data(),1.5f,350,&queryResult);
        check(queriedRange==350,"candidate radius never shrinks a larger native query");
    }
    localX=125; check(sample(oreGeometryHook,g) && g.position[0]==125,"geometry follows player movement");
    check(!sample(oreGeometryHook,g,999),"unrelated component unchanged");
    check(!sample(oreGeometryHook,g,0,999),"unrelated source unchanged");
    put(component,0x30,uint16_t{13}); check(!sample(oreGeometryHook,g),"different active mode is not overridden"); put(component,0x30,uint16_t{12});
    nativeGeometry=true; check(sample(oreGeometryHook,g) && g.position[0]==125,"owned mode replaces even successful native geometry with current player transform");
    check(sample(oreGeometryHook,g,999) && g.position[0]==777,"successful native geometry outside owned component is untouched");
    nativeGeometry=false;
    const auto hookEntries=geometryEntries.load();
    renderHook(component.data(),0.016f);
    check(originalRenders==1 && geometryWrites==1 && renderedX==125 && renderedRadius==250 && geometryEntries==hookEntries,
        "verified render callback applies geometry even if original render never reaches geometry detour");
    localX=145; renderHook(component.data(),0.016f);
    check(geometryWrites==2 && renderedX==145,"direct shader geometry follows live movement without target-event calibration");
    put(component,0x30,uint16_t{13}); renderHook(component.data(),0.016f);
    check(geometryWrites==2,"direct renderer refuses a replaced native slot"); put(component,0x30,uint16_t{12});
    put(components,0x40,uintptr_t{0}); renderHook(component.data(),0.016f);
    check(geometryWrites==2,"missing player model does not reach native shader setter"); put(components,0x40,addr(model));
    localX=125;
    // Reproduce the live regression: locally correct transform, wrong world tile.
    tileX=-10; tileZ=-3; localX=-610.536f;
    check(sample(oreGeometryHook,g) && std::fabs(g.position[0]+10610.536f)<0.002f && g.position[2]==-2970,
        "signed tile offsets reach native geometry hook outputs");
    renderHook(component.data(),0.016f);
    check(geometryWrites==3 && std::fabs(renderedX+10610.536f)<0.002f,"direct renderer receives absolute world coordinates");
    tileX=1; localX=999; check(sample(oreGeometryHook,g) && g.position[0]==1999,"last position before positive tile boundary");
    tileX=2; localX=0; check(sample(oreGeometryHook,g) && g.position[0]==2000,"positive tile crossing is continuous");
    tileX=-1; localX=-999; check(sample(oreGeometryHook,g) && g.position[0]==-1999,"last position before negative tile boundary");
    tileX=-2; localX=0; check(sample(oreGeometryHook,g) && g.position[0]==-2000,"negative tile crossing is continuous");
    tileX=0; tileZ=0; localX=125;
    const unsigned before=originalEffects; uint16_t other=13; startEffectHook(addr(component),&other);
    check(originalEffects==before+1,"other mode startup unchanged");
    modeTest.baseline.capturedAt=GetTickCount64()-120001; frameHook(123);
    check(modeTest.owned && stops==0,"ore-only ownership does not expire after calibration TTL");
    playerPresent=false; frameHook(123); check(!helmetOverride,"missing player disables override");
    playerPresent=true; frameHook(123); check(helmetOverride,"same owned player restores override on return");

    // Real x64 detour/trampoline calls with the ten-argument geometry ABI.
    check(MH_Initialize()==MH_OK,"initialize detour library");
    check(MH_CreateHook(reinterpret_cast<void*>(geometry),reinterpret_cast<void*>(oreGeometryHook),reinterpret_cast<void**>(&getGeometry))==MH_OK,"create geometry detour");
    applyGeometry=finalGeometry;
    check(MH_CreateHook(reinterpret_cast<void*>(finalGeometry),reinterpret_cast<void*>(oreApplyGeometryHook),reinterpret_cast<void**>(&applyGeometry))==MH_OK,"create final geometry detour");
    check(MH_CreateHook(reinterpret_cast<void*>(originalStart),reinterpret_cast<void*>(startEffectHook),reinterpret_cast<void**>(&startEffectOriginal))==MH_OK,"create startup detour");
    check(MH_CreateHook(reinterpret_cast<void*>(scan),reinterpret_cast<void*>(oreScanHook),reinterpret_cast<void**>(&oreScanOriginal))==MH_OK,"create scan detour");
    check(MH_CreateHook(reinterpret_cast<void*>(cache),reinterpret_cast<void*>(nearbyCacheHook),reinterpret_cast<void**>(&nearbyCacheOriginal))==MH_OK,"create candidate producer detour");
    check(MH_CreateHook(reinterpret_cast<void*>(query),reinterpret_cast<void*>(nearbyQueryHook),reinterpret_cast<void**>(&nearbyQueryOriginal))==MH_OK,"create candidate query detour");
    check(MH_EnableHook(MH_ALL_HOOKS)==MH_OK,"enable both detours");
    ApplyGeometryFn volatile finalGeometryTarget=finalGeometry;
    finalGeometryTarget(addr(component),g.position,g.look,g.up,30,30,0);
    check(finalGeometryWrites==1 && finalGeometryComponent==addr(component) && finalGeometryAngle==360 && finalGeometryRadius==250 && finalGeometryHat==1,
        "real final-setter trampoline clamps a later native radius reset for the owned mining component");
    finalGeometryTarget(addr(model),g.position,g.look,g.up,30,30,0);
    check(finalGeometryWrites==2 && finalGeometryComponent==addr(model) && finalGeometryAngle==30 && finalGeometryRadius==30 && finalGeometryHat==0,
        "final-setter detour preserves unrelated components");
    queryEntry=query;
    NearbyCacheFn volatile cacheTarget=cache;
    cacheTarget(addr(nearbyManager),addr(player));
    check(queriedRange==250 && queriedActorRadius==1.5f && queriedPosition==queryPosition.data() && queriedOutput==&queryResult && queryResult==0x1234abcd,
        "real producer/query x64 trampolines retain XMM2 actor radius, change XMM3 range and retain fifth stack argument/output");
    check(!oreCacheContext.manager,"real producer trampoline restores TLS scope");
    queryEntry(addr(nearbyManager),queryPosition.data(),1.5f,30,&queryResult);
    check(queriedRange==30,"real query trampoline outside producer is unchanged");
    OreScanFn volatile scanTarget=scan;
    scanTarget(addr(component),addr(player),30,&g);
    check(scannedRadius==250 && scannedComponent==addr(component) && scannedPlayer==addr(player) && scannedOutput==&g,"real x64 scan trampoline preserves float third argument and output pointer");
    GeometryFn volatile geometryTarget=geometry;
    diagnosticOnly=true;
    const auto unchanged=g;
    check(!sample(geometryTarget,g) && std::memcmp(&g,&unchanged,sizeof(g))==0,"real passive trampoline preserves native failure and all outputs");
    nativeGeometry=true;
    check(sample(geometryTarget,g) && g.position[0]==777,"real passive trampoline forwards native success");
    nativeGeometry=false;
    StartEffectFn volatile passiveEffectTarget=originalStart;
    uint16_t passiveMining=12; const auto originalCount=originalEffects;
    passiveEffectTarget(addr(component),&passiveMining);
    check(originalEffects==originalCount+1,"real passive startup trampoline preserves original helmet effect");
    diagnosticOnly=false;
    check(sample(geometryTarget,g) && g.position[0]==125,"real geometry trampoline preserves arguments and fallback outputs");
    StartEffectFn volatile effectTarget=originalStart;
    uint16_t mining=12; const auto effectCount=originalEffects;
    effectTarget(addr(component),&mining); check(originalEffects==effectCount,"real startup detour suppresses only owned mining mode");
    effectTarget(addr(component),&other); check(originalEffects==effectCount+1,"real startup trampoline forwards other modes");
    check(MH_Uninitialize()==MH_OK,"restore original test functions");
    getGeometry=geometry; applyGeometry=shaderGeometry; startEffectOriginal=originalStart;

    put(animation,0x10,uintptr_t{0});
    queueCommand(Off,selected,serial); frameHook(123);
    check(modeTest.owned && modeWork && stops==0 && !helmetOverride,"F9 during incomplete unload context defers unsafe native teardown");
    put(animation,0x10,addr(animationState)); stableFrame();
    check(!oreAutomatic && !modeTest.owned && !helmetOverride && stops==1,"F9 stops mode and disables automatic restart");
    renderHook(component.data(),0.016f); check(geometryWrites==3,"F9 leaves no repeated direct render writes");
    stableFrame(); check(starts==1,"F9 remains disabled across frames");
    check(!sample(oreGeometryHook,g),"geometry override off after F9");
    queueCommand(On,selected,serial); stableFrame(); check(starts==2 && modeTest.owned,"F8 re-enables automatic ore mode");
    queueCommand(Off,selected,serial); frameHook(123);
    queueCommand(On,selected,serial); put(component,0x48,uint16_t{99}); stableFrame();
    check(starts==2,"automatic mode does not replace another active mode");
    put(component,0x48,uint16_t{0xffff}); put(component,0x6c,uint32_t{123}); stableFrame();
    check(starts==2,"wait for original startup effect to finish");
    put(component,0x6c,uint32_t{0xffffffff}); playerPresent=false; stableFrame();
    check(starts==2,"no automatic calls without a live player");
    playerPresent=true; cancelDuringStart=true; oreArm.nextTry=0; frameHook(123); stableFrame();
    check(starts==3 && !oreAutomatic && modeTest.owned && !helmetOverride,"F9 during native activation still suppresses startup and retains cleanup");
    frameHook(123); check(!modeTest.owned && !modeWork,"cancelled automatic start cleaned up");
    cancelDuringStart=false;
    queueCommand(On,selected,serial); stableFrame(); check(starts==4 && modeTest.owned,"fresh request arms after cancelled experiment");
    equipmentResult=1;
    check(equipHook(addr(equipCondition),addr(equipContext),0,0)==0 && modeTest.desired,"absent physical helmet keeps owned mode enabled");
    put(equippedEntry,8,uint16_t{9}); put(equipmentBag,0x10,uint32_t{1});
    equipmentResult=0;
    check(equipHook(addr(equipCondition),addr(equipContext),0,0)==0,"real equipment predicate retains its native satisfied result");
    check(oreAutomatic && !helmetOverride && modeTest.owned && !modeTest.desired,"physical helmet pauses overrides but retains persistent enable state and native cleanup ownership");
    const auto beforeYieldStop=stops;
    frameHook(123); stableFrame();
    check(stops==beforeYieldStop+1 && !modeTest.owned && starts==4,"physical helmet cleans up on native frame without automatic restart");
    const auto nativeBefore=originalEffects;
    startEffectHook(addr(component),&mining);
    check(originalEffects==nativeBefore+1,"original B startup passes through while physical helmet owns control");
    put(equipmentBag,0x10,uint32_t{0}); equipmentResult=1; oreArm.nextTry=0;
    stableFrame(); check(starts==5 && modeTest.owned,"always-on mode resumes automatically after mining helmet removal");
    put(component,0x30,uint16_t{0xffff}); frameHook(123); stableFrame();
    check(modeTest.owned && oreAutomatic && starts==6,"always-on mode rearms after native slot replacement clears");
    queueCommand(Off,selected,serial); frameHook(123);
    queueCommand(On,selected,serial); oreArm.nextTry=0; oreArm.since=GetTickCount64()-2001;
    modeReadiness={addr(player),addr(model),GetTickCount64()-2001,0}; verifiedFrameTicks=50;
    lastGimmickUpdate=0; tickCount=0; frameHook(123);
    check(starts==7 && modeTest.owned && helmetOverride,"verified gameplay frames allow activation even with no gimmick ticks");
    queueCommand(Off,selected,serial); frameHook(123);
    check(!modeTest.owned && !helmetOverride,"F9 clears owned mode even in an area without ticking gimmicks");
    check(equipmentLocks==equipmentUnlocks,"preflight equipment locks balanced");
    check(acquired==released,"automatic actor references balanced");
    std::puts("PASS: owned geometry and final setter override native resets, direct render with no getter/target events, movement, teardown, helmet yield, native B precedence, observer policy and x64 detours");
}
