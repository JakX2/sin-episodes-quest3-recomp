# Gameplay Phase623: Retail 2006 CNPC_EliteGrunt::Spawn, weapon selection and native handle

## Source provenance

The owner's original `SE1/bin.zip` remains local and private. Both its PE32 DLLs match the recorded SHA256 fingerprints:

- `server.dll` 6,049,792 bytes, `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll` 3,592,192 bytes, `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

**One principal complete original method:** `CNPC_EliteGrunt::Spawn` at `0x22355820`, **505 bytes**, complete body SHA256 **`6d56850edc4675dc55273a9ff476e6e8cf0c425e187f9c527423dedabe115b99`**. Original primary Elite vtable `0x2248DF04`, entry `+0x64` (index 25) points to `0x22355820`. The method ends with its last `ret` at `0x22355A18`, immediately before `0xCC` padding.

**A second original method is now authenticated:** Elite vtable `+0x798` points to `0x22353F40`, a complete **13-byte method**, SHA256 `0f4ce58b1f4c81550434c7fef9f424f8a75030192740e245ac40af7b7ebf5bf5`. It loads the argument's low byte, writes it to `this+0xCB2`, then returns while popping its four-byte argument. Since Spawn passes zero, the call clears `+0xCB2`.

This is additional complete original machine code after the distinct Elite factory recovery of Phase622. As previous phase ranges and potentially duplicate wrappers require global deduplication, this report does **not** add 505 bytes to the Phase617 43-method/11,233-byte total without reconciling every original code range.

## Source-backed Spawn sequence

The original engine calls the Elite `Spawn` function on an already created `CNPC_EliteGrunt`. The source performs the following steps **in order**:

1. Compare the current weapon symbol `this+0xA24` (or empty fallback) against the `weapon_chaingun` literal at `0x224781A4`. If different, intern the latter with `0x221F3DE0` and write the symbol value at `+0xA24`. No extra weapon is invented.
2. Invoke inherited virtual `+0x68`, then the `+0x1C` output passed to `+0x6C`, then call inherited NPC Spawn `0x2235F2A0`.
3. Clear `this+0xFD9` to zero and invoke native virtual `+0x798` with zero. The **verified 13-byte target** writes zero into `this+0xCB2`; the reference now models that concrete field update rather than treating this virtual as an unknown black box.
4. Read game global `0x2259C208`, dereference `+0x28` as an `f32`, temporarily set x87 rounding control to **truncate toward zero**, convert, restore x87 control, and (only when different) invoke original virtual `+0x1B4` before writing field `+0x9C`.
5. **Read the same global float a second time**, with an independent x87 conversion, for original virtual `+0x1B0` before field `+0x98`. The host model preserves this distinction instead of silently assuming one shared read.
6. If absent, OR field `this+0xB4` with **`0x00100000`**, marking the network data dirty via `this+0x200` when `+0x1F0` is set, or edict `this+0x16C` state bit 0 when present, otherwise no dirty marker.
7. If absent, OR field `this+0xB4` with **`0x00002000`**, using the same dirty path. This mask is **`0x2000`, not `0x200000`**: the original instruction is `or ch,0x20`, and x86 `CH` corresponds to bit positions 8–15.
8. Call original `0x222AA640` to resolve the active native combat weapon through its native handle. Use native RTTI to cast from `CBaseCombatWeapon` to `CWeaponChaingun` (original type descriptors `0x224DE300` and `0x2252DA38` respectively; the source calls original runtime `0x223A5BAB`).
9. On success, invoke the typed result's original virtual `+0x08`, read the first dword of its returned pointer and write that encoded native value to `this+0xFDC`. If no matching typed weapon, write `0xFFFFFFFF` to `+0xFDC`.

The `+0xFDC` field is source-connected to the Elite animation-event override already modeled in Phase593: event `0x43` resolves a serial handle from that field before evaluating the original gate. This does not prove event execution in the live original game under the reconstructed modules, but establishes the original dataflow relationship.

## Portable implementation

The new `sin_original_elite_grunt_spawn_phase623` C++17 static library models only this sequence through an independently authored `IHost`. The host must supply genuine engine/weapon ownership, symbol interning, NPC lifecycle callbacks, dirty notification, ConVar reads and native RTTI/equipment handle resolution, and the authenticated original `+0xCB2` byte store.

The model:

- Refuses any classname other than exact `npc_druglab_grunt_elite`.
- Never allocates `CBaseNPC`, changes original `server.dll`, or passes retail vtable addresses into a modern native engine.
- Executes the common inherited lifecycle and chaingun-specific steps in original order, two separate scalar reads, both flag transitions and subsequent typed weapon lookup.
- Refuses nonfinite/out-of-range x87 conversion inputs rather than pretending portable C++ reproduces x87 integer-indefinite conversion/exception outcomes.
- Stops and reports the exact rejected/throwing stage and does not claim rollback of already applied original-side effects. Reentrant callbacks cannot execute a second Spawn.

Synthetic C++ regressions cover the full path, preexisting chaingun, unchanged values/flags, absent RTTI result, zero encoded handle preservation, dirty flag paths, stage refusals/exceptions, finite x87 boundaries and reentrancy. The new original-byte audit `research/tools/audit_original_elite_grunt_spawn_phase623.py` checks both original DLL digests, full 505-byte Spawn SHA, additional complete 13-byte setter SHA, both Elite vtable entries, literal/RTTI identity, 4 original call targets, operand witnesses and both native flag opcodes.

### Owner-only original verification

```sh
python3 research/tools/audit_original_elite_grunt_spawn_phase623.py \
  --server "<owner-private>/SE1/bin/server.dll" \
  --client "<owner-private>/SE1/bin/client.dll"
```

With `--self-test` the audit runs using **fabricated** addresses and instruction shapes only, refusing nonretail bytes. The actual original DLLs, disassembly and game assets are never committed to GitHub.

## What Phase623 does NOT prove

No reconstructed Elite `Spawn` has yet run on a **genuine original native engine-owned Elite NPC**. A verified native host still needs to connect factory ownership, real Source actor identity/edict, Spawn/Think/Remove, registered conditions, animation and equipment callbacks to our reconstructed gameplay modules. The Phase620 Windows proxy only validates optional native-host DLL loads; it does not implement such callbacks. No Android/Quest gameplay boot claimed. Already working HL2 physics, surfaces, models/props and rendering remain out of scope.
