# Original 2006 SiN Assault Rifle grenade: player/NPC secondary-fire integration

8 November 2026 | Independently authored C++17 gameplay reconstructed from original game evidence.

This phase connects the source-backed original Assault Rifle metadata, the existing generation-safe WeaponSession, and the new CGrenadeAR projectile lifecycle from PR #151. It does NOT create real native SourceVR entities or a playable Quest application.

## Actual original server.dll evidence

Original SiN Episodes: Emergence 2006 server.dll SHA256:
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

Previous private-binary audits in research/phases/sin-original-ar-grenade-lifecycle-and-ammo.md and research/phases/sin-original-2006-assault-rifle-grenade-creation.md prove:

- Original CWeaponAssaultRifle secondary firing routine begins at x86 0x22313C60. It checks a CBasePlayer virtual and uses MSVC RTTI to distinguish CNPC_GruntBase (branch at 0x22313DE7).
- The grunt-base branch multiplies a direction vector by exactly 1000.0f (original data at 0x223E97F0); the normal player branch uses 2000.0f (0x224771F0). The exact vector derivation and physical units have NOT been recovered; the port must accept a validated host-provided vector and must not guess muzzle offsets or normalize blindly.
- At 0x22313E25/0x22313E2A original code requests classname grenade_ar and invokes entity creation via original helper 0x22146C40 leading to CreateEntityByName 0x22266B00.
- Original secondary ammo type is weapon member +0x4E8. On the normal player path the original calls RemoveAmmo-like 0x222AA670 at 0x22313F39 with quantity ONE, after the entity creation call. Only a finite/valid owner ammo index is proven; invalid/infinite index handling needs further analysis.
- The CNPC_GruntBase branch does not traverse this same player-side ammo debit. This is NOT evidence of globally infinite NPC ammo. NPC launch authorization remains the engine/AI host's responsibility.
- The original weapon profile independently confirms secondary ammo ARNADE, projectile grenade_ar and model models/weapons/grenades/ar_grenade.mdl. The clip2_size of -1 is not a second ordinary magazine.

## Substantive implementation

- include/sin_compat/weapon_runtime.hpp: read-only WeaponKind accessor.
- include/sin_compat/weapon_session.hpp and src/weapon_session.cpp: can_launch_rifle_grenade readiness check and owner/weapon reference access. Requires an Assault Rifle with live selected weapon, generation-safe owner relation, no holster and no active reload. This is a preflight guard, not the original fire animation.
- include/sin_compat/original_rifle_grenade_launch.hpp and src/original_rifle_grenade_launch.cpp: player and grunt-specific launch paths, actual C++17 host APIs and real connection to the CGrenadeAR Runtime (PR #151).
- Player branch uses an authoritative host-owned ARNADE ammo snapshot, rejects unknown/infinite/invalid ammo semantics and requires one unit. World placement and the single player ammo charge are committed atomically by a future native host callback.
- Grunt branch requires live owner/weapon and an explicit host approval for a true grunt-base NPC, uses the original 1000 scalar and makes no claim about the NPC's broader ammo rules.
- Both paths prepare the original grenade_ar entity with original model and host-provided direction, arm an independently authored CGrenadeAR Runtime, and reject nonfinite/unrepresentable physics values.
- Host callback reentrancy guard prevents nested secondary-fire attempts. Failed preparation or commit leaves the output runtime unchanged. Failed atomic commit aborts the staged entity without consuming ammo.

## Explicit port-safety policy, not an original x86 claim

The original has a known call order of entity creation followed later by player RemoveAmmo(1); its original engine implementation's failure and networking semantics are not completely recovered.

This independent host contract therefore stages the projectile without publishing it, then requires an atomic publication and player ammo-debit transaction. This avoids charge-without-grenade and grenade-without-charge bugs. The all-or-nothing operation is a deliberate modern host contract, NOT a proven original Source transaction method.

Other original firing side effects, animation, attack intervals, prediction, NPC AI authorization and muzzle/trajectory physics are not guessed. This code cannot yet be called from SourceVR without a genuinely compatible Android ARM64 game entity and engine interface.

## Verification

New CMake static target sin_original_rifle_grenade_launch links the existing sin_weapon_runtime and sin_original_ar_grenade_runtime libraries.

New sin_original_rifle_grenade_launch_tests runs against two independent synthetic hosts, checking:
- normal player-shot launch velocity scale 2000, original projectile classname/model, one authoritative ARNADE debit, and armed grenade lifecycle;
- NPC grunt scale 1000, explicit AI approval and no deduction via the identified player ammo path;
- weapon kind/owner mismatch, stale/deselected weapon, reload and holster gates;
- finite empty ammo, invalid index, unverified infinite ammo semantics;
- wrong owner classification, invalid direction and game clock, duplicate projectile binding;
- failed staged entity creation, failed atomic commit with precise abort, and nested reentrant callbacks.

The official Android ARM64 API26 and Bionic x86_64 build scripts now cross-compile the new static library AND the original rifle-grenade integration regression executable. GitHub CI stages and actually executes the C++17 mock-host gameplay test on an Android API29 x86_64 Bionic emulator, verifies its pass marker, and archives an output log. This is real Android execution of host-neutral reconstructed gameplay, NOT an engine-issued SiN projectile, actual SourceVR loaded game module, or Quest 3 headset run.

Standalone tests (no original commercial files required):

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target sin_original_rifle_grenade_launch_tests
    ctest --test-dir build -R '^sin_original_rifle_grenade_launch_tests$' --output-on-failure

## Next gameplay and native engine blockers

Recover original player/NPC position and direction derivation, weapon attack timing and animation, failure-order and ammo edge semantics, original grenade physics/trace collision and precise detonation predicate, engine owner/attacker and damage/network callbacks. Bind the reconstructed player weapon and grenade projectile to real SourceVR-compatible ARM64 entity factories only after modern game interface method signatures, real native host/deployment path and the original old-to-new engine ABI adapters are verified.

This is executable SiN gameplay reconstruction, NOT a finished server.dll, client.dll, or standalone VR game. No proprietary DLLs, game data, APK, models or original native ELF files were committed.
