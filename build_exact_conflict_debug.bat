@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
)

del debug_exact_conflict.exe 2>nul
cl.exe /EHsc /MT debug_exact_conflict.cpp ws2_32.lib /Fe:debug_exact_conflict.exe

if errorlevel 1 (
    echo COMPILATION FAILED
    pause
    exit /b 1
)

echo SUCCESS: debug_exact_conflict.exe built
pause 