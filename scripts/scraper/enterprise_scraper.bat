@echo off
setlocal enabledelayedexpansion

REM ========================================================================
REM Enterprise Scraper for Rainmeter Manager
REM 
REM Features:
REM - Robust error handling and logging
REM - Virtual environment management
REM - Configuration validation
REM - Resumable operations
REM - Performance monitoring
REM - Secure credential handling
REM ========================================================================

REM ========== CONFIGURATION ==========
set "SCRAPER_VERSION=1.0.0"
set "LOG_DIR=logs"
set "LOG_FILE=scraper_%date:~-4,4%%date:~-7,2%%date:~-10,2%_%time:~0,2%%time:~3,2%%time:~6,2%.log"
set "LOG_FILE=%LOG_FILE: =0%"
set "VENV_DIR=venv"
set "CONFIG_FILE=config.json"
set "DEFAULT_OUTPUT_DIR=scraped_data"
set "DEFAULT_DELAY=1.0"
set "DEFAULT_WORKERS=3"
set "DEFAULT_TIMEOUT=30"
set "DEFAULT_RETRIES=3"
set "DEFAULT_USER_AGENT=RainmeterManager/1.0 Scraper"
set "RESUME_FILE=scraping_progress.json"
set "SUMMARY_FILE=scraping_summary.txt"

REM ========== PARSE COMMAND LINE ARGUMENTS ==========
set "OUTPUT_DIR=%DEFAULT_OUTPUT_DIR%"
set "DELAY=%DEFAULT_DELAY%"
set "WORKERS=%DEFAULT_WORKERS%"
set "TIMEOUT=%DEFAULT_TIMEOUT%"
set "RETRIES=%DEFAULT_RETRIES%"
set "USER_AGENT=%DEFAULT_USER_AGENT%"
set "RESUME=0"
set "VERBOSE=0"
set "FORCE=0"

:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="-h" goto :show_help
if /i "%~1"=="--help" goto :show_help
if /i "%~1"=="-o" set "OUTPUT_DIR=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="--output" set "OUTPUT_DIR=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="-d" set "DELAY=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="--delay" set "DELAY=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="-w" set "WORKERS=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="--workers" set "WORKERS=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="-t" set "TIMEOUT=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="--timeout" set "TIMEOUT=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="-r" set "RETRIES=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="--retries" set "RETRIES=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="--user-agent" set "USER_AGENT=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="--resume" set "RESUME=1" & shift & goto :parse_args
if /i "%~1"=="-v" set "VERBOSE=1" & shift & goto :parse_args
if /i "%~1"=="--verbose" set "VERBOSE=1" & shift & goto :parse_args
if /i "%~1"=="-f" set "FORCE=1" & shift & goto :parse_args
if /i "%~1"=="--force" set "FORCE=1" & shift & goto :parse_args
if /i "%~1"=="--version" (
    echo Rainmeter Manager Scraper v%SCRAPER_VERSION%
    exit /b 0
)
echo Unknown option: %~1
goto :show_help

:show_help
echo.
echo Rainmeter Manager Scraper v%SCRAPER_VERSION%
echo Usage: %~nx0 [options]
echo.
echo Options:
echo   -h, --help                 Show this help message
echo   -o, --output DIR           Output directory (default: %DEFAULT_OUTPUT_DIR%)
echo   -d, --delay SECONDS        Delay between requests in seconds (default: %DEFAULT_DELAY%)
echo   -w, --workers NUMBER       Number of parallel download workers (default: %DEFAULT_WORKERS%)
echo   -t, --timeout SECONDS      Request timeout in seconds (default: %DEFAULT_TIMEOUT%)
echo   -r, --retries NUMBER       Number of retries for failed requests (default: %DEFAULT_RETRIES%)
echo   --user-agent AGENT         Custom user agent string
echo   --resume                   Resume previously interrupted scraping
echo   -v, --verbose              Enable verbose output
echo   -f, --force                Force overwrite of existing data
echo   --version                  Show version information
echo.
exit /b 0

:args_done

REM ========== SETUP LOGGING ==========
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"
set "LOG_PATH=%LOG_DIR%\%LOG_FILE%"

REM Function to log messages
call :log_message "INFO" "Scraper started"
call :log_message "INFO" "Version: %SCRAPER_VERSION%"
call :log_message "INFO" "Output directory: %OUTPUT_DIR%"
call :log_message "INFO" "Workers: %WORKERS%"
call :log_message "INFO" "Delay: %DELAY% seconds"
call :log_message "INFO" "Timeout: %TIMEOUT% seconds"
call :log_message "INFO" "Retries: %RETRIES%"

REM ========== START TIMER ==========
set "SCRAPE_START_TIME=%time%"

REM ========== VERIFY ENVIRONMENT ==========
call :log_message "INFO" "Verifying environment..."

REM Check for Python
python --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    call :log_message "ERROR" "Python is not installed or not in PATH."
    call :log_message "ERROR" "Please install Python 3.7+ from https://www.python.org/downloads/"
    call :log_message "ERROR" "Make sure to check 'Add Python to PATH' during installation."
    exit /b 1
)

REM Check Python version
for /f "tokens=2" %%V in ('python --version 2^>^&1') do set "PY_VERSION=%%V"
call :log_message "INFO" "Python version: %PY_VERSION%"

REM Check if pip is available
pip --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    call :log_message "ERROR" "pip is not available."
    call :log_message "ERROR" "Please ensure pip is installed with Python."
    exit /b 1
)

REM ========== SETUP VIRTUAL ENVIRONMENT ==========
if not exist "%VENV_DIR%" (
    call :log_message "INFO" "Creating Python virtual environment..."
    python -m venv "%VENV_DIR%"
    if %ERRORLEVEL% NEQ 0 (
        call :log_message "ERROR" "Failed to create virtual environment."
        exit /b 1
    )
    call :log_message "INFO" "Virtual environment created successfully."
)

REM Activate virtual environment
call :log_message "INFO" "Activating virtual environment..."
call "%VENV_DIR%\Scripts\activate.bat"
if %ERRORLEVEL% NEQ 0 (
    call :log_message "ERROR" "Failed to activate virtual environment."
    exit /b 1
)

REM ========== INSTALL DEPENDENCIES ==========
call :log_message "INFO" "Installing required dependencies..."
python -m pip install --upgrade pip
if %ERRORLEVEL% NEQ 0 (
    call :log_message "ERROR" "Failed to upgrade pip."
    exit /b 1
)

REM Check if requirements.txt exists
if not exist "requirements.txt" (
    call :log_message "ERROR" "requirements.txt not found."
    call :log_message "ERROR" "Please ensure the requirements file is in the current directory."
    exit /b 1
)

REM Install dependencies
python -m pip install -r requirements.txt
if %ERRORLEVEL% NEQ 0 (
    call :log_message "ERROR" "Failed to install dependencies."
    call :log_message "ERROR" "Please check your internet connection and try again."
    exit /b 1
)

REM ========== CHECK CONFIGURATION ==========
if not exist "%CONFIG_FILE%" (
    call :log_message "ERROR" "Configuration file '%CONFIG_FILE%' not found."
    call :log_message "ERROR" "Please ensure the configuration file is in the current directory."
    exit /b 1
)

REM Validate configuration file
python -c "import json; json.load(open('%CONFIG_FILE%'))"
if %ERRORLEVEL% NEQ 0 (
    call :log_message "ERROR" "Invalid configuration file. JSON syntax error."
    exit /b 1
)

REM ========== CREATE OUTPUT DIRECTORY ==========
if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
    call :log_message "INFO" "Created output directory: %OUTPUT_DIR%"
) else (
    call :log_message "INFO" "Output directory already exists: %OUTPUT_DIR%"
    
    REM Check if force flag is set
    if exist "%OUTPUT_DIR%\%RESUME_FILE%" (
        if "%RESUME%"=="1" (
            call :log_message "INFO" "Resume file found. Will resume previous scraping session."
        ) else (
            if "%FORCE%"=="1" (
                call :log_message "WARNING" "Force flag set. Will overwrite existing data."
            ) else (
                call :log_message "WARNING" "Output directory contains previous scraping data."
                call :log_message "WARNING" "Use --resume to continue previous session or --force to start fresh."
                exit /b 1
            )
        )
    )
)

REM ========== ASK FOR CONFIRMATION ==========
if "%FORCE%"=="0" (
    echo.
    echo ================================================================
    echo SCRAPER CONFIGURATION
    echo ================================================================
    echo.
    echo Configuration:
    echo - Output directory: %OUTPUT_DIR%
    echo - Request delay: %DELAY% seconds
    echo - Download workers: %WORKERS%
    echo - Request timeout: %TIMEOUT% seconds
    echo - Request retries: %RETRIES%
    if "%RESUME%"=="1" echo - Resuming previous session
    echo.
    
    set /p CONFIRM="Start scraping with these settings? (y/n): "
    if /i not "%CONFIRM%"=="y" (
        call :log_message "INFO" "Scraping cancelled by user."
        exit /b 0
    )
)

REM ========== START SCRAPING ==========
call :log_message "INFO" "Starting scraper..."

echo.
echo ================================================================
echo STARTING SCRAPER
echo ================================================================
echo.

REM Build command line arguments
set "SCRAPER_ARGS=--config %CONFIG_FILE% --output %OUTPUT_DIR% --delay %DELAY% --workers %WORKERS% --timeout %TIMEOUT% --retries %RETRIES% --user-agent ^"%USER_AGENT%^""

if "%VERBOSE%"=="1" set "SCRAPER_ARGS=%SCRAPER_ARGS% --verbose"
if "%RESUME%"=="1" set "SCRAPER_ARGS=%SCRAPER_ARGS% --resume"
if "%FORCE%"=="1" set "SCRAPER_ARGS=%SCRAPER_ARGS% --force"

REM Run the scraper
python rainmeter_enterprise_scraper.py %SCRAPER_ARGS%

REM Check result
set "SCRAPER_EXIT_CODE=%ERRORLEVEL%"

REM ========== CALCULATE SCRAPING TIME ==========
set "SCRAPE_END_TIME=%time%"
call :calculate_time

REM ========== FINALIZE ==========
if %SCRAPER_EXIT_CODE% EQU 0 (
    call :log_message "SUCCESS" "Scraping completed successfully in %DURATION%!"
    
    echo.
    echo ================================================================
    echo SCRAPING COMPLETED SUCCESSFULLY IN %DURATION%!
    echo ================================================================
    echo.
    echo Check the '%OUTPUT_DIR%' directory for:
    echo - Downloaded skin packages (downloads/ folder)
    echo - Extracted skins (extracted_skins/ folder)
    echo - Metadata and reports (JSON, CSV, TXT files)
    echo.
    echo Log files:
    echo - %LOG_PATH% (detailed log)
    if exist "%OUTPUT_DIR%\%SUMMARY_FILE%" echo - %OUTPUT_DIR%\%SUMMARY_FILE% (summary report)
    echo.
) else (
    call :log_message "ERROR" "Scraping failed or was interrupted (Exit code: %SCRAPER_EXIT_CODE%)"
    
    echo.
    echo ================================================================
    echo SCRAPING FAILED OR WAS INTERRUPTED
    echo ================================================================
    echo.
    echo Check the log files for error details:
    echo - %LOG_PATH%
    if exist "%OUTPUT_DIR%\%RESUME_FILE%" echo - %OUTPUT_DIR%\%RESUME_FILE% (resume data)
    echo.
    echo You can resume scraping by running this script with the --resume flag.
    echo.
)

REM Deactivate virtual environment
call "%VENV_DIR%\Scripts\deactivate.bat"

exit /b %SCRAPER_EXIT_CODE%

REM ========== HELPER FUNCTIONS ==========

:log_message
set "LEVEL=%~1"
set "MESSAGE=%~2"
set "TIMESTAMP=%date% %time%"
echo [%TIMESTAMP%] [%LEVEL%] %MESSAGE%
echo [%TIMESTAMP%] [%LEVEL%] %MESSAGE% >> "%LOG_PATH%"
exit /b 0

:calculate_time
REM Convert start time to seconds
set /a "START_H=1%SCRAPE_START_TIME:~0,2%-100"
set /a "START_M=1%SCRAPE_START_TIME:~3,2%-100"
set /a "START_S=1%SCRAPE_START_TIME:~6,2%-100"
set /a "START_TOTAL_S=(START_H*3600)+(START_M*60)+START_S"

REM Convert end time to seconds
set /a "END_H=1%SCRAPE_END_TIME:~0,2%-100"
set /a "END_M=1%SCRAPE_END_TIME:~3,2%-100"
set /a "END_S=1%SCRAPE_END_TIME:~6,2%-100"
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
set "DURATION=%DURATION_H%:%DURATION_M%:%DURATION_S%"
exit /b 0

REM End of script
