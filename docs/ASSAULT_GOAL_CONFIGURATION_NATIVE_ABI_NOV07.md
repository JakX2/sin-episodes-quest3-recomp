# Original SiN assault goal SetParameters wired into native NPC C ABI

**November 7, 2026 | New host-side map I/O integration; stacked on PR #142**

## Original Windows x86 / SDK evidence

Original 2006 server.dll and its SDK FGD have previously been SHA256-pinned
and audited (see docs/ORIGINAL_GOAL_ASSAULT_BEGIN_INPUT_NOV06.md).
This phase does not claim fresh binary decompilation or a native Quest runtime.

| Original field or method | x86 evidence | Meaning |
| --- | --- | --- |
| ai_goal_assault rallypoint | goal +0x3C8 | m_RallyPoint name |
| ai_goal_assault AssaultCue | goal +0x3CC | 1 map input, 2 gunfire, 3 no wait |
| Goal actor setup hook | 0x220CFF00 | passes name and mode to SetParameters (0x220CFC00) |
| InputBeginAssault(void) | 0x220CFF30 | per-actor cue 1 plus COND_PROVOKED |
| CNPC_MobGrunt::InputAssault(string) | 0x22357E50 | direct NPC call to SetParameters with forced mode 3 at 0x22357E8B |

The original SetParameters ranks eligible matching rallypoints highest
priority first, then shortest squared 3D distance. Its native ownership
and cue-state transitions were recovered previously. See
docs/MOBGRUNT_ASSAULT_RALLY_DECOMP_NOV05.md and
docs/ORIGINAL_ASSAULT_SETPARAMETERS_OUTCOMES_NOV07.md.

## New engine-facing but engine-neutral bridge

include/sin_compat/assault_goal_configuration.hpp provides two C++17
adapters which call the actual separately built 8-export NPC shared C ABI:

1. configure_original_goal_assault_fanout takes a host snapshot of
   NPC references, original map rallypoint name and AssaultCue mode,
   then resolves each NPC session and asks the host to collect matching
   rallypoints and current NPC origin. It calls sin_assault_bind_v1
   with the exact requested mode, letting the compiled C ABI perform
   liveness, candidate, priority, distance and exclusive lock checks.
2. configure_original_mobgrunt_assault_input uses the same native
   binding path for one NPC and unconditionally forces original mode 3.
   It does not implicitly send BeginAssault or COND_PROVOKED.

The collector must implement real Source-compatible name/class matching
when a verified engine host becomes available. The bridge accepts only
host-supplied generation-aware refs, never x86 EHANDLE bytes.

A collected, empty candidate set is distinct from unavailable host
snapshot data (optional has no value). The resulting report includes
per-NPC native bind status and selected rallypoint token. It counts
invalid/duplicate actor refs, missing sessions, no candidate, lock
refusal, unresolved handle, expired generation, reentrancy, already-bound
session and callback failures. An empty rallypoint name or mode outside
FGD 1..3 is refused before touching the host.

These explicit error and duplicate policies are **new port protections**,
not recovered original C++ return types.

## Critical original ordering

Configuration does NOT signal BeginAssault: on a successful
SetParameters-like bind, configured mode is set and received cue is zero.
Only the later ai_goal_assault BeginAssault input calls
begin_original_goal_assault_fanout from PR #142, which requests the
original two NPC state resets, writes received cue 1, and signals the
host-registered runtime Provoked condition.

The direct MobGrunt Assault(string) input is separate, using mode 3
without silently delivering the goal input. Otherwise a scripted finale
could start combat in the wrong event order.

## Reproducible compiled tests

The existing tests/test_assault_goal_fanout.cpp now exercises the
new configuration boundary against the real compiled
libsin_assault_npc_c_abi.so. A synthetic level includes multiple NPCs
and shared rallypoint reservations, and tests invalid map modes,
duplicate/unresolved actor refs, two goal-configured NPCs, explicit
BeginAssault timing, already-bound refusal, direct MobGrunt mode 3,
empty rally list vs missing host snapshot, dead generation refusal,
collection exceptions and complete lease cleanup.

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target sin_assault_npc_c_abi sin_assault_goal_fanout_tests
    ctest --test-dir build -R '^sin_assault_goal_fanout_tests$' --output-on-failure

Android ARM64 builds the identical linked native C ABI integration
fixture; Bionic x86_64 emulator CI executes it. Those CI checks
do not register real NPC entities inside SourceVR or run an actual
SiN level.

## Remaining gating tasks

Actual SourceVR ARM64 game-module source, CreateInterface contract and
entity registration are still unavailable/unverified in this branch.
A verified host must implement class registration and Hammer I/O,
name lookup, NPC/session lifetime, condition registration, AI schedule
navigation and map outputs. Only then can the user-owned SiN campaign
assets be tested in-engine and eventually on Quest 3.

No proprietary client.dll, server.dll, maps, models or VCD assets
are committed here. This is not a playable Meta Quest 3 port.
