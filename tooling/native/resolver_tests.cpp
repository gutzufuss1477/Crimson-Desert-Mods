#include "bank_profiles.hpp"
#include <fstream>
#include <iterator>
#include <cstdio>
#include <cstdlib>
#include <string>

void require(bool ok,const char* reason) { if(!ok){std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1);} }
bool resolve_mod(const cd::Image& image, const cd::Plan& plan, cd::Addresses& addresses) {
    return &plan == &cd::bank_plan ? cd::resolve_bank(image, addresses) : addresses.resolve(image, plan);
}
int main(int argc,char** argv) {
    require(argc>=2,"provide local EXE snapshot");
    std::ifstream input(argv[1],std::ios::binary);
    require(input.good(),"open input");
    std::vector<uint8_t> raw((std::istreambuf_iterator<char>(input)),{});
    require(raw.size()>4096,"PE size");
    const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(raw.data());
    require(dos->e_magic==IMAGE_DOS_SIGNATURE && dos->e_lfanew>0 && static_cast<size_t>(dos->e_lfanew)<raw.size()-264,"PE header");
    const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(raw.data()+dos->e_lfanew);
    const uint32_t size=nt->OptionalHeader.SizeOfImage;
    auto* memory=static_cast<uint8_t*>(VirtualAlloc(nullptr,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    require(memory!=nullptr,"allocate read-only-analysis fixture");
    std::memcpy(memory,raw.data(),nt->OptionalHeader.SizeOfHeaders);
    const auto* sections=IMAGE_FIRST_SECTION(nt);
    for(uint32_t i=0;i<nt->FileHeader.NumberOfSections;++i) {
        const auto& s=sections[i];require(s.PointerToRawData<=raw.size() && s.SizeOfRawData<=raw.size()-s.PointerToRawData && s.VirtualAddress<=size && s.SizeOfRawData<=size-s.VirtualAddress,"section bounds");
        std::memcpy(memory+s.VirtualAddress,raw.data()+s.PointerToRawData,s.SizeOfRawData);
    }
    cd::Image image;require(image.init(memory,size,true),"parse mapped fixture");
    const cd::Plan* plans[]={&cd::mount_plan,&cd::healthbar_plan,&cd::bank_plan,&cd::mining_plan};
    const char* names[]={"mount","healthbar","bank","mining"};
    const bool previous=argc>2 && std::strcmp(argv[2],"previous")==0;
    for(unsigned i=0;i<4;++i) {
        cd::Addresses addresses;const auto tick=GetTickCount64();
        const bool ok=resolve_mod(image,*plans[i],addresses);
        std::printf("PLAN %s %s %llu ms id=%x reason=%s\n",names[i],ok?"PASS":"REFUSED",GetTickCount64()-tick,addresses.failed_id,addresses.failure);
        if(previous && i==2) require(!ok,"old bank layout must be rejected");
        else require(ok,"complete plan");
        if(ok) {
            const auto& p=*plans[i];
            for(uint32_t j=0;j<p.node_count;++j)std::printf("ADDRESS %s %x %x\n",names[i],p.nodes[j].id,addresses.rva(p.nodes[j].id));
            for(uint32_t j=0;j<p.global_count;++j)std::printf("ADDRESS %s %x %x\n",names[i],p.globals[j].id,addresses.rva(p.globals[j].id));
            for(uint32_t j=0;j<p.vtable_count;++j)std::printf("ADDRESS %s %x %x\n",names[i],p.vtables[j].id,addresses.rva(p.vtables[j].id));
            for(uint32_t j=0;j<p.alias_count;++j)std::printf("ADDRESS %s %x %x\n",names[i],p.aliases[j].id,addresses.rva(p.aliases[j].id));
        }
    }
    cd::Sha sha;std::array<uint8_t,32> digest{};
    require(sha.digest({'a','b','c'},digest.data()),"SHA operation");
    const uint8_t known[]={0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad};
    require(!std::memcmp(known,digest.data(),32),"known SHA256 vector");
    cd::Addresses mount;require(mount.resolve(image,cd::mount_plan),"mount negative fixture setup");
    const auto site=mount.rva(0x101248b);const uint8_t saved=memory[site];memory[site]^=1;
    require(!mount.resolve(image,cd::mount_plan) && !mount.rva(0x101248b),"changed opcode rejected and no partial addresses exposed");
    memory[site]=saved;
    // Duplicate an entire normalized Mount function at another indexed entry.
    require(mount.resolve(image,cd::mount_plan),"restore original");
    const auto start=mount.rva(0x1012270);const auto length=cd::mount_nodes[0].pattern->length;
    uint32_t duplicate=0;
    for(uint32_t i=0;i<image.exception_size;i+=12) {
        const auto a=image.get<uint32_t>(image.exception_rva+i),b=image.get<uint32_t>(image.exception_rva+i+4);
        if(a!=start && b>a && b-a>=length && image.range(a,length,IMAGE_SCN_MEM_EXECUTE)) { duplicate=a;break; }
    }
    require(duplicate!=0,"duplicate fixture space");
    std::vector<uint8_t> backup(memory+duplicate,memory+duplicate+length);
    std::memcpy(memory+duplicate,memory+start,length);
    require(cd::matches(image,duplicate,*cd::mount_nodes[0].pattern,sha),"duplicate matches normalized code");
    require(!mount.resolve(image,cd::mount_plan),"ambiguous function refused");
    std::memcpy(memory+duplicate,backup.data(),length);
    cd::Addresses health;require(health.resolve(image,cd::healthbar_plan),"vtable fixture setup");
    const auto slot=health.rva(0x59ea750);uint64_t original{};std::memcpy(&original,memory+slot,8);
    const uint64_t wrong=original+1;std::memcpy(memory+slot,&wrong,8);
    require(!health.resolve(image,cd::healthbar_plan),"wrong vtable hook target refused");
    std::memcpy(memory+slot,&original,8);
    // Our startup order must not matter: Mining must resolve after Mount's
    // breakpoint and Healthbar's branch/vtable hooks have already been installed.
    memory[site]=0xcc;
    require(health.resolve(image,cd::healthbar_plan),"coexistence setup");
    const auto branch=health.rva(0xedfdca)+9;
    const uint8_t before0=memory[branch],before1=memory[branch+1];
    memory[branch]=memory[branch+1]=0x90;std::memcpy(memory+slot,&wrong,8);
    cd::Addresses mining;
    require(mining.resolve(image,cd::mining_plan),"Mining tolerates our installed Mount/Healthbar hooks");
    if(!previous) { cd::Addresses bank;require(cd::resolve_bank(image,bank),"Bank tolerates our installed Mount/Healthbar hooks"); }
    memory[site]=saved;memory[branch]=before0;memory[branch+1]=before1;std::memcpy(memory+slot,&original,8);
    // Masked address bytes still require agreement between independent references.
    const auto& global=cd::mining_globals[0];require(global.count>=2,"multiple global witnesses");
    const auto ref=global.refs[0];const auto field=mining.rva(ref.owner)+ref.field;
    const auto displacement=image.get<int32_t>(field);const int32_t displaced=displacement+4;
    std::memcpy(memory+field,&displaced,4);
    require(!mining.resolve(image,cd::mining_plan) && mining.failed_id==global.id,"inconsistent global references refused");
    std::memcpy(memory+field,&displacement,4);
    if(!previous) {
        cd::Addresses bank;require(cd::resolve_bank(image,bank),"call edge fixture");
        const cd::Node* derived=nullptr;
        for(const auto& node:cd::bank_nodes)if(node.owner){derived=&node;break;}
        require(derived!=nullptr,"derived lookup witness");
        const auto call=bank.rva(derived->owner)+derived->field;
        const auto delta=image.get<int32_t>(call);const int32_t bad=delta+1;
        std::memcpy(memory+call,&bad,4);
        require(!cd::resolve_bank(image,bank),"changed call destination refused");
        std::memcpy(memory+call,&delta,4);
        require(cd::resolve_bank(image,bank),"restore bank lookup");
        // Gold's mov rcx,[rax+disp32] distinguishes the two layouts.
        const auto member=bank.rva(cd::bank_gold_id)+4191;
        const auto offset=image.get<uint32_t>(member);
        require(offset==0x210 || offset==0x218,"recognized gold member");
        memory[member]=0x20;  // unaudited +0x220
        require(!cd::resolve_bank(image,bank) && !bank.rva(cd::bank_gold_id) && !bank.rva(0x2df5450),
                "unknown gold layout disarms both timers");
        memory[member]=static_cast<uint8_t>(offset);
    }
    cd::Image malformed;require(!malformed.init(memory,100,true),"truncated image refused");
    // Exercise live-pointer semantics in a non-executable relocated fixture.
    // These game builds are fixed-base (no relocation directory): for them only
    // the fixture's in-module data pointers are rebased, not game code executed.
    const auto relocation=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    require(!relocation.Size || image.range(relocation.VirtualAddress,relocation.Size,IMAGE_SCN_MEM_READ),"relocation directory");
    const uint64_t delta=reinterpret_cast<uint64_t>(memory)-image.pointer_base;
    if(!relocation.Size) {
        for(const auto& s:image.sections) if((s.flags&IMAGE_SCN_MEM_READ) && !(s.flags&IMAGE_SCN_MEM_EXECUTE) && s.size>=8) {
            for(uint32_t pos=s.rva;pos<=s.rva+s.size-8;pos+=8) {
                uint64_t value=image.get<uint64_t>(pos);
                if(value>=image.pointer_base && value-image.pointer_base<image.size) {
                    value+=delta;std::memcpy(memory+pos,&value,8);
                }
            }
        }
    }
    for(uint32_t pos=0;pos<relocation.Size;) {
        const uint32_t block=relocation.VirtualAddress+pos,page=image.get<uint32_t>(block),bytes=image.get<uint32_t>(block+4);
        require(bytes>=8 && bytes<=relocation.Size-pos && bytes%2==0,"relocation block bounds");
        for(uint32_t j=8;j<bytes;j+=2) {
            const auto entry=image.get<uint16_t>(block+j);const uint32_t type=entry>>12,offset=entry&0xfff;
            if(!type)continue;
            require(type==IMAGE_REL_BASED_DIR64 && image.range(page+offset,8),"x64 relocation target");
            uint64_t value=image.get<uint64_t>(page+offset)+delta;
            std::memcpy(memory+page+offset,&value,8);
        }
        pos+=bytes;
    }
    require(image.init(memory,size),"ASLR mapped image");
    for(unsigned i=0;i<4;++i) {
        cd::Addresses addresses;const bool ok=resolve_mod(image,*plans[i],addresses);
        require(ok==!(previous && i==2),"ASLR resolution / old bank rejection");
    }
    std::puts("PASS: SHA, code mutation, missing/ambiguous patterns, call/global mismatch, vtable mismatch, coexistence, malformed image and rebased-pointer fixture; no game code executed");
    VirtualFree(memory,0,MEM_RELEASE);
}
