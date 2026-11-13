#pragma once
#include "catalog_generated.hpp"

namespace alden {
inline unsigned category(std::uint32_t id) {
    std::size_t lo=0, hi=sizeof(Catalog)/sizeof(Catalog[0]);
    while(lo<hi) {
        const auto mid=lo+(hi-lo)/2;
        if(Catalog[mid].id<id) lo=mid+1; else hi=mid;
    }
    return lo<sizeof(Catalog)/sizeof(Catalog[0]) && Catalog[lo].id==id ? Catalog[lo].tab : 255;
}
struct Row { std::uintptr_t stock; std::uint32_t item; std::uint32_t index; };
inline unsigned category(const Row& row) {
    if(row.index>=ExtraStart) return row.index<OfferCount && ExtraItems[row.index-ExtraStart]==row.item ? 6 : 255;
    if(row.index<ModOfferCount) return category(row.item);
    const auto original=row.index-ModOfferCount;
    return original<OriginalOfferCount && OriginalItems[original]==row.item ? 8 : 255;
}
// Validate the complete input before writing. Retain the original stock pointers
// and their order, including the original purchase and stock indices they own.
inline bool select_rows(const Row* input,std::size_t count,unsigned tab,
                        std::uintptr_t* output,std::size_t capacity,std::size_t& written) {
    written=0;
    if(tab>=TabCount || count>capacity || (count && (!input || !output))) return false;
    for(std::size_t i=0;i<count;++i)
        if(!input[i].stock || category(input[i])==255) return false;
    for(std::size_t i=0;i<count;++i)
        if(tab==0 || category(input[i])==tab) output[written++]=input[i].stock;
    return true;
}
}
