#define ALDEN_TAB_TESTING
#include "native_tabs.cpp"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <cstdlib>

namespace alden {
Ptr resolve_test_function(Ptr rva) { return rva; }
}
using Buffer=std::vector<unsigned char>;
Buffer load_file(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary);
    if(!file) std::abort();
    return Buffer(std::istreambuf_iterator<char>(file),{});
}
static Buffer expected;
static alden::MemoryReader* originalReader;
static alden::Ptr document;
static bool partialSuccess, originalSucceeds;
static unsigned decodeCalls;
bool __fastcall reject_modified(alden::Ptr destination,alden::Ptr stream) {
    auto& reader=*reinterpret_cast<alden::MemoryReader*>(stream);
    if(destination!=reinterpret_cast<alden::Ptr>(&document) || reader.cursor!=0) std::abort();
    ++decodeCalls;
    if(&reader!=originalReader) {
        if(decodeCalls!=1 || reader.size!=expected.size()) std::abort();
        document=1;
        reader.cursor=66684;
        return partialSuccess;
    }
    if(decodeCalls==2 && document!=1) std::abort();
    document=2;
    reader.cursor=originalSucceeds?reader.size:5;
    return originalSucceeds;
}
bool __fastcall read_stream(alden::Ptr,alden::Ptr stream) {
    auto& reader=*reinterpret_cast<alden::MemoryReader*>(stream);
    if(reader.size!=expected.size() || std::memcmp(reader.data,expected.data(),reader.size)) std::abort();
    reader.cursor=reader.size;
    return true;
}
int main(int argc,char** argv) {
    if(argc!=2)return 1;
    const std::filesystem::path root=argv[1];
    for(const std::string name: {"vanilla","better"}) {
        auto input=load_file(root/(name+".bin"));
        alden::categoryStyleFailure=false;
        expected=load_file(root/(name+"_expected.bin"));
        Buffer output;
        if(alden::runtime::patch_ui(input,output)!=alden::runtime::PatchResult::patched || output!=expected) return 2;
        alden::MemoryReader source{};
        source.vtable=alden::runtime::MemoryReaderVtable;
        source.size=static_cast<unsigned>(input.size());source.data=input.data();
        alden::originalDeserialize=read_stream;alden::ready=true;
        if(!alden::deserialize_hook(0,reinterpret_cast<alden::Ptr>(&source)) ||
           source.cursor!=source.size || source.data!=input.data() || !alden::category_controls_ready()) return 3;
        originalReader=&source;
        alden::originalDeserialize=reject_modified;
        for(bool partial:{false,true}) for(bool restore:{true,false}) {
            document=0;decodeCalls=0;partialSuccess=partial;originalSucceeds=restore;
            source.cursor=0;alden::categoryStyleFailure=false;
            const auto destination=reinterpret_cast<alden::Ptr>(&document);
            if(alden::deserialize_hook(destination,reinterpret_cast<alden::Ptr>(&source))!=restore ||
               decodeCalls!=2 || document!=2 || source.cursor!=(restore?source.size:5u) ||
               source.data!=input.data() || alden::category_controls_ready()) return 5;
            source.cursor=0;
            if(alden::deserialize_hook(destination,reinterpret_cast<alden::Ptr>(&source))!=restore || decodeCalls!=3) return 6;
        }
        input[input.size()/2]^=1;
        if(alden::runtime::patch_ui(input,output)!=alden::runtime::PatchResult::unsupported || !output.empty())return 4;
    }
    std::puts("PASS current shop UI and both grids; decoder failure or short read retries the untouched original; downstream failures propagate.");
}
