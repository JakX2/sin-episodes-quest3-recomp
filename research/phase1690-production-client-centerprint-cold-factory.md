# Phase 1690: actual production PE32 VCENTERPRINT002 cold service boundary

## Why this is the next retail startup dependency

This work is stacked on fully verified Phase1689 PR #1698, exact commit `7f64804a516319d5dbbed96af520026f51252419`: **319/319 native MSVC x86 Windows tests and all five applicable GitHub Actions workflows green**, including the Android ARM64 NDK library build and Bionic x86_64 emulator `dlopen` smoke. Phase1689 made the actual independently built shipping `client.dll` return a source-authenticated nine-slot `VClientEntityList003` secondary interface under a narrow owner-only diagnostic gate, but still deliberately refused original entity insertion and real gameplay.

The next exact original 2006 PE32 engine factory request after `VClientEntityList003` is `VCENTERPRINT002` at engine VA `0x20023D4A`, writing the returned pointer to original engine global `0x203E401C`; the next request after that is `ClientLeafSystem001` at `0x20023D70`. Both names, x86 native call order and relocated pointers are authenticated through the original 100-byte complete engine window at `0x20023D24` in Phase1689. The original source-engine client factory returned a global 8-slot object for `VCENTERPRINT002`, so refusing it is a genuine subsequent factory integration blocker.

Phase1690 adds only the **original-initial-state** native interface, not a fictional working GUI. It is intentionally not an HL2-equivalent physics, materials, model, prop or rendering task.

## Complete original whole-method evidence, not opcode counts

The unchanged owner-provided original client.dll SHA-256 is `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`, and original engine.dll SHA-256 is `f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`.

The original client factory thunk at `0x241EC540` is an exact six-byte `mov eax, 0x2431DBF0; ret` (complete SHA256 `81267ad71e6a917e5251cb606e3180b7d36d219e6be716ed03bc97674e9c0aca`), returning one process-global object. Original complete constructor `0x241EC5E0`, 16 bytes, SHA `2d512dbd13741df5f0c12d36879081b847815f0768b58eefca9df1bfe9da2d06`, writes a native virtual pointer to `0x242ABC4C` at object+0 and **clears its backing service pointer at object+0x04**. This exact cold state is also present in original initialized .data at `0x2431DBF0` (vptr 0x242ABC4C, +4 null).

The entire original 8-DWORD vtable at `0x242ABC4C` SHA256 is `35451d0a78e637ba3a333d8f942768cf1a938954277b92ab226ab98fff92edcb`. Each virtual pointer and both original factory/constructor vptr operands carry real PE32 HIGHLOW relocations. All eight original methods are full-body SHA-authenticated in the owner read-only audit:

| Native virtual | Original client VA | Whole original bytes | Native `__thiscall` callee cleanup | Cold `+4 == nullptr` behavior |
|---|---:|---:|---|---|
| slot0 | `0x241EC430` | 20 | RET16 | no-op |
| slot1 | `0x241EC480` | 45 | RET4 | no-op |
| slot2 | `0x241EC450` | 45 | RET4 | no-op |
| slot3 | `0x241EC4D0` | 20 | RET20 | no-op |
| slot4 | `0x241EC4B0` | 20 | RET20 | no-op |
| slot5 | `0x241EC4F0` | 18 | RET0 | no-op |
| slot6 | `0x241EC760` | 59 | RET4 | allocate 0xF8-byte GUI service, then call still-unmodeled 0x241EC5F0 |
| slot7 | `0x241EC510` | 43 | RET0 | no-op/clear null backing |

The forwarding paths for slots0–5,7 target virtual methods `backing+0x2B8`, `+0x2C4`, `+0x2C8`, `+0x2CC`, etc. Those forwarding services only exist **after** slot6 correctly constructs them. We do not invent, approximate or silently discard the slot6 service creation side effect. Calling slot6 in the independent DLL is instead an explicitly fatal, diagnostic unsupported callback until its constructor/GUI ownership can be decompiled and implemented.

Four directly authenticated complete engine xrefs demonstrate that the original retail engine itself uses the wrapper:
- `0x200AA539`, slot5, 15-byte complete caller SHA `a6b23915c43362bff5232308e260ed3e3782a2b41c809a514dbc2557821751b5`
- `0x200AA600`, slot4, 34-byte SHA `36e10b728d6b051ddc80f6d46dc0187cf811e33ed7cb32a0c1033da1ceeead0c`
- `0x200AA630`, slot5, 16-byte SHA `ccc47598cef68fa6fc3dbf9d72ebbe56bf0dee595f1bf6573f9aa00137e9fd37`
- `0x20150D56`, slot5, 19-byte SHA `b26177313caa60a0cf814a2e1db85ec864759755faf610b6ea5ad50c7bf96cbb`

The new `research/tools/audit_sin_original_client_centerprint_cold_factory.py` validates **both whole-image SHA256s**, all 8 complete original method hashes and native RET lengths, original nullable +4 source guards, slot6 0xF8 allocation/direct constructor CALL, original slot7 service reset, factory thunk, constructor and vtable relocations, 32-byte original vtable hash/order, and these four whole engine caller windows. The public `--self-test` constructs **invented sparse mutation windows** and rejects changes; no original game code or assets are shipped.

## What enters the real production DLL

`pc/independent_slot0/native_original_client_centerprint_cold_factory.h` adds a compiler-generated MSVC Win32 x86 object with exactly eight virtuals, source +4 backing pointer null, and process-local lifetime/guard fields following the source-observable fields. Virtual slots 0–5,7 perform the **original cold-state no-ops** with accurate callee cleanup for their known prototype arities. Slot6 is terminal with an explicit unsupported diagnostic, not a fake success. After client shutdown all methods terminate on revoked state rather than accessing stale engine data. No inherited virtual destructor can be inserted ahead of original slot0.

The **actual shipping** `pc/independent_slot0/slot0.cpp` uses that class only when `SIN_INDEPENDENT_CLIENT_CENTERPRINT_FACTORY_PROBE=1`, requiring all previous owner-only trace/session, production prediction and entity-list gates, the original 21 `VClient011` app-system factory dependencies, and successful source `cl_predict` read/optional native registration. Only after the source-correct empty entity list has been installed does `VClient011.Init` bind the cold centerprint singleton. The same real exported `CreateInterface("VCENTERPRINT002")` returns the process-lifetime pointer. The independent DLL is pinned with `GetModuleHandleExA(FROM_ADDRESS|PIN)` before giving the original engine a pointer to native C++ vtable code, to prevent module unmapping while the pointer may be retained by the engine. Client Shutdown revokes prediction, entity list and centerprint. **Ordinary launches still refuse all three incomplete factories.** Wrong version `VCENTERPRINT003` and next `ClientLeafSystem001` remain refused.

## Native loaded-DLL proof, owner launch and non-goals

New CTest `independent_client_centerprint_cold_factory_actual_dll_native` loads the ACTUAL independent shipping client.dll with Windows `LoadLibrary`, calls its real `CreateInterface("VClient011")`, passes the original 21 engine factory name/order using invented host-owned services and ConVar, then explicitly enables the prediction, entity-list and centerprint gates. It checks refusal before the gates, exact exported cold singleton identity, eight actual compiled methods owned by the module, original +4 null backing, original native `ECX` receiver and `ESP` callee-cleanup for slots0–5,7 (RET16, RET4, RET4, RET20, RET20, RET0, RET0), source engine-like slot4 call with four 0xFF color values and final0, repeated engine slot5, and that no call modifies the null backing. A separately launched child process invokes real vtable slot6 and MUST exit with native source diagnostic code `0x53494E45`. It also checks client Shutdown revocation, wrong/next interface refusal and DLL module lifetime pin.

The companion new CTest `independent_original_client_centerprint_cold_audit_synthetic` runs the source verification script in original-free mutation self-test mode. The verified parent Phase1689 had **319/319 Windows native CTests**; Phase1690 adds two concrete original-driven tests, targeting **321** on exact-head CI. This test total is regression accounting, not an instruction-execution milestone.

The reversible Windows owner-only launcher `Invoke-OwnerIndependentEngineProbe.ps1` adds `-EnableClientCenterprintFactoryProbe`, which requires the prior `-EnableClientEntityListFactoryProbe` switch and ultimately the prior `-EnableClientPredictionFactoryProbe`, Client-only mode, disposable-copy confirmation, SHA-verified original DLLs and automatic DLL restoration. A new environment variable is explicitly set to 0 in ordinary runs, scoped to the one child game process when opted in, and restored to its inherited process value in `finally`. The compiled cold wrapper also emits **first-call-only** `client-centerprint-slot4-cold` and `client-centerprint-slot5-cold` events to the existing owner diagnostic, matching directly SHA-authenticated original engine callers (`0x200AA600`, `0x200AA539`, `0x200AA630`, `0x20150D56`). No text, addresses, pointers, GUI state or other game data are logged. The actual independent DLL CTest invokes the real native slot4 once and slot5 twice, then opens **only its own PID/session trace file** to assert exactly one event per slot; the offline classifier publishes `OWNER_CLIENT_CENTERPRINT_COLD_METHOD_ENTRY`, separately from factory acceptance and unsupported service initialization. Any emitted event still means only the original cold/null no-op method executed, never that a centered message was rendered. Synthetic PowerShell tests deny unguarded switches without moving any fake original DLLs. The offline startup classifier tracks `client-centerprint-cold-bound` / `client-centerprint-cold-refused` separately from factory acceptance and unsupported callback terminals, and retains **NO GAME BOOT** verdict on the corresponding synthetic same-session replay.

No original retail game engine, its maps or assets are executed in the public CI. A real Windows game startup still requires source-authenticated original entity registrations and lifecycle callbacks, proper centerprint backing service and the next leaf-system interface. This phase deliberately avoids user-excluded Source/HL2 physics, surface materials, models/props and renderer implementation.
