# Phase635: actual 2006 Elite event-ID range conversion and static mapping owner

## Why this is gameplay-specific

Phase634 recovered original Elite `0x220398A0` flag clearing and `0x2203D2D0` conditional state-4 behavior from retail `server.dll`, but both still treated original `0x22073700` ID conversion as an opaque host callback. Phase635 recovers that complete native converter, pins the exact **Elite** virtual `+0x760` source of its mapping context, and identifies why runtime mapping ranges cannot be read from the PE32 file directly. No changes to the user's already working HL2 map, prop, rendering, physics, surfaces, model or material code.

All source claims refer to the owner-provided **original 2006 retail PE32 x86**:
- `server.dll` SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll` SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

## Two additional complete authenticated original methods (64 native bytes)

| Exact original method | Bytes | Whole source SHA256 |
|---|---:|---|
| `0x22073700`: linked-range source-ID converter | **58** | `5927384b29bc8c8ce0525ca759560659f83a87d2095bc051d0a1763c97df95c9` |
| `0x22355D40`: Elite virtual `+0x760` static mapping owner getter | **6** | `e80e4e201bee1506627b153171f783c06f9433cfee905f62d08a2dc048dbcc62` |

The original Elite **primary** vtable base is `0x2248DF04`; the exact DWORD at its slot **`+0x760`, VA `0x2248E664`** points to `0x22355D40`. This getter has the complete authentic code shape `mov EAX,0x2259C2F8; ret` and therefore returns a **fixed native mapping owner** `0x2259C2F8`, not a dynamically guessed or per-actor object. In the two Phase634 original callsites, original code adds **`0x34`** to the returned pointer, yielding the embedded first mapping node at **`0x2259C32C`**.

These are two distinct complete original-source methods, adding **64 original executable bytes**. The rest of the C++ model and tests are independent, non-proprietary source-backed reconstructions; no original game code or DLL bytes are committed.

### Complete `0x22073700` native control flow

1. Get signed 32-bit input ID from **`[ESP+4]`**. If **`-1`**, immediately return **`-1`** without reading the mapping head.
2. Otherwise start from the mapping-head `ECX` (the original caller computed virtual `+0x760` return pointer **`+0x34`**).
3. At the current node, read source low from **`[ECX+4]`**. If the low is special **`0x270F = 9999`**, skip this node unconditionally.
4. Otherwise check signed **`input >= source_low`** and signed **`input <= source_high`**, where source high is **`[ECX+8]`** and **inclusive**. If either comparison fails, load the next pointer from **`[ECX+0x10]`** and continue until a null pointer.
5. At the **first** matching node, compute original EAX `[ECX] - [ECX+4] + input` using native x86 **32-bit modulo wraparound** and return it. `[ECX]` is destination-range start. All source-range checks are signed despite modular destination arithmetic.
6. If the linked list is exhausted without a match, return signed **`-1`**.

The original converter has **no cycle check, no node budget, no null-head check and no table-size proof**; a malformed owner mapping can potentially hang or access an invalid address. The safe portable reference keeps native semantic decisions on valid linked lists but reports and refuses null heads, cycles, host read failure, exceptions, reentrant calls, and chains exceeding 512 entries. These are deliberate host guards **not claimed to exist in the 2006 original**.

A mapped expression can itself produce `-1` under 32-bit wrap. The reference distinguishes `Mapped(output=-1)` from `NotFound(output=-1)` so source code can be audited correctly, while Phase634 callers retain their authentic test against the **numeric** `-1`.

### Original data ownership and why static extraction stops

The constant mapping owner **`0x2259C2F8`** lives in `.data` at RVA `0x59C2F8`, and its first node **`0x2259C32C`** is at RVA `0x59C32C`. The original `.data` section begins at RVA **`0x4D6000`**, declares **`0xCAB24` virtual bytes** but contains only **`0x68000` on-disk raw bytes**. Both owner and first node are beyond that raw size but inside the section's virtual size.

This is not missing or encrypted on-disk mapping data. It is native virtual data that the PE loader zero-initializes and original game initialization subsequently populates. The original binary separately verifies these initializing callsites:

- **`0x223C6A68`: `push 0`; `0x223C6A6A`: `mov ECX,0x2259C32C`; `0x223C6A6F`: `call 0x22073610`**. This source initializer sets up the embedded mapping head, with `0x22073610` initializing sentinel-style fields including source low `9999` and null next pointer for this zero argument.
- **`0x22355AD4`: `mov ECX,0x2259C2F8`; `0x22355AD9`: `call 0x22088F10`**. This original startup/registration path passes the exact mapping-owner global to a method which references `+0x34` and other original mapping-related fields.

The **populated actual range nodes are still not authenticated** by Phase635. They may depend on original game startup, subsequent registration functions or dynamically supplied ranges. Do not read the raw PE section beyond its physical extent, pretend its BSS is a static constant map, or equate synthetic test entries with actual game ranges.

## Proven original caller and initializer edges

The whole-function auditor `research/tools/audit_original_elite_id_mapping_phase635.py` requires the **two exact retail SHA256 DLLs** and checks:

- Both complete original method SHA256 values, six-byte getter instruction shape and static return address.
- Exact Elite primary vtable `+0x760` DWORD target.
- The **two original direct Phase634 converter calls**: `0x220398C3 → 0x22073700` (original flag clearing), `0x2203D2EE → 0x22073700` (original flag `0x21` state test).
- The **two independently observed original mapping initializer edges**: `0x223C6A6F → 0x22073610`, `0x22355AD9 → 0x22088F10`.
- **20 converter instruction witnesses**, **3 caller/context witnesses**, and **3 original initializer witnesses**.
- Exact original `.text` and `.rdata` section identity and proof the owner and first mapping node lie in the **uninitialized virtual tail** of `.data`.

`--self-test` uses entirely invented method bytes, a fabricated six-byte address getter and a fabricated direct call; it must reject every single-byte whole-method mutation, sentinel corruption, wrong call target, and non-retail client/server binaries. Public CI never requires or packages the owner's proprietary DLLs.

## Compiled source references and test scenarios

- `include/sin_compat/original_elite_id_mapping_phase635.hpp`: exact native method/vtable/owner VAs, linked node offsets, `RangeNode`, host callback and error/provenance types.
- `src/original_elite_id_mapping_phase635.cpp`: C++17 source-faithful first-match signed range walker, original `9999` sentinel, `-1` early return, 32-bit wrapping destination, nullable next pointer, plus explicit host guardrails.
- `tests/test_original_elite_id_mapping_phase635.cpp`: first-match, overlapping ranges, exact low/high bounds, sentinel skips, negative source input, source ID `-1` before any memory read, wrapped signed output, a matched conversion that also yields `-1`, empty lists, missing nodes, cycles, a 512-node limit, exceptions, callback refusal and reentrancy.
- The same test builds the **entire previously recovered Phase632→633→634→635 original Elite event chain** as one compiled synthetic-host integration, exercising event `0x47`: original clock/deadline, three independent serial resolutions, virtual `+0x404`, target `+0x4EC` dirty-state write, three flag clears `3/4/5` **mapped through the actual reconstructed 58-byte linked-range algorithm**, original maintenance and source literal event `0x21` **mapped through the same algorithm** before state 4. The alternate event `0x43` gate path reaches the same mapping + state test.
- **Critical caveat:** its tiny range table (`3..5 → 1000000003..1000000005` and `33 → 1000000033`) is **fabricated for testing only**. It is explicitly *not* asserted to reflect actual runtime retail range data. Another test removes the fabricated mapping and demands that the real compiled chain fail closed instead of inventing absent entries.
- Native MSVC Win32 x86 tests, Linux C++17 CTests and SHA/PE identity original-free synthetic audit are included in repository CI. Android compatibility build/smoke runs on its separate existing pipeline, not as an original gameplay claim.

### Owner-only original DLL audit

```powershell
python research\tools\audit_original_elite_id_mapping_phase635.py `
  --server "D:\SiN-Original\server.dll" `
  --client "D:\SiN-Original\client.dll"
```

### Next original-specific research

Trace runtime initialization from **`0x22355AD9 → 0x22088F10`** and node-list setup **`0x223C6A6F → 0x22073610`**, then observe the owner mapping list after original level startup using the existing safe owner runtime trace framework. The original mapping contents may permit fully original-ID grounded flag behavior without fabricated test tables. Any runtime probe must obey the owner's disposable-copy requirement and should never patch the loaded original DLL.

**Proof boundary:** these builds test independently recovered source semantics and original-free synthetic objects only. No original game engine has been shown invoking our C++ replacement; no Android/Quest 3 original-level gameplay is claimed.
