#define HEALTHBAR_TESTING
#include "native_fix.cpp"
#include <cstdlib>

void require(bool condition,const char* message) {
    if(!condition) { std::fprintf(stderr,"FAIL: %s\n",message); std::exit(1); }
}
int main() {
    using namespace healthbar;
    require(identify_ui(HtmlHash,CssHash)==UiMode::compact,"compact Caites/legacy UIFix2 accepted");
    require(identify_ui(HtmlHash,FramedCssHash)==UiMode::framed,"framed Vanilla Multitarget accepted");
    require(identify_ui(SingleHtmlHash,SingleCssHash)==UiMode::single,"Classic preserves single-target mode");
    require(identify_ui(SingleHtmlHash,FramedCssHash)==UiMode::unsupported,"reject mixed UI pair");
    require(identify_ui(HtmlHash,SingleCssHash)==UiMode::unsupported,"reject inconsistent target mode");
    require(identify_ui("",CssHash)==UiMode::unsupported,"reject absent HTML");
    require(identify_ui(HtmlHash,"changed")==UiMode::unsupported,"reject unknown CSS");
    auto page=static_cast<uint8_t*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    require(page!=nullptr,"allocation");
    const size_t offset=128;
    std::memcpy(page+offset,Context+(WordRva-ContextRva),8);
    require(!patch_word(reinterpret_cast<uintptr_t>(page+offset+1)),"reject unaligned patch");
    auto saved=page[offset]; page[offset]^=1;
    require(!patch_word(reinterpret_cast<uintptr_t>(page+offset)),"reject mismatching guard");
    page[offset]=saved;
    DWORD previous{};
    require(VirtualProtect(page,4096,PAGE_EXECUTE_READ,&previous)!=FALSE,"read-only fixture");
    require(patch_word(reinterpret_cast<uintptr_t>(page+offset)),"atomic patch");
    for(size_t i=0;i<8;++i) require(page[offset+i]==((i==3 || i==4) ? 0x90 : Context[6+i]),"only JNE bytes changed");
    require(!patch_word(reinterpret_cast<uintptr_t>(page+offset)),"reject duplicate application");
    MEMORY_BASIC_INFORMATION info{}; VirtualQuery(page,&info,sizeof(info));
    require(info.Protect==PAGE_EXECUTE_READ,"restore protection");
    VirtualFree(page,0,MEM_RELEASE);

    // Execute the native compare/branch semantics in an isolated fixture.
    // ECX=selected actor; EDX=entry actor. Matching -> show (1), different -> hide (0).
    const uint8_t code[]={0x39,0xd1,0x75,0x06,0xb8,1,0,0,0,0xc3,0x31,0xc0,0xc3};
    page=static_cast<uint8_t*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    require(page!=nullptr,"code allocation"); std::memcpy(page,code,sizeof(code));
    require(VirtualProtect(page,4096,PAGE_EXECUTE_READ,&previous)!=FALSE,"fixture executable");
    FlushInstructionCache(GetCurrentProcess(),page,sizeof(code));
    using Select=int(__fastcall*)(uint32_t,uint32_t); const auto select=reinterpret_cast<Select>(page);
    require(select(11,11)==1 && select(11,22)==0,"vanilla single-target semantics");
    require(VirtualProtect(page,4096,PAGE_READWRITE,&previous)!=FALSE,"fixture edit");
    page[2]=0x90; page[3]=0x90;
    require(VirtualProtect(page,4096,PAGE_EXECUTE_READ,&previous)!=FALSE,"fixture executable again");
    FlushInstructionCache(GetCurrentProcess(),page,sizeof(code));
    require(select(11,11)==1 && select(11,22)==1 && select(22,11)==1,"multi-target comparison after patch");
    VirtualFree(page,0,MEM_RELEASE);
    std::puts("PASS: variant selection/single-target isolation; atomic guards, alignment, adjacent bytes, duplicate refusal, protection restoration, native target-switch branch behavior");
}
