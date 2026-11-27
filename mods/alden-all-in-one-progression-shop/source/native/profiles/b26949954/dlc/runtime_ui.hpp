// Included inside namespace alden after log/read/base/ready are defined.
using DeserializeFn=bool(__fastcall*)(Ptr,Ptr);
DeserializeFn originalDeserialize{};
std::atomic<bool> categoryStylesAvailable{},categoryStyleFailure{};
bool category_controls_ready(){return categoryStylesAvailable.load() && !categoryStyleFailure.load();}
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
    if(!ready.load() || categoryStyleFailure.load() || !read(stream,&source,sizeof(source)) ||
       source.vtable!=base+runtime::MemoryReaderVtable || source.mode || source.owned || source.ownedCapacity ||
       source.cursor || !source.data || source.size<sizeof(runtime::BuySourceTail) || source.size>1024*1024)
        return originalDeserialize(loader,stream);
    unsigned char tail[sizeof(runtime::BuySourceTail)]{};
    if(!read(reinterpret_cast<Ptr>(source.data)+source.size-sizeof(tail),tail,sizeof(tail)) ||
       std::memcmp(tail,runtime::BuySourceTail,sizeof(tail))) return originalDeserialize(loader,stream);
    // This build carries its category styles inside the compiled buy screen.
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
        char detail[192]{};
        std::snprintf(detail,sizeof(detail),"UI AUGMENT FAILED: native decoder result=%u; consumed=%u/%u; input=%u", static_cast<unsigned>(result),proxy.cursor,proxy.size,source.size);
        log(detail);
        categoryStyleFailure=true;
        categoryStylesAvailable=false;
        // Deserialize constructs a new document and releases the previous
        // document owned by loader. The original reader is still at byte zero.
        const bool restored=originalDeserialize(loader,stream);
        log(restored ? "Recovered original buy UI; categories disabled for this session"
                     : "Original buy UI also failed to decode");
        return restored;
    }
    // The outer loader checks the ORIGINAL stream for unread bytes. Only its
    // cursor is advanced; its buffer pointer, ownership and size never change.
    categoryStylesAvailable=true;
    reinterpret_cast<MemoryReader*>(stream)->cursor=source.size;
    log("UI AUGMENTED: native buy UI + category controls; original grid preserved; no shared UI file replacement");
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
    return true;
}
bool enable_runtime_hook() {
    if(
       MH_QueueEnableHook(reinterpret_cast<void*>(base+runtime::Deserialize))!=MH_OK) return false;
    return MH_ApplyQueued()==MH_OK;
}
void discard_runtime_hook() {

    MH_DisableHook(reinterpret_cast<void*>(base+runtime::Deserialize));

    MH_RemoveHook(reinterpret_cast<void*>(base+runtime::Deserialize));
    MH_Uninitialize();
}
