#pragma once
// Shared read-only native resolver. No hooks/writes before a complete plan passes.
// Copyright (c) 2026 Blablup. MIT.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cstdio>

namespace cd {
struct Fix { uint32_t offset; uint8_t width, kind; }; // 0: external address; 1: in-function RVA
struct Pattern {
    uint32_t length, anchor_offset, anchor_length;
    const uint8_t *anchor, *anchor_mask, *sha;
    const Fix* fixes; uint32_t fix_count;
};
struct Node { uint32_t id, hint; const Pattern* pattern; bool entry; uint32_t entry_offset, owner, field, end; };
struct Reference { uint32_t owner, field, end; };
struct Global { uint32_t id; const Reference* refs; uint32_t count, flags, span; };
struct Vtable { uint32_t id; const char* name; uint32_t object_offset; };
struct Alias { uint32_t id, owner, offset, expected_function; };
struct Edge { uint32_t owner, field, end, target; };
struct Plan {
    const Node* nodes; uint32_t node_count;
    const Global* globals; uint32_t global_count;
    const Vtable* vtables; uint32_t vtable_count;
    const Alias* aliases; uint32_t alias_count;
    const Edge* edges; uint32_t edge_count;
};
struct Section { uint32_t rva,size,flags; };
struct Image {
    const uint8_t* base{}; uint32_t size{},exception_rva{},exception_size{};
    uint64_t pointer_base{}; std::vector<Section> sections;
    template<class T> T get(uint32_t rva) const { T v{}; if(rva<=size && sizeof(T)<=size-rva) std::memcpy(&v,base+rva,sizeof v); return v; }
    bool range(uint32_t rva,uint32_t bytes,uint32_t flags=0) const {
        if(!bytes || rva>size || bytes>size-rva) return false;
        for(const auto& s:sections) if(rva>=s.rva && rva-s.rva<s.size && bytes<=s.size-(rva-s.rva))
            return (s.flags&flags)==flags;
        return false;
    }
    bool init(const void* memory,uint32_t available,bool file_pointers=false) {
        base=static_cast<const uint8_t*>(memory);size=available;sections.clear();
        if(!base || size<4096 || get<uint16_t>(0)!=0x5a4d) return false;
        const uint32_t pe=get<uint32_t>(0x3c);
        if(pe>size-264 || get<uint32_t>(pe)!=0x4550 || get<uint16_t>(pe+4)!=0x8664 || get<uint16_t>(pe+24)!=0x20b) return false;
        const auto count=get<uint16_t>(pe+6), optional=get<uint16_t>(pe+20);
        const uint32_t first=pe+24+optional, actual=get<uint32_t>(pe+24+56);
        if(!count || count>96 || optional<240 || actual>available || actual<4096 || first>actual || count*40u>actual-first) return false;
        size=actual;pointer_base=file_pointers?get<uint64_t>(pe+48):reinterpret_cast<uint64_t>(base);
        for(uint32_t n=0;n<count;++n) {
            const uint32_t p=first+n*40;
            Section s{get<uint32_t>(p+12),std::max(get<uint32_t>(p+8),get<uint32_t>(p+16)),get<uint32_t>(p+36)};
            if(s.rva>size || s.size>size-s.rva) return false;
            for(const auto& old:sections) if(s.rva<old.rva+old.size && old.rva<s.rva+s.size) return false;
            sections.push_back(s);
        }
        exception_rva=get<uint32_t>(pe+24+112+3*8);exception_size=get<uint32_t>(pe+24+116+3*8);
        return exception_size%12==0 && range(exception_rva,exception_size,IMAGE_SCN_MEM_READ);
    }
    uint32_t relative(const Reference& r,uint32_t owner) const {
        if(!owner || r.field>size-owner || !range(owner+r.field,4,IMAGE_SCN_MEM_EXECUTE)) return 0;
        const int64_t target=static_cast<int64_t>(owner)+r.end+get<int32_t>(owner+r.field);
        return target>0 && target<size?static_cast<uint32_t>(target):0;
    }
};
class Sha {
    BCRYPT_ALG_HANDLE algorithm{};
public:
    Sha() { BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0); }
    ~Sha() { if(algorithm) BCryptCloseAlgorithmProvider(algorithm,0); }
    bool digest(const std::vector<uint8_t>& bytes,uint8_t* out) const {
        if(!algorithm || bytes.size()>0xffffffff) return false;
        BCRYPT_HASH_HANDLE hash{};
        if(BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)<0) return false;
        const bool ok=BCryptHashData(hash,const_cast<PUCHAR>(bytes.data()),static_cast<ULONG>(bytes.size()),0)>=0 && BCryptFinishHash(hash,out,32,0)>=0;
        BCryptDestroyHash(hash);return ok;
    }
};
inline bool matches(const Image& image,uint32_t rva,const Pattern& p,const Sha& sha) {
    if(!p.length || p.length>65536 || p.anchor_offset>p.length || p.anchor_length>p.length-p.anchor_offset || !image.range(rva,p.length,IMAGE_SCN_MEM_EXECUTE)) return false;
    for(uint32_t n=0;n<p.anchor_length;++n) if(p.anchor_mask[n] && image.base[rva+p.anchor_offset+n]!=p.anchor[n]) return false;
    std::vector<uint8_t> bytes(image.base+rva,image.base+rva+p.length);
    uint32_t previous_end=0;
    for(uint32_t n=0;n<p.fix_count;++n) {
        const auto& f=p.fixes[n];
        if(!f.width || f.offset<previous_end || f.offset>p.length || f.width>p.length-f.offset) return false;
        previous_end=f.offset+f.width;
        if(f.kind==0) std::memset(bytes.data()+f.offset,0,f.width);
        else if(f.kind==1 && f.width==4) {
            uint32_t value{};std::memcpy(&value,bytes.data()+f.offset,4);
            if(value<rva || value-rva>=p.length) return false;
            value-=rva;std::memcpy(bytes.data()+f.offset,&value,4);
        } else return false;
    }
    uint8_t digest[32]{};
    return sha.digest(bytes,digest) && std::memcmp(digest,p.sha,32)==0;
}
inline uint32_t find_node(const Image& image,const Node& node,const Sha& sha) {
    uint32_t result=0,count=0;
    if(node.entry) {
        for(uint32_t pos=0;pos<image.exception_size;pos+=12) {
            const uint32_t entry=image.get<uint32_t>(image.exception_rva+pos);
            const uint32_t start=entry+node.entry_offset;
            if(start<entry)continue;
            if(matches(image,start,*node.pattern,sha)) { result=start;if(++count>1)return 0; }
        }
    } else {
        for(const auto& s:image.sections) if((s.flags&IMAGE_SCN_MEM_EXECUTE) && s.size>=node.pattern->length) {
            const auto& p=*node.pattern;uint32_t anchor=0;
            while(anchor<p.anchor_length && !p.anchor_mask[anchor])++anchor;
            if(anchor==p.anchor_length)return 0;
            for(uint32_t offset=0;offset<=s.size-p.length;++offset) {
                if(image.base[s.rva+offset+p.anchor_offset+anchor]!=p.anchor[anchor])continue;
                if(matches(image,s.rva+offset,p,sha)) { result=s.rva+offset;if(++count>1)return 0; }
            }
        }
    }
    return count==1?result:0;
}
inline uint32_t find_vtable(const Image& image,const Vtable& spec) {
    const size_t length=std::strlen(spec.name)+1;
    uint32_t td=0,names=0;
    for(const auto& s:image.sections) if((s.flags&IMAGE_SCN_MEM_READ) && !(s.flags&IMAGE_SCN_MEM_EXECUTE) && s.size>=length) {
        for(uint32_t pos=0;pos<=s.size-length;++pos) if(image.base[s.rva+pos]==static_cast<uint8_t>(spec.name[0]) && !std::memcmp(image.base+s.rva+pos,spec.name,length)) {
            if(s.rva+pos<16)return 0;
            td=s.rva+pos-16;if(++names>1)return 0;
        }
    }
    if(names!=1)return 0;
    uint32_t result=0,count=0;
    for(const auto& s:image.sections) if((s.flags&IMAGE_SCN_MEM_READ) && !(s.flags&(IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_MEM_WRITE)) && s.size>=24) {
        for(uint32_t p=s.rva;p<=s.rva+s.size-24;p+=4) {
            if(image.get<uint32_t>(p)!=1 || image.get<uint32_t>(p+4)!=spec.object_offset || image.get<uint32_t>(p+12)!=td || image.get<uint32_t>(p+20)!=p)continue;
            for(const auto& t:image.sections) if((t.flags&IMAGE_SCN_MEM_READ) && !(t.flags&(IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_MEM_WRITE)) && t.size>=16) {
                for(uint32_t q=t.rva;q<=t.rva+t.size-16;q+=8) if(image.get<uint64_t>(q)==image.pointer_base+p) {
                    const auto first=image.get<uint64_t>(q+8);
                    if(first<image.pointer_base || first-image.pointer_base>0xffffffff || !image.range(static_cast<uint32_t>(first-image.pointer_base),1,IMAGE_SCN_MEM_EXECUTE))continue;
                    result=q+8;if(++count>1)return 0;
                }
            }
        }
    }
    return count==1?result:0;
}
class Addresses {
    std::vector<std::pair<uint32_t,uint32_t>> values;
public:
    uint32_t failed_id{}; const char* failure="not resolved";
    uint32_t rva(uint32_t id) const { for(const auto& p:values)if(p.first==id)return p.second;return 0; }
    void set(uint32_t id,uint32_t value) { for(auto& p:values)if(p.first==id){p.second=value;return;}values.emplace_back(id,value); }
    bool resolve(const Image& image,const Plan& plan) {
        values.clear();failed_id=0;failure="function missing, ambiguous or changed";Sha sha;
        // Derived helpers are selected by verified callers, never by their first match.
        for(uint32_t pass=0;pass<=plan.node_count;++pass) {
            bool pending=false,progress=false;
            for(uint32_t i=0;i<plan.node_count;++i) {
                const auto& n=plan.nodes[i];if(rva(n.id))continue;
                pending=true;uint32_t found=0;
                if(n.owner) {
                    if(!rva(n.owner))continue;
                    found=image.relative({n.owner,n.field,n.end},rva(n.owner));
                    if(!matches(image,found,*n.pattern,sha))found=0;
                } else found=find_node(image,n,sha);
                if(!found){failed_id=n.id;values.clear();return false;}
                set(n.id,found);progress=true;
            }
            if(!pending)break;
            if(!progress){failure="cyclic/unresolved dependency";values.clear();return false;}
        }
        for(uint32_t i=0;i<plan.edge_count;++i) {
            const auto& e=plan.edges[i];
            if(image.relative({e.owner,e.field,e.end},rva(e.owner))!=rva(e.target)) { failed_id=e.target;failure="call relationship changed";values.clear();return false; }
        }
        for(uint32_t i=0;i<plan.global_count;++i) {
            const auto& g=plan.globals[i];uint32_t found=0;
            for(uint32_t j=0;j<g.count;++j) {
                const auto& ref=g.refs[j];const auto target=image.relative(ref,rva(ref.owner));
                if(!target || !image.range(target,g.span,g.flags) || (found && found!=target)) { failed_id=g.id;failure="global reference mismatch";values.clear();return false; }
                found=target;
            }
            if(!found){failed_id=g.id;failure="missing global reference";values.clear();return false;}set(g.id,found);
        }
        for(uint32_t i=0;i<plan.vtable_count;++i) {
            const auto& v=plan.vtables[i];const auto found=find_vtable(image,v);
            if(!found){failed_id=v.id;failure="RTTI/vtable missing or ambiguous";values.clear();return false;}set(v.id,found);
        }
        for(uint32_t i=0;i<plan.alias_count;++i) {
            const auto& a=plan.aliases[i];const uint32_t owner=rva(a.owner),target=owner+a.offset;
            if(!owner || target<owner || !image.range(target,a.expected_function?8:1) ||
                (a.expected_function && image.get<uint64_t>(target)!=image.pointer_base+rva(a.expected_function))) {
                failed_id=a.id;failure="derived site/vtable slot changed";values.clear();return false;
            }
            set(a.id,target);
        }
        failure="resolved";return true;
    }
};
inline bool loaded_image(Image& image) {
    const auto* base=reinterpret_cast<const uint8_t*>(GetModuleHandleW(nullptr));
    if(!base)return false;
    const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0 || dos->e_lfanew>4096)return false;
    const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
    return image.init(base,nt->OptionalHeader.SizeOfImage);
}
inline void write_status(const wchar_t* file,const char* state,const Addresses& addresses,uint32_t code=0) {
    Image image;
    if(!loaded_image(image) || image.size<0x8000000)return; // Ignore small test/loader hosts.
    wchar_t directory[32768]{},path[32768]{};
    const DWORD n=GetEnvironmentVariableW(L"LOCALAPPDATA",directory,32768);
    if(!n || n>=32700)return;
    if(wcscat_s(directory,L"\\CrimsonDesertMods"))return;
    CreateDirectoryW(directory,nullptr);
    if(swprintf_s(path,L"%s\\%s",directory,file)<=0)return;
    FILE* stream{};
    if(_wfopen_s(&stream,path,L"wb") || !stream)return;
    std::fprintf(stream,"State: %s\nInitialization code: %u\nResolver: %s\nSymbol: 0x%x\n",state,code,addresses.failure,addresses.failed_id);
    std::fclose(stream);
}
}
