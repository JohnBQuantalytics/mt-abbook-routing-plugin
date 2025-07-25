@echo off
echo Building Enhanced ML Response Debug...

REM Set up Visual Studio environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
)

echo Using Visual Studio compiler (32-bit)...

REM Clean previous builds
del debug_ml_response.exe 2>nul
del *.obj 2>nul

echo Compiling enhanced debug test...

REM Compile the test program
cl.exe /EHsc /I. /DWIN32 /D_WINDOWS /D_WIN32_WINNT=0x0601 ^
    /MT /O2 ^
    debug_ml_response.cpp ^
    /link ws2_32.lib kernel32.lib ^
    /OUT:debug_ml_response.exe ^
    /MACHINE:X86 ^
    /SUBSYSTEM:CONSOLE ^
    /NOLOGO

if errorlevel 1 (
    echo *** COMPILATION FAILED ***
    pause
    exit /b 1
)

echo.
echo *** SUCCESS: Enhanced Debug Test Built! ***
echo Output: debug_ml_response.exe
echo.
echo This will show EXACTLY what happens during recv()
echo including detailed timing and WSA error analysis.
echo.

dir debug_ml_response.exe
echo.
echo To run: debug_ml_response.exe
pause 