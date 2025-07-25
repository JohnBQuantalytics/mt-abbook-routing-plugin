@echo off
echo Building Meaningful Fields Test...

REM Set up Visual Studio environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
)

echo Using Visual Studio compiler (32-bit)...

REM Clean previous builds
del test_meaningful_fields.exe 2>nul
del *.obj 2>nul

echo Compiling meaningful fields test...

REM Compile the test program
cl.exe /EHsc /I. /DWIN32 /D_WINDOWS /D_WIN32_WINNT=0x0601 ^
    /MT /O2 ^
    test_meaningful_fields.cpp ^
    /link ws2_32.lib kernel32.lib ^
    /OUT:test_meaningful_fields.exe ^
    /MACHINE:X86 ^
    /SUBSYSTEM:CONSOLE ^
    /NOLOGO

if errorlevel 1 (
    echo *** COMPILATION FAILED ***
    pause
    exit /b 1
)

echo.
echo *** SUCCESS: Meaningful Fields Test Built! ***
echo Output: test_meaningful_fields.exe
echo.

dir test_meaningful_fields.exe
echo.
echo To run: test_meaningful_fields.exe
pause 