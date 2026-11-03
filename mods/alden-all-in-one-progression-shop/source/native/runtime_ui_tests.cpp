#define ALDEN_TAB_TESTING
#include "native_tabs.cpp"
#include <fstream>
#include <iterator>
#include <thread>
#include <filesystem>

using Bytes=std::vector<unsigned char>;
using alden::Ptr;
using alden::MemoryReader;
using ReadFn=bool(__fastcall*)(Ptr,void*,std::uint32_t);
static ReadFn nativeRead{};
static void check(bool value,const char* label) {
    if(!value) { std::fprintf(stderr,"FAIL: %s\n",label); std::exit(1); }
}
static Bytes load(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary);
    check(file.good(),"fixture input exists");
    return Bytes(std::istreambuf_iterator<char>(file),{});
}
struct Invocation {
    const Bytes* expected{};
    MemoryReader* source{};
    unsigned calls{};
    bool augmented{},fail{},partial{},nest{};
};
static thread_local Invocation invocation;
static const Bytes* nestedInput{};
static const Bytes* nestedExpected{};
static std::array<unsigned char,0x31410> contextStorage{};
static const Ptr Context=reinterpret_cast<Ptr>(contextStorage.data());
static constexpr Ptr Target=0x88770000,Parser=0x12121200,Errors=0x34343400;
static std::array<Ptr,4> loaderStorage={0,0,0,Context};
static const Ptr Loader=reinterpret_cast<Ptr>(loaderStorage.data());
static std::atomic<bool> cssCache{};
static bool cssNativeResult=true,compileResult=true,decodeResult=true;
static unsigned compileCalls{},cssDecodeCalls{},releaseCalls{},styleLoadCalls{},cacheBinds{};
static unsigned char compiledBytes[16]{};
static bool consumerMode{};
static Ptr __fastcall set_string(Ptr out,const char* text) {
    *reinterpret_cast<const char**>(out)=text; return out;
}
static void __fastcall release_string(Ptr) {}
static Ptr __fastcall lookup_css(Ptr context,Ptr text) {
    check(context==Context,"CSS lookup uses the same UI context as the buy loader");
    check(!std::strcmp(*reinterpret_cast<const char**>(text),alden::runtime::CssPath),"CSS cache lookup key");
    return cssCache.load() ? Target : 0;
}
static bool __fastcall compile_css(Ptr source,const char* path,Ptr output,Ptr errors) {
    ++compileCalls;
    auto& in=*reinterpret_cast<alden::CssBuffer*>(source);
    check(in.size==sizeof(alden::runtime::CssSource) && in.capacity==in.size,"native compiler buffer ABI");
    check(!std::memcmp(in.data,alden::runtime::CssSource,in.size),"native compiler receives exact authored CSS");
    check(!std::strcmp(path,alden::runtime::CssPath) && !errors,"own stylesheet key and isolated optional errors");
    check(in.data!=alden::runtime::CssSource,"native comment stripping cannot mutate embedded source");
    in.data[0]=0x20;
    auto& out=*reinterpret_cast<alden::CssBuffer*>(output);
    out.data=compiledBytes;out.capacity=16;out.size=16;
    return compileResult;
}
static bool __fastcall decode_css(Ptr context,Ptr input,Ptr target) {
    ++cssDecodeCalls;
    check(context==Context && target==Target,"native CSS decode keeps context and document");
    check(reinterpret_cast<alden::CssBuffer*>(input)->data==compiledBytes,"compiler output passed to CSS decoder");
    cssCache=true; // Also exercise a partially registered entry on decode failure.
    return decodeResult;
}
static void __fastcall release_buffer(Ptr buffer) {
    ++releaseCalls;
    check(reinterpret_cast<alden::CssBuffer*>(buffer)->data==compiledBytes,"compiled output freed by native release");
    *reinterpret_cast<alden::CssBuffer*>(buffer)={};
}
static bool __fastcall load_css(Ptr parser,Ptr context,const char* path,Ptr target,Ptr errors) {
    ++styleLoadCalls;
    check((consumerMode || parser==Parser) && context==Context && target==Target,"CSS loader parameters preserved");
    if(!std::strcmp(path,alden::runtime::CssPath)) {
        ++cacheBinds;
        check(cssCache.load() && !errors,"cached category CSS binds without disk read or shared error mutation");
        return true;
    }
    check(consumerMode || errors==Errors,"original CSS keeps its original error collector");
    return cssNativeResult;
}
namespace alden {
Ptr resolve_test_function(Ptr rva) {
    switch(rva) {
    case runtime::SetString:return reinterpret_cast<Ptr>(&set_string);
    case runtime::ReleaseString:return reinterpret_cast<Ptr>(&release_string);
    case runtime::LookupCss:return reinterpret_cast<Ptr>(&lookup_css);
    case runtime::CompileCss:return reinterpret_cast<Ptr>(&compile_css);
    case runtime::DecodeCss:return reinterpret_cast<Ptr>(&decode_css);
    case runtime::ReleaseBuffer:return reinterpret_cast<Ptr>(&release_buffer);
    default: volatile bool unexpected=true;if(unexpected)std::abort();return 0;
    }
}
}
static void run_bridge(const Bytes&,const Bytes&,bool,bool=false,bool=false,bool=false);

static bool __fastcall decode(Ptr loader,Ptr reader) {
    check(loader==Loader,"original loader identity retained");
    ++invocation.calls;
    auto& stream=*reinterpret_cast<MemoryReader*>(reader);
    check((reader!=reinterpret_cast<Ptr>(invocation.source))==invocation.augmented,"proxy only for supported buy input");
    check(stream.owned==0 && stream.ownedCapacity==0,"reader never owns ASI buffer");
    if(invocation.nest) {
        const auto outer=invocation;
        run_bridge(*nestedInput,*nestedExpected,true);
        invocation=outer;
    }
    if(invocation.fail) { stream.cursor=stream.size/2; return false; }
    Bytes seen(stream.size-stream.cursor);
    std::size_t done=0;
    const std::uint32_t amounts[]={1,4,19,257};
    unsigned step=0;
    while(done<seen.size()) {
        const auto count=static_cast<std::uint32_t>(std::min<std::size_t>(amounts[step++%4],seen.size()-done));
        // Execute the actual game's 64-byte memory-reader routine in this
        // isolated test process. It has no imports or relative references.
        check(nativeRead(reader,seen.data()+done,count),"native memory read succeeds");
        done+=count;
    }
    check(seen==*invocation.expected,"native decoder receives the independently expected complete tree");
    unsigned char byte{};
    check(!nativeRead(reader,&byte,1),"native reader rejects read past end");
    if(invocation.partial) --stream.cursor;
    return true;
}

// This entry gives MinHook a real function to redirect in the isolated host.
// The game executable is neither loaded nor injected into this test process.
__declspec(noinline) static bool __fastcall hook_target(Ptr loader,Ptr reader) {
    return decode(loader,reader);
}
__declspec(noinline) static bool __fastcall css_hook_target(Ptr parser,Ptr context,const char* path,Ptr target,Ptr errors) {
    return load_css(parser,context,path,target,errors);
}
static bool useInstalledHook{};
static void run_bridge(const Bytes& input,const Bytes& expected,bool augmented,bool fail,bool partial,bool nest) {
    const auto unchanged=input;
    MemoryReader reader{};
    reader.vtable=alden::base+alden::runtime::MemoryReaderVtable;
    reader.data=input.data(); reader.size=static_cast<std::uint32_t>(input.size());
    invocation={&expected,&reader,0,augmented,fail,partial,nest};
    const auto before=reader;
    alden::DeserializeFn volatile entry=useInstalledHook ? hook_target : alden::deserialize_hook;
    const bool result=entry(Loader,reinterpret_cast<Ptr>(&reader));
    check(result==!(fail||partial),"decoder failure/partial-success propagated once");
    check(invocation.calls==1,"native decoder called exactly once, without retrying a partially built tree");
    check(reader.cursor==(result ? reader.size : 0),"original cursor mapped only after complete success");
    reader.cursor=before.cursor;
    check(std::memcmp(&reader,&before,sizeof(reader))==0,"original buffer/length/ownership/header preserved");
    check(input==unchanged,"input bytes never modified");
}

#include "consumer_fixture.hpp"

int main(int argc,char** argv) {
    check(argc==2,"fixture directory argument");
    const std::filesystem::path fixtures=argv[1];
    const auto vanilla=load(fixtures/"vanilla.bin"),better=load(fixtures/"better.bin");
    const auto vanillaExpected=load(fixtures/"vanilla_expected.bin"),betterExpected=load(fixtures/"better_expected.bin");
    Bytes out;
    check(alden::runtime::patch_ui(vanilla,out)==alden::runtime::PatchResult::patched && out==vanillaExpected,"normal 2x4 tree matches published native controls");
    check(alden::runtime::patch_ui(better,out)==alden::runtime::PatchResult::patched && out==betterExpected,"original BI 3x5 tree and all its bytes preserved");
    // Already augmented and corrupted buy trees are refused, unrelated trees
    // pass through; no fallback to a replacement of the user's active UI.
    for(const auto& input:{vanillaExpected,betterExpected})
        check(alden::runtime::patch_ui(input,out)==alden::runtime::PatchResult::unsupported && out.empty(),"already augmented tree refused");
    auto damaged=vanilla; damaged[80]^=1;
    check(alden::runtime::patch_ui(damaged,out)==alden::runtime::PatchResult::unsupported && out.empty(),"unknown buy tree refused");
    Bytes unrelated(100,0x17);
    check(alden::runtime::patch_ui(unrelated,out)==alden::runtime::PatchResult::unrelated && out.empty(),"other views unchanged");
    for(std::size_t n=0;n<vanilla.size();n+=1937)
        check(alden::runtime::patch_ui({vanilla.data(),n},out)!=alden::runtime::PatchResult::patched,"truncated UI refused");
    void* memory=VirtualAlloc(nullptr,sizeof(alden::runtime::NativeMemoryReadCode),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    check(memory!=nullptr,"native reader scratch allocation");
    std::memcpy(memory,alden::runtime::NativeMemoryReadCode,sizeof(alden::runtime::NativeMemoryReadCode));
    DWORD previous{};
    check(VirtualProtect(memory,sizeof(alden::runtime::NativeMemoryReadCode),PAGE_EXECUTE_READ,&previous)!=0,"native reader scratch executable");
    FlushInstructionCache(GetCurrentProcess(),memory,sizeof(alden::runtime::NativeMemoryReadCode));
    nativeRead=reinterpret_cast<ReadFn>(memory);
    alden::base=0x140000000; alden::ready=true; alden::originalDeserialize=decode;
    alden::originalLoadCss=load_css;
    // The real preload precedes CSS registration. Preserve the augmented tree
    // for later construction, without inventing an extra stylesheet dependency.
    run_bridge(vanilla,vanillaExpected,true);
    run_bridge(better,betterExpected,true);
    check(!alden::category_controls_ready(),"controls stay inactive during early preload");
    const char* shopCss="UI/GameMain/Play/NpcStore2.css";
    check(alden::load_css_hook(Parser,Context,shopCss,Target,Errors),"original stylesheet succeeds");
    check(compileCalls==1 && cssDecodeCalls==1 && releaseCalls==1 && alden::category_css_ready(Context),"CSS compiled, registered and released before buy augmentation");
    check(alden::load_css_hook(Parser,Context,shopCss,Target,Errors),"second document binds cached CSS");
    check(compileCalls==1 && cacheBinds==1,"cached CSS not compiled again");
    run_bridge(vanilla,vanillaExpected,true);
    run_bridge(better,betterExpected,true);
    run_bridge(damaged,damaged,false);
    run_bridge(unrelated,unrelated,false);
    run_bridge(vanillaExpected,vanillaExpected,false);
    run_bridge(vanilla,vanillaExpected,true,true);
    run_bridge(better,betterExpected,true,false,true);
    nestedInput=&better; nestedExpected=&betterExpected;
    run_bridge(vanilla,vanillaExpected,true,false,false,true);
    alden::ready=false; run_bridge(vanilla,vanilla,false); alden::ready=true;

    // Reset/failed CSS cannot leave an unusable UI dependency behind.
    cssCache=false;
    run_bridge(better,betterExpected,true);
    compileResult=false;
    check(alden::load_css_hook(Parser,Context,shopCss,Target,Errors),"CSS compile failure preserves original load success");
    check(!alden::category_css_ready(Context),"failed CSS not marked ready");
    run_bridge(better,betterExpected,true);
    check(!alden::category_controls_ready(),"failed styles keep category callbacks inactive");
    compileResult=true;decodeResult=false;
    check(alden::load_css_hook(Parser,Context,shopCss,Target,Errors),"CSS decode failure preserves original load success");
    check(cssCache.load() && !alden::category_css_ready(Context),"partially registered failed CSS cannot enable augmentation");
    run_bridge(vanilla,vanillaExpected,true);
    check(alden::load_css_hook(Parser,Context,shopCss,Target,Errors) && !alden::category_css_ready(Context),"retry cannot accept a partially registered failed stylesheet");
    cssCache=false;decodeResult=true;cssNativeResult=false;
    const auto beforeFailure=compileCalls;
    check(!alden::load_css_hook(Parser,Context,shopCss,Target,Errors) && compileCalls==beforeFailure,"native CSS failure propagated without extra work");
    cssNativeResult=true;
    check(alden::load_css_hook(Parser,Context,"UI/GameMain/Play/Unrelated.css",Target,Errors) && compileCalls==beforeFailure,"other stylesheets unchanged");
    check(alden::load_css_hook(Parser,Context,shopCss,Target,Errors),"valid CSS recovered after cache reset");
    check(!alden::category_controls_ready(),"registration failure conservatively disables controls for this session");
    test_native_consumer();

    check(MH_Initialize()==MH_OK,"isolated MinHook initialization");
    void* trampoline{};
    check(MH_CreateHook(reinterpret_cast<void*>(&hook_target),reinterpret_cast<void*>(&alden::deserialize_hook),&trampoline)==MH_OK,"isolated decoder hook created");
    alden::originalDeserialize=reinterpret_cast<alden::DeserializeFn>(trampoline);
    trampoline=nullptr;
    check(MH_CreateHook(reinterpret_cast<void*>(&css_hook_target),reinterpret_cast<void*>(&alden::load_css_hook),&trampoline)==MH_OK,"isolated CSS loader hook created");
    alden::originalLoadCss=reinterpret_cast<alden::LoadCssFn>(trampoline);
    check(MH_QueueEnableHook(reinterpret_cast<void*>(&hook_target))==MH_OK && MH_QueueEnableHook(reinterpret_cast<void*>(&css_hook_target))==MH_OK && MH_ApplyQueued()==MH_OK,"CSS and decoder hooks enabled together");
    alden::LoadCssFn volatile cssEntry=css_hook_target;
    check(cssEntry(Parser,Context,shopCss,Target,Errors),"actual CSS hook/trampoline preserves five-argument ABI");
    useInstalledHook=true;
    run_bridge(vanilla,vanillaExpected,true);
    run_bridge(better,betterExpected,true);
    run_bridge(unrelated,unrelated,false);
    std::array<std::thread,4> workers;
    for(auto& worker:workers) worker=std::thread([&] {
        for(unsigned n=0;n<12;++n) {
            run_bridge(vanilla,vanillaExpected,true);
            run_bridge(better,betterExpected,true);
        }
    });
    for(auto& worker:workers) worker.join();
    check(MH_DisableHook(reinterpret_cast<void*>(&hook_target))==MH_OK,"isolated hook disabled");
    check(MH_RemoveHook(reinterpret_cast<void*>(&hook_target))==MH_OK,"isolated hook removed");
    check(MH_DisableHook(reinterpret_cast<void*>(&css_hook_target))==MH_OK && MH_RemoveHook(reinterpret_cast<void*>(&css_hook_target))==MH_OK,"isolated CSS hook removed");
    check(MH_Uninitialize()==MH_OK,"isolated hook resources released");
    VirtualFree(memory,0,MEM_RELEASE);
    std::puts("PASS: actual native CSS dependency loop reproduces old missing-dependency abort, then binds corrected UI with late CSS registration; optional CSS failure still reaches native UI binding with category controls inactive; exact known 2x4/BI 3x5 trees, original dependencies; native memory reader; EOF/failures; nested/concurrent loads; real dual MinHook trampolines. CSS compiler/decoder and final UI renderer remain fixtures.");
    return 0;
}
