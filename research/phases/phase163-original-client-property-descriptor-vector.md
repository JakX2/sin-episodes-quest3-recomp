# Phase 163: original client 24-byte property descriptor storage

**10 November 2026.** Real binary decompilation of SiN Episodes: Emergence (2006) original Windows x86 client.dll. Stacked on Phase 162.

**Important correction:** Native function 0x2400A300 is a generic **24-byte descriptor array insertion function**, not a Color text parser or texture decoder. The previous Phase162 report has been revised. Parsing native VGUI Color and string defaults remains a future task.

## Original executable provenance

- client.dll entire SHA-256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
- server.dll entire SHA-256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

| Original address | Entire original method size | SHA-256 of full original method region |
|---|---:|---|
| 0x2400A300 | 141 bytes | 3fc3fcbdc8f5f3515e23f4b2cfd0146af268702aa270d6e892935f9a9ecaccf8 |
| 0x2400B140 | 119 bytes | ca54ced9589a979692188164c701cb963a17196ed6bf5ffe886433c5f0fed60f |

Both SHA256 regions were measured directly against the private owner-held original 2006 DLL. Additionally, 27 sparse x86 instruction anchors and 11 exact native call targets were checked. The original executable bytes and licensed assets are not in the repository.

## Original array object layout and six-DWORD record

The native dynamic array contains these fields:

| Original object offset | Actual role |
|---|---|
| +0x00 | Pointer to element storage |
| +0x04 | Signed int32 capacity, in records |
| +0x08 | Signed int32 capacity-growth increment |
| +0x0C | Signed int32 live record count |
| +0x10 | Cached pointer to element storage, refreshed after insertion |

Each item is exactly 24 bytes (0x18), copied as **six 32-bit words** by original x86 MOV instructions. The fifth word contains the registration kind byte and may have other uninterpreted bits. The portable model preserves all six raw words; it does not reinterpret the native registry as an actual Color string parser.

## Complete normal-input insertion algorithm (0x2400A300)

The original function receives this pointer plus two stack arguments: an insertion index and a pointer to an existing 24-byte descriptor. It returns the insertion index, using native RET 8.

1. Read live count and capacity. If count+1 exceeds capacity, call original growth helper 0x2400B140 with **additional** slots count+1-capacity.
2. Increment count, refresh cached pointer at +0x10.
3. If old_count-index is positive, call original memmove at 0x24248B50 to move (old_count-index)*24 bytes from storage+index*24 to storage+(index+1)*24.
4. Compute the destination slot storage+index*24, copy all six original DWORDs from the supplied descriptor. Return the original insertion index.

The original x86 contains **no bounds validation** of index or live-count arithmetic. Deliberately malformed indexes, externally-owned buffers with insufficient room, or overflow can result in memory corruption. The portable C++ reference supports valid inserts (including middle and prepend) and rejects unsafe inputs instead of implementing these bugs. It also intentionally does not support descriptor source pointers aliasing the same live array because native shifts can mutate their contents.

## Complete normal-input capacity growth algorithm (0x2400B140)

Original +0x08 growth increment has three observed modes:

- **-1:** Do nothing. The original function returns without allocating even if more capacity was requested (fixed/external storage sentinel).
- **0:** If capacity is zero, initialize it to **2**, otherwise double the capacity until the required amount is reached.
- **Positive:** If capacity is zero initialize it to **2**, then add the positive fixed increment until enough slots are available.

Original required capacity equals old capacity plus the caller-supplied **extra** capacity. If the array already meets that amount, return unchanged. Finally, allocate new storage using 0x2416F8F0 if pointer is null, or reallocate using 0x2416F960 if nonnull, always for new_capacity*24 bytes. Original native routine returns with RET 4.

The C++ reference uses checked signed arithmetic and reports nonprogressing growth or overflow with std::nullopt rather than entering undefined memory operations.

## Connection to all seven native timer registrations

The original Phase162 guarded registration methods call **this exact array insertion function** with the owner-registry live count as insertion index and a freshly constructed six-word descriptor on the stack. The following original CALL rel32 instruction sites were independently checked:

| Original property registration | Native insertion CALL site |
|---|---|
| BreathTimerColor | 0x24230E37 |
| BreathBarForeground | 0x24230EA7 |
| BreathBarBackground | 0x24230F17 |
| PoisonTimerColor | 0x242364A7 |
| MutagenBarForeground | 0x24236517 |
| MutagenBarBackground | 0x24236587 |
| MutagenBarInactive | 0x242365F7 |

Four additional exact native call edges establish the storage implementation:
- 0x2400A319 to 0x2400B140: insertion invokes growth.
- 0x2400A34C to 0x24248B50: insertion shifts bytes via memmove.
- 0x2400B191 to 0x2416F960: reallocate existing buffer.
- 0x2400B1A9 to 0x2416F8F0: allocate initially null buffer.

**All eleven original direct CALL edges are verified.** The seven specific HUD registrations append; generic 0x2400A300 can also insert in the middle of the array.

## New original-free code and verification

- include/sin_compat/original_client_property_descriptor_array.hpp: C++17 original six-word descriptor, normal-input insertion, growth policy and safe handling of invalid configurations. A host callback can observe native-style capacity changes. No fabricated ABI object.
- tests/test_original_client_property_descriptor_array.cpp: strict CTest covering growth 0 to 2 to 4 to 8, fixed positive growth, disabled growth -1, all six descriptor words, middle insertion and appending the seven actual Phase162 properties.
- research/tools/audit_sin_original_client_property_descriptor_array.py: private owner-only full original client/server SHA chain, two entire function-region hashes, 27 sparse opcode anchors, 11 native direct-call checks.
- research/tests/test_sin_original_client_property_descriptor_array.py: original-free tampering tests for every witness, independent 288-case capacity-growth model and six-word record shift tests.
- research/phases/phase162-original-timer-property-registrars.md: corrected to distinguish native descriptor insertion from actual Color parsing.
- Root CMake/CTest test target and recovery ledger additions.

The reviewed original-method atlas increases **69 to 71 entries**, now 25 server and 46 client. These are verified manually selected methods and do not represent the fraction of total original game code recovered.

To run the owner-only original DLL audit:

    python3 research/tools/audit_sin_original_client_property_descriptor_array.py --client /private/client.dll --server /private/server.dll --output /private/phase163.json

The original owner registry resolver at 0x24011DC0, actual Color/string parsing into live HUD members, engine VGUI callbacks and fully game-loadable 2006 Windows client/server replacements remain incomplete. Android/Quest gameplay portability remains a subsequent milestone. No proprietary executable or asset data is distributed.
