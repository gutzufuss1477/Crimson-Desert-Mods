// Bank Refresh 1.0.0 - original mod code, Copyright (c) 2026 Blablup, MIT.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <intrin.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include "bank_core.hpp"
#include "hash_core.hpp"
#include "vendor/minhook/include/MinHook.h"

namespace {
using namespace bank_refresh;
constexpr uintptr_t RefreshRva=0x2b5ff30, LookupRva=0x9cb1d0;
constexpr uintptr_t LockRva=0x393650, NotifyRva=0x2b571f0, TimerReturnRva=0x2c3b385;
struct Guard { uintptr_t rva; uint8_t bytes[16]; };
constexpr Guard Guards[] = {
    {RefreshRva,{0x48,0x8b,0xc4,0x66,0x44,0x89,0x48,0x20,0x4c,0x89,0x40,0x18,0x48,0x89,0x50,0x10}},
    {LookupRva,{0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x56,0x57,0x41,0x56,0x48,0x83}},
    {LockRva,{0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0x89,0x11,0x48,0x8b,0xda}},
    {NotifyRva,{0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x56,0x57,0x41,0x56,0x48,0x83}},
    {0x2c3b380,{0xe8,0xab,0x4b,0xf2,0xff,0x48,0x83,0xc3,0x02,0x48,0x3b,0xde,0x75,0xc4,0x48,0x8d}}
};
using RefreshFn=int*(__fastcall*)(uintptr_t,int*,const Calendar*,uint16_t);
using LookupFn=uintptr_t(__fastcall*)(const uint16_t*);
using LockFn=void*(__fastcall*)(void*,uintptr_t,uint8_t);
using UnlockFn=void(__fastcall*)(uintptr_t,uint8_t);
using NotifyFn=void(__fastcall*)(uintptr_t,uint16_t);
HMODULE module{};
uintptr_t base{};
RefreshFn original{};
std::atomic<bool> faulted{};
uint32_t intervalGameMinutes=15;

// Values are returned only to the initialization thread. The production ASI has
// no exported test API, diagnostic files, logging queue or heartbeat thread.
enum class InitResult : DWORD {
    pending=0, active=1, path_failed=2, wrong_process=3, missing_ini=4,
    disabled=5, invalid_interval=6, legacy_diagnostic=7, hash_failed=8,
    unknown_build=9, byte_mismatch=10, pin_failed=11, hook_init_failed=12,
    hook_create_failed=13, hook_enable_failed=14
};
#ifdef BANK_REFRESH_TESTING
std::atomic<InitResult> testInitResult{InitResult::pending};
#endif

bool read(const void* p,void* out,size_t size) {
    if(!p) return false;
    __try { std::memcpy(out,p,size); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
template<class T> bool value(uintptr_t p,T& out) { return read(reinterpret_cast<void*>(p),&out,sizeof(out)); }
bool executable(uintptr_t p) {
    MEMORY_BASIC_INFORMATION m{};
    if(!p || !VirtualQuery(reinterpret_cast<void*>(p),&m,sizeof(m)) ||
        m.State!=MEM_COMMIT || (m.Protect&PAGE_GUARD)) return false;
    return (m.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))!=0;
}
bool writable_records(uintptr_t address,uint32_t count,size_t& bytes) {
    if(!record_span(address,count,bytes)) return false;
    MEMORY_BASIC_INFORMATION m{};
    if(!VirtualQuery(reinterpret_cast<void*>(address),&m,sizeof(m)) || m.State!=MEM_COMMIT ||
        (m.Protect&PAGE_GUARD) || !(m.Protect&(PAGE_READWRITE|PAGE_WRITECOPY))) return false;
    const auto region=reinterpret_cast<uintptr_t>(m.BaseAddress);
    return address>=region && address-region<m.RegionSize && bytes<=m.RegionSize-(address-region);
}

// Same engine-thread/lock path as the in-game validated prototype.
bool locked_adjust(uintptr_t component,uint16_t key,uint64_t now,uint64_t limit) {
    uintptr_t actor{},lock{},vtable{},acquire{},release{};
    if(!value(component+8,actor) || !actor || !value(actor+8,lock) || !lock ||
        !value(lock,vtable) || !value(vtable+0x18,acquire) || !value(vtable+0x20,release) ||
        !executable(acquire) || !executable(release)) return false;
    uintptr_t guard[2]{};
    reinterpret_cast<LockFn>(base+LockRva)(guard,lock,0);
    bool changed=false;
    __try {
        __try {
            auto records=*reinterpret_cast<BankRecord**>(component+0x110);
            const auto count=*reinterpret_cast<uint32_t*>(component+0x118);
            size_t bytes{};
            BankRecord snapshot[32]{};
            uint32_t index{};
            if(writable_records(reinterpret_cast<uintptr_t>(records),count,bytes) &&
                read(records,snapshot,bytes) && unique_record(snapshot,count,key,index)) {
                const auto before=snapshot[index].next;
                const auto proposed=cap_game_deadline(before,now,limit);
                if(proposed!=before) {
                    records[index].next=proposed;
                    changed=true;
                }
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) { faulted=true; }
    } __finally { reinterpret_cast<UnlockFn>(release)(lock,0); }
    return changed;
}

int* __fastcall refresh_hook(uintptr_t component,int* result,const Calendar* calendar,uint16_t key) {
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-base;
    int* returned=original(component,result,calendar,key);
    if(faulted || caller!=TimerReturnRva) return returned;
    Calendar date{};
    uint64_t now{},limit{};
    int nativeResult{};
    if(!read(calendar,&date,sizeof(date)) || !calendar_ticks(date,now) ||
        !game_deadline(date,intervalGameMinutes,limit) ||
        !read(returned,&nativeResult,sizeof(nativeResult)) || nativeResult!=0) return returned;
    const auto info=reinterpret_cast<LookupFn>(base+LookupRva)(&key);
    uintptr_t textHolder{},textData{};
    uint8_t kind=255;
    char name[8]{};
    if(!info || !value(info+8,textHolder) || !value(textHolder,textData) ||
        !read(reinterpret_cast<void*>(textData),name,sizeof(name)) ||
        !value(info+0x78,kind) || !eligible(name,kind)) return returned;
    if(locked_adjust(component,key,now,limit)) reinterpret_cast<NotifyFn>(base+NotifyRva)(component,key);
    return returned;
}

InitResult initialize_impl() {
    wchar_t directory[32768]{},iniPath[32768]{},exe[32768]{};
    const DWORD n=GetModuleFileNameW(module,directory,32768);
    if(!n || n>=32768) return InitResult::path_failed;
    auto slash=wcsrchr(directory,L'\\');
    if(!slash) return InitResult::path_failed;
    slash[1]=0;
    if(swprintf_s(iniPath,32768,L"%sBank_Refresh.ini",directory)<=0) return InitResult::path_failed;
    const DWORD len=GetModuleFileNameW(nullptr,exe,32768);
    const auto file=wcsrchr(exe,L'\\');
    if(!len || len>=32768 || !file || _wcsicmp(file+1,L"CrimsonDesert.exe")!=0) return InitResult::wrong_process;
    if(GetFileAttributesW(iniPath)==INVALID_FILE_ATTRIBUTES) return InitResult::missing_ini;
    if(GetPrivateProfileIntW(L"BankRefresh",L"Enabled",0,iniPath)!=1) return InitResult::disabled;
    wchar_t setting[64]{};
    const DWORD size=GetPrivateProfileStringW(L"BankRefresh",L"IntervalGameMinutes",L"",setting,64,iniPath);
    if(size>=63 || !parse_game_minutes(setting,intervalGameMinutes)) return InitResult::invalid_interval;
    // Do not silently activate an old prototype INI that explicitly requested observe-only mode.
    if(GetPrivateProfileIntW(L"BankRefresh",L"DiagnosticOnly",0,iniPath)!=0) return InitResult::legacy_diagnostic;
    char hash[65]{};
    if(!mh067::Sha256File(exe,hash)) return InitResult::hash_failed;
    if(std::strcmp(hash,ExpectedSha)!=0) return InitResult::unknown_build;
    base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    for(const auto& g:Guards) {
        uint8_t actual[16]{};
        if(!value(base+g.rva,actual) || std::memcmp(actual,g.bytes,16)!=0) return InitResult::byte_mismatch;
    }
    HMODULE pinned{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&initialize_impl),&pinned)) return InitResult::pin_failed;
    if(MH_Initialize()!=MH_OK) return InitResult::hook_init_failed;
    auto target=reinterpret_cast<void*>(base+RefreshRva);
    if(MH_CreateHook(target,reinterpret_cast<void*>(&refresh_hook),reinterpret_cast<void**>(&original))!=MH_OK) {
        MH_Uninitialize(); return InitResult::hook_create_failed;
    }
    if(MH_EnableHook(target)!=MH_OK) {
        MH_RemoveHook(target); MH_Uninitialize(); return InitResult::hook_enable_failed;
    }
    return InitResult::active;
}
DWORD WINAPI initialize(void*) {
    const auto result=initialize_impl();
#ifdef BANK_REFRESH_TESTING
    testInitResult=result;
#endif
    return static_cast<DWORD>(result);
}
}
#ifdef BANK_REFRESH_TESTING
extern "C" __declspec(dllexport) DWORD BankRefresh_TestInitResult() {
    return static_cast<DWORD>(testInitResult.load());
}
#endif
BOOL APIENTRY DllMain(HMODULE handle,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {
        module=handle;
        DisableThreadLibraryCalls(handle);
        HANDLE worker=CreateThread(nullptr,0,initialize,nullptr,0,nullptr);
        if(worker) CloseHandle(worker);
    }
    return TRUE;
}
