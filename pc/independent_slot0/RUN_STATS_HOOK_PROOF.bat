@echo off
setlocal
title SiN Episodes - Independent Stats user-message hook proof
cd /d "%~dp0"
echo.
echo SiN Episodes 2006 owner-only Stats hook diagnostic
echo Independent client and server DLLs are ALREADY included.
echo Original installed game files remain untouched.
echo Checks FileExists, Read, rb/GAME + rt/null and user-message IDs 27:28:5.
echo No gameplay or actual network message dispatch is claimed.
echo.
powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0Start-SiNFileSystemTest.ps1" -StagedStats -HookProof %*
set "RESULT=%ERRORLEVEL%"
echo.
if not "%RESULT%"=="0" echo Hook proof incomplete or refused; send the results ZIP for analysis.
echo The small results ZIP is written beside this BAT. No manual DLL copying needed.
echo.
pause
exit /b %RESULT%
