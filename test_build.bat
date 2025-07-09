@echo off
echo Building test program...

REM Set up Visual Studio environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

REM Compile test program
cl.exe /EHsc test_plugin.cpp /Fe:test_plugin.exe

echo Test program compiled.
pause 