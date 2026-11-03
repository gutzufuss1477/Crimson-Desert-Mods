#define ALDEN_TAB_TESTING
#include "native_tabs.cpp"
#include <array>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <thread>

static void check(bool condition,const char* message) {
    if(!condition) { std::fprintf(stderr,"FAIL: %s\n",message); std::exit(1); }
}
template<class T> static void put(alden::Ptr p,T value) { std::memcpy(reinterpret_cast<void*>(p),&value,sizeof(value)); }
template<class T> static T get(alden::Ptr p) { T value{}; check(alden::value(p,value),"fixture read"); return value; }

namespace fixture {
using alden::Ptr;
alignas(16) std::array<unsigned char,0x400> root{},stock{},bar{};
std::array<unsigned char,32> storeInfo{};
std::array<std::array<unsigned char,32>,567> items{};
std::array<std::array<unsigned char,0x68>,567> records{},descriptions{};
std::array<Ptr,567> pointers{};
std::array<Ptr,567> allPointers{};
unsigned fillCalls{},countCalls{},passthroughTab{},selected{},destroyCalls{};
unsigned initError{},bindError{};
bool selectorPresent=true;
bool shown{};
Ptr r() { return reinterpret_cast<Ptr>(root.data()); }
Ptr s() { return reinterpret_cast<Ptr>(stock.data()); }
Ptr b() { return reinterpret_cast<Ptr>(bar.data()); }
Ptr __fastcall init(Ptr p,Ptr result) { check(p==r(),"initialize own root");put(result,initError);return result; }
const char* __fastcall attribute(Ptr,Ptr key,const char*) {
    check(key==alden::base+alden::profile::TabSelectorKey,"category selector intern key");
    static const char absent[sizeof("#AldenCategoryTabs")]{};
    return selectorPresent ? "#AldenCategoryTabs" : absent;
}
Ptr __fastcall bind(Ptr p,Ptr result,Ptr out,Ptr) {
    check(p==r(),"bind own root");put(result,bindError);put(out,bindError ? Ptr{} : b());return result;
}
Ptr __fastcall store_info(Ptr,unsigned index) {
    check(index==7,"native store ordinal preserved"); return reinterpret_cast<Ptr>(storeInfo.data());
}
Ptr __fastcall item_info(Ptr,unsigned index) {
    check(index<items.size(),"native item ordinal in range"); return reinterpret_cast<Ptr>(items[index].data());
}
void __fastcall show(Ptr p) { check(p==b(),"show own bar"); shown=true; }
void __fastcall hide(Ptr p) { check(p==b(),"hide own bar"); shown=false; }
void __fastcall select(Ptr p,unsigned tab,bool,bool) { check(p==b(),"select own bar"); selected=tab; }
unsigned char __fastcall count(Ptr p,unsigned n) { ++countCalls; put(p+0x100,n);put(p+0x104,n); return 1; }
void __fastcall tab(Ptr,Ptr,bool,unsigned,unsigned,Ptr) { ++passthroughTab; }
Ptr __fastcall destroy(Ptr p,unsigned) { ++destroyCalls; return p; }
void __fastcall fill(Ptr p) {
    check(p==r(),"fill own root"); ++fillCalls;
    pointers=allPointers;
    put(s()+0x188,567u);
    put(s()+0x120,0u);
    alden::count_hook(s(),567);
}
void setup() {
    alden::base=0x140000000;
    alden::ready=true;
#ifdef ALDEN_RUNTIME_UI
    alden::categoryStylesAvailable=true; alden::categoryStyleFailure=false;
#endif
    alden::originalFill=fill; alden::originalCount=count; alden::originalTab=tab; alden::originalDestroy=destroy;
    alden::originalInit=init;initError=bindError=0;selectorPresent=true;
    root.fill(0);stock.fill(0);bar.fill(0);
    put(r(),alden::base+alden::profile::RootVtable);
    put(r()+8,r());
    put(r()+0x1f8,s());
    put(s(),alden::base+alden::profile::StockVtable);
    put(s()+0x17a,std::uint16_t{7});
    put(s()+0x180,reinterpret_cast<Ptr>(pointers.data()));
    put(s()+0x188,567u); put(s()+0x18c,567u);
    put(b(),alden::base+alden::profile::TabVtable); put(b()+0xd0,8u);
    put(reinterpret_cast<Ptr>(storeInfo.data())+8,std::uint16_t{3101});
    for(unsigned i=0;i<567;++i) {
        // Reverse the catalog's numeric-ID order so preservation is observable.
        const auto id=alden::Catalog[566-i].id;
        put(reinterpret_cast<Ptr>(items[i].data())+8,id);
        const Ptr rec=reinterpret_cast<Ptr>(records[i].data());
        put(rec+0x38,reinterpret_cast<Ptr>(descriptions[i].data()));
        put(reinterpret_cast<Ptr>(descriptions[i].data())+0x38,static_cast<std::uint16_t>(i));
        put(rec+0x28,i); // The native purchase row index must never be rewritten.
        allPointers[i]=rec;
    }
    pointers=allPointers;
    for(auto& state:alden::states) state={};
    alden::states[0]={r(),s(),b(),0,false,false,GetCurrentThreadId(),GetCurrentThreadId()};
    fillCalls=countCalls=passthroughTab=selected=destroyCalls=0; shown=false;
}
}

namespace alden {
Ptr resolve_test_function(Ptr rva) {
    if(rva==profile::Attribute) return reinterpret_cast<Ptr>(&fixture::attribute);
    if(rva==profile::BindTab) return reinterpret_cast<Ptr>(&fixture::bind);
    if(rva==profile::StoreInfo) return reinterpret_cast<Ptr>(&fixture::store_info);
    if(rva==profile::ItemInfo) return reinterpret_cast<Ptr>(&fixture::item_info);
    if(rva==profile::SelectTab) return reinterpret_cast<Ptr>(&fixture::select);
    if(rva==profile::Show) return reinterpret_cast<Ptr>(&fixture::show);
    if(rva==profile::Hide) return reinterpret_cast<Ptr>(&fixture::hide);
    check(false,"unexpected native function"); return 0;
}
}

int main() {
    using namespace alden;
    constexpr unsigned expected[]={567,94,93,71,95,125,39,50};
    fixture::setup();states[0]={};unsigned initialization=99;
#ifdef ALDEN_RUNTIME_UI
    categoryStylesAvailable=false;
    fixture::shown=true;
    init_hook(fixture::r(),reinterpret_cast<Ptr>(&initialization));
    check(initialization==0 && !states[0].root && !fixture::shown,"missing category styles preserve native initialization and explicitly hide tabs");
    categoryStylesAvailable=true; categoryStyleFailure=true;
    fixture::shown=true;
    init_hook(fixture::r(),reinterpret_cast<Ptr>(&initialization));
    check(initialization==0 && !states[0].root && !fixture::shown,"failed CSS registration leaves native list unfiltered and explicitly hides tabs");
    categoryStyleFailure=false;
#endif
    check(init_hook(fixture::r(),reinterpret_cast<Ptr>(&initialization))==reinterpret_cast<Ptr>(&initialization),"native initialization result preserved");
    check(initialization==0 && states[0].bar==fixture::b() && !fixture::shown,"compiled category UI binds and starts hidden");
    fixture::setup();states[0]={};fixture::selectorPresent=false;
    init_hook(fixture::r(),reinterpret_cast<Ptr>(&initialization));
    check(!states[0].root && states[0].ownerThread==0,"18a regression: unmodified compiled UI is diagnosed and not bound");
    fixture::setup();states[0]={};fixture::bindError=1;
    init_hook(fixture::r(),reinterpret_cast<Ptr>(&initialization));
    check(!states[0].root,"failed native binding retains no UI pointers");
    fixture::setup();states[0]={};fixture::initError=1;
    init_hook(fixture::r(),reinterpret_cast<Ptr>(&initialization));
    check(initialization==1 && !states[0].root,"native initialization failure passes through");
    // Reproduce the actual 18b failure: initialize on one Windows thread,
    // fill and click on another. This must reveal tabs and filter correctly.
    fixture::setup();states[0]={};
    std::thread loader([&] { init_hook(fixture::r(),reinterpret_cast<Ptr>(&initialization)); });
    loader.join();
    check(states[0].root==fixture::r() && states[0].loaderThread!=GetCurrentThreadId() &&
          states[0].ownerThread==0 && !fixture::shown,"loader binds without claiming update thread");
    fill_hook(fixture::r());
    check(fixture::shown && states[0].ownerThread==GetCurrentThreadId(),"18b regression: first fill takes ownership and shows tabs");
    tab_hook(fixture::r(),fixture::b(),false,0,6,0);
    check(get<unsigned>(fixture::s()+0x100)==39,"material tab filters after real cross-thread handoff");
    const auto beforeForeignFill=fixture::fillCalls;
    std::thread foreign([] { tab_hook(fixture::r(),fixture::b(),false,6,1,0); });
    foreign.join();
    check(fixture::fillCalls==beforeForeignFill && states[0].tab==6,"foreign thread cannot take over an owned root");
    std::thread destroyer([] { destroy_hook(fixture::r(),1); });
    destroyer.join();
    check(!states[0].root && fixture::destroyCalls==1,"destruction clears binding even on a different thread");
    std::thread loaderAgain([&] { init_hook(fixture::r(),reinterpret_cast<Ptr>(&initialization)); });
    loaderAgain.join();fill_hook(fixture::r());
    check(fixture::shown && states[0].tab==0 && get<unsigned>(fixture::s()+0x100)==567,"recreated root takes fresh ownership and opens All");
    fixture::setup();
    fill_hook(fixture::r());
    check(fixture::shown && fixture::selected==0,"Alden opens All with visible tabs");
    for(unsigned tab=1;tab<8;++tab) {
        tab_hook(fixture::r(),fixture::b(),false,tab-1,tab,0);
        check(get<unsigned>(fixture::s()+0x100)==expected[tab],"correct category count");
        unsigned selected=0;
        for(unsigned original=0;original<567;++original) {
            const auto id=Catalog[566-original].id;
            if(category(id)!=tab) continue;
            const auto pointer=fixture::pointers[selected++];
            check(pointer==fixture::allPointers[original],"display and purchase use original stock record");
            check(get<unsigned>(pointer+0x28)==original,"original purchase index retained");
        }
        check(selected==expected[tab],"complete category");
        fixture::fill(fixture::r()); // Native stock refresh after buying/restocking.
        check(get<unsigned>(fixture::s()+0x100)==expected[tab],"stock refresh retains selected filter");
    }
    tab_hook(fixture::r(),fixture::b(),false,7,0,0);
    check(fixture::pointers==fixture::allPointers,"All restores complete original order");
    const auto fills=fixture::fillCalls;
    tab_hook(fixture::r(),fixture::b(),true,0,6,0);
    tab_hook(fixture::r(),fixture::b(),false,0,99,0);
    check(fixture::fillCalls==fills,"preview and invalid tab do not rebuild");
    states[0].busy=true;
    tab_hook(fixture::r(),fixture::b(),false,0,6,0);
    check(fixture::fillCalls==fills,"recursive selection ignored");states[0].busy=false;
    tab_hook(fixture::r(),fixture::b()+8,false,0,1,0);
    check(fixture::passthroughTab==1,"unrelated tab routed unchanged");
    tab_hook(fixture::r(),fixture::b(),false,0,6,0);
    fill_hook(fixture::r());
    check(fixture::selected==0 && get<unsigned>(fixture::s()+0x100)==567,"reopen resets All");
    put(reinterpret_cast<Ptr>(fixture::storeInfo.data())+8,std::uint16_t{1234});
    fill_hook(fixture::r());
    check(!fixture::shown && get<unsigned>(fixture::s()+0x100)==567,"other merchants unchanged and bar hidden");
    fixture::setup();
    put(reinterpret_cast<Ptr>(fixture::items[300].data())+8,0xffffffffu);
    tab_hook(fixture::r(),fixture::b(),false,0,6,0);
    check(states[0].fault && states[0].tab==0 && fixture::selected==0,"unknown item falls back to All");
    check(fixture::pointers==fixture::allPointers,"invalid input produces no partial list writes");
    fixture::setup(); states[0].tab=6;
    put(fixture::s()+0x188,5000u);
    count_hook(fixture::s(),5000);
    check(states[0].fault && fixture::pointers==fixture::allPointers,"oversized list refused without writes");
    fixture::setup();states[0].tab=6;put(fixture::s()+0x188,0u);
    count_hook(fixture::s(),0);
    check(get<unsigned>(fixture::s()+0x100)==0 && !states[0].fault,"empty stock handled");
    fixture::setup();states[0].tab=6;states[0].ownerThread=0;
    count_hook(fixture::s(),567);
    check(fixture::pointers==fixture::allPointers,"other UI thread passes through");
    destroy_hook(fixture::r(),1);
    check(states[0].root==0 && fixture::destroyCalls==1,"destruction removes stored UI pointers");
    Row one{1,Catalog[0].id};Ptr out=0xdead;std::size_t written{};
    check(!select_rows(&one,1,8,&out,1,written) && out==0xdead,"invalid category is atomic");
    check(!select_rows(&one,1,0,&out,0,written) && out==0xdead,"insufficient capacity is atomic");
    std::puts("PASS: real loader/update thread handoff, cross-thread destruction/recreation and foreign-thread guard; compiled-UI binding/missing-selector regression; eight tabs, purchase identity/order, stock refresh, reopen, merchant isolation, invalid/empty data and reentrancy.");
    return 0;
}
