@echo off
echo Pushing ALL changes to GitHub...

git add .
git commit -m "v3.1 PRODUCTION: Add package info, cleanup tools, and documentation"
git push origin main

echo.
if %ERRORLEVEL%==0 (
    echo ✅ SUCCESS: All changes pushed to GitHub!
    echo Repository updated with v3.1 production-ready plugin
) else (
    echo ❌ ERROR: Push failed, please check git status
)

pause 