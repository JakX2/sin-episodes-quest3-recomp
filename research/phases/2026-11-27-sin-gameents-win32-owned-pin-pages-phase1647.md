# Phase 1647: Real Win32-owned virtual backing and exclusive lifetime pin

**Stack:** Phase 1646 PR #1655, head 353ea2f7216b2e8ccc6bf2c2d5caf693956a0d8a, with final native Windows 245/245 CTest, C++ reference, Android ARM64, Bionic emulator, factory compatibility and original PE32 authentication green.

## Why this change

Phases 1642-1646 independently reconstructed the original 2006 engine's 0x14-byte edict selection/restamp, generation reservation, reciprocal CBaseEntity/IServerUnknown network-property links and epoch-scoped authorization. Their host fixtures used stack-backed memory and boolean pin indicators. Such fixtures proved the *shape* of the protocol, but did not prove that a host could actually prevent a release/reallocation during the entire sequence.

This phase introduces an actual, owned Windows PE32 allocation lifetime primitive. It is still **not** a pin on retail engine.dll memory.

## Native backing and exclusive pin

New `Win32OwnedGameEntsPinPages` allocates three distinct, committed, private Win32 virtual-memory regions through `VirtualAlloc(MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)`. The logical extents are the original 2006 PE32 6 x 0x14-byte edict record table, 0x180-byte entity fixture, and 0x90-byte independently owned touch fixture. `VirtualQuery` confirms committed private address regions.

`Begin` acquires an exclusive `std::recursive_mutex` lock and retains it across the whole callback lifetime. `Validate` checks current thread, token, exact base pointers/extents and live pin under that lock; `End` releases precisely the matching token/thread lease. Calls from another thread fail non-blockingly through `try_lock`. Host methods `TryRelease` (which uses `VirtualFree(MEM_RELEASE)`) and `TryWrite` cannot mutate or free backing during a pin, whether called recursively from the callback thread or from a competitor. Bad-token end does not release a live pin. Owner lifetime must outlive all pins.

This is an allocation **lifetime** pin, not `VirtualLock`: the Windows memory manager may page committed memory out of RAM, but the committed virtual address and ownership cannot be freed or swapped by the cooperative host while its exclusive pin is held. This guarantee does not apply to arbitrary external code writing those addresses, or to the original retail allocator.

## End-to-end native integration

A new C++17 MSVC x86 test `test_server_gameents_win32_owned_pin_integration.cpp` uses that real backing for the existing independent adapters:

1. Verify all three actual `VirtualAlloc` pages are committed and distinguishable, `TryRelease` and `TryWrite` are refused while pinned, and invalid token/extent writes fail.
2. Feed the original-reconstructed edict selector and reuse adapter a host snapshot of the VirtualAlloc table. The authenticated free callback is invoked twice, while both nested and competing-thread `VirtualFree`/write attempts fail. An independent host-issued epoch is reserved and atomically committed into the Phase 1644 owner lifetime registry, *before* unpin.
3. Populate the three independently authenticated original PE32 link fields inside an owner pin: edict+0x0C = CBaseEntity, entity+0x168 = outer CBaseEntity, entity+0x16C = edict. These are native Win32 pointer values, not x64 fixture tokens.
4. Reacquire simultaneous edict/entity/touch pin, run the Phase 1645 reciprocal link attacher, register the live entity, and check Phase 1646 `ValidUnknownPairAtEpoch`, `ValidPairAtEpoch` and `ValidTouchAtEpoch` under the registry lease. Test that release still cannot occur until the separate entity retirement and edict forgetting have been reconciled.
5. Verify the actual `VirtualFree` path succeeds after registry reconciliation and that a fresh separate VirtualAlloc host with a denied count/epoch commit quarantines the independent allocator.

The CMake target registers one additional native Windows CTest case. **Expected native test count: 246** if final-head CI compiles and executes successfully. No commercial SiN game binaries, source bytes, injected hooks or original game processes are required.

## Limits

This is concrete ownership proof for **our own synthetic Win32 host allocations**. It does not show that the unmodified retail SiN engine uses this host, allows a pin across its callbacks, retains the entity and touch backing, or coordinates its edict array resizing through this mutex. Original source-specific global fields and constructor links remain research evidence, not an installed engine allocator. Retail `ServerGameEnts001` is unregistered. Native playable Windows startup and Android/Quest 3 gameplay remain unverified.

The user previously overrode generic HL2-equivalent physics, surface, prop and model work. Those areas are intentionally not revisited here.
