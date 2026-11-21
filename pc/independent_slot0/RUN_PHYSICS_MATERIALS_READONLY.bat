@echo off
setlocal DisableDelayedExpansion
title SiN Episodes - Physics material parameters READ ONLY (NOT GAME BOOT)
cd /d "%~dp0"
echo.
echo SiN Episodes: Emergence 2006 - PHYSICS MATERIAL DATA (READ ONLY)
echo Reads original VPhysicsSurfaceProps001 surface data for:
echo default, solidmetal, metalvehicle.
echo Five physical float fields each: friction, elasticity, density,
echo thickness, dampening. DOES NOT CALL ParseSurfaceData.
echo Uses authenticated originals in a disposable copy only.
echo.
powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0Start-SiNFileSystemTest.ps1" -StagedStats -HookProof -SoundscapeCatalog -SoundscapeCvarLookup -PhysicsFactoryLookup -SurfacePropsRead -SurfacePhysicsDataRead %*
set "RESULT=%ERRORLEVEL%"
echo.
if not "%RESULT%"=="0" echo Physics material probe incomplete or refused. Share the text-only ZIP.
echo Neither success nor failure establishes normal game startup.
pause
exit /b %RESULT%
