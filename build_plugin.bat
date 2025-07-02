@echo off
echo Building MT4/MT5 A/B-Book Server Plugin...

REM Set Visual Studio environment (adjust path as needed)
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

REM Compile the plugin
cl.exe /LD /EHsc /I. /DWIN32 /D_WINDOWS /D_USRDLL ^
    MT4_Server_Plugin.cpp ^
    /link ws2_32.lib ^
    /OUT:ABBook_Plugin.dll ^
    /DEF:plugin_exports.def

if %ERRORLEVEL%==0 (
    echo Plugin compiled successfully: ABBook_Plugin.dll
) else (
    echo Compilation failed!
    pause
    exit /b 1
)

echo.
echo Copy ABBook_Plugin.dll to your MT4/MT5 server plugins directory
echo Update ABBook_Config.ini with your CVM connection details
echo.
pause 