@echo off
setlocal

echo Setting up Visual Studio environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 (
    echo Failed to setup VS environment
    exit /b 1
)

echo.
echo Testing compilation of render_ipc_bridge.cpp...
cd /d D:\RainmeterManager
if not exist build\test mkdir build\test
cl /nologo /W4 /O2 /MT /EHsc /std:c++17 /D_UNICODE /DUNICODE /D_WINDOWS /DNDEBUG /Isrc /c src\render\ipc\render_ipc_bridge.cpp /Fobuild\test\render_ipc_bridge.obj

echo.
echo Exit code: %errorlevel%
