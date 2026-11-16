# Phase 245: original retail client array-owner conditional release

Stacked on Phase 244, PR #299. Owner-provided `sin files/bin.zip` in the linked Drive contains both full PE32 retail DLLs. This phase independently extracted them **privately**, checked complete SHA256s, and disassembled original x86 with GNU objdump. No DLL, original body, or disassembly is committed.

- Original client PE32: 3,592,192 bytes, SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- Original server PE32: 6,049,792 bytes, SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- New client function: VA `0x24105FE0`, **39 complete original bytes**, SHA256 `305edbceeb790e6a91f91fa5815f31c11f606f71e1831384350bb5c265186fe2`
- Original direct call: `0x24105FF0 → 0x2416F940` (native free routine).
- Return: `0x24106006` plain RET; nine trailing INT3 alignment bytes through `0x2410600F`.
- Host behavior: `this+8 == 0xFFFFFFFF` is a complete no-op. Otherwise if `this+0` pointer is non-null, free it and zero it; always zero `this+4` count. The owner is at least 12 bytes.
- The host model uses `std::vector` ownership and a synthetic release counter; it is **not** a native allocator, game ABI object, or independently loadable gameplay DLL.

Private auditor verifies both full-file hashes before inspecting function bytes, then checks the 39-byte body SHA, RET, alignment, and relative call target. Test with `python3 tools/audit_original_client_array_owner_clear_private.py /private/client.dll /private/server.dll`. Host regression: `c++ -std=c++17 -Wall -Wextra -Werror -pedantic -Iinclude tests/test_original_client_array_owner_clear.cpp -o /tmp/sin-phase245-test && /tmp/sin-phase245-test`.

This adds **one newly verified client function**, moving the Phase244 inventory from 361 to **362 (147 server, 215 client)**, contingent on the stacked predecessor remaining unchanged. Native original-code byte credit increases by 39. No retail engine initialization has been established; Windows x86 remains first priority, then Android, then Quest 3.
