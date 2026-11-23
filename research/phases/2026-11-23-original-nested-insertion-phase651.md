# Phase 651: original SiN nested tree insert and string-block array insertion

## Owner priority, provenance and proof boundary

Continue recovering missing original *SiN Episodes: Emergence* gameplay/entity name registration from the genuine owner 2006 retail binary, **not** reimplementing HL2 physics, material/surface properties, models, props or rendering which the owner has already demonstrated working in an HL2 level port.

Exact unchanged owner-private PE32 originals:

- server.dll SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- client.dll SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

**Three complete original methods, totaling 306 native x86 executable bytes**, independently authenticated against the original server PE `.text`:

| Original method | Complete length | SHA256 of entire original method |
|---|---:|---|
| `0x2228ED80` block-pointer array indexed insertion | 100 bytes | `b5212200ca591fab856c37cf4f4179a5585b9c7729d29158ef167ac45f9a9a52` |
| `0x2228F710` 12-byte nested-tree reference insertion | 94 bytes | `7ca27d4cc81fb20c1576b8289ed71e4d7f1bf889baef72018a811357a799f52d` |
| `0x2228E280` DWORD-array capacity growth | 112 bytes | `1b91b29a1820e36300655fa1f9440dfc0f2653be30b61ca5a6f1ea44d6e8565b` |

The private Python auditor checks **9 exact original relative CALL edges** including both Phase650 callers, **12 original branch destinations**, **125 address-pinned original x86 instruction witnesses** and whole original DLL fingerprints. Original `0x2228ED80` and `0x2228F710` end with INT3 padding. `0x2228E280` immediately precedes another valid entry at `0x2228E2F0` instead of INT3; the auditor verifies that next native entry's first four bytes and never incorrectly requires padding. Original binaries and proprietary byte extracts remain private; public self-test uses only invented bytes.

## Original array growth `0x2228E280`

Native ECX points to a three-DWORD array owner `[array+0]=pointer`, `[array+4]=capacity`, `[array+8]=growth-step` and the call has one DWORD requested increment.

- Growth-step `0xFFFFFFFF` is a full early return without any allocator activity.
- The native signed comparison is between capacity and `(capacity + increment)` with wrapping 32-bit arithmetic.
- For insufficient capacity, a zero capacity initializes to **8**, a zero growth-step doubles capacity, otherwise it adds the configured step, writing the new capacity to guest memory *inside each loop iteration*. The signed comparison repeats until sufficient.
- Regardless of whether growth was actually necessary, non-disabled execution calls the original engine allocation wrapper with **capacity × 4 modulo 2^32** bytes, using `0x2226E390` for null pointer and `0x2226E400` for an existing pointer. The return pointer, including null, overwrites `[array+0]` without native rollback.
- The underlying allocator ABI was recovered in Phase645. Its actual game-engine memory-manager implementation remains host supplied. Portable protections against pointer overflow, missing callbacks and pathological wrap-around are documented deviations from original native failure behavior.

## Original indexed block-pointer insertion `0x2228ED80`

Native ECX is the full block array object at **`map+0x1C`** with extra fields `[owner+0xC]=count` and `[owner+0x10]=cached array pointer`. Its two stack arguments are **insert index** and **pointer to a DWORD block pointer**, not merely a block value.

1. Load count and capacity. If **signed** `(count + 1) > capacity`, call original `0x2228E280` with increment `count + 1 - capacity`, preserving native 32-bit wrap.
2. Increment count at `[owner+0xC]` **before** copying or moving anything.
3. Read `[owner+0]` and assign the cached pointer `[owner+0x10]` even if no reallocation occurred.
4. If signed `count_after - index - 1 > 0`, call original `0x223A5D10` with **destination = array + index×4 + 4**, **source = array + index×4**, **byte length = (count_after - index - 1)×4**. Source/destination overlap, so this is a true memory-move dependency, not memcpy.
5. Read the DWORD through the second argument pointer and write it to array[index], then return the original insertion index. With a native null slot destination, it skips the final write.

`0x223A5D10` remains a mandatory typed host operation; this phase does not falsely label it reconstructed. Tests verify the shift, correct order of side effects, relocation preservation, middle insertion at capacity, and refusal behavior when a later callback fails.

## Original tree insertion `0x2228F710`

Native ECX=map, two stack arguments (pointer to 4-byte string reference and pointer to comparator context). The original method:

1. Calls `0x2228E470(map, key-ref, parent-index-out, branch-byte-out, context)` to determine the insertion position, writing the two intermediate results through stack pointers.
2. Calls `0x2228E620(map)` to acquire a new **12-byte nested-tree slot**, receiving a 16-bit index in AX.
3. Calls `0x2228F460(map, acquired-handle, parent-index, branch-byte)` to set node links and perform original red-black rebalancing through **its own** deeper call to `0x2228EDF0`.
4. Increments WORD active count at **`map+0x12`**, independent of the WORD next-unused slot at +0x16.
5. Re-reads array pointer `[map+4]` and writes the first **DWORD of the four-byte string reference** to `array + handle×12 + 8`.
6. Returns the acquired WORD handle in AX, native `RET 8`.

The methods `0x2228E470`, `0x2228E620`, `0x2228F460` and its `0x2228EDF0` rebalance remain explicitly typed host callbacks or downstream original dependencies. This phase reconstructs the **complete 94-byte wrapper and its verified sequencing**, not those unresolved methods. In particular, tests of two or more names use a *synthetic* host linker which preserves search topology but does **not** prove original red-black invariants. Do not mistake this for game-engine execution.

## Source reconstruction and tests

New C++17 source classes: `ArrayGrowth`, `BlockArrayInsert`, `TreeInsert`. All use a typed guest-memory/allocator/relocator host, record stage/failure/committed writes, and guard invalid guest pointers, endless growth, and reentry defensively. These guards are not native 2006 x86 error semantics.

- Standalone strict GCC C++17 with `-Wall -Wextra -Wpedantic -Werror` **passes**, and a separate ASan+UBSan compilation and test **passes**.
- Tests cover zero -> eight -> sixteen and larger DWORD capacity, additive growth-step, disabled sentinel, no-growth reallocation, signed and wrapping 32-bit capacity semantics, overlapping rightward move, insert-in-the-middle of a fully used block pointer array, count/cache write ordering, partial successful writes when later callbacks fail, throwing allocator, reentrant callback, and 12-byte tree node handle/reference storage.
- Additional compiled Phase648-651 integration test combines the previous original `0x2228F7A0` lookup, `0x2228EB40` bidirectional search, Phase648 reference resolver, Phase649 ASCII-mode delegated comparison, and Phase651 original modeled indexed array insert, its **original modeled growth**, and original modeled tree insertion. The host now supplies **only** the still-unrecovered tree-parent search, slot acquire, tree link/rebalance, memory move, and actual native allocator internals. No fake success-only `append` or `register` path remains in this test.
- The test registers names, verifies ASCII-case-insensitive duplicate matching, left/right traversal, and creates **nine distinct string blocks** to force the block-pointer array's **8 -> 16 synthetic relocation**, then verifies an old name remains discoverable after the relocation. It also tests an inserted DWORD at the middle of a full pointer array, which invokes a genuinely overlapping guest-memory move callback.
- Prior 39-symbol Phase638-649 outer tree test remains intact on the stacked branch. That test forwards synthetic numeric symbol identifiers, so it is **not** falsely claimed as the in-engine name-registration path.
- Standalone owner-private audit passes on the two unchanged retail DLLs. The invented-byte mutation auditor supports public CI via `--self-test`.

### Commands

Owner private audit:

`python research/tools/audit_original_nested_insertion_phase651.py --server ORIGINAL_SERVER_DLL --client ORIGINAL_CLIENT_DLL`

Public original-free mutation test:

`python research/tools/audit_original_nested_insertion_phase651.py --self-test`

## Remaining original SiN gameplay dependencies

The named native targets for subsequent recovery are `0x2228E470` (tree insertion-location search), `0x2228E620` (12-byte node acquisition/free-list handling), and `0x2228F460` (tree link and rebalance via `0x2228EDF0`). The actual engine allocator and original `0x223A5D10` memory-move helper also remain separate dependencies. The user already has working HL2-level rendering and physics; no reimplementation of those systems is warranted.

This phase does **not** demonstrate running the owner's original retail game DLL in a Windows process or executing playable SiN Episodes on Android/Quest 3. Cross-platform CI is a source-model compatibility test, not a game boot.
