@echo off
setlocal DisableDelayedExpansion
title SiN Episodes - ORIGINAL SERVER PHYSICS RESOURCES (NOT GAME BOOT)
cd /d "%~dp0"
echo.
echo SiN Episodes: Emergence - ORIGINAL SERVER PHYSICS RESOURCE READ
echo.
echo Uses a SHA-authenticated original retail game installation READ ONLY.
echo Makes a fresh entire disposable game copy outside Steam.
echo Stages ONLY an independent server.dll in that copy (retains client.dll).
echo Queries the original server physics interfaces in this exact order:
echo VPhysics030, VPhysicsCollision007, VPhysicsSurfaceProps001.
echo Reads original VFileSystem012 surfaceproperties resource scripts.
echo NEVER invokes ParseSurfaceData. NEVER claims normal game boot.
echo.
powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0Start-SiNFileSystemTest.ps1" -ServerPhysicsReadOnly
set "RESULT=%ERRORLEVEL%"
echo.
if not "%RESULT%"=="0" echo Server resource diagnostic refused or incomplete. Send the text-only ZIP.
echo Any expected 0x53494E7A termination is diagnostic-only, not normal DLLInit.
pause
exit /b %RESULT%
