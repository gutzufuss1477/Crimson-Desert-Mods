#define AUTO34_TEST_HOST
#include "automatic_material.cpp"
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
    set(Root+0x20,Manager); set(Manager+0x1228,uintptr_t{0x21000}); set(0x21000+0x160,uintptr_t{0x22000});
    set(0x22000+0x538,Groups); set(Image+0x6c8cce8,Pool); set(Pool+0x58,Names);
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
    field(generic,0,Image+0x55b7898); field(generic,8,reinterpret_cast<uintptr_t>(actor.data()));
    field(mode,0,Image+0x558d140); field(mode,0x30,uint16_t{0xffff}); field(mode,0x48,uint16_t{0xffff});
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
    field(handler,0,Image+0x57c3118); field(handler,0x6c,MineIndex); field(event,0,On);
    uint32_t result{}; assert(effectHook(generic.data(),&result,handler.data(),event.data())==&result);
    assert(result==42 && effects==1 && targets[0].mine);
    field(event,0,Off); targets[0].dispatched=true;
    effectHook(generic.data(),&result,handler.data(),event.data()); assert(!targets[0].ours && !targets[0].dispatched && effects==2);
    assert(mode==originalMode && releases==5);
}
unsigned eyeCalls{},orientationCalls{},rotationCalls{};
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
    assert(enqueues==3 && !targets[0].who.id && value<uint32_t>(reinterpret_cast<uintptr_t>(sent.data()))==Off);
}
}
int main() {
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    materials(); lifecycle(); camera(); targetsLifecycle();
    std::puts("PASS: material write bounds, moving position/camera, restore/foreign ownership, stale identities, native-mode yield, fresh event, live-effect retention and source cleanup.");
}
