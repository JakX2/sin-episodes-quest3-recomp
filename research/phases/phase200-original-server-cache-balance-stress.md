# Phase 200: independent high-bit-key and transactional cache-balancing stress

13 November 2026. Stacked on the **canonical Phase199 draft PR #252** after concurrently generated PR #253 was closed as a duplicate. This phase is **independent regression coverage**, not a new original DLL function recovery or a second implementation of the fixup.

## Source grounding and scope

Previous actual original SiN 2006 `server.dll` research has SHA-pinned the two 1,387-byte balancing functions (Phase197), four rotation methods (Phase198), 24-byte indexed nodes (Phase191), unsigned first-DWORD comparator (Phase192), and 16-bit indexed parent/color/root layout. Phase199 PR #252 composes those observed structures into a conventional red-black insertion fixup.

**The Phase199 candidate still has not been proven branch-for-branch equivalent to the original x86 red-parent loop.** Tests can demonstrate valid red-black behavior for synthetic data but cannot substitute for original-engine runtime traces. This phase therefore claims **zero newly verified native functions and zero newly verified original bytes**. Unique reviewed inventory remains **195 (103 server, 92 client)**.

## Additional independent tests

`tests/test_original_server_cache_balance_stress.cpp` tests the canonical Phase199 `original_server_cache_balance_composition.hpp` through the existing Phase196 link callback, using the real reconstructed Phase198 left/right rotations.

For **both** scene and model-sounds configurations:

- Exact unsigned 32-bit key extremes: `0`, `0x7FFFFFFF`, `0x80000000`, and `0xFFFFFFFF`, plus additional keys around both sides of bit31.
- **20 deterministically shuffled 64-node insertion sequences per configuration** (40 independent 64-node sequences total), checking invariants **after each insertion** rather than only at the end.
- For each intermediate tree: root black, child-parent reciprocal links, strict unsigned BST ordering, equality of root-to-sentinel black heights, no consecutive red nodes, every inserted node reachable exactly once, and unchanged original-layout key, payload and eight opaque record bytes.
- Alternating raw index values `i` and `i+0x10000`, verifying the original low-16 indexed slot behavior through the Phase196 link and Phase199 fixup.
- A deliberately corrupted synthetic parent chain that **recolors one level first** and then discovers an invalid grandparent during upward propagation. The host reference must throw without mutating the original caller's tree. This checks exception rollback after nontrivial local writes rather than only before the first edit.

The test's 64-key generators use a bijective odd 32-bit multiplier, so inserted keys remain distinct even when high bits wrap around. It deliberately accepts the unsigned key zero, which was missing from the original Phase199 six-key permutation fixture.

**Local test:** optimized GCC and Clang `-std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -UNDEBUG` builds passed with a locally matching version of the Phase199 fixup and the previously reviewed link/rotation headers. The hosted repository tests will compile this source against **the canonical PR #252 header**, which must pass separately before claiming integrated success.

CMake CTest targets are registered in the root reference build and the real Windows x86/MSVC factory-proxy CI build. No original game DLLs, assets or proprietary native code bodies are distributed.

## Concurrent-work resolution

A second Phase199 draft PR #253 was briefly opened concurrently with the hourly worker's PR #252. To prevent duplication, **#253 was closed unmerged** and the additional test coverage was moved to this separate stacked branch based on canonical PR #252. The existing Phase199 implementation remains untouched; no worker's files were force-pushed, overwritten, or merged.

## Next milestone

Obtain an owner-only **actual Windows retail engine boot** through the reversible original-DLL factory forwarding proxy, and examine the red-parent x86 branch-by-branch against the candidate fixup. The exact original filesystem interface method at `+0xD4` still needs key-source verification. No test here establishes a playable original Windows replacement game module, Android port, or Quest 3 build. Windows PC first, Android second, Quest 3 third.
