#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <cmath>

// The CPU material path verified by the Test33 live comparison. This header
// cannot call gameplay functions or write player/global SpecialMode lists.
namespace mh34 {
using Read = bool(*)(uintptr_t,void*,size_t);
using Write = bool(*)(uintptr_t,const void*,size_t);
template<class T> bool get(Read r,uintptr_t p,T& value) { return p && r(p,&value,sizeof(value)); }
struct Spec { const char* name; uint32_t bytes; uintptr_t vtable; };
inline constexpr Spec Specs[]={
    {"_detectModePosition",12,0x5d0f8b0}, {"_detectModeLook",12,0x5d0f8b0},
    {"_detectModeUp",12,0x5d0f8b0}, {"_detectModeAngle",4,0x5d0feb0},
    {"_detectModeRadius",4,0x5d0feb0}, {"_useHatMode",4,0x5d119b0},
    {"_highLightForVision",4,0x5d0feb0}, {"_specialModeType",4,0x5d119b0}
};
struct Binding { uintptr_t entry{},object{}; uint32_t id{},offset{}; };
struct View {
    uintptr_t group{},buffer{},base{},buckets{},nodes{};
    uint32_t size{};
    std::array<Binding,8> fields{};
};
struct Map { uint32_t buckets{},count{},capacity{},unused{}; uintptr_t bucketData{},nodes{}; };
static_assert(sizeof(Map)==0x20);

template<class Callback> bool names(Read read,uintptr_t nameEntries,uintptr_t table,Callback visit) {
    Map map{};
    if(!get(read,table,map) || !map.buckets || map.buckets>4096 ||
       !map.count || map.count>65536 || !map.bucketData || !map.nodes) return false;
    for(uint32_t b=0;b<map.buckets;++b) {
        std::array<uint32_t,64> bucket{};
        if(!read(map.bucketData+static_cast<uintptr_t>(b)*0x100,bucket.data(),0x100) || bucket[0]>31) return false;
        for(uint32_t j=0;j<bucket[0];++j) {
            const uint32_t index=bucket[3+j*2]; uintptr_t node{},text{}; uint32_t id{}; uint16_t len{};
            if(index>=65536 || !get(read,map.nodes+static_cast<uintptr_t>(index)*8,node) || !node ||
               !get(read,node+4,id) || id>4000000) return false;
            const auto record=nameEntries+static_cast<uintptr_t>(id)*16;
            if(!get(read,record,text) || !get(read,record+12,len) || !len || len>256) return false;
            char name[257]{};
            if(!read(text,name,len)) return false;
            if(visit(name,node+8,id)) return true;
        }
    }
    return true;
}
inline bool valid(Read read,const View& v) {
    uintptr_t buffer{},buckets{},nodes{}; uint32_t size{};
    if(!v.group || !get(read,v.group+0x78,buffer) || buffer!=v.buffer ||
       !get(read,v.group+0x80,size) || size!=v.size ||
       !get(read,v.group+0x68,buckets) || buckets!=v.buckets ||
       !get(read,v.group+0x70,nodes) || nodes!=v.nodes) return false;
    for(size_t i=0;i<v.fields.size();++i) {
        const auto& f=v.fields[i]; uintptr_t object{},vt{}; uint32_t id{}; uint16_t word{};
        if(!f.entry || !get(read,f.entry,object) || object!=f.object ||
           !get(read,f.entry-4,id) || id!=f.id || !get(read,f.entry+8,word) || word*4u!=f.offset ||
           f.offset+Specs[i].bytes>size || !get(read,object,vt) || vt!=v.base+Specs[i].vtable) return false;
    }
    return true;
}
inline bool resolve(Read read,uintptr_t root,uintptr_t base,View& output) {
    uintptr_t manager{},a{},b{},groups{},pool{},nameEntries{};
    if(!get(read,root+0x20,manager) || !manager || !get(read,manager+0x1228,a) || !a ||
       !get(read,a+0x160,b) || !b || !get(read,b+0x538,groups) || !groups ||
       !get(read,base+0x6c8cce8,pool) || !pool || !get(read,pool+0x58,nameEntries) || !nameEntries) return false;
    View v{}; v.base=base;
    if(!names(read,nameEntries,groups,[&](const char* name,uintptr_t entry,uint32_t) {
        return !std::strcmp(name,"GlobalMaterialGlobalParameter_Common") && get(read,entry,v.group);
    }) || !v.group || !get(read,v.group+0x78,v.buffer) || !v.buffer ||
       !get(read,v.group+0x80,v.size) || v.size<184 || v.size>65536 ||
       !get(read,v.group+0x68,v.buckets) || !get(read,v.group+0x70,v.nodes)) return false;
    unsigned found{};
    if(!names(read,nameEntries,v.group+0x58,[&](const char* name,uintptr_t entry,uint32_t id) {
        for(size_t i=0;i<v.fields.size();++i) if(!std::strcmp(name,Specs[i].name)) {
            auto& f=v.fields[i]; uint16_t word{};
            if(f.entry) { found=100; return true; }
            if(!get(read,entry,f.object) || !get(read,entry+8,word)) return true;
            f.entry=entry; f.id=id; f.offset=word*4u; ++found;
        }
        return found==8;
    }) || found!=8 || !valid(read,v)) return false;
    output=v; return true;
}

using Raw = std::array<uint8_t,12>;
using Values = std::array<Raw,8>;
template<class T> inline void put(Raw& r,const T& value) { static_assert(sizeof(T)<=12); std::memcpy(r.data(),&value,sizeof(value)); }
inline Values values(const float* position,const float* look,const float* up,float angle=30,float radius=30) {
    Values out{};
    std::memcpy(out[0].data(),position,12); std::memcpy(out[1].data(),look,12); std::memcpy(out[2].data(),up,12);
    put(out[3],angle); put(out[4],radius); put(out[5],uint32_t{1}); put(out[6],1.0f); put(out[7],uint32_t{0});
    return out;
}
inline bool finite(const Values& v) {
    for(size_t i=0;i<7;++i) if(i!=5) for(size_t j=0;j<Specs[i].bytes;j+=4) {
        float f{}; std::memcpy(&f,v[i].data()+j,4);
        if(!std::isfinite(f) || std::fabs(f)>1e8f) return false;
    }
    return true;
}
struct Ownership {
    View view{};
    Values originalProperty{},originalBuffer{},last{};
    std::array<bool,8> propertyWritten{},bufferWritten{};
    bool owned{};
    void abandon() { *this={}; }
    bool release(Read read,Write write) {
        if(!owned) return true;
        if(!valid(read,view)) { abandon(); return false; }
        bool good=true,changed=false; Raw current{};
        for(size_t i=0;i<8;++i) {
            const auto n=Specs[i].bytes;
            if(propertyWritten[i] && read(view.fields[i].object+0x30,current.data(),n) &&
               !std::memcmp(current.data(),last[i].data(),n)) {
                good=write(view.fields[i].object+0x30,originalProperty[i].data(),n) && good; changed=true;
            }
            if(bufferWritten[i] && read(view.buffer+view.fields[i].offset,current.data(),n) &&
               !std::memcmp(current.data(),last[i].data(),n)) {
                good=write(view.buffer+view.fields[i].offset,originalBuffer[i].data(),n) && good; changed=true;
            }
        }
        const uint8_t dirty=1;
        if(changed) good=write(view.group+0x90,&dirty,1) && good;
        abandon(); return good;
    }
    bool apply(Read read,Write write,const View& current,const Values& desired) {
        if(!finite(desired) || !valid(read,current)) return false;
        if(owned && !valid(read,view)) abandon();
        if(owned && (view.group!=current.group || view.buffer!=current.buffer)) release(read,write);
        if(!owned) {
            uint32_t type{}; float highlight{};
            if(!get(read,current.fields[7].object+0x30,type) || type!=255 ||
               !get(read,current.fields[6].object+0x30,highlight) || !std::isfinite(highlight) || highlight>0.001f) return false;
            view=current;
            for(size_t i=0;i<8;++i) if(!read(view.fields[i].object+0x30,originalProperty[i].data(),Specs[i].bytes) ||
                !read(view.buffer+view.fields[i].offset,originalBuffer[i].data(),Specs[i].bytes)) return false;
            owned=true;
        }
        for(size_t i=0;i<8;++i) {
            const auto n=Specs[i].bytes;
            last[i]=desired[i];
            propertyWritten[i]=true;
            if(!write(view.fields[i].object+0x30,desired[i].data(),n)) { release(read,write); return false; }
            bufferWritten[i]=true;
            if(!write(view.buffer+view.fields[i].offset,desired[i].data(),n)) { release(read,write); return false; }
        }
        const uint8_t dirty=1;
        if(!write(view.group+0x90,&dirty,1)) { release(read,write); return false; }
        return true;
    }
};
}
