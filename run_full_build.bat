@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
if errorlevel 1 (
    echo Failed to setup VS environment
    exit /b 1
)
cd /d D:\RainmeterManager
call scripts\buildscript.bat -c Release -p x64 --skip-installer --skip-tests
