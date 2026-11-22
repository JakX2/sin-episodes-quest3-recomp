# Gameplay Phase604: original Grunt behavior pointer restoration and transition

> **Phase605 correction using the retail original:** Phase604 function 0x2216DE40, Grunt vtable +0xA4, is the original **SAVEGAME RESTORE** wrapper. It restores the current behavior pointer from serialized AI_Behaviors version-2 records rather than choosing it from live combat AI conditions. Its actual owner+0xC18 pointer assignment, original method hash, and Grunt vtable attribution are still correct. The transition/reset methods remain authentic. See [Phase605 Save/Restore verification](2026-11-22-original-grunt-ai-behaviors-save-restore-phase605.md). Ordinary runtime policy selection is still unresolved.

## Retail source provenance

Original 2006 SiN Episodes: Emergence server.dll and client.dll from the owner's connected Drive SE1/bin.zip. Exact authentic SHA256:
- server: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
- client: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

Phase603 established the original RTTI class CAI_BehaviorHost<CAI_BaseNPC> and its native constructor setting Grunt owner+0xC18 to NULL. Phase535 authenticated eight embedded Grunt behavior registrations. Phases601–602 authenticated all their event+0x60 and activity+0x7C function bodies. Phase604 completes their **original Grunt-specific active behavior selection and lifecycle link**, rather than assuming Source SDK 2013 has identical mechanics.

## Four new exact whole-function native SHA256s, 331 distinct bytes

| Actual original method role | Start | Complete bytes | Whole original method SHA256 |
| --- | --- | ---: | --- |
| Select current pointer via behavior-array index | 0x2216DE40 | 104 | 273450c7062ad7ca28aa34edb7faa41c3d5f5d97641e943b32e387c8b06b3ee7 |
| Reset then inherited action | 0x2216E4E0 | 36 | 5dfa361d61a3eb70d088e81886e2378b987609cc4ccf74b62255e49521354b31 |
| Set current behavior, issue transition callbacks | 0x2216E510 | 104 | 3fd8b4e4642526aa8171927faf9be753b3d7761ec7e2c48a49e73786eafdd216 |
| Clear current behavior and forward | 0x2216E580 | 87 | ff4469eecb4800a67e882c186eaee30a0cfbe69f00e9eec8775f0a4bb0d6d568 |

Whole method SHA256s, exact INT3 alignment after each method, seven original E8 direct calls and 19 sparse original opcode/field witnesses verified against unchanged retail. This is **complete native body authentication**, not a claim that the whole game's original C++ source is recovered.

## Actual original Grunt vtable linkage

The existing original MobGrunt primary vtable 0x2249AAA4 contains:
- virtual +0xA4 -> **0x2216DE40**, the genuine behavior selection method
- virtual +0x660 -> **0x2216E4E0**, reset wrapper
- virtual +0x664 -> **0x2216E580**, clear-and-forward handler

The transition method 0x2216E510 is directly called by wrapper 0x2216E4E0. These are **Grunt-native original call paths**, not the parallel CAI_BehaviorHost<CAI_BlendedNPC> specialization at 0x22375D80, which was investigated independently and deliberately not conflated with Grunt.

### Original method 0x2216DE40

1. Calls native 0x22043AD0 with caller argument and tests its return.
2. On success, invokes owner virtual+0x4BC, then +0x4B8, and calls native helper 0x220CD630 with supplied argument and these results.
3. When helper returns index other than -1, calls owner virtual +0x4B8 to retrieve registered behavior pointer backing array. Loads pointer at index*4 and **writes pointer to owner+0xC18 at native 0x2216DE8B**.
4. If helper returns -1, **writes NULL to owner+0xC18 at 0x2216DE96**. The first predicate failure path does not perform these assignments.

Exact semantics of helper 0x220CD630 and certain original caller arguments remain unresolved. However, the native selected-array-member pointer assignment itself is now unambiguous.

### Original method 0x2216E510

1. Reads prior owner+0xC18 and receives proposed new behavior pointer.
2. Compares old and new and **stores the proposed pointer at native 0x2216E528** even if equal.
3. Only on pointer change, if new pointer nonnull: calls new behavior virtual +0x1C; writes native global byte 0x2254C0A0=1; calls new virtual +0x24; writes global byte 0x2254C0A0=0.
4. If old pointer nonnull, calls old virtual +0x20 and native helper 0x22054CA0.
5. Calls owner virtual **+0x778(old,new)** on change; unchanged pointer returns without callbacks.

The original global byte is a transition-scoped marker by observed code, not a recovered original C++ variable name.

### Original reset paths

- At 0x2216E4E0, passes NULL to the verified 0x2216E510 transition method, then forwards original arguments to native 0x2204CAE0.
- At 0x2216E580, snapshots prior owner+0xC18, directly sets owner+0xC18=NULL at 0x2216E591, invokes old behavior virtual+0x20 and 0x22054CA0 if old nonnull, notifies owner virtual+0x778, then forwards to native 0x2204CDD0.

These are verified original instruction/call orders. Historical Source class function identifiers and exception behavior remain unknown.

## Complete current Grunt behavior-host chain

Phase603 original ctor initializes current+0xC18 null -> Phase535 registers eight embedded behaviors at vector +0xC1C -> Phase604 actual Grunt vtable selects vector entry into current pointer and invokes lifecycle transitions -> Phase601 original event gate presets behavior byte+0x08 and invokes behavior event virtual+0x60 -> all eight original registered behaviors' event 0x54 routes reset marker via original seven-byte 0x220CD2F0 -> shared NPC event logic can reach GRUNT_FLANK only when its inner parameter is 1 and genuine route length strictly >240.0f. Phase602 independently authenticates distinct behavior activity translations.

Unresolved: potential other behavior types at runtime, all specialized event handling, helper 0x220CD630 internals, original animation asset event emitter, precise historical C++ member name, and real engine-host execution.

## Reproduction

Auditor: research/tools/audit_sin_original_grunt_behavior_selection_phase604.py

    python research/tools/audit_sin_original_grunt_behavior_selection_phase604.py --self-test
    python research/tools/audit_sin_original_grunt_behavior_selection_phase604.py --bin-zip /private/SE1/bin.zip

Owner-retail original full method audit PASS; original-free synthetic self-test flips all five bytes of seven E8 source edges, rejecting **35** corrupted encodings and mismatched original DLLs. Registered as CTest. No retail executable bytes or raw disassembly deposited in public repository.

**Phases601–604 total: 19 complete source-authenticated original gameplay methods and 9,134 original method bytes**, not a playable Windows/Android/Quest 3 replacement DLL. No generic Source/HL2 physics, models, props, surfaces or rendering work.
