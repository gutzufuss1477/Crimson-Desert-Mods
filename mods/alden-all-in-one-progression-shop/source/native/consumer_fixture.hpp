// Execute the supported game's actual dependency loop. Only external
// calls/data are redirected to fixtures; branch/order behavior is game code.
#include "consumer_test_generated.hpp"

static bool consumerHookEnabled{};
static unsigned consumerLoads{},consumerFinalized{},consumerBound{};
static std::vector<std::string> consumerNames;

static bool __fastcall consumer_load(Ptr parser,Ptr context,const char* path,Ptr target,Ptr errors) {
    ++consumerLoads; consumerNames.emplace_back(path);
    if(!consumerHookEnabled && !std::strcmp(path,alden::runtime::CssPath)) return false;
    return consumerHookEnabled ? alden::load_css_hook(parser,context,path,target,errors)
                               : load_css(parser,context,path,target,errors);
}
static void __fastcall consumer_finalize(Ptr target) {
    check(target==Target,"native consumer finalizes its original document");
    ++consumerFinalized;
}
static void __fastcall consumer_bind() { ++consumerBound; }
static void __fastcall consumer_noop() {}

static Ptr consumer_external(Ptr target) {
    if(target==ConsumerLoadCss) return reinterpret_cast<Ptr>(&consumer_load);
    if(target==ConsumerFinalizeStyles) return reinterpret_cast<Ptr>(&consumer_finalize);
    if(target==ConsumerBindStyles) return reinterpret_cast<Ptr>(&consumer_bind);
    return reinterpret_cast<Ptr>(&consumer_noop);
}
static void reset_css_fixture() {
    alden::categoryContexts={};
    alden::categoryStylesAvailable=false; alden::categoryStyleFailure=false;
    cssCache=false; compileResult=decodeResult=cssNativeResult=true;
    compileCalls=cssDecodeCalls=releaseCalls=styleLoadCalls=cacheBinds=0;
}
static void test_native_consumer() {
    auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    check(code!=nullptr,"native consumer executable storage");
    std::memcpy(code,NativeStyleConsumerCode,sizeof(NativeStyleConsumerCode));
    std::size_t island=(sizeof(NativeStyleConsumerCode)+15)&~std::size_t(15);
    for(const auto& fix:ConsumerCalls) {
        const auto target=consumer_external(fix.target);
        code[island]=0x48;code[island+1]=0xb8;
        std::memcpy(code+island+2,&target,8);
        code[island+10]=0xff;code[island+11]=0xe0;
        const auto relative=static_cast<std::int32_t>(island-fix.offset-4);
        std::memcpy(code+fix.offset,&relative,4);
        island+=16;
    }
    check(island<4000,"consumer trampoline space bounded");
    for(const auto& fix:ConsumerReferences) {
        std::int32_t old{};std::memcpy(&old,code+fix.offset,4);
        const auto end=static_cast<std::int64_t>(fix.target)-old-ConsumerStyleConsumer;
        check(end>0 && end<=sizeof(NativeStyleConsumerCode),"RIP-relative instruction endpoint");
        const auto relative=static_cast<std::int32_t>(4000-end);
        std::memcpy(code+fix.offset,&relative,4);
    }
    DWORD previous{};
    check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&previous)!=0,"native consumer executable protection");
    FlushInstructionCache(GetCurrentProcess(),code,4096);
    using ConsumerFn=void(__fastcall*)(Ptr,Ptr,Ptr);
    const auto consume=reinterpret_cast<ConsumerFn>(code);
    std::array<Ptr,8> view{},document{},styleSystem{};
    std::array<Ptr,16> descriptor{},root{};
    view[4]=reinterpret_cast<Ptr>(document.data()); document[5]=Target;
    const Ptr stylePointer=reinterpret_cast<Ptr>(styleSystem.data());
    std::memcpy(contextStorage.data()+0xb0,&stylePointer,sizeof(stylePointer));
    descriptor[0]=reinterpret_cast<Ptr>(root.data());
    const auto run=[&](bool extraDependency,bool useHook) {
        std::vector<const char*> names(std::begin(OriginalStyleNames),std::end(OriginalStyleNames));
        if(extraDependency) names.push_back(alden::runtime::CssPath);
        std::vector<Ptr> references;
        for(auto& name:names) references.push_back(reinterpret_cast<Ptr>(&name));
        descriptor[1]=reinterpret_cast<Ptr>(references.data()); descriptor[2]=names.size();
        consumerHookEnabled=useHook;consumerMode=true;
        consumerLoads=consumerFinalized=consumerBound=0;consumerNames.clear();
        consume(Context,reinterpret_cast<Ptr>(view.data()),reinterpret_cast<Ptr>(descriptor.data()));
        consumerMode=false;
        check(consumerLoads==names.size(),"native dependency loop visits all expected paths in order");
        for(std::size_t n=0;n<names.size();++n)
            check(consumerNames[n]==names[n],"native CSS dependency order preserved");
    };
    reset_css_fixture();
    run(true,false);
    check(!consumerBound && !consumerFinalized,"original native consumer reproduces blank-view path on missing added dependency");
    reset_css_fixture();
    run(false,true);
    check(consumerBound==1 && consumerFinalized==1 && alden::category_controls_ready(),"native consumer binds UI after late category CSS registration");
    check(compileCalls==1 && cssDecodeCalls==1,"exactly one category stylesheet compiled during native consumption");
    run(false,true);
    check(consumerBound==1 && compileCalls==1 && cacheBinds==1,"subsequent construction binds cached category styles");
    reset_css_fixture();compileResult=false;
    run(false,true);
    check(consumerBound==1 && consumerFinalized==1 && !alden::category_controls_ready(),"optional compile failure still reaches native UI binding with controls inactive");
    reset_css_fixture();decodeResult=false;
    run(false,true);
    check(consumerBound==1 && !alden::category_controls_ready(),"optional decode failure still reaches native UI binding");
    reset_css_fixture();
    run(false,true);
    check(alden::category_controls_ready(),"valid session ready for remaining hook tests");
    VirtualFree(code,0,MEM_RELEASE);
}
