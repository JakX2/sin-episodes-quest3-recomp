# Phase 547: Original 2006 client token storage, sentinel and string-page ownership

## What was examined

The owner-provided original 2006 *SiN Episodes: Emergence* `client.dll` and `server.dll` were read **locally**. The whole-file SHA-256 identities were independently checked before verifying native code:

- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

This continuation follows Phase 545/546's *distinct* string-token red-black tree and two string comparator modes, rather than the earlier 24-byte converter registry. The original `0x241D3900` token-index allocator uses original helpers `0x241D33E0` and `0x241D34E0`; the original `0x241D4A80` intern routine uses `0x241D3D10` and `0x241D4060`. These original CALL relationships are separately rechecked, not inferred from method proximity.

This is independently authored, **bounded, readable source-level reverse engineering** of the original binary's storage path. Source names and C++ types below are descriptive, **not symbols recovered from Ritual's source code**. The original executable bytes, PDBs, full disassembly and licensed game files are not committed.

## Nine independently authenticated original methods

| Native original VA | Exact complete bytes | Original complete-function SHA-256 | Bounded role |
|---|---:|---|---|
| `0x241D33E0` | 84 | `53ae0afeb7402568cb9a6b9b1feeba349e7a16e4070e7a1f2205c963d4f366ff` | Resolve a 16-bit token-node index to a 12-byte record, or the original global sentinel |
| `0x241D3440` | 39 | `d74a6d305dd4c5f752fbc8125f8e5ecf7b597c9e68710715b91b78c9eff00f39` | Release a dynamic array only when the original grow-mode permits it |
| `0x241D3470` | 53 | `4ddcc57be0e926c518af6de9dc1c9e9becb495545a855d573a6a0ce2e81a59ed` | Shift a run of 4-byte pointer-array elements left, without updating logical count |
| `0x241D34E0` | 115 | `b01c92221c5fdb33aa5135ef4c01968b9b93c0e6fa0add63e93489475cfe34a3` | Select node-array capacity and call native allocator, 12 bytes per element |
| `0x241D3560` | 112 | `ec6c3aa11039b1dfe047e7956b901b39027e56ea76efe1b2e954d8b655d61a8e` | Select pointer-array capacity and call native allocator, 4 bytes per element |
| `0x241D35D0` | 17 | `a3769bba1d056aa8563ed86f52ef4fcb70fbd830eb1520a99cc079c785565142` | Return 16-bit parent index from the resolved node record `+0x04` |
| `0x241D3960` | 46 | `4723bd92f6896d1f8887b2652295669e8280e5d4373b6cf2947b7947ed0927a4` | Extend a 4-byte pointer-list by a requested count and refresh its cached pointer |
| `0x241D3D10` | 49 | `4ab5c9df0008a18273557239880acde69b643370c1c936e12d96a5f2bcff4f25` | Find the first string page with at least a requested number of bytes free |
| `0x241D4060` | 100 | `51a642927a366b450ae5ffc17e16160325e17f12af49f89c7fd914bcf6487712` | Insert a pointer into the 4-byte string-page list, shifting later pointers right |

**Total: nine complete and non-overlapping methods, 615 authentic original instruction bytes.** The client binary has an important boundary nuance: `0x241D3560` ends at the exact address `0x241D35D0`, where the **separate** 17-byte accessor begins **without INT3 padding**. The auditor checks the full 112-byte native growth method and the next method's entry independently. The other eight functions have `INT3` at their exact end boundaries. Counting from the next INT3 instead would mistakenly fuse two different functions.

## `0x241D33E0`: explicit missing-node singleton

This method accepts a native object through `ECX` and a 16-bit node index on the stack. It also contains a **first-use global initialization** path:

1. Read original global flag at `0x24377054`, bit `1`; if not set, OR that bit into the global flag.
2. Initialize three consecutive unsigned 16-bit values at global `0x2437704C`, `+2`, `+4` to `0xFFFF`, and the word at `+6` to `1`.
3. Read the caller's 16-bit node index.
4. If index is **not** `0xFFFF`, use the native node-array pointer stored at object `+0x04`; return `array + 12*index`.
5. Otherwise return **the address of the shared original sentinel**, `0x2437704C`.

A bounded source-like description of the selection itself:

```cpp
// The actual original has a global bit-controlled sentinel initializer.
NativeNode* resolve(TokenTree* tree, uint16_t index) {
    if (index == 0xFFFF)
        return sharedOriginalMissingNode();  // 0x2437704C
    return reinterpret_cast<NativeNode*>(tree->nodeData) + index;
}
```

The pseudocode is **not** a drop-in native ABI replacement. This is not merely `return nullptr` for missing tokens: the native function deliberately returns a real global record address. Initial sentinel fields support prior red-black fixup reports; the original lazy-init bit alone does *not* prove thread-safe one-time initialization or absence of races.

The small, separate `0x241D35D0` wrapper calls this helper and reads the node's **16-bit parent word at +4**. It is not part of the growth routine immediately preceding it.

## `0x241D34E0`: 12-byte node-array capacity and allocation

The function takes the array subobject through `ECX`, with native `data` pointer at `+0`, signed 32-bit capacity at `+4`, and growth mode/increment at `+8`.

- Mode `-1` returns without attempting allocation. The same value also governs the adjacent release routine, and is best described as a **no-allocator/fixed-storage guard**, not proof of external ownership in all cases.
- Otherwise, the function derives a target from `capacity + additionalCount`, and if capacity is insufficient enters a loop:
  - Existing capacity zero: initialize capacity to **3**.
  - Existing capacity nonzero and growth increment zero: **double** capacity.
  - Existing capacity nonzero and growth increment nonzero: **add** that increment repeatedly until capacity is enough.
- It then computes **`capacity * 12`** bytes. If the current array pointer exists, it calls original `0x2416F960` (resize/reallocation-style). Otherwise it calls original `0x2416F8F0` (allocation-style). The resulting pointer is stored at `+0`.

The original uses signed comparisons and unchecked 32-bit arithmetic. The binary **does not establish safe handling of null allocation return, overflow, or unusual negative increments**. Those require independent host safety checks; a host should not simply transpose the original unchecked memory operations.

Caller connection: `0x241D391C` invokes this helper on the **array subobject at token tree `+0x04`**, with an additional element count of `1`, when the high-water token index reaches the current capacity.

## `0x241D3560`: distinct 4-byte pointer-array capacity

This is structurally related to, but **not identical to**, the preceding 12-byte function. Its initial zero-capacity size is **8** elements, and it passes **`capacity * 4`** bytes to the same original allocation or resize routines. It also has the `-1` no-allocation guard and the additive-vs-doubling growth control. The original pointer lives at array-subobject `+0`, capacity at `+4` and growth-mode at `+8`.

It is called by the original string-page-list operations `0x241D3960` and `0x241D4060`. The user-facing project already had the larger string-token tree; we should **not** confuse its 12-byte nodes with the storage of the string *pages*, which is an array of native 4-byte pointers.

The release helper `0x241D3440` checks the same `+8` mode against `-1`. If permitted, it frees the non-null original native array pointer using `0x2416F940`, sets data pointer to null, and sets capacity to zero. It does **not** assert the whole tree or page registry was destroyed.

The neighboring `0x241D3470` moves a 4-byte array run left using original `0x24248B50`, only when the remaining count is positive and shift amount is positive. This helper **does not itself decrement the array's logical element count**, so calling it alone is not an erase operation. Both start/shift inputs are stack arguments; caller-side bounds are unproven.

## `0x241D3D10`: the original string-page space search

This compact helper is directly invoked by the previously authenticated string intern routine `0x241D4A80` at site `0x241D4AE6`. It reads the native page-count at token context `+0x28` and the page-pointer table at `+0x1C`. For each page pointer, it reads the page's first two **32-bit header fields**:

- page `+0`: total storage bytes;
- page `+4`: already consumed bytes.

The function returns the first page index for which `total - used >= requested` under the original signed comparison. If no page qualifies, it returns **`0xFFFFFFFF`**. The original contains **no allocation** inside this routine.

```cpp
// Bounded logical algorithm for valid nonnegative page header fields.
int findPageWithFreeBytes(const Page* const* pages,
                          int pageCount, int bytesNeeded) {
    for (int i=0; i<pageCount; ++i)
        if ((pages[i]->total - pages[i]->used) >= bytesNeeded)
            return i;
    return -1;
}
```

This directly explains the larger caller's fallback: when no page has enough room, it allocates a fresh string-page block, then inserts that new page pointer into the page list. That later allocation and copy logic belongs to the **previously authenticated** `0x241D4A80` body and is **not double-counted** here. The original page header arithmetic is not proof of validation against corrupt headers.

## `0x241D3960` and `0x241D4060`: page pointer-list operations

`0x241D3960` extends a 4-byte-pointer list by a requested count. It compares `count + requested` to capacity, calls `0x241D3560` when additional storage is needed, increments `count` at list `+0x0C`, and refreshes the cached native backing pointer at list `+0x10`. The input is treated as a count, **not bytes**.

`0x241D4060` inserts a single 4-byte pointer at a caller-supplied position. It ensures a capacity for `count+1`, increments `count`, moves later entries up by one with original `0x24248B50` when the insertion is not at the end, copies the supplied pointer into the new position and returns the position. Important: the method does not prove that arbitrary out-of-bounds positions are accepted or safe; its regular caller supplies a position chosen by the string-page allocation path.

The prior original intern routine calls this insertion helper at **`0x241D4B24`**, passing the selected page-array index and the address of the newly allocated page pointer. Combined with the `0x241D3D10` page-space scan, this reveals the complete *control-flow chain* for choosing a page or appending a new page entry. It does **not** by itself prove every subsequent byte-copy and native failure path of the larger intern routine.

## New independent C++ reference and checks

`include/sin_compat/original_client_token_storage.hpp` models only **authentic observed decision logic**:

- 16-bit node index to 12-byte offset or special sentinel classification;
- distinct x12 vs x4 array growth rules, initial capacities 3 and 8, additive/doubling selection and `-1` fixed-storage guard;
- first-page-with-room search from the original two-field page header.

`tests/test_original_client_token_storage.cpp` tests the field sizes, 16-bit sentinel, byte strides, growth scenarios, fixed-storage mode, nonpositive-invalid host inputs, 32-bit overflow refusal and empty/exhausted page lists. These use **synthetic host inputs only**. Bounds and overflow rejection are independent host protections, not assertions about the original 2006 x86 crash or exception behavior. The code **does not simulate native allocation, memory ownership, x86 exception handling or exact page insertion ABI**.

`research/tools/audit_sin_original_client_token_storage.py` validates the owner's *entire* original server/client SHA-256 identities, nine independently sized complete original method SHA-256s, **15 direct CALL rel32 edges**, **43 exact instruction witnesses** and the real adjacent-method boundary. The offline `--self-test` corrupts every byte of a synthetic CALL, verifies refusal and separately refuses fake whole-file DLL hashes.

Local commands:

```sh
python3 research/tools/audit_sin_original_client_token_storage.py --self-test
python3 research/tools/audit_sin_original_client_token_storage.py --bin-zip /path/to/your/original/bin.zip
```

Original DLL bytes remain owner-private and are never committed. The locally run original audit passed; GCC and Clang C++17 reference tests passed with strict warnings-as-errors.

## What remains unclear

- `0x241D34E0` and `0x241D3560` do not prove a safe allocation-null or signed-overflow handling policy. The original might not have had one.
- Actual 2006 string-page allocator/destructor ownership, underallocation and release behavior: cross-call to `0x2416F8F0`, `0x2416F960`, and `0x2416F940` is verified, but these are only *roles* in this analysis.
- The remaining `0x241D4A80` intern path's exact write/lifetime ordering on invalid memory, and the complete semantics of the 1,387-byte token red-black fixup.
- A genuine original game-engine-owned `client.dll` or a coherent project-wide translation of all native functions. Decompilation and Windows/Android/Quest runtime integration remain separate milestones.

**Next research target:** `0x241D3D10` now has a complete page-search description; continue with the native string-page construction and release helpers reached from `0x241D4A80` and its teardown path, then revisit the remaining token-tree color/rebalance branches. Focus game-specific code rather than reimplementing HL2 physics, materials, models or props.