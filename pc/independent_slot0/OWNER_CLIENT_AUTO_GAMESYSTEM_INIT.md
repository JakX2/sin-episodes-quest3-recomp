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
