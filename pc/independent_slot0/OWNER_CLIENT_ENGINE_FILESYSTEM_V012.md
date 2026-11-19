# SDK 2004 VFileSystem012 Win32 read-only engine experiment

Stacked on PR #449. The historical Source SDK 2004 reference
Source-SDK-Archives/source-sdk-2004, src_mod/public/filesystem.h,
blob SHA b01864b6b8aae8b300079c663fa4bb192d67debe declares
VFileSystem012 with IBaseFileSystem primary methods:
Read=0, Open=2, Close=3, Size(handle)=7, FileExists=10.
SiN's original client Init requests VFileSystem012 (21 original factory queries).
These are SDK-derived slots, **not yet owner-retail-verified** for SiN.

Two opt-in stages after the previously verified real sound-system Init:

1. Presence only: launcher switch -EnableClientEngineFsProbe sets
   SIN_INDEPENDENT_CLIENT_ENGINE_FS_PROBE=1; calls primary FileExists
   on the two paths scripts/stats/stats_definitions.txt and
   scripts/stats/stats_display_layout.txt with pathID=nullptr.
   Engine search mounts (including packages) may resolve these paths.
   Exit 0x53494E68=both found, 0x53494E69=missing, 0x53494E6A=invalid ABI.

2. Bounded reads: -EnableClientEngineFsReadProbe also sets
   SIN_INDEPENDENT_CLIENT_ENGINE_FS_READ_PROBE=1, requiring presence flag.
   Calls Open(rb), Size(handle), Read chunks, Close on all paths.
   256 KiB limit per file, read-only, counts validated with the independent
   scripts parser. Exit 0x53494E6B=original counts, 0x53494E6C=refused,
   0x53494E6D=valid syntax with unexpected counts.

Read stage is independently gated. First trial should test ONLY presence;
after verifying it against owner retail, test reads separately.
No loose-file content root is required for the engine variant.
No original commercial files or binary bytes are committed.

Native fixture verifies MSVC x86 method slots with a synthetic engine
object, missing paths, partial reads, size bounds, close-on-refusal,
count mismatch and retry. It is NOT a retail engine execution.
Use only an existing disposable game copy and the reversible owner launcher;
do not stage the independent DLLs into your Steam installation.
Original C_StatsSystem helper, engine resource list and event callbacks
remain unresolved; no normal VClient011::Init success or map load.

The MSVC x86 synthetic fixture showed that the two overloaded Size
methods occupy slots 6/7 in the opposite order from their source
listing: slot6 dispatched Size(path,pathID), while slot7 is the
candidate for Size(handle). This compiler-ABI distinction is still
only synthetic evidence until the owner tests the 2006 engine.

## Example reversible owner launch

Use the same disposable game-copy prerequisites as the previous
owner-verified sound probe. Check that the selected independent build
directory contains client.dll and the disposable copy retains matching
original SHA-pinned client.dll/server.dll before running.

```powershell
$game = 'D:\SiN-DisposableTest'  # example disposable copy only
$probe = '.\Invoke-OwnerIndependentEngineProbe.ps1'
$independent = '<directory-containing-new-client.dll>'
& $probe -GameBin (Join-Path $game 'SE1\bin') `
  -GameExecutable (Join-Path $game 'SinEpisodes.exe') `
  -IndependentBin $independent -ProbeRole Client `
  -EnableClientSoundProbe -EnableClientMaterialProbe `
  -EnableClientScreenProbe -EnableClientGaussianProbe `
  -EnableClientCvarProbe -EnableClientOwnedStartupProbe `
  -EnableClientVGuiProbe -EnableClientVGuiLayoutProbe `
  -EnableClientEngineVGuiSlot1Probe `
  -EnableClientVGuiControlsBootstrapProbe `
  -EnableClientMatSurfaceProbe -EnableClientSoundSystemInitProbe `
  -EnableClientEngineFsProbe -DisposableTestCopyConfirmed
```

The first test only checks FileExists via engine mounts. Only after
its trace and 0x53494E68 terminal exit are confirmed should the
separate -EnableClientEngineFsReadProbe switch be added for a second
run. Each command is a deliberate terminal diagnostic, not a game boot.
Check OWNER_INDEPENDENT_RESTORATION: VERIFIED_COMPLETE after each launch;
do not rerun if recovery is incomplete.

The SDK-specific ABI ordering is synthetic MSVC x86 evidence; the
retail-engine result remains unknown until the owner conducts this test.

## Two-stage owner retail launch helper (Phase 467)

A new one-command reversible helper selects all prior *client* startup
gates through the owner-verified sound-system Init and then checks the
original 2006 engine filesystem. This requires a fully disposable
owner-owned Windows game installation and the latest independent PE32
client from the CI diagnostic bundle; no public CI executes this helper
in real launch mode.

The disposable root MUST contain SinEpisodes.exe, SE1/GameInfo.txt,
both untouched original SHA-pinned DLLs in SE1/bin, and the explicit
SIN_INDEPENDENT_DISPOSABLE_TEST_COPY.txt marker containing exactly
SIN_INDEPENDENT_DISPOSABLE_TEST_COPY_V1. Do not use the Steam game
directory or a linked/junction-mounted copy. The lower-level launcher
enforces these prerequisites and restores the game DLL after each run.

First, presence ONLY (one terminal diagnostic process):

    .\Invoke-OwnerFileSystemRetailGate.ps1 `
      -DisposableGameRoot 'D:\SiN-DisposableTest' `
      -IndependentBin 'D:\SiN-IndependentBundle\bin' `
      -Phase Presence `
      -TranscriptPath 'D:\SiN-ProbeReports\presence.txt' `
      -DisposableTestCopyConfirmed

The owner helper must report
OWNER_ENGINE_FS_GATE_RESULT: PRESENCE_DIAGNOSTIC_ONLY_NO_GAME_BOOT
and save the full original launcher transcript. It requires:
retail exit 0x53494E68, BOTH mounted source paths found, matching
session-local client events, and restored original DLLs.

ONLY THEN, on a second run with a NEW transcript file:

    .\Invoke-OwnerFileSystemRetailGate.ps1 `
      -DisposableGameRoot 'D:\SiN-DisposableTest' `
      -IndependentBin 'D:\SiN-IndependentBundle\bin' `
      -Phase Read `
      -PriorPresenceTranscript 'D:\SiN-ProbeReports\presence.txt' `
      -TranscriptPath 'D:\SiN-ProbeReports\read.txt' `
      -DisposableTestCopyConfirmed

Read-phase preflight refuses a missing, mismatched, cross-session or
restoration-incomplete previous presence report. A read-phase terminal
exit of 0x53494E6B must also contain the exact parsed count record
62:8:156:66. The helper will not label mismatched or missing evidence
as success. A real original Windows engine process remains the sole
source of authentic runtime evidence.

The read-stage gate is a sequencing convenience, not cryptographic
attestation of a transcript's provenance. Reports are local owner data
and must NOT be uploaded automatically to public GitHub Actions.
If the launcher reports INCOMPLETE_MANUAL_RECOVERY_REQUIRED, stop and
restore the original DLL safely before retrying.

For synthetic flag inspection without touching game files:

    .\Invoke-OwnerFileSystemRetailGate.ps1 `
      -DisposableGameRoot 'D:\SiN-DisposableTest' `
      -IndependentBin 'D:\SiN-IndependentBundle\bin' -PlanOnly

The public CTest named independent_owner_filesystem_gate_synthetic
uses fake paths/transcripts in PlanOnly mode; it never starts the
commercial executable or substitutes its original modules.
