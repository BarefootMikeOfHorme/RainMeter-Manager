@echo off
setlocal enabledelayedexpansion

set "ROOT_DIR=%~dp0"
set "LOG_DIR=%ROOT_DIR%build\logs"

echo Root dir: %ROOT_DIR%
echo Log dir: %LOG_DIR%

if not exist "%LOG_DIR%" (
    echo Creating log directory...
    mkdir "%LOG_DIR%"
)

echo Done!
