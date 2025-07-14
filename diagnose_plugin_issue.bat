@echo off
echo MT4 Plugin Diagnostic Script
echo =============================
echo This script helps identify issues with plugin loading
echo.

REM Create log file
set LOG_FILE=plugin_diagnostic.log
echo Plugin Diagnostic Report > %LOG_FILE%
echo Generated: %DATE% %TIME% >> %LOG_FILE%
echo. >> %LOG_FILE%

echo 1. Checking system information...
echo System Information: >> %LOG_FILE%
systeminfo | findstr /B /C:"OS Name" /C:"System Type" /C:"OS Version" >> %LOG_FILE%
echo. >> %LOG_FILE%

echo 2. Checking Visual Studio installations...
echo Visual Studio Installations: >> %LOG_FILE%
if exist "C:\Program Files\Microsoft Visual Studio\" (
    dir "C:\Program Files\Microsoft Visual Studio\" /B >> %LOG_FILE%
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\" (
    dir "C:\Program Files (x86)\Microsoft Visual Studio\" /B >> %LOG_FILE%
)
echo. >> %LOG_FILE%

echo 3. Checking for plugin files...
echo Plugin Files: >> %LOG_FILE%
if exist "ABBook_Plugin_32bit.dll" (
    echo ABBook_Plugin_32bit.dll - EXISTS >> %LOG_FILE%
    echo File size: >> %LOG_FILE%
    dir ABBook_Plugin_32bit.dll >> %LOG_FILE%
) else (
    echo ABBook_Plugin_32bit.dll - NOT FOUND >> %LOG_FILE%
)

if exist "ABBook_Plugin.dll" (
    echo ABBook_Plugin.dll - EXISTS >> %LOG_FILE%
    echo File size: >> %LOG_FILE%
    dir ABBook_Plugin.dll >> %LOG_FILE%
) else (
    echo ABBook_Plugin.dll - NOT FOUND >> %LOG_FILE%
)

if exist "ABBook_Config.ini" (
    echo ABBook_Config.ini - EXISTS >> %LOG_FILE%
) else (
    echo ABBook_Config.ini - NOT FOUND >> %LOG_FILE%
)
echo. >> %LOG_FILE%

echo 4. Checking plugin dependencies...
if exist "ABBook_Plugin_32bit.dll" (
    echo Plugin Dependencies (32-bit): >> %LOG_FILE%
    dumpbin /dependents ABBook_Plugin_32bit.dll >> %LOG_FILE% 2>&1
) else (
    echo Cannot check dependencies - ABBook_Plugin_32bit.dll not found >> %LOG_FILE%
)
echo. >> %LOG_FILE%

echo 5. Checking Visual C++ Runtime...
echo Visual C++ Runtime Check: >> %LOG_FILE%
reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x86" >> %LOG_FILE% 2>&1
reg query "HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\VisualStudio\14.0\VC\Runtimes\x86" >> %LOG_FILE% 2>&1
echo. >> %LOG_FILE%

echo 6. Checking file permissions...
echo File Permissions: >> %LOG_FILE%
if exist "ABBook_Plugin_32bit.dll" (
    icacls ABBook_Plugin_32bit.dll >> %LOG_FILE%
) else (
    echo Cannot check permissions - file not found >> %LOG_FILE%
)
echo. >> %LOG_FILE%

echo 7. Testing plugin loading...
echo Plugin Loading Test: >> %LOG_FILE%
if exist "ABBook_Plugin_32bit.dll" (
    echo Attempting to load plugin... >> %LOG_FILE%
    rundll32.exe ABBook_Plugin_32bit.dll,PluginInit >> %LOG_FILE% 2>&1
    if %ERRORLEVEL%==0 (
        echo Plugin loaded successfully >> %LOG_FILE%
    ) else (
        echo Plugin failed to load - Error code: %ERRORLEVEL% >> %LOG_FILE%
    )
) else (
    echo Cannot test loading - plugin file not found >> %LOG_FILE%
)
echo. >> %LOG_FILE%

echo 8. Checking MT4 server process...
echo MT4 Server Process Check: >> %LOG_FILE%
tasklist | findstr /I "terminal" >> %LOG_FILE%
tasklist | findstr /I "mt4" >> %LOG_FILE%
tasklist | findstr /I "mt5" >> %LOG_FILE%
echo. >> %LOG_FILE%

echo 9. Checking antivirus interference...
echo Antivirus Check: >> %LOG_FILE%
powershell -Command "Get-MpPreference | Select-Object -Property ExclusionPath" >> %LOG_FILE% 2>&1
echo. >> %LOG_FILE%

echo 10. Generating recommendations...
echo Recommendations: >> %LOG_FILE%
echo. >> %LOG_FILE%

if not exist "ABBook_Plugin_32bit.dll" (
    echo - CRITICAL: Plugin DLL not found. Run build_plugin_32bit.bat first. >> %LOG_FILE%
)

if not exist "ABBook_Config.ini" (
    echo - WARNING: Configuration file missing. Plugin may not initialize properly. >> %LOG_FILE%
)

echo - Ensure MT4 server is running as administrator >> %LOG_FILE%
echo - Check that plugin directory has write permissions >> %LOG_FILE%
echo - Verify Visual C++ Redistributable (x86) is installed >> %LOG_FILE%
echo - Add plugin DLL to antivirus exception list >> %LOG_FILE%
echo - Confirm MT4 server is 32-bit if using 32-bit plugin >> %LOG_FILE%

echo.
echo Diagnostic complete! Check %LOG_FILE% for detailed results.
echo.
echo Please send this log file to support for analysis.
echo.
pause 