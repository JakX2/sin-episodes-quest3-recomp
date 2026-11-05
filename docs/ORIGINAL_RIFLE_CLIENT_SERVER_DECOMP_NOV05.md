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

## Verify without modifying the game

```powershell
py research\tools\audit_sin_rifle_client_server_registration.py --server "D:\SiN\bin\server.dll" --client "D:\SiN\bin\client.dll" --output "D:\SiN_Test_Only\rifle_client_server_audit.json"
```

Adjust the input paths to the location of the two *actual original 2006 game DLLs*; the tool refuses other hashes and refuses to overwrite an existing JSON output. The tool relies on the project's existing standard-library `verify_gameplay_entity_registry.PEImage`. Tests construct fake registration thunks and client entries; no original files are in the test fixtures.

## Next reverse-engineering gates

1. Further decode the now-verified shared primary-fire mode branches, burst counter at +0x584, clip decrement at +0x4EC, and distinction between 75ms rifle interval and other cooldowns.
2. Decode the genuine client `DT_WeaponAssaultRifle` receive table built at `0x2422A660` (name at `0x242C298C`), then follow its `baseclass` property into inherited weapon network and prediction fields.
3. Reverse the full grenade projectile lifecycle including owner, physics, impact, damage, fuse, and ammo decrement.
4. Implement equivalents through a **buildable native engine/game DLL** and class factory, not by copying x86 addresses into ARM64. The stock SourceVR content-only mod loader does not accept custom game-server DLL replacements.
5. Keep the verified HL2 map-defined `roof_grunt_1_seq1` model/AI/AR2 prototype as an independent regression baseline. Its Combine AI and AR2 are not authentic Ritual gameplay.
