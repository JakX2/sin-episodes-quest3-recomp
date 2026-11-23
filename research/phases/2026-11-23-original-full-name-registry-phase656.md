# Phase 656: Full original SiN string registry chain (integration milestone)

## Goal and proof boundary

This phase adds **no newly authenticated original x86 function bodies**.
Instead, it closes a major *integration gap* in the previous original
`server.dll` function reconstructions: replacing the synthetic tree
parent search, free-slot acquisition, red-black link/rebalance and
one-sided name comparator in the Phase651 compiled test with **all their
actual separately reconstructed C++ models** from Phases 648 through 655.

The source original remains the owner-provided 2006 retail
`server.dll`, SHA256
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`,
and `client.dll`, SHA256
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
Their proprietary machine code is **not** committed to the repository.

No reimplementation of already-functioning Half-Life 2 physics, material
or surface properties, props, model loading or rendering is involved.

## Compiled C++ graph

The new regression file
`tests/test_original_full_name_registry_chain_phase656.cpp`
runs these original-method **derived source models together**, without
replacing original game behavior with synthetic callback success:

| Stage | Recovered implementation | Purpose |
|---|---|---|
| Phase648 | `0x2228E9B0` nested string reference comparator | Resolve/compare inline block-offset or fallback name refs |
| Phase649 | `0x223A5574` delegated string casefold/mode | Preserve original case-insensitive ordering |
| Phase650 | `0x2228EB40`, `0x2228EA30`, `0x2228F7A0` | Tree search, first-fit string-block selection, lookup-or-create |
| Phase651 | `0x2228E280`, `0x2228ED80`, `0x2228F710` | DWORD block-array growth/insert, tree insert |
| Phase652 | `0x2228E470`, `0x2228E620`, `0x2228F460` | Parent search, node acquisition and linking |
| Phase653 | `0x2228E100`, `0x2228E200` | Node resolution and 12-byte slot growth |
| Phase654 | `0x2228E710`, `0x2228E800`, `0x2228EDF0` | Paired rotations and red-black recoloring |
| Phase655 | `0x2228EC50`, `0x2228F400`, `0x2228F3C0` | Free-list reset, owned name-block teardown, non-allocating view lookup |

All of these components are compiled from the existing
`src/original_*.cpp` implementations, not textually duplicated. The
typed guest-host only supplies **guest memory**, original malloc/realloc/
free dispatch side effects, overlapping guest-memory move semantics and
external locale/engine dependencies. These engine callbacks remain
host-supplied, not reverse-engineered to a complete memory manager.

The earlier Phase651 test cases run with the now-recovered actual
Phase652-654 insertion path. The new additional case begins with a
zero-capacity map, inserts eleven distinct **actual terminated C-string
names** and forces original slot capacity progression
`0 → 3 → 6 → 12`, including two synthetic guest-array relocations.
It builds its own block-pointer list using original Phase651 growth and
string blocks using Phase650, with original Phase648/649 casefolding.

## What it verifies

1. After **every** insert, receives the exact newly allocated 16-bit
   node handle and advances active count.
2. After all eleven inserts, traverses the relocated tree, checking
   every parent backlink, sorted case-insensitive name order, no red
   node with a red child, equal black heights, black root, and every
   allocated slot visited exactly once.
3. Looks up `aPpLe` with the Phase655 `0x2228F3C0` wrapper, which
   calls the **actual compiled Phase650 two-direction search** and
   Phase648/649 comparator. It must find existing `Apple` index 1
   without another allocation or insertion.
4. Invokes the original Phase655 release model, verifies that the
   string block is passed to the recovered allocator free-wrapper,
   the node array and block pointer list remain allocated, and all
   eleven node slots form the original ascending free list.
5. Inserts `Reborn` into the cleared registry, **reusing slot 0**
   without growing or reallocating 12-byte node storage. The new
   root is black and the next-unused slot count remains eleven.
6. Verifies that a previously registered string (`MANGO`) no longer
   resolves, even though it existed before the reset.

The test retains the earlier Phase651 duplicate-search and block
growth/relocation cases and error-path tests, now using the recovered
insertion path rather than artificial link callbacks.

## Additional stress and sanitizer coverage

The Phase656 integration was extended to **32 independently shuffled but
deterministic nine-name insertion orders**, using the real Phase648/649
case-insensitive comparator and the compiled Phase650-655 C++ models.
After **each inserted name**, the test traverses the complete red-black
tree, checks parent backlinks, red/black correctness, equal black height,
and verifies all currently allocated indices remain reachable after
`0→3→6→12` slot allocation and relocation. Each order then tests
case-insensitive `aLpHa` lookup, release of all name blocks,
9-node free-list generation, and a fresh insertion that reuses node 0
without new slot-array reallocation.

A dedicated
`.github/workflows/sin-phase656-string-registry-sanitizers.yml`
compiles all eight source-reference libraries and the complete test
using **Clang C++17 AddressSanitizer + UndefinedBehaviorSanitizer** on
Linux, without any original retail executable bytes. This is independent
of the main GCC/Windows CTest matrix.

## Build and limitations

The CMake test
`sin_original_full_name_registry_chain_phase656_tests` explicitly
links all required Phase648-655 source libraries. A native Win32
MSVC x86 test target links the matching
`sin_pc_original_*` libraries. Cross-platform CI evidence is added
to the PR when an exact commit completes.

Passing this is **not** proof of binary-equivalence for arbitrary guest
states, Windows retail `CreateInterface` loading, real engine allocator
ABI, actual map/entity spawn, Android playable levels, or Quest 3 VR.
The synthetic guest has correct typed runtime callback contracts for
the measured cases but is not the original engine process.

The immediate next in-game milestone is to bind the derived naming and
entity registry to the exact original Windows module interfaces, then
trace a single original SiN level's entity creation and weapon action
against the retail game. Avoid spending cycles reconstructing HL2
physics or models already proven to function.
