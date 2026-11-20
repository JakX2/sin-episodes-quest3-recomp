# Phase 493: owner-retail soundscape refusal diagnostics

## Why this stage matters

The last genuine owner-retail Windows checkpoint is 0x53494E74.
Phase 486 prepared a new read-only soundscape diagnostic with potential
exit 0x53494E76 for a verified original 7:301:242:3 catalog, or
0x53494E77 for a refusal. Neither new code has been observed yet in
a real owner-side game run.

Previously, missing engine source files, bad manifest text, bad nested
soundscape structure, low-level read failures and invalid filesystem
ABI all reported one broad failure. This phase adds a bounded reason
so a single user test can identify the first stopped boundary.

## New trace event and exact format

The independent client emits a new optional event, named
engine-fs-soundscape-detail, strictly after the original read/result
probe and before its terminal process exit.

Its version is:

    stage:bridge-result:manifest-result:catalog-result:reads-attempted:reads-succeeded

All strings are fixed independent enum names; counters are bounded
unsigned integers. No original game script, asset name, file path,
machine absolute path, binary data or object address is serialized.
The formatter rejects output truncation instead of printing a
partial diagnostic.

| Stage | What the rebuilt code has established |
| --- | --- |
| resolve-abi | Interface/method ABI checks refused before calls |
| read-manifest | Original-style rb/GAME manifest file read refused |
| validate-manifest | Independent manifest syntax/path check refused |
| read-script | Referenced manifest file engine read failed |
| script-budget | Independent per-file or total staging cap refused |
| build-catalog | All resource reads succeeded, but nested KeyValues-like catalog parse refused |
| complete | All staged reads and owned parsing succeeded |

The first unsuccessful script attempt is visible by count:
reads-attempted includes the failed request; reads-succeeded does not.
The Source filesystem helper uses a 256 KiB raw-resource cap, which
is narrower than the manifest owner's 512 KiB source limit. An engine
read refusal can therefore mean missing resource, bad Size/Read or
size policy; its stage must NOT be misinterpreted as a confirmed
missing retail file.

Complete plus 0x53494E77 means the independently parsed catalog's
values (root or 7 files, 301 named definitions, 242 nested references,
3 unresolved names) differed from the privately SHA-authenticated
original resources, even though all files were successfully loaded.

## Session-proof and safety boundary

The existing same-session classifier outputs
OWNER_CLIENT_SOUNDSCAPE_DETAILS only after the earlier fully
verified 27:28:5 Stats hook prefix and an ordered soundscape
entered/counts/result/detail/terminal-stop sequence. Duplicate,
premature, malformed, path-like and unrelated-session detail records
are not promoted into the summary. Missing new details are tolerated
for compatibility with older saved traces, and the optional new field
cannot change the original success verdict.

The owner-only RUN_SOUNDSCAPE_CATALOG_TEST.bat remains the packaged
entrypoint. It uses an original SHA-matched installation to produce
a separate disposable game copy with full restoration checks.
Original client/server DLLs are never uploaded to CI or modified in
the installed game. A small text-only result ZIP contains same-session
trace logs, including the new failure detail when present.

## Implemented and exercised

- client_soundscape_engine_fs_bridge.h: stage and read counters.
- client_soundscape_retail_gate.h: carries diagnostic independently
  of normal success/refusal status.
- client_soundscape_diagnostic_formatter.h: fixed-size path-free
  serialization, no user strings.
- slot0.cpp: optional new engine-fs-soundscape-detail event only
  in the existing terminal soundscape probe.
- test_client_soundscape_engine_fs_bridge.cpp: fake engine manifest,
  missing file, malformed catalog, short/oversized read and retry.
- test_client_soundscape_retail_gate.cpp: original-count synthetic
  7/301/242/3 positive fixture and independent failure stages.
- test_client_soundscape_diagnostic_formatter.cpp: exact safe output,
  privacy alphabet, invalid counters and truncation refusals.
- Test-OwnerIndependentStartupVerdict.ps1: same-session detail
  extraction and forged/duplicate record rejection.

## Explicit non-goals

The new field proves no game boot or engine sound playback. Normal
independent VClient011::Init still returns failure. The previous
observed owner checkpoint is 0x53494E74; 0x53494E76/77 have not been
seen in a genuine owner-side retail execution. The Android NDK
smoke checks do not represent playable Android or Quest 3 builds.
