# Phase 173: Original server classname registry insertion and symbol interning

11 November 2026. Stacked on Phase172 draft PR #225. Verified against the **actual original 2006 SiN Episodes: Emergence Windows PE32 `server.dll`** from the owner-provided private game archive, not a Source SDK guess. Proprietary executable regions, complete original disassembly and assets are not committed.

## Complete original PE32 evidence

- Original `server.dll` SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Original `client.dll` SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

| Verified original VA | Exact size | Original full region SHA256 | Role |
|---|---:|---|---|
| `0x2228B0A0` | 102 | `e09e609fb445ee14e43c8dcd9633b786c25609c1a81be79087cd8b3e11621ecb` | Registry 16-byte record insertion and indexing |
| `0x2228A3B0` | 98 | `364bc2a8b99b85727e3b1a44260fd397cd89ff2246e909323bd5169fe78c8ab1` | Registry free-list or freshly grown slot acquisition |
| `0x2228EA30` | 49 | `4ab5c9df0008a18273557239880acde69b643370c1c936e12d96a5f2bcff4f25` | First-fit string-pool chunk chooser |
| `0x2228F7A0` | 261 | `d84e5bbcbf05a7501ef5094690ae0827a991d8e63a33d33fe60d46a32d15268f` | Symbol interner: existing-name lookup or new pool-backed name insertion |

**510 complete native instruction bytes verified**, four entire-function SHA256 checks, 55 sparse instruction witnesses, ten internal direct `CALL rel32` sites and seven incoming call sites from previously recovered registration, lookup and symbol-wrapper code. The owner-only auditor chains Phase172 and therefore all prior reviewed methods and both full-original-DLL SHA256 checks. These functions are attributed at the instruction/call level; remaining engine-owned routines are not claimed reconstructed.

## Registry insertion, 0x2228B0A0

1. Accept the address of a caller-supplied key/record and a second DWORD argument. Invoke original `0x2228A280` with key, second argument and two stack scratch outputs. Its lower-level table search/unlink behavior is still opaque. **Do not call this a successful “find or reuse” operation** based only on its name.
2. **Unconditionally** call original slot allocator `0x2228A3B0`, even after the pre-insert operation. No branch skips the slot acquisition.
3. Call original `0x2228AF00` to link the slot using two stack scratch values returned/filled by the pre-insert operation.
4. Increment 16-bit registry insertion count at registry `+0x12` with modular wrap.
5. Compute `registry[+0x04 storage base] + index*16 + 8`. On normal (nonnull computed address) path, copy the 16-bit key word from `[key+0]` to `[record+8]` and the DWORD `[key+4]` to `[record+12]`. Return the slot as an unsigned 16-bit value.
6. The later Phase172 registration callback **overwrites** record `+12` with the provider pointer. This explains why insertion's initial 32-bit record value and final registered provider are separate stages.

The normal native path assumes valid allocations and pointer arithmetic; the host-neutral reference uses callbacks for the opaque `0x2228A280` preparation and `0x2228AF00` linking.

## Slot acquisition, 0x2228A3B0

- Read free-list head, a 16-bit value at registry `+0x14`; `0xFFFF` means no available recycled slot.
- On no free slot, compare capacity DWORD at `+0x08` with 16-bit used count at `+0x16`. If **equal**, call `0x22288390` to grow with integer argument `1`. Allocate slot at old used count and increment that 16-bit count, preserving original wrap behavior.
- If a recycled slot is available, keep its index, invoke `0x22288300` to obtain the recycled record, then replace the free-list head with the 16-bit value at record `+2`. Do not increment used count.
- For either path, write `0xFFFF` to the first WORD at `storage_base+slot*16+8` when nonnull; cache current storage base at registry `+0x18`; return index.
- This behavior is tested for fresh, recycled and chained slots, growth, and 16-bit counter overflow. The latter is an **invalid production precondition**, not a safety guarantee. The actual grow and recycled-record lookup stay host-owned.

## First-fit string-pool selection, 0x2228EA30

- Read signed pool chunk count at interner `+0x28`. If no chunks, return signed `-1`.
- Iterate array of chunk metadata pointers at `+0x1C` in ascending index order.
- Each chunk has 32-bit `capacity` at `+0` and `used` at `+4`. Compute 32-bit subtraction `(capacity-used)` and compare it **as signed** against the requested size (`JGE`). Return the first passing index; if none pass, return `-1`.
- The C++ reference mirrors 32-bit modular subtraction and signed interpretation, including unusual corrupted-size edge cases. This is not a vector-capacity heuristic or an assumed unsigned comparison.

## Symbol interning, 0x2228F7A0

1. Null incoming classname: immediately assign 16-bit `0xFFFF` to the caller's output and return, without modifying interner scratch or touching pool storage.
2. Nonnull classname: store raw pointer at interner `+0x30`, initialize two 16-bit local sentinel fields to `0xFFFF`, and call native `0x2228EB40` to search existing names. If it returns a valid 16-bit symbol ID, write it to output and return. The intern table's comparison/hash is not fabricated.
3. On a miss, scan the zero-terminated bytes. **Requested size includes the trailing NUL**. Call `0x2228EA30` to select the first chunk with sufficient capacity.
4. If no chunk fits, allocate a chunk whose `capacity=max(requested_size,0x800)`. The original passes **`capacity+11` raw bytes** to `0x2226E390`, initializes capacity and used metadata (used=0), then registers the allocation via `0x2228ED80` for a chunk index. The lower-level allocator and pool-vector metadata routines remain callbacks.
5. Read the selected chunk's old used offset. Copy precisely the `requested_size` bytes (including NUL) into the chunk payload at offset `+8+old_used`, using DWORD-then-byte tail copies. Advance the DWORD used count by the requested size.
6. Call native `0x2228F710` with the **16-bit chunk index and low 16 bits of the old used offset**. Its returned 16-bit symbol ID is assigned to the caller's output. The pool descriptor registration is not approximated with a guessed hash map.

The C++17 host-neutral reference faithfully preserves the observable branch and callback sequence without claiming to recreate the original MSVC32 object/stack-return ABI. It also exposes the native 16-bit descriptor truncation and 2048-byte minimum chunk behavior.

## Code, regression and execution

- `include/sin_compat/original_server_registry_intern_insert.hpp`: four portable C++17 functions, explicit original engine callbacks, original signed-comparison semantics, raw original-key field copy.
- `tests/test_original_server_registry_intern_insert.cpp`: complete fresh/recycled slot and count transitions; name hit/miss/null paths; pool first-fit; 2048-byte and oversized fresh allocation; copied NUL; chunk offset16 truncation; observable callback order and native DWORD wrapping.
- `research/tools/audit_sin_original_server_registry_intern_insert.py`: full original DLL chain and four complete native function hashes, 55 opcode witnesses, 17 original call edges.
- `research/tests/test_sin_original_server_registry_intern_insert.py`: nine independent tests with per-witness and per-call mutation, a 125-case signed first-fit matrix, a 256-case interning gate matrix and 64-case slot selection matrix.
- Root CMake registers `sin_original_server_registry_intern_insert_tests` and inherited Windows x86 / C++ CI.
- Recovery atlas **101 -> 105 manually reviewed original functions (55 server / 50 client)**. The first-fit chooser has concrete branch and arithmetic reconstruction; the other three remain partly host-dependent, conservatively marked `machine_code_verified`. This is **not a percentage of whole-game recovery**.

Owner-only native validation:

```sh
python3 research/tools/audit_sin_original_server_registry_intern_insert.py --client /private/client.dll --server /private/server.dll
python3 -m unittest discover -s research/tests -p test_sin_original_server_registry_intern_insert.py -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_server_registry_intern_insert_tests
ctest --test-dir build -R sin_original_server_registry_intern_insert_tests --output-on-failure
```

## Next original dependencies and explicit limits

- Native pre-insertion search/removal `0x2228A280`, registry slot linking `0x2228AF00`, symbolic string index `0x2228F710`, existing-symbol match `0x2228EB40`, and low-level chunk allocation/list insertion `0x2228ED80` remain priority targets.
- Actual engine entity allocator and registered `flak` provider virtuals are not yet implemented. No playable standalone original Windows 2006 DLL pair, Android gameplay runtime or Quest 3 playable game is claimed. Windows PC reconstruction remains the first milestone, before Android/Quest 3.
- The PR must remain **draft, stacked on Phase172 PR #225, unmerged**.