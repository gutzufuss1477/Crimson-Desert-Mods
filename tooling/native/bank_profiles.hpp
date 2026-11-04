#pragma once
#include "profiles_26361232.hpp"

namespace cd {
// The dispatcher still calls both handlers at the validated sites. Build
// 26423880 changed one member access in gold (+0x210 -> +0x218). Keep
// that operand in the full-function hash for each accepted layout.
inline constexpr uint32_t bank_gold_id = 0x2df40f0;
inline constexpr uint8_t bank_gold_sha_26423880[] = {
    0xc8,0x6b,0x07,0x1b,0xc1,0x62,0x60,0x06,0xc2,0x09,0x43,0xb3,0x9a,0x1d,0x48,0x4e,
    0x50,0xc7,0x3d,0x75,0x4a,0x01,0xd3,0xf3,0x05,0xaa,0xe3,0xfa,0x68,0xaa,0x74,0xb7
};
inline constexpr Pattern bank_gold_26423880 = [] {
    auto pattern = p_2df40f0;
    pattern.sha = bank_gold_sha_26423880;
    return pattern;
}();
inline constexpr auto bank_nodes_26423880 = [] {
    auto nodes = std::to_array(bank_nodes);
    for (auto& node : nodes) {
        if (node.id == bank_gold_id) {
            node.pattern = &bank_gold_26423880;
            node.hint = 0x2dfd7d0;
        }
    }
    return nodes;
}();
inline constexpr Plan bank_plan_26423880 = [] {
    auto plan = bank_plan;
    plan.nodes = bank_nodes_26423880.data();
    return plan;
}();

inline bool resolve_bank(const Image& image, Addresses& addresses) {
    if (addresses.resolve(image, bank_plan_26423880)) return true;
    return addresses.failed_id == bank_gold_id && addresses.resolve(image, bank_plan);
}
}
