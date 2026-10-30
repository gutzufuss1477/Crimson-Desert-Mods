#pragma once
#include <windows.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace ore_range {
inline constexpr uintptr_t ReaderRva=0x1710f60,LevelOffset=0x3c,DefaultOffset=0xd;
inline constexpr std::array<uint8_t,16> ReaderHead={0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x70,0x20,0x57,0x48,0x83,0xec,0x60};
using Reader=uint8_t (__fastcall*)(void*,void*);
inline Reader original{};
inline std::atomic<unsigned> changed{},refused{};
template<class Read> bool guard(uintptr_t base,Read read) {
    std::array<uint8_t,16> head{};
    return read(base+ReaderRva,head.data(),head.size()) && head==ReaderHead;
}
inline bool eligible(uint32_t key,std::string_view name,int32_t level,uint8_t useDefault) {
    return key==1708 && name=="Item_Collect_Ore" && level==5 && useDefault==0;
}
// Proven reader layout for the hash-guarded Steam 2.04.00 executable. The caller
// publishes the new record only after this reader returns. Never store a record
// pointer or write an already published record from the INI worker.
inline void extend(void* output) {
    __try {
        const auto record=reinterpret_cast<uint8_t*>(output);
        if(!record || *reinterpret_cast<uint32_t*>(record+8)!=1708)return;
        const auto stringObject=*reinterpret_cast<const char* const* const*>(record);
        if(!stringObject || !*stringObject) { ++refused;return; }
        const char* name=*stringObject;
        constexpr char expected[]="Item_Collect_Ore";
        // Include the terminating zero; prefixes/suffixes must not pass.
        if(std::memcmp(name,expected,sizeof expected)!=0) { ++refused;return; }
        auto* level=reinterpret_cast<int32_t*>(record+LevelOffset);
        if(!eligible(1708,expected,*level,record[DefaultOffset])) { ++refused;return; }
        *level=7; // Native capacity 128, covering every supported INI range 10..120.
        ++changed;
    } __except(EXCEPTION_EXECUTE_HANDLER) { ++refused; }
}
inline uint8_t __fastcall onRead(void* stream,void* output) {
    const auto result=original(stream,output);
    if(result)extend(output);
    return result;
}
}
