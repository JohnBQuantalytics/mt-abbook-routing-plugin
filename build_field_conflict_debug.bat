@echo off
echo Building Field Conflict Debugger...

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
)

del debug_field_conflict.exe 2>nul

cl.exe /EHsc /MT debug_field_conflict.cpp ws2_32.lib /Fe:debug_field_conflict.exe /NOLOGO

if errorlevel 1 (
    echo *** COMPILATION FAILED ***
    pause
    exit /b 1
)

echo SUCCESS: debug_field_conflict.exe built
pause 