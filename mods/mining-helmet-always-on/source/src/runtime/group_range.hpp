#pragma once
#include <windows.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>

namespace ore_range {
inline constexpr uintptr_t ReaderRva=0x1711b10,LevelOffset=0x38,DefaultOffset=9;
inline constexpr std::array<uint8_t,16> ReaderHead={0x48,0x8b,0xc4,0x48,0x89,0x58,0x10,0x48,0x89,0x70,0x20,0x57,0x48,0x83,0xec,0x60};
using Reader=uint8_t (__fastcall*)(void*,void*);
inline Reader original{};
inline std::atomic<unsigned> changed{},refused{};
template<class Read> bool guard(uintptr_t base,Read read) {
    std::array<uint8_t,16> head{};
    return read(base+cd::mining_addresses.rva(static_cast<uint32_t>(ReaderRva)),head.data(),head.size()) && head==ReaderHead;
}
// The 2.06 reader publishes key and static-info ordinal before the flags.
inline void extend(void* output) {
    __try {
        auto* record=static_cast<uint8_t*>(output);
        if(!record || *reinterpret_cast<uint32_t*>(record)!=1708) return;
        auto* level=reinterpret_cast<int32_t*>(record+LevelOffset);
        if(*reinterpret_cast<uint32_t*>(record+4)!=783 || record[DefaultOffset]!=0 || *level!=5) {
            ++refused;
            return;
        }
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
