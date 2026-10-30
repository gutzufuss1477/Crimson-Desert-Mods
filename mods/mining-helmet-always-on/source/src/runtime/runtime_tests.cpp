#define AUTO34_TEST_HOST
#include "mining_helmet.cpp"
#include <unordered_map>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <limits>
namespace {
std::unordered_map<uintptr_t,uint8_t> memory;
struct Written { uintptr_t address; size_t size; };
std::vector<Written> writes;
void reserve(uintptr_t p,size_t n) { for(size_t i=0;i<n;++i) memory[p+i]=0; }
bool rd(uintptr_t p,void* v,size_t n) {
    auto* out=static_cast<uint8_t*>(v);
    for(size_t i=0;i<n;++i) { const auto it=memory.find(p+i); if(it==memory.end()) return false; out[i]=it->second; }
    return true;
}
bool wr(uintptr_t p,const void* v,size_t n) {
    for(size_t i=0;i<n;++i) if(!memory.contains(p+i)) return false;
    const auto* in=static_cast<const uint8_t*>(v);
    for(size_t i=0;i<n;++i) memory[p+i]=in[i];
    writes.push_back({p,n}); return true;
}
template<class T> void set(uintptr_t p,T v) { reserve(p,sizeof(v)); wr(p,&v,sizeof(v)); }
template<class T> T get(uintptr_t p) { T v{}; assert(rd(p,&v,sizeof(v))); return v; }
constexpr uintptr_t Image=0x140000000,Root=0x10000,Manager=0x20000,Pool=0x30000,Names=0x40000,
    Groups=0x50000,Group=0x60000,Buffer=0x70000;
void map(uintptr_t p,uintptr_t buckets,uintptr_t nodes,uint32_t n) {
    mh34::Map m{1,n,n,0,buckets,nodes}; reserve(p,sizeof(m)); wr(p,&m,sizeof(m)); reserve(buckets,256); reserve(nodes,n*8);
    set(buckets,n);
}
void name(uint32_t id,const char* text,uintptr_t textAddress) {
    const auto n=std::strlen(text)+1;
    reserve(textAddress,n); wr(textAddress,text,n);
    reserve(Names+id*16,16); set(Names+id*16,textAddress); set(Names+id*16+12,static_cast<uint16_t>(n));
}
void node(uintptr_t nodes,uintptr_t bucket,uint32_t index,uintptr_t nodeAddress,uint32_t id,uintptr_t object,uint16_t word=0) {
    set(nodes+index*8,nodeAddress); set(bucket+8+index*8,uint32_t{123}); set(bucket+12+index*8,index);
    reserve(nodeAddress,24); set(nodeAddress+4,id); set(nodeAddress+8,object); set(nodeAddress+16,word);
}
mh34::View fixture() {
    memory.clear(); writes.clear();
    set(Root+0x20,Manager); set(Manager+0x1280,uintptr_t{0x21000}); set(0x21000+0x160,uintptr_t{0x22000});
    set(0x22000+0x560,Groups); set(Image+0x70cac60,Pool); set(Pool+0x58,Names);
    map(Groups,0x51000,0x52000,1); name(1,"GlobalMaterialGlobalParameter_Common",0x53000);
    node(0x52000,0x51000,0,0x54000,1,Group);
    reserve(Group,0x100); reserve(Buffer,484); set(Group+0x78,Buffer); set(Group+0x80,uint32_t{484});
    map(Group+0x58,0x61000,0x62000,8);
    const uint16_t offsets[]={36,32,28,43,44,45,17,20};
    for(uint32_t i=0;i<8;++i) {
        const auto prop=uintptr_t{0x90000}+i*0x100,nodeAddress=uintptr_t{0x80000}+i*0x100;
        name(i+2,mh34::Specs[i].name,0xa0000+i*0x100);
        node(0x62000,0x61000,i,nodeAddress,i+2,prop,offsets[i]);
        reserve(prop,0x80); set(prop,Image+mh34::Specs[i].vtable);
    }
    set(0x90730,uint32_t{255}); set(Buffer+80,uint32_t{255});
    mh34::View view{}; assert(mh34::resolve(rd,Root,Image,view)); writes.clear(); return view;
}
void materials() {
    auto view=fixture();
    const float p[3]={-10605,587,-3939},look[3]={0,0,1},up[3]={0,1,0};
    auto data=mh34::values(p,look,up); mh34::Ownership owner;
    assert(owner.apply(rd,wr,view,data));
    assert(get<float>(Buffer+176)==30 && get<uint32_t>(Buffer+80)==0 && get<float>(Buffer+68)==1);
    // All writes are limited to the proved property/puffer pair and dirty byte.
    for(const auto& w:writes) {
        bool permitted=w.address==Group+0x90 && w.size==1;
        for(size_t i=0;i<8;++i) permitted|=(w.address==view.fields[i].object+0x30 || w.address==view.buffer+view.fields[i].offset) && w.size==mh34::Specs[i].bytes;
        assert(permitted);
    }
    const float moved[3]={-10000,600,-3500}; data=mh34::values(moved,look,up);
    assert(owner.apply(rd,wr,view,data)); assert(get<float>(Buffer+144)==moved[0]);
    assert(owner.release(rd,wr)); assert(get<float>(Buffer+144)==0 && get<uint32_t>(Buffer+80)==255);

    view=fixture(); assert(owner.apply(rd,wr,view,data));
    set(Buffer+80,uint32_t{7}); set(view.fields[7].object+0x30,uint32_t{7});
    assert(owner.release(rd,wr)); assert(get<uint32_t>(Buffer+80)==7); // Preserve a foreign writer.

    view=fixture(); set(view.fields[7].object+0x30,uint32_t{0}); writes.clear();
    assert(!owner.apply(rd,wr,view,data) && writes.empty()); // Native material owner wins.

    view=fixture(); assert(owner.apply(rd,wr,view,data));
    set(view.fields[0].entry,uintptr_t{0xdeadbeef}); writes.clear();
    assert(!owner.release(rd,wr) && writes.empty()); // No writes to recycled property objects.

    view=fixture(); auto invalid=data; mh34::put(invalid[3],std::numeric_limits<float>::quiet_NaN()); writes.clear();
    assert(!owner.apply(rd,wr,view,invalid) && writes.empty());
    set(0x61000,uint32_t{32}); mh34::View refused{}; assert(!mh34::resolve(rd,Root,Image,refused));
}
alignas(16) std::array<uint8_t,0x100> actor{};
alignas(16) std::array<uint8_t,0x200> parts{};
alignas(16) std::array<uint8_t,0x700> generic{};
alignas(16) std::array<uint8_t,0x100> mode{};
unsigned releases{},enqueues{},effects{};
std::array<uint8_t,0xe8> sent{};
template<class T,size_t N> void field(std::array<uint8_t,N>& a,size_t off,T v) { std::memcpy(a.data()+off,&v,sizeof(v)); }
auto34::Ref* __fastcall lookup(uintptr_t,auto34::Ref* ref,uint32_t id,uintptr_t) {
    if(id==99 || id==111) { ref->actor=reinterpret_cast<uintptr_t>(actor.data()); ref->acquired=1; } return ref;
}
auto34::Ref* __fastcall player(uintptr_t,auto34::Ref* ref,uintptr_t,uintptr_t) { return lookup(0,ref,99,0); }
void __fastcall release(auto34::Ref* r) { ++releases; r->acquired=0; }
void* __fastcall init(void* p) { std::memset(p,0,0xb8); return p; }
uint32_t* __fastcall enqueue(void*,uint32_t* result,const void* e,uintptr_t) {
    ++enqueues; std::memcpy(sent.data(),e,sent.size()); *result=0; return result;
}
uint32_t* __fastcall effect(void*,uint32_t* result,const void*,const void*) { ++effects; *result=42; return result; }
auto34::Identity setupNative() {
    using namespace auto34;
    base=Image; ready=true; releases=enqueues=effects=0; targets={};
    actor={}; parts={}; generic={}; mode={};
    field(actor,0x60,uint32_t{99}); field(actor,0x90,uint32_t{111}); field(actor,0x68,reinterpret_cast<uintptr_t>(parts.data()));
    field(parts,0x30,reinterpret_cast<uintptr_t>(generic.data())); field(parts,0x178,reinterpret_cast<uintptr_t>(mode.data()));
    field(generic,0,Image+0x5938938); field(generic,8,reinterpret_cast<uintptr_t>(actor.data()));
    field(mode,0,Image+0x590cf78); field(mode,0x30,uint16_t{0xffff}); field(mode,0x48,uint16_t{0xffff});
    lookupActor=lookup; lookupPlayer=player; modeOwner=player; releaseRef=release; eventInit=init; enqueueOriginal=enqueue; effectOriginal=effect;
    Identity id{}; assert(identify(reinterpret_cast<uintptr_t>(generic.data()),id)); return id;
}
void lifecycle() {
    using namespace auto34;
    auto id=setupNative(); const auto originalMode=mode;
    assert(nativeInactive(id.actor)); field(mode,0x30,uint16_t{12}); assert(!nativeInactive(id.actor)); mode=originalMode;
    Target target{}; target.who=id; target.source=111; target.ours=true;
    assert(send(0,target,On));
    assert(value<uint32_t>(reinterpret_cast<uintptr_t>(sent.data()))==On);
    assert(value<uint32_t>(reinterpret_cast<uintptr_t>(sent.data()+0x10))==111);
    assert(value<uint32_t>(reinterpret_cast<uintptr_t>(sent.data()+0x14))==111);
    assert(value<uint64_t>(reinterpret_cast<uintptr_t>(sent.data()+0x18))==123456);
    for(size_t i=0x20;i<sent.size();++i) assert(sent[i]==0); // No borrowed event pointers.
    auto invalid=target; invalid.who.actor+=16; const auto calls=enqueues;
    assert(!send(0,invalid,On) && enqueues==calls); assert(mode==originalMode);
    targets[0]=target; clearTargets(0x1234,true); assert(enqueues==calls+1);
    assert(value<uint32_t>(reinterpret_cast<uintptr_t>(sent.data()))==Off);
    assert(value<uint32_t>(reinterpret_cast<uintptr_t>(sent.data()+0x10))==111); // Recorded source survives character changes.
    targets[0]=target; clearTargets(0x1234,false); assert(enqueues==calls+1); // Yield does not send native OFF.
    targets[0]=target;
    alignas(16) std::array<uint8_t,0xe0> handler{}; std::array<uint8_t,0xe8> event{};
    field(handler,0,Image+0x5d27610); field(handler,0x6c,MineIndex); field(event,0,On);
    uint32_t result{}; assert(effectHook(generic.data(),&result,handler.data(),event.data())==&result);
    assert(result==42 && effects==1 && targets[0].mine);
    field(event,0,Off); targets[0].dispatched=true;
    effectHook(generic.data(),&result,handler.data(),event.data()); assert(!targets[0].ours && !targets[0].dispatched && effects==2);
    assert(mode==originalMode && releases==5);
}
unsigned eyeCalls{},orientationCalls{},rotationCalls{};
std::array<uint8_t,0x220> modeRecord{};
uintptr_t __fastcall modeLookup(uintptr_t unused,uint32_t index) {
    assert(unused==0 && index==13); // New lookup ABI: EDX value, not RCX pointer.
    return reinterpret_cast<uintptr_t>(modeRecord.data());
}
void currentModeLayout() {
    using namespace auto34;
    modeInfo=modeLookup; modeRecord={};
    field(modeRecord,8,uint32_t{10});
    field(modeRecord,0x1d0,uint8_t{1}); field(modeRecord,0x208,uint8_t{1});
    field(modeRecord,0x1ec,1.0f); // Up Y, at vector base 0x1e8.
    field(modeRecord,0x1fc,1.0f); // Forward Z, at vector base 0x1f4.
    float look[3]{},up[3]{};
    assert(modeAxes(look,up) && look[0]==0 && look[1]==0 && look[2]==1);
    assert(up[0]==0 && up[1]==1 && up[2]==0);
    const auto good=modeRecord;
    for(const size_t offset:{size_t{8},size_t{0x1d0},size_t{0x208}}) {
        modeRecord=good; modeRecord[offset]=0; assert(!modeAxes(look,up));
    }
    modeRecord={}; field(modeRecord,0,uint32_t{10});
    field(modeRecord,0x1c8,uint8_t{1}); field(modeRecord,0x200,uint8_t{1});
    assert(!modeAxes(look,up)); // Old layout must never pass the identity checks.
}
float expectedDirection[3]={1,0,0};
float* __fastcall eye(uintptr_t player,float* p) {
    assert(player==99); ++eyeCalls;
    p[0]=123; p[1]=456; p[2]=-789; return p;
}
void __fastcall orient(float* q,const float* direction,float* worldUp,int a,int b,int c) {
    ++orientationCalls;
    assert(a==3 && b==2 && c==-1);
    for(unsigned i=0;i<3;++i) assert(direction[i]==expectedDirection[i]);
    assert(worldUp[0]==0 && worldUp[1]==1 && worldUp[2]==0);
    q[0]=q[1]=q[2]=0; q[3]=1;
}
void __fastcall rotate(float* v,const float* q) {
    ++rotationCalls; assert(q[3]==1);
    for(unsigned i=0;i<3;++i) v[i]*=2;
}
void camera() {
    using namespace auto34;
    eyePosition=eye; orientation=orient; rotateVector=rotate;
    float direction[4]={2,0,0,0},worldUp[4]={0,2,0,0},p[3]{},look[3]={0,0,1},up[3]={0,1,0};
    assert(cameraGeometry(99,direction,worldUp,p,look,up));
    assert(p[0]==123 && p[1]==456 && p[2]==-789 && look[2]==1 && up[1]==1);
    // Camera turn, independent of character/model facing; fresh data each call.
    direction[0]=expectedDirection[0]=0; direction[2]=expectedDirection[2]=-1;
    assert(cameraGeometry(99,direction,worldUp,p,look,up));
    assert(eyeCalls==2 && orientationCalls==2 && rotationCalls==4);
    direction[0]=std::numeric_limits<float>::quiet_NaN();
    assert(!cameraGeometry(99,direction,worldUp,p,look,up) && eyeCalls==2);
}
void targetsLifecycle() {
    using namespace auto34;
    auto id=setupNative();
    targets[0].who=id; targets[0].source=111; targets[0].seen=10000;
    updateTargets(0x1234,10000); assert(enqueues==1 && targets[0].ours);
    targets[0].seen=13000; updateTargets(0x1234,13000);
    assert(enqueues==1); // Unknown object is visited once, not retriggered each frame.
    targets[0].mine=true; updateTargets(0x1234,13001);
    assert(enqueues==2); // Confirmed mining effect can be reacquired if absent.
    alignas(16) std::array<uint8_t,0xa0> component{};
    alignas(16) std::array<uint8_t,0x250> record{};
    alignas(16) std::array<uint8_t,0x18> refs{},reference{},effectObject{};
    field(parts,0x60,reinterpret_cast<uintptr_t>(component.data()));
    field(component,8,id.actor); field(component,0x90,reinterpret_cast<uintptr_t>(record.data())); field(component,0x98,uint32_t{1});
    field(record,0,MineKey); field(record,0x1f8,reinterpret_cast<uintptr_t>(refs.data())); field(record,0x200,uint32_t{1});
    field(refs,0x10,reinterpret_cast<uintptr_t>(reference.data())); field(reference,8,reinterpret_cast<uintptr_t>(effectObject.data()));
    targets[0].seen=16000; updateTargets(0x1234,16000);
    assert(enqueues==2); // A live effect is left intact, preventing timer resets/pulsing.
    updateTargets(0x1234,19001);
    assert(enqueues==2 && targets[0].who.id && targets[0].held); // Still alive just outside scan.
    assert(heldMiningEffects==1);
    updateTargets(0x1234,16000+MiningReentryGraceMs+1);
    assert(enqueues==3 && !targets[0].who.id && value<uint32_t>(reinterpret_cast<uintptr_t>(sent.data()))==Off);
}
void reentryRetention() {
    using namespace auto34;
    const auto id=setupNative();
    heldMiningEffects=0; miningReentries=0; heldExpirations=0;
    targets[0].who=id; targets[0].source=111; targets[0].seen=10000;
    targets[0].mine=true; targets[0].ours=true; targets[0].dispatched=true;
    targets[0].retry=500000; enqueues=0;
    updateTargets(0x1234,13000);
    assert(enqueues==0 && targets[0].held && heldMiningEffects==1);
    markTargetSeen(targets[0],13001,75.0f*75.0f);
    assert(!targets[0].held && targets[0].seen==13001 && miningReentries==1);
    updateTargets(0x1234,13001);
    assert(enqueues==0 && heldMiningEffects==0);
    updateTargets(0x1234,16002);
    assert(targets[0].held && enqueues==0);
    updateTargets(0x1234,13001+MiningReentryGraceMs+1);
    assert(!targets[0].who.id && enqueues==1 && heldExpirations==1);

    // Unknown/non-mining candidates still expire at the original 2 seconds.
    const auto genericIdentity=setupNative(); enqueues=0;
    targets[0].who=genericIdentity; targets[0].source=111; targets[0].seen=10000;
    targets[0].mine=false; targets[0].ours=true;
    updateTargets(0x1234,12001);
    assert(enqueues==1 && !targets[0].who.id);
}
void progressiveActivation() {
    using namespace auto34;
    auto id=setupNative();
    targets[0].who=id;
    targets[0].source=111;
    targets[0].seen=10000;
    targets[0].distance2=100.0f*100.0f;
    approachingRetries=0; lateMiningIdentifications=0;
    updateTargets(0x1234,10000);
    assert(enqueues==1 && targets[0].probes==1);
    assert(targets[0].lastProbedDistance2==10000.0f);
    targets[0].seen=13000;
    targets[0].distance2=90.0f*90.0f;
    updateTargets(0x1234,13000);
    assert(enqueues==1 && approachingRetries==0); // Movement too small.
    targets[0].seen=16000;
    targets[0].distance2=70.0f*70.0f;
    updateTargets(0x1234,16000);
    assert(enqueues==2 && approachingRetries==1 && targets[0].probes==2);
    targets[0].seen=19000;
    targets[0].distance2=49.0f*49.0f;
    updateTargets(0x1234,19000);
    assert(enqueues==3 && approachingRetries==2 && targets[0].probes==3);
    alignas(16) std::array<uint8_t,0xe0> handler{};
    std::array<uint8_t,0xe8> event{};
    field(handler,0,Image+0x5d27610);
    field(handler,0x6c,MineIndex);
    field(event,0,On);
    uint32_t result{};
    effectHook(generic.data(),&result,handler.data(),event.data());
    assert(targets[0].mine && lateMiningIdentifications==1);
    const auto before=enqueues;
    targets[0].seen=19001; targets[0].distance2=20.0f*20.0f;
    updateTargets(0x1234,19001);
    assert(enqueues==before); // No pointless retry on a confirmed live/retiring effect.
    // A non-mining actor cannot receive unbounded ON attempts.
    id=setupNative();
    targets[0].who=id; targets[0].source=111; targets[0].seen=10000;
    targets[0].dispatched=true; targets[0].probes=MaxUnconfirmedProbes;
    targets[0].distance2=1; targets[0].lastProbedDistance2=10000;
    enqueues=0;
    updateTargets(0x1234,10000);
    assert(enqueues==0);
}
std::vector<float> queryRadii;
bool nestedQuery{};
void __fastcall query(uintptr_t manager,const float* position,float actorRadius,float radius,void* output) {
    assert(manager==123 || manager==456);
    assert(position[0]==10 && actorRadius==2.5f && output==reinterpret_cast<void*>(789));
    queryRadii.push_back(radius);
    if(nestedQuery && queryRadii.size()==1) auto34::queryHook(manager,position,actorRadius,30,output);
}
void ranges() {
    using namespace auto34;
    assert(configure("native") && visualRange==20 && scanRange==30 && !radialPath);
    assert(configure("visual",90) && visualRange==90 && scanRange==30);
    assert(configure("scan",60) && visualRange==90 && scanRange==60);
    assert(!configure("range",121) && visualRange==90 && scanRange==60);
    assert(!configure("range",std::numeric_limits<float>::quiet_NaN()));
    assert(!configure("scan",0) && !configure("visual",-1));
    assert(configure("range",90) && visualRange==90 && scanRange==90);
    ready=true; queryOriginal=query; queryRadii.clear(); scopedQueries=0; extendedQueries=0;
    const float position[3]={10,20,30};
    queryHook(123,position,2.5f,30,reinterpret_cast<void*>(789));
    assert(queryRadii.back()==30 && scopedQueries==0); // Native unscoped calls unchanged.
    {
        QueryScope scope({123,90});
        queryHook(456,position,2.5f,30,reinterpret_cast<void*>(789));
        assert(queryRadii.back()==30 && scopedQueries==0); // Unrelated manager unchanged.
        queryHook(123,position,2.5f,30,reinterpret_cast<void*>(789));
        assert(queryRadii.back()==90 && extendedQueries==1);
        queryHook(123,position,2.5f,100,reinterpret_cast<void*>(789));
        assert(queryRadii.back()==100 && extendedQueries==1); // Never reduce a caller's larger range.
        queryRadii.clear(); nestedQuery=true;
        queryHook(123,position,2.5f,30,reinterpret_cast<void*>(789));
        assert(queryRadii.size()==2 && queryRadii[0]==90 && queryRadii[1]==30);
        assert(queryContext.manager==123); // Outer context restored after nested call.
    }
    assert(queryContext.manager==0); nestedQuery=false;
    const auto view=fixture(); mh34::Ownership material;
    const float look[3]={0,0,1},up[3]={0,1,0};
    assert(material.apply(rd,wr,view,mh34::values(position,look,up,30,90)));
    assert(get<float>(view.fields[4].object+0x30)==90 && get<float>(Buffer+176)==90);
    assert(get<float>(Buffer+172)==30); // Cone angle remains native.
    assert(material.apply(rd,wr,view,mh34::values(position,look,up,30,30)));
    assert(get<float>(Buffer+176)==30); // A/B toggle applies without mode or effect resets.
    assert(material.release(rd,wr));
}
}
// Independent transcription of the verified DXIL distance branch, including
// the cone denominator floor. Test output rather than just chosen constants.
float shaderVisibility(float x,float y,float z,float angle,float radius,uint32_t hat,float highlight) {
    const auto saturate=[](float f) { return (f<0)?0.0f:((f>1)?1.0f:f); };
    const float distance=std::sqrt(x*x+y*y+z*z);
    if(hat) return highlight*(std::min)(1-saturate(distance*0.05f),0.5f);
    const float perpendicular=std::sqrt(x*x+y*y);
    const float coneRadius=std::tan(angle*0.01745329238474369f)*z;
    const float cone=1-saturate(perpendicular/(std::max)(0.001f,coneRadius));
    const float contribution=cone*(1-saturate(distance/(radius*0.7f)));
    return highlight*saturate(contribution+1-saturate(distance/(radius*0.1f)));
}
void adjustableShader() {
    const float origin[3]{},look[3]={0,0,1},up[3]={0,1,0};
    const auto view=fixture(); mh34::Ownership ownership;
    assert(ownership.apply(rd,wr,view,range36::values(origin,look,up,60)));
    const float angle=get<float>(Buffer+172),radius=get<float>(Buffer+176),highlight=get<float>(Buffer+68);
    const uint32_t hat=get<uint32_t>(Buffer+180);
    assert(radius==600 && angle==0 && hat==0 && highlight==0.5f);
    for(const float z:{-24.0f,0.0f,24.0f}) {
        const float x=std::sqrt(30*30-z*z);
        assert(shaderVisibility(x,0,z,30,30,1,1)==0); // Existing native cutoff.
        assert(std::fabs(shaderVisibility(x,0,z,angle,radius,hat,highlight)-0.25f)<0.00001f);
        assert(shaderVisibility(x*2,0,z*2,angle,radius,hat,highlight)==0); // Requested cutoff.
    }
    // Live A/B returns to the original, fixed-distance shader branch.
    assert(ownership.apply(rd,wr,view,range36::values(origin,look,up,60,false)));
    assert(get<uint32_t>(Buffer+180)==1 && get<float>(Buffer+176)==30 && get<float>(Buffer+68)==1);
    assert(ownership.apply(rd,wr,view,range36::values(origin,look,up,90)));
    assert(get<float>(Buffer+176)==900 && get<uint32_t>(Buffer+180)==0);
    assert(ownership.apply(rd,wr,view,range36::values(origin,look,up,100,true,1.0f)));
    assert(get<float>(Buffer+68)==1.0f);
    assert(shaderVisibility(80,0,0,0,1000,0,1.0f)>shaderVisibility(80,0,0,0,1000,0,0.5f));
    assert(ownership.apply(rd,wr,view,range36::values(origin,look,up,120)));
    assert(get<float>(Buffer+176)==1200 && get<uint32_t>(Buffer+180)==0);
    assert(shaderVisibility(100,0,0,0,1200,0,0.5f)>0.08f);
    assert(shaderVisibility(121,0,0,0,1200,0,0.5f)==0);

    assert(ownership.release(rd,wr));
    assert(get<float>(Buffer+176)==0 && get<uint32_t>(Buffer+80)==255);
}
void iniParsing() {
    using ini37::Result;
    float value=73,strength=3;
    assert(ini37::parse("[MiningHelmetAlwaysOn]\nRange=100\nGlowStrength=1.0",value,&strength)==Result::Applied);
    assert(value==100 && strength==1.0f);
    for(const auto* bad:{"1.5","0.49","not-a-number","nan"}) {
        strength=0.8f; value=83;
        auto input=std::string("[MiningHelmetAlwaysOn]\nRange=80\nGlowStrength=")+bad;
        assert(ini37::parse(input,value,&strength)!=Result::Applied && value==83 && strength==0.8f);
    }
    strength=3; value=45;
    assert(ini37::parse("[MiningHelmetAlwaysOn]\nRange=55",value,&strength)==Result::Applied && strength==0.5f);
    assert(ini37::parse("[MiningHelmetAlwaysOn]\nRange=55\nGlowStrength=0.7\nGlowStrength=0.8",value,&strength)==Result::DuplicateGlowStrength);
    for(const auto& text:{"[MiningHelmetAlwaysOn]\nRange=90", "\xef\xbb\xbf[MININGHELMETALWAYSON]\r\n range = 90 ; further away\r\n",
        "[Unrelated]\nRange=1\n[MiningHelmetAlwaysOn]\nUnknown=123\nRange=90\n[Other]\nRange=500"}) {
        assert(ini37::parse(text,value)==Result::Applied && value==90);
    }
    assert(ini37::parse("[MiningHelmetAlwaysOn]\nRange=75.5 # comment",value)==Result::Applied && value==75.5f);
    assert(ini37::parse("[MiningHelmetAlwaysOn]\nRange=10",value)==Result::Applied && value==10);
    assert(ini37::parse("[MiningHelmetAlwaysOn]\nRange=120",value)==Result::Applied && value==120);
    for(const auto& text:{"", "[Other]\nRange=60", "[MiningHelmetAlwaysOn]\n", "[MiningHelmetAlwaysOn\nRange=60",
        "[MiningHelmetAlwaysOn]\nRange=60\nRange=90", "[MiningHelmetAlwaysOn]\nRange=60\n[MiningHelmetAlwaysOn]\nRange=90"}) {
        value=73; assert(ini37::parse(text,value)!=Result::Applied && value==73);
    }
    for(const auto& invalid:{"0","-1","9.99","120.1","nan","inf","1e999","90foo","75,5","","90 40"}) {
        value=73;
        assert(ini37::parse(std::string("[MiningHelmetAlwaysOn]\nRange=")+invalid,value)!=Result::Applied && value==73);
    }
    std::string utf16="\xff\xfe";
    for(const char c:std::string("[MiningHelmetAlwaysOn]\r\nRange=80.5")) { utf16.push_back(c); utf16.push_back('\0'); }
    assert(ini37::parse(utf16,value)==Result::Applied && value==80.5f);
    utf16.push_back('a'); assert(ini37::parse(utf16,value)==Result::EncodingError && value==80.5f);
    assert(ini37::parse(std::string(65537,' '),value)==Result::TooLarge);
}
void iniFiles() {
    using ini37::Result;
    wchar_t temporary[MAX_PATH]{}; assert(GetTempPathW(MAX_PATH,temporary));
    const auto directory=std::wstring(temporary)+L"mining-ini-\u00e4-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
    assert(CreateDirectoryW(directory.c_str(),nullptr));
    ini37::Watcher watcher; watcher.path=directory+L"\\"+ini37::Filename;
    assert(watcher.poll()==Result::Missing && watcher.range==60);
    const auto save=[&](const char* contents) {
        HANDLE f=CreateFileW(watcher.path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
        assert(f!=INVALID_HANDLE_VALUE); DWORD written{};
        const auto length=static_cast<DWORD>(std::strlen(contents));
        assert(WriteFile(f,contents,length,&written,nullptr) && written==length); assert(CloseHandle(f));
    };
    save("[MiningHelmetAlwaysOn]\nRange=60");
    assert(watcher.poll()==Result::Applied && watcher.range==60);
    assert(watcher.poll()==Result::Unchanged && watcher.reloads==1);
    // Real-file reloads include same-length saves, missing/partial values,
    // file deletion and re-creation; invalid input must not reset settings.
    save("[MiningHelmetAlwaysOn]\nRange=90\nGlowStrength=1.0");
    assert(watcher.poll()==Result::Applied && watcher.range==90 && watcher.strength==1.0f && watcher.reloads==2);
    auto34::ini={}; auto34::ini.path=watcher.path;
    auto34::reloadIni();
    assert(auto34::visualRange==90 && auto34::scanRange==90 && auto34::radialPath && auto34::glowStrength==1.0f);
    save("[MiningHelmetAlwaysOn]\nRange=");
    assert(watcher.poll()==Result::InvalidRange && watcher.range==90);
    auto34::reloadIni(); assert(auto34::visualRange==90 && auto34::scanRange==90);
    assert(watcher.poll()==Result::Unchanged && watcher.status==Result::InvalidRange);
    assert(DeleteFileW(watcher.path.c_str()));
    assert(watcher.poll()==Result::Missing && watcher.range==90);
    save("[MiningHelmetAlwaysOn]\nRange=45");
    assert(watcher.poll()==Result::Applied && watcher.range==45 && watcher.strength==0.5f && watcher.reloads==3);
    auto34::reloadIni(); assert(auto34::visualRange==45 && auto34::scanRange==45);
    auto34::configure("native"); auto34::reloadIni();
    assert(!auto34::radialPath); // An unchanged INI does not fight a diagnostic A/B override.
    assert(DeleteFileW(watcher.path.c_str())); assert(RemoveDirectoryW(directory.c_str()));
    ini37::Watcher besideModule; assert(besideModule.fromModule(GetModuleHandleW(nullptr)));
    assert(besideModule.path.ends_with(ini37::Filename));
    // The INI controls both search and fade; low cutoffs retain the native
    // minimum search without shrinking unrelated game queries.
    assert(auto34::configure("range",10) && auto34::visualRange==10 && auto34::scanRange==30 && auto34::radialPath);
    assert(auto34::configure("range",120) && auto34::visualRange==120 && auto34::scanRange==120);
}
void lanternYieldRecovery() {
    using namespace auto34;
    const auto view=fixture();
    const auto identity=setupNative();
    materialOwnership={}; nativeYielded=false; owned=0;
    const float p[3]{1,2,3},look[3]{0,0,1},up[3]{0,1,0};
    const auto desired=range36::values(p,look,up,60);
    assert(materialOwnership.apply(rd,wr,view,desired));
    assert(get<uint32_t>(Buffer+80)==0 && get<float>(Buffer+68)==0.5f && get<float>(Buffer+176)==600.0f);

    targets[0].who=identity;
    targets[0].source=111;
    targets[0].ours=true;
    owned=1;
    enqueues=0;
    const auto written=writes.size();
    suspend(0x1234,true,rd,wr);
    assert(nativeYielded.load() && materialOwnership.owned);
    assert(writes.size()==written); // No native material writes during handoff.
    assert(enqueues==0 && targets[0].who.id==0); // Native OFF is not overridden.
    assert(get<uint32_t>(Buffer+80)==0); // The old mod value may remain stuck.
    suspend(0x1234,false,rd,wr);
    assert(!nativeYielded.load() && !materialOwnership.owned);
    assert(get<uint32_t>(Buffer+80)==255 && get<float>(Buffer+68)==0.0f);
    assert(get<uint32_t>(view.fields[7].object+0x30)==255);
    assert(materialOwnership.apply(rd,wr,view,desired)); // Glow reacquired without restart.

    // A lantern-owned material value must not be overwritten by our cleanup.
    suspend(0x1234,true,rd,wr);
    set(Buffer+80,uint32_t{103});
    set(view.fields[7].object+0x30,uint32_t{103});
    suspend(0x1234,false,rd,wr);
    assert(get<uint32_t>(Buffer+80)==103);
    assert(get<uint32_t>(view.fields[7].object+0x30)==103);
    assert(!materialOwnership.apply(rd,wr,view,desired)); // Foreign mode wins.
    set(Buffer+80,uint32_t{255});
    set(view.fields[7].object+0x30,uint32_t{255});
    assert(materialOwnership.apply(rd,wr,view,desired));
    suspend(0x1234,false,rd,wr);
}

uint8_t groupNativeResult=1;
unsigned groupNativeCalls{};
uint8_t __fastcall groupNativeRead(void*,void*) { ++groupNativeCalls;return groupNativeResult; }
void groupRange() {
    using namespace ore_range;
    original=groupNativeRead;changed=0;refused=0;
    std::array<uint8_t,0x1d8> record{};
    const char* label="Item_Collect_Ore";
    field(record,8,uint32_t{1708});field(record,0,reinterpret_cast<uintptr_t>(&label));
    field(record,LevelOffset,int32_t{5});
    const auto before=record;
    assert(onRead(nullptr,record.data())==1 && groupNativeCalls==1 && changed==1);
    int32_t level{};std::memcpy(&level,record.data()+LevelOffset,4);assert(level==7);
    for(size_t i=0;i<record.size();++i)if(i<LevelOffset || i>=LevelOffset+4)assert(record[i]==before[i]);
    for(uint32_t key:{0u,1707u,1709u,1000632u,1711u,1000816u,0xffffffffu}) {
        record=before;field(record,8,key);const auto saved=record;
        onRead(nullptr,record.data());assert(record==saved);
    }
    for(int32_t prior:{-1,0,4,6,7,8,31,100}) {
        record=before;field(record,LevelOffset,prior);const auto saved=record;
        onRead(nullptr,record.data());assert(record==saved);
    }
    for(uint8_t flag:{uint8_t{1},uint8_t{2},uint8_t{255}}) {
        record=before;record[DefaultOffset]=flag;const auto saved=record;
        onRead(nullptr,record.data());assert(record==saved);
    }
    record=before;label="Item_Collect_Ore_Drop";
    onRead(nullptr,record.data());assert(record==before);
    label="Item_Collect_Ore";groupNativeResult=0;
    assert(onRead(nullptr,record.data())==0 && record==before);
    groupNativeResult=1;assert(onRead(nullptr,nullptr)==1);
    auto* inaccessible=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);
    assert(inaccessible && onRead(nullptr,inaccessible)==1);
    VirtualFree(inaccessible,0,MEM_RELEASE);assert(changed==1);
    assert(guard(0,[](uintptr_t address,void* out,size_t n){assert(address==ReaderRva && n==16);std::memcpy(out,ReaderHead.data(),n);return true;}));
    assert(!guard(0,[](uintptr_t,void* out,size_t n){std::memset(out,0,n);return true;}));
    assert(ini37::DefaultRange==60 && ini37::MaximumRange<=128);
}
int main() {
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    groupRange(); materials(); lifecycle(); currentModeLayout(); camera(); targetsLifecycle(); reentryRetention(); progressiveActivation(); ranges(); adjustableShader(); iniParsing(); iniFiles(); lanternYieldRecovery();
    std::puts("PASS: guarded ore group capacity, progressive ON activation, bounded retry, re-entry retention, lantern recovery, material ownership, extended queries, radial range and INI reload.");
}
