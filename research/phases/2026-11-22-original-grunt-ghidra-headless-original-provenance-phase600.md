# Gameplay Phase600: Ghidra headless original Grunt event-handler provenance

## Target / progress

Owner-supplied Phase598 report is SHA-authenticated against untouched retail SiN Episodes: Emergence 2006 binaries:

- server.dll SHA256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
- client.dll SHA256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

Earlier Phase598 established eight original RTTI-backed Grunt behavior vtables, and 16 entries at virtual offsets +0x60 and +0x7C, corresponding to **13 distinct original executable entry addresses**. Phase599 checked those records, and provided a Ghidra script for candidate native function boundaries, event 0x54 immediate sites, and member +0x08 operand references.

Phase600 now supplies a no-GUI **Ghidra headless launcher** and a strict output validator for the Phase599 Ghidra report. This removes the manual project import and Script Manager steps, but does **not** make claims about 13 complete decompiled original functions. The commercial original DLL files remain on the owner's computer.

## Comparative SDK findings are not 2006 source proof

In the published Valve Source SDK 2013 implementation, CAI_BehaviorBase::HandleAnimEvent calls the behavior's BackBridge_HandleAnimEvent to route the event to its owning host. Other default methods, such as StartTask, RunTask, AimGun and ShouldAlwaysThink, clear m_fOverrode to indicate falling back. This is a useful comparison for native flow and marker tracing but **does not prove any specific 2006 SiN override control flow or class identity**.

Public comparison:
- https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/game/server/ai_behavior.cpp
- https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/game/server/ai_behavior.h

## Committed tools and owner workflow

- research/tools/launch_sin_original_grunt_ghidra_phase600.py: validates exact retail SHA256 on both original DLLs and the Phase598 JSON; searches common Ghidra install locations or accepts --ghidra-home; imports the DLL into a temporary Ghidra project; invokes the existing Ghidra Phase599 Python script by -postScript; then validates 13-target source-metadata output and deletes the temporary Ghidra project.
- research/tools/triage_sin_original_grunt_ghidra_phase600.py: revalidates detailed original method target mapping, the eight behavior classes and five activity targets, source hashes, role attribution, Ghidra candidate function starts and candidate site counts. Rejects records that wrongly assert source semantics or native complete method boundaries.
- research/ghidra/GruntBehaviorDispatchPhase599.py: previously committed Ghidra read-only function-site probe, reused unchanged.

The owner also has an attached Phase600 downloadable ZIP with both Python scripts, the Phase599 Ghidra script, the authenticated Phase598 JSON, a double-click Windows batch launcher and installation instructions.

Reproduce from the original installed bin directory:

    cd "D:\SiN-DisposableTest\SE1\bin"
    python .\launch_sin_original_grunt_ghidra_phase600.py --self-test
    python .\triage_sin_original_grunt_ghidra_phase600.py --self-test
    python .\launch_sin_original_grunt_ghidra_phase600.py --bin-dir . --ghidra-home "D:\Tools\ghidra_12.0_PUBLIC"

The example Ghidra directory is illustrative. Ghidra and the compatible Java runtime are separate prerequisites. Set GHIDRA_HOME or pass the real install folder to --ghidra-home. The launcher automatically cleans up its temporary project and never modifies the original game DLLs.

Output files (metadata only, no original bytes):
- grunt_behavior_phase599_ghidra.json: 13 candidate function extents, Ghidra-selected raw PE span hashes when contiguous, native direct-call target addresses, candidate immediate 0x54 and +0x08 operand instruction locations.
- grunt_behavior_phase600_triage.json: sorted 13-target provenance/priority information.
- grunt_behavior_phase600_headless.log: local diagnostics, useful if Ghidra analysis or script execution fails.

The owner should return only the detailed Phase599 Ghidra JSON. The candidate SHA256s are **NOT** independently authenticated full method bodies until original return/padding/branch/table boundaries are checked, and a reference to byte+0x08 is **NOT** proof of a write to the active behavior's override marker.

## Native research targets, unchanged from Phase598

Event +0x60 function pointers: Rappel 0x220DEA60; FuncTank 0x22392970; Takecover 0x22399650; Beserk 0x22391C90; ActBusy 0x2238EB40; Assault 0x220CF190; GruntStandoff 0x220E0290; Follow 0x220D6FF0.

Activity +0x7C function pointers: shared 0x220CD400 (Rappel/Beserk/ActBusy/Assault), FuncTank 0x22392700, Takecover 0x22398670, GruntStandoff 0x220E1EE0, Follow 0x220D33B0.

Original +0xC18 identity as current active behavior remains a structural hypothesis, while original 0x2202D930 uses the attached object byte +0x08 as a marker. Event 0x54's handling by each specific behavior remains unknown.

## Verification and scope limits

Two original-free Python self-tests cover safe Ghidra argv construction, 13-target validation and corrupted-report refusal; both are registered in synthetic CTest. This does not require original DLLs in CI. A genuine Windows Ghidra execution was not performed in this assistant session because the owner's original binaries/Ghidra environment were not available.

**New complete 2006 native method bodies authenticated in Phase600: zero**. No generic HL2 physics, surfaces, props/models, native DLL replacement, Android runtime or Quest 3 boot work.
