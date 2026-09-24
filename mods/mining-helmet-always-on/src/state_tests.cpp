#include "capture_state.hpp"
#include <cstdio>
#include <cstdlib>
using namespace mh125;
void check(bool yes, const char* msg) { if (!yes) { std::fprintf(stderr, "FAIL: %s\n", msg); std::exit(1); } }
Event event(uint32_t kind, uint32_t player=17) {
    Event e{}; set(e, 0, kind); set(e, 0x10, player); set(e, 0x14, player); return e;
}
int main() {
    State s; Identity a{100,200,300,17}; Dispatch d;
    check(!s.capture(a,event(Off),100), "off without calibration rejected");
    s.capture(a,event(On),100);
    check(s.request(On,110)==0, "native active must block replay");
    s.capture(a,event(Off,18),120);
    check(s.request(On,130)==0, "different player must block replay");
    s.capture(a,event(Off),140);
    check(s.request(On,150)==1, "valid native on/off pair becomes eligible");
    check(s.next(152,d) && d.kind==On, "pending target selected independent of target ticks");
    check(!s.next(153,d), "single shot, not every frame");
    s.result(d,9);
    check(s.syntheticCount()==0, "queue failure does not claim effect ownership");
    s.request(On,160); check(s.next(161,d), "retry available after failure"); s.result(d,0);
    check(s.request(On,162)==0, "no stacking duplicate on");
    check(s.request(Off,163)==1, "off only for owned effect");
    check(s.next(164,d) && d.kind==Off, "off uses recorded off payload"); s.result(d,0);
    check(s.syntheticCount()==0 && s.request(Off,165)==0, "cleanup complete");
    s.request(On,170); check(!s.next(6000,d), "expired command not replayed later");
    s.capture(a,event(On),6100); s.capture(a,event(Off),6101); s.request(On,6102);
    s.request(On,17000);
    check(s.next(17001,d), "static ore without its own updates remains dispatchable");
    check(s.current(d), "dispatch current before new native event");
    s.capture(a,event(On),17002);
    check(!s.current(d), "new native event invalidates in-flight dispatch");
    s.capture(a,event(On),18000); s.capture(a,event(Off),18001);
    check(s.request(On,18000+MaxAgeMs+1)==0, "old calibration expires");
    s.capture(a,event(On),200000); s.capture(a,event(Off),200001); s.request(On,200002);
    s.capture(a,event(On),200003);
    check(!s.next(200004,d), "new native activation cancels pending injection");
    State full;
    for(size_t i=0;i<MaxTargets;++i) full.capture({i+1,i+2,uint32_t(i+3),17},event(On),100);
    check(!full.capture({999,999,999,17},event(On),101) && full.overflow==1, "bounded capture buffer");
    State scene;
    for(unsigned i=0;i<81;++i) { Identity id{i+1,i+2,i+3,17}; scene.capture(id,event(On),100); scene.capture(id,event(Off),105); }
    check(scene.request(On,30000)==81 && scene.overflow==0,"all 81 observed targets fit after 30 seconds without target ticks");
    unsigned sent=0; while(scene.next(30001,d)) ++sent;
    check(sent==81 && !scene.hasPending(),"all selected targets leave pending queue exactly once");
    std::puts("PASS: calibration, static-target scheduling, single-shot, native precedence, expiry, cleanup, 81-target regression");
}
