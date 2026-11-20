# Phase 486: owner-only retail soundscape catalog read milestone

## What is genuinely new

The independent Windows x86 client.dll now contains an **additional
strictly opt-in terminal diagnostic**, placed **AFTER** the authentic
owner-retail Stats file/hook read path that previously reached
0x53494E74. It does not modify the normal default Init path.

If all prior checks succeed and the special option is present, it
invokes the independently authored Phase 484 VFileSystem012 read-only
soundscape bridge with the game engine's already-acquired
VFileSystem012 pointer. It reads the real game's
scripts/soundscapes_manifest.txt and its seven referenced text files
using the original KeyValues LoadFromFile mode **rb** and pathID
**GAME**. It parses the manifest and nested soundscapes into owned
C++ objects, with no use of proprietary KeyValues runtime functions.

A source-derived exact count gate requires:

- 7 script files.
- 301 named soundscapes.
- 242 nested playsoundscape references.
- 3 references unresolved within those seven original script files.
- Original manifest root spelling: soundscaples_manifest.

This count is grounded in independent SHA-authenticated private read
of the owner's genuine scripts.zip, NOT synthesized from a guess.

The diagnostic writes only short session-isolated statuses and counts.
It never writes game files, prints individual soundscape contents or
plays original audio.

## Distinct exit codes and evidence

- **0x53494E76**: previous independent Stats hook gate passed, then
  actual engine filesystem returned the original seven soundscape
  files, and the independently owned 7:301:242:3 catalog verified.
- **0x53494E77**: the read, parser or original-count check refused;
  no game-init success is claimed.

These exits are **newly implemented possible outcomes** until the
owner actually runs the retail game test. The last VERIFIED
owner-retail-engine observation is still **0x53494E74**.

A successful new owner run also requires the exact same-session
report line:

OWNER_CLIENT_SOUNDSCAPE_CATALOG_RESULTS: sin-independent-<session>-<pid>.log:client:owned-soundscape-catalog-verified-original-files:7:301:242:3

The strict verdict is
CLIENT_SOUNDSCAPE_CATALOG_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF.

## One-click workflow without modifying the original game

The Windows CI diagnostic bundle adds:

RUN_SOUNDSCAPE_CATALOG_TEST.bat

Extract the complete CI artifact somewhere **outside** the live
SiN Episodes installation. Double-click the batch file, choose the
ORIGINAL SinEpisodes.exe game directory (the launcher requires exact
SHA-256 original client and server DLLs), then inspect the produced
text-only SiN_FileSystem_Test_Results_*.zip.

The standalone runner stages a fresh disposable Windows TEMP copy,
checks original pair hashes, runs independent client/server DLLs there,
performs presence and read phases first, followed by the owned Stats
hook proof and soundscape read; it then restores and re-verifies
the original DLLs. If restoration is uncertain, it retains the TEMP
copy for safe manual/automatic recovery instead of deleting it.

For a command-line dry run (no retail game copy or launch):

powershell -NoProfile -STA -ExecutionPolicy Bypass -File .\Start-SiNFileSystemTest.ps1 -GameRoot "C:\synthetic-no-game" -StagedStats -HookProof -SoundscapeCatalog -PlanOnly

The corresponding underlying owner filesystem gate accepts only
Phase=Soundscape and a fully restored prior Read transcript, itself
requiring a genuine earlier Presence transcript. It prevents direct
accidental activation without earlier security gates. The option is
explicitly session-bound and restored after the launch.

## Limits

This DOES NOT implement the real original C_SoundscapeSystem object,
register triggers or DSP, play WAVs, mix audio, create renderer/physics
systems, or reach a normal VClient011::Init return-one branch. It
does not claim game menus, gameplay, Android port or Quest 3 build.

Public GitHub Actions validates with original-free Windows PE32
fake engine/filesystem objects, fabricated full 7/301/242/3
script resources, malformed/absent source refusals, PowerShell
dry-run and session-validation guards. The original owner binary
and original soundscape scripts are never uploaded to GitHub.
