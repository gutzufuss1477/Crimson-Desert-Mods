// Execute the current game's complete UIPropertyConditionData reader in an
// isolated test process. External string/allocation/diagnostic helpers are
// replaced; the actual reader's branches and byte reads remain game code.
#include "property_condition_generated.hpp"

static unsigned propertyErrors{};
static void __fastcall property_context(Ptr out,Ptr reader,Ptr) {
    *reinterpret_cast<Ptr*>(out)=reader;
}
static bool __fastcall property_string(Ptr reader,Ptr out) {
    std::uint32_t size{};
    if(!nativeRead(reader,&size,4) || size>4096) return false;
    std::array<unsigned char,4096> text{};
    if(!nativeRead(reader,text.data(),size)) return false;
    *reinterpret_cast<Ptr*>(out)=0;
    return true;
}
static void __fastcall property_error() { ++propertyErrors; }
static void __fastcall property_unexpected() { check(false,"property fixture must use preallocated object"); }
static Ptr property_external(Ptr target) {
    switch(target) {
    case 0x16436f0:return reinterpret_cast<Ptr>(&property_context);
    case 0x1643f30:return reinterpret_cast<Ptr>(&property_string);
    case 0x3aacb0:return reinterpret_cast<Ptr>(&property_error);
    case 0x1595880:case 0x5485a0:return reinterpret_cast<Ptr>(&property_unexpected);
    default:check(false,"unexpected property-reader dependency");return 0;
    }
}
static void test_native_property_condition() {
    auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    check(code!=nullptr,"property reader scratch allocation");
    std::memcpy(code,NativePropertyConditionCode,sizeof(NativePropertyConditionCode));
    std::size_t island=(sizeof(NativePropertyConditionCode)+15)&~std::size_t(15);
    for(const auto& fix:PropertyCalls) {
        const auto target=property_external(fix.target);
        code[island]=0x48;code[island+1]=0xb8;
        std::memcpy(code+island+2,&target,8);
        code[island+10]=0xff;code[island+11]=0xe0;
        const auto relative=static_cast<std::int32_t>(island-fix.offset-4);
        std::memcpy(code+fix.offset,&relative,4);island+=16;
    }
    check(island<4000,"property reader trampoline bound");
    for(const auto& fix:PropertyReferences) {
        std::int32_t old{};std::memcpy(&old,code+fix.offset,4);
        const auto end=static_cast<std::int64_t>(fix.target)-old-PropertyReaderRva;
        check(end>0 && end<=sizeof(NativePropertyConditionCode),"property RIP reference endpoint");
        const auto relative=static_cast<std::int32_t>(4000-end);
        std::memcpy(code+fix.offset,&relative,4);
    }
    DWORD previous{};
    check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&previous)!=0,"property reader executable protection");
    FlushInstructionCache(GetCurrentProcess(),code,4096);
    using PropertyFn=bool(__fastcall*)(Ptr,Ptr);
    const auto decodeProperty=reinterpret_cast<PropertyFn>(code);
    std::array<Ptr,6> vtable{};vtable[5]=reinterpret_cast<Ptr>(nativeRead);
    const auto run=[&](const Bytes& input,bool expected,unsigned consumed,int cssFlag) {
        std::array<unsigned char,16> property{};property[8]=2;property[9]=0xab;
        Ptr output=reinterpret_cast<Ptr>(property.data());
        MemoryReader stream{};stream.vtable=reinterpret_cast<Ptr>(vtable.data());
        stream.data=input.data();stream.size=static_cast<std::uint32_t>(input.size());
        check(decodeProperty(reinterpret_cast<Ptr>(&stream),reinterpret_cast<Ptr>(&output))==expected,"actual property reader result");
        check(stream.cursor==consumed,"actual property reader consumed exact bytes");
        if(cssFlag>=0) check(property[9]==cssFlag,"actual reader stores hasCssAttribute separately");
    };
    propertyErrors=0;
    run({0},true,1,-1);
    run({1,0,0,0,0,0},false,6,-1); // The old six-byte payload is incomplete.
    check(propertyErrors==1,"native reader reproduces the missing-field error");
    run({1,0,0,0,0,0,0},true,7,0); // New field, native default false.
    run({1,0,0,0,0,0,1},true,7,1); // Missing field otherwise swallows the next optional flag.
    for(unsigned n=1;n<7;++n) {
        const Bytes full={1,0,0,0,0,0,0};
        const Bytes truncated(full.begin(),full.begin()+n);
        std::array<unsigned char,16> property{};Ptr output=reinterpret_cast<Ptr>(property.data());
        MemoryReader stream{};stream.vtable=reinterpret_cast<Ptr>(vtable.data());
        stream.data=truncated.data();stream.size=n;
        check(!decodeProperty(reinterpret_cast<Ptr>(&stream),reinterpret_cast<Ptr>(&output)),"truncated property rejected by actual reader");
    }
    VirtualFree(code,0,MEM_RELEASE);
    std::puts("PASS: actual b26636972 UIPropertyConditionData reader rejects old payload, accepts added hasCssAttribute byte, and detects truncation.");
}
