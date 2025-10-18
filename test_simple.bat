@echo off
setlocal enabledelayedexpansion

set "TESTDIR=D:\RainmeterManager\build\logs"

echo Testing if not exist...
if not exist "%TESTDIR%" (
    echo Directory does not exist
)

echo Done!
