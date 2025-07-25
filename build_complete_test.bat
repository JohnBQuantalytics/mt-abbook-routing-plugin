@echo off
echo Building Complete Format Test...

REM Set up Visual Studio environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
)

echo Using Visual Studio compiler (32-bit)...

REM Clean previous builds
del test_complete_format.exe 2>nul
del *.obj 2>nul

echo Compiling complete format test...

REM Compile the test program
cl.exe /EHsc /I. /DWIN32 /D_WINDOWS /D_WIN32_WINNT=0x0601 ^
    /MT /O2 ^
    test_complete_format.cpp ^
    /link ws2_32.lib kernel32.lib ^
    /OUT:test_complete_format.exe ^
    /MACHINE:X86 ^
    /SUBSYSTEM:CONSOLE ^
    /NOLOGO

if errorlevel 1 (
    echo *** COMPILATION FAILED ***
    pause
    exit /b 1
)

echo.
echo *** SUCCESS: Complete Format Test Built! ***
echo Output: test_complete_format.exe
echo.
echo This will progressively test from minimal working format
echo to find the maximum number of fields that work.
echo.

dir test_complete_format.exe
echo.
echo To run: test_complete_format.exe
pause 