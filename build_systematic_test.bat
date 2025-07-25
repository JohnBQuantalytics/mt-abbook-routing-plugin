@echo off
echo Building Systematic ML Test...

REM Set up Visual Studio environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
)

echo Using Visual Studio compiler (32-bit)...

REM Clean previous builds
del systematic_ml_test.exe 2>nul
del *.obj 2>nul

echo Compiling systematic ML test...

REM Compile the test program
cl.exe /EHsc /I. /DWIN32 /D_WINDOWS /D_WIN32_WINNT=0x0601 ^
    /MT /O2 ^
    systematic_ml_test.cpp ^
    /link ws2_32.lib kernel32.lib ^
    /OUT:systematic_ml_test.exe ^
    /MACHINE:X86 ^
    /SUBSYSTEM:CONSOLE ^
    /NOLOGO

if errorlevel 1 (
    echo *** COMPILATION FAILED ***
    pause
    exit /b 1
)

echo.
echo *** SUCCESS: Systematic ML Test Built! ***
echo Output: systematic_ml_test.exe
echo.
echo This will try 10+ different formats systematically
echo until we find one that works with the ML service.
echo.

dir systematic_ml_test.exe
echo.
echo To run: systematic_ml_test.exe
pause 