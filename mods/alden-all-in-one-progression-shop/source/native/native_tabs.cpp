// Alden category tabs, v17.1, Steam build 26361232. Copyright 2026 Blablup. MIT.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <string>
#include <mutex>
#include "hash_core.hpp"
#include "filter_core.hpp"
#include "profile_generated.hpp"
#ifdef ALDEN_RUNTIME_UI
#include "ui_patch_core.hpp"
#include "vendor/minhook/include/MinHook.h"
#endif

namespace alden {
using Ptr=std::uintptr_t;
using InitFn=Ptr(__fastcall*)(Ptr,Ptr);
using DestroyFn=Ptr(__fastcall*)(Ptr,unsigned);
using FillFn=void(__fastcall*)(Ptr);
using TabFn=void(__fastcall*)(Ptr,Ptr,bool,unsigned,unsigned,Ptr);
using CountFn=unsigned char(__fastcall*)(Ptr,unsigned);
using BindFn=Ptr(__fastcall*)(Ptr,Ptr,Ptr,Ptr);
using InfoFn=Ptr(__fastcall*)(Ptr,unsigned);
using SelectFn=void(__fastcall*)(Ptr,unsigned,bool,bool);
using AttributeFn=const char*(__fastcall*)(Ptr,Ptr,const char*);
HMODULE module{};
Ptr base{};
InitFn originalInit{};
DestroyFn originalDestroy{};
FillFn originalFill{};
TabFn originalTab{};
CountFn originalCount{};
std::atomic<bool> ready{};
HANDLE logfile=INVALID_HANDLE_VALUE;
SRWLOCK logLock=SRWLOCK_INIT;
// The game initializes UI on a loader thread, then opens/updates it on its
// gameplay UI thread. Publish bindings under the same recursive lock used by
// callbacks; the first fill establishes each root's update-thread ownership.
// Recursion is needed for native Fill -> stock count and Tab -> Fill callbacks.
std::recursive_mutex stateMutex;
struct State {
    Ptr root{},stock{},bar{}; unsigned tab{}; bool busy{},fault{};
    DWORD loaderThread{},ownerThread{};
};
State states[16]{};

void log(const char* message) {
    if(logfile==INVALID_HANDLE_VALUE) return;
    AcquireSRWLockExclusive(&logLock);
    char line[600];
    const int n=_snprintf_s(line,sizeof(line),_TRUNCATE,"[%llu] %s\r\n",GetTickCount64(),message);
    if(n>0) { DWORD wrote{}; WriteFile(logfile,line,static_cast<DWORD>(n),&wrote,nullptr); FlushFileBuffers(logfile); }
    ReleaseSRWLockExclusive(&logLock);
}
bool read(Ptr address,void* out,std::size_t bytes) {
    if(!address) return false;
    __try { std::memcpy(out,reinterpret_cast<const void*>(address),bytes); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
template<class T> bool value(Ptr p,T& v) { return read(p,&v,sizeof(v)); }
#ifdef ALDEN_TAB_TESTING
Ptr resolve_test_function(Ptr rva);
template<class Fn> Fn function(Ptr rva) { return reinterpret_cast<Fn>(resolve_test_function(rva)); }
#else
template<class Fn> Fn function(Ptr rva) { return reinterpret_cast<Fn>(base+rva); }
#endif
bool on_ui_thread(const State& s) { return s.ownerThread && s.ownerThread==GetCurrentThreadId(); }
State* by_root(Ptr root) { for(auto& s:states) if(s.root==root) return &s; return nullptr; }
State* by_stock(Ptr stock) { for(auto& s:states) if(s.root && s.stock==stock) return &s; return nullptr; }

#ifdef ALDEN_RUNTIME_UI
#include "runtime_ui.hpp"
#endif

bool is_alden(Ptr stock) {
    Ptr vtable{}; std::uint16_t index{},key{}; std::uint8_t mode{},housing{},alternate{};
    if(!value(stock,vtable) || vtable!=base+profile::StockVtable ||
       !value(stock+0x178,mode) || mode!=0 || !value(stock+0x17a,index) || index==0xffff ||
       !value(stock+0x1f0,housing) || housing || !value(stock+0x1f1,alternate) || alternate) return false;
    const auto info=function<InfoFn>(profile::StoreInfo)(0,index);
    return value(info+8,key) && key==3101;
}
void show_bar(State& s,bool show) {
    function<FillFn>(show ? profile::Show : profile::Hide)(s.bar);
}
void select_tab(State& s,unsigned tab) {
    function<SelectFn>(profile::SelectTab)(s.bar,tab,false,false);
}

Ptr __fastcall init_hook(Ptr root,Ptr result) {
    const auto returned=originalInit(root,result);
    unsigned error{};
    if(!ready.load()) return returned;
    if(!value(result,error) || error) { log("INIT REFUSED: native shop initialization failed"); return returned; }
    Ptr element{};
    if(!value(root+8,element) || !element) { log("INIT REFUSED: root element missing"); return returned; }
    const char* selector=function<AttributeFn>(profile::Attribute)(element,base+profile::TabSelectorKey,"");
    char marker[sizeof("#AldenCategoryTabs")]{};
    if(!selector || !read(reinterpret_cast<Ptr>(selector),marker,sizeof(marker)) ||
       std::memcmp(marker,"#AldenCategoryTabs",sizeof(marker))) {
        Ptr stock{}; std::uint8_t mode=255;
        value(root+0x1f8,stock); if(stock) value(stock+0x178,mode);
        if(mode==0) log("INIT REFUSED: buy UI has no category selector; compiled shop UI overlay not active");
        return returned;
    }
    const std::lock_guard<std::recursive_mutex> lock(stateMutex);
    State* slot=by_root(root);
    if(!slot) for(auto& s:states) if(!s.root) { slot=&s; break; }
    if(!slot) { log("REFUSED: category UI state limit reached"); return returned; }
    Ptr bar{},stock{},vt{}; unsigned bindingError=~0u;
    function<BindFn>(profile::BindTab)(root,reinterpret_cast<Ptr>(&bindingError),
        reinterpret_cast<Ptr>(&bar),base+profile::TabSelectorKey);
    if(bindingError || !bar || !value(bar,vt) || vt!=base+profile::TabVtable ||
       !value(root+0x1f8,stock) || !stock) {
        char message[220];
        _snprintf_s(message,sizeof(message),_TRUNCATE,
            "BIND REFUSED: error=%u root=%p bar=%p vtable=%p stock=%p",bindingError,
            reinterpret_cast<void*>(root),reinterpret_cast<void*>(bar),reinterpret_cast<void*>(vt),reinterpret_cast<void*>(stock));
        log(message); return returned;
    }
#ifdef ALDEN_RUNTIME_UI
    if(!category_controls_ready()) {
        function<FillFn>(profile::Hide)(bar);
        *slot={};
        log("INIT BYPASS: category styles unavailable; tabs hidden and complete native shop list retained");
        return returned;
    }
#endif
    *slot={root,stock,bar,0,false,false,GetCurrentThreadId(),0};
    show_bar(*slot,false);
    char message[160];
    _snprintf_s(message,sizeof(message),_TRUNCATE,
        "BOUND: native category TabBar attached; loader_thread=%lu; awaiting first shop fill",slot->loaderThread);
    log(message);
    return returned;
}
Ptr __fastcall destroy_hook(Ptr root,unsigned flags) {
    if(ready.load()) {
        const std::lock_guard<std::recursive_mutex> lock(stateMutex);
        if(auto* s=by_root(root)) *s={};
    }
    return originalDestroy(root,flags);
}

unsigned char __fastcall count_hook(Ptr stock,unsigned count) {
    if(!ready.load()) return originalCount(stock,count);
    std::unique_lock<std::recursive_mutex> lock(stateMutex);
    auto* s=by_stock(stock);
    if(!s || !on_ui_thread(*s) || s->tab==0 || s->fault || !is_alden(stock)) {
        lock.unlock(); return originalCount(stock,count);
    }
    Ptr entries{}; unsigned actual{},capacity{},depthRows{};
    Row rows[4096];
    bool valid=count<=4096 && value(stock+0x180,entries) && value(stock+0x188,actual) &&
        value(stock+0x18c,capacity) && actual==count && capacity>=count &&
        value(stock+0x170,depthRows) && depthRows==0;
    for(unsigned n=0;valid && n<count;++n) {
        Ptr record{},description{}; std::uint8_t type{}; std::uint16_t index{};
        valid=value(entries+n*sizeof(Ptr),record) && record && value(record+0x38,description) && description &&
            value(description+0x60,type) && type==0 && value(description+0x38,index) && index!=0xffff;
        if(!valid) break;
        const auto info=function<InfoFn>(profile::ItemInfo)(0,index);
        rows[n].stock=record;
        valid=value(info+8,rows[n].item) && value(record+0x28,rows[n].index) && category(rows[n])!=255;
    }
    std::size_t selected{};
    if(!valid || !select_rows(rows,count,s->tab,reinterpret_cast<Ptr*>(entries),capacity,selected)) {
        s->fault=true; s->tab=0;
        const bool wasBusy=s->busy;
        s->busy=true;
        select_tab(*s,0);
        s->busy=wasBusy;
        log("REFUSED: unfamiliar stock layout/catalog; complete native assortment retained");
        lock.unlock(); return originalCount(stock,count);
    }
    *reinterpret_cast<unsigned*>(stock+0x188)=static_cast<unsigned>(selected);
    // The grid count setter clamps scrolling. Keep its focused item in bounds too.
    unsigned focus{};
    if(value(stock+0x120,focus) && (selected==0 || focus>=selected))
        *reinterpret_cast<unsigned*>(stock+0x120)=0;
    char text[150];
    _snprintf_s(text,sizeof(text),_TRUNCATE,"FILTER: tab=%u native_rows=%u visible_rows=%zu",s->tab,count,selected);
    log(text);
    lock.unlock(); return originalCount(stock,static_cast<unsigned>(selected));
}

void __fastcall fill_hook(Ptr root) {
    if(!ready.load()) { originalFill(root); return; }
    std::unique_lock<std::recursive_mutex> lock(stateMutex);
    State* s=by_root(root);
    if(!s) { lock.unlock(); originalFill(root); return; }
    if(!s->ownerThread) {
        s->ownerThread=GetCurrentThreadId();
        char message[160];
        _snprintf_s(message,sizeof(message),_TRUNCATE,
            "UI THREAD: loader=%lu update=%lu; ownership set by first shop fill",s->loaderThread,s->ownerThread);
        log(message);
    }
    if(!on_ui_thread(*s)) {
        log("FILL REFUSED: update thread changed after first fill");
        lock.unlock(); originalFill(root); return;
    }
    s->tab=0; s->fault=false; s->busy=true; value(root+0x1f8,s->stock);
    originalFill(root);
    unsigned headers{};
    const bool active=value(s->bar+0xd0,headers) && headers==TabCount && is_alden(s->stock);
    if(active) select_tab(*s,0);
    show_bar(*s,active);
    s->busy=false;
    log(active ? "OPEN: Alden, nine tabs including Original, default All" : "HIDDEN: another merchant or unsupported TabBar");
}

void __fastcall tab_hook(Ptr root,Ptr bar,bool preview,unsigned previous,unsigned selected,Ptr header) {
    if(!ready.load()) { originalTab(root,bar,preview,previous,selected,header); return; }
    std::unique_lock<std::recursive_mutex> lock(stateMutex);
    auto* s=by_root(root);
    if(!s || !on_ui_thread(*s) || s->bar!=bar) {
        lock.unlock(); originalTab(root,bar,preview,previous,selected,header); return;
    }
    if(preview || s->busy || selected>=TabCount || !is_alden(s->stock)) return;
    if(s->tab==selected && !s->fault) return;
    s->tab=selected; s->fault=false; s->busy=true;
    // Use the game's existing full list/focus/tooltip refresh. Its stock count
    // callback filters the original records before any visible rows are built.
    originalFill(root);
    if(s->fault) select_tab(*s,0);
    s->busy=false;
}

bool mounted_ui(const std::wstring& directory) {
    const auto manifest=directory+L"dmm\\manifest.tsv";
    HANDLE f=CreateFileW(manifest.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,
                         OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(f==INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    if(!GetFileSizeEx(f,&size) || size.QuadPart<1 || size.QuadPart>1048576) { CloseHandle(f); return false; }
    std::string contents(static_cast<std::size_t>(size.QuadPart),'\0'); DWORD got{};
    const bool ok=ReadFile(f,contents.data(),static_cast<DWORD>(contents.size()),&got,nullptr)!=FALSE;
    CloseHandle(f);
    if(!ok || got!=contents.size()) return false;
    unsigned seen=0;
    for(std::size_t start=0;start<contents.size();) {
        auto end=contents.find('\n',start);
        auto line=contents.substr(start,end==std::string::npos ? end : end-start);
        start=end==std::string::npos ? contents.size() : end+1;
        if(!line.empty() && line.back()=='\r') line.pop_back();
        const char* expected=nullptr; unsigned bit=0; std::size_t offset=0;
        const std::string html="file\tui/xml/gamemain/play/npcstorebuyview2.html\t";
#ifdef ALDEN_RUNTIME_UI
        const std::string css="file\tui/xml/gamemain/play/aldencategorytabs.css\t";
#else
        const std::string css="file\tui/xml/gamemain/play/npcstore2.css\t";
#endif
        const std::string binary="file\tui/bin__/npcstorebuyview2_game.uibinary\t";
#ifdef ALDEN_RUNTIME_UI
        // No HTML/binary replacement is required. When a compiled buy UI is
        // mounted, it must be one of the supported unmodified game/BI inputs.
        if(line.compare(0,css.size(),css)==0) { expected=runtime::CssHash; bit=2; offset=css.size(); }
        if(line.compare(0,binary.size(),binary)==0) { expected=runtime::VanillaInputHash; bit=4; offset=binary.size(); }
#else
        if(line.compare(0,html.size(),html)==0) { expected=profile::HtmlHash; bit=1; offset=html.size(); }
        if(line.compare(0,css.size(),css)==0) { expected=profile::CssHash; bit=2; offset=css.size(); }
        if(line.compare(0,binary.size(),binary)==0) { expected=profile::BinaryUiHash; bit=4; offset=binary.size(); }
#endif
        if(!expected) continue;
        if(seen&bit) return false;
        wchar_t path[32768]{}; char digest[65]{};
        if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,line.c_str()+offset,-1,path,32768) ||
           !mh067::Sha256File(path,digest)) return false;
#ifdef ALDEN_RUNTIME_UI
        if(std::strcmp(expected,digest) && !(bit==4 && !std::strcmp(digest,runtime::BetterInputHash))) return false;
#else
        if(std::strcmp(expected,digest)) return false;
#endif
        seen|=bit;
    }
#ifdef ALDEN_RUNTIME_UI
    return (seen&2)!=0;
#else
    return seen==7;
#endif
}
bool exchange(Ptr slot,Ptr expected,Ptr desired) {
    if(slot%sizeof(Ptr)) return false;
    DWORD old{};
    if(!VirtualProtect(reinterpret_cast<void*>(slot),sizeof(Ptr),PAGE_READWRITE,&old)) return false;
    const auto before=InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(slot),
        reinterpret_cast<void*>(desired),reinterpret_cast<void*>(expected));
    DWORD unused{};
    if(!VirtualProtect(reinterpret_cast<void*>(slot),sizeof(Ptr),old,&unused)) log("WARNING: vtable protection restore failed");
    return reinterpret_cast<Ptr>(before)==expected;
}
DWORD WINAPI initialize(void*) {
    wchar_t self[32768]{},exe[32768]{};
    if(!GetModuleFileNameW(module,self,32768) || !GetModuleFileNameW(nullptr,exe,32768)) return 1;
    const auto logPath=std::wstring(self)+L".log";
    logfile=CreateFileW(logPath.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,
                        CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
#ifdef ALDEN_RUNTIME_UI
    log(runtime::Version);
#else
    log("Alden Category Tabs v17.1 / Steam 26361232 / single-step keyboard and controller tabs");
#endif
    const auto leaf=wcsrchr(exe,L'\\');
    if(!leaf || _wcsicmp(leaf+1,L"CrimsonDesert.exe")) { log("REFUSED: wrong process"); return 2; }
    char hash[65]{};
    if(!mh067::Sha256File(exe,hash) || std::strcmp(hash,profile::ExeHash)) { log("REFUSED: unsupported game build"); return 3; }
    const std::wstring directory(exe,static_cast<std::size_t>(leaf-exe+1));
    if(!mounted_ui(directory)) {
#ifdef ALDEN_RUNTIME_UI
        log("REFUSED: own category CSS missing/different or legacy/unsupported buy UI overlay; remove old Alden UI files");
#else
        log("REFUSED: category HTML/CSS/compiled UI missing or different");
#endif
        return 4;
    }
    base=reinterpret_cast<Ptr>(GetModuleHandleW(nullptr));
    for(const auto& guard:profile::Guards) {
        unsigned char bytes[16]{};
        if(!read(base+guard.rva,bytes,sizeof(bytes)) || std::memcmp(bytes,guard.bytes,sizeof(bytes))) {
            log("REFUSED: native function guard mismatch"); return 5;
        }
    }
    originalInit=function<InitFn>(profile::Init); originalDestroy=function<DestroyFn>(profile::Destroy);
    originalFill=function<FillFn>(profile::Fill); originalTab=function<TabFn>(profile::TabChanged);
    originalCount=function<CountFn>(profile::SetCount);
    struct Hook { Ptr slot,expected,replacement; };
    const Hook hooks[]={
        {base+profile::RootVtable+8,base+profile::Init,reinterpret_cast<Ptr>(&init_hook)},
        {base+profile::RootVtable,base+profile::Destroy,reinterpret_cast<Ptr>(&destroy_hook)},
        {base+profile::RootVtable+27*8,base+profile::Fill,reinterpret_cast<Ptr>(&fill_hook)},
        {base+profile::RootVtable+114*8,base+profile::TabChanged,reinterpret_cast<Ptr>(&tab_hook)},
        {base+profile::StockVtable+181*8,base+profile::SetCount,reinterpret_cast<Ptr>(&count_hook)},
    };
    for(const auto& h:hooks) { Ptr current{}; if(!value(h.slot,current) || current!=h.expected) {
        log("REFUSED: vtable already changed"); return 6; } }
    HMODULE pinned{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&init_hook),&pinned)) return 7;
#ifdef ALDEN_RUNTIME_UI
    if(!prepare_runtime_hook()) return 9;
#endif
    std::size_t installed=0;
    for(const auto& h:hooks) {
        if(!exchange(h.slot,h.expected,h.replacement)) {
            while(installed) { const auto& prior=hooks[--installed]; exchange(prior.slot,prior.replacement,prior.expected); }
#ifdef ALDEN_RUNTIME_UI
            discard_runtime_hook();
#endif
            log("REFUSED: concurrent hook change; previous hooks rolled back"); return 8;
        }
        ++installed;
    }
#ifdef ALDEN_RUNTIME_UI
    if(!enable_runtime_hook()) {
        while(installed) { const auto& prior=hooks[--installed]; exchange(prior.slot,prior.replacement,prior.expected); }
        discard_runtime_hook();
        log("REFUSED: UI decoder hook activation failed; callback hooks rolled back"); return 10;
    }
#endif
    ready=true;
    log("ARMED: five UI callbacks installed; waiting for Alden buy screen");
    return 0;
}
}

#ifndef ALDEN_TAB_TESTING
BOOL APIENTRY DllMain(HMODULE self,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {
        alden::module=self;
        DisableThreadLibraryCalls(self);
        HANDLE thread=CreateThread(nullptr,0,alden::initialize,nullptr,0,nullptr);
        if(thread) CloseHandle(thread);
    }
    return TRUE;
}
#endif
