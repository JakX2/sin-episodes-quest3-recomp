# Gameplay Phase632: Recover second original Elite animation handler (+0x44C)

## Source identity and why this is distinct

The owner's unchanged original 2006 **SiN Episodes: Emergence** PE32 `server.dll` is the sole source; full SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. The companion original `client.dll` hash is `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Phase631 authenticated but did **not** reconstruct a second, distinct Elite event-handler method at **`0x223556A0`** (complete **232 bytes**, SHA256 `66a3abbe73c87aaa6ea5f7c9c97547b99a4c2fe6e626c6bb1f77c1a8a8c60262`). Original Elite vtable base `0x2248DF04`, original offset **`+0x44C`**, exact vtable DWORD `0x2248E350` -> `0x223556A0`. Original returns `ret 4`, ends at `0x22355788`; the next executable function begins at `0x22355790`.

This is **NOT** the previously reconstructed `0x22355480` animation handler (Phase593, vtable-dispatch different) and **NOT** the timed `+0x424` callback `0x223555F0` recovered in Phase631. Both original animation handlers may use event ID `0x43` or `0x47`, but **they make different decisions and must not be merged**. The original event argument at method entry is `[ESP+4]` (verified via `mov ECX,[ESP+8]` following one `push ESI`). Its first DWORD is compared to literal event IDs `0x43` and `0x47`.

## Complete source-backed original control flow

### Event ID neither 0x43 nor 0x47

Calls inherited method **`0x2235CCE0`** with the same original event argument pointer, returns immediately. No 0x43 gate, Elite handle access or 0x47 callbacks on this path.

### Original 0x43 branch

1. Read Elite **byte `+0xFD9`**. If nonzero, immediately call inherited `0x2235CCE0` with the original event and return.
2. Otherwise, read Elite encoded serial **DWORD `+0xFDC`**. If not `0xFFFFFFFF`, load the original serial table pointer from global **`0x224F5620`**, mask handle lower 12 bits, index 16-byte serial entries, compare high serial bits and read the target pointer if matched. Invalid/mismatched handles produce **ECX = 0**.
3. **Always** call the previously authenticated original gate **`0x22315320`** at the direct callsite **`0x22355772`**, even if the target pointer is zero. The gate's first read is `[ECX+0x594]` with **no null guard**. The model reports an original null-pointer hazard and does not attempt the invalid native read or invent a graceful original fallback.
4. If the gate returns nonzero, invoke original **`0x2203D2D0(0)`** at `0x2235577F`; else return without that call. This handler does **not** itself write Elite `+0xFD9`; do not invent such a write from this function.

### Original 0x47 branch

1. Call original **`0x220326D0`** with Elite ECX; if byte result is false, simply return.
2. If true, call **`0x222AA640`** on the Elite to obtain a first target. If null, skip directly to `0x2203D2D0(0)` and return.
3. If nonnull, call **`0x222AA640` AGAIN** to obtain a second target. The returned object is dereferenced to call dynamic **vtable offset `+0x404`**. The native code does not null-check this second result, so a changed or missing pointer is a potential fault. The resulting EAX value is stored as a local DWORD.
4. Call **`0x222AA640` a THIRD time**. Add offset `+0x4EC` to its returned object, and invoke original **`0x2203F8E0`** with a pointer to the local DWORD returned from the second target's virtual `+0x404` call. The third returned pointer is also not null-checked in native code. The portable model refuses this hazard rather than dereferencing null.
5. Call original **`0x220398A0`** three times with arguments **3**, **4**, **5**, in that precise order.
6. Invoke original **`0x2203D2D0(0)`** and return. There is no inherited `0x2235CCE0` call on either successful 0x47 route.

The original function names/semantics for `0x220326D0`, `0x222AA640`, `0x2203F8E0`, `0x220398A0`, `0x2203D2D0` and target virtual `+0x404` are **not proven here**. The model exposes their exact source addresses, argument values, and call order as opaque host callbacks without substituting modern Source SDK method names.

## Deliverables

- `include/sin_compat/original_elite_secondary_event_phase632.hpp`: host-neutral model and typed stages/codes for source-backed event paths.
- `src/original_elite_secondary_event_phase632.cpp`: reconstructs the complete two-event dispatch, inherited forwarding and callback sequence, checks native invalid targets, and guards reentrancy.
- `tests/test_original_elite_secondary_event_phase632.cpp`: exhaustive positive branches and negative tests: inherited fallback, FD9 bypass, blocked/permitted gate, null handle/mismatch, false 0x47 predicate, missing first object, missing second/third objects, distinct three lookup results, +0x404 return value forwarded to the +0x4EC component, exact numbered 3→4→5 order, callback failures, exceptions and reentrant dispatch.
- `research/tools/audit_original_elite_secondary_event_phase632.py`: requires both exact retail DLL SHA256s, authenticates the **entire** 232-byte original function and original `+0x44C` vtable pointer, verifies **12** original direct-call rel32 edges and selected original instruction witnesses. Public CI uses invented bytes for SHA/call/vtable mutation refusal.
- Root and native MSVC Win32 x86 CMake builds/CTests.

The Phase631 report already counted this method as **authenticated but not implemented**. Phase632 adds its independent reference implementation without falsely incrementing global unique original native byte/function ledgers.

## Third owner-original hardware observation profile: Event

The Phase627/628 independent Win32 x86 hardware debugger retains all original PE32 SHA, live entry prefix, DR0–DR3 and abort/rollback guards. Adds **`--watch-profile event`** to watch four simultaneous owner-retail entrypoints (because x86 hardware has only four such watch registers):

| DR slot | Original entry | Native original VA |
|---|---|---|
| DR0 | Elite Spawn | `0x22355820` |
| DR1 | Original first Elite animation handler | `0x22355480` |
| DR2 | **Original second Elite animation handler (+0x44C)** | `0x223556A0` |
| DR3 | Original resolved-target 0x43 gate | `0x22315320` |

Unlike guessed Source SDK interface hooks, the debugger records only hardware execution entries from a **SHA256-authenticated original game module**, and uses read-only Win32 `ReadProcessMemory` to read a **single DWORD event ID** from the exact original event-argument pointer at `[ESP+4]` on DR1 and DR2. Any unreadable pointer is labeled **unclassified** (`event_id_readable:0`). All actual original memory pointers, engine addresses, native object contents, and event payload bytes remain absent from the trace; existing NPC/target ordinals are process-local anonymized labels.

The owner PowerShell wrapper adds `-WatchProfile Event`, independently validates the profile tag in same-PID JSONL evidence and reports four separately classified categories:

- Original first Elite handler entry with event ID 0x43/0x47;
- Original second Elite handler entry with event ID 0x43/0x47;
- Original second handler with unreadable event ID;
- Original 0x43 gate entry.

It reports no owner-original runtime proof unless process, retail SHA, live instruction prefix, watch completeness, and detach/exit criteria all pass. It never treats synthetic CI tests as real gameplay callbacks.

### Owner example (offline/disposable 2006 copy only)

```powershell
cmake -S .\pc\windows_x86_proxy -B .\build-owner-win32 -A Win32
cmake --build .\build-owner-win32 --config Release --target sin_pc_owner_elite_hwtrace_phase627

powershell -NoProfile -ExecutionPolicy Bypass -File .\pc\windows_x86_proxy\Invoke-OwnerEliteHardwareTrace.ps1 `
  -GameExecutable "D:\SiN-DisposableTest\SinEpisodes.exe" `
  -GameBin "D:\SiN-DisposableTest\SE1\bin" `
  -TraceExecutable ".\build-owner-win32\bin\Release\sin_pc_owner_elite_hwtrace_phase627.exe" `
  -OutputTrace "D:\SiN-DisposableTest\elite-events-phase632.jsonl" `
  -DisposableTestCopyConfirmed -WatchProfile Event -TimeoutSeconds 180
```

Load a map with an Elite and let an animation event fire. Upload the resulting JSONL file for cross-method call sequence review.

## Not claimed

The original engine has **not yet** invoked any reconstructed DLL code. Neither Android/Quest nor the actual Windows original game is run in public CI. Synthetic actor instances and original-free test memory can verify source-order callbacks and handle hazards, but cannot prove gameplay on a native engine. The selected fourth watch slot precludes simultaneously tracing timed update and both event handlers; use the separate Phase631 Weapon profile for the timed callback.

No already working HL2 physics, surfaces, models, props, maps, materials or rendering logic was touched.
