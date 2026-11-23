# Gameplay Phase631: Original Elite virtual +0x424 reaches the weapon-state updater

## Retail 2006 original evidence

All findings come from the owner's unmodified *SiN Episodes: Emergence* original x86 `server.dll`, SHA256:

`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

The exact paired original `client.dll` SHA256 is `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Phase629 authenticated the complete gate `0x22315320` (82 bytes) and timed update method `0x22315390` (173 bytes) on a weapon-linked serial-handle target, but it did not establish **when the engine invokes that update**. Phase630 made their logic accessible to a host, but it did not claim a verified original caller.

Phase631 finds the original **Elite primary vtable offset `+0x424`**. The original table starts at `0x2248DF04` and its DWORD at **`0x2248E328`** is exactly `0x223555F0`. This is an independently authenticated **113-byte** native callback (ends immediately before `int3` padding at `0x22355661`), full original SHA256:

`daede98b8e9cdc123089f1ac3b9e284382dd5e82e062781dd4db0e8063ad6f2e`

It has two separate `call 0x22315390` edges at `0x22355645` and `0x22355654`. After either, it tail-jumps to inherited Grunt method `0x2235DF10` (jumps at `0x2235564D` and `0x2235565C`).

A nearby **distinct** complete original 232-byte Elite function at `0x223556A0`, original SHA256 `66a3abbe73c87aaa6ea5f7c9c97547b99a4c2fe6e626c6bb1f77c1a8a8c60262`, is primary vtable offset `+0x44C` (entry `0x2248E350`). It *also* calls the gate at `0x22315320` via exact `call` at `0x22355772`. That secondary handler's complete source is **authenticated, not yet ported**; the current Phase631 gameplay reference implements only the 113-byte tick caller.

### Exact source-level tick control flow

The callback's `ECX` is the Elite object; `ESI=ECX`. It does not read the engine clock itself:

1. **First** read `[Elite+0xFDC]`. If `0xFFFFFFFF`, skip weapon update and tail-jump inherited `0x2235DF10`.
2. For a non-invalid handle, capture the original global serial table at `0x224F5620`. Mask 12-bit index (`handle & 0xFFF`), scale each entry to 16 bytes, compare high serial bits (`handle >> 12`) and dereference entry object pointer. If serial mismatches or entity pointer is null, skip update and invoke inherited method.
3. After a valid **first** pointer, reread `[Elite+0xFDC]`. The handle may have changed. **Reuse the previously loaded serial table base** for the second lookup, rather than fetching another table.
4. If the second handle and serial still match, pass the **second** pointer to original `0x22315390` at `0x22355645`, then tail-jump inherited `0x2235DF10`. The first pointer is only an existence gate, not the one the updater receives.
5. If the second handle is invalid or its serial mismatches, execute `xor ECX,ECX` then **still call** original `0x22315390` at `0x22355654`, with `ECX=0` before the inherited tail jump. Because the original updater first reads `[ECX+0x594]`, this creates a possible **null-target fault** if runtime ownership changes between lookups. The original code does not provide a safe null fallback here. A second lookup may also return a zero pointer without a native check.

This explains why the already-recovered timed state machine belongs to a specific original Elite virtual callback rather than a generic periodic update. It also points to the **specific original vtable slot** requiring an engine-verified callback bridge.

## Phase631 bounded host reference and tests

`include/sin_compat/original_elite_weapon_tick_phase631.hpp` and `src/original_elite_weapon_tick_phase631.cpp` reconstruct the complete 113-byte control-flow sequence as a host-owned C++17 model. All native side effects and entity-table ownership are supplied by an `IHost`; no Source C++ classes, vtable execution or original native DLL calls are invented.

The model reproduces **two independent live handle reads**, **two serial resolutions with one table capture**, update invocation only on the *second* resolved target, and inherited Grunt delegation. The model deliberately **fails closed** with `OriginalNullTargetHazard` when the second target disappears instead of calling original `0x22315390` on `ECX=0`. This safer behavior is **not** attributed to the retail game. Callback failures preserve the original stage and do not pretend to roll back engine effects.

`tests/test_original_elite_weapon_tick_phase631.cpp` checks:
- two different valid handles and two different resolved objects;
- first missing, stale, or zero native target -> inherited-only path;
- second missing, stale, or zero target -> original null fault hazard, no invalid call;
- exact original update-then-inherited call sequence;
- injected host failure, exception, and reentrancy guard.

`research/tools/audit_original_elite_weapon_tick_phase631.py` requires both exact original DLL SHA256s. It authenticates **both complete** additional original method SHA256s, original Elite primary vtable offsets `+0x424` and `+0x44C`, two updater calls, two inherited tail-jumps, secondary gate callsite, and 24 isolated source instruction witnesses. Its public `--self-test` only mutates fabricated bytes and never ships game code.

### Owner original audit

```powershell
python research\tools\audit_original_elite_weapon_tick_phase631.py `
  --server "D:\SiN-Original\server.dll" `
  --client "D:\SiN-Original\client.dll"
```

## Phase631 new real-game callback observation profile

The existing Phase627/628 **owner-only 32-bit Windows hardware tracer** gains `--watch-profile weapon` / wrapper `-WatchProfile Weapon`. It uses the exact existing per-thread DR0–DR3 safe ownership/rollback, retail SHA256 verification and live instruction-prefix checks, but watches these four original function entries **together in the actual game process**:

| DR slot | Original 2006 callback | Original VA | Module RVA |
| --- | --- | --- | --- |
| DR0 | Elite Spawn | `0x22355820` | `0x355820` |
| DR1 | **Elite vtable +0x424 tick** | `0x223555F0` | `0x3555F0` |
| DR2 | weapon-linked 0x43 gate | `0x22315320` | `0x315320` |
| DR3 | **weapon-linked timed updater** | `0x22315390` | `0x315390` |

The original profile remains the default (`-WatchProfile Elite`) and still watches factory-create, Spawn, animation event and factory release. Because x86 hardware provides only four execution watch slots, a single process run **cannot** watch all eight original entrypoints simultaneously.

When watching weapons, the log records **separate anonymized process-local ordinal namespaces** for original Elite `ECX` (Spawn/tick) and resolved native target `ECX` (gate/update). These are observation identities only; neither reveals game-memory pointer values nor proves a durable Source serial/generation ID. JSONL records also include exactly one `watch_profile_weapon` or `watch_profile_elite` marker, checked by the PowerShell runner against the requested mode, plus original DLL SHA, real PID and complete instrumentation checks.

A real game run is owner-only: the original executable and DLLs are not downloaded, executed or redistributed by public GitHub CI.

### Owner Windows example

Build `sin_pc_owner_elite_hwtrace_phase627` from this stacked branch as in Phase627/628, and use a **disposable offline original game copy**:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\pc\windows_x86_proxy\Invoke-OwnerEliteHardwareTrace.ps1 `
  -GameExecutable "D:\SiN-DisposableTest\SinEpisodes.exe" `
  -GameBin "D:\SiN-DisposableTest\SE1\bin" `
  -TraceExecutable ".\build-owner-win32\bin\Release\sin_pc_owner_elite_hwtrace_phase627.exe" `
  -OutputTrace "D:\SiN-DisposableTest\elite-weapon-phase631.jsonl" `
  -DisposableTestCopyConfirmed -WatchProfile Weapon -TimeoutSeconds 180
```

Load a map containing `npc_druglab_grunt_elite`, trigger a weapon-related animation, then review the `ORIGINAL_ENGINE_ELITE_TICK`, `ORIGINAL_WEAPON_GATE_ENTRY` and `ORIGINAL_WEAPON_TIMED_UPDATE` observation statuses. **Zero callbacks in a completed, authenticated interval means not observed**, not unreachable or unimplemented.

The next integration milestone is to use these genuine original-game observation counts, along with a host-confirmed engine entity lifecycle, to connect the original Elite virtual `+0x424` callback to Phase630's native-source gate/update adapter. The project still has **no proven original engine callback into reconstructed game code**, no complete playable replacement game DLL and no in-game Android/Quest boot. No physics, surfaces, models, props or rendering work was changed.
