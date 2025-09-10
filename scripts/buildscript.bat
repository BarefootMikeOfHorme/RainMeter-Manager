@echo off
setlocal enabledelayedexpansion

REM ========================================================================
REM Enterprise Build Script for Rainmeter Manager
REM
REM Features:
REM - Robust error handling and logging
REM - Configurable build options
REM - Support for different build configurations (Debug/Release)
REM - Automatic dependency checking
REM - Proper versioning support
REM - Build timing metrics
REM ========================================================================

REM ========== CONFIGURATION ==========
set "PROJECT_NAME=RainmeterManager"
set "VERSION_MAJOR=1"
set "VERSION_MINOR=0"
set "VERSION_PATCH=0"
set "BUILD_NUMBER=0"
if defined CI_BUILD_NUMBER set "BUILD_NUMBER=%CI_BUILD_NUMBER%"

REM Default build configuration
set "CONFIG=Release"
set "PLATFORM=x64"
set "VERBOSE=0"
set "SKIP_TESTS=0"
set "SKIP_INSTALLER=0"
set "LOG_FILE=build_log_%date:~-4,4%%date:~-7,2%%date:~-10,2%_%time:~0,2%%time:~3,2%%time:~6,2%.txt"
set "LOG_FILE=%LOG_FILE: =0%"

REM ========== PARSE COMMAND LINE ARGUMENTS ==========
:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="-h" goto :show_help
if /i "%~1"=="--help" goto :show_help
if /i "%~1"=="-c" set "CONFIG=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="--config" set "CONFIG=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="-p" set "PLATFORM=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="--platform" set "PLATFORM=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="-v" set "VERBOSE=1" & shift & goto :parse_args
if /i "%~1"=="--verbose" set "VERBOSE=1" & shift & goto :parse_args
if /i "%~1"=="--skip-tests" set "SKIP_TESTS=1" & shift & goto :parse_args
if /i "%~1"=="--skip-installer" set "SKIP_INSTALLER=1" & shift & goto :parse_args
if /i "%~1"=="--version" (
    echo %PROJECT_NAME% v%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH% (Build %BUILD_NUMBER%)
    exit /b 0
)
echo Unknown option: %~1
goto :show_help

:show_help
echo.
echo %PROJECT_NAME% Build Script
echo Usage: %~nx0 [options]
echo.
echo Options:
echo   -h, --help             Show this help message
echo   -c, --config CONFIG    Build configuration (Debug or Release, default: Release)
echo   -p, --platform PLATFORM  Build platform (x86 or x64, default: x64)
echo   -v, --verbose          Enable verbose output
echo   --skip-tests           Skip running tests
echo   --skip-installer       Skip building the installer
echo   --version              Show version information
echo.
exit /b 0

:args_done

REM ========== SETUP LOGGING ==========
set "ROOT_DIR=%~dp0.."
set "BUILD_DIR=%ROOT_DIR%\build"
set "LOG_DIR=%BUILD_DIR%\logs"
set "BIN_DIR=%BUILD_DIR%\bin\%PLATFORM%\%CONFIG%"
set "OBJ_DIR=%BUILD_DIR%\obj\%PLATFORM%\%CONFIG%"
set "RES_DIR=%ROOT_DIR%\resources"
set "SRC_DIR=%ROOT_DIR%\src"
set "TEST_DIR=%ROOT_DIR%\tests"
set "INSTALLER_DIR=%ROOT_DIR%\installer"

if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"
set "LOG_PATH=%LOG_DIR%\%LOG_FILE%"

REM Start build timer
set "BUILD_START_TIME=%time%"

echo [%date% %time%] [INFO] Build script started > "%LOG_PATH%"
echo [%date% %time%] [INFO] Configuration: %CONFIG% >> "%LOG_PATH%"
echo [%date% %time%] [INFO] Platform: %PLATFORM% >> "%LOG_PATH%"

echo %PROJECT_NAME% Build Script [%CONFIG%|%PLATFORM%]
echo ============================================================

REM ========== VERIFY ENVIRONMENT ==========
echo Verifying build environment...
echo [%date% %time%] [INFO] Running Phase 2 enhanced dependency verification... >> "%LOG_PATH%"

REM Run PowerShell dependency verification script if available
if exist "%ROOT_DIR%\scripts\verify_dependencies.ps1" (
    echo Running comprehensive dependency check...
    powershell -ExecutionPolicy Bypass -File "%ROOT_DIR%\scripts\verify_dependencies.ps1" -Verbose
    if %ERRORLEVEL% NEQ 0 (
        echo [%date% %time%] [ERROR] Dependency verification failed >> "%LOG_PATH%"
        echo ERROR: Critical dependencies missing. See verification output above.
        exit /b 1
    )
    echo [%date% %time%] [SUCCESS] All dependencies verified successfully >> "%LOG_PATH%"
) else (
    echo [%date% %time%] [WARNING] Dependency verification script not found, running basic checks >> "%LOG_PATH%"
)

REM Check for Visual Studio environment
where cl >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [%date% %time%] [ERROR] Visual C++ Compiler (cl.exe) not found. >> "%LOG_PATH%"
    echo ERROR: Visual C++ Compiler (cl.exe) not found.
    echo Please run this script from a Visual Studio Developer Command Prompt.
    echo You can find it in: Start Menu -^> Visual Studio 20XX -^> Developer Command Prompt
    exit /b 1
)

REM Check resource compiler
where rc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [%date% %time%] [ERROR] Resource Compiler (rc.exe) not found. >> "%LOG_PATH%"
    echo ERROR: Resource Compiler (rc.exe) not found.
    echo Please run this script from a Visual Studio Developer Command Prompt.
    exit /b 1
)

REM Check for NSIS if installer is requested
if "%SKIP_INSTALLER%"=="0" (
    where makensis >nul 2>&1
    if %ERRORLEVEL% NEQ 0 (
        echo [%date% %time%] [WARNING] NSIS Compiler (makensis.exe) not found. >> "%LOG_PATH%"
        echo WARNING: NSIS Compiler (makensis.exe) not found.
        echo Installer will not be created.
        echo Please install NSIS from https://nsis.sourceforge.io/Download
        set "SKIP_INSTALLER=1"
    )
)

REM ========== CREATE BUILD DIRECTORIES ==========
echo Creating build directories...
echo [%date% %time%] [INFO] Creating build directories... >> "%LOG_PATH%"

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"
if not exist "%OBJ_DIR%" mkdir "%OBJ_DIR%"

REM ========== SET COMPILER FLAGS ==========
if /i "%CONFIG%"=="Debug" (
    set "CFLAGS=/nologo /W4 /Od /Zi /MTd /EHsc /D_UNICODE /DUNICODE /D_WINDOWS /D_DEBUG /Fd%OBJ_DIR%\ /Fo%OBJ_DIR%\ /c"
    set "LFLAGS=/nologo /DEBUG /SUBSYSTEM:WINDOWS /INCREMENTAL user32.lib gdi32.lib comctl32.lib shell32.lib shlwapi.lib comdlg32.lib ole32.lib advapi32.lib wininet.lib dbghelp.lib version.lib bcrypt.lib wintrust.lib crypt32.lib"
    echo [%date% %time%] [INFO] Using Debug configuration flags >> "%LOG_PATH%"
) else (
    set "CFLAGS=/nologo /W4 /O2 /GL /MT /EHsc /D_UNICODE /DUNICODE /D_WINDOWS /DNDEBUG /Fo%OBJ_DIR%\ /c"
    set "LFLAGS=/nologo /SUBSYSTEM:WINDOWS /LTCG user32.lib gdi32.lib comctl32.lib shell32.lib shlwapi.lib comdlg32.lib ole32.lib advapi32.lib wininet.lib dbghelp.lib version.lib bcrypt.lib wintrust.lib crypt32.lib"
    echo [%date% %time%] [INFO] Using Release configuration flags >> "%LOG_PATH%"
)

REM Set version information
set "CFLAGS=%CFLAGS% /DVERSION_MAJOR=%VERSION_MAJOR% /DVERSION_MINOR=%VERSION_MINOR% /DVERSION_PATCH=%VERSION_PATCH% /DBUILD_NUMBER=%BUILD_NUMBER%"

REM ========== PHASE 2 APPLICATION CORE CHECKS ==========
echo Checking Phase 2 application core requirements...
echo [%date% %time%] [INFO] Checking Phase 2 application core requirements... >> "%LOG_PATH%"

REM Check for RAINMGRApp singleton implementation
if exist "%SRC_DIR%\app\rainmgrapp.cpp" (
    echo [%date% %time%] [SUCCESS] RAINMGRApp singleton implementation found >> "%LOG_PATH%"
) else (
    echo [%date% %time%] [WARNING] RAINMGRApp singleton not implemented yet - Phase 2 target >> "%LOG_PATH%"
)

REM Check for service locator pattern
if exist "%SRC_DIR%\core\service_locator.cpp" (
    echo [%date% %time%] [SUCCESS] Service locator implementation found >> "%LOG_PATH%"
) else (
    echo [%date% %time%] [WARNING] Service locator not implemented yet - Phase 2 target >> "%LOG_PATH%"
)

REM Check for configuration management system
if exist "%SRC_DIR%\config\config_manager.cpp" (
    echo [%date% %time%] [SUCCESS] Configuration management system found >> "%LOG_PATH%"
) else (
    echo [%date% %time%] [WARNING] Configuration management not implemented yet - Phase 2 target >> "%LOG_PATH%"
)

REM ========== PREBUILD STEPS ==========
echo Running prebuild steps...
echo [%date% %time%] [INFO] Running prebuild steps... >> "%LOG_PATH%"

REM Generate version header
echo #pragma once > "%SRC_DIR%\version.h"
echo #define VERSION_MAJOR %VERSION_MAJOR% >> "%SRC_DIR%\version.h"
echo #define VERSION_MINOR %VERSION_MINOR% >> "%SRC_DIR%\version.h"
echo #define VERSION_PATCH %VERSION_PATCH% >> "%SRC_DIR%\version.h"
echo #define BUILD_NUMBER %BUILD_NUMBER% >> "%SRC_DIR%\version.h"
echo #define VERSION_STRING "%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%.%BUILD_NUMBER%" >> "%SRC_DIR%\version.h"

REM ========== BUILD RESOURCES ==========
echo Building resources...
echo [%date% %time%] [INFO] Building resources... >> "%LOG_PATH%"

rc /nologo /d%CONFIG% /fo"%OBJ_DIR%\resource.res" "%RES_DIR%\resource.rc"
if %ERRORLEVEL% NEQ 0 (
    echo [%date% %time%] [ERROR] Resource compilation failed >> "%LOG_PATH%"
    echo ERROR: Resource compilation failed
    exit /b 1
)

REM ========== COMPILE SOURCE FILES ==========
echo Compiling source files...
echo [%date% %time%] [INFO] Compiling source files... >> "%LOG_PATH%"

REM Get list of source files
set "SOURCE_FILES="
for /r "%SRC_DIR%" %%F in (*.c *.cpp) do (
    set "SOURCE_FILES=!SOURCE_FILES! "%%F""
)

if "%VERBOSE%"=="1" echo Source files to compile: %SOURCE_FILES%

REM Compile each source file
for %%F in (%SOURCE_FILES%) do (
    set "FILENAME=%%~nxF"
    if "%VERBOSE%"=="1" echo Compiling %%F...
    echo [%date% %time%] [INFO] Compiling %%F... >> "%LOG_PATH%"
    cl %CFLAGS% "%%F"
    if !ERRORLEVEL! NEQ 0 (
        echo [%date% %time%] [ERROR] Failed to compile %%F >> "%LOG_PATH%"
        echo ERROR: Failed to compile %%F
        exit /b 1
    )
)

REM ========== LINK APPLICATION ==========
echo Linking application...
echo [%date% %time%] [INFO] Linking application... >> "%LOG_PATH%"

REM Get list of object files
set "OBJ_FILES="
for %%F in ("%OBJ_DIR%\*.obj") do (
    set "OBJ_FILES=!OBJ_FILES! "%%F""
)

REM Link the application
link %LFLAGS% /OUT:"%BIN_DIR%\%PROJECT_NAME%.exe" %OBJ_FILES% "%OBJ_DIR%\resource.res"

if %ERRORLEVEL% NEQ 0 (
    echo [%date% %time%] [ERROR] Linking failed >> "%LOG_PATH%"
    echo ERROR: Build failed
    exit /b 1
)

REM ========== RUN TESTS ==========
if "%SKIP_TESTS%"=="0" (
    echo Running tests...
    echo [%date% %time%] [INFO] Running tests... >> "%LOG_PATH%"
    
    if exist "%TEST_DIR%\run_tests.bat" (
        call "%TEST_DIR%\run_tests.bat" "%BIN_DIR%"
        if !ERRORLEVEL! NEQ 0 (
            echo [%date% %time%] [ERROR] Tests failed >> "%LOG_PATH%"
            echo ERROR: Tests failed
            exit /b 1
        )
    ) else (
        echo [%date% %time%] [WARNING] No test script found at %TEST_DIR%\run_tests.bat >> "%LOG_PATH%"
        echo WARNING: No test script found at %TEST_DIR%\run_tests.bat
    )
) else (
    echo [%date% %time%] [INFO] Skipping tests as requested >> "%LOG_PATH%"
    echo Skipping tests as requested
)

REM ========== COPY REQUIRED FILES ==========
echo Copying required files to bin directory...
echo [%date% %time%] [INFO] Copying required files to bin directory... >> "%LOG_PATH%"

xcopy /y "%RES_DIR%\folder_icon.png" "%BIN_DIR%\" >nul
xcopy /y "%RES_DIR%\icon.ico" "%BIN_DIR%\" >nul
xcopy /y "%ROOT_DIR%\LICENSE.txt" "%BIN_DIR%\" >nul

echo Creating license file for installer...
echo [%date% %time%] [INFO] Creating license file for installer... >> "%LOG_PATH%"

echo Rainmeter Manager License > "%BIN_DIR%\LICENSE.txt"
 echo. >> "%BIN_DIR%\LICENSE.txt"
 echo Copyright (c) 2025 Michael D Shortland (aka BarefootMike) with his wonderful assistant WarpTerminal >> "%BIN_DIR%\LICENSE.txt"
 echo. >> "%BIN_DIR%\LICENSE.txt"
 echo Permission is hereby granted, free of charge, to any person obtaining a copy >> "%BIN_DIR%\LICENSE.txt"
 echo of this software and associated documentation files... >> "%BIN_DIR%\LICENSE.txt"

REM ========== CREATE INSTALLER ==========
if "%SKIP_INSTALLER%"=="1" (
    echo [%date% %time%] [INFO] Skipping installer creation as requested >> "%LOG_PATH%"
    echo Skipping installer creation.
) else (
    echo Creating installer...
    echo [%date% %time%] [INFO] Creating installer... >> "%LOG_PATH%"
    
    REM Copy installer resources
    xcopy /y "%INSTALLER_DIR%\installer.nsi" "%BIN_DIR%\" >nul
    xcopy /y "%RES_DIR%\installer_image.bmp" "%BIN_DIR%\" >nul
    xcopy /y "%RES_DIR%\header.bmp" "%BIN_DIR%\" >nul
    
    REM Update version information in installer script
    powershell -Command "(Get-Content '%BIN_DIR%\installer.nsi') -replace '!define VERSION \".*\"', '!define VERSION \"%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%.%BUILD_NUMBER%\"' | Set-Content '%BIN_DIR%\installer.nsi'"
    
    REM Build installer
    pushd "%BIN_DIR%"
    makensis /V2 installer.nsi
    if %ERRORLEVEL% NEQ 0 (
        popd
        echo [%date% %time%] [ERROR] Installer creation failed >> "%LOG_PATH%"
        echo ERROR: Installer creation failed
        exit /b 1
    )
    popd
    
    echo Moving installer to build directory...
    move /y "%BIN_DIR%\%PROJECT_NAME%-Setup.exe" "%BUILD_DIR%\%PROJECT_NAME%-%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%.%BUILD_NUMBER%-Setup.exe" >nul
    
    echo [%date% %time%] [INFO] Installer created: %BUILD_DIR%\%PROJECT_NAME%-%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%.%BUILD_NUMBER%-Setup.exe >> "%LOG_PATH%"
)

REM ========== CALCULATE BUILD TIME ==========
set "BUILD_END_TIME=%time%"

REM Convert start time to seconds
set /a "START_H=1%BUILD_START_TIME:~0,2%-100"
set /a "START_M=1%BUILD_START_TIME:~3,2%-100"
set /a "START_S=1%BUILD_START_TIME:~6,2%-100"
set /a "START_TOTAL_S=(START_H*3600)+(START_M*60)+START_S"

REM Convert end time to seconds
set /a "END_H=1%BUILD_END_TIME:~0,2%-100"
set /a "END_M=1%BUILD_END_TIME:~3,2%-100"
set /a "END_S=1%BUILD_END_TIME:~6,2%-100"
set /a "END_TOTAL_S=(END_H*3600)+(END_M*60)+END_S"

REM Handle day crossover
if %END_TOTAL_S% LSS %START_TOTAL_S% set /a "END_TOTAL_S+=86400"

REM Calculate duration
set /a "DURATION_S=END_TOTAL_S-START_TOTAL_S"
set /a "DURATION_M=DURATION_S/60"
set /a "DURATION_S=DURATION_S%%60"
set /a "DURATION_H=DURATION_M/60"
set /a "DURATION_M=DURATION_M%%60"

REM Format duration
if %DURATION_H% LSS 10 set "DURATION_H=0%DURATION_H%"
if %DURATION_M% LSS 10 set "DURATION_M=0%DURATION_M%"
if %DURATION_S% LSS 10 set "DURATION_S=0%DURATION_S%"
set "BUILD_DURATION=%DURATION_H%:%DURATION_M%:%DURATION_S%"

REM ========== FINALIZE ==========
echo [%date% %time%] [SUCCESS] Build completed successfully in %BUILD_DURATION%! >> "%LOG_PATH%"

echo.
echo ============================================================
echo Build completed successfully in %BUILD_DURATION%!
echo ============================================================
echo Output executable: %BIN_DIR%\%PROJECT_NAME%.exe

if "%SKIP_INSTALLER%"=="0" (
    echo Installer: %BUILD_DIR%\%PROJECT_NAME%-%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%.%BUILD_NUMBER%-Setup.exe
)

exit /b 0
