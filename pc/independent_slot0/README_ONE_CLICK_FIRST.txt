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

VERIFIED RETAIL MILESTONE FROM OWNER TEST ON 2026-11-20
------------------------------------------------------
A live original Windows engine returned 0x53494E68 (both files found)
and later 0x53494E6B (actual VFileSystem012 read/parsed counts
62:8:156:66). This is real engine read-method evidence, not boot.

The second attempt showed a Windows UnauthorizedAccessException when
the launcher attempted to restore the original DLL inside its TEMP
test copy. The installed game was NOT modified. New versions retry
the post-exit file lock and attempt SHA-checked restoration before
deciding to retain the temporary folder.

RECOVER AN OLDER RETAINED TEST COPY (NO MANUAL DLL HANDLING)
----------------------------------------------------------
Extract this updated package anywhere outside the game folder, then
double-click RECOVER_PREVIOUS_TEST.bat. It searches only the precise
sin-independent-fs-<10 hex> directories directly under Windows TEMP,
checks the disposable-test marker and original 2006 SHA256 DLL hashes,
and attempts bounded restoration. If both originals are verified, it
removes that disposable copy. If the copy is still in use or the backup
is invalid, the utility refuses deletion and prints its recovery error.

You do NOT have to manually move any DLLs, even when a previous run
left behind a temporary test copy. The old ZIP results remain valid
research data; do not delete the reports ZIP.

NEXT TEST: ORIGINAL TWO-STAGE STATISTICS LOADING
-----------------------------------------------
The previous owner retail engine confirmed VFileSystem012 access and
the combined 62:8:156:66 Stats parse (0x53494E6B). The new diagnostic
tests the ORIGINAL per-file Open arguments and a clean-room 66-ID
catalog, and does not claim full game startup.

Double-click RUN_STAGED_STATS_TEST.bat instead of RUN_SIN_TEST.bat.
Choose your original game folder once. The package automatically
copies the game to a temporary folder, runs the two verified safety
stages, then tests:
  Stage 3: definitions Open("rb", "GAME")
  Stage 4: display layout Open("rt", null pathID)
  Independent: 66 original stat names, 62 bound definitions,
               8 percentage references, 156 layout pairs,
               66 divider lines.

It saves staged.txt and staged-run.txt in the final results ZIP.
SUCCESS: 0x53494E70, original-mode staged owned binding verified.
FAIL:    0x53494E71 (definitions), 0x53494E72 (layout),
         0x53494E73 (ABI or source counts).

No original KeyValues object, unknown game callback or complete
VClient011 Init is executed. Existing RUN_SIN_TEST.bat still runs
only the previous two filesystem phases, and all DLLs are included
in the package. There is NO manual DLL copying.

NEW: INDEPENDENT STATISTICS USER-MESSAGE HOOK PROOF
--------------------------------------------------
The actual 2026-11-20 owner results reached 0x53494E70, proving
the original 2006 engine filesystem's rb/GAME and rt/null reads and
the independent 66-ID statistics catalog. The old trace did NOT
separately identify the three new user-message hook bindings.

To test that exact next milestone, double-click RUN_STATS_HOOK_PROOF.bat
in this extracted package. Choose the ORIGINAL game root only.
No copying, DLL replacement, or setup is required.

The new terminal diagnostic reports 0x53494E74 ONLY if:
- both original stats file reads and staged 66-ID binding succeeded;
- the independent Stats HookMessage registry has exactly 3 entries;
- their original protocol IDs are StaticStat=27, DynamicStat=28 and
  ResetHUD=5, in that order;
- all 7 independent modeled steps were completed.

A refusal reports 0x53494E75. The owner report includes the exact
same-session 27:28:5 hook sequence. This proves INDEPENDENT metadata
binding only, not actual game-engine dispatch, KeyValues or playable
client initialization. RUN_SIN_TEST.bat and RUN_STAGED_STATS_TEST.bat
retain their older behavior.

The launcher automatically creates and cleans a disposable game copy,
restoring original SHA-pinned client/server DLLs before deletion.
Send the results ZIP to continue decompilation. The ZIP contains
diagnostic text only, never the original retail game binaries.
