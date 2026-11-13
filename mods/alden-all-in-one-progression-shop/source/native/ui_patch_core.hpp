#pragma once
#include <array>
#include <span>
#include <vector>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include "runtime_generated.hpp"

namespace alden::runtime {
using Bytes=std::span<const unsigned char>;
enum class PatchResult { unrelated, unsupported, patched };

inline bool memory_hash(Bytes data,char digest[65]) {
    if(data.size()>std::numeric_limits<ULONG>::max()) return false;
    BCRYPT_ALG_HANDLE algorithm{};
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)!=0) return false;
    unsigned char raw[32]{};
    const auto result=BCryptHash(algorithm,nullptr,0,const_cast<PUCHAR>(data.data()),
        static_cast<ULONG>(data.size()),raw,sizeof(raw));
    BCryptCloseAlgorithmProvider(algorithm,0);
    if(result!=0) return false;
    mh067::HashToHex(raw,digest);
    return true;
}

inline bool is_buy_ui(Bytes data) {
    return data.size()>=sizeof(BuySourceTail) && data.size()<=1024*1024 &&
        std::equal(std::begin(BuySourceTail),std::end(BuySourceTail),data.end()-sizeof(BuySourceTail));
}

// Only insert owned category controls and a root selector. Neither input's
// grid, event code, tooltip nor original stylesheet dependencies
// are rewritten. Source bytes are immutable; the output exists only in memory.
inline PatchResult patch_ui(Bytes input,std::vector<unsigned char>& output) {
    output.clear();
    if(!is_buy_ui(input)) return PatchResult::unrelated;
    char digest[65]{};
    if(!memory_hash(input,digest)) return PatchResult::unsupported;
    const char* expected=nullptr;
    std::span<const Edit> selectedEdits;
    if(!std::strcmp(digest,VanillaInputHash)) { expected=VanillaOutputHash; selectedEdits=Edits; }
    if(!std::strcmp(digest,BetterInputHash)) { expected=BetterOutputHash; selectedEdits=BetterEdits; }
    if(!expected) return PatchResult::unsupported;
    std::size_t cursor=0;
    output.reserve(input.size()+AddedSize);
    for(const auto& edit:selectedEdits) {
        if(edit.offset<cursor || edit.offset>input.size() || edit.before.size()>input.size()-edit.offset ||
           !std::equal(edit.before.begin(),edit.before.end(),input.begin()+edit.offset)) {
            output.clear(); return PatchResult::unsupported;
        }
        output.insert(output.end(),input.begin()+cursor,input.begin()+edit.offset);
        output.insert(output.end(),edit.after.begin(),edit.after.end());
        cursor=edit.offset+edit.before.size();
    }
    output.insert(output.end(),input.begin()+cursor,input.end());
    if(output.size()!=input.size()+AddedSize || !memory_hash(output,digest) || std::strcmp(expected,digest)) {
        output.clear(); return PatchResult::unsupported;
    }
    return PatchResult::patched;
}
}
