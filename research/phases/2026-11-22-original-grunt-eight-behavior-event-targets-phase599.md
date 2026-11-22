# Gameplay Phase599: original Grunt behavior vtable targets and event-dispatch function triage

## Retail source and evidence limits

The owner uploaded the exact Phase598 address-only report from untouched retail SiN Episodes: Emergence 2006 server.dll and client.dll. Authentic original full-file SHA256:
- server: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
- client: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

It additionally confirms eight MSVC RTTI-authenticated original behavior vtables and 16 original virtual slot pointers in the executable .text section, after earlier method authentication of the 0x2202D930 optional event intermediary. The original function boundaries and virtual semantics remain unproven by this pointer-only report. Identification of Grunt owner+0xC18 as active behavior is still the strong but not native-class-RTTI-proven hypothesis from Phase597.

Phase599 adds a source-pinned, original-free independent validator of the whole report, and a read-only Ghidra handoff to investigate 13 original function entries.

## Actual original vtable target matrix

| Original class RTTI | Embedded owner member | Original event virtual +0x60 | Original activity virtual +0x7C |
| --- | --- | --- | --- |
| CAI_RappelBehavior | +0xCFC | 0x220DEA60 | 0x220CD400 |
| CAI_FuncTankBehavior | +0xCD8 | 0x22392970 | 0x22392700 |
| CAI_TakecoverBehavior | +0xD20 | 0x22399650 | 0x22398670 |
| CAI_BeserkBehavior | +0xDA0 | 0x22391C90 | 0x220CD400 |
| CAI_ActBusyBehavior | +0xF88 | 0x2238EB40 | 0x220CD400 |
| CAI_AssaultBehavior | +0xDB8 | 0x220CF190 | 0x220CD400 |
| CGruntStandoffBehavior | +0xDE8 | 0x220E0290 | 0x220E1EE0 |
| CAI_FollowBehavior | +0xEB8 | 0x220D6FF0 | 0x220D33B0 |

All eight original event pointers are **distinct**. Four behaviors (Rappel, Beserk, ActBusy, Assault) share original activity delegate **0x220CD400**; four have independent delegates. **16 authenticated original pointers reduce to 13 unique target code addresses: 8 event / 5 activity.**

The earlier original Grunt Phase592 method reads NPC owner+0xC18, writes pointed object's byte+8 to 1, dispatches virtual +0x60(event), and forwards to shared NPC only if that byte is cleared. Phase587 also uses the same NPC pointer for virtual +0x7C(activity). Published Source behavior-host layout and Phase535's eight registered embedded objects support the hypothesis that this is the active behavior and override flag, but vtable target addresses do **not** establish runtime behavior selection or event 0x54 consumption. Do not assert that any specific handler suppresses GRUNT_FLANK.

## Deliverables

### Original-free sorted metadata validator

research/tools/triage_sin_original_grunt_behavior_vtables_phase599.py

Checks exact original hashes, previous intermediary, eight class identities, embedded member offsets, vtables, original event/activity function addresses, text-section membership, reported grouping, duplicate refusal and false runtime claims. The mutation self-test rejects seven independent corrupted metadata cases. It never reads proprietary game bytes.

Run:

    python research/tools/triage_sin_original_grunt_behavior_vtables_phase599.py --self-test
    python research/tools/triage_sin_original_grunt_behavior_vtables_phase599.py --input grunt_behavior_phase598_targets.json --out grunt_behavior_phase599_triage.json

### Ghidra original-DLL function provenance pass

research/ghidra/GruntBehaviorDispatchPhase599.py

Import/auto-analyze the untouched original server.dll in Ghidra. Run this read-only Ghidra Jython script. Select Phase598 owner report, local untouched original server.dll file, and desired JSON output when prompted (or pass those three paths as headless script arguments).

The script:
1. Authenticates the actual local original PE full SHA256, original image base and all 16 virtual pointers against Ghidra's imported program memory.
2. For 13 distinct event/activity targets, reports recognized native function starts or ambiguity, Ghidra-estimated function span/size, and an original-PE raw span SHA256 **only for contiguous recognized bodies**. These are candidates, not verified whole-function hashes until actual ret/end and jump table boundaries are checked.
3. Reports Ghidra instruction addresses/mnemonics that carry scalar 0x54 or 0x43 (potential event branches), plus heuristic operand references to byte+8 (potential override-marker use); receiver/object provenance, writes and event semantics remain unproven.
4. Includes direct CALL destination addresses and refuses to invent missing functions. No original machine bytes, proprietary disassembly or copyrighted assets are exported.

## Next verification

Inspect the event handlers at 0x220DEA60, 0x22392970, 0x22399650, 0x22391C90, 0x2238EB40, 0x220CF190, 0x220E0290, 0x220D6FF0 and shared ACT delegate 0x220CD400 first. Confirm method bodies and whether the original event ID 0x54 affects behavior+8 before deciding which behavior consumes the event. Trace native writes to Grunt owner+0xC18 to validate the active-behavior identification.

No native method bodies newly authenticated in Phase599. No direct DLL recreation, executable patch, HL2-equivalent physics/surface/prop/model work, Android or VR boot claimed.
