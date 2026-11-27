# Phase 1649: Bounded original-engine edict record observation, unpinned

**Stack:** Phase 1648 PR #1657, final SHA `beacf07ed5010304657a54bf9f81c0a7ab7819ab`, all six CI families green and Windows MSVC x86 CTest 247/247. No retail process observation yet.

## Original 2006 PE32 evidence reused

The independently authenticated original SiN Episodes: Emergence engine.dll SHA-256 is `f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`. Existing read-only SHA-window audits `audit_sin_original_engine_gameents_edict_retire.py` and `audit_sin_original_engine_gameents_edict_reuse.py` establish the original engine-owned edict stride `0x14` and exact offsets:

| Per-edict offset | Meaning | Read type |
| --- | --- | --- |
| `+0x00` | flags including original active bit `0x02` | uint32 |
| `+0x04` | original retail per-edict serial increment / retirement sentinel | uint32 |
| `+0x10` | x87 edict reuse timestamp | unmodified IEEE-754 float bits, uint32 |

`+0x08` networkable and `+0x0C` IServerUnknown are **not read** by this observer. It does not dereference entity pointers or access other object memory. The original retail 32-bit serial is not a trusted 64-bit lifetime epoch.

## Owner-only command

The new `research/tools/observe_sin_original_engine_edict_records.py` imports the Phase 1648 `Win32Reader` read/query-only process handle, strict unmodified `SinEpisodes.exe` executable name, game-root/`bin/engine.dll` layout and exact SHA-256 gate. Live mode requires an explicit original game PID and `--owner-read-only-confirmed`. The user must choose 1-4 distinct zero-based edict indices and a bounded sample count/interval. There is NO automatic broad entity scan.

Example, after the owner has launched the unmodified original game on Windows:

```powershell
$game = Get-Process -Name SinEpisodes | Select-Object -First 1
py -3 research/tools/observe_sin_original_engine_edict_records.py `
  --pid $game.Id --owner-read-only-confirmed --edict-index 2 `
  --samples 300 --interval-ms 100
```

This command samples for approximately 30 seconds; each sample performs two independent reads of the **four** allocator globals, followed by two reads of the **three** requested edict DWORD fields, then another allocator descriptor read (the code performs first descriptor, two field passes, final descriptor). A sample is accepted as `double_checked_unpinned` only when both global descriptors are plausible and exactly equal, all requested indices are inside the reported *used* count and capacity, all addresses fit PE32 arithmetic, and each record's two field triples match. Any torn, moved or unreadable sample is marked inconclusive and cannot contribute valid record fields.

Printed observations include flags, original uint32 serial and timestamp bit pattern only. The tool records the number of comparable adjacent observations where the same original edict base pointer and capacity persisted and where flags, serial, or time bits differed. They are **observed differences**, not proof of exactly one allocation or free, and not proof of a continuous physical pin. Even two identical readbacks cannot exclude ABA transitions between reads. No original DLL bytes are exported, no retail factory is installed and there are no process writes, hooks, remote threads, debug ports or telemetry.

## CI, negative tests and scope

`--self-test` constructs only fake global DWORDs and fake 20-byte edict records. It verifies source-backed offsets, two-pass record reads, no access to the `+0x08/+0x0C` pointer fields, invalid/duplicate/unallocated indices, count-capacity inconsistency, whole PE32 extent overflow, relocated global bases, unreadable fields, a changed owner pointer between descriptor reads, a serial changing between record scans, and adjacency resets on missing samples or a relocated edict array. It requires no game image or Windows; CI runs it on Linux synthetic decomp and native Windows MSVC x86 CTest. One additional CTest case is expected, bringing the native total from 247 to 248.

## Remaining original-game integration gaps

A user-run retail Windows sample would be useful evidence for per-edict fields and serial changes, not for physical ownership or synchronization. The original engine may mutate records between **any** of the observer's individual ReadProcessMemory calls. SHA-verifying the loaded module's on-disk file does not attest unchanged in-memory code. A continuous retail allocator pin with genuine entity/touch backing and safe `ServerGameEnts001` registration remains unverified. No playable Windows, Android or Quest 3 build is claimed.

Continue to exclude generic HL2-equivalent physics, surfaces, props and model loading per the user's explicit priority override.
