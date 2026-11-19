# Original client C_StatsSystem storage reconstruction (Win32 x86)

The owner-original SiN Episodes 2006 client.dll SHA256 is
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Direct original disassembly and complete-function SHA256 verification
pin the first two C_StatsSystem Init helper bodies:

| Original VA | Bytes | Complete SHA256 | Meaning |
|---|---:|---|---|
| `0x241FF420` | 171 | `a4d63f60e7bc440135df208c7bf1c5cf7aabe9f5c9d9ab08c855e0937d40d696` | Replaces 55 static entries |
| `0x241FF8E0` | 166 | `714148c283d7bb4f41c5501f11e24185444342f957574c8dc6fb28925e67130e` | Replaces 11 dynamic entries |
| `0x24206430` | 89 | `aac276edaceb675ad8a9cc12dd0a014518e396d7f275c21df71f934d92b7c9fc` | Stat-definition dispatcher |

Both original functions take an internal CUtlVector-like pointer
container (data pointer at offset 0, capacity +4, count +12, an
aliased backing pointer +16). They free/reset old contents and append
a pointer for each index, even when a single allocation returns null.

**Static records:** 55 independently allocated 8-byte records,
field 0 receives the result of native 0x24206430(index, empty string,
-1), field +4 starts null. Its underlying 0x24205BC0 type switch
supports cases 0..15; higher indices return null, but native still
calls the dispatcher for all 55 successful record allocations.

**Dynamic records:** 11 separately allocated 20-byte records with
all five 32-bit fields initialized to zero.

`client_stats_storage_init.h` owns these entries with stable
`unique_ptr` storage and array-indexed pointer slots, preserving
cardinality, initialization, per-slot allocation refusal and repeat
reset behavior. It hooks the first two *known* steps into the existing
seven-stage StatsSystem Init ordering; the remaining owner setup,
display-layout loader and three event registrations are still
explicitly injected and fail-closed if unavailable.

This is **not** a byte-for-byte replacement for original CUtlVector,
native type-specific stat-definition constructors, registry or
`C_StatsSystem` itself. It does not invoke original virtual methods,
change the retail engine, or establish successful full VClient011 Init.
The owner-verified Windows checkpoint remains 0x53494E63.
