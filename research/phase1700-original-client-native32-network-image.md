# Phase 1700: source-backed 32-bit receive-table memory image, not a game boot

## Actual owner Windows state

The last genuine original SiN Episodes Windows engine session loaded the independently reconstructed PE32 client DLL, obtained the 21 original source app interfaces, connected the sound emitter, registered client prediction and accepted the independent prediction, entity list, centerprint and leaf factories. Original engine then invoked `VClient011.slot5` and **deliberately terminated with 0x53494E45**: the independent DLL cannot yet legitimately supply a fully executable 2006 `ClientClass*` and `RecvTable*` registry. Retail `client.dll` was restored and retail `server.dll` remained unchanged.

This phase **does not** disable that safety stop, replace the user's Windows game DLLs, or claim playable startup. It supplies the next independent engineering prerequisite: a **bounded 32-bit *simulated memory image*** that replays the original engine's verified receive-table traversal rules in owned host memory.

## Source-authenticated inputs carried into the image

Original client source binary read-only SHA256:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdac0f8728e1841d`.

Research phases 1696–1699 recovered:

| Original source object | Total | Source evidence |
| --- | ---: | --- |
| Registered `ClientClass` nodes in real head-to-tail order | **191** | Original static constructors, 1,582 CRT entries, full linked chain |
| Original statically constructed `RecvTable` objects | **211** | 214 original constructor callsites, 191 registered classes +20 auxiliary |
| Native original 60-byte static `RecvProp` descriptors | **1,110** | All original table property counts and all original six numeric type tags |
| Original static nested type-5 edges | **192** | Original table pointers at native descriptor +0x28 and actual source callers |
| Original dynamically constructed generic nested tables | **14** | Eleven registered-class arrays, three auxiliary arrays, 2006 generic `0x24126E90` native builder |
| Generated generic child descriptor elements | **691** | 579 registered-class +112 auxiliary, actual original `000`...`255` source name table |
| Special animation overlay | **1** | Original independent 621-byte method `0x241271B0`, two specialized native callbacks not yet emulated |
| Total original type-5 outer properties | **207** | **192 +14 +1** exact and disjoint, no unclassified native type-5 descriptor |

Original native static type tags are **414 type0 integer-like, 300 type1 float, 164 type2 vector3, 12 type3 string, 13 type4 array, 207 type5 nested**: total **1,110**. The generated children have separately source-proven numeric element types 0 or 1. This is native metadata, not fully known packet serialization, quantization or actual proxy ownership.

The compact source metadata `include/sin_compat/original_client_native32_recvtable_manifest.hpp` retains **all 211 exact original static table property counts** (211 source integer bytes) and **all 1,110 actual original numeric RecvProp type DWORD tags**, densely represented as 1,110 hex nibbles (555 packed bytes). The source property count bytes SHA256 is `ee7010aab35e8e31381069f56e19113de7e8bad292c902137ef999b17390a204`; original unpacked native type-byte stream SHA256 `4db1788356cca573eb6a21250cedfe785b700be738434dd08adf50f0acad2ec0`; packed nibble-byte stream SHA256 `b0feb09024519c08d0c6d95c3e0ff73ed9bad4bc821798241b3a93f6bd1dec4b`. The 14 actual generic dynamic parent properties are encoded as compact `(parentSourceTableOrdinal,parentPropertySlot,sourceArrayShapeOrdinal)` triples.

The C++17 **compile-time** assertions require exact total table counts, original type histogram and **207/207 native type-5 source routing**, with every original native type-5 descriptor marked *exactly once*: 192 real original static links from `original_client_static_recvtable_links.hpp`, 14 genuine generic source arrays, and one recognized special animation overlay slot. Any source-map mismatch rejects compilation rather than inventing a networking behavior.

## Owned 32-bit address-space replay

`include/sin_compat/original_client_native32_recvtable_image.hpp` constructs a **private host memory vector**, addressed by integer tokens starting at fake address `0x10000000`. These *are not executable or dereferenceable Windows process addresses*. All access goes through bounds-checked little-endian `Arena::Read8/Write8/Read32/Write32` and is limited to allocated host bytes.

Within that sandbox the image allocates:

- **191** original source-order linked class **20-byte prefixes**, with pointers to real source class names, the corresponding owned table tokens, and the next-class token at prefix **+0x10**. Both native creator/callback slots at +0/+4 remain **null** because the original entity and temp-event factory functions have not been independently made safe.
- **211** owned 20-byte table headers, with correct original native count at header **+4**, property-array token **+0**, original constructor null auxiliary **+8**, and visited bytes +0x10/+0x11 cleared. Where table names or opaque remaining bytes are not proven, **no fabricated usable string pointer is published**.
- **1,110** 60-byte static descriptor slots with actual original numeric native type DWORD at descriptor **+4**. Original flags/proxies/offsets/names for every static slot are *not* fabricated: only proven source fields and pointers are written.
- **192** actual original static child RecvTable pointers at descriptor **+0x28**, referencing valid owned child-table tokens and exactly matching the original source graph.
- **14** generated child-table headers with **691** host-owned native 60-byte child descriptors, original three-digit indexed names `000` through each array's final index, source numeric type0/1 and *child-relative* byte member offset **+0x2C** starting **0, stride, 2×stride**. The type-5 **outer** descriptor carries the proven original x86 parent-object base at its own byte +0x2C. Do not mix the two.
- The one **special animation-overlay** source native type-5 descriptor is deliberately left with **null child RecvTable** because the specialized callback semantics and allocation ownership are not fully reconstructed. It is explicitly counted as blocked; the image never claims complete engine-compatible recursion.

The image now has **two deliberately distinct traversal entrypoints**, both reproducing only authenticated original engine recursion fields (table count, descriptor stride 60, native type-5 child at descriptor +0x28, visited byte +0x11):

- **`Image::WalkFromClientClassRoots()`** matches the *real original engine startup* entrypoint `0x20023BB0`: traverse the actual 191 `VClient011.slot5` class nodes in original linked-list order, check each class +0x0C table pointer, then recursively follow those table and dynamic child pointers. The source graph reaches **210 static tables and 14 generated child tables**, or **224 table objects**, with **1,796 property descriptors**. The one auxiliary table not directly or transitively reachable by this source path is **`DT_Animationlayer`** (native static ordinal 196, containing five descriptors with type sequence `0,1,1,1,0`). This is an existence-vs-reachability distinction, *not proof that no other original game subsystem can access that table*.
- **`Image::WalkAllStaticTables()`** is an *exhaustive research inventory* traversal, starting from all 211 native static tables rather than the engine's 191 class roots. It visits **225 table objects** (211+14) and **1,801 descriptors** (1,110+691). It is **not** the real original engine's root selection.

Both traversals observe the same **207 original native type-5 outer properties**, report exactly **one intentionally blocked special overlay**, and can never return an engine-compatible pointer. The test deliberately mutates an otherwise unreachable `DT_Animationlayer` descriptor: the class-root path remains 224/1796, while the exhaustive path refuses the corrupt descriptor. This establishes that the two traversals genuinely differ, rather than reporting two arbitrary hardcoded totals.

Neither path invokes the original engine's additional `0x20086930` table-registration helper, actual 2006 native RecvProxy callbacks, game entity/temp-event creators, allocator cleanup or true Windows runtime calls. A successful scan is insufficient to expose a live `VClient011.slot5` pointer.

**Permanent runtime safety guard for this stage:** `CanExposeToRetailEngine()` ALWAYS returns `false`. A successful host graph traversal is only an ABI-layout *preflight*, not a playable SiN client.

## Tests and reproduction

`tests/test_original_client_native32_recvtable_image.cpp` uses the real authenticated 191-class ordered source manifest and all exact native table/descriptor metadata; checks every class linked node and name, all 211 owned native header records, all 1,110 native property numeric type tags, all 192 original static child refs, and all 14 dynamic child subtables with 691 native relative offsets and exact numeric property names. It additionally verifies the **real 191-root startup count 224/1796** versus the **full 211-root inventory 225/1801**, the original auxiliary table that is not reached through the class-root graph, class-linked pointer mutations, and incorrect native descriptor rejection. It verifies the special overlay remains blocked, error handling on invalid native token/type/count and the persistent closed retail-engine guard. Unlike a synthetic equal-cardinality graph, it validates **actual original source type/parent-child topology in simulated owned memory**.

Independent MSVC Win32 x86 tests, Linux strict C++17 + ASan/UBSan, Android ARM64 cross-compilation and x86_64 Bionic emulator test with exact pass marker and archived output are wired. They use NO original proprietary executable bytes or engine gameplay DLLs. Only public recovered numeric metadata and manually authored C++ source are involved.

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -UNDEBUG -Iinclude tests/test_original_client_native32_recvtable_image.cpp -o /tmp/sin-native32-owned-image
/tmp/sin-native32-owned-image research/manifests/original_client_class_catalog_phase1696.tsv
```

## Next real work

To *actually* allow original Windows engine `VClient011.slot5` to return, reconstruct the complete source-correct 60-byte property values, 20-byte table storage and mutability, source receive proxy callback signatures, native client class entity/temp-event creators and allocation lifetime. In particular, recover the special `DT_OverlayVars` child descriptors and true overlay callbacks and validate the original engine's additional receive-table helper behavior. Before any owner Windows diagnostic test, run independent authentic PE32 ABI/proxy integration tests and retain the existing disposable-copy SHA-pinned originals/restore guard.

This is strictly **network-client reconstruction**, not user-excluded physics, surfaces, props/models or renderer work. There is NO new retail game launch, map progression, Android APK game runtime or Quest VR frame.
