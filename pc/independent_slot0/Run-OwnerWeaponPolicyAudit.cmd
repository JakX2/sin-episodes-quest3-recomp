@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Run-OwnerWeaponPolicyAudit.ps1" -OriginalServer "%~1"
set "RESULT=%ERRORLEVEL%"
echo.
echo Audit exit code: %RESULT%
pause
exit /b %RESULT%
