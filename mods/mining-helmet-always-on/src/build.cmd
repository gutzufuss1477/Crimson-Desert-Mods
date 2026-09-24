@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
pushd "%~dp0"
if not exist "out" mkdir "out"
cl /nologo /W3 /O2 /MT /c /Fo:out\ vendor\minhook\src\buffer.c vendor\minhook\src\hook.c vendor\minhook\src\trampoline.c vendor\minhook\src\hde\hde64.c
if errorlevel 1 exit /b 1
lib /nologo /OUT:out\minhook.lib out\buffer.obj out\hook.obj out\trampoline.obj out\hde64.obj
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /Fe:out\state_tests.exe /Fo:out\state_tests.obj state_tests.cpp
if errorlevel 1 exit /b 1
out\state_tests.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /Fe:out\hook_tests.exe /Fo:out\hook_tests.obj hook_tests.cpp /link out\minhook.lib bcrypt.lib user32.lib
if errorlevel 1 exit /b 1
out\hook_tests.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /Fe:out\mode_tests.exe /Fo:out\mode_tests.obj mode_tests.cpp /link out\minhook.lib bcrypt.lib user32.lib
if errorlevel 1 exit /b 1
out\mode_tests.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /Fe:out\ore_tests.exe /Fo:out\ore_tests.obj ore_tests.cpp /link out\minhook.lib bcrypt.lib user32.lib
if errorlevel 1 exit /b 1
out\ore_tests.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /guard:cf /LD /Fo:out\asi.obj asi.cpp /link /OUT:out\Mining_Helmet_Always_On_2.03.00.asi /MAP:out\Mining_Helmet_Always_On_2.03.00.map /DYNAMICBASE /NXCOMPAT /GUARD:CF out\minhook.lib bcrypt.lib user32.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /Fe:out\load_smoke.exe /Fo:out\load_smoke.obj load_smoke.cpp
if errorlevel 1 exit /b 1
out\load_smoke.exe "%CD%\out\Mining_Helmet_Always_On_2.03.00.asi"
if errorlevel 1 exit /b 1
popd
exit /b 0
