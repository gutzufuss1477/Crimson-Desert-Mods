// Healthbar NativeFix2: distinct framed/compact UI variants; same tested native patch.
// Copyright (c) 2026 Blablup. MIT.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include "hash_core.hpp"
#include "../../../../tooling/native/healthbar_26949954.hpp"

namespace healthbar {
uintptr_t UpdateRva=0xedfc10, SlotRva=0x59ea750, HeadUpVtableRva=0x59ec0e8;
uintptr_t ContextRva=0xf84aaf, WordRva=0xf84ab8;
constexpr uint8_t Context[]={0x8b,0x43,0x04,0x39,0x86,0xa4,0x01,0x00,0x00,0x75,0x1f,
    0x48,0x8b,0x5b,0x08,0x48,0x8b,0x8b,0xe0,0x00,0x00,0x00,0x48,0x85,0xc9,0x74,0x21};
constexpr uint8_t UpdateHead[]={0x48,0x89,0x5c,0x24,0x08,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};
cd::Addresses addresses;
enum class UiMode { unsupported, single, compact, framed };
constexpr char singleHash[]="29f93032c459a0c96a40e573a54f0e0954109ae7e64ff538659a0e6b5a09f9a7";
constexpr char framedHash[]="77f883a8f45b0784d514b8cb2d3eaf791a1a17f1fed0b040ce12cabd57e84a79";
constexpr char compactHash[]="2340961d36f1e14a4556c996eefc38d6b5d4bc06c7a1f303af40e9c0c7523336";
UiMode identify_ui(const char* html, const char* css) {
    if (*css) return UiMode::unsupported;
    if (std::strcmp(html,singleHash)==0) return UiMode::single;
    if (std::strcmp(html,framedHash)==0) return UiMode::framed;
    if (std::strcmp(html,compactHash)==0) return UiMode::compact;
    return UiMode::unsupported;
}
using Update=void(__fastcall*)(uintptr_t);
HMODULE self{};
uintptr_t base{};
Update original{};
HANDLE logfile=INVALID_HANDLE_VALUE;
SRWLOCK loglock=SRWLOCK_INIT;
std::atomic<int> patchState{}; // 0 awaiting first UI tick, 1 applying, 2 active, -1 refused
std::atomic<uint64_t> frames{};

void log(const char* message) {
    if(logfile==INVALID_HANDLE_VALUE) return;
    AcquireSRWLockExclusive(&loglock);
    char line[1024];
    const int n=_snprintf_s(line,sizeof(line),_TRUNCATE,"[%llu] %s\r\n",GetTickCount64(),message);
    if(n>0) { DWORD written{}; WriteFile(logfile,line,static_cast<DWORD>(n),&written,nullptr); FlushFileBuffers(logfile); }
    ReleaseSRWLockExclusive(&loglock);
}
bool read(uintptr_t p,void* dest,size_t bytes) {
    if(!p) return false;
    __try { std::memcpy(dest,reinterpret_cast<const void*>(p),bytes); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
template<class T> bool value(uintptr_t p,T& result) { return read(p,&result,sizeof(result)); }

// One aligned eight-byte CAS changes only the two-byte JNE to NOP/NOP.
// No torn branch displacement; the six adjacent instruction bytes stay intact.
bool patch_word(uintptr_t word) {
    if(word%8) return false;
    uint64_t expected{},replacement{};
    std::memcpy(&expected,Context+(WordRva-ContextRva),sizeof(expected));
    replacement=(expected & ~uint64_t{0xffff}) | uint64_t{0x9090};
    uint64_t current{};
    if(!value(word,current) || current!=expected) return false;
    DWORD oldProtect{};
    if(!VirtualProtect(reinterpret_cast<void*>(word),8,PAGE_EXECUTE_READWRITE,&oldProtect)) return false;
    const auto before=InterlockedCompareExchange64(reinterpret_cast<volatile LONG64*>(word),
        static_cast<LONG64>(replacement),static_cast<LONG64>(expected));
    const bool changed=static_cast<uint64_t>(before)==expected;
    if(changed) FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(word),8);
    DWORD unused{};
    if(!VirtualProtect(reinterpret_cast<void*>(word),8,oldProtect,&unused)) log("WARNING: protection restore failed");
    return changed;
}

void snapshot(uintptr_t root,uint64_t frame) {
    // Only primitive UI state, on the original UI update thread. No actor-memory
    // retention, extra target creation, save data or gameplay-stat writes.
    uint32_t count{},target{},shown{},active{};
    uintptr_t nodes{};
    uint8_t enabled{};
    if(!value(root+0x174,count) || count>4096 || !value(root+0x188,nodes) ||
       !value(root+0x1a4,target) || !value(root+0x194,enabled)) return;
    for(uint32_t i=0;i<count && i<256;++i) {
        uintptr_t node{},head{},vt{},hp{};
        uint8_t inactive{},show{},hide{};
        if(!value(nodes+i*8,node) || !value(node+8,head) || !value(head,vt) ||
           vt!=base+HeadUpVtableRva || !value(head+0x116,inactive) ||
           !value(head+0xe0,hp) || !hp || !value(hp+0xbd,show) || !value(hp+0xbe,hide)) continue;
        if(!inactive) ++active;
        if(show && !hide) ++shown;
    }
    static uint32_t lastTarget=0xffffffff, lastShown=0xffffffff, lines{};
    static uint64_t lastTime{};
    const uint64_t now=GetTickCount64();
    if(lines>=160 || (target==lastTarget && shown==lastShown && now-lastTime<5000)) return;
    lastTarget=target; lastShown=shown; lastTime=now; ++lines;
    char text[256];
    _snprintf_s(text,sizeof(text),_TRUNCATE,
        "UI frame=%llu target=%u entries=%u active_hp=%u requested_shown=%u enable_bits=%u patch=%d",
        frame,target,count,active,shown,static_cast<unsigned>(enabled),patchState.load());
    log(text);
}

void __fastcall update_hook(uintptr_t root) {
    int expected=0;
    if(patchState.compare_exchange_strong(expected,1)) {
        uint8_t bytes[sizeof(Context)]{};
        const bool ok=read(base+ContextRva,bytes,sizeof(bytes)) &&
            std::memcmp(bytes,Context,sizeof(bytes))==0 && patch_word(base+WordRva);
        patchState=ok ? 2 : -1;
        log(ok ? "ACTIVE: current-target-only branch removed on UI thread (2 bytes)" :
                 "REFUSED: live branch guard or atomic patch failed; original behavior retained");
    }
    original(root); // Exactly once, with all native eligibility and cleanup paths.
    snapshot(root,frames.fetch_add(1)+1);
}

bool matching_file(const wchar_t* path,const char* expected) {
    char hash[65]{};
    return mh067::Sha256File(path,hash) && std::strcmp(hash,expected)==0;
}
UiMode mounted_ui(const std::wstring& directory) {
    const std::wstring manifest=directory+L"dmm\\manifest.tsv";
    HANDLE f=CreateFileW(manifest.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,
        OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(f==INVALID_HANDLE_VALUE) return UiMode::unsupported;
    char text[65537]{}; DWORD got{};
    const bool ok=ReadFile(f,text,65536,&got,nullptr)!=FALSE;
    CloseHandle(f);
    if(!ok || got==65536) return UiMode::unsupported;
    std::string contents(text,got);
    char htmlHash[65]{},cssHash[65]{};
    for(size_t start=0;start<contents.size();) {
        const size_t end=contents.find('\n',start);
        std::string line=contents.substr(start,end==std::string::npos ? end : end-start);
        start=end==std::string::npos ? contents.size() : end+1;
        if(!line.empty() && line.back()=='\r') line.pop_back();
        char* digest=nullptr;
        const std::string html="file\tui/xml/gamemain/play/subtitletagview.html\t";
        const std::string css="file\tui/xml/gamemain/play/subtitletagview.css\t";
        size_t offset{};
        if(line.compare(0,html.size(),html)==0) { digest=htmlHash; offset=html.size(); }
        if(line.compare(0,css.size(),css)==0) { digest=cssHash; offset=css.size(); }
        if(!digest) continue;
        if(digest[0]) return UiMode::unsupported; // Ambiguous duplicate target.
        wchar_t path[32768]{};
        if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,line.c_str()+offset,-1,path,32768) ||
           !mh067::Sha256File(path,digest)) return UiMode::unsupported;
    }
    return identify_ui(htmlHash,cssHash);
}

DWORD WINAPI initialize(void*) {
    wchar_t modulePath[32768]{},exe[32768]{};
    if(!GetModuleFileNameW(self,modulePath,32768) || !GetModuleFileNameW(nullptr,exe,32768)) return 1;
    const std::wstring logPath=std::wstring(modulePath)+L".log";
    logfile=CreateFileW(logPath.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,
        CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    log("Healthbar v5 test; structural resolver; baseline Steam 26636972");
    const wchar_t* leaf=wcsrchr(exe,L'\\');
    if(!leaf || _wcsicmp(leaf+1,L"CrimsonDesert.exe")) {
        log("REFUSED: wrong executable name"); return 2;
    }
    const std::wstring directory(exe,static_cast<size_t>(leaf-exe+1));
    const UiMode mode=mounted_ui(directory);
    if(mode==UiMode::unsupported) { log("REFUSED: mounted UI hashes not supported"); return 3; }
    if(mode==UiMode::single) { log("CLASSIC SINGLE TARGET: no native patch or hook installed"); return 0; }
    log(mode==UiMode::framed ? "MODE: Vanilla Multitarget, classic framed bars" : "MODE: Caites/legacy UIFix2, compact thin bars");
    base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    cd::Image image;
    if(!cd::loaded_image(image) || !addresses.resolve(image,cd::healthbar_plan)) {
        log(addresses.failure);return 4;
    }
    UpdateRva=addresses.rva(0xedfc10);SlotRva=addresses.rva(0x59ea750);
    HeadUpVtableRva=addresses.rva(0x59ec0e8);ContextRva=addresses.rva(0xedfdca);
    WordRva=addresses.rva(0xedfdd0);

    uint8_t bytes[sizeof(Context)]{},head[sizeof(UpdateHead)]{};
    uintptr_t slot{};
    if(!read(base+ContextRva,bytes,sizeof(bytes)) || std::memcmp(bytes,Context,sizeof(bytes)) ||
       !read(base+UpdateRva,head,sizeof(head)) || std::memcmp(head,UpdateHead,sizeof(head)) ||
       !value(base+SlotRva,slot) || slot!=base+UpdateRva) {
        log("REFUSED: native code/vtable guards differ"); return 4;
    }
    HMODULE pinned{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&update_hook),&pinned)) { log("REFUSED: module pin failed"); return 5; }
    original=reinterpret_cast<Update>(slot);
    DWORD protection{};
    if(!VirtualProtect(reinterpret_cast<void*>(base+SlotRva),8,PAGE_READWRITE,&protection)) {
        log("REFUSED: vtable protection failed"); return 6;
    }
    void* before=InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(base+SlotRva),
        reinterpret_cast<void*>(&update_hook),reinterpret_cast<void*>(slot));
    DWORD unused{};
    if(!VirtualProtect(reinterpret_cast<void*>(base+SlotRva),8,protection,&unused)) log("WARNING: vtable protection restore failed");
    if(before!=reinterpret_cast<void*>(slot)) { log("REFUSED: vtable changed concurrently"); return 7; }
    log("ARMED: structural/code/vtable/UI guards passed; waiting for the first root UI update");
    Sleep(45000);
    char status[160];
    _snprintf_s(status,sizeof(status),_TRUNCATE,"45-second check: UI calls=%llu patch=%d",frames.load(),patchState.load());
    log(status);
    return 0;
}
} // namespace healthbar

#ifndef HEALTHBAR_TESTING
BOOL APIENTRY DllMain(HMODULE module,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {
        healthbar::self=module;
        DisableThreadLibraryCalls(module);
        HANDLE thread=CreateThread(nullptr,0,healthbar::initialize,nullptr,0,nullptr);
        if(thread) CloseHandle(thread);
    }
    return TRUE;
}
#endif
