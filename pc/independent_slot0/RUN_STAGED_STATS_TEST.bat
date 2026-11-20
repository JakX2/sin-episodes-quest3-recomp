@echo off
setlocal
title SiN Episodes - original-mode staged stats test
cd /d "%~dp0"
echo.
echo   SiN Episodes 2006 - Original-mode staged stats diagnostic
echo   No manual DLL copying. The installed game is READ ONLY.
echo   Tests presence, engine read, then rb/GAME and rt/null staging.
echo.
powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0Start-SiNFileSystemTest.ps1" -StagedStats %*
set "RESULT=%ERRORLEVEL%"
echo.
if not "%RESULT%"=="0" echo Staged diagnostic incomplete or refused. See RESULTS_ZIP.
echo The results ZIP is saved next to this BAT. Send it for analysis.
echo.
pause
exit /b %RESULT%
