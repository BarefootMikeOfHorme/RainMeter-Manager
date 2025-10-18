@echo off
setlocal

echo Setting up Visual Studio environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 (
    echo Failed to setup VS environment
    exit /b 1
)

echo.
echo Testing compilation of widget_manager.cpp...
cd /d D:\RainmeterManager
cl /nologo /W4 /O2 /MT /EHsc /D_UNICODE /DUNICODE /D_WINDOWS /DNDEBUG /Isrc /c src\widgets\widget_manager.cpp /Fobuild\test\widget_manager.obj

echo.
echo Exit code: %errorlevel%
