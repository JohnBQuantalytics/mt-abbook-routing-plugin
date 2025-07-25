@echo off
echo Building Field 60 Encoding Debugger...

REM Set up Visual Studio environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
)

echo Using Visual Studio compiler (32-bit)...

REM Clean previous builds
del debug_field_60_encoding.exe 2>nul
del *.obj 2>nul

echo Compiling field 60 encoding debugger...

REM Compile the debug program
cl.exe /EHsc /I. /DWIN32 /D_WINDOWS /D_WIN32_WINNT=0x0601 ^
    /MT /O2 ^
    debug_field_60_encoding.cpp ^
    /link ws2_32.lib kernel32.lib ^
    /OUT:debug_field_60_encoding.exe ^
    /MACHINE:X86 ^
    /SUBSYSTEM:CONSOLE ^
    /NOLOGO

if errorlevel 1 (
    echo *** COMPILATION FAILED ***
    pause
    exit /b 1
)

echo.
echo *** SUCCESS: Field 60 Debugger Built! ***
echo Output: debug_field_60_encoding.exe
echo.

dir debug_field_60_encoding.exe
echo.
pause 