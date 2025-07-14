@echo off
echo Building MT4/MT5 A/B-Book Server Plugin (32-bit)...

REM Try to find Visual Studio installation automatically
set VS_FOUND=0

REM Try VS 2022 first (32-bit)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars32.bat"
    set VS_FOUND=1
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"
    set VS_FOUND=1
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars32.bat"
    set VS_FOUND=1
)

REM Try VS 2019 if VS 2022 not found (32-bit)
if %VS_FOUND%==0 (
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars32.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars32.bat"
        set VS_FOUND=1
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
        set VS_FOUND=1
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars32.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars32.bat"
        set VS_FOUND=1
    )
)

if %VS_FOUND%==0 (
    echo ERROR: Visual Studio 2019 or 2022 not found!
    echo Please install Visual Studio with C++ development tools.
    echo Or manually set the path to vcvars32.bat in this script.
    pause
    exit /b 1
)

echo Using Visual Studio compiler (32-bit)...

REM Clean previous build
if exist ABBook_Plugin_32bit.dll del ABBook_Plugin_32bit.dll
if exist ABBook_Plugin_32bit.lib del ABBook_Plugin_32bit.lib
if exist ABBook_Plugin_32bit.exp del ABBook_Plugin_32bit.exp
if exist *.obj del *.obj

REM Compile the plugin for 32-bit with MT4 compatibility flags
cl.exe /LD /EHsc /I. /DWIN32 /D_WINDOWS /D_USRDLL /D_WIN32_WINNT=0x0601 ^
    /MT /O2 /Zi /Fd:ABBook_Plugin_32bit.pdb ^
    MT4_ABBook_Plugin.cpp ^
    /link ws2_32.lib user32.lib kernel32.lib ^
    /OUT:ABBook_Plugin_32bit.dll ^
    /DEF:plugin_exports.def ^
    /MACHINE:X86 ^
    /SUBSYSTEM:WINDOWS ^
    /NOLOGO

if %ERRORLEVEL%==0 (
    echo Plugin compiled successfully: ABBook_Plugin_32bit.dll
    echo.
    echo Checking plugin dependencies...
    dumpbin /dependents ABBook_Plugin_32bit.dll
    echo.
    echo This is a 32-bit version compatible with 32-bit MT4/MT5 servers
    echo Copy ABBook_Plugin_32bit.dll to your MT4/MT5 server plugins directory
    echo.
    echo Required files for deployment:
    echo - ABBook_Plugin_32bit.dll
    echo - ABBook_Config.ini
    echo.
) else (
    echo Compilation failed!
    echo Check the error messages above for details
    pause
    exit /b 1
)

echo.
echo Update ABBook_Config.ini with your CVM connection details
echo.
pause 