# Original static client AutoGameSystem identity and slot-zero Init evidence

Read-only analysis of original owner `client.dll`, SHA256
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

After the previously mapped twelve direct registration callsites, four
registered objects have **initialized vptrs in the original raw PE**.
We verified the object vptr, `vtable[-1]` MSVC
`RTTICompleteObjectLocator` (signature/offset/constructor-displacement
all zero), original `TypeDescriptor` class name, and method at slot 0:

| Object VA | RTTI class | Vtable VA | Virtual Init VA | Verified code |
|---|---|---|---|---|
| 0x242FFC1C | CSoundEmitterSystem | 0x242A6BE8 | 0x241BC950 | Complete 11 bytes |
| 0x2431DD84 | CViewportClientSystem | 0x242AC284 | 0x241ED310 | Complete 14 bytes |
| 0x242FC9DC | CPerfVisualBenchmark | 0x242A209C | 0x24182800 | First 32 bytes only |
| 0x242FA6B8 | CClientThinkList | 0x24297300 | 0x24103DF0 | Complete 11 bytes |

The `research/tools/audit_sin_client_static_gamesystem_rtti.py`
auditor checks these claims and their exact method SHA256 digests against
owner-original bytes, without exposing binaries. The original-free
mutation tests detect tampered vtables, RTTI, function entry bytes,
or incorrect original input.

**Three complete method behaviors are now identified:**

- `CSoundEmitterSystem::Init` delegates to an engine-owned sound emitter
  object at original global `0x2433F6E4`, via its virtual slot 1,
  returning that result directly.
- `CViewportClientSystem::Init` invokes the object at original global
  `0x2437F8A0` through virtual slot 7, discards its return, then
  always returns true.
- `CClientThinkList::Init` zeros the DWORD at `this+0x48`, zeros
  the byte at `this+0x4C`, then returns true.
- `CPerfVisualBenchmark::Init` is longer, requires additional
  parsing/resource setup, and is **not reconstructed** from a 32-byte
  prefix hash.

These are genuine RTTI class identities, not guesses based on object
address proximity. Other registered pointers are .bss or runtime
subobject addresses, and their initialized vtables cannot be proven
from raw-file inspection alone. Do not assume that an original static
object can be used inside our separate independently compiled DLL.

This phase still does not complete original initialization or boot.
