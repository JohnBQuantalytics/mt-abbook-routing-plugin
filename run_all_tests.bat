@echo off
echo ================================================================
echo ABBook Plugin - Complete Test Suite
echo ML Scoring Service: 188.245.254.12:50051
echo ================================================================
echo.

:: Set error handling
setlocal enabledelayedexpansion

:: Create test results directory
if not exist "test_results" mkdir test_results
set TIMESTAMP=%date:~-4,4%%date:~-10,2%%date:~-7,2%_%time:~0,2%%time:~3,2%%time:~6,2%
set TIMESTAMP=%TIMESTAMP: =0%
set RESULTS_DIR=test_results\%TIMESTAMP%
mkdir "%RESULTS_DIR%"

echo Test Results Directory: %RESULTS_DIR%
echo.

:: Step 1: Build everything
echo ==========================================
echo STEP 1: Building Plugin and Test Suite
echo ==========================================
call build_comprehensive_test.bat > "%RESULTS_DIR%\build_output.log" 2>&1

if %errorlevel% neq 0 (
    echo ❌ Build failed! Check build_output.log
    type "%RESULTS_DIR%\build_output.log"
    pause
    exit /b 1
)

echo ✅ Build successful
echo.

:: Step 2: Check if required files exist
echo ==========================================
echo STEP 2: Verifying Required Files
echo ==========================================

set FILES_OK=1

if exist ABBook_Plugin_32bit.dll (
    echo ✅ ABBook_Plugin_32bit.dll found
) else (
    echo ❌ ABBook_Plugin_32bit.dll not found
    set FILES_OK=0
)

if exist test_mt4_plugin_comprehensive.exe (
    echo ✅ test_mt4_plugin_comprehensive.exe found
) else (
    echo ❌ test_mt4_plugin_comprehensive.exe not found
    set FILES_OK=0
)

if exist ABBook_Config.ini (
    echo ✅ ABBook_Config.ini found
) else (
    echo ❌ ABBook_Config.ini not found
    set FILES_OK=0
)

if %FILES_OK% neq 1 (
    echo ❌ Required files missing! Cannot continue.
    pause
    exit /b 1
)

echo.

:: Step 3: Quick network connectivity test
echo ==========================================
echo STEP 3: Testing Network Connectivity
echo ==========================================

echo Testing connectivity to 188.245.254.12:50051...
powershell -Command "try { $tcp = New-Object Net.Sockets.TcpClient; $tcp.Connect('188.245.254.12', 50051); $tcp.Close(); Write-Host '✅ Network connectivity OK' } catch { Write-Host '❌ Network connectivity failed' }"
echo.

:: Step 4: Run comprehensive test
echo ==========================================
echo STEP 4: Running Comprehensive Test Suite
echo ==========================================

echo Starting comprehensive test...
echo Output will be logged to: %RESULTS_DIR%\test_output.log
echo.

test_mt4_plugin_comprehensive.exe > "%RESULTS_DIR%\test_output.log" 2>&1

if %errorlevel% neq 0 (
    echo ❌ Test execution failed!
    echo Last 20 lines of output:
    powershell -Command "Get-Content '%RESULTS_DIR%\test_output.log' | Select-Object -Last 20"
) else (
    echo ✅ Test execution completed
    echo.
    echo Test Summary:
    powershell -Command "Get-Content '%RESULTS_DIR%\test_output.log' | Select-String -Pattern '(SUCCESS|ERROR|FAILED)' | Select-Object -Last 10"
)

echo.

:: Step 5: Collect log files
echo ==========================================
echo STEP 5: Collecting Log Files
echo ==========================================

echo Copying log files to results directory...

if exist ABBook_Plugin_Debug.log (
    copy ABBook_Plugin_Debug.log "%RESULTS_DIR%\ABBook_Plugin_Debug.log" > nul
    echo ✅ ABBook_Plugin_Debug.log collected
) else (
    echo ❌ ABBook_Plugin_Debug.log not found
)

if exist ABBook_Plugin.log (
    copy ABBook_Plugin.log "%RESULTS_DIR%\ABBook_Plugin.log" > nul
    echo ✅ ABBook_Plugin.log collected
) else (
    echo ❌ ABBook_Plugin.log not found
)

for %%f in (ABBook_Test_*.log) do (
    if exist "%%f" (
        copy "%%f" "%RESULTS_DIR%\%%f" > nul
        echo ✅ %%f collected
    )
)

echo.

:: Step 6: Generate test report
echo ==========================================
echo STEP 6: Generating Test Report
echo ==========================================

echo Generating test report...
(
    echo ABBook Plugin Test Report
    echo Generated: %date% %time%
    echo ========================================
    echo.
    echo Test Configuration:
    echo - ML Service: 188.245.254.12:50051
    echo - Plugin: ABBook_Plugin_32bit.dll
    echo - Test Suite: test_mt4_plugin_comprehensive.exe
    echo.
    echo Test Results:
    echo.
    if exist "%RESULTS_DIR%\test_output.log" (
        powershell -Command "Get-Content '%RESULTS_DIR%\test_output.log' | Select-String -Pattern '(SUCCESS|ERROR|FAILED|INFO)' | ForEach-Object { $_.Line }"
    )
    echo.
    echo Log Files:
    dir "%RESULTS_DIR%\*.log" /B
    echo.
    echo Test completed at: %date% %time%
) > "%RESULTS_DIR%\test_report.txt"

echo ✅ Test report generated: %RESULTS_DIR%\test_report.txt
echo.

:: Step 7: Display summary
echo ==========================================
echo STEP 7: Test Summary
echo ==========================================

echo.
echo Results saved to: %RESULTS_DIR%
echo.
echo Key files to check:
echo - test_report.txt          : Complete test report
echo - test_output.log          : Full test execution log
echo - ABBook_Plugin_Debug.log  : Plugin debug information
echo - ABBook_Plugin.log        : Trade routing decisions
echo - build_output.log         : Build process log
echo.

:: Check for key indicators
set SUCCESS_COUNT=0
set ERROR_COUNT=0

if exist "%RESULTS_DIR%\test_output.log" (
    for /f %%i in ('powershell -Command "Get-Content '%RESULTS_DIR%\test_output.log' | Select-String -Pattern 'SUCCESS' | Measure-Object | Select-Object -ExpandProperty Count"') do set SUCCESS_COUNT=%%i
    for /f %%i in ('powershell -Command "Get-Content '%RESULTS_DIR%\test_output.log' | Select-String -Pattern 'ERROR' | Measure-Object | Select-Object -ExpandProperty Count"') do set ERROR_COUNT=%%i
)

echo Test Statistics:
echo - Success operations: %SUCCESS_COUNT%
echo - Error operations: %ERROR_COUNT%
echo.

if %ERROR_COUNT% gtr 0 (
    echo ⚠️  Some errors were detected. Check the logs for details.
    echo.
    echo Recent errors:
    if exist "%RESULTS_DIR%\test_output.log" (
        powershell -Command "Get-Content '%RESULTS_DIR%\test_output.log' | Select-String -Pattern 'ERROR' | Select-Object -Last 3"
    )
) else (
    echo ✅ All tests completed successfully!
)

echo.
echo ================================================================
echo Test suite completed. Press any key to exit...
echo ================================================================
pause 