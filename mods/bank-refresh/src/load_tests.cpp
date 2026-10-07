#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int wmain(int argc,wchar_t** argv) {
    if(argc!=3) return 2;
    const auto expected=wcstoul(argv[2],nullptr,10);
    HMODULE loaded=LoadLibraryW(argv[1]);
    if(!loaded) { std::printf("FAIL: LoadLibrary %lu\n",GetLastError()); return 1; }
    const auto address=GetProcAddress(loaded,"BankRefresh_TestInitResult");
    if(expected==0) {
        if(address) { std::puts("FAIL: test export in production binary"); return 1; }
        Sleep(1000);
        std::puts("PASS: production ASI loads without test exports");
        return 0;
    }
    if(!address) { std::puts("FAIL: instrumented test export missing"); return 1; }
    DWORD (*result)();
    static_assert(sizeof(result)==sizeof(address));
    std::memcpy(&result,&address,sizeof(result));
    for(unsigned i=0;i<200;++i) {
        const auto actual=result();
        if(actual) {
            if(actual!=expected) { std::printf("FAIL: init result %lu expected %lu\n",actual,expected); return 1; }
            std::printf("PASS: init result %lu\n",actual);
            return 0;
        }
        Sleep(25);
    }
    std::puts("FAIL: initialization timeout");
    return 1;
}
