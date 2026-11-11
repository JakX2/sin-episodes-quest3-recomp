# Phase 174: Original 2006 server registry trees and symbol-record installation

11 November 2026. Decompilation of the owner-supplied original **SiN Episodes: Emergence (2006)** Windows PE32 `server.dll`, stacked on Phase173 draft PR #226. No original proprietary executable bodies or complete native disassembly is published.

## Exact original binary provenance

- Original `server.dll` SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Original `client.dll` SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- The original-only audit chains Phase173 and additionally verifies all four complete native function region hashes, 56 sparse original x86 opcode witnesses, four exact direct CALL rel32 instructions in these regions, and four known callers. Synthetic fixture mutations demonstrate each witness is effective and intentionally fail whole-function checks.

| Original `server.dll` VA | Full native length | Exact SHA256 | Behavior |
|---|---:|---|---|
| `0x2228A280` | 255 | `54592a1af0f32a419dc34900a07841d6f7ddf15e56cc48b80c06da7a998b78eb` | 16-byte-node registry insertion-position traversal |
| `0x2228A380` | 39 | `6913ebe2f4e0b5df55d05094db7e47e45db00f0a3388932c3da16f16ed296a3d` | Indexed-storage guarded release and clearing |
| `0x2228EB40` | 266 | `e100a658453e531d18ff90797618c5f3d2fa61304b9d8f54c50b7458f8804550` | 12-byte-node symbol ordered-tree search |
| `0x2228F710` | 94 | `7ca27d4cc81fb20c1576b8289ed71e4d7f1bf889baef72018a811357a799f52d` | Symbol record preparation, allocation, linking, and raw-word installation |

**654 newly fingerprinted original native bytes.** Recovering a function body and modeling its control flow does not prove the full engine-specific C++ object layout or native x86 ABI is executable in a replacement DLL.

## Registry insertion preparation: `0x2228A280`

Called directly from original `0x2228B0A0` at `0x2228B0BB`. Initializes the caller's output 16-bit candidate index to `0xFFFF` and direction byte to 0, **even if the native registry root at +0x10 is empty**. Otherwise walks a 16-byte-per-node tree starting at its 16-bit root.

On each visited index, it writes the current candidate index, calls the object function pointer at +0 using the original caller key and the node's data at +8, writes the callback's boolean as the direction byte, then follows the WORD at node +0 if true or node +2 if false. Both branches lazily initialize the same original four-WORD sentinel at `0x2258DF88` guarded by bit0 at `0x2258DF90`. The first initialization sets the guard *before* the sentinel WORD writes. The loop exits only after the next 16-bit index becomes `0xFFFF`. This is a **search for the insertion location, not insertion by itself**. The original native comparator and cycle/corrupted-index behavior remain external.

## Symbol search: `0x2228EB40`

Directly called by original interning `0x2228F7A0` at `0x2228F7DB`, and another original symbol routine at `0x2228F3EC`. Uses a **different** tree structure: 12 bytes per record, with child indices at record +0/+2 and comparison payload at +8. Traversal begins at the table's root WORD +0x10; if it is `0xFFFF`, returns the invalid sentinel immediately.

Each visited node calls the function pointer at table +0 with the search key and node payload (first comparison). A true result follows the node's first child WORD. If false, a *second* callback invocation swaps the argument order: **second false returns the current index as a match**, whereas **second true follows the node's second child WORD**. The sentinel is lazily initialized only when a child is followed, at global `0x2259012C` with guard bit0 at `0x22590134`. The two sentinel objects are distinct and must not be collapsed into one state.

### Why the distinction matters

The first function always traverses to a terminal index while reporting a parent/direction for insertion. The second function returns immediately on the equality case and distinguishes first-versus-second comparator argument order. Implementing them as a generic sorted-tree search would change native observable callback and output behavior.

## Symbol record registration: `0x2228F710`

Original direct CALL targets in order:

1. `0x2228F72B -> 0x2228E470`: prepare symbol record insertion operands and native callback state.
2. `0x2228F732 -> 0x2228E620`: acquire a 16-bit symbol slot, retaining returned `DI`.
3. `0x2228F746 -> 0x2228F460`: link/register the slot and the prepared operands.
4. Increment the original symbol table's WORD at +0x12, modulo 65536.
5. Address the original 12-byte slot as `[allocation + (slot*12) + 8]`; if the computed pointer is non-null, copy **one raw DWORD from the original first argument** into that field, and return the 16-bit slot. This is an actual raw data-word store after registration, not a hash computation or character copy.

The host-neutral model exposes the original three engine operations as ordered callbacks and models the count, slot scaling, and final write. Native allocation, record pointer validation, and ABI-specific source argument are not yet ported.

## Guarded indexed-storage release: `0x2228A380`

- If the object DWORD at +8 equals `0xFFFFFFFF`, returns without changing any fields.
- Otherwise, if allocation DWORD +0 is non-null, calls original `0x2226E3E0` with that pointer and clears DWORD +0.
- Then clears DWORD +4 unconditionally. Original DWORD +8 stays unchanged.
- Unlike a general-purpose destructor, this is a small specific cleanup routine. Calling it 'registry destructor' without verified type attribution would be misleading.

## Reconstructed code and tests

- `include/sin_compat/original_server_registry_tree_search.hpp`: portable C++17 models for the four functions, with callback boundaries for unresolved original comparators, slot allocation, symbol linking, and memory release.
- `tests/test_original_server_registry_tree_search.cpp`: empty-tree paths, two tree edge words and opposite comparator behavior, lazy sentinel initialization, matching-symbol short circuit, no-match, 16-bit counter wrap, and allocation cleanup.
- `research/tools/audit_sin_original_server_registry_tree_search.py`: owner-only SHA256-pinned native evidence checks chaining Phase173.
- `research/tests/test_sin_original_server_registry_tree_search.py`: ten original-free tests, per-opcode and per-CALL mutations, branch matrices, and full-hash rejection for fake fixtures.
- Root CMake/CTest target `sin_original_server_registry_tree_search_tests`.
- Recovery atlas **105 -> 109 manually reviewed original functions: 59 server and 50 client**. The new methods are conservatively `machine_code_verified` or `behavior_reconstructed` and **are not executable-original-ABI proof**.

Owner-local validation (does not upload the DLLs):

```sh
python3 research/tools/audit_sin_original_server_registry_tree_search.py --client /path/to/original/client.dll --server /path/to/original/server.dll
python3 -m unittest discover -s research/tests -p test_sin_original_server_registry_tree_search.py -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_server_registry_tree_search_tests
ctest --test-dir build -R sin_original_server_registry_tree_search_tests --output-on-failure
```

## Dependencies still missing

Native comparator function pointers, full real symbol-table allocator/map semantics, engine-local MSVC x86 entity construction and destruction, original `flak` projectile collision/impact callbacks, and full executable glue remain unresolved. These testable host-neutral models are steps toward playable **Windows DLLs first**, then Android and Quest 3, but **are not** a drop-in replacement client/server DLL pair.

Next native candidates: symbol-lookup comparator callbacks for the 12-byte tree, `0x2228E470` record preparation, `0x2228E620` slot acquisition, and `0x2228F460` record linking.