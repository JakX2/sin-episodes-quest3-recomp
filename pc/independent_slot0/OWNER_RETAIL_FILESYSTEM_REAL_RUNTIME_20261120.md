# Owner-observed Windows 2006 Source filesystem milestone (2026-11-20)

This is a **real engine execution checkpoint**, not another synthetic
interface fixture. An owner ran the standalone Windows PE32 test from
Phase 468 against a separate authenticated SiN Episodes: Emergence
game copy and supplied the program's generated diagnostic ZIP.

The private ZIP was inspected directly, not inferred from a CI job.
Only the original-free findings below are recorded publicly. The
owner's local filesystem paths, private logs, retail DLLs and scripts
are **not** included in this repository.

## Live presence-only launch

- Retail Source engine requested the independent **VClient011** from
  independently built non-forwarding client.dll; it entered Init.
- Engine GetScreenSize, game CVar requests, original VGUI interfaces
  and the real VSoundEmitter001 Init returned as in prior checkpoints.
- The retail engine handed out **VFileSystem012**.
- The SDK-guided **FileExists** virtual call for each of the two
  scripts/stats resource paths returned **both-present**.
- Child process exit status: **0x53494E68**,
  INDEPENDENT_ENGINE_FS_BOTH_PATHS_PRESENT_DIAGNOSTIC_STOP.
- SHA256-authenticated original client DLL restoration in the disposable
  game copy: **VERIFIED_COMPLETE**.

## Live bounded read-only filesystem launch

A second, separate launch enabled the deep filesystem read probe.
The original engine successfully completed the SDK-guided:
- **Open** for the two known stats resource paths,
- **Size(FileHandle_t)** rather than the path overload,
- **Read** of whole bounded documents, and
- **Close** of the successfully opened handles.

The independently owned parser validated the original retail scripts:
**62 stat definitions, 8 percentage_of references, 156 layout pairs and
66 dividers**. Trace:

    engine-fs-presence-result: both-present
    engine-fs-read-counts: 62:8:156:66
    engine-fs-read-result: parsed-and-verified

Actual retail process exit status: **0x53494E6B**,
INDEPENDENT_ENGINE_FS_STATS_PARSED_DIAGNOSTIC_STOP.

The resulting host exit verifies both resource reads and parsed counts,
not original KeyValues construction or native C_StatsSystem::Init.
The experimental **ReadPair** callback uses Open mode "rb" and null
pathID for BOTH files. Original 2006 stats catalog code separately
uses rb/GAME for definitions and rt/null for layout. Those precise
original **staged** reader parameters have NOT yet been live tested.

## The independent restoration failure is separate

The first launch restored the SHA-matched original DLL successfully.
On the second launch, the independent diagnostic had already returned
**0x53494E6B** and the owner trace already reported full counts.
The launcher then attempted to restore the original client DLL within
its **temporary game copy**, but Windows refused immediate replacement
with **UnauthorizedAccessException**.

The SHA-verified original backup was intentionally retained. The
installed game files were not the DLL swap target. The one-click
runner's final outcome was therefore **NOT_VERIFIED_OR_SKIPPED** even
though the read methods executed successfully. Do not conflate the
failed cleanup verdict with a failed filesystem ABI/read probe.

A transient Windows file lock is plausible, but the exception type
alone does not establish which process or subsystem held the lock.
The new recovery path retries bounded post-exit restoration and
continues to refuse destruction of any unverified original backup.

## Evidence limits and next work

- **New highest authentic independent-client Windows checkpoint:
  0x53494E6B.** Prior highest 0x53494E63 remains confirmed.
- No full normal client Init return, complete server Init, game menu,
  map spawn, Android or Quest 3 boot was demonstrated.
- No real original KeyValues system or original C_StatsSystem event
  registry ABI was called from these new owner-side tests.
- The next owner-side non-playable diagnostic should verify staged
  **rb/GAME** and **rt/null** resource opening, then independently
  bind the original 66-ID catalog and layout with the live read-only
  engine filesystem. Only then address missing subsequent callbacks.
- All unrelated reported CI tests are source-free synthetic fixtures
  and cannot substitute for this actual owner runtime evidence.

Source: private owner-supplied standalone filesystem results, dated
2026-11-20. Reproduce against a fresh owner-owned disposable copy, NOT
the installed Steam game folder.
