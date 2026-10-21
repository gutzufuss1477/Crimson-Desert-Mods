@echo off
setlocal
where cl >nul 2>nul
if not errorlevel 1 goto compiler_ready
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 1
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "BANK_VS=%%i"
if not defined BANK_VS exit /b 1
call "%BANK_VS%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
:compiler_ready
pushd "%~dp0"
if not exist out mkdir out
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /Fe:out\core_tests.exe /Fo:out\core_tests.obj core_tests.cpp
if errorlevel 1 exit /b 1
out\core_tests.exe
if errorlevel 1 exit /b 1
cl /nologo /W3 /O2 /MT /c /Fo:out\ vendor\minhook\src\buffer.c vendor\minhook\src\hook.c vendor\minhook\src\trampoline.c vendor\minhook\src\hde\hde64.c
if errorlevel 1 exit /b 1
lib /nologo /OUT:out\minhook.lib out\buffer.obj out\hook.obj out\trampoline.obj out\hde64.obj
if errorlevel 1 exit /b 1
rc /nologo /foout\bank_refresh.res bank_refresh.rc
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /guard:cf /LD /Fo:out\bank_refresh.obj bank_refresh.cpp /link /OUT:out\Bank_Refresh_2.03.02.asi /MAP:out\Bank_Refresh_2.03.02.map /DYNAMICBASE /NXCOMPAT /GUARD:CF /Brepro out\bank_refresh.res out\minhook.lib bcrypt.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /guard:cf /Fe:out\hook_tests.exe /Fo:out\hook_tests.obj hook_tests.cpp /link out\minhook.lib bcrypt.lib
if errorlevel 1 exit /b 1
out\hook_tests.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /guard:cf /DBANK_REFRESH_TESTING /LD /Fo:out\bank_refresh_test.obj bank_refresh.cpp /link /OUT:out\bank_refresh_instrumented.dll /IMPLIB:out\bank_refresh_test.lib /DYNAMICBASE /NXCOMPAT /GUARD:CF out\minhook.lib bcrypt.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /W4 /WX /EHsc /O2 /MT /Fe:out\load_tests.exe /Fo:out\load_tests.obj load_tests.cpp
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Test-Load.ps1
if errorlevel 1 exit /b 1
popd
exit /b 0
