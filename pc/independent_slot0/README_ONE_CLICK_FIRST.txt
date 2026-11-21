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

NEW OPTIONAL SOUNDSCAPE CATALOG READ TEST (2026-11-20)
-----------------------------------------------------
The package now contains RUN_SOUNDSCAPE_CATALOG_TEST.bat.
This runs the SAME protected presence/read/staged-stats/hook path,
then uses the original 2006 engine VFileSystem012 with rb/GAME
to load the game's soundscape manifest and all seven referenced
scripts into independently owned bounded parser objects.

Choose your original installed game directory through the folder
picker. This does NOT replace or edit any DLL in your installed game.
A separate SHA-verified TEMP game copy is used and restored.

EXPECTED NEW RETAIL CODE (not yet owner-verified): 0x53494E76.
Its independent catalog must contain 7 original text files,
301 soundscapes, 242 playsoundscape references and 3 unresolved
references within those files. Refusal uses 0x53494E77.

This is NOT normal VClient011::Init, real soundscape playback,
playable Windows build, Android/Quest 3 runtime or game level load.
Only the old 0x53494E74 checkpoint has been observed in retail so far.
The user-facing results ZIP contains TEXT ONLY. For detailed scope,
see OWNER_SOUNDSCAPE_RETAIL_GATE_20261120.md.

Dry run, without touching any installed game:
powershell -NoProfile -STA -ExecutionPolicy Bypass -File .\Start-SiNFileSystemTest.ps1 -GameRoot "C:\synthetic-no-game" -StagedStats -HookProof -SoundscapeCatalog -PlanOnly

PHASE 493: IF THE SOUNDSCAPE STAGE REFUSES (2026-11-20)
-------------------------------------------------------
Run RUN_SOUNDSCAPE_CATALOG_TEST.bat from the complete extracted
Windows GitHub Actions diagnostic artifact, OUTSIDE the installed
SiN Episodes game. Select your SHA-authenticated original game
folder when prompted. The tool uses a disposable TEMP copy.

The new Phase 493 diagnostic writes a single bounded failure stage
to SUMMARY.txt in the generated SiN_FileSystem_Test_Results_*.zip
when the owner engine reaches the optional soundscape probe:

  Soundscape read/parse stage: resolve-abi / read-manifest /
  validate-manifest / read-script / script-budget / build-catalog /
  complete

It also records how many of the seven source script reads were
attempted and succeeded. Details are available as
OWNER_CLIENT_SOUNDSCAPE_DETAILS in staged.txt and the raw
same-session sin-independent-*.log files.

If you see 0x53494E77, send the small results ZIP. There is no
need to guess which script failed or manually change the original
installation. If it stops at 0x53494E74 or an earlier code, the new
soundscape stage was not reached; send the ZIP to identify the
earlier gate. Do not delete a retained TEMP copy while the
restoration guard reports uncertainty.

0x53494E76 is a diagnostic success for file reading and the owned
catalog, NOT successful game startup or actual audio playback.
At the time this package was authored, that new code had not yet
been observed in a real owner-run retail process. Original complete
VClient011::Init remains incomplete.

PHASE 497: OPTIONAL REAL-ENGINE READ-ONLY CONVAR LOOKUP
------------------------------------------------------
Use the updated Phase 497 standalone Windows artifact. Run the normal
RUN_SOUNDSCAPE_CATALOG_TEST.bat to perform ONLY the existing 7:301:242:3
catalog diagnostic. Phase 497 adds an OPTIONAL additional test:

  RUN_SOUNDSCAPE_CATALOG_TEST.bat -SoundscapeCvarLookup

This retains the entire existing chain of SHA-pinned original game
validation, disposable TEMP copy, presence/read/Stats/hook proof,
automatic restoration and small text-only ZIP output.

The extra opt-in step is executed only after the soundscape catalog
diagnostic returns and before its existing terminal exit code:
  - Original 2006 C_SoundscapeSystem virtual slot3, method 0x240F3C40.
  - ICvar::FindVar("snd_soundmixer") via VEngineCvar002 virtual slot3.
  - ICvar::FindVar("dsp_volume") via the same virtual slot.
  - Cache references but NEVER dereference or modify ConVar values.
  - Never register console variables, change sound settings, play
    audio or continue normal VClient011::Init.

The optional report line in SUMMARY.txt is:
  Original soundscape ConVar lookup (read-only): <status>:<requested>:<found>:<mixer-present>:<dsp-present>

Valid statuses identify both found, one found, none found or an
ABI/callback refusal. Missing ConVars are a NORMAL source-permitted
outcome and do not indicate game init failure. The primary soundscape
0x53494E76/77 exit code and original 7:301:242:3 catalog verdict
remain identical with this extra lookup. No pointer address, variable
value or proprietary original file text is printed.

If the process exits before the last stage, the extra summary is
NOT_RECORDED, not a successful lookup. Send the small ZIP as usual;
do not modify the installed original game or stage DLLs manually.
This is diagnostics, NOT normal game boot. The last owner-observed
retail checkpoint before this package was 0x53494E74.

PHASE 513: NEXT ORIGINAL ENGINE CLIENT PHYSICS FACTORY DIAGNOSTIC
---------------------------------------------------------------
Your previous owner-run soundscape test passed! The real engine exited
0x53494E76, and both snd_soundmixer and dsp_volume were found using
the original CVar ABI. You do not need to repeat the older test.

For the NEXT read-only experiment on an updated, CI-verified package,
open Windows cmd in the extracted package directory and run:

  RUN_SOUNDSCAPE_CATALOG_TEST.bat -SoundscapeCvarLookup -PhysicsFactoryLookup

The new -PhysicsFactoryLookup flag tests the original 2006 client.dll
97-byte mandatory helper's THREE engine physics interface name queries
through VClient011::Init's SECOND factory argument (physicsFactory):
  1. VPhysics030
  2. VPhysicsSurfaceProps001
  3. VPhysicsCollision007

The independent DLL does not copy, dereference or retain any returned
engine interface pointer. It does not call the subsequent original
physics coupling helper, does not set CVar values, does not return
successful VClient011 Init and does not run any map.

The one-click launcher still checks the untouched SHA-matched original
client/server pair, creates an isolated disposable game copy, runs
presence/read/staged-stat/owned-hook/soundscape/CVar prerequisites
and restores the verified originals. No manual DLL replacement.

In the generated small text-only results ZIP, SUMMARY.txt now includes:

  Original physics Factory triple (read-only):
    original-physics-three-available:3:3:1
    or a specific original-physics-missing-... status,
    or NOT_RECORDED when earlier gates did not complete.

The normal soundscape 0x53494E76/77 exit codes remain unchanged.
A valid new physics result must also be in the same ordered client
session after the completed original read-only CVar lookup.
Please send the small SiN_FileSystem_Test_Results_*.zip as before.
No game files, commercial DLL bytes or private machine paths need to
be uploaded. This still does not prove a playable Windows or VR boot.

PHASE 514: ORIGINAL PHYSICS SURFACEPROPERTIES FILE READ (READ-ONLY)
------------------------------------------------------------------
The owner already completed our original source soundscape/CVar
diagnostic successfully (Windows 0x53494E76; both ConVars found).
The next combined original Source2006 physics and resource check is:

  RUN_SOUNDSCAPE_CATALOG_TEST.bat -SoundscapeCvarLookup -PhysicsFactoryLookup -SurfacePropsRead

The new -SurfacePropsRead opt-in requires the exact three original
physics Factory interfaces to have been found:
  VPhysics030, VPhysicsSurfaceProps001, VPhysicsCollision007.
After that, it uses the already owner-verified VFileSystem012
interface to read:
  scripts/surfaceproperties_manifest.txt ("rb" / "GAME")
  scripts/surfaceproperties.txt          ("rb" / null pathID)
  scripts/surfaceproperties_hl2.txt      ("rb" / null pathID)

The independent parser requires source-original retail shape:
two script files, 93 unique material surface names, 71 "base" keys.
It never calls original VPhysicsSurfaceProps001::ParseSurfaceData
or touches sound/audio, collision objects, materials or gameplay.

The original soundscape terminal codes 0x53494E76/77 still apply.
A new summary line independently shows whether all three physics
interfaces were found and if the subsequent surfaceproperties
resource shape was valid. The test does NOT claim real game boot.

As with prior diagnostics, extract all files outside the installed
game, launch from that extracted folder, select the genuine installed
game root, and upload the generated small text-only results ZIP.
No manual DLL copying or retail file changes. The disposable-copy
launcher verifies original SHA256 fingerprints and restoration.

PHASE 515: PHYSICS SURFACE REGISTRATION (EXPLICIT ENGINE MUTATION)
-----------------------------------------------------------------
Owner Windows retail evidence as of 2026-11-21:
  Original physics Factory 3/3: VERIFIED
  Original surfaceproperties resource list/shape 2:2:93:71:0: VERIFIED
  Original ParseSurfaceData virtual method invoked: NOT YET OBSERVED
  Full original VClient011/ServerGameDLL003 Init: NOT COMPLETED

After the Phase 515 Windows CI and package are verified, a NEW,
separately opt-in experiment (never implicitly enabled by a read-only
test) can exercise VPhysicsSurfaceProps001 virtual slot1:

  RUN_SOUNDSCAPE_CATALOG_TEST.bat -SoundscapeCvarLookup -PhysicsFactoryLookup -SurfacePropsRead -SurfacePropsRegister

This EXPERIMENT writes to original engine physics-surface state inside
the disposable test process, unlike Phases 513 and 514. The original
engine might reject an ABI assumption or crash. Do not attempt it
against an installed game directory. The one-click launcher still
authenticates original DLL SHA256, copies to an isolated temporary
folder and verifies restore/cleanup before reporting results.

The expected read-only 93/71 resource stage must pass before any native
parse method is called. Two returned ParseSurfaceData calls mean only
that both virtual invocations returned, not that physics data is
registered or a game map can be loaded. The standard diagnostic
termination status 0x53494E76 is not changed. See
OWNER_SURFACEPROPS_REGISTRATION_20261121.md for full provenance and
remaining engine ABI limitations.
