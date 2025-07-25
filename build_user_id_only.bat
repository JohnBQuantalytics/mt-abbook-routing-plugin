@echo off
echo Building User ID Only Test...

REM Set up Visual Studio environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
)

echo Using Visual Studio compiler (32-bit)...

REM Clean previous builds
del debug_user_id_only.exe 2>nul
del *.obj 2>nul

echo Compiling user ID only test...

REM Compile the test program
cl.exe /EHsc /I. /DWIN32 /D_WINDOWS /D_WIN32_WINNT=0x0601 ^
    /MT /O2 ^
    debug_user_id_only.cpp ^
    /link ws2_32.lib kernel32.lib ^
    /OUT:debug_user_id_only.exe ^
    /MACHINE:X86 ^
    /SUBSYSTEM:CONSOLE ^
    /NOLOGO

if errorlevel 1 (
    echo *** COMPILATION FAILED ***
    pause
    exit /b 1
)

echo.
echo *** SUCCESS: User ID Only Test Built! ***
echo Output: debug_user_id_only.exe
echo.
echo This sends ONLY field 51 (user_id) to isolate the exact bug.
echo If it still fails, we know the issue is in our encoding.
echo.

dir debug_user_id_only.exe
echo.
echo To run: debug_user_id_only.exe
pause 