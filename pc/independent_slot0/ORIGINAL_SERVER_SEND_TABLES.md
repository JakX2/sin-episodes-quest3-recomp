# Phase 331: original server send-table inventory and initial nested schema

The owner-private SHA-256 authenticated server.dll
(568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650)
was inspected read-only. The SendTable::Construct helper at RVA 0x25E850
writes property pointer +0, count +4, name +8, precalc +12 (zero),
initialized +16 (false) and written +17 (false). SDK 2004 dt_send.h
corroborates this 20-byte x86 layout.

All 164 class-associated table constructors and 20 additional nested or
shared table constructors were identified by the three observed
name-pointer load forms (EAX, ECX, EDX), followed by count, property
storage, receiver and call target. Total: 184 named tables, 1028 directly
declared property slots (858 in the 164 class-associated tables and 170
in 20 additional tables). Counts exclude the first dummy SendProp used
by SDK-style table construction. This is not a count of fully decoded
properties or flattened wire fields. Three tables have zero direct
properties: DT_BaseTempEntity, DT_GameRulesProxy, DT_SE1GameRules.

Inspect-OwnerOriginalSendTables.py authenticates the original hash,
links each registered class to its table storage, checks mapped bounds
and uniqueness, and verifies a canonical inventory digest. The
equivalent extraction and bounds checks ran against the actual owner
binary locally. The public parser regression uses invented records.

## First concrete nested schema

The original DT_SE1Proxy initializer RVA 0x2D5C30 constructs exactly
two properties, excluding the dummy slot:

| Index | Name | Type | Offset | Table | Proxy |
| --- | --- | --- | --- | --- | --- |
| 0 | baseclass | data table | 0 | DT_GameRulesProxy | direct |
| 1 | se1_gamerules_data | data table | 0 | DT_SE1GameRules | custom |

The referenced original table counts are both zero. The custom proxy
at RVA 0x2D5C00 sets eight 32-bit recipient words to all ones and
returns the current game-rules singleton. Standard direct and pointer
proxies at RVAs 0x25E590 and 0x25E5A0 return the data or its stored
pointer without modifying recipients. The original SendPropDataTable
helper at 0x25EDD0 constructs a 0x4C-byte property: type 5, elements 1,
stride -1, flags 0x200 only for the two standard proxy identities,
other scalar/array fields zero; the copy helper at 0x25E6D0 corroborates
the individual x86 field offsets. The owner's SDK dt_send.cpp and
dt_send.h confirm those roles.

server_send_datatable_core.h implements the verified record layout,
bounded table construction, nested-property initialization, standard
proxy semantics and the two-property SE1Proxy graph with an explicitly
supplied game-rules callback. The original game-rules singleton is NOT
accessed. The independent test supplies its own game-rules state and
checks all 256 recipient bits. The native fixture verifies all 18
property field offsets and both record strides, and exact graph names,
order, offsets and flags. No scalar, array or general serialization
implementation is claimed.

This graph is not exported to the engine. GetAllServerClasses remains
guarded until the full class registry, nested schemas, real proxy and
encoder semantics, and ownership are ready. These are independent
implementations from observed interfaces and behavior, not copied
commercial methods or a playable DLL.
