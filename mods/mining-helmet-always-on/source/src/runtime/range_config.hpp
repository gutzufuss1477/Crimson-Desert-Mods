#pragma once
#include <windows.h>
#include <string>
#include <string_view>
#include <charconv>
#include <cmath>
#include <utility>

namespace ini37 {
inline constexpr float DefaultRange=60,MinimumRange=10,MaximumRange=120;
inline constexpr float DefaultGlowStrength=0.5f,MinimumGlowStrength=0.5f,MaximumGlowStrength=1.0f;
inline constexpr wchar_t Filename[]=L"Mining_Helmet_Always_On.ini";
enum class Result { Unchanged, Applied, Missing, ReadError, TooLarge, EncodingError, MissingRange, InvalidRange, OutOfRange, DuplicateRange, InvalidGlowStrength, OutOfRangeGlowStrength, DuplicateGlowStrength, SyntaxError };
inline const char* describe(Result result) {
    switch(result) {
    case Result::Unchanged:return "unchanged";
    case Result::Applied:return "applied";
    case Result::Missing:return "missing; keeping last valid range (startup default 60)";
    case Result::ReadError:return "read failed; keeping last valid range";
    case Result::TooLarge:return "INI exceeds 64 KiB; keeping last valid range";
    case Result::EncodingError:return "invalid INI encoding; keeping last valid range";
    case Result::MissingRange:return "missing [MiningHelmetAlwaysOn] Range; keeping last valid range";
    case Result::InvalidRange:return "Range must be a finite decimal number; keeping last valid range";
    case Result::OutOfRange:return "Range must be between 10 and 120; keeping last valid range";
    case Result::DuplicateRange:return "duplicate Range setting; keeping last valid range";
    case Result::InvalidGlowStrength:return "GlowStrength must be a finite decimal number";
    case Result::OutOfRangeGlowStrength:return "GlowStrength must be between 0.5 and 1.0";
    case Result::DuplicateGlowStrength:return "duplicate GlowStrength setting";
    default:return "invalid INI syntax; keeping last valid range";
    }
}
inline bool space(char c) { return c==' ' || c=='\t' || c=='\r' || c=='\n'; }
inline std::string_view trim(std::string_view text) {
    while(!text.empty() && space(text.front())) text.remove_prefix(1);
    while(!text.empty() && space(text.back())) text.remove_suffix(1);
    return text;
}
inline bool equal(std::string_view a,std::string_view b) {
    if(a.size()!=b.size()) return false;
    const auto lower=[](char c) { return c>='A' && c<='Z'?static_cast<char>(c+('a'-'A')):c; };
    for(size_t i=0;i<a.size();++i) if(lower(a[i])!=lower(b[i])) return false;
    return true;
}
inline bool decode(std::string_view raw,std::string& text) {
    if(raw.starts_with("\xef\xbb\xbf")) raw.remove_prefix(3);
    else if(raw.starts_with("\xff\xfe") || raw.starts_with("\xfe\xff")) {
        const bool little=static_cast<unsigned char>(raw[0])==0xff;
        raw.remove_prefix(2);
        if(raw.size()%2) return false;
        std::wstring wide;
        for(size_t i=0;i<raw.size();i+=2) {
            const auto a=static_cast<unsigned char>(raw[i]),b=static_cast<unsigned char>(raw[i+1]);
            wide.push_back(static_cast<wchar_t>(little?(a|(b<<8)):(b|(a<<8))));
        }
        if(wide.empty()) { text.clear(); return true; }
        const auto n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,wide.data(),static_cast<int>(wide.size()),nullptr,0,nullptr,nullptr);
        if(!n) return false;
        text.resize(n);
        if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,wide.data(),static_cast<int>(wide.size()),text.data(),n,nullptr,nullptr)!=n) return false;
        return text.find('\0')==std::string::npos;
    }
    text.assign(raw);
    return text.find('\0')==std::string::npos;
}
inline Result parse(std::string_view raw,float& output,float* outputStrength=nullptr) {
    if(raw.size()>65536) return Result::TooLarge;
    std::string decoded;
    if(!decode(raw,decoded)) return Result::EncodingError;
    std::string_view remaining=decoded;
    bool active=false,found=false,foundStrength=false;
    float range=DefaultRange,strength=DefaultGlowStrength;
    while(!remaining.empty()) {
        const auto end=remaining.find('\n');
        auto line=remaining.substr(0,end);
        remaining=end==std::string_view::npos?std::string_view{}:remaining.substr(end+1);
        line=trim(line.substr(0,line.find_first_of(";#")));
        if(line.empty()) continue;
        if(line.front()=='[') {
            if(line.back()!=']') return Result::SyntaxError;
            active=equal(trim(line.substr(1,line.size()-2)),"MiningHelmetAlwaysOn");
            continue;
        }
        if(!active) continue;
        const auto split=line.find('=');
        if(split==std::string_view::npos) return Result::SyntaxError;
        const auto key=trim(line.substr(0,split));
        if(equal(key,"GlowStrength")) {
            if(foundStrength) return Result::DuplicateGlowStrength;
            foundStrength=true;
            const auto v=trim(line.substr(split+1));
            if(v.empty()) return Result::InvalidGlowStrength;
            const auto parsed=std::from_chars(v.data(),v.data()+v.size(),strength);
            if(parsed.ec!=std::errc{} || parsed.ptr!=v.data()+v.size() || !std::isfinite(strength)) return Result::InvalidGlowStrength;
            if(strength<MinimumGlowStrength || strength>MaximumGlowStrength) return Result::OutOfRangeGlowStrength;
            continue;
        }
        if(!equal(key,"Range")) continue;
        if(found) return Result::DuplicateRange;
        found=true;
        const auto value=trim(line.substr(split+1));
        if(value.empty()) return Result::InvalidRange;
        const auto parsed=std::from_chars(value.data(),value.data()+value.size(),range);
        if(parsed.ec!=std::errc{} || parsed.ptr!=value.data()+value.size() || !std::isfinite(range)) return Result::InvalidRange;
        if(range<MinimumRange || range>MaximumRange) return Result::OutOfRange;
    }
    if(!found) return Result::MissingRange;
    output=range;
    if(outputStrength) *outputStrength=strength;
    return Result::Applied;
}
inline Result read(const wchar_t* path,std::string& text) {
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE) {
        const auto error=GetLastError();
        return error==ERROR_FILE_NOT_FOUND || error==ERROR_PATH_NOT_FOUND?Result::Missing:Result::ReadError;
    }
    BY_HANDLE_FILE_INFORMATION before{},after{};
    if(!GetFileInformationByHandle(file,&before)) { CloseHandle(file); return Result::ReadError; }
    if(before.nFileSizeHigh || before.nFileSizeLow>65536) { CloseHandle(file); return Result::TooLarge; }
    text.resize(before.nFileSizeLow);
    DWORD actual{};
    const bool ok=ReadFile(file,text.data(),before.nFileSizeLow,&actual,nullptr) && actual==before.nFileSizeLow &&
        GetFileInformationByHandle(file,&after) && before.nFileSizeHigh==after.nFileSizeHigh && before.nFileSizeLow==after.nFileSizeLow &&
        CompareFileTime(&before.ftLastWriteTime,&after.ftLastWriteTime)==0;
    CloseHandle(file);
    return ok?Result::Applied:Result::ReadError;
}
struct Watcher {
    std::wstring path;
    std::string previous;
    bool sampled{};
    float range=DefaultRange,strength=DefaultGlowStrength;
    Result status=Result::Unchanged;
    unsigned reloads{};
    bool fromModule(HMODULE module) {
        wchar_t filename[32768]{};
        const auto n=GetModuleFileNameW(module,filename,32768);
        if(!n || n>=32768) return false;
        path.assign(filename,n);
        const auto slash=path.find_last_of(L"\\/");
        if(slash==std::wstring::npos) return false;
        path.resize(slash+1); path+=Filename;
        return true;
    }
    // Run on the worker thread only. A failed/partial save never resets range.
    // Comparing file contents also catches same-size and same-timestamp edits.
    Result poll() {
        std::string text;
        const auto loaded=read(path.c_str(),text);
        if(loaded!=Result::Applied) { status=loaded; sampled=false; return status; }
        if(sampled && text==previous) return Result::Unchanged;
        previous=std::move(text); sampled=true;
        float candidate{},candidateStrength{};
        status=parse(previous,candidate,&candidateStrength);
        if(status==Result::Applied) { range=candidate; strength=candidateStrength; ++reloads; }
        return status;
    }
};
}
