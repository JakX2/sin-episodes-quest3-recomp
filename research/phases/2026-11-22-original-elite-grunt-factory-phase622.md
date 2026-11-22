# Gameplay Phase622: Original CNPC_EliteGrunt native factory is distinct from MobGrunt

## Evidence and scope

Directly examined the owner-provided **SHA256-matching original 2006** `server.dll` and `client.dll` locally, without putting either file or extracted instruction sequences in the public repository.

Phase621 recovered a fourth original **CNPC_MobGrunt** alias `npc_druglab_grunt`. This phase explicitly does **not** relabel the **Elite Grunt** as a fifth MobGrunt variant. The retail `npc_druglab_grunt_elite` factory registration at VA `0x223C6940` was already authenticated by Phase530 (22 bytes; original SHA256 `c1d85ff86f8114d66c6ef3938ea76e886089c30bef00f991becacbd3908b7097`).

The Elite factory object at `0x225346A0` stores the **different** vtable `0x2248DB64`. The associated Microsoft C++ RTTI is `.?AV?$CEntityFactory@VCNPC_EliteGrunt@@@@`, different from the `CNPC_MobGrunt` factory RTTI. The original SDK's `SinEpisodes.fgd` lists `npc_druglab_grunt_elite` separately from pistol, scattergun and assault-rifle Grunts, with editor-facing `additionalequipment` choice `weapon_chaingun`. The FGD **does not prove** the constructor or Spawn automatically equips that weapon.

## Four complete original Elite factory method bodies

| Method | Original VA | Complete bytes | Original SHA256 |
|---|---|---:|---|
| Factory creator wrapper | `0x22355C70` | 23 | `6eecf5ca8aa241b73c4f1557c98667941ac865d58014926bdfdd055381792de9` |
| Allocation and virtual initializer | `0x22355C90` | 61 | `1a5252daa1697cb3309a38c3581f0ee6400b4a412a8f6518c598635665489ae0` |
| Native Elite constructor | `0x22355CD0` | 90 | `309d176029f698699be3888311b0d93713a952e24d503622227290dc9923ca32` |
| Factory delegate release | `0x223552C0` | 16 | `ed8abac6973adb2766ebee7d4e8a19d917263843bcec4e4a25f52cc4c4203a21` |

**Accounting note:** The factory creator and delegate release bodies have the exact same complete SHA256 as their MobGrunt counterparts but live at distinct VAs. The 61-byte allocation helper and the 90-byte Elite-specific constructor have distinct contents. The earlier 43-method / 11,233-byte ledger is not silently rewritten: reconciliation with that phase's original ranges is required before a global count is claimed.

## Source-backed native control flow

1. The original creator wrapper calls the Elite allocation helper and returns `native_object + 0x160` (the engine networkable/edict-facing subobject view).
2. The allocation helper requests **`0xFE0` bytes**, not MobGrunt's `0xFD8`. It calls constructor `0x22355CD0` and then dispatches through the newly constructed object's virtual `+0x70`. On original allocation failure, the assembly follows a **null-dereference-prone path**; the clean-room model deliberately fails closed instead.
3. The constructor calls shared base initializer `0x22360D80`; installs three Elite-specific vptrs at offsets `+0`, `+0x790`, `+0xC14`, pointing to original retail vtables `0x2248DF04`, `0x2248DED8`, `0x2248DE9C` respectively.
4. It calls a static initialization helper `0x22360BC0`, compares a cached generation word at `0x22534694` against `0x2254A3C0`. Only when they differ, it calls Elite-specific static initializer `0x22355A20`, sets global byte at `0x22534690` to 1, and caches the current generation word.
5. It writes an unclassified **`0xFFFFFFFF` sentinel to new object field `+0xFDC`**, and returns the original object.
6. The separately registered factory release delegate checks for non-null input and dispatches `vtable +0x18`. This is only the factory delegation shape; a new native runtime must still implement actual object ownership and destruction.

The six direct calls in the source-authenticated methods are checked by a Python original-only auditor; complete functions and all three original vptr writes are separately SHA/code-pattern checked. Field semantic meanings beyond the observed machine code remain unproven.

## Portable reconstruction

The new `sin_original_elite_grunt_factory_phase622` static C++ library models the **exact elite class only** via a callback-based host that must supply the real native allocation, base construction, vptr installation, source static initialization, generation guard, final sentinel store and original v+0x70 initialization.

No original PE32 vtable address is dereferenced or copied as a live pointer. The host adapter must provide its **own verified** runtime memory/layout and identity. The portable reference refuses unsupported classname aliases, invalid allocation, address overflow, any rejected stage, and reentrancy. It includes independent (not original-game) unpublished-allocation cleanup on failure, without claiming unknown original side effects are reversed.

The synthetic C++ tests verify stage order, epoch-equal vs epoch-changed branches, `+0xFDC` sentinel, reference return adjustment, refusal and reentrancy. The Python synthetic verifier tests structural mutation rejection and refuses unrelated DLL fingerprints.

### Owner-only original retail authentication

```sh
python3 research/tools/audit_original_elite_grunt_factory_phase622.py \
  --server "<private-game-copy>/SE1/bin/server.dll" \
  --client "<private-game-copy>/SE1/bin/client.dll"
```

No original DLL bytes or original instruction dumps are checked in. No real engine-owned Elite Grunt is spawned by this phase; Spawn weapon selection, AI schedule, navigation and native lifecycle integration remain outstanding. The project continues to skip generic HL2 physics, surfaces, models, props and rendering.
