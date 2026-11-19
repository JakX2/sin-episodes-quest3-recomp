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

## All sixteen native stat definition type identities

Using the original 2006 switch table at `0x24205DF0`, all sixteen
case entry instructions, allocation sizes, native constructor calls,
final vptr assignments, MSVC CompleteObjectLocators and
RTTI TypeDescriptors are now authenticated separately. Source code
`client_stats_definition_types.h` exposes metadata **without copying
or calling the original constructors**.

| Index | Original class | Original allocation |
|---:|---|---:|
| 0 | CIntStat | 16 |
| 1 | CFloatStat | 16 |
| 2 | CIntOutOfStat | 20 |
| 3 | CFloatOutOfStat | 20 |
| 4 | CPercentageStat | 20 |
| 5 | CAccuracyStat | 20 |
| 6 | CAverageStat | 20 |
| 7 | CTimeStat | 20 |
| 8 | CPerTimeStat | 28 |
| 9 | CPerTimeStat | 28 |
| 10 | CPerTimeStat | 28 |
| 11 | CAveragePerTimeStat | 32 |
| 12 | CMaxStat | 16 |
| 13 | CMinStat | 16 |
| 14 | CMaxAccumulatorStat | 20 |
| 15 | CMinAccumulatorStat | 20 |

Cases 8–10 share one CPerTimeStat vtable but pass distinct
floating-point initialization bit patterns `0x3F800000`,
`0x3C888889`, and `0x3991A2B4`.
Case 11 initializes CAveragePerTimeStat with `0x3F800000`.
These are 16 dispatch entries but **14 distinct classes**.

The owner-only `audit_sin_client_stats_definition_types.py`
verifies the full dispatch SHA, every table entry and constructor
allocation, exact MSVC RTTI class identity, vptr assignment and
rate parameters. Original-free CI mutations reject mismatched
metadata. This inventory reduces constructor unknowns but DOES NOT
equate to 16 reconstructed type-specific method implementations.
