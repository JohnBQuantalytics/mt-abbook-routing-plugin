@echo off
echo Building User ID Field Debug Test...

REM Set up Visual Studio environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
)

echo Using Visual Studio compiler (32-bit)...

REM Clean previous builds
del debug_user_id_field.exe 2>nul
del *.obj 2>nul

echo Compiling debug test...

REM Compile the test program
cl.exe /EHsc /I. /DWIN32 /D_WINDOWS ^
    /MT /O2 ^
    debug_user_id_field.cpp ^
    /link kernel32.lib ^
    /OUT:debug_user_id_field.exe ^
    /MACHINE:X86 ^
    /SUBSYSTEM:CONSOLE ^
    /NOLOGO

if errorlevel 1 (
    echo *** COMPILATION FAILED ***
    pause
    exit /b 1
)

echo.
echo *** SUCCESS: Debug Test Built! ***
echo Output: debug_user_id_field.exe
echo.
echo This will show exactly how we encode field 51 (user_id)
echo and help identify the wire type issue.
echo.

dir debug_user_id_field.exe
echo.
echo To run: debug_user_id_field.exe
pause 