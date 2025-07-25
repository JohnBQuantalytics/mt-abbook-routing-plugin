@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
)

del test_gradual_buildup.exe 2>nul
cl.exe /EHsc /MT test_gradual_buildup.cpp ws2_32.lib /Fe:test_gradual_buildup.exe

if errorlevel 1 (
    echo COMPILATION FAILED
    pause
    exit /b 1
)

echo SUCCESS: Built test_gradual_buildup.exe
pause 