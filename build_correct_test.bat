@echo off
echo Building Correct Format ML Service Test...

REM Set up Visual Studio environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
)

echo Using Visual Studio compiler (32-bit)...

REM Clean previous builds
del test_ml_correct_format.exe 2>nul
del *.obj 2>nul

echo Compiling correct format ML service test...

REM Compile the test program
cl.exe /EHsc /I. /DWIN32 /D_WINDOWS /D_WIN32_WINNT=0x0601 ^
    /MT /O2 ^
    test_ml_correct_format.cpp ^
    /link ws2_32.lib kernel32.lib ^
    /OUT:test_ml_correct_format.exe ^
    /MACHINE:X86 ^
    /SUBSYSTEM:CONSOLE ^
    /NOLOGO

if errorlevel 1 (
    echo *** COMPILATION FAILED ***
    pause
    exit /b 1
)

echo.
echo *** SUCCESS: Correct Format Test Built! ***
echo Output: test_ml_correct_format.exe
echo.
echo This test uses the EXACT format from work requirements:
echo - Raw TCP connection with length-prefixed binary packets
echo - Proper protobuf encoding with correct field numbers
echo - Based on actual ScoringRequest/ScoringResponse spec
echo.

dir test_ml_correct_format.exe
echo.
echo To run: test_ml_correct_format.exe
pause 