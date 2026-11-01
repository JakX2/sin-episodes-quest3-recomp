# Phase 86: Source SDK NPC-maker inputs, quota-rearm rule, and scripted integration

**Evidence levels:** Original owner-held SiN Episodes SDK \`base.fgd\` is source material for editor input names and the specific exhausted-quota warning; code is an **independently authored host-neutral C++17 reference implementation**. No original Source/Ritual C++ implementation, game binary, level, Android gameplay module or physical headset is exercised.

## What the original SDK actually says

The BaseNPCMaker class in \`SinEpisodesSDK/bin/base.fgd\` declares:

| Original input | SDK-documented effect |
|---|---|
| Spawn(void) | Spawn an NPC |
| Toggle(void) | Toggle maker enabled/disabled |
| AddMaxChildren(integer) | Add to the total number that can spawn |
| SetMaxChildren(integer) | Set the total number that can spawn |
| SetMaxLiveChildren(integer) | Set the maximum simultaneously live children |
| SetSpawnFrequency(float) | Set the seconds between NPC spawns |
| Enable / Disable | Inherited from EnableDisable base class |

Crucially, the SDK description for **AddMaxChildren** and **SetMaxChildren** states that a previously exhausted maker **will not begin spawning again merely because its quota increases; it must be re-enabled with the Enable input**. This is stronger evidence than an inferred implementation detail.

The SDK also documents \`SpawnFrequency=-1\` to spawn again when the **last NPC dies**, and \`MaxLiveChildren=-1\` as unlimited simultaneous children. This phase fixes the earlier reference model's error that could spawn prematurely on re-enabling a death-driven maker with existing live children.

The SDK alone does not establish the exact C++ method implementations, internal game clock, vis checks, template placement, or how the real engine handles all unsupported parameter values. These remain model assumptions, documented below.

## New native C++ implementation

The existing \`NpcMakerRuntime\` now exposes:

- \`add_max_children\` and \`set_max_children\`, with signed overflow/underflow protection and finite nonnegative totals;
- \`set_max_live_children\` with special \`-1\` unlimited limit and rejection of zero or other invalid negative limits;
- \`set_spawn_frequency\` with positive intervals or \`-1\` death-driven mode;
- \`needs_reenable\` and \`is_enabled\` query functions for host use.

Once a finite quota is exhausted, the model records a re-enable latch. Changing the quota leaves that latch set; the maker's next automatic spawn and explicit Spawn are blocked until an explicit **Enable** call. The Enable input rearms the existing maker, even if its enabled flag already remained true. A new exhaustion cycle can emit \`OnAllSpawned\` again, while independently authored Hammer output max-fire limits still apply across those cycles.

Changing the maximum live-child count does **not** delete existing children. Increasing the limit can unblock scheduled spawning; decreasing it preserves existing children but blocks additional ones until capacity is available. After changing \`SpawnFrequency\`, a positive interval reschedules a future attempt, while \`-1\` waits for the final live child to die.

The new \`npc_maker_input_bridge.hpp/.cpp\` recognizes the **exact** SDK input names (including Enable, Disable, Toggle and Spawn). It strictly parses decimal integer and float input parameters, rejecting junk suffixes, nonfinite floats, malformed strings, integer overflow and out-of-range settings. The bridge returns typed status and, on Spawn, typed events. **It is not a Source \`variant_t\` or a registered engine entity input handler**.

## Verification

- **test_npc_maker_dynamic_inputs.cpp:** Validates exhausted quota, Add/Set mutation, explicit re-arm requirement, live limit changes, runtime frequency switches, -1 death-driven waiting, invalid float/int parameters, integer overflow, recycled entity references, and no unexpected auto-resume.
- **test_npc_maker_scripted_inputs.cpp:** Uses the existing Phase 74 \`EntityIoRuntime\` for a fully synthetic map: \`OnStartTouch\` enables an initially disabled maker; the maker exhausts its first child quota and emits \`OnAllSpawned\`; a delayed \`AddMaxChildren=2\` input increases the quota without spawning; another delayed \`Enable\` rearms it. Children are removed in turn and cause subsequent \`OnSpawnNPC\`, renewed \`OnAllSpawned\` and \`OnAllSpawnedDead\` messages. The map links and times are invented **for a test**; no original BSP/VMF output string is claimed to have that exact sequence.
- Previously implemented Phase 85 maker and entity I/O tests still pass, ensuring ordinary makers are not broken by the exhaustion latch.

The new source is compiled into the pre-existing portable native/Android ARM64 static library via CMake. The separate public C ABI remains unchanged. Local GCC C++17 builds use \`-Wall -Wextra -Wpedantic -Werror\` and passed both new tests plus the Phase 85 tests with AddressSanitizer/UBSan. GitHub CI independently builds the full project and runs CTests.

To reproduce:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

## Explicit limitations

- \`Spawn\` requires an enabled, non-exhausted maker and honors the re-enable latch in this reference model. **The FGD does not verify exactly how the original engine handles forced spawning on disabled makers.**
- A quota decrease to a total already spawned sets the rearm latch but does not retroactively emit an \`OnAllSpawned\` output. Real engine behavior is not verified.
- The reference schedules eligible makers immediately when rearmed, and unblocks after live limit changes. Actual Source thinking cadence and delays need original DLL or engine testing.
- There is no real \`npc_template_maker\` engine entity, \`TemplateName\` cloning, destination selection, visibility/radius spawn, Save/Restore, real NPC AI, physical headset execution or playable campaign.

**R1 engine integration issue #12 and campaign scripting issue #20 remain open.** No original SDK, maps, DLLs, APKs, models or assets are published in this branch.
