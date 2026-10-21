#pragma once
#include "../material/material_only.hpp"

namespace range36 {
// Game 2.03.02 EffectMeshStandard pixel shader:
// hat != 0: min(1 - saturate(distance * .05), .5), ignores radius.
// hat == 0: cone contribution + 1 - saturate(distance / (.1 * radius)).
// A zero cone angle reduces the cone contribution to its 0.001-unit axis.
// This experimental path uses the remaining radial contribution. Half
// highlight intensity is configurable, 0.5 keeps the native peak.
// See research/range36-shader-evidence.md in the local analysis worktree.
inline mh34::Values values(const float* p,const float* look,const float* up,
                          float cutoff,bool radial=true,float strength=0.5f) {
    if(!radial) return mh34::values(p,look,up,30,30);
    auto result=mh34::values(p,look,up,0,cutoff*10.0f);
    mh34::put(result[5],uint32_t{0});
    mh34::put(result[6],strength);
    return result;
}
}
