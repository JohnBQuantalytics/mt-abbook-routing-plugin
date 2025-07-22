@echo off
echo ===============================================
echo Cleaning up repository - removing outdated files
echo ===============================================

REM Remove build artifacts
echo Removing build artifacts...
del *.pdb 2>nul
del *.ilk 2>nul  
del *.obj 2>nul
del *.lib 2>nul
del *.exp 2>nul

REM Remove outdated plugin versions
echo Removing outdated plugin versions...
del ABBook_Plugin_Fixed_32bit.dll 2>nul
del simple_test_plugin_32bit.dll 2>nul
del ABBook_Plugin_32bit.pdb 2>nul

REM Remove outdated source files
echo Removing outdated source files...
del MT4_ABBook_Plugin.cpp 2>nul
del MT4_ABBook_Plugin_Fixed.cpp 2>nul
del test_plugin_simple.cpp 2>nul

REM Remove outdated build scripts  
echo Removing outdated build scripts...
del build_fixed_plugin.bat 2>nul
del build_simple_test.bat 2>nul
del build_plugin_32bit.bat 2>nul

REM Remove outdated zip creation scripts
echo Removing outdated zip scripts...
del create_v3.1_final.bat 2>nul
del make_v3.1_zip.bat 2>nul
del create_bulletproof_zip.bat 2>nul
del create_bulletproof_zip.ps1 2>nul
del create_simple_zip.ps1 2>nul

REM Remove outdated documentation
echo Removing outdated documentation...
del ABBook_Plugin_v3.1_Package_Info.txt 2>nul
del CRITICAL_FIXES_SUMMARY_v3.1.md 2>nul
del CLEANUP_INSTRUCTIONS.md 2>nul
del TESTING_SUMMARY.md 2>nul

REM Remove old test files
echo Removing old test files...
del test_plugin.cpp 2>nul
del simple_connection_test.cpp 2>nul
del test_connection.bat 2>nul

echo.
echo ===============================================
echo Repository cleanup completed!
echo ===============================================
echo.
echo Kept essential files:
echo ✓ ABBook_Plugin_Official_32bit.dll (current version)
echo ✓ MT4_ABBook_Plugin_Official.cpp (current source)
echo ✓ build_official_plugin.bat (current build)
echo ✓ make_bulletproof_zip.bat (current zip creation)
echo ✓ ABBook_Plugin_BULLETPROOF_v4.0.zip (latest package)
echo ✓ All current documentation files
echo ✓ Configuration and testing files
echo.
pause 