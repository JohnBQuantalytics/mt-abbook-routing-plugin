@echo off
echo Building MT4/MT5 A/B-Book Server Plugin...

REM Try to find Visual Studio installation automatically
set VS_FOUND=0

REM Try VS 2022 first
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    set VS_FOUND=1
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    set VS_FOUND=1
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    set VS_FOUND=1
)

REM Try VS 2019 if VS 2022 not found
if %VS_FOUND%==0 (
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"
        set VS_FOUND=1
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
        set VS_FOUND=1
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
        set VS_FOUND=1
    )
)

if %VS_FOUND%==0 (
    echo ERROR: Visual Studio 2019 or 2022 not found!
    echo Please install Visual Studio with C++ development tools.
    echo Or manually set the path to vcvars64.bat in this script.
    pause
    exit /b 1
)

echo Using Visual Studio compiler...

REM Compile the plugin
cl.exe /LD /EHsc /I. /DWIN32 /D_WINDOWS /D_USRDLL ^
    MT4_ABBook_Plugin.cpp ^
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