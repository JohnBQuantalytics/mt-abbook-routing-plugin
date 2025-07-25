@echo off
echo Building ML Response Debugger...
echo =================================

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 2>nul
)

del debug_ml_response.exe 2>nul

echo Compiling debug_ml_response.cpp...
cl.exe /EHsc /MT debug_ml_response.cpp ws2_32.lib /Fe:debug_ml_response.exe

if errorlevel 1 (
    echo.
    echo *** COMPILATION FAILED ***
    echo Check the error messages above
    pause
    exit /b 1
)

echo.
echo =======================================
echo *** SUCCESS: ML RESPONSE DEBUGGER BUILT ***
echo =======================================
echo Output: debug_ml_response.exe
echo.
echo This will analyze the exact ML service response
echo to find where the score field is located.
echo.
pause 