@echo off
setlocal
title SiN Episodes - independent filesystem diagnostic
cd /d "%~dp0"
echo.
echo   SiN Episodes: Emergence - standalone filesystem test
echo   Your Steam installation will NOT be modified.
echo.
powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0Start-SiNFileSystemTest.ps1" %*
set "TEST_EXIT=%ERRORLEVEL%"
echo.
if not "%TEST_EXIT%"=="0" echo Test ended without a verified complete filesystem read. See Results.
echo Detailed results are saved in the package's Results folder.
echo.
pause
exit /b %TEST_EXIT%
