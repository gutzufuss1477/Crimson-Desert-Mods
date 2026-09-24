// Detours extend the owned mining candidate query/scan, supply live geometry,
// and omit this mod's separate startup sequence. All other modes pass through.
using StartEffectFn=void (__fastcall*)(uintptr_t,const uint16_t*);
using RotateVectorFn=void (__fastcall*)(float*,const float*);
StartEffectFn startEffectOriginal{};
RotateVectorFn rotateVector{};
// MH128.4: user confirmed consistent all-angle glow and original helmet B.
// MH128.5 proved that the upstream 30-unit candidate query was not the final
// visible cutoff.  Use a deliberately larger value in all three remaining
// stages so the next game test can distinguish the material fade boundary.
constexpr float OreAngleDegrees=360.0f,OreRange=250.0f;
using OreScanFn=void (__fastcall*)(uintptr_t,uintptr_t,float,void*);
OreScanFn oreScanOriginal{};
std::atomic<unsigned> oreScanOverrides{0};
std::atomic<unsigned> nativeMiningSequencePasses{0};
std::atomic<unsigned> oreGeometryCalls{0},oreSequenceSkips{0},oreGeometryFailures{0};
std::atomic<unsigned> geometryEntries{0},geometryNativeSuccess{0},geometryNativeFailure{0};
std::atomic<unsigned> geometryContextRejected{0},geometryPlayerRejected{0},sequenceEntries{0};
std::atomic<uint64_t> nextGeometryObservation{0};
std::atomic<unsigned> oreRenderEntries{0},oreRenderApplies{0},oreRenderFailures{0},oreRenderContextRejected{0};
std::atomic<unsigned> oreFinalGeometryEntries{0},oreFinalGeometryOverrides{0};
std::atomic<uint64_t> nextOreRenderLog{0};
bool ourOreContext(uintptr_t component,uint16_t index) {
    return ready.load(std::memory_order_acquire) && oreOnly.load() && helmetOverride.load() &&
        component==oreComponent.load() && index==oreIndex.load() && index!=0xffff;
}
bool ownedOrePlayer(uintptr_t component,uintptr_t player) {
    uint32_t id{},key{}; uint16_t active{}; uint8_t slot{};
    const uint16_t index=oreIndex.load();
    if(diagnosticOnly || !ourOreContext(component,index) || !player || player!=overridePlayer.load() ||
       !read(player+0x60,id) || id!=overridePlayerId.load()) return false;
    const auto info=modeInfo(&index);
    return info && read(info,key) && key==10 && read(info+0x11,slot) && slot<2 &&
        read(component+0x30+slot*0x18,active) && active==index;
}
void __fastcall oreApplyGeometryHook(uintptr_t component,const float* position,const float* look,const float* up,
                                     float angle,float radius,uint32_t hat) {
    oreFinalGeometryEntries.fetch_add(1,std::memory_order_relaxed);
    if(!diagnosticOnly && ourOreContext(component,oreIndex.load())) {
        angle=OreAngleDegrees;
        radius=OreRange;
        hat=1;
        const auto count=oreFinalGeometryOverrides.fetch_add(1,std::memory_order_relaxed)+1;
        if(count==1) log("ore_final_geometry_override","\"angle\":%.6g,\"radius\":%.6g,\"hat\":%u",angle,radius,hat);
    }
    applyGeometry(component,position,look,up,angle,radius,hat);
}
#include "ore_range.hpp"
void __fastcall oreScanHook(uintptr_t component,uintptr_t player,float radius,void* output) {
    uint32_t id{}; uint16_t active{};
    if(!diagnosticOnly && ourOreContext(component,oreIndex.load()) &&
       player==overridePlayer.load() && read(player+0x60,id) && id==overridePlayerId.load()) {
        const uint16_t index=oreIndex.load(); uint8_t slot{}; uint32_t key{};
        const auto ownedInfo=modeInfo(&index);
        if(ownedInfo && read(ownedInfo,key) && key==10 && read(ownedInfo+0x11,slot) && slot<2 &&
           read(component+0x30+slot*0x18,active) && active==index &&
           std::isfinite(radius) && radius>=0 && radius<OreRange) {
            radius=OreRange;
            if(oreScanOverrides.fetch_add(1)==0) log("ore_scan_range","\"radius\":%.6g",radius);
        }
    }
    oreScanOriginal(component,player,radius,output);
}
void __fastcall startEffectHook(uintptr_t component,const uint16_t* indexPtr) {
    const auto entry=sequenceEntries.fetch_add(1);
    if(diagnosticOnly) {
        uint16_t nativeIndex=0xffff;
        if(indexPtr) read(reinterpret_cast<uintptr_t>(indexPtr),nativeIndex);
        if(entry<16) log("native_sequence_observed","\"component\":\"%llx\",\"index\":%u",component,nativeIndex);
        startEffectOriginal(component,indexPtr); return;
    }
    uint16_t index{}; uint8_t slot{}; uint32_t key{};
    if(indexPtr && read(reinterpret_cast<uintptr_t>(indexPtr),index) &&
       (ourOreContext(component,index) || (oreOnly.load() && component==oreStartingComponent && index==oreStartingIndex))) {
        const auto info=modeInfo(&index);
        if(info && read(info,key) && key==10 && read(info+0x11,slot) && slot<2) {
            // Same local slot state as the original empty-sequence-name branch.
            // Do not touch a native sequence left by an original B activation.
            uint32_t id{};
            if(read(component+0x6c+slot*4,id) && id==0xffffffff) {
                storeNative(component+0xf0+slot,uint8_t{0});
                if(oreSequenceSkips.fetch_add(1)==0) log("ore_start_sequence_omitted","\"key\":10");
                return;
            }
        }
    }
    uint16_t nativeIndex{}; uint32_t nativeKey{};
    if(indexPtr && read(reinterpret_cast<uintptr_t>(indexPtr),nativeIndex)) {
        const auto nativeInfo=modeInfo(&nativeIndex);
        if(nativeInfo && read(nativeInfo,nativeKey) && nativeKey==10 && nativeMiningSequencePasses.fetch_add(1)<16)
            log("native_mining_sequence_passthrough","\"component\":\"%llx\",\"index\":%u",component,nativeIndex);
    }
    startEffectOriginal(component,indexPtr);
}
bool unitVector(float* v) {
    const float n=v[0]*v[0]+v[1]*v[1]+v[2]*v[2];
    if(!std::isfinite(n) || n<0.00001f) return false;
    const float scale=1.0f/std::sqrt(n);
    for(unsigned i=0;i<3;++i) v[i]*=scale;
    return true;
}
bool oreGeometry(uintptr_t component,uintptr_t player,DetectGeometry& geometry,const char** failure=nullptr,TiledModelTransform* observedTransform=nullptr) {
    const auto stage=[failure](const char* name) { if(failure) *failure=name; };
    uint32_t id{},key{}; uint16_t index=oreIndex.load(); uint8_t slot{},type{};
    stage("context_or_player_identity");
    if(!ourOreContext(component,index) || player!=overridePlayer.load() ||
       !read(player+0x60,id) || id!=overridePlayerId.load()) return false;
    const auto info=modeInfo(&index);
    uint16_t active{};
    stage("active_mining_slot");
    if(!info || !read(info,key) || key!=10 || !read(info+0x11,slot) || slot>1 ||
       !read(component+0x30+slot*0x18,active) || active!=index ||
       !read(info+0x1c8,type) || type!=1) return false;
    uintptr_t components{},model{},vt{},fn{};
    stage("live_player_model");
    if(!read(player+0x68,components) || !components || !read(components+0x40,model) || !model ||
       !read(model,vt) || !vt || !read(vt+0x178,fn) || !fn) return false;
    // +0x1c is tile-local translation. Signed tile indices at +0x28/+0x2a
    // must be added before passing position to the absolute shader parameters.
    alignas(16) TiledModelTransform transform{};
    reinterpret_cast<TransformFn>(fn)(model,&transform);
    if(observedTransform) *observedTransform=transform;
    std::memcpy(geometry.rotation,transform.rotation,16);
    stage("absolute_world_position");
    if(!transform.worldPosition(geometry.position)) return false;
    stage("mining_geometry_parameters");
    if(!readBytes(reinterpret_cast<void*>(info+0x1ec),geometry.look,12) ||
       !readBytes(reinterpret_cast<void*>(info+0x1e0),geometry.up,12) ||
       !read(info+0x1f8,geometry.angle) || !read(info+0x1fc,geometry.radius)) return false;
    float qnorm=0;
    stage("transform_quaternion_or_radius");
    for(float v:geometry.rotation) qnorm+=v*v;
    if(!std::isfinite(qnorm) || qnorm<0.5f || qnorm>1.5f ||
       !std::isfinite(geometry.radius) || geometry.radius<=0 || geometry.radius>2000 ||
       !std::isfinite(geometry.angle) || geometry.angle<=0) return false;
    rotateVector(geometry.look,geometry.rotation); rotateVector(geometry.up,geometry.rotation);
    geometry.angle=OreAngleDegrees; geometry.radius=OreRange;
    geometry.hat=1; geometry.type=1;
    stage("finite_normalized_geometry");
    return unitVector(geometry.look) && unitVector(geometry.up) && geometry.valid();
}
bool renderOwnedOre(uintptr_t component,uintptr_t player) {
    if(diagnosticOnly) return false;
    if(!ourOreContext(component,oreIndex.load())) return false;
    oreRenderEntries.fetch_add(1);
    DetectGeometry value{}; const char* failure="unknown"; TiledModelTransform transform{};
    if(!oreGeometry(component,player,value,&failure,&transform)) {
        if(oreRenderFailures.fetch_add(1)==0) log("ore_render_refused","\"reason\":\"%s\"",failure);
        return false;
    }
    // A worker may have received F9 or recognized the physical helmet while
    // geometry was being resolved. Do not apply after that cancellation.
    if(!ourOreContext(component,oreIndex.load())) return false;
    applyGeometry(component,value.position,value.look,value.up,value.angle,value.radius,value.hat);
    const auto count=oreRenderApplies.fetch_add(1)+1;
    const auto now=GetTickCount64(); auto due=nextOreRenderLog.load();
    if(count==1 || (now>=due && nextOreRenderLog.compare_exchange_strong(due,now+2000))) {
        log("ore_render_applied","\"count\":%u,\"component\":\"%llx\",\"player\":\"%llx\",\"x\":%.6g,\"y\":%.6g,\"z\":%.6g,\"look_x\":%.6g,\"look_y\":%.6g,\"look_z\":%.6g,\"angle\":%.6g,\"radius\":%.6g,\"hat\":%u",
            count,component,player,value.position[0],value.position[1],value.position[2],value.look[0],value.look[1],value.look[2],value.angle,value.radius,value.hat);
        log("ore_world_position","\"tile_x\":%d,\"tile_z\":%d,\"local_x\":%.9g,\"local_y\":%.9g,\"local_z\":%.9g,\"world_x\":%.9g,\"world_y\":%.9g,\"world_z\":%.9g",
            static_cast<int>(transform.tileX),static_cast<int>(transform.tileZ),transform.localPosition[0],transform.localPosition[1],transform.localPosition[2],value.position[0],value.position[1],value.position[2]);
    }
    return true;
}
bool __fastcall oreGeometryHook(void* self,uintptr_t player,float* position,float* rotation,
                                float* look,float* up,float* angle,float* radius,uint32_t* hat,uint8_t* type) {
    const auto entry=geometryEntries.fetch_add(1)+1;
    const bool result=getGeometry(self,player,position,rotation,look,up,angle,radius,hat,type);
    if(result) geometryNativeSuccess.fetch_add(1); else geometryNativeFailure.fetch_add(1);
    const auto component=reinterpret_cast<uintptr_t>(self);
    const bool contextMatches=ourOreContext(component,oreIndex.load());
    const bool playerMatches=player==overridePlayer.load();
    const auto now=GetTickCount64(); auto due=nextGeometryObservation.load();
    if(entry==1 || (now>=due && nextGeometryObservation.compare_exchange_strong(due,now+2000))) {
        uint16_t first=0xffff,second=0xffff; uint32_t playerId{};
        read(component+0x30,first); read(component+0x48,second); read(player+0x60,playerId);
        float observedAngle{},observedRadius{}; uint32_t observedHat{}; uint8_t observedType{};
        if(result) {
            read(reinterpret_cast<uintptr_t>(angle),observedAngle); read(reinterpret_cast<uintptr_t>(radius),observedRadius);
            read(reinterpret_cast<uintptr_t>(hat),observedHat); read(reinterpret_cast<uintptr_t>(type),observedType);
        }
        log("native_geometry_observed","\"entry\":%u,\"result\":%s,\"component\":\"%llx\",\"player\":\"%llx\",\"player_id\":%u,\"slot0\":%u,\"slot1\":%u,\"context_matches\":%s,\"player_matches\":%s,\"expected_component\":\"%llx\",\"expected_player\":\"%llx\",\"angle\":%.6g,\"radius\":%.6g,\"hat\":%u,\"type\":%u",
            entry,result?"true":"false",component,player,playerId,first,second,contextMatches?"true":"false",playerMatches?"true":"false",
            oreComponent.load(),overridePlayer.load(),observedAngle,observedRadius,observedHat,observedType);
    }
    if(diagnosticOnly) return result;
    if(!contextMatches) { geometryContextRejected.fetch_add(1); return result; }
    if(!playerMatches) { geometryPlayerRejected.fetch_add(1); return result; }
    DetectGeometry value{};
    if(!oreGeometry(component,player,value)) { oreGeometryFailures.fetch_add(1); return result; }
    std::memcpy(position,value.position,12); std::memcpy(rotation,value.rotation,16);
    std::memcpy(look,value.look,12); std::memcpy(up,value.up,12);
    *angle=value.angle; *radius=value.radius; *hat=value.hat; *type=value.type;
    if(oreGeometryCalls.fetch_add(1)==0)
        log("ore_geometry_override","\"x\":%.6g,\"y\":%.6g,\"z\":%.6g,\"angle\":%.6g,\"radius\":%.6g",value.position[0],value.position[1],value.position[2],value.angle,value.radius);
    return true;
}
struct OreArmState { uintptr_t player{}; uint32_t id{}; uint64_t since{},nextTry{}; } oreArm;
#ifdef MH125_TEST_HOST
uint32_t testModeRecordCount=16;
#endif
bool modeRecordCount(uint32_t& count) {
#ifdef MH125_TEST_HOST
    count=testModeRecordCount; return true;
#else
    uintptr_t table{};
    return read(base+0x6d6e3e8,table) && table && read(table+8,count) && count>0 && count<=512;
#endif
}
void armAutomaticOreMode() {
    if(diagnosticOnly) return;
    if(modeWork.load()) return;
    const auto now=GetTickCount64();
    if(now<oreArm.nextTry) return;
    oreArm.nextTry=now+1000;
    uintptr_t manager{};
    if(!actorManager(manager)) { oreArm.player=0; return; }
    ActorLease player; lookupPlayer(manager,&player.ref,0,0);
    uint32_t playerId{};
    if(!player.ref.acquired || !player.ref.actor || !read(player.ref.actor+0x60,playerId) || !playerId) {
        oreArm.player=0; return;
    }
    if(oreArm.player!=player.ref.actor || oreArm.id!=playerId) {
        oreArm.player=player.ref.actor; oreArm.id=playerId; oreArm.since=now; return;
    }
    if(now-oreArm.since<2000) return;
    bool equipped{};
    if(!physicalMiningHelmet(player.ref.actor,equipped) || equipped) return;
    ActorLease owner; modeOwner(player.ref.actor,&owner.ref,0,0);
    uintptr_t component{}; uint32_t ownerId{},count{}; uint16_t active[2]{};
    if(!owner.ref.acquired || !owner.ref.actor || !modeComponent(owner.ref.actor,component,ownerId) ||
       !read(component+0x30,active[0]) || !read(component+0x48,active[1]) ||
       active[0]!=0xffff || active[1]!=0xffff || !nativeModeReady(owner.ref.actor) || !modeRecordCount(count)) return;
    for(uint16_t index=0;index<count;++index) {
        const auto info=modeInfo(&index); uint32_t key{}; uint8_t type{},slot{}; uint32_t sequence{};
        if(!info || !read(info,key) || key!=10 || !read(info+0x1c8,type) || type!=1 ||
           !read(info+0x11,slot) || slot>1 || !read(component+0x6c+slot*4,sequence) || sequence!=0xffffffff) continue;
        AcquireSRWLockExclusive(&stateLock);
        if(oreAutomatic.load() && !modeTest.owned && !modeTest.desired) {
            modeTest.baseline={component,owner.ref.actor,player.ref.actor,ownerId,playerId,key,index,slot,now};
            modeTest.captured=true; modeTest.desired=true; ++modeTest.serial; modeWork=true;
            log("ore_automatic_armed","\"key\":%u,\"index\":%u,\"player\":%u",key,index,playerId);
        }
        ReleaseSRWLockExclusive(&stateLock);
        return;
    }
}
bool queueOreCommand(uint32_t eventId,unsigned& selected,unsigned& serial) {
    if(diagnosticOnly) { selected=0; serial=0; return false; }
    if(!oreOnly.load()) return queueModeCommand(eventId,selected,serial);
    oreAutomatic.store(eventId==On);
    cleanupAt=0;
    if(eventId==Off) return queueModeCommand(Off,selected,serial);
    AcquireSRWLockShared(&stateLock);
    serial=modeTest.serial; selected=1;
    ReleaseSRWLockShared(&stateLock);
    log("ore_automatic_enabled"); return true;
}
