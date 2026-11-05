# SiN Episodes 2006 original rifle: client/server decomp evidence

**Date:** Nov 5, 2026. **Status:** verified read-only original Windows x86 PE analysis, not recovered C++ source or playable Quest 3 code.

## Provenance

Owner-supplied original game binaries:
- \`server.dll\`: SHA-256 \`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650\`, image base \`0x22000000\`.
- \`client.dll\`: SHA-256 \`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d\`, image base \`0x24000000\`.

The owner provided both DLL files through \`bin.zip\`. Original binary bytes are **never** included in repository commits. The companion verifier checks complete x86 instruction shapes, call edges, RTTI anchors, names and expected literal values.

## New: original server-side weapon AND grenade registration proven

At \`server.dll:0x223C4F60\` a complete 22-byte static thunk calls the server factory registry getter \`0x2228B110\`, then invokes its register method with \`weapon_assault_rifle\` (name at \`0x22477C30\`) and original factory object \`0x2252D4FC\`.

At \`server.dll:0x223C4FC0\` a distinct static thunk invokes the **same registry** to register the grenade entity \`grenade_ar\` (name at \`0x22477230\`) and factory object \`0x2252D500\`.

Readable, partial *reconstruction* rather than copied game source:

\`\`\`cpp
void original_weapon_registration() {
    auto registry = get_original_server_entity_registry(); // 0x2228B110
    registry->register_class("weapon_assault_rifle", original_weapon_factory);
}
void original_grenade_registration() {
    auto registry = get_original_server_entity_registry();
    registry->register_class("grenade_ar", original_grenade_factory);
}
\`\`\`

This proves that the game code registers each classname explicitly. It does not give us the original class or factory source code.

## New: original client registration proven

At original \`client.dll:0x24270B80\` a client-side registration function calls getter \`0x24103BF0\` and invokes a virtual registration function with these exact arguments:

| Witness | Value |
|---|---|
| Classname | \`weapon_assault_rifle\`, literal VA \`0x2429B020\` |
| Table-like pointer | \`0x242C2954\` |
| Size argument | \`0x830\` (2096 decimal) |
| Registration callback pointer | \`0x2422B5B0\` |
| Client class RTTI | \`C_WeaponAssaultRifle\`, name VA \`0x24320A60\` |

The table pointer and size argument are verified but the network/prediction field layout **is not yet decoded**. Do not assume \`0x830\` is the network message payload size.

## New: rifle RTTI -> vtable -> launcher call path

MSVC RTTI for server \`CWeaponAssaultRifle\` is located through TypeDescriptor \`0x2252D58C\` and Complete Object Locator \`0x224CF3A8\`. Original constructor code at \`0x22313C18\` installs virtual table \`0x224776AC\`.

- \`vtable + 0x388\` (index 226): \`0x2230BA30\`, adjacent shared/unknown method, **not yet conclusively labeled**.
- \`vtable + 0x38C\` (index 227): \`0x22313C60\`, verified projectile creation routine.
- \`0x22313E25\`: push \`grenade_ar\` string.
- \`0x22313E2A\`: direct call to spawn helper \`0x22146C40\`.
- \`0x22146C48\`: helper calls \`CreateEntityByName\` at \`0x22266B00\`.
- \`0x22313E5A\`: reference to original secondary ammo literal \`ARNADE\`.
- \`0x22313FBB\` and \`0x22313FDE\`: direct state/timestamp-update calls.

The routine references float constants **1000.0**, **2000.0**, **0.5**, **1.0**. The decompiled instructions show launch-vector scaling, entity creation, projectile setup and timing calculations. However, the precise conditional semantics, vector units, ownership/damage setup, and ammo consumption remain unresolved. Neither the constants nor the following pseudocode should be treated as final shipping-game rules:

\`\`\`cpp
void original_rifle_projectile_path(/* unknown exact signature */) {
    // Verify owner and ammo branches: detailed predicates unresolved.
    const float velocity_scale = branch_condition ? 1000.0f : 2000.0f;
    auto grenade = spawn_helper("grenade_ar", origin, direction * velocity_scale, owner);
    // Follow-up virtual calls configure grenade state/owner.
    // Refer to "ARNADE" ammo.
    // Update two weapon state/timing values using game time + 0.5 and + 1.0.
}
\`\`\`

## Verify without modifying the game

\`\`\`powershell
py research\tools\audit_sin_rifle_client_server_registration.py --server "D:\SiN\bin\server.dll" --client "D:\SiN\bin\client.dll" --output "D:\SiN_Test_Only\rifle_client_server_audit.json"
\`\`\`

Adjust the input paths to the location of the two *actual original 2006 game DLLs*; the tool refuses other hashes and refuses to overwrite an existing JSON output. The tool relies on the project's existing standard-library \`verify_gameplay_entity_registry.PEImage\`. Tests construct fake registration thunks and client entries; no original files are in the test fixtures.

## Next reverse-engineering gates

1. Identify the original rifle's inherited/shared primary attack method, trace clip decrement and 45-round primary firing cadence.
2. Decode the client table at \`0x242C2954\` and map prediction/receive fields to original server data.
3. Reverse the full grenade projectile lifecycle including owner, physics, impact, damage, fuse, and ammo decrement.
4. Implement equivalents through a **buildable native engine/game DLL** and class factory, not by copying x86 addresses into ARM64. The stock SourceVR content-only mod loader does not accept custom game-server DLL replacements.
5. Keep the verified HL2 map-defined \`roof_grunt_1_seq1\` model/AI/AR2 prototype as an independent regression baseline. Its Combine AI and AR2 are not authentic Ritual gameplay.
