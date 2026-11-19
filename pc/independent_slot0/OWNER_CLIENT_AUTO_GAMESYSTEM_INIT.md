# Original 2006 post-MatSystemSurface automatic game-system initialization

Private source: original 2006 SiN Episodes: Emergence client.dll SHA256
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.
Inspected read-only from the owner's Drive bin.zip. No original code copied.

The previous owner runtime checkpoint (0x53494E61) establishes real
MatSystemSurface003 acquisition. Original VClient011::Init next
registers several application/game-system objects by calls to helper
0x24156840. After numerous registrations it calls mandatory helper
0x24156860 at Init callsite 0x240FE4B3; a false result branches to
the shared client Init failure exit 0x240FE59E.

Original complete native functions and digests:

| VA | Size | SHA256 |
| --- | ---: | --- |
| 0x24156840 | 23 | e48841e7e14a9565ac425416c3fc3230615c8246fa603be65d7149500396292e |
| 0x24156860 | 277 | b4f5ca8aefcb1248cdd7a62edf042a3f7fd5a04ac782fde277d13f335067ccb7 |
| 0x24156980 | 70 | 5e6a0f107b7cf1f04f3331c1ca790683a42b104fd276dbc6e522c2b69483206b |

Detailed original control flow:
- Pending linked list head at original 0x24366C8C, constructor node
  points to successor at offset +4 (first virtual vptr at +0).
- Started flag byte 0x24366C84 distinguishes pending head insertion
  from post-init direct vector insertion.
- Original direct register 0x24156840 inserts into a vector with data
  0x242FBFA8 and size 0x242FBFB4; no immediate duplicate filter.
- 0x24156860 merges pending list in LIFO constructor order into the
  direct vector. It compares each pending object with all registered
  pointers and **skips duplicates** with native diagnostic:
  "AutoGameSystem already added to game system list!!!".
- It clears the pending head, sets the started flag **before** invoking
  Init on all registered systems.
- Each object is called via virtual slot zero, bool result checked;
  the **first false result** aborts immediately. Native loop checks
  live vector count on each iteration, so a callback can append and
  have the newly registered object initialized on the same pass.
- A failed object does not visibly trigger rollback in this helper.

The new independent host model and native MSVC x86 test preserve
constructor LIFO, direct registration, duplicate skip, flag ordering,
first-failure behavior and live-count iteration. It deliberately
refuses invalid null pointers and malformed cycles that would cause
unsafe original pointer behavior. No retail object pointers, unknown
native singleton constructors, or GUI root panels are fabricated.

This recovers another **mandatory original VClient011 Init gate** beyond
the factory and VGUI checks, but this helper has NOT yet run inside
the actual 2006 game process with our independent client. No full
Init return or playable game is claimed.

## Exact twelve original direct registration sources

Original VClient011 Init calls 0x24156840 twelve times before its
mandatory game-system Init gate. The owner-only audit verifies all
twelve exact native CALL rel32 instructions; the portable header
exposes original callsites and pointer provenance without inventing
gameplay class names. Ordered native registration source values:

1. Return of 0x2413D240
2. Return of 0x241BCAA0
3. Nonnull [0x242FA79C] +4, else null
4. Return of 0x24123CB0
5. Return of 0x241ED2F0
6. Return of 0x24105660
7. Pointer [0x242FA890]
8. Address 0x24378DE8
9. Address 0x24378D18
10. Address 0x242FA6B8
11. Return of 0x240F23A0
12. Return of 0x24181FC0

The next implementation target is the concrete constructors and
their original virtual Init methods. This source list is metadata,
not a live game-system implementation.

## Six native static object getters, one lazy initializer

Direct inspection of the SHA-authenticated original binary confirms
six of the twelve direct-registration sources are short six-byte
constant-pointer getters, with no other side effects in those helpers:

| Getter VA | Returned original object VA | Whole-function SHA256 |
| --- | --- | --- |
| 0x2413D240 | 0x2436212C | 476c508b6290a607c69eb45a804d6a3321986ec976bb899174e13dd4c9bf4be9 |
| 0x241BCAA0 | 0x242FFC1C | 47eb8a989c87817033ced6033cd041b6be91946dc2196cbe63f8578e98675762 |
| 0x24123CB0 | 0x243589A8 | 746aa02a3ce17e03d0b9cb08aad13a7d8baf1554dd3c9611ae506b5fd3151798 |
| 0x241ED2F0 | 0x2431DD84 | cc2d6553a8a626bd33b95745ab14cd90d24ace839df34df9dbb174e4f3a09dfc |
| 0x240F23A0 | 0x2433E4F8 | 383c1684e871fd4f861f3b2c4f221fd6c9a5b068a3e5c7183b7045a22d445073 |
| 0x24181FC0 | 0x242FC9DC | 321038b118e32adb91abf36814bdfe20ce0cfbde70a28a2ff6caa304e1868e5c |

A seventh source, original `0x24105660`, is **not** a pure
getter: first call sets a bit at `0x2433FDB4`, initializes
the object at `0x2433FD9C`, registers shutdown, then returns that
object's address. Its complete 75-byte native region has SHA256
`e7a2d2c4016bc3394dde6bfc556f312f311ca1072ae210fb0038464d83fe1c54`.

These identities do not recover those objects' virtual Init bodies.
The portable `kOriginalDirectStaticGetters` and
`LazySingletonReturn` record this distinction so a future startup
implementation does not silently skip singleton initialization.
