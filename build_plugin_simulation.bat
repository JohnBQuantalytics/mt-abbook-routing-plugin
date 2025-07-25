@echo off
echo Building Plugin Simulation Test...
echo ===================================

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
)

del test_plugin_simulation.exe 2>nul

echo Compiling test_plugin_simulation.cpp...
cl.exe /EHsc /MT test_plugin_simulation.cpp /Fe:test_plugin_simulation.exe

if errorlevel 1 (
    echo.
    echo *** COMPILATION FAILED ***
    echo Check the error messages above
    pause
    exit /b 1
)

echo.
echo =======================================
echo *** SUCCESS: SIMULATION TEST BUILT ***
echo =======================================
echo Output: test_plugin_simulation.exe
echo.
echo This test will:
echo - Load the plugin DLL dynamically
echo - Initialize the plugin (MtSrvStartup)
echo - Create realistic trade data (NZDUSD, user 16813)
echo - Call MtSrvTradeTransaction directly
echo - Show real ML service communication
echo - Verify the complete A/B routing flow
echo.
echo Ready to run: .\test_plugin_simulation.exe
echo.
pause 