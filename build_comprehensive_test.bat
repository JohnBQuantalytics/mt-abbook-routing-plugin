@echo off
echo ====================================
echo Building ABBook Plugin and Test Suite
echo ML Scoring Service: 188.245.254.12:50051
echo ====================================

:: Check if compiler is available
where cl.exe >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: Visual Studio compiler (cl.exe) not found
    echo Please run this from a Visual Studio Developer Command Prompt
    pause
    exit /b 1
)

:: Clean old files
echo Cleaning old files...
del /q ABBook_Plugin_32bit.dll 2>nul
del /q test_mt4_plugin_comprehensive.exe 2>nul
del /q *.obj 2>nul
del /q *.lib 2>nul
del /q *.exp 2>nul
del /q *.pdb 2>nul

:: Build the plugin DLL
echo Building ABBook Plugin DLL...
cl.exe /LD /EHsc /O2 /DWIN32 /D_WINDOWS /D_USRDLL MT4_ABBook_Plugin.cpp /Fe:ABBook_Plugin_32bit.dll /link /DEF:plugin_exports.def ws2_32.lib kernel32.lib user32.lib advapi32.lib

if %errorlevel% neq 0 (
    echo Error: Failed to build ABBook Plugin DLL
    pause
    exit /b 1
)

echo ABBook Plugin DLL built successfully

:: Build the comprehensive test
echo Building comprehensive test...
cl.exe /EHsc /O2 /DWIN32 /D_WINDOWS test_mt4_plugin_comprehensive.cpp /Fe:test_mt4_plugin_comprehensive.exe /link ws2_32.lib kernel32.lib user32.lib

if %errorlevel% neq 0 (
    echo Error: Failed to build comprehensive test
    pause
    exit /b 1
)

echo Comprehensive test built successfully

:: Clean temporary files
echo Cleaning temporary files...
del /q *.obj 2>nul
del /q *.lib 2>nul
del /q *.exp 2>nul

:: Check if files were created
if exist ABBook_Plugin_32bit.dll (
    echo ✓ ABBook_Plugin_32bit.dll created
) else (
    echo ✗ ABBook_Plugin_32bit.dll not found
)

if exist test_mt4_plugin_comprehensive.exe (
    echo ✓ test_mt4_plugin_comprehensive.exe created
) else (
    echo ✗ test_mt4_plugin_comprehensive.exe not found
)

if exist ABBook_Config.ini (
    echo ✓ ABBook_Config.ini found
) else (
    echo ✗ ABBook_Config.ini not found
)

echo.
echo ====================================
echo Build Summary
echo ====================================
echo Plugin DLL: ABBook_Plugin_32bit.dll
echo Test App: test_mt4_plugin_comprehensive.exe
echo Config: ABBook_Config.ini
echo.
echo Next steps:
echo 1. Ensure ML scoring service is running at 188.245.254.12:50051
echo 2. Run: test_mt4_plugin_comprehensive.exe
echo 3. Check log files for results
echo ====================================

pause 