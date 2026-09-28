@echo off
setlocal
rem The public build uses only the verified material implementation.
rem Historical full-mode source files in this directory are not linked.
if "%VSCMD_ARG_TGT_ARCH%"=="x64" goto compiler_ready
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 2
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "MINING_VSROOT=%%I"
if not defined MINING_VSROOT exit /b 2
call "%MINING_VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "%errorlevel%"=="0" exit /b %errorlevel%
:compiler_ready
pushd "%~dp0material"
if not exist "out" mkdir "out"
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /Fe:out\automatic_material_tests.exe /Fo:out\automatic_material_tests.obj automatic_material_tests.cpp /link bcrypt.lib user32.lib
if not "%errorlevel%"=="0" exit /b %errorlevel%
out\automatic_material_tests.exe
if not "%errorlevel%"=="0" exit /b %errorlevel%
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /guard:cf /LD /Fo:out\automatic_material.obj automatic_material.cpp /link /OUT:out\Mining_Helmet_Always_On_2.03.02.asi /MAP:out\Mining_Helmet_Always_On_2.03.02.map /DYNAMICBASE /NXCOMPAT /GUARD:CF bcrypt.lib user32.lib
if not "%errorlevel%"=="0" exit /b %errorlevel%
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /Fe:out\load_smoke.exe /Fo:out\load_smoke.obj load_smoke.cpp
if not "%errorlevel%"=="0" exit /b %errorlevel%
out\load_smoke.exe "%CD%\out\Mining_Helmet_Always_On_2.03.02.asi"
if not "%errorlevel%"=="0" exit /b %errorlevel%
popd
exit /b 0
