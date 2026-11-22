# Gameplay Phase606: actual original CNPC_MobGrunt runtime AI behavior priority

## Source, and separation from Phase605 savegame restoration

This pass examines exact original 2006 SiN Episodes: Emergence PE32 source from the owner's connected Drive SE1/bin.zip, authenticating server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 and client.dll SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

Phase605 proved Grunt vtable +0xA4 -> 0x2216DE40 is a **savegame RESTORE** method, not a live AI decision. Phase606 finds the **true runtime selection loop 0x2234E590**, which is directly invoked at original 0x2235E6EE from original **CNPC_MobGrunt vtable +0x434 -> 0x2235E4E0**, among several other authentic original callers. The selector itself is not a vtable entry and must not be confused with the Grunt restore wrapper.

The original behavior registration order was already independently authenticated in Phase535. All new method SHA256/boundary evidence below comes from the original 2006 executable, not the public 2013 Source SDK.

## Three newly whole-method-authenticated functions, 240 original bytes

| Role | Original VA | Whole length | SHA256 of complete original native body |
| --- | --- | ---: | --- |
| Runtime behavior priority selector | 0x2234E590 | **152** | 726b660c52b649fea86e5fff6018dbf27d67136ad951ffc51328ba22d14ff744 |
| MobGrunt behavior acceptance virtual +0x77C | 0x2216DDE0 | **5** | f4c6d7ae520f88aecb3ea65952e885437fa4a6ce4b5c3439a161d1c5d8e42863 |
| Behavior schedule bridge calling selector | 0x2234E730 | **83** | 38c124a7ea670e7d5277f7175d8f6114934eaf2adeaefe796785bcddddcee52f |

The full method spans terminate at 0x2234E628, 0x2216DDE5 and 0x2234E783, followed by exact original INT3 padding of 8, 11 and 13 bytes. This avoids the earlier mistake of treating several adjacent functions near 0x2234E510 as one large body.

## Verified original Grunt route and eligibility policy

Original **CNPC_MobGrunt primary vtable 0x2249AAA4**:
- vslot +0x434 = **0x2235E4E0**. Original instruction at 0x2235E6EE inside that Grunt schedule method is a true direct E8 CALL to **0x2234E590**. This makes the selector a Grunt runtime code path, not merely a neighboring unrelated NPC's virtual method.
- vslot +0x77C = **0x2216DDE0**. This complete 5-byte native method returns **AL=1 unconditionally** for the original MobGrunt implementation. Other classes could override this virtual and must be checked independently.

Actual source-backed control flow for 0x2234E590:

1. Read registered behavior array count at original owner+0xC28 and backing pointer at owner+0xC1C.
2. Iterate registered behavior pointers in order from index zero.
3. Invoke each candidate's virtual byte offset **+0x18**. If false, advance index without calling the owner acceptance virtual.
4. When candidate+0x18 is true, invoke owner virtual byte offset **+0x77C(candidate)**. If false, continue to later candidates.
5. If both are true, fetch exactly that registered behavior pointer again, pass it to original **0x2216E510**, the Phase604-verified current-pointer transition/lifecycle method, then return **AL=1**.
6. If all candidates fail, read former owner+0xC18, write NULL to owner+0xC18, and only when previous current pointer was nonnull invoke old behavior virtual+0x20, native shared cleanup 0x22054CA0, and owner virtual+0x778(old,NULL). Return **AL=0**.

For **CNPC_MobGrunt**, original owner virtual+0x77C always returns true. Therefore the first registered behavior whose vfunc+0x18 predicate succeeds wins. Do not assign a precise historical C++ member/method name to +0x18 until the individual behavior implementations are authenticated.

### Original priority sequence, not constructor address order

Phase535 authenticated the registration sequence:

1. CAI_RappelBehavior
2. CAI_FuncTankBehavior
3. CAI_TakecoverBehavior
4. CAI_BeserkBehavior
5. CAI_ActBusyBehavior
6. CAI_AssaultBehavior
7. CGruntStandoffBehavior
8. CAI_FollowBehavior

The constructor constructs embedded objects in a **different order** and their numeric owner offsets are not sorted. The selector traverses this registration vector, not the constructor construction order or member address order.

### Original schedule bridge 0x2234E730

This complete 83-byte routine first invokes 0x2234E590 at 0x2234E734. It reads active owner+0xC18 afterward, writes owner byte+0xC30=1, and if nonnull invokes current behavior virtual+0x58 plus additional virtual+0xD4 processing. If no active behavior exists, its original final action is a tail JMP to base native 0x2204FD80. That behavior must be modeled separately from the candidate eligibility loop; the exact symbolic names of +0x58, +0xD4 and +0xC30 remain unproven.

Original direct selector E8 references authenticated by address include 0x2234E734, Grunt's 0x2235E6EE, and 0x22366137 / 0x2236E6F5 / 0x2236E74D. This audit also checks 0x2234E61E -> 0x2216E510 and bridge E9 0x2234E77E -> 0x2204FD80. The exact subclasses for the additional non-Grunt callsite owners are not all established here.

## Practical host-side reference, not engine-loaded original code

The new original-free header in include/sin_compat/original_grunt_behavior_priority_phase606.hpp reproduces the ordered first-eligible loop using explicit Host callbacks for:
- candidate eligibility virtual+0x18
- owner acceptance virtual+0x77C
- current behavior transition, linked to the earlier Phase604 semantics
- no-eligible cleanup and old-behavior notification

Its C++ regression tests check every registered class name/order, first true winner, priority over numeric member offsets, clearing a prior behavior when none qualifies, empty cleanup, and an optional generic host veto (other possible subclasses). MobGrunt's actual acceptance callback always returns true. The adapter does not invent eligibility results, allocate original NPC objects, or claim an engine-loaded replacement DLL.

## Audit and validation

Read-only native auditor: research/tools/audit_sin_original_grunt_runtime_behavior_priority_phase606.py

    python research/tools/audit_sin_original_grunt_runtime_behavior_priority_phase606.py --self-test
    python research/tools/audit_sin_original_grunt_runtime_behavior_priority_phase606.py --bin-zip /private/SE1/bin.zip

It checks the original SHA256-pinned DLL pair; three complete whole method hashes and genuine following INT3; real MobGrunt vtable slots +0x434 and +0x77C; six native E8 calls and one E9 tailjump; and 15 sparse opcode witnesses. Original-free negative testing flips every byte of all seven E8/E9 encodings (**35 corruptions refused**). In addition to the Python self-test, CMake registers a compiled C++ selection behavior regression.

**New authentic native source recovery: three methods / 240 bytes. Phases601–606 total: 25 complete methods / 10,026 original machine-code bytes**. This is not a full portable replacement server.dll, playable Windows build, Android runtime or Quest 3 port. Standard HL2-equivalent physics, surfaces, prop rendering and models remain explicitly out of scope.
