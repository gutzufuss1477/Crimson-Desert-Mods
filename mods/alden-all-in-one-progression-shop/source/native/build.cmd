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
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /Fe:out\native_tests.exe /Fo:out\native_tests.obj native_tests.cpp /link bcrypt.lib
if errorlevel 1 exit /b 1
out\native_tests.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /guard:cf /LD /Fo:out\native_tabs.obj native_tabs.cpp /link /OUT:"out\Alden Category Tabs.asi" /MAP:out\native_tabs.map /DYNAMICBASE /NXCOMPAT /GUARD:CF /Brepro bcrypt.lib
if errorlevel 1 exit /b 1
popd
exit /b 0
