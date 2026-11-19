#pragma once
#include "shop_data_generated.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <unordered_map>
#include <vector>

namespace alden::shop_data {
using Address = std::uintptr_t;
// The game's ItemInfo registry stores each key with the file offset and the
// currently materialized item. DLC and base keys share this ordinal space.
struct RegisteredItem {
    std::uint32_t key, fileOffset;
    Address data, reserved;
};
static_assert(sizeof(RegisteredItem)==24);
struct Array { Address data; std::uint32_t count, capacity; };
struct FixResult {
    const char* error{};
    unsigned key{}, linked{}, priced{};
};
template<class T> struct Edit { T* field; T before, after; };

// Access supplies only field reads, game data page checks and the typed ItemInfo
// lookup. The full catalog is checked before changing any purchase data.
template<class Access>
FixResult apply(Access& access, Address merchant, const Array& registered) {
    FixResult result;
    auto fail = [&](const char* reason, unsigned key) {
        result.error=reason; result.key=key; return result;
    };
    std::uint16_t merchantKey{};
    Array stock{};
    if(!access.get(merchant+8,merchantKey) || merchantKey!=3101 ||
       !access.get(merchant+0x58,stock) || !stock.data || stock.count!=std::size(Offers) ||
       !registered.data || !registered.count || registered.count>=0xffffu)
        return fail("merchant or item registry layout",3101);

    std::unordered_map<unsigned,std::uint16_t> ordinals;
    ordinals.reserve(registered.count);
    for(unsigned i=0;i<registered.count;++i) {
        unsigned key{};
        if(!access.get(registered.data+i*sizeof(RegisteredItem),key) ||
           !ordinals.emplace(key,static_cast<std::uint16_t>(i)).second)
            return fail("duplicate or unreadable registered item",key);
    }
    std::vector<Edit<std::uint16_t>> links;
    std::vector<Edit<std::uint64_t>> prices;
    links.reserve(234); prices.reserve(std::size(Prices));
    for(unsigned i=0;i<stock.count;++i) {
        const auto& offer=Offers[i];
        const auto found=ordinals.find(offer.item);
        const Address row=stock.data+104u*i;
        unsigned purchase{},saved{};
        Address description{};
        std::uint8_t type{};
        std::uint16_t item{};
        if(found==ordinals.end() || !access.get(row+0x28,purchase) || purchase!=i ||
           !access.get(row+0x2c,saved) || saved!=offer.saved ||
           !access.get(row+0x38,description) || !description ||
           !access.get(description+0x60,type) || type!=0 ||
           !access.get(description+0x38,item))
            return fail("offer identity",offer.item);
        if(item==found->second) continue;
        if(item!=0xffff || !offer.dlc || !access.writable(description+0x38,sizeof(item)))
            return fail("conflicting offer item",offer.item);
        links.push_back({reinterpret_cast<std::uint16_t*>(description+0x38),item,found->second});
    }

    unsigned lastKey=0;
    Address item=0;
    for(const auto& rule:Prices) {
        if(rule.item!=lastKey) {
            const auto found=ordinals.find(rule.item);
            if(found==ordinals.end()) return fail("missing catalog item",rule.item);
            item=access.item(found->second);
            unsigned key{};
            if(!item || !access.get(item+8,key) || key!=rule.item)
                return fail("item identity",rule.item);
            lastKey=rule.item;
        }
        Address listAddress=item+0x198;
        if(rule.tier) {
            Address levels{};
            Array tiers{};
            if(!access.get(item+0x190,levels) || !levels || !access.get(levels,tiers) ||
               !tiers.data || tiers.count!=1) return fail("refinement tier",rule.item);
            listAddress=tiers.data+0x48;
        }
        Array list{};
        if(!access.get(listAddress,list) || !list.data || list.count<=rule.slot || list.count>64)
            return fail("price list",rule.item);
        const Address entry=list.data+rule.slot*24;
        const auto currency=ordinals.find(rule.currency);
        std::uint16_t key{},wrapper{};
        std::uint64_t amount{};
        if(currency==ordinals.end() || !access.get(entry,key) || key!=currency->second ||
           !access.get(entry+20,wrapper) || wrapper!=key || !access.get(entry+8,amount))
            return fail("price currency",rule.item);
        if(amount==1) continue;
        if(amount!=rule.original || !access.writable(entry+8,sizeof(amount)))
            return fail("conflicting price",rule.item);
        prices.push_back({reinterpret_cast<std::uint64_t*>(entry+8),amount,1});
    }
    // Another enabled mod may already have supplied compatible links/prices.
    // Reopening an unchanged shop has an empty edit list.
    for(const auto& edit:links) if(*edit.field!=edit.before) return fail("offer changed during validation",3101);
    for(const auto& edit:prices) if(*edit.field!=edit.before) return fail("price changed during validation",3101);
    for(const auto& edit:prices) *edit.field=edit.after;
    for(const auto& edit:links) *edit.field=edit.after;
    result.linked=static_cast<unsigned>(links.size());
    result.priced=static_cast<unsigned>(prices.size());
    return result;
}
} // namespace alden::shop_data
