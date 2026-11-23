# Phase 650: SiN 2006 original nested string lookup and search

## Owner priority

Recover missing **SiN Episodes: Emergence** 2006 symbol/entity/NPC gameplay routines. Do not duplicate HL2-equivalent physics, material/surface parameters, models/props, or rendering, which the owner already verified when porting levels into HL2.

This is original nested symbol **lookup and string storage**, not a new generic map implementation. The retail `server.dll` and `client.dll` remain private. Both unchanged whole-file SHA256s were independently verified:

- server.dll SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- client.dll SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

## Native provenance: 578 complete original bytes

| Original VA | Full size | Original whole-method SHA256 |
|---|---:|---|
| `0x2228EB40` two-direction nested tree search | 268 | `5e96923bb70b37ce9ee0c8028727f6c5487b9ad1b3026629dc1440e436fafe12` |
| `0x2228EA30` string block-fit selection | **49** | `4ab5c9df0008a18273557239880acde69b643370c1c936e12d96a5f2bcff4f25` |
| `0x2228F7A0` original name lookup and absent-name insertion | 261 | `d84e5bbcbf05a7501ef5094690ae0827a991d8e63a33d33fe60d46a32d15268f` |

The original block-fit helper is **49** bytes, NOT the preliminary 47 bytes, because its `RET 4` is 3 bytes long and must be included in the whole-method hash. All three whole original methods are followed immediately by `INT3` padding and belong to the original PE32 executable `.text` section.

The owner-only auditor checks the complete original method SHA256s, **5 original rel32 calls** (`0x2228F7DB` to search, `0x2228F806` to fit, `0x2228F823` to memory allocate, `0x2228F844` to append block, `0x2228F88D` to nested tree registration), **19 control-flow branch edges**, **76 address-pinned original x86 opcode witnesses**, and **2 indirect comparator call sites** in the search function. A public original-free `--self-test` independently mutates invented methods/calls/branches only. No proprietary game DLL or binary code blobs are checked into GitHub.

## Original 0x2228EB40 semantics

Native `__thiscall ECX=map`, stack args `(string_ref* key, map* comparator_context)`, `RET 8`. The root is a WORD at `map+0x10`; FFFF is empty. Array pointer is the DWORD at `map+4`; the node stride is **12 bytes**, with child indices in WORDs `slot+0` (left), `slot+2` (right) and the **four-byte string reference** beginning at `slot+8`.

At each visited node the original first invokes the comparator pointer stored at `[map+0]` using cdecl `(key_ref, node_ref, context)` and checks AL:

- true: descend **left**;
- false: invoke the same pointer in **reverse order** `(node_ref, key_ref, context)`;
- reverse true: descend **right**;
- both false: the key is **equivalent** under the map's comparator, so return the existing node index in AX. (This handles differences in original spelling when the map uses case-insensitive comparison.)

On the first left/right traversal, original global `0x22590134` low flag bit is set and four original sentinel WORD fields at `0x2259012C` initialized to `FFFF, FFFF, FFFF, 0001`. It does not initialize this sentinel for an empty tree or an immediate equality return. The portable reference retains this lazy state and actual committed host-memory writes. The comparator callback can run the reconstructed **Phase648 nested comparator → Phase649 0x223A5574 mode switch**.

The portable 65536-iteration cycle cap, out-of-range address refusals, callback-failure status, and reentrancy refusal are protective modeling behavior, **not native retail x86 exception/failure handling**.

## Original 0x2228EA30 semantics

Native `__thiscall ECX=map`, arg1=DWORD `bytes_required`, `RET 4`. Reads **signed** number of blocks at `map+0x28`; when count <=0 returns `FFFFFFFF`. Otherwise iterates the array of block pointers at `DWORD[map+0x1C]` and finds the **first block where signed 32-bit `(capacity - used) >= signed(bytes_required)`**, taking 32-bit overflow semantics. Each block has `DWORD capacity+0`, `DWORD used+4` and character storage at `block+8`. Returns block index in EAX or `FFFFFFFF` when none fits.

## Original 0x2228F7A0 semantics

Native `__thiscall ECX=map`, arguments `(WORD* out_tree_handle, const char* name)`, `RET 8`:

1. Null `name`: writes FFFF to output immediately, with no map mutation.
2. Otherwise stores the input name pointer at `DWORD[map+0x30]`. Reuses the original incoming string-pointer stack argument as a mutable four-byte **FFFF/FFFF temporary string reference**. This makes the Phase648 comparator resolve the temporary key from the original map's fallback pointer.
3. Calls original `0x2228EB40` tree lookup with that reference and context map. If found, stores/returns the existing 16-bit tree handle. **No copying or new allocation occurs**.
4. If absent, computes original `strlen(name)+1` including the terminator, then calls `0x2228EA30` to find a block with enough space.
5. If no block fits, allocates **max(length,2048)+11 bytes** via original `0x2226E390`. Initializes block header capacity and used=0, then calls original **`0x2228ED80`** to append its pointer to the array beginning at `map+0x1C`.
6. Reads selected block's current used count, copies the whole terminated string to `block+8+used` (retail x86 `REP MOVSD` then `REP MOVSB`), and advances the block used count by length.
7. Constructs string reference `WORD(block_index), WORD(old_block_used)` (including original 16-bit narrowing), calls original **`0x2228F710`** to register the reference into the nested tree, and stores its returned AX handle in the caller's output WORD.

**Important boundary:** The implementations of the two deeper functions **`0x2228ED80`** (array append/growth) and **`0x2228F710`** (tree slot insertion) have **not** been fully decompiled in this phase. They remain typed, required guest host callbacks, as does allocation via Phase645 original dispatch. The string lookup, first-fit, bidirectional comparison, duplicate detection, new block header/string copying and forwarding of insertion-reference values are now modeled source operations. The owner source's allocator and original string decoding are not fabricated.

The source uses an explicitly caller-supplied guest `scratch_ref` instead of mutating the real x86 incoming stack word. That abstraction is documented and testable. Existing 39-symbol outer tree registration tests remain, but their name parameters are invented integer tokens; they cannot honestly be claimed to run through the original C-string-based inner lookup until a separate string-backed integration path is used.

## Verified integration and regression

New strict C++17 standalone integration composes real compiled **Phase648** (original 113-byte delegated nested string comparator) and **Phase649** (67B dispatch/78B ASCII comparator) implementations with Phase650 search/fit/lookup models, plus explicit synthetic host callbacks for the still-unrecovered append and insertion. It exercises:

- Null string returns FFFF without touching the map.
- First insertion of `Alpha`, with 2048-byte default block capacity, `used=6`, new ref `(block=0,offset=0)`, output handle 0.
- Repeated differently-cased `ALPHA` returns the same existing handle, with **no additional string copy, block allocation, or nested registration**.
- `Zulu`, `aardvark`, `zulu` traverse both directions, take the same case-insensitive identity behavior and leave original sentinel initialization traces.
- Forced capacity exhaustion allocates **a second block** with ref `(block=1,offset=0)`, and `ECHO` finds it without new storage.
- Signed original block-fit comparisons, required host allocation/append failure without inventing rollback, original fallback pointer writes, comparator exception/reentrancy refusal, and deliberately cyclic corrupted tree capped defensively.

Local **Clang C++17 `-Wall -Wextra -Wpedantic -Werror`**, standalone compiled CTests, and independent **AddressSanitizer/UndefinedBehaviorSanitizer** pass. The owner-only audit runs on the genuine retail DLLs and succeeds. The public invented-byte auditor self-test passes. The PR builds the C++17 model and tests on native Linux and Windows x86; the existing Android ARM64 compilation and x86_64 Bionic `dlopen` smoke remain compatibility checks only.

## Next priorities

1. **`0x2228F710`** original nested-tree insertion (94 complete native bytes), required to eliminate the synthetic insertion callback. It calls `0x2228E470`, `0x2228E620`, and `0x2228F460`.
2. **`0x2228ED80`** original block pointer array append (100 complete native bytes), required to eliminate the synthetic block allocation-list management; uses `0x2228E280` grow helper and original move function.
3. Establish actual runtime behavior within retail Windows game, then adapt it to Android/Quest 3. Never conflate synthetic tree tests or Android module `dlopen` with a playable SiN game.
