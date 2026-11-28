# Phase 1689: production original client entity-list factory, source-authenticated empty-state owner

## Executive distinction

This phase advances a concrete engine-facing dependency in the **real independently built production `client.dll`**, not another stand-alone instruction-count fixture. The original 2006 PE32 `engine.dll` requests `VClientPrediction001`, invokes its native slot1, and then **requires `VClientEntityList003`**. Phase1688 had already made the prediction factory and client-owned `cl_predict` ConVar registration work in the *real independent DLL*, verified as 317/317 native Windows tests and all four applicable GitHub Actions workflows; the next original engine factory request still failed because the replacement refused the entity-list interface.

The engine's **next two factory requests** after that entity-list dependency are `VCENTERPRINT002` and `ClientLeafSystem001`. Neither is implemented or silently claimed as ready by this phase.

## Original 2006 code evidence

Owner-original retail archives in Drive are hashed **before any source claims**:
- Retail `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Retail `engine.dll`: `f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`.

The original engine executes its prediction factory request window at `0x20023CF2` and prediction virtual slot1 at `0x20023D21` (Phase1658), then requests the three immediately following interfaces in exact order:

| Original engine factory instruction | Requested interface | Original result global |
|---|---|---|
| `0x20023D24` | `VClientEntityList003` | `0x203E4018` |
| `0x20023D4A` | `VCENTERPRINT002` | `0x203E401C` |
| `0x20023D70` | `ClientLeafSystem001` | `0x203E4020` |

The exact 100-byte original engine block `0x20023D24..0x20023D87` has SHA-256 `9fe9b8d464295f162bfdc7325ed70aff5e8b6ea053207740ade4112356c1037a`. In each call the engine pushes a null status, pushes the literal named-interface string, calls through the same captured client factory global `0x203E3F04`, cleans the caller stack by 8, tests EAX and stores the result into the source global. All nine address operands carry original PE32 HIGHLOW relocations. The owner-only `research/tools/audit_sin_original_client_engine_entitylist_factory_boundary.py` checks the **whole engine+client image SHAs**, entire original 100-byte block digest, this exact native call shape and ordering, literal source version names, each relocation, and original nine+three relocated entity-list vtables. Its **public** `--self-test` constructs only invented sparse-opcode windows, mutates and rejects them; no commercial binary is included in CI.

Existing source authenticated in Phases229–231 and 1656–1657 independently confirms the *actual* client `CreateInterface("VClientEntityList003")` factory thunk at `0x241056B0` returns the **embedded SECONDARY subobject `0x2434FDDC`**, not its parent `0x2433FDC8`. Their difference is exactly `0x10014`, independently corroborated by original MSVC RTTI COL at `0x242E59D4`. Two distinct original virtual tables are involved: 9-slot embedded secondary `0x24297390`, and 3-slot parent `0x242973B8`. The original raw owner slots start at parent+4, are 4,096 entries of 16 bytes, and source secondary mirrors start at secondary+0x14, are 4,096 entries of 8 bytes. Only the original networked live count has a separate 2,048-entry limit, which must not reduce mirror capacity. The original base entity-list constructor at `0x24127E80` also loops through all 4096 16-byte raw records and stores each **record's own address** at raw+`0x08` and raw+`0x0C` to initialize its intrusive links, then seeds the raw+`0x04` serial using `vstdlib.dll`'s random integer function masked to `0x7FFF`. The independently owned empty diagnostic preserves both self-links but deliberately uses deterministic zero serials. It does **not** reconstruct the original random state or base object's full free-list heads, so it remains prohibited from accepting real entity insertions.

## Actual production native PE32 implementation

`pc/independent_slot0/native_original_client_entitylist_empty_factory.h` defines a process-lifetime independently authored MSVC x86 class with **three virtual parent slots** and a **nine-virtual secondary**. It owns a real contiguous 4,096×16-byte raw table at parent+4, 16 source-owned gap bytes, and the complete nine-slot secondary virtual object exactly at parent+`0x10014`, with original secondary data fields and 4,096×8-byte mirror storage. The production DLL's own native C++ compiler, not an emulated table of function addresses, generates the correct `__thiscall RET4`/RET0 ABI for all 9 secondary methods. The source virtual order and signatures are:

| Secondary slot | Original native semantics | Expected caller-observable empty-state result |
|---|---|---|
| 0 | Networkable by mirror index, RET4 | null for each absent/invalid index |
| 1 | Networkable from EHandle, RET4 | null |
| 2 | Unknown from EHandle serial/index, RET4 | null |
| 3 | Client entity by raw index, RET4 | null |
| 4 | Client entity from EHandle, RET4 | null |
| 5 | Networked live count with optional secondary-flagged count, RET4 | zero |
| 6 | Highest entity index, RET0 | -1 |
| 7 | Set maximum entities, RET4 | original +0x08 field, allowed 0–4096 |
| 8 | Get maximum entities, RET0 | last source field value |

**Crucial limitation:** the original parent's separate slot0 and slot1 create and remove actual entities and delegate to still-incomplete entity/handle helpers. This implementation **does NOT fabricate live entities, handles, networkable registrations, player ownership, or data received from the real engine**. Both original primary slots are terminal production diagnostic guards; attempts to use either terminate rather than quietly invent a world or return a false success. Parent scalar deleting destructor also terminates because the factory owner is intentionally retained for the process lifetime. The nine safe secondary methods likewise fail closed if an impossible nonempty raw/mirror record is observed, after owner revocation, or when invalid `SetMaxEntities` exceeds known storage capacity. Source unchecked invalid indexes are deliberately bounded as an explicit safety deviation.

The real production `pc/independent_slot0/slot0.cpp` uses a **third separate opt-in** `SIN_INDEPENDENT_CLIENT_ENTITYLIST_FACTORY_PROBE=1`, requiring Phase1688's pre-existing validated `SIN_INDEPENDENT_CLIENT_PREDICTION_FACTORY_PROBE=1`/session. Only AFTER the source-attested VClient011 21-query prefix and successful `cl_predict` prediction initialization does it allocate/bind the original-shaped empty native owner. The same **actual** exported client `CreateInterface` returns its embedded secondary pointer under this gate, refuses absent/wrong versions and always refuses `VCENTERPRINT002` and `ClientLeafSystem001`. On ordinary launcher invocations the entity-list interface remains refused exactly as before. Client Shutdown revokes both prediction and entity-list factories. Pinning the independent DLL using `GetModuleHandleExA(FROM_ADDRESS|PIN)` protects the engine-visible native virtual methods from being unmapped after an ordinary FreeLibrary.

## Native test and owner-only retail probe

New Windows `test_original_client_entitylist_factory_actual_dll.cpp` loads the **actual shipping independent `client.dll`** using Win32 `LoadLibrary` and obtains `VClient011`, then calls the real 21-dependency Init with synthetic app-system factories and an independently owned ConVar. The real exported `CreateInterface` is tested under default/refused owner configurations and the explicit post-prediction entity-list gate. The test validates all 9 compiled secondary and all 3 compiled parent vtable function pointers belong to the **loaded replacement DLL**, the exact original secondary/primary and raw table relative addresses, empty 4096-slot raw/mirror records, and original count/max/high-water offsets. It invokes ALL 9 authentic native methods through the real compiled vtable using MSVC x86 inline assembly to check ECX receiver and ESP caller stack balance. It tests max values, boundary indices and reserved/stale EHandles, wrong interface refusal and client Shutdown. A separately launched child process invokes real parent registration virtual slot0 with two arguments and must terminate with the existing fail-closed `0x53494E45` code; no entity is ever fabricated. The test additionally verifies that an engine-retained entity-list pointer remains mapped after the host releases the original LoadLibrary reference.

New launcher `-EnableClientEntityListFactoryProbe` is only accepted alongside `-EnableClientPredictionFactoryProbe`, which already requires Client-only mode and refuses simultaneous unrelated terminal sound probes. The launcher continues verifying the actual commercial originals' complete SHA256, a positively marked disposable test installation, non-reparse parent paths, and reversible client.dll backup/restore. Both owner environment variables are scoped to that game child process and restored from inherited values in `finally`. The offline startup classifier now emits `OWNER_CLIENT_ENTITYLIST_EMPTY_FACTORY_BOUNDARY` when it sees the new source-backed diagnostic. Synthetic PowerShell tests insist on rejecting an unguarded flag and refuse to claim boot if the trace reaches the empty entity list then fails at actual entity insertion.

Two material new native Windows CTests are registered: real shipping PE32 client.dll entity-list integration, and the original-free mutation self-audit. The **previous verified baseline was 317/317**, so this branch aims for **319 native tests**, conditional on exact-head CI success. The test count is only a regression accounting measure, not a goal in itself. No original retail game engine has been run in public CI.

## Next engine evidence

With the original-source-defined empty entity list available under owner-only diagnostic, a **real retail engine process** should either request `VCENTERPRINT002`, request another earlier system, or call an unimplemented original registration/lifecycle callback. Each outcome is more precise than claiming that a dummy entity list enabled gameplay. Subsequent work should be based on that real first failure and source decompilation of its actual dependency. The user-excluded Source/HL2-equivalent physics, surface materials, prop/model loading/rendering tasks remain untouched.
