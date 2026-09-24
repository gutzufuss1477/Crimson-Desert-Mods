// Checked counterpart of 0x14061d8c0's startup-sequence path. Only called
// on the registered effect thread, with fresh source and owner ActorLeases.
struct NativeLockRef { uintptr_t object{}; uint8_t ownsReference{}; uint8_t padding[7]{}; };
using LockOwnerFn=NativeLockRef* (__fastcall*)(NativeLockRef*,uintptr_t,uint8_t);
using UnlockOwnerFn=void (__fastcall*)(uintptr_t,uintptr_t);
using EffectEntryFn=uintptr_t (__fastcall*)(uintptr_t);
using TransformFn=void (__fastcall*)(uintptr_t,void*);
using CreateSequenceFn=uint32_t (__fastcall*)(uintptr_t,const char*,const void*,uintptr_t,uint8_t,uint8_t);
using FindSequenceFn=uintptr_t (__fastcall*)(uintptr_t,uint32_t);
using AttachSequenceFn=void (__fastcall*)(uintptr_t,const void*);
using StopSequenceFn=uintptr_t (__fastcall*)(uintptr_t,uint32_t,uint8_t);
LockOwnerFn lockOwner{};
EffectEntryFn effectEntry{};
CreateSequenceFn createSequence{};
FindSequenceFn findSequence{};
AttachSequenceFn attachSequence{};
StopSequenceFn stopSequence{};
struct SequenceClosure { uintptr_t component{},callback{},extra{}; uint16_t flags{}; uint8_t padding[6]{}; };
static_assert(sizeof(SequenceClosure)==32 && sizeof(NativeLockRef)==16);
template<class T> void storeNative(uintptr_t address,T value) {
    std::memcpy(reinterpret_cast<void*>(address),&value,sizeof(value));
}
struct BorrowedOwnerLock {
    NativeLockRef ref{};
    UnlockOwnerFn unlock{};
    bool acquire(uintptr_t owner) {
        uintptr_t object{},vt{},fn{};
        if(!read(owner+8,object) || !object || !read(object,vt) || !vt ||
           !read(vt+0x20,fn) || !fn) return false;
        unlock=reinterpret_cast<UnlockOwnerFn>(fn);
        // The caller's ActorLease owns lifetime; borrow only the native lock.
        lockOwner(&ref,object,0);
        return true;
    }
    ~BorrowedOwnerLock() { if(ref.object) unlock(ref.object,0); }
    BorrowedOwnerLock()=default;
    BorrowedOwnerLock(const BorrowedOwnerLock&)=delete;
    BorrowedOwnerLock& operator=(const BorrowedOwnerLock&)=delete;
};
bool effectManager(uintptr_t& manager) {
#ifdef MH125_TEST_HOST
    manager=2; return true;
#else
    uintptr_t root{},effects{};
    return read(base+0x6d69190,root) && root && read(root+0xd8,effects) && effects &&
        read(effects+0x40,manager) && manager;
#endif
}
struct PreparedEffect {
    uintptr_t manager{},entry{};
    uint32_t id{0xffffffff};
    uint8_t componentRefresh{},entryRefresh{};
    bool created{};
};
bool effectRejected(const char* reason,uint32_t id=0xffffffff) {
    log("mode_effect_prepare_failed","\"reason\":\"%s\",\"sequence\":%u",reason,id);
    return false;
}
bool prepareModeEffect(uintptr_t component,uintptr_t owner,uint32_t ownerId,
                       uint16_t index,uint8_t slot,uintptr_t info,PreparedEffect& prepared) {
    uintptr_t nameRef{},name{},components{},model{},vt{},transformFn{}; char first{};
    if(slot>1 || !effectManager(prepared.manager) || !read(info+0x18,nameRef) || !nameRef ||
       !read(nameRef,name) || !name || !read(name,first) || !first ||
       !read(owner+0x68,components) || !components || !read(components+0x40,model) || !model ||
       !read(model,vt) || !vt || !read(vt+0x178,transformFn) || !transformFn)
        return effectRejected("missing_sequence_or_model");
    {
        BorrowedOwnerLock lock;
        if(!lock.acquire(owner)) return effectRejected("owner_lock_unavailable");
        uint32_t existing{},count{}; uintptr_t entries{};
        if(!read(component+0x6c+slot*4,existing) || !read(component+0xc8,entries) ||
           !read(component+0xd0,count) || count>4096 || (count && !entries))
            return effectRejected("invalid_entry_pool");
        if(existing!=0xffffffff) {
            for(uint32_t i=0;i<count;++i) {
                uintptr_t entry{}; uint32_t id{},entryOwner{}; uint16_t entryIndex{};
                if(!read(entries+i*8,entry) || !entry || !read(entry+0x10,id) || id!=existing) continue;
                if(!read(entry+8,entryOwner) || entryOwner!=ownerId || !read(entry+0xc,entryIndex) ||
                   entryIndex!=index || !findSequence(prepared.manager,id))
                    return effectRejected("existing_sequence_mismatch",existing);
                prepared.entry=entry; prepared.id=id;
                if(!read(component+0xf0+slot,prepared.componentRefresh) || !read(entry+0x18,prepared.entryRefresh))
                    return effectRejected("unreadable_refresh_flags",existing);
                log("mode_effect_prepared","\"sequence\":%u,\"reused\":true",id);
                return true;
            }
            return effectRejected("existing_sequence_not_in_pool",existing);
        }
        prepared.entry=effectEntry(component);
        if(!prepared.entry) return effectRejected("entry_allocation_failed");
        if(!read(prepared.entry+0x18,prepared.entryRefresh)) return effectRejected("unreadable_entry");
        storeNative(prepared.entry+8,ownerId);
        storeNative(prepared.entry+0xc,index);
    }
    alignas(16) uint8_t transform[48]{};
    reinterpret_cast<TransformFn>(transformFn)(model,transform);
    prepared.id=createSequence(prepared.manager,reinterpret_cast<const char*>(name),transform,prepared.entry,0,0);
    storeNative(prepared.entry+0x10,prepared.id);
    if(prepared.id==0xffffffff) return effectRejected("native_create_returned_minus_one");
    prepared.created=true;
    const uintptr_t sequence=findSequence(prepared.manager,prepared.id);
    if(!sequence) {
        // Native stop tolerates an absent lookup; keep the pool entry reserved
        // until native lifecycle code releases it. Never fabricate a free entry.
        stopSequence(prepared.manager,prepared.id,0);
        return effectRejected("native_lookup_returned_null",prepared.id);
    }
    const SequenceClosure closure{component,base+0x6250b0};
    attachSequence(sequence+0x150,&closure);
    storeNative(component+0x6c+slot*4,prepared.id);
    storeNative(component+0xf0+slot,uint8_t{0});
    log("mode_effect_prepared","\"sequence\":%u,\"reused\":false",prepared.id);
    return true;
}
void restorePreparedRefresh(uintptr_t component,uint8_t slot,const PreparedEffect& prepared) {
    uint32_t current{},entryId{};
    if(read(component+0x6c+slot*4,current) && current==prepared.id &&
       read(prepared.entry+0x10,entryId) && entryId==prepared.id) {
        // modeOn's reuse branch marks a refresh; a fresh native start does not.
        // Preserve the fresh-start semantics of the checked preparation path.
        storeNative(component+0xf0+slot,prepared.componentRefresh);
        storeNative(prepared.entry+0x18,prepared.entryRefresh);
    }
}
void abandonPreparedEffect(uintptr_t component,uint8_t slot,const PreparedEffect& prepared) {
    if(!prepared.created) { restorePreparedRefresh(component,slot,prepared); return; }
    // Stop outside the owner lock: the native completion callback takes it.
    stopSequence(prepared.manager,prepared.id,0);
    uint32_t current{};
    if(read(component+0x6c+slot*4,current) && (current==prepared.id || current==0xffffffff)) {
        storeNative(component+0x6c+slot*4,uint32_t{0xffffffff});
        storeNative(component+0xf0+slot,uint8_t{0});
    }
    log("mode_effect_abandoned","\"sequence\":%u",prepared.id);
}
