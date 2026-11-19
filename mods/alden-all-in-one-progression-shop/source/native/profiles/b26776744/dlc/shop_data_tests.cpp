#include "shop_data.hpp"
#include "filter_core.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>

using namespace alden::shop_data;
static void require(bool ok,const char* msg) {
    if(!ok) { std::fprintf(stderr,"FAIL: %s\n",msg); std::exit(1); }
}
template<class T> static void put(Address dest,const T& v) { std::memcpy(reinterpret_cast<void*>(dest),&v,sizeof(v)); }
template<class T> static Address addr(T& obj) { return reinterpret_cast<Address>(&obj); }
struct PriceRow { std::uint16_t currency; std::uint8_t pad[6]; std::uint64_t value; std::uint32_t sym; std::uint16_t wrapper; std::uint16_t tail; };
static_assert(sizeof(PriceRow)==24 && offsetof(PriceRow,wrapper)==20);
struct MockItem {
    alignas(8) unsigned char data[0x1b0]{}, level[0x68]{};
    Array levels{};
    PriceRow basePrices[2]{},tierPrices[2]{};
};
struct Fixture {
    alignas(8) unsigned char merchant[0x70]{};
    std::vector<std::array<unsigned char,104>> stock{std::size(Offers)}, desc{std::size(Offers)};
    std::vector<RegisteredItem> ids;
    std::vector<MockItem> items;
    std::unordered_map<unsigned,std::uint16_t> ordinal;
    Array registry{};
    template<class T> bool get(Address from,T& out) {
        if(!from)return false;
        std::memcpy(&out,reinterpret_cast<const void*>(from),sizeof(T)); return true;
    }
    bool writable(Address,std::size_t) { return true; }
    Address item(std::uint16_t id) { return ids.at(id).data; }
    Fixture() {
        std::set<unsigned> keys{1};
        for(const auto& o:Offers) keys.insert(o.item);
        for(const auto& p:Prices) keys.insert(p.currency);
        items.resize(keys.size());
        for(auto key:keys) {
            auto ix=static_cast<std::uint16_t>(ids.size());
            ordinal[key]=ix; auto& it=items[ix]; put(addr(it.data)+8,key);
            ids.push_back({key,0,addr(it.data),0});
        }
        registry={addr(ids.front()),static_cast<unsigned>(ids.size()),static_cast<unsigned>(ids.size())};
        put(addr(merchant)+8,std::uint16_t{3101});
        put(addr(merchant)+0x58,Array{addr(stock.front()),840,840});
        for(unsigned i=0;i<std::size(Offers);++i) {
            const auto& o=Offers[i]; Address r=addr(stock[i]),d=addr(desc[i]);
            put(r+0x28,i); put(r+0x2c,o.saved); put(r+0x38,d);
            put(d+0x38,o.dlc ? std::uint16_t{0xffff} : ordinal.at(o.item));
            put(d+0x60,std::uint8_t{0});
        }
        for(const auto& rule:Prices) {
            auto& it=items[ordinal.at(rule.item)];
            auto& row=(rule.tier?it.tierPrices:it.basePrices)[rule.slot];
            row.currency=row.wrapper=ordinal.at(rule.currency);
            row.value=rule.original;
            if(rule.tier) {
                it.levels={addr(it.level),1,1};
                put(addr(it.data)+0x190,addr(it.levels));
                put(addr(it.level)+0x48,Array{addr(it.tierPrices),2,2});
            } else put(addr(it.data)+0x198,Array{addr(it.basePrices),2,2});
        }
    }
};
int main() {
    Fixture f;
    auto& last=f.items[f.ordinal.at(Prices[std::size(Prices)-1].item)];
    const auto& lastRule=Prices[std::size(Prices)-1];
    auto& lastAmount=(lastRule.tier?last.tierPrices:last.basePrices)[lastRule.slot].value;
    lastAmount=987654;
    auto before=f.desc;
    auto result=apply(f,addr(f.merchant),f.registry);
    auto& wound=f.items[f.ordinal.at(1002580)];
    auto& destruction=f.items[f.ordinal.at(1002785)];
    require(result.error && f.desc==before && wound.basePrices[0].value==2000 &&
            destruction.tierPrices[0].value==633,"late conflicting price leaves every offer and price intact");
    lastAmount=lastRule.original;
    result=apply(f,addr(f.merchant),f.registry);
    require(!result.error && result.linked==234 && result.priced==1047,"complete catalog correction");
    require(wound.basePrices[0].value==1 && wound.tierPrices[0].value==1 &&
            destruction.basePrices[0].value==1 && destruction.tierPrices[0].value==1,
            "both screenshot prices are one, including refinement");
    for(const auto& rule:Prices) {
        const auto& it=f.items[f.ordinal.at(rule.item)];
        auto& price=(rule.tier?it.tierPrices:it.basePrices)[rule.slot];
        require(price.value==1 && price.currency==f.ordinal.at(rule.currency),"authored price and currency");
    }
    unsigned dlc=0;
    for(unsigned i=0;i<std::size(Offers);++i) {
        std::uint16_t ix{}; f.get(addr(f.desc[i])+0x38,ix);
        unsigned id{}; f.get(f.item(ix)+8,id);
        require(id==Offers[i].item,"native item lookup resolves each actual offer");
        auto tab=alden::category({addr(f.stock[i]),id,Offers[i].saved});
        require(tab!=255,"native row is classifiable");
        if(tab==9) ++dlc;
    }
    require(dlc==234,"all DLC rows select the DLC tab");
    result=apply(f,addr(f.merchant),f.registry);
    require(!result.error && !result.linked && !result.priced,"reopening is idempotent");
    std::puts("PASS: 840 shop rows, 234 missing expansion references, all 1050 price fields, screenshot items, category classification, rollback on conflict and repeat opening.");
}
