@echo off
echo Building Protobuf ML Service Test...

REM Set up Visual Studio environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
)

echo Using Visual Studio compiler (32-bit)...

REM Clean previous builds
del test_ml_protobuf.exe 2>nul
del *.obj 2>nul

echo Compiling protobuf ML service test...

REM Compile the test program
cl.exe /EHsc /I. /DWIN32 /D_WINDOWS /D_WIN32_WINNT=0x0601 ^
    /MT /O2 ^
    test_ml_protobuf.cpp ^
    /link ws2_32.lib kernel32.lib ^
    /OUT:test_ml_protobuf.exe ^
    /MACHINE:X86 ^
    /SUBSYSTEM:CONSOLE ^
    /NOLOGO

if errorlevel 1 (
    echo *** COMPILATION FAILED ***
    pause
    exit /b 1
)

echo.
echo *** SUCCESS: Protobuf ML Test Built! ***
echo Output: test_ml_protobuf.exe
echo.
echo This program will test sending protobuf ScoringRequest
echo to the ML service with sample trading data.
echo.

dir test_ml_protobuf.exe
echo.
echo To run the test: test_ml_protobuf.exe
pause 