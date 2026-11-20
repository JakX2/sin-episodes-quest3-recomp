@echo off
setlocal
title SiN Episodes - Recover older disposable tests
cd /d "%~dp0"
echo.
echo Checking Windows TEMP for older SiN filesystem diagnostic copies.
echo Your installed SiN Episodes folder will NOT be modified.
echo Recovery requires an exact marker and authenticated original DLLs.
echo.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Recover-SiNRetainedTestCopy.ps1" -ScanTemp -CleanupRecoveredCopy
set "RESULT=%ERRORLEVEL%"
echo.
if not "%RESULT%"=="0" echo Some copies could not be safely recovered. No forced removal was performed.
echo.
pause
exit /b %RESULT%
