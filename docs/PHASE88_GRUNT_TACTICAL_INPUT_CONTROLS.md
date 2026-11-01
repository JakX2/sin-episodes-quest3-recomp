# Phase 88: Original SiN grunt tactical-input state and equipment choices

**Evidence tiers:** Original, owner-held 2006 SiN Episodes server.dll + SinEpisodes.fgd confirm named input registrations, datamap field names and FGD editor defaults. The C++17 component is an independently authored, engine-neutral reference state machine. It is **not** a recovered SinTEK combat AI algorithm, Source engine object, actual NPC perception, or playable Quest module.

## Exact provenance

- Original server.dll SHA-256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
- Original SinEpisodes.fgd SHA-256: 4e803f2e23e09ee2dd275064ee71be133536549d34321ff2c7237320ac8d60f9
- The independently authored verifier checks original input names near their corresponding input-handler registration strings, plus grenade datamap field/member pairs. It verifies **name registration**, not what the handlers actually do internally.

### Grounded input and config contract

| FGD input / key | Original source description | Corresponding DLL registry |
|---|---|---|
| LookOn(void) | Look normally | LookOn / InputLookOn |
| LookOff(void) | Don't look for myself; use other squad members' eyes | LookOff / InputLookOff |
| StartPatrolling(void) | Patrol when idle or alert | StartPatrolling / InputStartPatrolling |
| StopPatrolling(void) | Stop patrolling when idle or alert | StopPatrolling / InputStopPatrolling |
| ThrowGrenades(bool) | Toggle heavy grenade usage | ThrowGrenades / InputThrowGrenades |
| NumGrenades | Default 5; choices 0 through 10 and 999999 unlimited | NumGrenades / m_iNumGrenades |
| GrenadeHeight | Editor default 0; vertical distance grenade heuristic | GrenadeHeight / m_flGrenadeHeight |

BaseGrunt FGD spawn flag **65536** means initial LookOff; flag **16777216** means don't secondary fire. Additional flags for sitting, DNA scanner, one-hit kill and ammo drops are defined in the SDK but not implemented in this component.

The original FGD also defines these enemy defaults, matched to classnames found in the original DLL:

| Original classname | FGD default weapon |
|---|---|
| npc_sintek_grunt | weapon_assault_rifle (also permits scattergun and NPC pistol) |
| npc_druglab_grunt_pistol | weapon_npc_pistol |
| npc_druglab_grunt_scattergun | weapon_scattergun |
| npc_druglab_grunt_assault_rifle | weapon_assault_rifle |
| npc_druglab_grunt_elite | weapon_chaingun |

Other NPC subclasses, such as jetpack grunts, have separate FGD base classes and are outside Phase 88's grounded model.

## Portable C++17 implementation

The new GruntTacticalControls, in include/sin_compat/grunt_tactical_controls.hpp and src/grunt_tactical_controls.cpp, registers generation-safe EntityRef handles with SDK-constrained equipment and grenade configuration.

- Tracks LookOn/LookOff, Start/StopPatrolling, and ThrowGrenades(bool) as typed, idempotent input-state changes, rejecting bad/unknown input names and invalid bool payloads.
- A snapshot returns **preferred vision source (self vs squad)**, patrol request only while the host reports Idle/Alert, grenade-aggression request, secondary-fire permission based on the FGD spawnflag, configured grenade height and remaining grenade inventory.
- Grenade count decrement happens **only after a host reports a successful grenade throw**; the unlimited FGD sentinel is not decremented. The runtime does not decide when a throw is legal, which trajectory is safe, which target is perceived, or how the original dynamic difficulty modifies enemy choice.
- Rejects unsupported grunt classes/equipment, nonfinite or negative grenade-height values, unsupported grenade counts and stale/recycled generation handles.
- The reference's strict input parser, grenade-consumption policy and registration limits are intentionally testable decisions; original C++ handler details were **not** established by string registries or editor text.

## Tests

Two new C++17 tests are registered with CMake and compiled into the existing portable native / Android ARM64 library:

1. sin_grunt_tactical_controls_tests: FGD defaults, special flags, squad sight, patrol states, bool input validation, weapon choices, finite/unlimited grenade consumption, stale handles and validation failures.
2. sin_grunt_tactical_entity_io_tests: a **synthetic** map trigger schedules LookOn, StartPatrolling and ThrowGrenades through the existing Phase 74 EntityIoRuntime, verifying timestamp ordering, caller/activator propagation, one-shot bindings and post-destruction safety. The synthetic trigger wiring is not taken from any released SiN level.

Eight independently authored Python tests validate the read-only provenance auditor against fabricated FGD and DLL-string fixtures, including detached/missing handler names, invalid defaults, corruption and strict SHA checking.

## Reproduction (owner retains original assets privately)

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure
    python3 -m unittest discover -s research/tests -p 'test_*.py' -v
    python3 research/tools/audit_original_grunt_inputs.py /private/server.dll /private/SinEpisodesSDK.zip

The last command requires the original owner-held DLL and SDK archive, which are **not** part of the public repository. The new public verifier and unit tests contain no original DLL/SDK content.

## Still missing for authentic combat

The original server.dll functions implementing actual senses, squad enemy knowledge, AI schedules and task decisions, animation, covers, grenade trajectory, secondary-fire probability, firing delays, precise grenade-height logic and dynamic difficulty must still be studied in detail. This phase intentionally stops short of claiming AI parity; it supplies a reproducible state/input seam for a future engine adapter.

No headset testing, live NPC execution, BSP load, native SourceVR game-module implementation or copyrighted game asset publication. Engine issue #12 and campaign runtime issue #20 stay open.
