# Phase 99: verified VGUI keyboard mapping and sound token crosswalk

Based on SHA-256-pinned 2006 SiN client/server and engine DLLs and SourceVR 0.5.5 ARM64 libraries. The audit chains all six Phase98 binary checks. Proprietary executables are never committed.

## VGUI_System010 index 24

The original key translator at 0x1001CB60 calls lazy initializer 0x100227A0. The latter initializes a 256-DWORD BSS table at 0x10053270 using REP STOSD, a register-derived key 48 -> 1 assignment, and 98 consecutive ten-byte MOV-immediate assignments starting at 0x100227CD. It uses a one-time flag at 0x10053E74.

Result: 99 distinct mapped entries, 157 zero entries. Reconstructed table SHA-256: 6c743e453ce5d6b91214fa2db2fa6b935d7b1ca886c34ebda9538f1ab80a10b0. Complete initializer SHA-256 (1027 bytes): 205064fb73c9f792c651907085e04c740ee7a8f406b845837e18dde957d307c0. The table is generated at runtime and cannot be read as static bytes from the DLL file. Original lookup returns 0 for negative or >=256 keys.

The auditor checks 17 opcode anchors, each of 98 assignment instructions, bounds, uniqueness and final table hash. Optional --export-keymap privately writes the owner-derived 256 entries. Public deliverables include only fingerprints and code, not original binaries or the generated table.

## Sound token width mismatch

Legacy VSoundEmitter001 slot4 at 0x2C005FD0 validates full signed-32 IDs, 0 <= token < count at this+0x70. The SourceVR VSoundEmitter002 native slot8 at 0x13520 first masks to 16 bits, then checks native counters/generation. Do not truncate legacy IDs into native IDs.

Old validator SHA-256 (23 bytes): 4742cc7ff52778ba3ecf9cb0b021116ec4ebcf094068f99b9e01b4a305638145. Native validator SHA-256 (76 bytes): 44f13012e9a0558361a5e4d8ea8fd9b01bddc8f6c04cded69214ea1719272a26.

The host-only C++17 VerifiedSoundTokenCrosswalk rejects invalid IDs, native candidates beyond 65534, failed validation, bad name round trips and conflicting mappings. Names are owned and stable across additions. Native sound slots 7 and 10 remain unverified candidates, not callable live methods.

## Files and testing

- include/sin_compat/legacy_sound_vgui_prebridge.hpp
- research/tools/audit_legacy_keymap_sound_namespace.py
- research/tests/test_legacy_keymap_sound_namespace.py
- research/tests/sin_legacy_sound_vgui_prebridge_reference.cpp
- CMakeLists.txt

Local audit on all six private binaries passed, 36/36 Python tests passed, CTest 3/3 passed. The implementation neither completes libclient.so/libserver.so nor runs SiN on Quest 3. Next: actual native sound method signature checks and VGUI InputInternal slot56 replacement.