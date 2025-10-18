@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cd /d D:\RainmeterManager
call scripts\buildscript.bat -c Release -p x64 --skip-installer --skip-tests
