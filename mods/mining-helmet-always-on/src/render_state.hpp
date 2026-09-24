#pragma once
#include <cmath>
#include <cstdint>
#include <cstddef>
namespace mh125 {
// Native model transform is tiled, unlike the absolute geometry shader input.
// Verified by 0x14050ea00/0x14050ea80 and tile-size constant 0x145d736f4.
struct TiledModelTransform {
    float scale[3]{};
    float rotation[4]{};
    float localPosition[3]{};
    int16_t tileX{},tileZ{};
    uint32_t padding{};
    bool worldPosition(float* output) const {
        constexpr float tileSize=1000.0f;
        output[0]=localPosition[0]+static_cast<float>(tileX)*tileSize;
        output[1]=localPosition[1];
        output[2]=localPosition[2]+static_cast<float>(tileZ)*tileSize;
        return std::isfinite(output[0]) && std::isfinite(output[1]) && std::isfinite(output[2]);
    }
};
static_assert(sizeof(TiledModelTransform)==48);
static_assert(offsetof(TiledModelTransform,rotation)==12);
static_assert(offsetof(TiledModelTransform,localPosition)==28);
static_assert(offsetof(TiledModelTransform,tileX)==40 && offsetof(TiledModelTransform,tileZ)==42);
struct alignas(16) DetectGeometry {
    alignas(16) float position[4]{};
    alignas(16) float rotation[4]{};
    alignas(16) float look[4]{};
    alignas(16) float up[4]{};
    float angle{},radius{};
    uint32_t hat{};
    uint8_t type{};
    uint8_t padding[3]{};
    bool valid() const {
        if(hat!=1 || !std::isfinite(angle) || !std::isfinite(radius) || angle<=0 || radius<=0) return false;
        for(auto v:position) if(!std::isfinite(v)) return false;
        for(auto v:look) if(!std::isfinite(v)) return false;
        for(auto v:up) if(!std::isfinite(v)) return false;
        return true;
    }
};
enum class RenderAction { none, apply, clear, yieldToNative };
struct RenderState {
    DetectGeometry sample{};
    uint32_t player{};
    uint64_t capturedAt{};
    bool captured{},wanted{},applied{};
    bool observe(const DetectGeometry& geometry,uint32_t source,uint64_t now,bool nativeFindMine) {
        if(!nativeFindMine || !geometry.valid() || !source) return false;
        const bool first=!captured || player!=source;
        sample=geometry; player=source; capturedAt=now; captured=true;
        return first;
    }
    bool available(uint64_t now) const { return captured && now-capturedAt<=120000; }
    bool enable(uint64_t now) { wanted=available(now); return wanted; }
    RenderAction step(uint32_t currentPlayer,bool nativeGeometry,uint64_t now) {
        if(nativeGeometry) {
            const bool relinquish=applied || wanted;
            applied=false; wanted=false;
            return relinquish?RenderAction::yieldToNative:RenderAction::none;
        }
        if(wanted && currentPlayer==player && available(now)) { applied=true; return RenderAction::apply; }
        wanted=false;
        if(applied) { applied=false; return RenderAction::clear; }
        return RenderAction::none;
    }
};
}
