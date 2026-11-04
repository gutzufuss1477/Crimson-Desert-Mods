// Included inside namespace alden after log/read/base/ready are defined.
using DeserializeFn=bool(__fastcall*)(Ptr,Ptr);
DeserializeFn originalDeserialize{};
using LoadCssFn=bool(__fastcall*)(Ptr,Ptr,const char*,Ptr,Ptr);
using CompileCssFn=bool(__fastcall*)(Ptr,const char*,Ptr,Ptr);
using DecodeCssFn=bool(__fastcall*)(Ptr,Ptr,Ptr);
using SetStringFn=Ptr(__fastcall*)(Ptr,const char*);
using LookupCssFn=Ptr(__fastcall*)(Ptr,Ptr);
LoadCssFn originalLoadCss{};
std::mutex cssMutex;
struct CssContext { Ptr address{}; bool ready{}; };
std::array<CssContext,64> categoryContexts{};
std::atomic<bool> categoryStylesAvailable{},categoryStyleFailure{};
bool category_controls_ready() { return categoryStylesAvailable.load() && !categoryStyleFailure.load(); }

// Native CSS loader 0x41bbf30 uses this buffer for the text compiler
// 0x41ac150 and binary decoder 0x41a7e60. Capacity precedes byte length.
struct CssBuffer {
    unsigned char* data{};
    std::uint32_t capacity{},size{};
    signed char allocationTag=-1;
    unsigned char padding[7]{};
};
static_assert(sizeof(CssBuffer)==0x18 && offsetof(CssBuffer,size)==0xc);

bool css_cached(Ptr context) {
    Ptr name=base+runtime::EmptyString;
    function<SetStringFn>(runtime::SetString)(reinterpret_cast<Ptr>(&name),runtime::CssPath);
    const auto found=function<LookupCssFn>(runtime::LookupCss)(context,reinterpret_cast<Ptr>(&name));
    function<FillFn>(runtime::ReleaseString)(reinterpret_cast<Ptr>(&name));
    return found!=0;
}
bool category_css_ready(Ptr context) {
    if(!context) return false;
    const std::lock_guard<std::mutex> lock(cssMutex);
    for(const auto& saved:categoryContexts) if(saved.address==context && saved.ready) return css_cached(context);
    return false;
}

bool ensure_category_css(Ptr parser,Ptr context,Ptr target) {
    if(!context || !target) return false;
    const std::lock_guard<std::mutex> lock(cssMutex);
    CssContext* slot=nullptr;
    for(auto& saved:categoryContexts) if(saved.address==context) { slot=&saved; break; }
    if(!slot) for(auto& saved:categoryContexts) if(!saved.address) { slot=&saved; break; }
    if(!slot) { log("CSS REFUSED: context capacity reached; native buy UI retained"); return false; }
    const bool failedBefore=slot->address==context && !slot->ready;
    *slot={context,false};
    bool loaded=false;
    const bool cached=css_cached(context);
    if(failedBefore && cached) {
        log("CSS REFUSED: earlier incomplete registration still cached; native buy UI retained"); return false;
    }
    if(cached) {
        // Native cache-hit branch binds the existing stylesheet to this
        // document and checks duplicate bindings. It performs no file read.
        loaded=originalLoadCss(parser,context,runtime::CssPath,target,0);
    } else {
        std::vector<unsigned char> text;
        try { text.assign(std::begin(runtime::CssSource),std::end(runtime::CssSource)); }
        catch(const std::bad_alloc&) { log("CSS REFUSED: allocation failed; native buy UI retained"); return false; }
        // The compiler replaces comment bytes in its input. Supply a private
        // mutable copy, and let the game own/free only its compiled output.
        CssBuffer input{ text.data(),static_cast<std::uint32_t>(text.size()),static_cast<std::uint32_t>(text.size()) };
        CssBuffer compiled{};
        if(function<CompileCssFn>(runtime::CompileCss)(reinterpret_cast<Ptr>(&input),runtime::CssPath,
                                                     reinterpret_cast<Ptr>(&compiled),0))
            loaded=function<DecodeCssFn>(runtime::DecodeCss)(context,reinterpret_cast<Ptr>(&compiled),target);
        function<FillFn>(runtime::ReleaseBuffer)(reinterpret_cast<Ptr>(&compiled));
    }
    if(loaded && css_cached(context)) {
        slot->ready=true;
        log("CSS READY: category rules registered in native stylesheet cache and bound to document");
        return true;
    }
    log("CSS REFUSED: category stylesheet registration failed; native buy UI retained");
    return false;
}

bool __fastcall load_css_hook(Ptr parser,Ptr context,const char* path,Ptr target,Ptr errors) {
    const bool result=originalLoadCss(parser,context,path,target,errors);
    if(!result || !ready.load()) return result;
    char name[sizeof("UI/GameMain/Play/NpcStore2.css")]{};
    if(read(reinterpret_cast<Ptr>(path),name,sizeof(name)) && name[sizeof(name)-1]==0 &&
       !_stricmp(name,"UI/GameMain/Play/NpcStore2.css")) {
        if(ensure_category_css(parser,context,target)) categoryStylesAvailable=true;
        else categoryStyleFailure=true;
    }
    // Failure of our optional stylesheet never changes native load success.
    return result;
}

// Native memory reader constructed in UI loader RVA 0x41e4805. The original
// payload is borrowed at +0x20; ownership fields at +0x10/+0x18 remain zero.
// Native read RVA 0x1601230 reads size/+0x28, cursor/+0x2c and copies bytes.
struct MemoryReader {
    Ptr vtable{};
    unsigned char mode{},padding[7]{};
    Ptr owned{},ownedCapacity{};
    const unsigned char* data{};
    std::uint32_t size{},cursor{};
};
static_assert(sizeof(MemoryReader)==0x30);
static_assert(offsetof(MemoryReader,data)==0x20 && offsetof(MemoryReader,cursor)==0x2c);

bool __fastcall deserialize_hook(Ptr loader,Ptr stream) {
    MemoryReader source{};
    if(!ready.load() || !read(stream,&source,sizeof(source)) ||
       source.vtable!=base+runtime::MemoryReaderVtable || source.mode || source.owned || source.ownedCapacity ||
       source.cursor || !source.data || source.size<sizeof(runtime::BuySourceTail) || source.size>1024*1024)
        return originalDeserialize(loader,stream);
    unsigned char tail[sizeof(runtime::BuySourceTail)]{};
    if(!read(reinterpret_cast<Ptr>(source.data)+source.size-sizeof(tail),tail,sizeof(tail)) ||
       std::memcmp(tail,runtime::BuySourceTail,sizeof(tail))) return originalDeserialize(loader,stream);
    // Decoding preloads the tree before styles are loaded. The native consumer
    // later processes its unchanged original CSS dependency list; our CSS hook
    // registers/binds the category rules while it loads NpcStore2.css.
    // Do not gate this earlier phase on a stylesheet cache populated later.
    std::vector<unsigned char> input,augmented;
    try {
        input.resize(source.size);
        if(!read(reinterpret_cast<Ptr>(source.data),input.data(),input.size()))
            return originalDeserialize(loader,stream);
        if(runtime::patch_ui(input,augmented)!=runtime::PatchResult::patched) {
            log("UI AUGMENT REFUSED: unknown/already modified buy UI; original stream retained");
            return originalDeserialize(loader,stream);
        }
    } catch(const std::bad_alloc&) {
        log("UI AUGMENT REFUSED: allocation failed; original stream retained");
        return originalDeserialize(loader,stream);
    }
    // The native reader creates owned/interned tree data. Its normal caller
    // frees the file buffer immediately after this synchronous operation.
    // A stack-local borrowed reader therefore keeps native allocation and
    // destruction unchanged, including recursive/nested loads on other threads.
    MemoryReader proxy=source;
    proxy.data=augmented.data();
    proxy.size=static_cast<std::uint32_t>(augmented.size());
    const bool result=originalDeserialize(loader,reinterpret_cast<Ptr>(&proxy));
    if(!result || proxy.cursor!=proxy.size) {
        log("UI AUGMENT FAILED: native decoder rejected or did not consume augmented stream");
        return false;
    }
    // The outer loader checks the ORIGINAL stream for unread bytes. Only its
    // cursor is advanced; its buffer pointer, ownership and size never change.
    reinterpret_cast<MemoryReader*>(stream)->cursor=source.size;
    log("UI AUGMENTED: native buy UI + nine categories; original grid preserved; no shared UI file replacement");
    return true;
}

bool prepare_runtime_hook() {
    for(const auto& guard:runtime::Guards) {
        unsigned char observed[16]{};
        if(!read(base+guard.rva,observed,sizeof(observed)) || std::memcmp(observed,guard.bytes,sizeof(observed))) {
            log("REFUSED: UI decoder/memory reader function guard mismatch"); return false;
        }
    }
    Ptr readSlot{};
    if(!value(base+runtime::MemoryReaderVtable+5*sizeof(Ptr),readSlot) || readSlot!=base+runtime::MemoryRead) {
        log("REFUSED: native memory reader vtable changed"); return false;
    }
    if(MH_Initialize()!=MH_OK) { log("REFUSED: MinHook initialization failed"); return false; }
    void* trampoline{};
    const auto status=MH_CreateHook(reinterpret_cast<void*>(base+runtime::Deserialize),
        reinterpret_cast<void*>(&deserialize_hook),&trampoline);
    if(status!=MH_OK) { MH_Uninitialize(); log("REFUSED: UI decoder hook preparation failed"); return false; }
    originalDeserialize=reinterpret_cast<DeserializeFn>(trampoline);
    trampoline=nullptr;
    if(MH_CreateHook(reinterpret_cast<void*>(base+runtime::LoadCss),
                    reinterpret_cast<void*>(&load_css_hook),&trampoline)!=MH_OK) {
        MH_RemoveHook(reinterpret_cast<void*>(base+runtime::Deserialize)); MH_Uninitialize();
        log("REFUSED: native stylesheet hook preparation failed"); return false;
    }
    originalLoadCss=reinterpret_cast<LoadCssFn>(trampoline);
    return true;
}
bool enable_runtime_hook() {
    if(MH_QueueEnableHook(reinterpret_cast<void*>(base+runtime::LoadCss))!=MH_OK ||
       MH_QueueEnableHook(reinterpret_cast<void*>(base+runtime::Deserialize))!=MH_OK) return false;
    return MH_ApplyQueued()==MH_OK;
}
void discard_runtime_hook() {
    MH_DisableHook(reinterpret_cast<void*>(base+runtime::LoadCss));
    MH_DisableHook(reinterpret_cast<void*>(base+runtime::Deserialize));
    MH_RemoveHook(reinterpret_cast<void*>(base+runtime::LoadCss));
    MH_RemoveHook(reinterpret_cast<void*>(base+runtime::Deserialize));
    MH_Uninitialize();
}
