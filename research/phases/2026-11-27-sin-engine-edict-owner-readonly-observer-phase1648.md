# Phase 1648: Owner-only read-only original engine edict allocator observation

**Stack:** Phase 1647 PR #1656, synthetic Windows Win32 `VirtualAlloc`-owned exclusive pin contract. This phase gathers observations from an **actual, separately launched original retail game process**; it does not install an allocation hook or pin original engine memory.

## Source-backed offset provenance

The independently authenticated original SiN Episodes: Emergence engine.dll SHA-256 is `f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`, original image base `0x20000000`. The existing `audit_sin_original_engine_gameents_edict_reuse.py` instruction windows prove four separate edict allocator globals in the original negative-index selector:

| Original VA | Meaning | Read type |
| --- | --- | --- |
| `0x2079DA64` | protected-last edict index | uint32 |
| `0x2079DAFC` | current used edict count | uint32 |
| `0x2079DB00` | edict capacity | uint32 |
| `0x2079DB04` | edict array base pointer | PE32 pointer DWORD |

The new observer uses actual module base + (original VA - original image base) and handles a relocated PE32 image. It does NOT assume `engine.dll` always loads at `0x20000000`.

## Read-only owner tool

`research/tools/observe_sin_original_engine_edict_owner.py` uses only Python standard library `ctypes` and `hashlib`. Its Windows live mode:

- Requires explicit original-game PID and `--owner-read-only-confirmed`; rejects all other processes by the executable name `SinEpisodes.exe`.
- Locates loaded `engine.dll` via Windows Toolhelp module enumeration, requires the root/bin installation layout and confirms the **exact original SHA-256 of the loaded module's on-disk file** before observing process globals.
- Opens the game only with `PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION`. There are NO write, patch, remote-thread, injection, debug, module-loading or original-dll replacement operations.
- Reads only the four 32-bit original allocator globals with `ReadProcessMemory`, one DWORD at a time. Does not read edict, entity, touch or game code memory. Rejects overflow and bounds-inconsistent snapshots; classifies startup/teardown and torn snapshots as uncertain.
- Samples at bounded frequency (1..2000 samples, interval 20..2000 ms) and counts *only adjacent plausible observations* where the base pointer changed. Gaps reset the observed move comparison. It never asserts no relocation between samples, nor does it call a repeated base pointer a stable lifetime pin.
- Hides edict pointer values by default; `--show-pointers` is an explicit opt-in for owner-local detailed logs. No telemetry or original game bytes are uploaded.

Owner-only usage after manually starting the **original unmodified game** on Windows:

```powershell
$game = Get-Process -Name SinEpisodes -ErrorAction Stop | Select-Object -First 1
py -3 research/tools/observe_sin_original_engine_edict_owner.py `
  --pid $game.Id --owner-read-only-confirmed --samples 300 --interval-ms 100
```

This reports up to 30 seconds of read-only observations. If the engine is not loaded, the process is inaccessible, the executable/module paths are unexpected or the SHA differs, the tool refuses instead of guessing. If the game exits mid-sample, it reports unreadable samples. No script is run automatically against the owner's machine.

## Synthetic regression and verification

`--self-test` is portable and original-free: it tests exact SHA-backed VAs, image-base relocation, synthetic four-DWORD snapshots, absent/torn reads, zero/partial states, capacity/count inconsistencies, arithmetic overflow, and adjacent-only base-change accounting. On Linux it makes no Win32 calls. On Windows the same self-test additionally opens **only its own test process** with `PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION`, reads one local four-byte test value with `ReadProcessMemory`, and closes the handle. It never opens the game in CI. The regression is wired to both Windows MSVC native CTest and the Linux original-free decomp GitHub Actions workflow. The new synthetic test adds one more CTest case beyond Phase 1647.

## Limits

This can produce **empirical snapshots of the real original retail engine** once the owner explicitly runs it. It cannot alone demonstrate how the retail allocator synchronizes threads, whether FreeContainingEntity callbacks preserve backing continuously, whether an edict record is live between two samples, or whether the unmodified engine can accept a new host pin protocol. SHA of an on-disk DLL is not an attestation of unchanged in-memory executable pages. No results of a retail process observation are claimed here. The original retail ServerGameEnts001 factory remains disabled in the independent DLLs, and native playable Windows, Android or Quest 3 startup remain unverified.

Generic HL2-equivalent physics, surfaces, props and model loading stay out of scope under the existing priority override.
