@echo off
echo Building Field 51 Debug Test...

REM Set up Visual Studio environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
)

echo Using Visual Studio compiler (32-bit)...

REM Clean previous builds
del debug_field_51.exe 2>nul
del *.obj 2>nul

echo Compiling field 51 debug test...

REM Compile the test program
cl.exe /EHsc /I. /DWIN32 /D_WINDOWS ^
    /MT /O2 ^
    debug_field_51.cpp ^
    /link kernel32.lib ^
    /OUT:debug_field_51.exe ^
    /MACHINE:X86 ^
    /SUBSYSTEM:CONSOLE ^
    /NOLOGO

if errorlevel 1 (
    echo *** COMPILATION FAILED ***
    pause
    exit /b 1
)

echo.
echo *** SUCCESS: Field 51 Debug Test Built! ***
echo Output: debug_field_51.exe
echo.
echo This will analyze the exact wire type encoding for field 51
echo and identify why the ML service still reports a wire type error.
echo.

dir debug_field_51.exe
echo.
echo To run: debug_field_51.exe
pause 