@echo off
setlocal
if "%VSCMD_ARG_TGT_ARCH%"=="x64" goto compiler_ready
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "ALDEN_VSROOT=%%I"
if not defined ALDEN_VSROOT exit /b 2
call "%ALDEN_VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 2
:compiler_ready
pushd "%~dp0"
if not exist out mkdir out
cl /nologo /W3 /O2 /MT /c /Fo:out\ vendor\minhook\src\buffer.c vendor\minhook\src\hook.c vendor\minhook\src\trampoline.c vendor\minhook\src\hde\hde64.c
if errorlevel 1 exit /b 1
set "ALDEN_HOOK_OBJS=out\buffer.obj out\hook.obj out\trampoline.obj out\hde64.obj"
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /DALDEN_RUNTIME_UI /Fe:out\native_tests.exe /Fo:out\native_tests.obj native_tests.cpp %ALDEN_HOOK_OBJS% /link bcrypt.lib
if errorlevel 1 exit /b 1
out\native_tests.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /DALDEN_RUNTIME_UI /Fe:out\runtime_ui_tests.exe /Fo:out\runtime_ui_tests.obj runtime_ui_tests.cpp %ALDEN_HOOK_OBJS% /link bcrypt.lib
if errorlevel 1 exit /b 1
out\runtime_ui_tests.exe fixtures
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /guard:cf /DALDEN_RUNTIME_UI /LD /Fo:out\native_tabs.obj native_tabs.cpp %ALDEN_HOOK_OBJS% /link /OUT:"out\Alden Category Tabs.asi" /MAP:out\native_tabs.map /DYNAMICBASE /NXCOMPAT /GUARD:CF /Brepro bcrypt.lib
if errorlevel 1 exit /b 1
popd
exit /b 0
