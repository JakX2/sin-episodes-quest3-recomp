# Phase 229: original embedded HLTV and entity-list virtual accessors

14 November 2026. Stacked DRAFT on Phase228 PR #283. Privately verified the original 2006 SiN Episodes PE32 binaries: server.dll 6,049,792 bytes, SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650; client.dll 3,592,192 bytes, SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d. No original game DLLs, proprietary function-body dumps, disassemblies or assets are committed.

## Twelve previously unreviewed whole-original native functions

All methods are in the **final original embedded-interface vtables** already established by Phase228. Original server HLTVDirector001 interface pointer = containing object 0x2257AE98 + 8; final vtable 0x22453DA0. Original client VClientEntityList003 interface pointer = containing object 0x2433FDC8 + 0x10014; final vtable 0x24297390. All method offsets below are relative to the *embedded interface this pointer*, not the parent pointer.

| DLL | Method native VA | Final vtable slot | Original code bytes | Complete original function SHA256 |
| --- | --- | ---: | ---: | --- |
| server | 0x2225F920 | 3 | 4 | f7e0208f5e988ae39c7a45dfddbd12921d02f9c3deb5695d4c534c8ab0ccdb0f |
| server | 0x2225F930 | 1 | 9 | f862b03f571a3d453c5ef9b7e990a48ac6059bd0f34bac7da7740d0d8fc81cd9 |
| server | 0x2225F940 | 7 | 4 | a4b7d4539fa03abbe69a398ca110f35831e031556ffd14e0089956276d803f49 |
| server | 0x2225F950 | 4 | 4 | 2e339cdc5de837ec151dbde90057caeabba18ac3ea32e7c2cffc8f95df41a6ff |
| server | 0x2225F960 | 5 | 4 | 74032ad0e931f3c4730df99da43479f9f5315cc898c7c8614278631cb2c16a62 |
| server | 0x2225F970 | 6 | 30 | 3bb7057383057b467573a706b63c99a8a0e5a96a9458c6b6295a720eef8e79f7 |
| server | 0x2225F990 | 8 | 6 | bbfc0932da48683f3210f699f5e5a3892cecaa5c8fd48ae3a2eb4a5bcfb5dbcc |
| client | 0x241056F0 | 0 | 11 | c6dd5f622ddbfbf71a8fc6775e469a11093e9667280a57175103822991205917 |
| client | 0x24105700 | 5 | 22 | 574b3832d44e07ecd636cbdc983c184df58eee49e690b4532e8b4aa5fafda8a8 |
| client | 0x24105720 | 7 | 10 | b35caac991eb0d06657c3fbba49c85de5f83e8cf4de0b07b70fa4b90cb5770fd |
| client | 0x24105730 | 8 | 4 | 916ae10aa3fd76a4d5b25b2fb66e9df0fd04957c7ab10930268d1ed7808be689 |
| client | 0x24105740 | 6 | 4 | 74032ad0e931f3c4730df99da43479f9f5315cc898c7c8614278631cb2c16a62 |

**12 complete new original SHA-reviewed functions: seven server, five client, 112 original x86 code bytes.** Two four-byte functions in separate original DLLs have an identical full SHA256 because their entire method instructions are identical; they are separately addressed and independently reviewed, not duplicate catalog rows.

## Original server HLTVDirector001 interface slots

- Slot 1: returns DWORD[this+4] != 0, via native SETNE.
- Slot 3: returns DWORD[this+4].
- Slot 4: returns DWORD[this+0x0C].
- Slot 5: returns DWORD[this+0x10].
- Slot 6: takes an output pointer, reads three consecutive DWORDs from this+0x14, +0x18 and +0x1C, and writes them in *forward* order to the caller's destination. Original native RET 4. Forward overlap can cascade.
- Slot 7: x87 FLD DWORD[this+8], returning original binary32 float in ST(0). The independent host returns float, not original x87 status/trap state.
- Slot 8: MOV EAX, original absolute VA 0x2251B054; RET. The pointer immediate in original code at 0x2225F991 has an original PE32 HIGHLOW relocation. Host uses a rebased address TOKEN, never dereferences it.

## Original client VClientEntityList003 interface slots

- Slot 0: reads DWORD[this+0x14 + index*8] using DWORD index from the first stack argument and RET 4. The stride is EIGHT bytes, not four. Host refuses invalid offsets/pointer wrapping.
- Slot 5: only the LOW BYTE of selector is compared with 1. If equal, returns modulo-32bit DWORD[this+4] + DWORD[this+0x0C]; otherwise simply DWORD[this+4]. In particular selector 0x101 takes the addition branch.
- Slot 6: returns DWORD[this+0x10].
- Slot 7: stores the first stack DWORD at this+8 and returns that DWORD in EAX, RET 4.
- Slot 8: returns DWORD[this+8].

Each of the twelve original final vtable DWORD slots lives in original .rdata, points to the exact .text method VA, and has a real PE32 HIGHLOW relocation in the original DLL. All entire original method SHA256s, proper C3 versus RET 4 cleanup instructions, exact INT3 padding and selected sparse opcode witnesses are checked.

## Source and tests

- research/tools/audit_sin_original_embedded_interface_virtual_accessors.py: authenticates BOTH original whole DLL SHA256s before crediting new methods; checks twelve complete native function SHA256s and boundaries, vtable slot pointers and twelve relocation sites, plus the server global absolute pointer and relocation.
- research/tests/test_sin_original_embedded_interface_virtual_accessors.py: synthetic-only mutations of each instruction witness, return, INT3 pad, vtable slot, HIGHLOW pointer, original server pointer and section/base. Synthetic bytes are not authentic recovered functions.
- include/sin_compat/original_embedded_virtual_accessors.hpp: portable C++17 independent byte-buffer reference for the twelve native field methods, relative to the embedded interface pointer. No real Source engine-owned object or proprietary method is executed.
- tests/test_original_embedded_virtual_accessors.cpp: verifies exact virtual slot identities, status and DWORD reads, x87 binary32 payloads, forward copying including overlap, global address rebasing, stride-eight client reads, low-byte selector, wraparound, setter/getter round-trip and host-only invalid pointer refusal.
- Root Linux CTest and native MSVC Win32 x86 CTest registrations test the clean-room models, not gameplay.

Owner-only original audit:

    python3 research/tools/audit_sin_original_embedded_interface_virtual_accessors.py --archive /private/bin.zip

Synthetic tests:

    python3 -m unittest discover -s research/tests -p 'test_sin_original_embedded_interface_virtual_accessors.py' -v

## Scope and next integration target

Reviewed whole-original native-function catalog **303 -> 315**, split **147 server / 168 client**. Only selected small methods in two vtables are recovered, not the complete interfaces. Unreviewed game methods, surrounding engine objects, virtual callbacks, CRT startup order and actual Windows game loading reconstructed gameplay DLLs remain substantial. Main platform order: **Windows PC first, Android ARM64 second, Quest 3 third**.
