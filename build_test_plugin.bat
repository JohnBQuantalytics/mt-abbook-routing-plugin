@echo off
echo Building Simple Test Plugin for MT4 Compatibility Check...

REM Set up Visual Studio environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
)

echo Using Visual Studio compiler (32-bit)...

REM Clean previous builds
del test_plugin_simple_32bit.dll 2>nul
del *.obj 2>nul

echo Compiling simple test plugin...

REM Compile the test plugin
cl.exe /LD /EHsc /I. /DWIN32 /D_WINDOWS /D_USRDLL /D_WIN32_WINNT=0x0601 ^
    /MT /O2 ^
    test_plugin_simple.cpp ^
    /link kernel32.lib ^
    /OUT:test_plugin_simple_32bit.dll ^
    /DEF:plugin_exports.def ^
    /MACHINE:X86 ^
    /SUBSYSTEM:WINDOWS ^
    /NOLOGO

if errorlevel 1 (
    echo *** COMPILATION FAILED ***
    pause
    exit /b 1
)

echo.
echo *** SUCCESS: Test plugin built! ***
echo Output: test_plugin_simple_32bit.dll
echo.
echo This plugin can be used to test basic MT4 compatibility
echo before deploying the full ABBook plugin.
echo.

dir test_plugin_simple_32bit.dll
pause 