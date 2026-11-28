# Phase 1692: original CGameStringPool owned behavior

## Scope and baseline

Phase1691 PR #1700 exact head `a152cc0106862fd29981e711414c0e64d6b36246` was verified by 321/321 Windows CTests and all four applicable workflows. It is still an owner-only startup diagnostic, not retail game boot. Phase1692 does not extend empty factory negotiation. It reconstructs a concrete non-rendering game service, the first registered original client game system.

The exact owner original `client.dll` has SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`. Read-only disassembly confirms original VClient011.Init at 0x240FE3F6 calls getter 0x2413D240, then at 0x240FE3FC registers its return via 0x24156840. Getter returns 0x2436212C. Existing `client_original_gamesystem_sequence.h` identifies this object as CGameStringPool from its original RTTI and 12-entry vtable. Its original Init (0x240566C0) returns true without resetting existing strings. The previous independent StringPoolSystem adapter modeled only that return.

## Source behavior

| Original client VA | Observed behavior |
|---|---|
| 0x2413D2D0 | Constructs inner tree at this+4, installs original vtable 0x2429AD94 |
| 0x241C6FB0 | Stores comparator 0x241C5EB0, initial 256 nodes, 32-node growth, allocates 0xC00 bytes (12 per node), uint16 root/free sentinels 0xFFFF |
| 0x2413D250 | C-string allocation wrapper; returns null for empty input, otherwise calls the owned intern helper |
| 0x2413D270 | Calls lookup-only helper without inserting |
| 0x241C71D0 | Searches tree; if key exists returns the existing node's stored pointer, else duplicates the C-string via 0x24261CAE and inserts it through 0x241C7170 |
| 0x241C7010 | Searches the same tree; returns stored pointer or null for a missing node |
| 0x241C5EB0 | Compares two pointed-to C-strings through original CRT 0x2425F81B |
| 0x2425F81B / 0x24261C60 | Default CRT comparison folds ASCII A-Z; alternate original locale-sensitive branch exists |
| 0x2413D2F0 | Original game-system vtable slot5: adjusts this+4 and tail-jumps to string/tree clear |
| 0x241C7050 | Iterates owned entries, frees each string, resets the tree via 0x241C6730 |
| 0x241C6730 | Rebuilds the free chain, resets active count/root, retains tree allocation |
| 0x2413D300 / 0x241C7110 | Base unregister, clear strings, free owned backing tree storage and optional delete |

Source callers demonstrate this is not an unused initializer. Direct call at 0x24097CF5 stores the returned pool pointer into a temporary record subsequently supplied to helper 0x240970A0. Calls 0x241BF934 and 0x241BF94A write interned pointers to a caller-owned field +0x30; 0x241BF967 writes another to +0x2C. Other direct allocation calls include 0x2418EF9C, 0x2418F2E4, 0x24191754, 0x24193833, 0x24194DDF, 0x2419A892, 0x2419A9D7, 0x2419ABE6, 0x2419BBFB, 0x2419BC28, 0x241AC767. The higher-level caller types and semantics remain separate recovery work.

The original-only audit `research/tools/audit_sin_original_client_game_string_pool.py` pins the entire client SHA256, all 12 vtable pointers and their whole 48-byte table hash, 21 complete method-body digests, 18 direct native CALL/JMP edges, empty allocation wrapper branch, 256/32/12 tree parameters and 16-bit sentinel witnesses. The actual original locally matched these hashes and call/branch witnesses. Public CI self-test checks invented digest and call/jump mutation cases, never runs or distributes the original executable bytes.

## Owned reconstruction and integration

New `include/sin_compat/original_client_game_string_pool.hpp` implements a C++17 owning pool with independent stable node storage, deterministic source-default ASCII case-insensitive comparison, first-spelling preservation, lookup-only misses, null for the empty string, independently copied strings and no pointer invalidation during insertion. The existing native `pc/independent_slot0/client_known_constructed_gamesystem_init.h` StringPoolSystem adapter now owns that actual pool. Its source Init remains trivial true without clearing existing data. The adapter exposes explicit `ClearOnOriginalSlot5` for the recovered clear callback, and the same existing independent registry test verifies a pooled string survives Init and is removed by the clear.

**Implementation decisions / limitations:** this is observable behavior rather than a byte-identical native CUtlRBTree recreation. The independently owned std::set releases nodes on clear rather than retaining the original free-list storage. It implements the original default C-locale comparator, not the original locale-flag-dependent alternative. The independent wrapper deliberately refuses null input, embedded NUL views, allocation failure and additional distinct keys beyond the 0xFFFF usable-index boundary. These are conservative safety deviations, not claims about the original overflow/crash behavior. A duplicate remains available even at full capacity. No stale pointer may be dereferenced after clear. The pool is noncopyable/nonmovable so owned string pointer lifetime cannot silently migrate. The current native registry adapter is not the original PE32 object layout and has not been invoked by an owner retail game engine or integrated into the production VClient011 full game-system Init path.

## Tests and verification

The exact committed portable header and test file were built locally using GCC C++17 `-Wall -Wextra -Wpedantic -Werror` and passed. Git blob hashes for the tested files matched the repository (`26a8a068d629186ad6f9fe3a1f238ab9a358799d` and `51d5b9dc7350bbb1a962dbd6a33ab4e5ce2abaf6`). The same regression passed AddressSanitizer, UndefinedBehaviorSanitizer, and leak detection. The test fills 65,535 unique strings, tests source-default comparison and non-ASCII byte distinction, stable first-string addresses across insertions, overflow refusal, duplicate at capacity, non-mutating lookup, clear/reuse and separate owner lifetime.

The reference CMake and independent MSVC x86 CMake both build this exact test. The existing native constructed-game-system test is strengthened, and a separate source audit self-test is registered. Both Android scripts explicitly compile the actual regression and check target ELF machine; the Android emulator workflow pushes/runs that same test on Bionic and retains its exact PASS evidence, not merely an unchanged unrelated Android smoke run. Exact-head CI is pending when this note is written. No Quest headset execution or commercial game boot is claimed.

Next: connect recovered pool to a source-authenticated actual client gameplay caller or complete the client game-system registry lifecycle integration. Do not invent a successful full VClient011.Init by bypassing unresolved original systems. No user-excluded Source/HL2-equivalent physics, surfaces, props/models or renderer changes.
