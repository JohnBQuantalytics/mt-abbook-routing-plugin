@echo off
echo Building UTF-8 Symbol Encoding Test...

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
if errorlevel 1 (
    echo Error: Visual Studio not found or failed to initialize
    pause
    exit /b 1
)

echo Compiling test_utf8_symbol_fix.cpp...
cl /EHsc test_utf8_symbol_fix.cpp /Fe:test_utf8_symbol_fix.exe

if errorlevel 1 (
    echo Error: Compilation failed
    pause
    exit /b 1
)

echo Success: test_utf8_symbol_fix.exe built successfully
echo Running test...
echo.
test_utf8_symbol_fix.exe

pause 