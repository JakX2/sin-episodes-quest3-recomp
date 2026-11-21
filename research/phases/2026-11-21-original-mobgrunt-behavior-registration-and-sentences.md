# Phase535: original MobGrunt eight embedded behaviors, registration order and sentence-parameter initialization

## Scope and authenticated original

This phase continues Phase534's seven verified `CAI_*` factories in the user's SHA-pinned retail 2006 SiN Episodes: Emergence `server.dll`. Scope is strictly the Grunt-specific NPC lifecycle; it does not reproduce HL2 model loading, physics, surfaces, map rendering, props or other already-functional engine systems. The commercial original remains private.

- Original `server.dll` SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Original `client.dll` SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original MobGrunt primary vtable: `0x2249AAA4`, independently RTTI-confirmed in Phase534 as `CNPC_MobGrunt`.
- `+0x3F8` post-construction wrapper `0x22359210`, **57 complete bytes**.
- `+0x76C` behavior-register virtual `0x2235F6E0`, **438 complete bytes**.
- `+0x7AC` sentence-key virtual `0x22353F80`, **6 complete bytes**.
- Member initializer at `0x22358FA0`, **238 complete bytes**.

The new verifier additionally SHA-authenticates original base Grunt constructor `0x22360D80` (338 bytes), dynamic-array insertion method `0x22055670` (98 bytes), and common behavior-owner binding method `0x220CDBD0` (10 bytes). Total **7 complete methods, 1,185 original bytes**. Eight separate base-constructor CALL instructions are checked as well as eight registered embedded offsets and each corresponding 2006 x86 RTTI descriptor.

## What `+0x76C` actually does

`0x2235F6E0` receives the `CNPC_MobGrunt*` owner. It uses the collection at `owner+0xC1C` and its count field at `owner+0xC28`. Eight times, it computes the address of an already-constructed embedded behavior object, inserts that address at the collection's previous length via `0x22055670`, dispatches the behavior's virtual **byte +0x04** passing the NPC owner, then stores `owner+0xC14` into the behavior's field at **`+0x0C`**. The shared virtual method `0x220CDBD0` is just the two-instruction owner-field store at behavior `+0x04` followed by `ret 4`.

Original iteration order, **not numerical order by member address**, with independently recovered original constructor/vtable identities:

| # | Owner offset | RTTI-confirmed exact class name | Original embedded constructor | Original installed vtable |
|---:|---:|---|---|---|
| 1 | `0xCFC` | `CAI_RappelBehavior` | `0x220DEC20` | `0x223F956C` |
| 2 | `0xCD8` | `CAI_FuncTankBehavior` | `0x22393E00` | `0x224ABEC4` |
| 3 | `0xD20` | `CAI_TakecoverBehavior` | `0x22399CD0` | `0x224AE624` |
| 4 | `0xDA0` | `CAI_BeserkBehavior` | `0x223925C0` | `0x224AB88C` |
| 5 | `0xF88` | `CAI_ActBusyBehavior` | `0x22391BB0` | `0x224AB494` |
| 6 | `0xDB8` | `CAI_AssaultBehavior` | `0x220D0C80` | `0x223F5544` |
| 7 | `0xDE8` | `CAI_StandoffBehavior` | `0x220E2850` | `0x223FA1B4` |
| 8 | `0xEB8` | `CAI_FollowBehavior` | `0x220D6690` | `0x223F6B7C` |

The original constructor `0x22360D80` constructs all eight embedded members, **in a different order**: FuncTank, Rappel, Takecover, Beserk, Assault, Standoff, Follow, ActBusy. `CAI_BeserkBehavior` is spelled *Beserk* in the original RTTI. These behaviors are not eight `new` allocations by the registration function: they were **embedded in the already-created `0xFD8`-byte NPC object**. A port adapter must not allocate them as unrelated standalone entities just because registration iterates eight times.

The original method ends by returning `AL=1`; there is no explicit failure/rollback branch visible inside the 438-byte registration routine. Our new host-only reference can refuse callbacks to avoid silently continuing with missing components. This protective fail-closed policy is **independent port engineering**, not a claim about native Win32 success/failure logic.

## Sentence-parameter key after behavior registration

Original `+0x3F8` wrapper does the following in strict order:

1. Call `0x22031D50` for the seven AI factories and six subsequent wiring operations. Abort on `AL=0`.
2. Call Grunt virtual `+0x76C`, which maps to `0x2235F6E0` and registers the eight embedded behaviors. Abort on `AL=0` (though the exact concrete method returns one).
3. Call Grunt virtual `+0x7AC`, which maps to `0x22353F80`. It returns a **constant string pointer** at `0x2248CD64` to `NPC_Grunt.SentenceParameters`.
4. Call `0x22358FA0` with `this=owner+0xCC0`, the owner pointer and the returned key. Then return `AL=1`.

The 238-byte `0x22358FA0` additionally performs global parameter availability checks and optional parsing via `0x221B4EA0`. We have not assigned speculative public meanings to the global fields at `0x2259C6C0`, `0x2259C6BC`, `0x22534E20`, `0x22534E1C` or `0x225721B0`; the independent host must supply a valid sentence-parameter initialization operation, not memcpy a 2006 object into a Quest process.

## Independently authored implementation and tests

The additions are:

- `include/sin_compat/original_grunt_behavior_postinit.hpp` and `src/original_grunt_behavior_postinit.cpp`: exact 8-element registration metadata, explicit array/owner slots, non-overlapping callback stages and non-reentrant fail-closed host model.
- `include/sin_compat/original_grunt_full_ai_postinit.hpp` and `src/original_grunt_full_ai_postinit.cpp`: a pipeline that executes the existing Phase534 core initializer before Phase535 behavior registration/sentence binding, matching the original `0x22359210` gates.
- `tests/test_original_grunt_behavior_postinit.cpp`: ordered 25 host actions (8×3 registration operations + sentence stage), failure at each, exception capture, retry and reentrancy checks.
- `tests/test_original_grunt_full_ai_postinit.cpp`: 13 Phase534 calls followed by 25 Phase535 calls, and prevents later stages when an earlier stage fails; checks a reentrancy refusal.
- `research/tools/audit_sin_original_grunt_behavior_postinit.py`: exact SHA256 PE32 provenance; seven complete method hashes totaling **1,185 bytes**; original vtable entry checks; 8 constructor edges, 8 append edges, 8 member-offset references, 8 virtual owner dispatches and 8 final pointer stores; 8 RTTI names; sentence string and member offset; synthetic mutation/self-test.
- Root `CMakeLists.txt` adds both libraries and three new CTest cases.

Local original-Pe32 audit against the owner's supplied `bin.zip` succeeded. Local C++17 GCC and Clang strict `-Wall -Wextra -Wpedantic -Werror` tests succeeded. Local CMake/CTest: **3/3 pass**. Synthetic CI may confirm build but **does not contain commercial game DLLs**.

## Relationship to owner's retail Grunt experiments

The owner submitted two retail-engine console captures from map `SE1_U4Lab05` (`condump000.txt`, `condump001.txt`), summarized in separate Phase534 research notes. They prove real retail `SCHED_GRUNT_RANGE_ATTACK1` tasks for Pistol and Assault Rifle Grunts and, for the Assault Rifle Grunt, a failed `SCHED_GRUNT_MOBILITY_SIDESTEP`, `SCHED_GRUNT_COMBAT_FAIL` fallback and `COND_CAN_RANGE_ATTACK1` break back into ranged fire. The presence of a registered `CAI_AssaultBehavior` at original `+0xDB8` is consistent with the separate original assault behavior/rally-point research, but **neither console trace independently prints the eight behavior pointers**. Do not mislabel tasks or task counters as native object-construction validation.

## Remaining blockers

These are **host-side reference libraries**, not an operational original-engine NPC or a new playable `server.dll`. Windows x86 integration still requires actual `ServerGameDLL003` and `ServerGameEnts001` ABI, engine-owned `CAI_*` objects / edicts / network classes, and the Grunt's real behavior registration and schedule selection within an initialized game engine. The next meaningful work is a Windows x86 entity/AI host integration rather than rebuilding original physics, assets or materials. Android and Quest 3 follow a working PC build.