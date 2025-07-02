@echo off
echo Compiling CVM connection test...

REM Compile the test program
cl.exe simple_connection_test.cpp /link ws2_32.lib /OUT:connection_test.exe

if %ERRORLEVEL%==0 (
    echo Compilation successful!
    echo.
    echo Running connection test to 128.140.42.37:50051...
    echo.
    connection_test.exe
) else (
    echo Compilation failed!
    echo Make sure you have Visual Studio or Windows SDK installed
    pause
)

echo.
pause 