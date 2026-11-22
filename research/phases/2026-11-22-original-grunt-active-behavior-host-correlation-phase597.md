# Gameplay Phase 597: Original Grunt active-behavior host hypothesis

## Status and evidence standard

**High-confidence structural identification hypothesis, not a newly authenticated 2006 native function or confirmed RTTI type.** The owner's Phase595 report validated the unchanged retail server.dll full SHA256 and returned 245 candidate +0xC18 uses. Phase596 identified two genuinely authenticated native Grunt member reads, separated unrelated attached-flying vector references, and prepared Ghidra instruction-boundary triage. This phase cross-correlates those native findings with our earlier Grunt constructor/AI behavior research and Valve's published Source SDK behavior-host declarations.

No new original DLL bytes, raw assembly, code dumps, assets, executable patches, or playable runtime are committed.

## Four-field 32-bit owner layout correspondence

| Native Grunt member | Evidence in our 2006 research | Plausible published Source SDK counterpart |
| --- | --- | --- |
| **owner +0xC14** | Phase531 authenticates a third constructor-installed vptr at +0xC14 | Embedded private IBehaviorBackBridge secondary interface vptr in CAI_BehaviorHost<BASE_NPC> |
| **owner +0xC18** | Phase587 activity virtual 0x2235A250 reads it (candidate native site 0x2235A375); Phase592 event gate 0x2202D930 reads it (candidate 0x2202D93E) | CAI_BehaviorBase *m_pCurBehavior, the currently active behavior pointer |
| **owner +0xC1C** | Phase535 registers eight embedded AI behaviors into collection at +0xC1C | CUtlVector<CAI_BehaviorBase*> m_Behaviors starts here |
| **owner +0xC28** | Phase535 separately validates the collection count at +0xC28 | CUtlVector's m_Size, 12 bytes after its CUtlMemory storage header |

The Source SDK class declaration orders m_pCurBehavior immediately before m_Behaviors. It also inherits the separate IBehaviorBackBridge interface, creating a plausible 32-bit secondary vfptr before both fields. The exact original vptr identity and object field alignments have **not** been independently proved by this correlation alone.

**Published comparative sources, not original 2006 source code:**
- https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/game/server/ai_behavior.h
- https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/game/server/ai_component.h
- https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/game/server/ai_behavior.cpp

The published CAI_BehaviorHost<BASE_NPC> contains m_pCurBehavior and m_Behaviors, with HandleAnimEvent forwarding to the current behavior. CAI_Component owns an outer pointer and a vfptr on 32-bit MSVC, while CAI_BehaviorBase contains a bool m_fOverrode after that base. Therefore **behavior pointer +0x08** is an unusually strong structural match for the original event gate's raw byte marker +0x08.

## Interpretation of Phase592's original marker gate

Native method 0x2202D930, already whole-SHA authenticated in Phase592, does the following for non-0x43 events:

1. Checks owner+0xC18 pointer.
2. When nonnull, writes raw byte +0x08 of that pointed-to object to 1.
3. Calls the pointed-to object's virtual slot **byte offset +0x60(event)**.
4. If the byte remains nonzero, skips inherited shared NPC event dispatch; if the byte is zero, calls native 0x22050150(event).

The published CAI_BehaviorBase has a member m_fOverrode that participates in other behavior-host interception decisions. The original marker's **value** rather than the virtual return determines whether the base handler runs. A symbolic interpretation is plausible:

    pCurrentBehavior = owner.CurrentBehavior;  // HYPOTHESIS for raw owner+C18
    if (pCurrentBehavior) {
        pCurrentBehavior->overrode = true;       // raw behavior+8
        pCurrentBehavior->HandleAnimEvent(ev);  // original virtual+0x60, name HYPOTHESIS
        if (pCurrentBehavior->overrode) return;
    }
    OriginalSharedNPCEvent(ev);

The observed control flow is original; all C++ member names and exact method name are tentative comparative terminology.

The same pointer's Phase587 virtual **byte offset +0x7C(activity)** is consistent with behavior activity translation (published CAI_BehaviorBase defines BridgeNPC_TranslateActivity), but we **must not** equate these slot offsets with modern SDK vtable positions without original-vtable authentication.

## Which actual behavior might receive event 0x54?

Phase535 already authenticated **eight** embedded Grunt behaviors and their original offsets:

- CAI_RappelBehavior (owner+0xCFC)
- CAI_FuncTankBehavior (owner+0xCD8)
- CAI_TakecoverBehavior (owner+0xD20)
- CAI_BeserkBehavior (owner+0xDA0)
- CAI_ActBusyBehavior (owner+0xF88)
- CAI_AssaultBehavior (owner+0xDB8)
- CGruntStandoffBehavior (owner+0xDE8)
- CAI_FollowBehavior (owner+0xEB8)

These are registered at owner+0xC1C. A current-behavior pointer at +0xC18 could dynamically point at any appropriate registered active behavior. It need not name one fixed allocated component class. We have **not** shown that event 0x54 is consumed or cleared by any specific one of these eight. Do not invent a deterministic GRUNT_FLANK success case or assume the behavior-host analog is source-identical across Source branches.

## Revised native verification priority

1. At original Grunt constructor(s), confirm the exact initialized value of owner+0xC18 and the RTTI/type of the secondary vfptr at owner+0xC14. Phase595's candidate write-immediate sites **0x22353C3C** and **0x22353D4C** are candidates, not verified constructor lines.
2. In Grunt behavior selection/transition functions, find writes to owner+0xC18, and determine whether assigned pointers alias one of the eight embedded behavior subobjects. Inspect prior possible callsites 0x2208C00E and 0x2208C054 only after validating x86 instruction boundaries.
3. Resolve vtable targets +0x60 and +0x7C for the **actual pointed-to behavior type**, not for an arbitrary class sharing +0xC18.
4. Check native writes to behavior+0x08 across those methods. Only when behavior-specific event 0x54 paths are traced can we say when GRUNT_FLANK event forwarding is blocked or permitted.
5. Reconcile published SDK m_fOverrode semantics with the 2006 original: later Source SDK implementations are comparative context, **not proof of 2006 event behavior**.

The architecture is now more focused: investigate **current active behavior selection and its virtual overrides** rather than search for a single enigmatic component factory. New native method count: **0**. New original DLL method bytes authenticated: **0**.
