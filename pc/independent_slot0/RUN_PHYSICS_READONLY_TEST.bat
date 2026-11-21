@echo off
setlocal DisableDelayedExpansion
title SiN Episodes - ORIGINAL PHYSICS READ-ONLY diagnostic (NOT GAME BOOT)
cd /d "%~dp0"
echo.
echo SiN Episodes: Emergence 2006 - original engine physics READ-ONLY
echo Verifies engine Factory interfaces plus original surface-properties files.
echo NEVER calls ParseSurfaceData. No normal game initialization or map loading.
echo The SHA-checked installed originals are COPIED to a disposable TEMP folder.
echo A small text-only SiN_FileSystem_Test_Results_*.zip is saved beside this BAT.
echo.
powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0Start-SiNFileSystemTest.ps1" -StagedStats -HookProof -SoundscapeCatalog -SoundscapeCvarLookup -PhysicsFactoryLookup -SurfacePropsRead
set "RESULT=%ERRORLEVEL%"
echo.
if not "%RESULT%"=="0" echo Physics read-only diagnostic incomplete or refused. Share the text-only ZIP.
echo The test cannot establish game boot or playable levels.
pause
exit /b %RESULT%
