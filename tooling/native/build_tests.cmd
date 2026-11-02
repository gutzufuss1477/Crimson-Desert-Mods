@echo off
setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "RESOLVER_VS=%%I"
if not defined RESOLVER_VS exit /b 2
call "%RESOLVER_VS%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 2
pushd "%~dp0"
if not exist out mkdir out
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /Fe:out\resolver_tests.exe /Fo:out\resolver_tests.obj resolver_tests.cpp /link bcrypt.lib
set "RESULT=%errorlevel%"
popd
exit /b %RESULT%
