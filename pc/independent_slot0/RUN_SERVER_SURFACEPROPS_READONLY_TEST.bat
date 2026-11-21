@echo off
setlocal DisableDelayedExpansion
title SiN Episodes - ORIGINAL SERVER SURFACEPROPS RESOURCE READ-ONLY (NOT GAME BOOT)
cd /d "%~dp0"
echo.
echo SiN Episodes: Emergence 2006 - original SERVER physics plus surface scripts
echo.
echo Physics Factory queries: VPhysics030, VPhysicsCollision007, VPhysicsSurfaceProps001
echo Then READ ONLY original rb/GAME manifest and both rb/null source scripts.
echo NO calls to original VPhysicsSurfaceProps001::ParseSurfaceData.
echo NO full server/client DLLInit or playable map boot.
echo.
echo The installed original game is never modified. The guarded launcher uses
echo a SHA-authenticated disposable TEMP copy and verifies original restoration.
echo It retains the original client.dll and stages ONLY independent server.dll
echo inside the copy. Output is a small text-only results ZIP.
echo.
powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0Start-SiNFileSystemTest.ps1" -ServerPhysicsFactory -ServerSurfacePropsRead %*
set "RESULT=%ERRORLEVEL%"
echo.
if not "%RESULT%"=="0" echo Server physics resources were NOT VERIFIED in engine. Share text-only ZIP.
echo No engine physics registration or game map boot has been attempted.
pause
exit /b %RESULT%
