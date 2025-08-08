@echo off
setlocal enabledelayedexpansion

REM ========================================================================
REM Enterprise Setup and Run Script for RainmeterUI Complete Scraper
REM
REM Features:
REM - Robust error handling and logging
REM - Virtual environment management
REM - Configuration validation
REM - Command-line options support
REM - Performance monitoring
REM - Resumable operations
REM ========================================================================

REM ========== CONFIGURATION ==========
set "SCRAPER_VERSION=1.0.0"
set "LOG_DIR=logs"
set "LOG_FILE=scraper_%date:~-4,4%%date:~-7,2%%date:~-10,2%_%time:~0,2%%time:~3,2%%time:~6,2%.log"
set "LOG_FILE=%LOG_FILE: =0%"
set "VENV_DIR=venv"
set "CONFIG_FILE=..\rainmeterui_categories.json"
set "DEFAULT_OUTPUT_DIR=scraped_data"
set "DEFAULT_DELAY=1.0"
set "DEFAULT_WORKERS=3"
set "DEFAULT_TIMEOUT=30"
set "DEFAULT_RETRIES=3"
set "RESUME=0"
set "VERBOSE=0"

REM ========== PARSE COMMAND LINE ARGUMENTS ==========
set "OUTPUT_DIR=%DEFAULT_OUTPUT_DIR%"
set "DELAY=%DEFAULT_DELAY%"
set "WORKERS=%DEFAULT_WORKERS%"
set "TIMEOUT=%DEFAULT_TIMEOUT%"
set "RETRIES=%DEFAULT_RETRIES%"
set "BATCH_MODE=0"

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
if /i "%~1"=="--resume" set "RESUME=1" & shift & goto :parse_args
if /i "%~1"=="-v" set "VERBOSE=1" & shift & goto :parse_args
if /i "%~1"=="--verbose" set "VERBOSE=1" & shift & goto :parse_args
if /i "%~1"=="--batch" set "BATCH_MODE=1" & shift & goto :parse_args
if /i "%~1"=="--version" (
    echo RainmeterUI Complete Scraper v%SCRAPER_VERSION%
    exit /b 0
)
echo Unknown option: %~1
goto :show_help

:show_help
echo.
echo RainmeterUI Complete Scraper v%SCRAPER_VERSION%
echo Usage: %~nx0 [options]
echo.
echo Options:
echo   -h, --help                 Show this help message
echo   -o, --output DIR           Output directory (default: %DEFAULT_OUTPUT_DIR%)
echo   -d, --delay SECONDS        Delay between requests in seconds (default: %DEFAULT_DELAY%)
echo   -w, --workers NUMBER       Number of parallel download workers (default: %DEFAULT_WORKERS%)
echo   -t, --timeout SECONDS      Request timeout in seconds (default: %DEFAULT_TIMEOUT%)
echo   -r, --retries NUMBER       Number of retries for failed requests (default: %DEFAULT_RETRIES%)
echo   --resume                   Resume previously interrupted scraping
echo   -v, --verbose              Enable verbose output
echo   --batch                    Run in batch mode (no user prompts)
echo   --version                  Show version information
echo.
exit /b 0

:args_done

REM ========== SETUP LOGGING ==========
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"
set "LOG_PATH=%LOG_DIR%\%LOG_FILE%"

REM Start timer
set "SCRAPE_START_TIME=%time%"

echo [%date% %time%] [INFO] Scraper setup started > "%LOG_PATH%"
echo [%date% %time%] [INFO] Version: %SCRAPER_VERSION% >> "%LOG_PATH%"

echo RainmeterUI Complete Scraper - Enterprise Setup and Run v%SCRAPER_VERSION%
echo ================================================================

REM ========== VERIFY ENVIRONMENT ==========
echo Verifying environment...
echo [%date% %time%] [INFO] Verifying environment... >> "%LOG_PATH%"

REM Check if Python is installed
python --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [%date% %time%] [ERROR] Python is not installed or not in PATH. >> "%LOG_PATH%"
    echo ERROR: Python is not installed or not in PATH.
    echo Please install Python 3.7+ from https://www.python.org/downloads/
    echo Make sure to check "Add Python to PATH" during installation.
    echo.
    if "%BATCH_MODE%"=="0" pause
    exit /b 1
)

REM Check Python version
for /f "tokens=2" %%V in ('python --version 2^>^&1') do set "PY_VERSION=%%V"
echo [%date% %time%] [INFO] Python version: %PY_VERSION% >> "%LOG_PATH%"

echo Python found: %PY_VERSION%

REM Check if pip is available
pip --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [%date% %time%] [ERROR] pip is not available. >> "%LOG_PATH%"
    echo ERROR: pip is not available.
    echo Please ensure pip is installed with Python.
    echo.
    if "%BATCH_MODE%"=="0" pause
    exit /b 1
)

REM Get pip version
for /f "tokens=2" %%V in ('pip --version') do set "PIP_VERSION=%%V"
echo [%date% %time%] [INFO] pip version: %PIP_VERSION% >> "%LOG_PATH%"

echo pip found: v%PIP_VERSION%

REM ========== SETUP VIRTUAL ENVIRONMENT ==========
echo Setting up Python virtual environment...
echo [%date% %time%] [INFO] Setting up Python virtual environment... >> "%LOG_PATH%"

if not exist "%VENV_DIR%" (
    echo Creating Python virtual environment...
    echo [%date% %time%] [INFO] Creating Python virtual environment... >> "%LOG_PATH%"
    
    python -m venv "%VENV_DIR%"
    if %ERRORLEVEL% NEQ 0 (
        echo [%date% %time%] [ERROR] Failed to create virtual environment. >> "%LOG_PATH%"
        echo ERROR: Failed to create virtual environment.
        if "%BATCH_MODE%"=="0" pause
        exit /b 1
    )
    echo Virtual environment created successfully.
    echo [%date% %time%] [INFO] Virtual environment created successfully. >> "%LOG_PATH%"
)

REM Activate virtual environment
echo Activating virtual environment...
echo [%date% %time%] [INFO] Activating virtual environment... >> "%LOG_PATH%"

call "%VENV_DIR%\Scripts\activate.bat"
if %ERRORLEVEL% NEQ 0 (
    echo [%date% %time%] [ERROR] Failed to activate virtual environment. >> "%LOG_PATH%"
    echo ERROR: Failed to activate virtual environment.
    if "%BATCH_MODE%"=="0" pause
    exit /b 1
)

echo Virtual environment activated.
echo [%date% %time%] [INFO] Virtual environment activated. >> "%LOG_PATH%"

REM ========== INSTALL DEPENDENCIES ==========
echo Installing required dependencies...
echo [%date% %time%] [INFO] Installing required dependencies... >> "%LOG_PATH%"

python -m pip install --upgrade pip
if %ERRORLEVEL% NEQ 0 (
    echo [%date% %time%] [ERROR] Failed to upgrade pip. >> "%LOG_PATH%"
    echo ERROR: Failed to upgrade pip.
    if "%BATCH_MODE%"=="0" pause
    exit /b 1
)

REM Check if requirements.txt exists
if not exist "requirements.txt" (
    echo [%date% %time%] [ERROR] requirements.txt not found. >> "%LOG_PATH%"
    echo ERROR: requirements.txt not found.
    echo Please ensure the requirements file is in the current directory.
    if "%BATCH_MODE%"=="0" pause
    exit /b 1
)

REM Install dependencies
echo [%date% %time%] [INFO] Installing packages from requirements.txt... >> "%LOG_PATH%"
python -m pip install -r requirements.txt
if %ERRORLEVEL% NEQ 0 (
    echo [%date% %time%] [ERROR] Failed to install dependencies. >> "%LOG_PATH%"
    echo ERROR: Failed to install dependencies.
    echo Please check your internet connection and try again.
    if "%BATCH_MODE%"=="0" pause
    exit /b 1
)

echo Dependencies installed successfully.
echo [%date% %time%] [INFO] Dependencies installed successfully. >> "%LOG_PATH%"

REM ========== CHECK CONFIGURATION ==========
echo Checking configuration...
echo [%date% %time%] [INFO] Checking configuration... >> "%LOG_PATH%"

REM Check if configuration file exists
if not exist "%CONFIG_FILE%" (
    echo [%date% %time%] [ERROR] Configuration file '%CONFIG_FILE%' not found. >> "%LOG_PATH%"
    echo ERROR: Configuration file '%CONFIG_FILE%' not found.
    echo Please ensure the file is in the expected location.
    echo Expected location: %CONFIG_FILE%
    echo.
    if "%BATCH_MODE%"=="0" pause
    exit /b 1
)

REM Validate configuration file
echo Validating configuration file...
echo [%date% %time%] [INFO] Validating configuration file... >> "%LOG_PATH%"

python -c "import json; json.load(open('%CONFIG_FILE%'))" 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [%date% %time%] [ERROR] Invalid configuration file. JSON syntax error. >> "%LOG_PATH%"
    echo ERROR: Invalid configuration file. JSON syntax error.
    if "%BATCH_MODE%"=="0" pause
    exit /b 1
)

echo Configuration file found and validated.
echo [%date% %time%] [INFO] Configuration file found and validated. >> "%LOG_PATH%"

REM ========== CREATE OUTPUT DIRECTORY ==========
echo [%date% %time%] [INFO] Setting up output directory: %OUTPUT_DIR% >> "%LOG_PATH%"

if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
    echo Created output directory: %OUTPUT_DIR%
    echo [%date% %time%] [INFO] Created output directory: %OUTPUT_DIR% >> "%LOG_PATH%"
) else (
    echo Output directory already exists: %OUTPUT_DIR%
    echo [%date% %time%] [INFO] Output directory already exists: %OUTPUT_DIR% >> "%LOG_PATH%"
    
    REM Check for resume data
    if exist "%OUTPUT_DIR%\scraping_progress.json" (
        if "%RESUME%"=="1" (
            echo Resume file found. Will resume previous scraping session.
            echo [%date% %time%] [INFO] Resume file found. Will resume previous scraping session. >> "%LOG_PATH%"
        ) else (
            echo WARNING: Output directory contains previous scraping data.
            echo [%date% %time%] [WARNING] Output directory contains previous scraping data. >> "%LOG_PATH%"
            echo Consider using --resume to continue previous session.
        )
    )
)

REM ========== ASK FOR CONFIRMATION ==========
if "%BATCH_MODE%"=="0" (
    echo ================================================================
    echo SCRAPER CONFIGURATION
    echo ================================================================
    echo.
    
    if not defined DELAY (
        set /p DELAY="Enter delay between requests in seconds (default: %DEFAULT_DELAY%): "
        if "!DELAY!"=="" set "DELAY=%DEFAULT_DELAY%"
    )
    
    if not defined WORKERS (
        set /p WORKERS="Enter number of parallel download workers (default: %DEFAULT_WORKERS%): "
        if "!WORKERS!"=="" set "WORKERS=%DEFAULT_WORKERS%"
    )
    
    if not defined OUTPUT_DIR (
        set /p OUTPUT_DIR="Enter output directory (default: %DEFAULT_OUTPUT_DIR%): "
        if "!OUTPUT_DIR!"=="" set "OUTPUT_DIR=%DEFAULT_OUTPUT_DIR%"
    )
    
    echo.
    echo Configuration:
    echo - Output directory: %OUTPUT_DIR%
    echo - Request delay: %DELAY% seconds
    echo - Download workers: %WORKERS%
    echo - Request timeout: %TIMEOUT% seconds
    echo - Request retries: %RETRIES%
    if "%RESUME%"=="1" echo - Resuming previous session
    if "%VERBOSE%"=="1" echo - Verbose mode enabled
    echo.
    
    set /p CONFIRM="Start scraping with these settings? (y/n): "
    if /i not "%CONFIRM%"=="y" (
        echo [%date% %time%] [INFO] Scraping cancelled by user. >> "%LOG_PATH%"
        echo Scraping cancelled by user.
        if "%BATCH_MODE%"=="0" pause
        exit /b 0
    )
)

REM ========== START SCRAPING ==========
echo [%date% %time%] [INFO] Starting scraper... >> "%LOG_PATH%"

echo.
echo ================================================================
echo STARTING SCRAPER
echo ================================================================
echo.

REM Build command line arguments
set "SCRAPER_ARGS=--config "%CONFIG_FILE%" --output "%OUTPUT_DIR%" --delay %DELAY% --workers %WORKERS% --timeout %TIMEOUT% --retries %RETRIES%"

if "%VERBOSE%"=="1" set "SCRAPER_ARGS=%SCRAPER_ARGS% --verbose"
if "%RESUME%"=="1" set "SCRAPER_ARGS=%SCRAPER_ARGS% --resume"

REM Run the scraper
echo [%date% %time%] [INFO] Executing: python rainmeter_complete_scraper.py %SCRAPER_ARGS% >> "%LOG_PATH%"
python rainmeter_complete_scraper.py %SCRAPER_ARGS%

REM Check result
set "SCRAPER_EXIT_CODE=%ERRORLEVEL%"
echo [%date% %time%] [INFO] Scraper finished with exit code: %SCRAPER_EXIT_CODE% >> "%LOG_PATH%"

REM ========== CALCULATE EXECUTION TIME ==========
set "SCRAPE_END_TIME=%time%"

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

REM ========== FINALIZE ==========
if %SCRAPER_EXIT_CODE% EQU 0 (
    echo [%date% %time%] [SUCCESS] Scraping completed successfully in %DURATION%! >> "%LOG_PATH%"
    
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
    echo - %LOG_PATH% (setup log)
    echo - %OUTPUT_DIR%/complete_scraper.log (detailed scraper log)
    echo - %OUTPUT_DIR%/scraping_summary.txt (summary report)
    echo.
) else (
    echo [%date% %time%] [ERROR] Scraping failed or was interrupted (Exit code: %SCRAPER_EXIT_CODE%) >> "%LOG_PATH%"
    
    echo.
    echo ================================================================
    echo SCRAPING FAILED OR WAS INTERRUPTED
    echo ================================================================
    echo.
    echo Check the log files for error details:
    echo - %LOG_PATH% (setup log)
    echo - %OUTPUT_DIR%/complete_scraper.log (scraper log)
    echo - %OUTPUT_DIR%/scraping_progress.json (resume data)
    echo.
    echo You can resume scraping by running this script with the --resume flag:
    echo   %~nx0 --resume
    echo.
)

REM Deactivate virtual environment
echo [%date% %time%] [INFO] Deactivating virtual environment... >> "%LOG_PATH%"
call "%VENV_DIR%\Scripts\deactivate.bat"

echo [%date% %time%] [INFO] Script completed. >> "%LOG_PATH%"

if "%BATCH_MODE%"=="0" (
    echo Press any key to exit...
    pause >nul
)

exit /b %SCRAPER_EXIT_CODE%
