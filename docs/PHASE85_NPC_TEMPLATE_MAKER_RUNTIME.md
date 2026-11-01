# Phase 85: SDK-grounded NPC template-maker state machine

**Proof level:** Original owner-held *SiN Episodes* SDK FGD/map metadata plus independently authored C++17 reference logic, native tests, and native/Android compilation. This is **not** original Ritual's NPC-maker source, a native SourceVR game module, real NPC AI or a playable level.

## Original SDK definitions

The original \`base.fgd\` declares \`BaseNPCMaker\` and \`npc_template_maker\`, with the following editor-facing configuration and outputs:

| Original key / FGD declaration | SDK documentation |
|---|---|
| \`MaxNPCCount\` (default 1) | Total NPC count before exhaustion |
| \`MaxLiveChildren\` (default 5) | Maximum live NPCs; -1 means no simultaneous limit |
| \`SpawnFrequency\` (default 5 seconds) | Time between spawns; **-1** means replace when the last child dies |
| \`StartDisabled\` (default 0 via \`EnableDisable\`) | Requires enabling before automatic spawning |
| Maker spawnflag 32 | Infinite Children |
| Maker spawnflag 128 | Don't Spawn While Visible |
| Maker spawnflag 1024 | Don't use dynamic difficulty |
| \`OnSpawnNPC\` | Event when a child spawns; **child is the activator** |
| \`OnAllSpawned\` | All allotted NPCs have been spawned |
| \`OnAllSpawnedDead\` | Total quota exhausted and all spawned NPCs are dead |

The original FGD also defines inputs \`Spawn\`, \`Toggle\`, \`AddMaxChildren\`, \`SetMaxChildren\`, \`SetMaxLiveChildren\`, \`SetSpawnFrequency\` and \`SpawnNPCInRadius\` / \`SpawnNPCInLine\` on the template subclass. This phase implements **Enable**, **Disable**, **Toggle** and explicit spawning semantics in the portable runtime. The other inputs and special placement logic need their own verification and work; their mere FGD existence is not a claim of implementation.

Private inspection of the three SDK VMF source maps yielded these maker configurations:

| Map | Makers | All start disabled? | Total NPC quota distribution | Live-child-limit distribution |
|---|---:|---|---|---|
| \`se1_docks04\` | 20 | Yes | 1:10, 2:4, 3:1, 4:4, 6:1 | 1:13, 2:5, 3:2 |
| \`se1_highrise01\` | 54 | Yes | 1:37, 2:10, 3:7 | 1:38, 5:16 |
| \`arena_office\` | 1 | Yes | 1:1 | 6:1 |

The sample campaign maps use positive SpawnFrequency values (1, 2, 3, 4, 5, 7 or 9 seconds), whereas the arena map demonstrates the **-1 death-driven special case**. These are source-map settings, not recorded spawn times or demonstrated runtime behavior.

## C++17 gameplay implementation

\`include/sin_compat/npc_maker_runtime.hpp\` and \`src/npc_maker_runtime.cpp\` add a standalone, host-neutral \`NpcMakerRuntime\`:

- Maker IDs and child IDs are tracked by generation-safe \`EntityRef\`, preventing double-removal and stale slot reuse.
- Host controls actual child creation via \`INpcMakerHost::create_child\`; no entity templates, world geometry, NavMesh, animation or SinTEK NPC behavior are invented.
- Implements explicit enable, disable, toggle, and manual spawn requests, subject to registered quota/live-child limits.
- Schedules positive-frequency makers against finite monotonic host-provided time; \`-1\` makers can replace a child after the **last** live child is removed. A per-call attempt budget protects frames from unbounded catch-up.
- Reports **Spawned**, **AllSpawned**, and **AllSpawnedDead** as independent output events. Emission of output names and actual entity I/O is left to an engine host adapter. Event delivery ordering is deterministic in the synthetic tests.
- Rejects nonfinite or invalid frequencies, negative quotas, invalid live limits, missing/stale maker references, duplicate maker registration and failed child-creation handles.

**These timing choices are explicit reference-model assumptions rather than exact original behavior:**

- A newly enabled maker may attempt its first spawn on that same synthetic tick. The actual Ritual initial-delay convention is not verified.
- An explicit Spawn request requires an enabled maker. Original Source input behavior when disabled is not established by FGD alone.
- When a live-child slot becomes available, the synthetic maker resumes immediately from the host's current time rather than replaying all missed periods.
- A failed host spawn defers another positive-frequency attempt; the original engine's retry/error policy is unverified.
- The runtime does not implement player visibility constraints, destination selection, radius/line tracing, dynamic difficulty modifier selection, save/restore or quota-modifying inputs.

## Native tests and integration with Phase 74 output queue

Two new CTests:

1. \`sin_npc_maker_runtime_tests\` verifies disabled makers, enable/toggle behavior, quotas, live-child limits, duplicate death notifications, no-limit and infinite modes, deadline catch-up budgets, the \`-1\` death-replacement setting, invalid configurations, stale maker generation, and failed child creation recovery.
2. \`sin_npc_maker_entity_io_tests\` constructs a synthetic map event chain. A trigger's \`OnStartTouch\` enables a maker; the first child triggers \`OnSpawnNPC\`, a second child exhausts the quota and triggers \`OnAllSpawned\`, and its removal triggers \`OnAllSpawnedDead\`. Output events are forwarded to the real portable Phase 74 \`EntityIoRuntime\`, which propagates child activators and queued delays to a mock recipient. No original map scripting text is copied.

The new \`npc_maker_runtime.cpp\` is compiled into the existing portable static library **and its Android ARM64 NDK build**, with no changes to the 13-symbol v1 host C ABI.

Local native standalone tests compiled with GCC C++17 \`-Wall -Wextra -Wpedantic -Werror\` and passed with AddressSanitizer/UBSan. GitHub CI should independently run both new CTests, the existing native suite, Python synthetic research tests and Android jobs.

## Reproduction

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

Original SDK files remain with the owner and are **not** committed.

## Remaining implementation gaps

A real engine integration will need to instantiate a Source-compatible \`npc_template_maker\`, bind actual \`Spawn\`/Enable/Disable/Toggle inputs, create correct SinTEK entities from \`TemplateName\`, track actual NPC death, resolve dynamic difficulty, handle exhaustion adjustment and report actual output connections. The original 2006 DLL implementation and the SourceVR engine adapter remain separate proof requirements. **Quest/HMD testing is explicitly deferred.** Issues #12 and #20 remain open.
