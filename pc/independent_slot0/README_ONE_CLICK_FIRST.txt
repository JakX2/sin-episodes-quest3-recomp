SiN EPISODES: EMERGENCE - STANDALONE WINDOWS FILESYSTEM TEST
================================================================

THIS ZIP ALREADY INCLUDES THE INDEPENDENT client.dll AND server.dll.
You do NOT need to copy, replace, extract or configure any DLL yourself.

QUICK START
-----------
1. Extract the ENTIRE ZIP to a regular folder such as your Desktop.
   Do not extract it inside the installed game directory.
2. Double-click RUN_SIN_TEST.bat.
3. When the folder dialog appears, select the original SiN Episodes
   Emergence folder containing SinEpisodes.exe, then click OK.
4. The test copies the game to a private temporary folder and starts
   the presence-only diagnostic. If that succeeds AND the original DLL
   is restored, it automatically checks the two stats files with
   bounded read-only filesystem methods.
5. When the window finishes, find SiN_FileSystem_Test_Results_....zip
   in the SAME folder as RUN_SIN_TEST.bat. Send that small ZIP to
   ChatGPT for diagnosis. It contains text reports, NOT game DLLs.

YOUR ORIGINAL GAME INSTALLATION
-------------------------------
The test NEVER replaces any DLL in the folder you selected.
It reads only from that installed folder, validates original hashes,
and makes a completely separate disposable test copy automatically.
No Steam installation modifications or manually created markers.
You need enough free space in the Windows TEMP drive for another
full copy of the game. The test may also refuse if the retail DLLs
have been modified or your game files contain junctions/symlinks.

The temporary copy is deleted ONLY after verifying restoration of
the original DLLs. If the diagnostic cannot prove restoration, it
preserves the disposable copy and prints its recovery location.
You should never manually remove backups from a running game.

IF YOU WANT A COMMAND INSTEAD OF THE FOLDER PICKER
--------------------------------------------------
Run this in a PowerShell terminal inside the extracted package:

powershell -NoProfile -STA -ExecutionPolicy Bypass -File .\Start-SiNFileSystemTest.ps1 -GameRoot "C:\path\to\SiN Episodes Emergence"

OUTPUT
------
SUMMARY.txt: which stage passed or failed
presence.txt / read.txt: exact owner startup evidence
presence-run.txt / read-run.txt: diagnostic wrapper transcript
sin-independent-*.log: session traces (if generated)
copy-summary.txt: disposable copy status
SiN_FileSystem_Test_Results_....zip: the reports packaged for sharing

The terminal diagnostic intentionally stops the game, and is NOT a
normal successful VClient011::Init, game menu, map load, or gameplay.
The objective is checking the original Source filesystem ABI before
continued decompilation, Windows port work, Android, and Quest 3.

BUILD PROVENANCE
----------------
The package is produced by the repository's Windows GitHub Actions
and contains a SHA256 manifest.json recording every included file
and the exact source commit. No retail original DLLs are distributed.
