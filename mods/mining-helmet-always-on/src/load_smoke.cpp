#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
int wmain(int argc,wchar_t** argv) {
    if(argc!=2) return 2;
    if(!LoadLibraryW(argv[1])) { std::printf("FAIL: LoadLibrary error %lu\n",GetLastError()); return 1; }
    // Keep the module resident while its startup worker performs the wrong-process refusal.
    Sleep(2000);
    std::puts("PASS: ASI loads in x64 test host; verify wrong_process refusal in smoke log.");
    return 0;
}
