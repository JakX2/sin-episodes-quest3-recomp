# SiN Episodes 2006 original rifle: client/server decomp evidence

**Date:** Nov 5, 2026. **Status:** verified read-only original Windows x86 PE analysis, not recovered C++ source or playable Quest 3 code.

## Provenance

Owner-supplied original game binaries:
- `server.dll`: SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`, image base `0x22000000`.
- `client.dll`: SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`, image base `0x24000000`.

The owner provided both DLL files through `bin.zip`. Original binary bytes are **never** included in repository commits. The companion verifier checks complete x86 instruction shapes, call edges, RTTI anchors, names and expected literal values.

## New: original server-side weapon AND grenade registration proven

At `server.dll:0x223C4F60` a complete 22-byte static thunk calls the server factory registry getter `0x2228B110`, then invokes its register method with `weapon_assault_rifle` (name at `0x22477C30`) and original factory object `0x2252D4FC`.

At `server.dll:0x223C4FC0` a distinct static thunk invokes the **same registry** to register the grenade entity `grenade_ar` (name at `0x22477230`) and factory object `0x2252D500`.

Readable, partial *reconstruction* rather than copied game source:

```cpp
void original_weapon_registration() {
    auto registry = get_original_server_entity_registry(); // 0x2228B110
    registry->register_class("weapon_assault_rifle", original_weapon_factory);
}
void original_grenade_registration() {
    auto registry = get_original_server_entity_registry();
    registry->register_class("grenade_ar", original_grenade_factory);
}
```

This proves that the game code registers each classname explicitly. It does not give us the original class or factory source code.

## New: original client registration proven

At original `client.dll:0x24270B80` a client-side registration function calls getter `0x24103BF0` and invokes a virtual registration function with these exact arguments:

| Witness | Value |
|---|---|
| Classname | `weapon_assault_rifle`, literal VA `0x2429B020` |
| Client C++ class name string | `0x242C2954` = `C_WeaponAssaultRifle` |
| Size argument | `0x830` (2096 decimal) |
| Registration callback pointer | `0x2422B5B0` |
| Client class RTTI | `C_WeaponAssaultRifle`, name VA `0x24320A60` |

**Further decompilation resolved an important distinction:** `0x242C2954` is the literal client class name `C_WeaponAssaultRifle`, **not a network table pointer**. The separate `DT_WeaponAssaultRifle` receive-table name is at `0x242C298C`. The original client initialization routine `0x2422A660` refers to the receive-table name at `0x2422A6A8`, declares **one direct property**, and invokes a receive-table constructor at `0x24126A30`. A nearby property-construction call references `baseclass` (`0x2427643C`). This is strong evidence the rifle's own receive table delegates to an inherited baseclass, but the inherited fields and exact prediction behavior still need decoding. The verified `0x830` argument is not established as network payload size.

## New: rifle RTTI -> vtable -> launcher call path

MSVC RTTI for server `CWeaponAssaultRifle` is located through TypeDescriptor `0x2252D58C` and Complete Object Locator `0x224CF3A8`. Original constructor code at `0x22313C18` installs virtual table `0x224776AC`.

- `vtable + 0x388` (index 226): `0x2230BA30`, adjacent shared/unknown method, **not yet conclusively labeled**.
- `vtable + 0x38C` (index 227): `0x22313C60`, verified projectile creation routine.
- `0x22313E25`: push `grenade_ar` string.
- `0x22313E2A`: direct call to spawn helper `0x22146C40`.
- `0x22146C48`: helper calls `CreateEntityByName` at `0x22266B00`.
- `0x22313E5A`: reference to original secondary ammo literal `ARNADE`.
- `0x22313FBB` and `0x22313FDE`: direct state/timestamp-update calls.

The routine references float constants **1000.0**, **2000.0**, **0.5**, **1.0**. The decompiled instructions show launch-vector scaling, entity creation, projectile setup and timing calculations. However, the precise conditional semantics, vector units, ownership/damage setup, and ammo consumption remain unresolved. Neither the constants nor the following pseudocode should be treated as final shipping-game rules:

```cpp
void original_rifle_projectile_path(/* unknown exact signature */) {
    // Verify owner and ammo branches: detailed predicates unresolved.
    const float velocity_scale = branch_condition ? 1000.0f : 2000.0f;
    auto grenade = spawn_helper("grenade_ar", origin, direction * velocity_scale, owner);
    // Follow-up virtual calls configure grenade state/owner.
    // Refer to "ARNADE" ammo.
    // Update two weapon state/timing values using game time + 0.5 and + 1.0.
}
```

## Newly recovered primary-fire dispatch and clip update

The same rifle vtable has a virtual method at **`+0x388` -> `0x2230BA30`** that dispatches through shared firing-mode code. One branch calls the shared shot routine **`0x2230B430`** directly (callsite `0x2230BA7D`); a burst/queued-shot path goes through `0x2230B7F0` and calls the same shot routine from `0x2230B84E`. The `+0x584` field used in that latter routine is a countdown/queued-shot counter, **not the magazine size**.

The original `CWeaponAssaultRifle` vtable method at **`+0x3AC` -> `0x22313850`** returns the float stored at original VA `0x22477204`: **0.075 seconds**. Shared firing code at `0x2230B4C8` calls this virtual interval method and uses it to advance primary-fire timing. This original binary constant now appears as metadata in the engine-neutral `OriginalAssaultRifle` profile, with a C++ test, but has not yet been applied to a native engine firing loop.

The shared shot routine also reads **weapon + `0x4EC`** at `0x2230B584` and passes a fired-shot counter into integer subtraction helper **`0x222A8210`** from `0x2230B59F`; the helper's x86 instruction `sub ecx, [esi]` verifies clip subtraction. The shipped weapon script independently gives the original magazine capacity as 45 rounds. The code has more branching and ammo/prediction side effects; do not assume a uniform 13.33 shots/second under every fire mode or after dry fire.

Partial reconstruction with unresolved logic preserved:

```cpp
void shared_primary_fire_path(/* inherited Source weapon class */) {
    // Check valid owner, available clip, weapon mode and event state.
    float interval_seconds = rifle_virtual_fire_interval(); // 0.075 for SiN rifle
    fired_shots = decide_shots_due_from_time_and_mode(/* unknown exact conditions */);
    // Compose shot info and invoke original engine fire bullets path.
    clip1 -= min(fired_shots, clip1);
    // Update next-shot state, fire effects and animations.
}
```

This is a reconstructed **control-flow summary**, not exact recovered C++.

## Newly located original SinTEK Druglab grunt implementations

The **actual map class** `npc_druglab_grunt_assault_rifle` is separately registered in the original `server.dll`, not merely covered by the general `npc_sintek_grunt` family. The owner has already verified an isolated HL2 proxy for `roof_grunt_1_seq1` from this map class. The original factory and RTTI establish the game code we actually need to reproduce:

| Original BSP classname | Original server registry thunk | Original C++ factory specialization | Factory object |
|---|---|---|---|
| `npc_druglab_grunt_assault_rifle` | `0x223C7020` | `CEntityFactory<CNPC_MobGrunt>` | `0x22536848` |
| `npc_druglab_grunt_elite` | `0x223C6940` | `CEntityFactory<CNPC_EliteGrunt>` | `0x225346A0` |

Their name string arguments are at `0x22490EA0` and `0x2248E760`, respectively. Both stubs call the same native entity registry getter at `0x2228B110`, but pass **different factory objects**. MSVC RTTI confirms original class `CNPC_MobGrunt` (type descriptor at `0x22536824`, factory RTTI at `0x225367F4`) versus `CNPC_EliteGrunt` (factory RTTI at `0x225346AC`). These are verified original PE32 x86 byte patterns, not guesses from FGD alone.

The original MobGrunt-specific adjacent metadata string area has labels such as `InputThrowGrenades`, `InputAssault`, `InputLookOn`, `InputLookOff`, `GrenadeHeight`, `m_iNumGrenades`, `m_flShotDelay`, `m_nShots`, `m_flNextAltFireTime`, `m_bCanCrouch` and `m_nKickDamage`. This directly aligns with the original finale BSP's `NumGrenades=5`, `GrenadeHeight=0` and relay-driven `Assault` events. The list is **field/input name evidence only**; the actual datamap field offsets, input dispatch behavior, scheduling and timing are not yet recovered.

Elite-specific skill labels also exist in the original server binary: `sk_elite_grunt_health`, `sk_elite_grunt_kick` and `sk_elite_grunt_head_mult`. Their actual configured values and gameplay application have not been established.

**Engineering consequence:** restore two separate native NPC classes and corresponding factory registrations, rather than substituting `npc_sintek_grunt` for both or treating HL2 `npc_combine_s` as finished implementation. Keep the proven one-grunt desktop HL2 proxy as a smoke-test baseline while independent original AI logic is reconstructed.

## Decompiled MobGrunt data-map offsets and scripted I/O (original server.dll)

The following are not just strings: the original `server.dll` contains consistent **52-byte (`0x34`) Source `typedescription_t` records** with a field type, name, member offset, flag bits, optional external Hammer name, and input function pointer. The new fail-closed verifier reads those actual records and original x86 input-handler instructions. All positions below are relative to the **original 2006 x86 MobGrunt object**, not the future Quest AArch64 class.

| MobGrunt member | Original object offset | Datamap / Hammer name |
|---|---:|---|
| `m_iNumGrenades` | `+0xC38` | `NumGrenades` |
| `m_flGrenadeHeight` | `+0xC70` | `GrenadeHeight` |
| `m_flShotDelay` | `+0xC9C` | internal field |
| `m_nShots` | `+0xC98` | internal field |
| `m_flNextAltFireTime` | `+0xC90` | internal field |
| `m_vecAltFireTarget` | `+0xFCC` | internal field |
| `m_iLastAnimEventHandled` | `+0xFC8` | internal field |
| `m_bCanCrouch` | `+0xC66` | internal field |
| `m_nKickDamage` | `+0xC54` | internal field |

The original script-addressed input dispatch table is now traced:

| Hammer input | C++ data-map handler | Original `server.dll` implementation VA |
|---|---|---|
| `Assault` | `InputAssault` | `0x22357E50` |
| `ThrowGrenades` | `InputThrowGrenades` | `0x22357EA0` |
| `LookOn` | `InputLookOn` | `0x2235DDC0` |
| `LookOff` | `InputLookOff` | `0x2235F260` |

**Handler-level machine-code facts:**
- `InputAssault` at `0x22357E50` prepares an internal object at **`this+0xDB8`**, pushes integer `3` and invokes `0x220CFC00`. The meaning of the state `3` and the actual schedule transition must still be reversed.
- `InputThrowGrenades` at `0x22357EA0` inspects input data's type discriminator `6`; it **resets `this+0xC70`** (the mapped `m_flGrenadeHeight`) to zero and updates a byte at `this+0xC6C` based on the supplied value, or clears the byte on the other branch. The exact semantic field name for `+0xC6C` is not yet confirmed. This is the input handler, **not** a proof that a grenade is immediately thrown.

This provides a concrete route for restoring the original `begin_roof_battle_seq1` relay's `Assault` input and the FGD-defined grenade controls. The original map's `NumGrenades=5` and `GrenadeHeight=0` correspond to real native datamap fields; they are not arbitrary editor-only metadata.

Verifier: `research/tools/audit_sin_original_mobgrunt_datamap.py`, with synthetic regression tests `research/tests/test_audit_sin_original_mobgrunt_datamap.py`. It refuses any unexpected original DLL hash, datamap offset, field type, registration string or handler instruction. It neither outputs machine code nor needs any proprietary binary in the GitHub repository.

## Complete original MobGrunt data-map: 39 descriptors recovered

The original `CNPC_MobGrunt` data-map is now fully enumerated at `0x22534E8C..0x22535644`: **39 contiguous 52-byte descriptors**. The updated verifier compares all **33 saved/input-key fields** and **6 Hammer input handlers** against the original binary's field strings, type IDs, object offsets, flags, Hammer names and function pointers. A discontinuity, duplicate, missing entry or changed descriptor fails the check.

Additional reconstructed member fields include `m_vecTossVelocity +0xC58`, `m_bStanding +0xC64`, `m_bShouldPatrol +0xC65`, `m_bFirstEncounter +0xC67`, `m_flNextGrenadeCheck +0xC80`, `m_bWounded +0xCA0`, `m_bHaveSat +0xCB0`, `m_hCurrentGrenade +0xCB8` and `m_hTargetBarrel +0xCBC`. This is a serialization/input map, **not** an entire source class layout or a proof of runtime tactics. The verifier emits the complete human-readable inventory without reproducing game code.

### Original patrol and grenade controls

Native `InputStartPatrolling` at `0x22357E30` writes byte `1` to `this+0xC65`, mapped to `m_bShouldPatrol`. `InputStopPatrolling` at `0x22357E40` writes `0` to the same boolean. These are recovered and tested directly from the original x86 instruction bytes.

Crucially, the data-map establishes that the earlier unidentified `this+0xC6C` boolean is actually **`m_bGrenadeHappy`**. `InputThrowGrenades` writes the supplied boolean to this member and resets `m_flGrenadeHeight` at `+0xC70`. It does not directly spawn a grenade in that input handler. This explains how map scripts can enable grenade behavior independently of the rifle's secondary `grenade_ar` projectile path.

### `InputAssault`: string input to an internal assault behavior

The x86 handler at `0x22357E50` tests the incoming Source variant's string type ID `2`; for a non-string value, it calls conversion helper `0x221561D0`, and for a null string uses an empty-string fallback at `0x223D47A8`. It interns the resulting string via `0x221F3DE0`, then passes it and numeric state `3` to `0x220CFC00`, operating on the **internal behavior object at `this+0xDB8`**. The original map's relay value `seq1_assualt_point_start` therefore follows a real named-input processing path. The actual meaning of behavior state `3`, targeting, movement schedules and squad coordination remain to be decompiled.

### `LookOn` and `LookOff`: original engine flag transitions

At `0x2235DDC0`, `InputLookOn` clears bit `0x00010000` from a flags word at `this+0xB4`; at `0x2235F260`, `InputLookOff` sets that same bit. Both paths contain a matching dirty/notification update when the flag changes. This strongly suggests target-visibility/AI targetability control, **not** an animation sequence command. Source SDK singleplayer branches define `FL_NOTARGET` as bit `1<<16`; this is consistent with the observed operation, but the exact branch used by this original SiN executable should be validated before fixing a symbolic constant in the port.

Source SDK branch-sensitive reference: https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/public/const.h

## Verified original MSVC32 virtual-table and inheritance topology

The full original `server.dll` PE was further inspected for MSVC RTTI Complete Object Locators (COL), class-hierarchy descriptors and constructor-assigned virtual pointers. Unlike matching arbitrary names, these checks bind **specific native C++ classes to original object layouts and constructor instructions**. New verifier: `research/tools/audit_sin_original_druglab_vtables.py`.

| Original C++ class | Primary vtable | Auxiliary vtable at +0x790 | Auxiliary vtable at +0xC14 |
|---|---|---|---|
| `CNPC_MobGrunt` | `0x2249AAA4` | `0x2249AA7C` | `0x2249AA40` |
| `CNPC_EliteGrunt` | `0x2248DF04` | `0x2248DED8` | `0x2248DE9C` |

Original constructors install these three pointers in an ordered sequence beginning at `0x2236A35F` for MobGrunt and `0x22355CD8` for Elite. The corresponding original Complete Object Locators carry exact offsets `0`, `0x790` and `0xC14`. Both classes share auxiliary interface implementations, while their primary virtual tables differ. These offsets describe original 32-bit MSVC layout only, not a Quest 3 ABI.

**RTTI hierarchy establishes both classes are siblings under a common base `CNPC_GruntBase`.** Each class-hierarchy descriptor lists **17 entries including self**, spanning `CAI_BaseHumanoid`, `CAI_BaseNPC`, `CBaseCombatCharacter` and `CBaseEntity`. The interface positions identify the auxiliary subobjects:

| Subobject offset | Original RTTI types |
|---|---|
| `0x000` | Derived grunt, shared `CNPC_GruntBase` and Source entity/AI base classes |
| `0x790` | `CAI_DefMovementSink`, `IAI_MovementSink` |
| `0xC14` | `IBehaviorBackBridge` |

Therefore reconstruct **`CNPC_GruntBase` once with common behavior**, then provide distinct subclasses/factories for the rifle MobGrunt and elite. This is far more faithful than treating the elite as a cosmetic MobGrunt or leaving both as stock Combine NPCs.

The new synthetic regression tests validate both class vtables, constructor assignments and 17-entry hierarchy, including failed assertions when RTTI or movement subobject offsets change. The owner-supplied pinned original `server.dll` was also inspected locally at those exact virtual addresses. No original binary bytes are distributed by the repository.

## Verify without modifying the game

```powershell
py research\tools\audit_sin_rifle_client_server_registration.py --server "D:\SiN\bin\server.dll" --client "D:\SiN\bin\client.dll" --output "D:\SiN_Test_Only\rifle_client_server_audit.json"
```

Adjust the input paths to the location of the two *actual original 2006 game DLLs*; the tool refuses other hashes and refuses to overwrite an existing JSON output. The tool relies on the project's existing standard-library `verify_gameplay_entity_registry.PEImage`. Tests construct fake registration thunks and client entries; no original files are in the test fixtures.

## Next reverse-engineering gates

1. Further decode the now-verified shared primary-fire mode branches, burst counter at +0x584, clip decrement at +0x4EC, and distinction between 75ms rifle interval and other cooldowns.
2. Decode the genuine client `DT_WeaponAssaultRifle` receive table built at `0x2422A660` (name at `0x242C298C`), then follow its `baseclass` property into inherited weapon network and prediction fields.
3. Reverse the full grenade projectile lifecycle including owner, physics, impact, damage, fuse, and ammo decrement.
4. Continue from the now-verified MobGrunt data-map/handler addresses: identify the `+0xDB8` assault state machine and `+0xC6C` grenade toggle semantics; then reconstruct `CNPC_EliteGrunt` schedule and skill convar behavior.
4. Implement equivalents through a **buildable native engine/game DLL** and class factory, not by copying x86 addresses into ARM64. The stock SourceVR content-only mod loader does not accept custom game-server DLL replacements.
5. Keep the verified HL2 map-defined `roof_grunt_1_seq1` model/AI/AR2 prototype as an independent regression baseline. Its Combine AI and AR2 are not authentic Ritual gameplay.
