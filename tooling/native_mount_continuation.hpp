#pragma once
// Native b26636972 post-read comparison and UI notification; no game process calls.
static unsigned notifiedCount;
static U64 notifiedValue;
static void __fastcall record_notification(void*,U64 current,U64 maximum,U64 step,U64 minimum,bool,bool) {
    if(maximum!=5 || step!=1 || minimum!=0) std::abort();
    ++notifiedCount;notifiedValue=current;
}
static bool test_native_mount_notification() {
    #include "native_mount_code.generated.hpp"
    void* executable=VirtualAlloc(nullptr,sizeof(code),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!executable)return false;memcpy(executable,code,sizeof(code));DWORD previous{};
    if(!VirtualProtect(executable,sizeof(code),PAGE_EXECUTE_READ,&previous))return false;
    FlushInstructionCache(GetCurrentProcess(),executable,sizeof(code));
    using Fn=void(__fastcall*)(void*,U64,U64);const auto run=reinterpret_cast<Fn>(executable);
    U64 vtable[0xa8]{};vtable[0x538/8]=reinterpret_cast<U64>(&record_notification);
    U8 actor[0x300]{},context[0x500]{},target=0xCC;wr64(actor,reinterpret_cast<U64>(vtable));
    U8* field=actor+0x230;wr64(field,4000);wr64(field+8,5000);wr64(field+24,1000);
    field[0x25]=3;field[0x26]=15;*reinterpret_cast<U16*>(field+0x28)=50;
    setctx64(context,0xE8,reinterpret_cast<U64>(actor));setctx64(context,0x88,4000);setctx64(context,0x80,5000);
    EXCEPTION_RECORD_MIN exception{};exception.ExceptionCode=STATUS_BREAKPOINT;exception.ExceptionAddress=&target;
    EXCEPTION_POINTERS_MIN pointers{&exception,context};g_target=&target;g_original=0x4d;g_flush=0;g_singleStep=0;
    if(veh_handler(&pointers)!=EXCEPTION_CONTINUE_EXECUTION)return false;
    notifiedCount=0;notifiedValue=0;
    run(actor,ctx64(context,0x88),ctx64(context,0x80));
    const bool updated=notifiedCount==1 && notifiedValue==5;
    run(actor,5000,5000);
    const bool stable=notifiedCount==1;
    VirtualFree(executable,0,MEM_RELEASE);
    return updated && stable;
}
