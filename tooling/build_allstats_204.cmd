@echo off
setlocal
if "%VSCMD_ARG_TGT_ARCH%"=="x64" goto compiler_ready
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "MOUNT_VSROOT=%%I"
if not defined MOUNT_VSROOT exit /b 2
call "%MOUNT_VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 2
:compiler_ready
pushd "%~dp0"
if not exist out mkdir out
set "MOD=..\mods\real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor\variants"
cl /nologo /O2 /GS- /Oi /LD /Fo:out\stats.obj "%MOD%\all-mounts-level-5-all-stats\src\AllMountsLvL5AllStats_20402.c" /link /OUT:out\AllMountsLvL5AllStats_2.04.02_b26329202.asi /NODEFAULTLIB /ENTRY:DllMain /DYNAMICBASE /NXCOMPAT /Brepro
if errorlevel 1 exit /b 1
cl /nologo /O2 /MT /DALL_STATS /Fe:out\stats_tests.exe /Fo:out\stats_tests.obj allstats_tests.c
if errorlevel 1 exit /b 1
out\stats_tests.exe
if errorlevel 1 exit /b 1
popd
exit /b 0
