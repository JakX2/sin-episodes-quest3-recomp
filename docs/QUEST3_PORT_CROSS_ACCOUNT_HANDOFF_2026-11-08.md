# SiN Episodes: Emergence -> standalone Meta Quest 3: cross-account handoff

**Prepared 8 November 2026. Project owner: JakX2. Source of truth: GitHub.**

This document exists so development can resume from a DIFFERENT ChatGPT
account, another assistant, local tools or another collaborator without
relying on old chat memory. It contains no private binaries or credentials.

## READ THIS FIRST

The game is **SiN Episodes: Emergence (2006)**. The target is an
**independent, native Android ARM64 standalone VR version for Meta Quest 3**.
"Q3" means **Quest 3**, never Quake III or id Tech 3.
The intended outcome is a **final, playable original SiN campaign in VR**,
not a view-only map, an inert DLL, or an Android diagnostic program.
Reuse a lawful engine/VR runtime only when binary interfaces and distribution
permissions are resolved; respect the owner's original game assets.

**State now: NOT playable, NO finished Quest APK.** Tested engine-neutral
reconstructed pieces, not full gameplay inside native SourceVR. The project
is an evolving reverse engineering / independent reconstruction effort.
Do not call an emulator smoke test "on-headset validation".

GitHub repository:
https://github.com/JakX2/sin-episodes-quest3-recomp

**Most recent continuation branch as of this document:**
feature/sin-gameplay-entity-factory-bridge-20261108

This branch is stacked on #152. All work stays in isolated **DRAFT PRs**
and should NOT merge into main or overwrite user assets without explicit
owner instruction.

## Current draft PR chain (each base is previous PR's branch)

| PR | Stage | Proven or implemented result |
| --- | --- | --- |
| #145 | Factory registration | Original SiN vs SourceVR native interface mismatch |
| #146 | Native loader audit | GAMEBIN, native .so path, module-name translation, dlopen |
| #147 | ARM64 native module names | Separate inert libserver.so and libclient.so, safe CreateInterface |
| #148 | Actual original/native vtables | SiN 31 server/48 client vs SourceVR ARM64 47 server/78 client methods |
| #149 | Startup dispatch | ARM64 client init slot0, shutdown slot2, server description slot12 |
| #150 | Original DLLInit/Init requests | 16 server and 23 client required original interface lookups |
| #151 | Original grenade runtime | Projectile think, AI danger, explosion, blast and once-only guard |
| #152 | ARNADE firing path | Rifle player/NPC grenade launch, ammo and generation-safe staged entity callbacks |
| #153 (this branch, once PR is created) | Original touch collision gates | x86 original +0x11C, +0x120 mask4, +0xFC, +0x795 and virtual Detonate slot250; C++ adapter + tests |

PR URLs have predictable repository form:
https://github.com/JakX2/sin-episodes-quest3-recomp/pull/152
(choose any number above). Earlier stacked PRs #1..144 also preserve
substantial gameplay, NPC assault, original 2006 map/entity analysis,
weapon runtime, C APIs and XR math, even though they are not merged.

Before changing anything, **inspect live GitHub PR/branch/CI status**.
If automation added a newer PR, resume from its head, not this
snapshot. Do NOT restart the project from a stale main checkout.

## Verified private source files (NOT in repository)

Owner-uploaded privately in previous sessions:

- bin.zip: contains genuine original SiN 2006 client.dll and server.dll.
- bin 1.zip: contains additional original installed game bin files.
- source vr.z01.bin, source vr.z02.bin, source vr.zip: parts of an
  owner-supplied SourceVR 0.5.5 archive; rejoined privately when needed.
- Older technical notes "SiN Episodes Quest 3.txt",
  "Quest 3 Port Milestone.txt", "SiN Phase 102 Update.txt" and
  "Sin ep part 2.txt".

Pinned original SHA256:
- server.dll = 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
- client.dll = 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

**NEVER upload/commit any original proprietary DLL, SourceVR .so/APK,
game BSP, models, sound, SDK-owned source, personal data or credentials
to the public GitHub repository.** Keep originals in a private work
directory, confirm SHA256, commit independently authored C++ and
metadata-only audits. Verify compatibility, don't redistribute them.

If switching ChatGPT accounts, **this GitHub repo will still exist
under the same owner** provided access to it remains. The *new*
ChatGPT account will need to connect/authorize GitHub separately,
receive these private archives from the owner if genuine binary
revalidation is needed, and receive this handoff explicitly.
Do not assume memory, Project file attachments or scheduled
automations automatically migrate across accounts.

## Current supported functionality, do not exaggerate

- Extensive real original 2006 x86 DLL SHA-checked class/RTTI,
  vtable, factory, event, weapon/NPC and map-input evidence.
- Independently authored portable C++17 gameplay: weapons,
  reload/viewmodel, assault NPC goals, JC/Jessica scene handlers,
  grenade_ar flight/detonation, rifle secondary firing,
  contact gate adapter.
- C ABI libraries and diagnostic native Android ELF outputs;
  robust desktop tests and Android ARM64 cross-builds.
- Android x86_64 Bionic emulator has run diagnostic and simulated
  gameplay unit tests; this DOES NOT load an actual original SiN
  mission through a compatible native game DLL.
- Some XR-to-Source pose mapping math tests; no proven native Quest
  stereo rendering, controller handling or campaign session.

## Fundamental compatibility blocker

Original SiN 2006 Windows modules are 32-bit PE DLLs:
factory server "ServerGameDLL003", client "VClient011";
31-slot server and 48-slot client primary game vtables.

SourceVR 0.5.5 native AArch64 uses ELF libraries:
factory server tries "ServerGameDLL010" through "ServerGameDLL004";
client requires "VClient017"; 47-slot native server and
78-slot native client game vtables. Direct aliases, stale x86
pointers or guessed virtual methods ARE NOT compatible.

SourceVR's public content-mod launcher path does not support mods
with their own client/server game DLLs. A genuinely supported custom
native game host, engine source integration or separate standalone
loader/deployment strategy remains necessary. Current libserver.so
and libclient.so are *inert, fail-closed probes* that do not implement
IServerGameDLL/IBaseClientDLL.

Additional interface mismatch: original game requires
VSoundEmitter001 (among many verified DLLInit dependencies)
while investigated native SourceVR registers VSoundEmitter002
with nonidentical observed vtable semantics. Never fake it.

## Exact newest original collision evidence

Original CGrenadeAR::touch at x86 server VA
0x223142F0..0x22314353, 100-byte SHA256:
945fb670960ad71aba1a1e4f5e183122c35697fdef8f1021d9d99dae56197b91

- Checks contacted entity raw +0x11C nonzero.
- Rejects contacted entity raw flags +0x120 bit0x04.
- Calls contacted object's virtual method at +0xFC,
  compares returned reference with projectile helper.
- If both nonnull and different: requests assignment of 1
  to projectile raw byte +0x795 by real setter 0x222B2B40.
- If +0x795 is nonzero: calls virtual Detonate +0x3E8
  (= x86 table method 250), else returns.
- Original Spawn clears raw +0x795 at 0x22314C73.

Fresh source/verification:
include/sin_compat/original_ar_grenade_touch.hpp
src/original_ar_grenade_touch.cpp
tests/test_original_ar_grenade_touch.cpp
research/tools/audit_sin_original_ar_touch_gate.py
research/tests/test_audit_sin_original_ar_touch_gate.py
research/phases/sin-original-ar-grenade-touch-branch-gates.md

**Important:** Runtime::armed() in portable PR #151 is a one-shot
detonation guard, NOT original raw +0x795 networked byte. The new
adapter keeps the two independent, leaving true C++ field setter,
unknown helper semantic identity and native replication to a
verified engine host.

## Build and audit reproducibility

Clone and checkout the current branch:

    git clone https://github.com/JakX2/sin-episodes-quest3-recomp.git
    cd sin-episodes-quest3-recomp
    git fetch origin
    git checkout feature/sin-gameplay-entity-factory-bridge-20261108

Native desktop C++:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel 2
    ctest --test-dir build --output-on-failure

Targeted standalone C++:

    cmake --build build --target sin_original_ar_grenade_touch_tests
    ctest --test-dir build -R '^sin_original_ar_grenade_touch_tests$' --output-on-failure

Original-free Python regression:

    python3 -m unittest discover -s research/tests -p 'test_audit_sin_original_ar_touch_gate.py' -v

Private ORIGINAL binary audit, on authorized local copy:

    python3 research/tools/audit_sin_original_ar_touch_gate.py --server /private/original/server.dll

ARM64 with an installed official Android NDK (see CI for pinned
NDK revision and environment setup):

    bash scripts/build_android_arm64.sh

Android emulator CI additionally builds and executes
sin_android_original_ar_grenade_touch_tests on real x86_64 Bionic
and archives an evidence log. That is a fake-host unit test,
not Quest execution.

GitHub Actions:
https://github.com/JakX2/sin-episodes-quest3-recomp/actions

## PR #154: verified original entity factories, now behavior-backed

The most recent new stacked DRAFT PR is:
https://github.com/JakX2/sin-episodes-quest3-recomp/pull/154
Head: feature/sin-gameplay-entity-factory-bridge-20261108
Base: PR #153 branch. No merge.

A genuine SHA-pinned 2006 server.dll audit found exact original
entity-registry getter usage for two source-proven classes:
- weapon_assault_rifle: registration 0x223C4F60, classname literal
  0x22477C30, original entity-factory object 0x2252D4FC.
- grenade_ar: registration 0x223C4FC0, classname literal
  0x22477230, original entity-factory object 0x2252D500.

New real independent C++17 host-neutral factory:

include/sin_compat/original_gameplay_entity_factory.hpp
src/original_gameplay_entity_factory.cpp

It registers these exact classnames into an explicit host registrar,
creates an unowned real portable Assault Rifle world pickup,
binds its original WeaponSession when picked up by a verified
generation-safe player, creates the real CGrenadeAR flight and
detonation Runtime, and binds the actual touch flag+blast logic
with a required original network-field setter callback. Failed
registration is rolled back; unknown map classnames are not
falsely accepted; it protects against stale generations and
reentrant entity callbacks.

Tests:
tests/test_original_gameplay_entity_factory.cpp
research/tools/audit_sin_original_rifle_grenade_factories.py
research/tests/test_audit_sin_original_rifle_grenade_factories.py

Full phase documentation:
research/phases/sin-original-rifle-grenade-gameplay-entity-factory-bridge.md

CMake, Android ARM64 and Android Bionic emulator mock-host tests
are included. The native SourceVR 47/78 vtable-compatible game
server and genuine engine entity registry are STILL UNIMPLEMENTED.
PR #154 cannot eliminate the actual runtime unknown-entity errors
until the real engine loads these callbacks and creates game
actors with correct lifetimes, network semantics and assets.

IMPORTANT: Among the 49 unknown classes in the Drive logs,
only weapon_assault_rifle is implemented by this two-name
bridge. grenade_ar is an original class verified from original
rifle firing but wasn't in the five runtime error inventories.
Other original classes, including trigger_dynamic_difficulty
and MobGrunt variants, still require a concrete runtime.

## New real map runtime evidence: five Google Drive text logs

The owner has supplied a folder of actual map load console logs,
containing five .txt files for Arena, Finale, Highrise, U4-to-Pit
and Intro-to-U4. All five were inspected on 8 November 2026.
The durable, source-attributed inventory is:

research/phases/sin-original-map-runtime-unknown-entity-logs-nov08.md

Google Drive folder:
https://drive.google.com/drive/folders/1ExaddVDbt0UTrvec4jJdg1lXVdn6AAEJ?usp=sharing

Across those five logs, **49 distinct game entity classnames** were
rejected in **1,531 unknown-entity messages**. Most common:
npc_druglab_grunt_pistol (195), npc_druglab_grunt_assault_rifle (103),
prop_exploding_barrel (92), trigger_dynamic_difficulty (88),
npc_druglab_grunt_elite (69) and npc_druglab_grunt_scattergun (66).
These are error counts from potentially repeated load attempts,
NOT unique world placements or unimplemented C++ functions.

CRITICAL: The original 2006 server.dll has a proven static factory
registration for trigger_dynamic_difficulty, but the runtime that
produced these text logs still could not instantiate it. This
confirms why we need a REAL native engine-host game module
and entity registry, not only decompiled classes or mock-host tests.

The logs also report point_template target failures, missing
cinematic .vcd scenes, Highrise prop_door_rotating model
door_options, VMT material parse issues and some save open errors.
These are separate from the primary missing game factory problem.
No raw console logs, screenshots or proprietary game content were
copied to GitHub; the public report records aggregate metadata only.

## High-value next development tasks, in order

1. Inspect live branch/PR head and latest GitHub Actions result.
   Fix actual failures before opening another stacked PR.
2. Finish original grenade think callback x86 0x22314360:
   separately identify original +0x795 flag transitions,
   +0x7C0 timer condition, actual touch-versus-think ordering,
   original collision/trace predicate and referenced entity
   accessor semantics. Implement independently, with real
   private PE32 checks and synthetic regression; avoid guessed
   physics names and do not confuse original flag with one-shot.
3. Link the verified touch/think/detonate path into an actual
   engine-owned projectile entity once native game ABI, entity
   creation and engine collision callbacks can be validated.
4. Recover needed native IServerGameDLL/IBaseClientDLL startup
   method signatures and implement a **real** compatible module
   or an approved standalone host. Both game factories must
   initialize successfully and spawn a SiN map and player.
5. Connect actual NPC/weapon/damage/map I/O/client/network
   gameplay, assets from the authorized user's copy, and stereo
   OpenXR rendering/tracked Quest inputs.
6. Release only after real headset installation and the campaign
   can be played end-to-end with stable input, combat, audio,
   saving, performance and acceptable comfort.

Prefer **real functioning code and end-to-end progress** over
another purely illustrative audit. Use CI for every PR, leave
stacked PRs draft and unmerged. Never claim an inert module or
synthetic Android host is a playable game.

## Paste this into a new account/chat to resume

> Continue the SiN Episodes: Emergence 2006 standalone **Meta Quest 3
> Android ARM64 VR port**, NOT Quake III. The source of truth is
> GitHub JakX2/sin-episodes-quest3-recomp, especially
> docs/QUEST3_PORT_CROSS_ACCOUNT_HANDOFF_2026-11-08.md.
> Connect to my GitHub and first inspect the newest stacked DRAFT
> PR/CI head, starting at PR #154 if no newer work exists.
> Original 2006 server.dll/client.dll and SourceVR 0.5.5 ELFs
> are private and should only be read from owner-provided files.
> Do NOT commit them. Priorities: real original gameplay DLL
> reconstruction, precise CGrenadeAR think/touch behavior,
> native ARM64 IServerGameDLL/IBaseClientDLL, real entity host,
> campaign loading and Quest OpenXR. Write tested source code and
> stacked draft PRs, never merge without my request; do not
> pretend the diagnostic libserver/libclient probes are a game.
> Only ask me to test after a genuinely installable playable
> release candidate is verified. Use this handoff for all
> details and continue working rather than restarting research.

## Ownership and account transfer notes

GitHub repositories/commits, open PRs, source, documentation,
workflows and workflow artifacts (subject to their retention)
are independent of a ChatGPT plan. Keep a local clone for redundancy.
The previous account's chat memory, tool connections, personal
file uploads and hourly automation are separate product state.
Reconnect GitHub and re-share this handoff/private assets in
the new account; recreate recurring automation there if desired.

**Do not copy secrets into the new chat or this document.**
