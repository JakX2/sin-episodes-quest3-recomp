@echo off
setlocal DisableDelayedExpansion
title SiN Episodes - ORIGINAL PHYSICS REGISTER AND READBACK (NOT GAME BOOT)
cd /d "%~dp0"
echo.
echo SiN Episodes: Emergence 2006 - PHYSICS REGISTRATION EXPERIMENT
echo.
echo WARNING: Unlike the read-only probe, this test INVOKES original
echo VPhysicsSurfaceProps001::ParseSurfaceData inside the isolated copy.
echo The physics engine may reject the ABI or crash. No normal game boot.
echo Your original Steam DLLs are never replaced by this launcher.
echo The launcher verifies retail SHA256 and restores the disposable copy.
echo.
choice /C YN /N /M "Start the MUTATING physics test? Press Y to opt in or N to cancel: "
if errorlevel 2 (
  echo Physics registration NOT requested. No game was launched.
  pause
  exit /b 2
)
if not errorlevel 1 (
  echo Physics confirmation unavailable. No game was launched.
  pause
  exit /b 2
)
echo.
powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0Start-SiNFileSystemTest.ps1" -StagedStats -HookProof -SoundscapeCatalog -SoundscapeCvarLookup -PhysicsFactoryLookup -SurfacePropsRead -SurfacePropsRegister -SurfacePropsReadback %*
set "RESULT=%ERRORLEVEL%"
echo.
if not "%RESULT%"=="0" echo Physics register/readback incomplete or refused. Share the text-only ZIP.
echo A returned method call proves neither a valid registered table nor game initialization.
pause
exit /b %RESULT%
