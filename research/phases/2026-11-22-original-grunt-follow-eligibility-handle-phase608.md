# Gameplay Phase608: complete native Grunt Follow eligibility continuation +0xE4

## Original 2006 source and native method identity

Directly read the owner's unmodified 2006 SiN Episodes: Emergence retail binaries from their connected Drive SE1/bin.zip. Original SHA256: server 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650; client 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

Phase607 independently authenticated all eight concrete Grunt behavior +0x18 eligibility methods. Follow's 31-byte entry at original 0x220D15D0 first calls the owning NPC's virtual+0x450, returning false if that gate fails, and then tail-jumps to the Follow behavior's virtual byte offset **+0xE4**. Phase608 now authenticates the exact target and the complete original method behind that second virtual call.

- Original Follow RTTI-backed final behavior vtable: **0x223F6B7C**.
- Exact native slot **+0x18 -> 0x220D15D0**, previously authenticated whole method **31B**, SHA256 c60968717e032c273c8f7a7c5d32edb08f3f8d8d44aa3781d3f68d11579fd542.
- Exact native slot **+0xE4 -> 0x220D2370**. New whole original method **115B**, SHA256 ceffc4ba3ad0438c39110002eb8734ad6010061e27733a72bae8627473113b4d.
- The method terminates at exclusive **0x220D23E3**, with **13 original INT3 bytes** until **0x220D23F0**, which starts a different original function.

These original source hashes, exact vtable pointers, 23 sparse byte-level opcode witnesses, and the distinct next-function opener are checked against the original 2006 retail PE32. No original DLL or bulk disassembly is committed.

## Genuine native Follow +0xE4 logic

The original routine reads an encoded 32-bit entity handle at **behavior+0x14** and tests against invalid sentinel 0xFFFFFFFF. A valid handle is split into:
- Low **12 bits**, mask 0xFFF: entity lookup-table index
- Upper **20 bits**, right shift 12: serial number

It fetches the global entity-list pointer from **0x224F5620**, then addresses an entry at table +4+16*index, checks that entry's serial at +4 equals the encoded serial, and requires a non-null entity pointer. If any of those initial conditions fail, returns false.

Then it performs **a second fresh read and resolution** of behavior+0x14, again splitting the low 12-bit index and checking the high serial in that table. If this second serial agrees, it reads the selected entity's 32-bit field at **entity+0xBC**, shifts right 15, bitwise inverts, and masks to 1. On a stable valid handle, the return is therefore:

    owner_virtual_plus_0x450() &&
    handle_is_not_invalid &&
    serial_checked_entity_pointer_is_valid &&
    ((entity_field_plus_0xBC & 0x00008000) == 0)

The historical C++ name and meaning of entity flags field+0xBC is **not established**. Do not rename bit15 to a modern Source engine flag without original proof.

## Significant original unsafe path and porting distinction

**Two handle reads matter.** The second lookup has a distinct failure branch that sets ECX=0 and then attempts to read **[ECX+0xBC]** at original native 0x220D23D3. Thus if the initial handle resolves successfully but the handle changes to an invalid or stale serial between the two lookups, the original 2006 method may dereference a null pointer instead of safely returning false. We do not infer how often this can occur under the original engine thread/lifetime rules.

This is an original memory-safety observation, not a request to replicate a crash. Our portable, independent C++17 host model:
1. checks the original owner-gate first;
2. reads and serial-validates the first handle;
3. **re-reads** the handle and repeats the serial-validated lookup;
4. returns false if either lookup is invalid/missing, rather than dereferencing a missing entity;
5. returns true exactly when the final resolved field+0xBC lacks bit 0x8000.

On stable valid handles, this preserves the native observable result. On an invalidated second lookup, **safe refusal is an explicit port engineering deviation**. The original engine's entity lifetime contract and multi-thread behavior are not separately authenticated here.

## Source-backed port model and regression coverage

Files:
- research/tools/audit_sin_original_follow_handle_predicate_phase608.py: full original PE32 pair SHA guard, the two RTTI-backed virtual targets, 115-byte SHA, correct 13 INT3 padding, original next-function opener, 23 sparse opcode witnesses, plus original-free 32-case semantic truth-table self-test.
- include/sin_compat/original_grunt_follow_eligibility_phase608.hpp: C++17 Host interface with owner gate, two encoded-handle reads, serial-aware entity flags resolution, bit15 check, explicit fail-closed safety policy for second lookup.
- tests/test_original_grunt_follow_eligibility_phase608.cpp: checks owner-gate short circuit, initial invalid sentinel, stale serial, stable valid flag clear/set, correct low-index/high-serial decomposition, two actual lookups, second stale/invalid handle refusal and changed but valid second handle.
- CMake synthetic Python and standalone C++ regression CTests.

Reproduce the native audit:

    python research/tools/audit_sin_original_follow_handle_predicate_phase608.py --self-test
    python research/tools/audit_sin_original_follow_handle_predicate_phase608.py --bin-zip /private/SE1/bin.zip

**Source verification:** direct original retail inspection confirmed complete 115-byte SHA256, exact both Follow vtable pointers, 13 CC padding, distinct next function at 0x220D23F0, and all 23 original source opcode witnesses. The full Python auditor independently refuses a wrong original DLL pair; CI runs its original-free synthetic test only. This phase does not claim that a Windows replacement server.dll, Android/Quest 3 runtime or actual NPC-host integration works.

## Milestone and remaining gameplay work

**One new complete source-authenticated native method, 115 original bytes. Total since Phase601: 34 whole native methods, 10,800 original machine-code bytes.** Follow's immediate +0x18 and deeper +0xE4 eligibility conditions are now resolved on stable entity handles. Other complex predicates, notably Assault's helper paths and Takecover's time-state and target constraints, still need original instruction-by-instruction reconstruction before a behavior-equivalent game port is possible.

The user has already ported the generic Source/HL2 map, model and prop facilities; no further work here on physics/surfaces/assets/models or rendering.
