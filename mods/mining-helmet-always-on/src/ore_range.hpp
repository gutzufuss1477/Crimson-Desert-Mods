// 0x9af360 populates the shared nearby-actor cache before SpecialMode scans it.
// Its 0x9abde0 query uses a separate default radius of 30 (0x6cc0bb8).
// Enlarge that argument only inside the owned player's cache population call;
// retain native enumeration, actor leases, parallel distance jobs and sorting.
using NearbyCacheFn=void (__fastcall*)(uintptr_t,uintptr_t);
using NearbyQueryFn=void (__fastcall*)(uintptr_t,const float*,float,float,void*);
NearbyCacheFn nearbyCacheOriginal{};
NearbyQueryFn nearbyQueryOriginal{};
struct OreCacheContext { uintptr_t manager{},player{},component{}; };
thread_local OreCacheContext oreCacheContext{};
struct OreCacheScope {
    OreCacheContext previous;
    explicit OreCacheScope(OreCacheContext current):previous(oreCacheContext) { oreCacheContext=current; }
    ~OreCacheScope() { oreCacheContext=previous; }
    OreCacheScope(const OreCacheScope&)=delete;
    OreCacheScope& operator=(const OreCacheScope&)=delete;
};
std::atomic<unsigned> oreCacheCalls{0},oreQueryOverrides{0};
std::atomic<uint64_t> nextOreCacheLog{0};
void __fastcall nearbyQueryHook(uintptr_t manager,const float* position,float actorRadius,float range,void* output) {
    const auto scope=oreCacheContext;
    if(manager && manager==scope.manager && ownedOrePlayer(scope.component,scope.player) &&
       std::isfinite(range) && range>=0 && range<OreRange) {
        const float original=range;
        range=OreRange;
        if(oreQueryOverrides.fetch_add(1)==0)
            log("ore_candidate_query_extended","\"original_radius\":%.6g,\"radius\":%.6g",original,range);
    }
    nearbyQueryOriginal(manager,position,actorRadius,range,output);
}
void __fastcall nearbyCacheHook(uintptr_t manager,uintptr_t player) {
    const auto component=oreComponent.load();
    const bool owned=manager && ownedOrePlayer(component,player);
    // Clear the outer scope on nested unrelated calls, and restore on return.
    OreCacheScope scope(owned?OreCacheContext{manager,player,component}:OreCacheContext{});
    nearbyCacheOriginal(manager,player);
    if(!owned) return;
    const auto calls=oreCacheCalls.fetch_add(1)+1;
    const auto now=GetTickCount64(); auto due=nextOreCacheLog.load();
    if(calls==1 || (now>=due && nextOreCacheLog.compare_exchange_strong(due,now+5000))) {
        uintptr_t entries{}; uint32_t count{}; float lastSquared{};
        const bool readable=read(manager+0x308,entries) && read(manager+0x310,count) && count<=65536 &&
            (!count || (entries && read(entries+static_cast<uintptr_t>(count-1)*0x28+0x20,lastSquared))) &&
            std::isfinite(lastSquared) && lastSquared>=0;
        log("ore_candidate_cache","\"calls\":%u,\"query_overrides\":%u,\"readable\":%s,\"targets\":%u,\"last_distance\":%.6g",
            calls,oreQueryOverrides.load(),readable?"true":"false",readable?count:0,readable?std::sqrt(lastSquared):0.0f);
    }
}
