@echo off
setlocal EnableExtensions DisableDelayedExpansion
cd /d "%~dp0"
title Crimson Desert - Bank Refresh Save Patcher

echo ============================================================
echo   Crimson Desert - Bank Refresh Save Patcher v1.3.0
echo ============================================================
echo.

set "PYEXE="
set "PYARGS="
set "PYMGR="

call :find_python
if defined PYEXE goto :python_ready

echo Python 3.10 or newer was not found.
echo Python 3.14 will now be installed automatically.
echo An internet connection is required for this one-time setup.
echo.
call :install_python
if errorlevel 1 goto :python_install_failed

call :find_python
if not defined PYEXE goto :python_install_failed

goto :python_ready

:find_python
set "PYEXE="
set "PYARGS="

where py >nul 2>nul
if not errorlevel 1 (
    py -3 -c "import sys; raise SystemExit(0 if sys.version_info >= (3,10) else 1)" >nul 2>nul
    if not errorlevel 1 (
        set "PYEXE=py"
        set "PYARGS=-3"
        exit /b 0
    )
)

where python >nul 2>nul
if not errorlevel 1 (
    python -c "import sys; raise SystemExit(0 if sys.version_info >= (3,10) else 1)" >nul 2>nul
    if not errorlevel 1 (
        set "PYEXE=python"
        set "PYARGS="
        exit /b 0
    )
)

for %%P in (
    "%LOCALAPPDATA%\Programs\Python\Python314\python.exe"
    "%LOCALAPPDATA%\Programs\Python\Python313\python.exe"
    "%LOCALAPPDATA%\Programs\Python\Python312\python.exe"
    "%LOCALAPPDATA%\Programs\Python\Python311\python.exe"
    "%LOCALAPPDATA%\Programs\Python\Python310\python.exe"
    "%ProgramFiles%\Python314\python.exe"
    "%ProgramFiles%\Python313\python.exe"
    "%ProgramFiles%\Python312\python.exe"
    "%ProgramFiles%\Python311\python.exe"
    "%ProgramFiles%\Python310\python.exe"
) do (
    if exist "%%~fP" (
        "%%~fP" -c "import sys; raise SystemExit(0 if sys.version_info >= (3,10) else 1)" >nul 2>nul
        if not errorlevel 1 (
            set "PYEXE=%%~fP"
            set "PYARGS="
            exit /b 0
        )
    )
)

call :find_pymanager
if defined PYMGR (
    "%PYMGR%" exec -V:3.14 -c "import sys; raise SystemExit(0 if sys.version_info >= (3,10) else 1)" >nul 2>nul
    if not errorlevel 1 (
        set "PYEXE=%PYMGR%"
        set "PYARGS=exec -V:3.14"
        exit /b 0
    )
)

exit /b 1

:find_pymanager
set "PYMGR="
where pymanager >nul 2>nul
if not errorlevel 1 (
    set "PYMGR=pymanager"
    exit /b 0
)

for %%P in (
    "%LOCALAPPDATA%\Microsoft\WindowsApps\pymanager.exe"
    "%LOCALAPPDATA%\Microsoft\WindowsApps\PythonSoftwareFoundation.PythonManager_3847v3x7pw1km\pymanager.exe"
    "%LOCALAPPDATA%\Microsoft\WindowsApps\PythonSoftwareFoundation.PythonManager_qbz5n2kfra8p0\pymanager.exe"
) do (
    if exist "%%~fP" (
        set "PYMGR=%%~fP"
        exit /b 0
    )
)

for /d %%D in ("%LOCALAPPDATA%\Microsoft\WindowsApps\PythonSoftwareFoundation.PythonManager_*") do (
    if exist "%%~fD\pymanager.exe" (
        set "PYMGR=%%~fD\pymanager.exe"
        exit /b 0
    )
)
exit /b 1

:install_python
where winget >nul 2>nul
if not errorlevel 1 (
    echo Installing the official Python Install Manager with WinGet...
    winget install 9NQ7512CXL7T -e --accept-package-agreements --accept-source-agreements --disable-interactivity
    if not errorlevel 1 goto :install_runtime
    echo.
    echo WinGet installation failed. Trying the official Python.org installer...
)

where powershell >nul 2>nul
if errorlevel 1 exit /b 1

echo Installing the official Python Install Manager from python.org...
powershell -NoProfile -ExecutionPolicy Bypass -Command "try { Add-AppxPackage -AppInstallerFile 'https://www.python.org/ftp/python/pymanager/pymanager.appinstaller' -ErrorAction Stop; exit 0 } catch { Write-Host $_.Exception.Message; exit 1 }"
if errorlevel 1 exit /b 1

:install_runtime
call :find_pymanager
if not defined PYMGR (
    echo.
    echo Python Install Manager was installed, but its command could not be located yet.
    exit /b 1
)

echo.
echo Installing Python 3.14 runtime...
"%PYMGR%" install 3.14
if errorlevel 1 exit /b 1

"%PYMGR%" exec -V:3.14 -c "import sys; print('Python', sys.version.split()[0], 'is ready.')"
if errorlevel 1 exit /b 1

set "PYEXE=%PYMGR%"
set "PYARGS=exec -V:3.14"
exit /b 0

:python_install_failed
echo.
echo ERROR: Python could not be installed automatically.
echo Check the internet connection and make sure Microsoft App Installer is available.
echo No save was modified.
pause
exit /b 1

:python_ready
"%PYEXE%" %PYARGS% -c "import sys; raise SystemExit(0 if sys.version_info >= (3,10) else 1)" >nul 2>nul
if errorlevel 1 (
    echo Python 3.10 or newer is required.
    echo No save was modified.
    pause
    exit /b 1
)

"%PYEXE%" %PYARGS% -c "import lz4.block" >nul 2>nul
if errorlevel 1 (
    echo Installing required Python package: lz4
    "%PYEXE%" %PYARGS% -m pip install --user lz4
    if errorlevel 1 (
        echo.
        echo ERROR: Could not install lz4. Check the internet connection and try again.
        echo No save was modified.
        pause
        exit /b 1
    )
)

echo.
"%PYEXE%" %PYARGS% "%~dp0crimson_bank_save_helper.py"
set "RC=%errorlevel%"
echo.
pause
exit /b %RC%
