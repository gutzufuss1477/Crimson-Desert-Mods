@echo off
setlocal
if "%VSCMD_ARG_TGT_ARCH%"=="x64" goto compiler_ready
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 2
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "MINING_VSROOT=%%I"
if not defined MINING_VSROOT exit /b 2
call "%MINING_VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "%errorlevel%"=="0" exit /b %errorlevel%
:compiler_ready
pushd "%~dp0runtime"
if not exist "out" mkdir "out"
cl /nologo /W3 /O2 /MT /c /Fo:out\ ..\vendor\minhook\src\buffer.c ..\vendor\minhook\src\hook.c ..\vendor\minhook\src\trampoline.c ..\vendor\minhook\src\hde\hde64.c
if not "%errorlevel%"=="0" exit /b %errorlevel%
lib /nologo /OUT:out\minhook.lib out\buffer.obj out\hook.obj out\trampoline.obj out\hde64.obj
if not "%errorlevel%"=="0" exit /b %errorlevel%
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /Fe:out\range_ini_tests.exe /Fo:out\range_ini_tests.obj runtime_tests.cpp /link out\minhook.lib bcrypt.lib user32.lib
if not "%errorlevel%"=="0" exit /b %errorlevel%
out\range_ini_tests.exe
if not "%errorlevel%"=="0" exit /b %errorlevel%
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /guard:cf /LD /Fo:out\range_ini.obj mining_helmet.cpp /link /OUT:out\Mining_Helmet_Always_On_2.03.02.asi /MAP:out\range_ini.map /DYNAMICBASE /NXCOMPAT /GUARD:CF out\minhook.lib bcrypt.lib user32.lib
if not "%errorlevel%"=="0" exit /b %errorlevel%
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /Fe:out\load_smoke.exe /Fo:out\load_smoke.obj ..\material\load_smoke.cpp
if not "%errorlevel%"=="0" exit /b %errorlevel%
out\load_smoke.exe "%CD%\out\Mining_Helmet_Always_On_2.03.02.asi"
if not "%errorlevel%"=="0" exit /b %errorlevel%
popd
exit /b 0
