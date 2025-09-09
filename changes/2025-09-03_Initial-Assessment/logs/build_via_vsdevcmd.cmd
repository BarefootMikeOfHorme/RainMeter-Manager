@echo off
setlocal

set "ROOT=C:\Users\Administrator\RainmeterManager"
set "VSDEV="

for %%P in ("C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat") do (
  if exist %%~P (
    set "VSDEV=%%~P"
    goto :found
  )
)

echo [ERROR] VsDevCmd.bat not found in default VS2022 locations.
echo Tried:
echo   - C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat
echo   - C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat
echo   - C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat
exit /b 2

:found
echo Using VS Dev Cmd: %VSDEV%
call "%VSDEV%" -arch=x64
if errorlevel 1 (
  echo [ERROR] Failed to initialize VS development environment.
  exit /b 3
)

cd /d "%ROOT%"
set "DBGLOG=%~1"
set "RELLOG=%~2"
if "%DBGLOG%"=="" set "DBGLOG=%ROOT%\changes\2025-09-03_Initial-Assessment\logs\build_Debug.txt"
if "%RELLOG%"=="" set "RELLOG=%ROOT%\changes\2025-09-03_Initial-Assessment\logs\build_Release.txt"

echo [INFO] Building Debug x64 ...
call scripts\buildscript.bat --config Debug --platform x64 > "%DBGLOG%" 2>&1
set "ERR1=%ERRORLEVEL%"

echo [INFO] Building Release x64 ...
call scripts\buildscript.bat --config Release --platform x64 > "%RELLOG%" 2>&1
set "ERR2=%ERRORLEVEL%"

if not "%ERR1%"=="0" (
  echo [ERROR] Debug build failed with code %ERR1%
  exit /b %ERR1%
)
if not "%ERR2%"=="0" (
  echo [ERROR] Release build failed with code %ERR2%
  exit /b %ERR2%
)

echo [SUCCESS] Builds completed successfully.
exit /b 0

