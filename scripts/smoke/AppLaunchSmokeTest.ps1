Param(
    [string]$ExePath = "..\out\x64\Release\RainmeterManager.exe",
    [int]$TimeoutSec = 30
)

Write-Host "RainmeterManager Smoke Test - App Launch and Window Creation" -ForegroundColor Cyan
Write-Host "=============================================================" -ForegroundColor Cyan
Write-Host ""

# Check if exe exists
if (-not (Test-Path $ExePath)) {
    Write-Host "ERROR: Executable not found at: $ExePath" -ForegroundColor Red
    exit 1
}

Write-Host "Starting application: $ExePath" -ForegroundColor Yellow

try {
    # Start the process
    $proc = Start-Process -FilePath $ExePath -PassThru -ErrorAction Stop
    
    Write-Host "Process started with PID: $($proc.Id)" -ForegroundColor Green
    
    # Wait for window creation
    $sw = [Diagnostics.Stopwatch]::StartNew()
    $windowFound = $false
    
    Write-Host "Waiting for main window creation (timeout: $TimeoutSec seconds)..." -ForegroundColor Yellow
    
    while ($sw.Elapsed.TotalSeconds -lt $TimeoutSec) {
        # Refresh process info
        try {
            $proc.Refresh()
            
            if ($proc.HasExited) {
                Write-Host "ERROR: Process exited prematurely with exit code: $($proc.ExitCode)" -ForegroundColor Red
                exit 1
            }
            
            # Check for main window handle
            if ($proc.MainWindowHandle -ne 0) {
                $windowFound = $true
                Write-Host "SUCCESS: Main window created!" -ForegroundColor Green
                Write-Host "  Window Handle: 0x$([Convert]::ToString($proc.MainWindowHandle, 16))" -ForegroundColor Gray
                Write-Host "  Window Title: $($proc.MainWindowTitle)" -ForegroundColor Gray
                break
            }
        } catch {
            Write-Host "ERROR: Failed to query process: $_" -ForegroundColor Red
            exit 1
        }
        
        Start-Sleep -Milliseconds 200
    }
    
    if (-not $windowFound) {
        Write-Host "ERROR: Main window not created within $TimeoutSec seconds" -ForegroundColor Red
        if (-not $proc.HasExited) {
            $proc.Kill()
        }
        exit 1
    }
    
    # Give it a moment to stabilize
    Start-Sleep -Milliseconds 500
    
    # Verify process is still running
    $proc.Refresh()
    if ($proc.HasExited) {
        Write-Host "ERROR: Process exited after window creation with exit code: $($proc.ExitCode)" -ForegroundColor Red
        exit 1
    }
    
    Write-Host ""
    Write-Host "All smoke tests PASSED!" -ForegroundColor Green
    Write-Host "  - Process started successfully" -ForegroundColor Gray
    Write-Host "  - Main window created" -ForegroundColor Gray
    Write-Host "  - Process remains stable" -ForegroundColor Gray
    
    # Clean shutdown
    Write-Host ""
    Write-Host "Closing application..." -ForegroundColor Yellow
    
    if (-not $proc.HasExited) {
        $proc.CloseMainWindow() | Out-Null
        $shutdownWait = 5
        $closed = $proc.WaitForExit($shutdownWait * 1000)
        
        if (-not $closed) {
            Write-Host "WARNING: Graceful shutdown timeout, forcing termination" -ForegroundColor Yellow
            $proc.Kill()
            $proc.WaitForExit(2000) | Out-Null
        } else {
            Write-Host "Application closed gracefully" -ForegroundColor Green
        }
    }
    
    exit 0
    
} catch {
    Write-Host "FATAL ERROR: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host $_.ScriptStackTrace -ForegroundColor DarkRed
    exit 1
} finally {
    # Ensure cleanup
    if ($proc -and -not $proc.HasExited) {
        try {
            $proc.Kill() | Out-Null
        } catch {
            # Ignore cleanup errors
        }
    }
}
