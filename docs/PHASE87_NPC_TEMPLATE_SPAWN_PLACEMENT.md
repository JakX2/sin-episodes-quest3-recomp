# Phase 87: SDK-grounded NPC template placement and destination selection

**Evidence:** Owner-supplied SiN Episodes Hammer FGD and original SDK VMF configuration metadata, plus independently written C++17 host-neutral logic. This is not original Ritual C++ gamecode, a SourceVR game module, or a headset test.

## Exact SDK editor contract

The owner SDK base.fgd defines npc_template_maker with TemplateName, Radius (default 256), DestinationGroup, CriterionVisibility (0 visible, 1 hidden, 2 any), and CriterionDistance (0 nearest, 1 farthest, 2 any). Spawnflag 256 forces radius spawning, and inherited flag 128 means Don't Spawn While Visible. Template maker inputs are SpawnNPCInRadius, SpawnNPCInLine, and ChangeDestinationGroup. The info_npc_spawn_destination entity has a ReuseDelay (default 1 second), RenameNPC, and an OnSpawnNPC output when used.

**This is the original FGD editor specification, not the original executable's full placement algorithm.**

### Owner SDK map summary (original Hammer source files only)

| Map | Makers | Named NPC templates | Destination points | Makers with DestinationGroup | Flag 256 force radius | Flag 128 block visible |
|---|---:|---:|---:|---:|---:|---:|
| se1_docks04 | 20 | 20 | 0 | 0 | 15 | 3 |
| se1_highrise01 | 54 | 54 | 22 | 10 | 0 | 26 |
| arena_office | 1 | 1 | 17 | 1 | 0 | 1 |

Docks radius settings are 0 (15) and 64 (5). Highrise radius is 256 (53) and 32 (1). All 39 destination points in highrise and arena use a 1-second ReuseDelay. None of these destination points has RenameNPC set, even though it is supported by the FGD. All 75 makers explicitly identify a TemplateName. These counts come from the independently authored private SDK aggregate audit and are **not** statistics from released compiled BSP campaigns.

## Portable gameplay implementation

- NpcSpawnPlacementPlanner and INpcSpawnPlacementWorld select a named destination or ask a host for a radius/line location.
- Selection checks generation-safe entity refs, matching destination group, available space, visibility criterion, host player location for nearest/farthest, finite world coordinates and destination reuse cooldown. Equal-distance or unspecified preferences select by stable entity ID. **Selection and fallback policies are reference-model decisions, not demonstrated Ritual parity.**
- A successful host-created template clone consumes the destination cooldown and fires a host callback for that destination's OnSpawnNPC. Failed clones do not consume cooldown.
- The host, not this library, implements actual cloning, RenameNPC application, collision traces, NavMesh/pathfinding, occlusion and ground/line/radius placement.
- NpcMakerPlacementHostAdapter plugs into the Phase 85/86 NpcMakerRuntime without altering its quota/live-cap/child lifetime contract. Explicit line/radius requests pass through maker.spawn_now to honor quota exhaustion.
- The placement input bridge handles actual SDK names ChangeDestinationGroup, SpawnNPCInRadius and SpawnNPCInLine plus existing BaseNPCMaker inputs. The new component does not expose engine-specific Source C++ APIs or expand the C ABI.

### Model limitations

- If a destination group is set but no eligible point exists, the reference reports NoDestination rather than falling back to radius. The original engine fallback is unknown.
- Host must sync the placement clock to the maker clock by calling adapter.set_time(t) before maker.advance_to(t). The library does not yet solve real engine tick synchronization.
- For a forced SpawnNPCInLine request, host-provided line geometry is used even if the maker has a force-radius flag; original flag/input precedence is unverified.
- The host must create a truly new child reference (and avoid reentrant callbacks). Real Source entity and template factories are unavailable.
- The example tests use wholly synthetic templates and geometry, not leaked or proprietary level data.

## Verification

Three native C++17 CTests:

1. sin_npc_spawn_placement_tests covers visibility/near/far/don't-care filters, strict monotonic time, exact reuse expiry, stale destination generations, deterministic ID ordering, invalid settings, missing templates, host creation rejection, forced radius and line.
2. sin_npc_maker_placement_integration_tests covers two destination points through the existing maker runtime, live-child caps, quota exhaustion, a forced line spawn, and quota re-enable.
3. sin_npc_maker_placement_inputs_tests checks exact SDK scripted input names, group changes, radius/line placement and quota enforcement.

Three local executables passed GCC C++17 strict warnings and AddressSanitizer/UBSan. Ten synthetic original-free Python VMF tests check default values, invalid fields and nonleakage of target/template identifiers.

Reproduce native, Python and private aggregate audits:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure
    python3 -m unittest discover -s research/tests -p 'test_*.py'
    python3 research/tools/audit_sin_sdk_spawn_placement.py /private/SinEpisodesSDK.zip

No original SDK files, DLLs, APKs, names, map coordinates or asset bytes are committed.

**Still blocked:** real NPC template cloning and engine entity lifecycle, original AI, BSP loading, SourceVR-compatible game module interfaces, and campaign validation. Issues #12 and #20 remain open. All physical VR/headset testing is deferred.
