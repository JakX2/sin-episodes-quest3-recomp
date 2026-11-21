@echo off
setlocal
title SiN Episodes - Original-engine startup diagnostic (NOT GAME BOOT)
cd /d "%~dp0"
echo.
echo SiN Episodes: Emergence 2006 owner-only Source engine diagnostic
echo SHA-authenticated installed originals are copied to a SEPARATE TEMP folder.
echo Both independent x86 DLLs are included in this package.
echo Stages: Stats file presence, Stats read, 66-stat binding and
echo 27:28:5 owned hook proof, then original soundscape rb/GAME reads.
echo Expected diagnostic only: 7 files, 301 soundscapes, 242 references, 3 unresolved.
echo This test DOES NOT start normal gameplay or render/audio playback.
echo CAUTION: The optional -SurfacePropsRegister switch calls a MUTATING original
echo physics-engine method. Use ONLY in the safeguarded disposable test copy.
echo The ordinary invocation without this opt-in remains read-only.
echo.
powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0Start-SiNFileSystemTest.ps1" -StagedStats -HookProof -SoundscapeCatalog %*
set "RESULT=%ERRORLEVEL%"
echo.
if not "%RESULT%"=="0" echo Soundscape probe incomplete or refused; share the small results ZIP.
echo Text-only report ZIP is saved next to this BAT. Originals are never staged in the installed game.
echo.
pause
exit /b %RESULT%
