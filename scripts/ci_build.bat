@echo off
setlocal enabledelayedexpansion

REM ========================================================================
REM Local CI/CD Script for Rainmeter Manager
REM
# This script automates the full build, test, and packaging process.
# It can be run manually or scheduled with Windows Task Scheduler.
#
# Features:
# - Automated build of multiple configurations
# - Automated testing
# - Static code analysis (if configured)
# - Automated packaging
# - Build status reporting
# - Version tracking in metadata without filename changes
# - Detailed changelog generation tracking actual code changes
# - Phase 2 application core layer validation
# - Enhanced dependency verification
REM
REM VERSIONING SYSTEM NOTES:
REM This system tracks versions in metadata only, not in filenames.
REM All version information is stored in the following locations:
REM  - version.h (source code version definitions)
REM  - build_info_*.txt files (per-build metadata)
REM  - changelog_*.txt files (detailed code changes)
REM  - version_history.csv (centralized version database)
REM
REM Filenames remain consistent across builds to ensure stable references
REM while still maintaining full traceability of changes.
REM ========================================================================

REM ========== CONFIGURATION ==========
set "PROJECT_NAME=RainmeterManager"
set "VERSION_FILE=%~dp0\..\src\version.h"
set "LOG_DIR=%~dp0\..\build\logs"
set "TIMESTAMP=%date:~-4,4%%date:~-7,2%%date:~-10,2%_%time:~0,2%%time:~3,2%%time:~6,2%"
set "TIMESTAMP=%TIMESTAMP: =0%"
set "LOG_FILE=%LOG_DIR%\ci_build_%TIMESTAMP%.log"
set "SUMMARY_FILE=%LOG_DIR%\ci_summary_%TIMESTAMP%.txt"
set "CHANGELOG_FILE=%LOG_DIR%\changelog_%TIMESTAMP%.txt"
set "BUILD_SCRIPT=%~dp0\buildscript.bat"
set "FORCE_INCREMENT=0"
set "SKIP_TESTS=0"
set "EMAIL_REPORT=0"
set "EMAIL_RECIPIENT="
set "CHANGELOG_ENABLED=1"
set "SRC_DIR=%~dp0\..\src"

REM ========== PARSE COMMAND LINE ARGUMENTS ==========
:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="-h" goto :show_help
if /i "%~1"=="--help" goto :show_help
if /i "%~1"=="--increment" set "FORCE_INCREMENT=1" & shift & goto :parse_args
if /i "%~1"=="--skip-tests" set "SKIP_TESTS=1" & shift & goto :parse_args
if /i "%~1"=="--email" set "EMAIL_REPORT=1" & set "EMAIL_RECIPIENT=%~2" & shift & shift & goto :parse_args
goto :args_done

:show_help
echo.
echo %PROJECT_NAME% CI Build Script
echo Usage: %~nx0 [options]
echo.
echo Options:
echo   -h, --help             Show this help message
echo   --increment            Force version increment
echo   --skip-tests           Skip running tests
echo   --email recipient      Send email report to specified recipient
echo.
exit /b 0

:args_done

REM ========== SETUP LOGGING ==========
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"
echo [%date% %time%] CI Build started > "%LOG_FILE%"
echo %PROJECT_NAME% CI Build Report > "%SUMMARY_FILE%"
echo ====================================================== >> "%SUMMARY_FILE%"
echo Build started: %date% %time% >> "%SUMMARY_FILE%"
echo. >> "%SUMMARY_FILE%"

REM ========== FUNCTIONS ==========
:log_message
set "LEVEL=%~1"
set "MESSAGE=%~2"
set "TIMESTAMP=%date% %time%"
echo [%TIMESTAMP%] [%LEVEL%] %MESSAGE%
echo [%TIMESTAMP%] [%LEVEL%] %MESSAGE% >> "%LOG_FILE%"
goto :eof

:generate_changelog
call :log_message "INFO" "Generating changelog..."

REM Create changelog file
echo CHANGELOG - Version %VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%.%BUILD_NUMBER% > "%CHANGELOG_FILE%"
echo ============================================================ >> "%CHANGELOG_FILE%"
echo Generated: %date% %time% >> "%CHANGELOG_FILE%"
echo. >> "%CHANGELOG_FILE%"

REM Check if git is available to generate better changelogs
git --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    call :log_message "INFO" "Using git to generate detailed changelog"
    
    REM Check if this is a git repository
    git rev-parse --is-inside-work-tree >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        REM Get previous build version tag if it exists
        for /f "tokens=*" %%a in ('git tag -l "v*" --sort=-v:refname 2^>nul ^| findstr /R "^v[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*$" ^| head -n 1') do set "PREV_VERSION=%%a"
        
        if defined PREV_VERSION (
            REM Get changes since previous version
            echo Changes since %PREV_VERSION%: >> "%CHANGELOG_FILE%"
            echo. >> "%CHANGELOG_FILE%"
            git log --pretty=format:"%%h - %%s (%%an, %%ad)" --date=short %PREV_VERSION%..HEAD >> "%CHANGELOG_FILE%"
            
            REM Get file changes
            echo. >> "%CHANGELOG_FILE%"
            echo. >> "%CHANGELOG_FILE%"
            echo Files changed since %PREV_VERSION%: >> "%CHANGELOG_FILE%"
            echo. >> "%CHANGELOG_FILE%"
            git diff --name-status %PREV_VERSION%..HEAD >> "%CHANGELOG_FILE%"
            
            REM Get detailed diff of important files (like .c, .cpp, .h files)
            echo. >> "%CHANGELOG_FILE%"
            echo. >> "%CHANGELOG_FILE%"
            echo Detailed changes in source files: >> "%CHANGELOG_FILE%"
            echo. >> "%CHANGELOG_FILE%"
            git diff %PREV_VERSION%..HEAD --unified=3 -- "%SRC_DIR%\*.c" "%SRC_DIR%\*.cpp" "%SRC_DIR%\*.h" >> "%CHANGELOG_FILE%"
        ) else (
            REM If no previous version tag, get all changes
            echo All changes (no previous version tag found): >> "%CHANGELOG_FILE%"
            echo. >> "%CHANGELOG_FILE%"
            git log --pretty=format:"%%h - %%s (%%an, %%ad)" --date=short >> "%CHANGELOG_FILE%"
            
            REM Get file listing
            echo. >> "%CHANGELOG_FILE%"
            echo. >> "%CHANGELOG_FILE%"
            echo Current files: >> "%CHANGELOG_FILE%"
            echo. >> "%CHANGELOG_FILE%"
            git ls-files >> "%CHANGELOG_FILE%"
        )
    ) else (
        call :log_message "WARNING" "Not a git repository, using manual changelog generation"
        goto :manual_changelog
    )
) else (
    call :log_message "WARNING" "Git not found, using manual changelog generation"
    goto :manual_changelog
)

goto :changelog_done

:manual_changelog
REM Manual changelog generation by comparing file timestamps
echo Manual changelog (based on file timestamps): >> "%CHANGELOG_FILE%"
echo. >> "%CHANGELOG_FILE%"

REM Find recently modified files (last 7 days)
echo Recently modified files: >> "%CHANGELOG_FILE%"
echo. >> "%CHANGELOG_FILE%"

REM Use forfiles to find files modified in the last week
forfiles /P "%SRC_DIR%" /S /D -7 /C "cmd /c echo @path - @fdate @ftime" >> "%CHANGELOG_FILE%" 2>nul

REM Include a listing of source files for reference
echo. >> "%CHANGELOG_FILE%"
echo. >> "%CHANGELOG_FILE%"
echo Source files listing: >> "%CHANGELOG_FILE%"
echo. >> "%CHANGELOG_FILE%"
dir /B /S "%SRC_DIR%\*.c" "%SRC_DIR%\*.cpp" "%SRC_DIR%\*.h" >> "%CHANGELOG_FILE%" 2>nul

:changelog_done
call :log_message "INFO" "Changelog generated: %CHANGELOG_FILE%"
goto :eof

:increment_version
call :log_message "INFO" "Incrementing build version..."

if not exist "%VERSION_FILE%" (
    call :log_message "ERROR" "Version file not found: %VERSION_FILE%"
    goto :eof
)

REM Read current version
set "VERSION_MAJOR="
set "VERSION_MINOR="
set "VERSION_PATCH="
set "BUILD_NUMBER="

for /f "tokens=2" %%i in ('findstr /C:"#define VERSION_MAJOR" "%VERSION_FILE%"') do set "VERSION_MAJOR=%%i"
for /f "tokens=2" %%i in ('findstr /C:"#define VERSION_MINOR" "%VERSION_FILE%"') do set "VERSION_MINOR=%%i"
for /f "tokens=2" %%i in ('findstr /C:"#define VERSION_PATCH" "%VERSION_FILE%"') do set "VERSION_PATCH=%%i"
for /f "tokens=2" %%i in ('findstr /C:"#define BUILD_NUMBER" "%VERSION_FILE%"') do set "BUILD_NUMBER=%%i"

if not defined VERSION_MAJOR set "VERSION_MAJOR=1"
if not defined VERSION_MINOR set "VERSION_MINOR=0"
if not defined VERSION_PATCH set "VERSION_PATCH=0"
if not defined BUILD_NUMBER set "BUILD_NUMBER=0"

REM Increment build number
set /a "BUILD_NUMBER=%BUILD_NUMBER%+1"

call :log_message "INFO" "Version updated to %VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%.%BUILD_NUMBER%"

REM Update version file
(
    echo #pragma once
    echo #define VERSION_MAJOR %VERSION_MAJOR%
    echo #define VERSION_MINOR %VERSION_MINOR%
    echo #define VERSION_PATCH %VERSION_PATCH%
    echo #define BUILD_NUMBER %BUILD_NUMBER%
    echo #define VERSION_STRING "%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%.%BUILD_NUMBER%"
) > "%VERSION_FILE%.new"

move /y "%VERSION_FILE%.new" "%VERSION_FILE%" >nul

echo Current version: %VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%.%BUILD_NUMBER% >> "%SUMMARY_FILE%"
echo. >> "%SUMMARY_FILE%"
goto :eof

:run_build
set "CONFIG=%~1"
set "PLATFORM=%~2"

call :log_message "INFO" "Building %CONFIG%|%PLATFORM% configuration..."
echo Building %CONFIG%|%PLATFORM% configuration... >> "%SUMMARY_FILE%"

set BUILD_COMMAND=%BUILD_SCRIPT% --config %CONFIG% --platform %PLATFORM%
if "%SKIP_TESTS%"=="1" set "BUILD_COMMAND=%BUILD_COMMAND% --skip-tests"

call :log_message "DEBUG" "Running command: %BUILD_COMMAND%"
call %BUILD_COMMAND%

if %ERRORLEVEL% NEQ 0 (
    call :log_message "ERROR" "Build failed for %CONFIG%|%PLATFORM%"
    echo   - FAILED >> "%SUMMARY_FILE%"
    set "BUILD_STATUS=FAILED"
) else (
    call :log_message "INFO" "Build successful for %CONFIG%|%PLATFORM%"
    echo   - SUCCESS >> "%SUMMARY_FILE%"
)

goto :eof

:send_email_report
if "%EMAIL_REPORT%"=="0" goto :eof
if "%EMAIL_RECIPIENT%"=="" goto :eof

call :log_message "INFO" "Sending email report to %EMAIL_RECIPIENT%..."

set "EMAIL_SUBJECT=%PROJECT_NAME% Build Report - %BUILD_STATUS%"
set "EMAIL_BODY=Build completed with status: %BUILD_STATUS%. See attached report for details."
set "EMAIL_COMMAND=powershell -Command "^
"$EmailFrom = 'ci-build@localhost';^
$EmailTo = '%EMAIL_RECIPIENT%';^
$Subject = '%EMAIL_SUBJECT%';^
$Body = '%EMAIL_BODY%';^
$SMTPServer = 'localhost';^
$SMTPMessage = New-Object System.Net.Mail.MailMessage($EmailFrom,$EmailTo,$Subject,$Body);^
$Attachment = New-Object System.Net.Mail.Attachment('%SUMMARY_FILE%');^
$SMTPMessage.Attachments.Add($Attachment);^
$SMTPClient = New-Object Net.Mail.SmtpClient($SmtpServer, 25);^
$SMTPClient.Send($SMTPMessage);^
$Attachment.Dispose();""

call :log_message "DEBUG" "Email command: %EMAIL_COMMAND%"

%EMAIL_COMMAND% 2>nul

if %ERRORLEVEL% NEQ 0 (
    call :log_message "WARNING" "Failed to send email report"
) else (
    call :log_message "INFO" "Email report sent successfully"
)

goto :eof

REM ========== MAIN PROCESS ==========
echo %PROJECT_NAME% CI Build
echo ======================================================

REM Set default build status
set "BUILD_STATUS=SUCCESS"

REM Ensure build directory exists
if not exist "%~dp0\..\build" mkdir "%~dp0\..\build"

REM Increment version if requested
if "%FORCE_INCREMENT%"=="1" call :increment_version

REM Generate changelog if enabled
if "%CHANGELOG_ENABLED%"=="1" call :generate_changelog

REM Phase 2 application core validation
call :log_message "INFO" "Validating Phase 2 application core requirements..."
echo Phase 2 Application Core Status: >> "%SUMMARY_FILE%"
if exist "%SRC_DIR%\app\rainmgrapp.cpp" (
    echo   - RAINMGRApp singleton: IMPLEMENTED >> "%SUMMARY_FILE%"
) else (
    echo   - RAINMGRApp singleton: NOT IMPLEMENTED (Phase 2 target) >> "%SUMMARY_FILE%"
)
if exist "%SRC_DIR%\core\service_locator.cpp" (
    echo   - Service Locator: IMPLEMENTED >> "%SUMMARY_FILE%"
) else (
    echo   - Service Locator: NOT IMPLEMENTED (Phase 2 target) >> "%SUMMARY_FILE%"
)
if exist "%SRC_DIR%\config\config_manager.cpp" (
    echo   - Configuration Manager: IMPLEMENTED >> "%SUMMARY_FILE%"
) else (
    echo   - Configuration Manager: NOT IMPLEMENTED (Phase 2 target) >> "%SUMMARY_FILE%"
)
echo. >> "%SUMMARY_FILE%"

REM Run builds for different configurations
echo Build Configurations: >> "%SUMMARY_FILE%"
echo Version: %VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%.%BUILD_NUMBER% >> "%SUMMARY_FILE%"
echo. >> "%SUMMARY_FILE%"
call :run_build "Release" "x64"
call :run_build "Debug" "x64"

REM If any build failed, set status to FAILED
if "%BUILD_STATUS%"=="FAILED" (
    call :log_message "ERROR" "One or more builds failed"
) else (
    call :log_message "INFO" "All builds completed successfully"
)

REM Generate final report
echo. >> "%SUMMARY_FILE%"
echo Build completed: %date% %time% >> "%SUMMARY_FILE%"
echo Final status: %BUILD_STATUS% >> "%SUMMARY_FILE%"

REM Output summary to console
echo.
echo ======================================================
echo Build Summary:
echo ======================================================
type "%SUMMARY_FILE%"
echo ======================================================

REM Archive build artifacts with version information in metadata
set "ARCHIVE_DIR=%~dp0\..\build\archive"
if not exist "%ARCHIVE_DIR%" mkdir "%ARCHIVE_DIR%"

REM Create version info file for this build
echo Build information > "%ARCHIVE_DIR%\build_info_%TIMESTAMP%.txt"
echo ================================= >> "%ARCHIVE_DIR%\build_info_%TIMESTAMP%.txt"
echo Version: %VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%.%BUILD_NUMBER% >> "%ARCHIVE_DIR%\build_info_%TIMESTAMP%.txt"
echo Build time: %date% %time% >> "%ARCHIVE_DIR%\build_info_%TIMESTAMP%.txt"
echo Status: %BUILD_STATUS% >> "%ARCHIVE_DIR%\build_info_%TIMESTAMP%.txt"
echo. >> "%ARCHIVE_DIR%\build_info_%TIMESTAMP%.txt"
echo NOMENCLATURE NOTES: >> "%ARCHIVE_DIR%\build_info_%TIMESTAMP%.txt"
echo - Binary names remain consistent (%PROJECT_NAME%.exe) >> "%ARCHIVE_DIR%\build_info_%TIMESTAMP%.txt"
echo - Version tracked in this metadata file and in version_history.csv >> "%ARCHIVE_DIR%\build_info_%TIMESTAMP%.txt"
echo - Each build has a unique timestamp: %TIMESTAMP% >> "%ARCHIVE_DIR%\build_info_%TIMESTAMP%.txt"
echo. >> "%ARCHIVE_DIR%\build_info_%TIMESTAMP%.txt"

REM Copy changelog to archive
if exist "%CHANGELOG_FILE%" (
    echo Changelog available: See changelog_%TIMESTAMP%.txt >> "%ARCHIVE_DIR%\build_info_%TIMESTAMP%.txt"
    xcopy /y "%CHANGELOG_FILE%" "%ARCHIVE_DIR%\changelog_%TIMESTAMP%.txt" >nul
    call :log_message "INFO" "Copied changelog to archive"
)

REM Copy build artifacts without version in filenames
if exist "%~dp0\..\build\bin\x64\Release\%PROJECT_NAME%.exe" (
    echo Archiving release build artifacts...
    xcopy /y "%~dp0\..\build\bin\x64\Release\%PROJECT_NAME%.exe" "%ARCHIVE_DIR%\" >nul
    call :log_message "INFO" "Archived release build to %ARCHIVE_DIR%"
)

if exist "%~dp0\..\build\%PROJECT_NAME%-Setup.exe" (
    echo Archiving installer...
    xcopy /y "%~dp0\..\build\%PROJECT_NAME%-Setup.exe" "%ARCHIVE_DIR%\" >nul
    call :log_message "INFO" "Archived installer to %ARCHIVE_DIR%"
)

REM Record build in version log
set "VERSION_LOG=%~dp0\..\build\version_history.csv"
if not exist "%VERSION_LOG%" (
    echo Timestamp,Version,Status,Changelog > "%VERSION_LOG%"
)

REM Include changelog path in the version history
if exist "%CHANGELOG_FILE%" (
    echo %TIMESTAMP%,%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%.%BUILD_NUMBER%,%BUILD_STATUS%,changelog_%TIMESTAMP%.txt >> "%VERSION_LOG%"
) else (
    echo %TIMESTAMP%,%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%.%BUILD_NUMBER%,%BUILD_STATUS%,none >> "%VERSION_LOG%"
)
call :log_message "INFO" "Updated version history log"

REM Send email report if configured
call :send_email_report

REM Exit with appropriate code
if "%BUILD_STATUS%"=="FAILED" (
    exit /b 1
) else (
    exit /b 0
)
