@echo off
echo ===============================================
echo Creating BULLETPROOF MT4 Plugin Package v4.0
echo ===============================================
echo.

REM Clean up old zip files
del ABBook_Plugin_*.zip 2>nul
del *_v3*.zip 2>nul

echo Creating ABBook_Plugin_BULLETPROOF_v4.0.zip...
echo.

REM Use PowerShell to create zip
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
"$files = @( ^
'ABBook_Plugin_Official_32bit.dll', ^
'MT4_ABBook_Plugin_Official.cpp', ^
'build_official_plugin.bat', ^
'plugin_exports.def', ^
'BULLETPROOF_PLUGIN_FEATURES.md', ^
'PLUGIN_VERSIONS_EXPLAINED.md', ^
'README.md', ^
'TECHNICAL_SPECIFICATION.md', ^
'PRODUCT_REQUIREMENTS_DOCUMENT.md', ^
'BROKER_INTEGRATION_QUESTIONS.md', ^
'DEPLOYMENT_CHECKLIST.md', ^
'INSTALLATION_MANUAL.md', ^
'ABBook_Config.ini', ^
'scoring.proto', ^
'test_scoring_service.py', ^
'TESTING_GUIDE_ML_SERVICE.md', ^
'CLIENT_ACCESS_GUIDE.md' ^
); ^
$existing = @(); ^
foreach ($f in $files) { if (Test-Path $f) { $existing += $f; Write-Host 'Adding:' $f } }; ^
Write-Host ''; ^
Write-Host 'Creating zip with' $existing.Count 'files...'; ^
Compress-Archive -Path $existing -DestinationPath 'ABBook_Plugin_BULLETPROOF_v4.0.zip' -Force; ^
$zip = Get-Item 'ABBook_Plugin_BULLETPROOF_v4.0.zip'; ^
Write-Host 'SUCCESS: Package created!' -ForegroundColor Green; ^
Write-Host 'File:' $zip.Name; ^
Write-Host 'Size:' ([math]::Round($zip.Length/1MB, 2)) 'MB';"

if errorlevel 1 (
    echo ERROR: Failed to create zip package
    pause
    exit /b 1
)

echo.
echo ===============================================
echo *** BULLETPROOF PLUGIN PACKAGE READY ***
echo ===============================================
echo Package: ABBook_Plugin_BULLETPROOF_v4.0.zip
echo.
echo BULLETPROOF FEATURES:
echo ✓ NEVER unloads if ML service unavailable
echo ✓ Automatic retry with smart backoff  
echo ✓ Graceful fallback to A-book routing
echo ✓ Official MT4 structures (no corruption)
echo ✓ Production ready for immediate deployment
echo.
echo Ready for deployment to MT4 server!
echo ===============================================

REM Show package details
echo.
echo Package details:
dir ABBook_Plugin_BULLETPROOF_v4.0.zip

echo.
echo All done!
pause 