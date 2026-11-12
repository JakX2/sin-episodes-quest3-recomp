# Phase 183: Original SiN 2006 client KeyValues nested lookup, creation and typed setters

12 November 2026. Continues draft Phase182 PR #235. The source of truth is the exact owner-supplied 2006 Win32 PE32 `client.dll` and `server.dll`, not a modern Source SDK substitution. No proprietary original DLLs, full native disassemblies, game assets or script data are committed.

## Source integrity and new original functions

- Original client `client.dll`: 3,592,192 bytes, SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`, preferred image base `0x24000000`.
- Original server `server.dll`: 6,049,792 bytes, SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- **Six distinct, complete, SHA-pinned original client functions covering 609 native bytes.** They were checked against actual local original PE32 code, not inferred from strings:

| Original entry VA | Complete body length | SHA256 of exact original body | Behavior recovered |
| --- | ---: | --- | --- |
| `0x241656E0` | 267 | `132b5ecb73db4bee8a0d930a26c35006115d25aceaf6d42b3e6a4558c46cfbc3` | Nested `KeyValues` FindKey-like lookup, optional creation |
| `0x24165AE0` | 126 | `e23c92e211203861ea33fa597159207d0dc27582008b2dcce8d598699425258a` | Set narrow string, free previous buffers |
| `0x24165B60` | 117 | `7e252f1ef17364fca5531956e1bda4cbeedc9032d4ead89c985bf6ba6e9f550d` | Set UTF-16 string, free previous buffers |
| `0x24165BE0` | 33 | `e2c8ded0de6cb99c3c1fd37ec712b02f4c882e206abf7a4fbcb20c06647d2ccd` | Find/create and write type `2`, raw DWORD +0x0C |
| `0x24165C10` | 33 | `b4ee4b29519066a4b4ab518ab541e7659a4b2ae1bfca7411025a243165a81829` | Find/create and write type `3`, raw float bits +0x0C |
| `0x24165C40` | 33 | `91ba8d9a8fffdb393e9a7f239751eb1d8ff53db6d57071b1c302421954a69764` | Find/create and write type `4`, raw DWORD +0x0C |

All first and last instruction boundaries were checked; the `FindKey`-like routine has **two distinct native `RET 8` exits**, so its complete code length includes both (`0x241657E8` contains the final `RET 8`, ending at `0x241657EB`). Both full original file digests are checked before claiming native coverage.

## Native binary behavior

### Nested find and create at `0x241656E0`

- Receives `this` in `ECX`, a C-string key path and a 32-bit create flag on stack; returns a KeyValues child pointer (or null), cleaning eight stack bytes.
- Null or empty key immediately returns null. The native path scans for slash ASCII `0x2F`. A path segment before slash is copied into a **fixed-size stack scratch buffer** without verified length bounds. Original KeyValuesSystem dispatch at IAT `0x24276230`, virtual offset `+0x0C`, converts the segment name to a DWORD symbol. **The external symbol interning algorithm and its case semantics have not been recovered here.**
- Starting at `this + 0x1C`, traverses linked child/peer nodes via `next+0x18`, matching the DWORD name symbol in node `+0`.
- After no local match, an optional fallback object at parent `+0x20` is recursively queried with creation disabled. Original recursion edges are verified at `0x24165777` and `0x241657D2`.
- On no result, creation-disabled returns null. Creation-enabled calls the external KeyValues allocator for `0x28` bytes, the constructor `0x24165010`, appends as a peer or installs the first child, clears the parent type at `+0x10`, then processes a slash suffix recursively.
- On invalid null allocation success, native code can dereference a null pointer. Portable reference adds explicit null checks and finite recursion/peer-scan guards. These are safety differences, **not** claims about original code.

### Narrow/wide and numeric payload setters

- `0x24165AE0` calls the above function with creation enabled. On a child it calls native free `0x2416FD60` for prior `node+0x04` and `node+0x08` buffers, clears the wide field, treats null source as an original empty narrow literal at `0x242D330E`, allocates `strlen(text)+1` via `0x2416FCF0`, copies the terminated string, and writes type `1` at `node+0x10`.
- `0x24165B60` mirrors that behavior for the wide pointer at `node+0x08`, clearing `node+0x04`, uses the original wide-empty literal at `0x242D37BC`, calls original wide-length helper `0x24249FA9`, allocates `(wchar_count+1)*2`, copies, sets type `5`.
- The three **separate** verified 33-byte numeric/raw setters all call `FindKey(create=1)` then write exact original value bits to `node+0x0C` and a corresponding type tag DWORD at `node+0x10`: `2`, `3`, or `4`. The float setter stores original 32-bit IEEE754 bits, not string formatting.
- The original small setters have no explicit string-buffer cleanup; the model likewise leaves preexisting string state intact on numeric writes. The original wide/string setter allocator failure and invalid incoming pointers are not defined as safe operations.

## Evidence and integration tests

- `research/tools/audit_sin_original_client_keyvalues_setters.py`: complete original paired-DLL SHA256 gate, six full native region SHA256s, **54 sparse opcode witnesses**, **16 exact native CALL rel32 edges**, **four Phase182 caller-site CALLs**, two original empty narrow/wide literals. No complete original executable function contents are embedded.
- `research/tests/test_sin_original_client_keyvalues_setters.py`: **11 original-free mutation and shape tests**, including all 54 opcode sites and all 20 direct CALL sites. Synthetic original-free fixtures fail full original function SHA checks.
- `include/sin_compat/original_client_keyvalues_setters.hpp`: original 40-byte x86 layout, 4-byte raw pointer fields, type/links offsets plus portable host-owning KeyValues tree, externalized symbol lookup and node creation, branch models for six entrypoints. Does **not** cast 64-bit host pointers to original MSVC32 objects.
- `tests/test_original_client_keyvalues_setters.cpp`: creation/no-creation, nested slash parsing, peer traversal, chained fallback, null/empty strings, narrow/wide free ordering, signed int, float bit identity, raw pointer tag, invalid input safety. Optimized GCC/Clang C++17 builds passed with `-Wall -Wextra -Wpedantic -Werror -UNDEBUG`.
- Added both root CTest and actual Windows x86 MSVC proxy-project CTest (Windows must execute and pass before this phase is declared CI green).

### What is not completed

Original `KeyValuesSystem` interface at IAT `0x24276230`, full constructor `0x24165010`, allocator/free `0x2416FCF0/0x2416FD60`, live game engine callback ABI, and actual Windows executable/level initialization remain open work. The recovered C++ is a **host-neutral reference**, not a replacement `client.dll` or proof of gameplay. The method ledger should grow from **146 to 152 unique reviewed functions (74 server, 78 client)**, barring overlap checks. Windows game first, Android second, Quest 3 third.

## Owner-local verification command

```sh
python3 research/tools/audit_sin_original_client_keyvalues_setters.py --client /original/client.dll --server /original/server.dll
python3 -m unittest discover -s research/tests -p 'test_sin_original_client_keyvalues_setters.py' -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_client_keyvalues_setters_tests
ctest --test-dir build -R sin_original_client_keyvalues_setters_tests --output-on-failure
```