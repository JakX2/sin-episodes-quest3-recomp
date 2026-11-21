@echo off
setlocal DisableDelayedExpansion
title SiN Episodes - ORIGINAL SERVER PHYSICS FACTORY (NOT GAME BOOT)
cd /d "%~dp0"
echo.
echo SiN Episodes: Emergence 2006 - SERVER-side physics interface test
echo.
echo Phase520: tests ORIGINAL SERVER order using independently compiled server.dll:
echo    VPhysics030 ; VPhysicsCollision007 ; VPhysicsSurfaceProps001
echo.
echo Separate from CLIENT physics registration and surface-readback tests.
echo The original installed game is READ ONLY. The launcher makes a disposable
echo full game copy, authenticates the original DLLs and verifies restoration.
echo Server-only mode leaves the original client.dll unchanged in the clone.
echo.
echo By default the launcher selects the first existing retail BSP name,
echo only to cause the engine to request ServerGameDLL003::DLLInit.
echo To specify a retail map:  RUN_SERVER_PHYSICS_FACTORY_TEST.bat -ServerMap name
echo This is a READ-ONLY interface diagnostic, NOT playable map startup.
echo.
powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0Start-SiNFileSystemTest.ps1" -ServerPhysicsFactory %*
set "RESULT=%ERRORLEVEL%"
echo.
if not "%RESULT%"=="0" echo Server physics factory test did not reach a verified 3/3 result. Share text-only ZIP.
echo No independent ServerGameDLL003 normal initialization is claimed.
pause
exit /b %RESULT%
