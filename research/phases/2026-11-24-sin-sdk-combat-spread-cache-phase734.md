# Phase 734: original cached combat-spread semantics

Owner SDK reopened: SinEpisodesSDK.zip SHA256
5a934e6c3f99151f2ba1a90fd080c7d6cd3f80a2b2b8118e5ef9cb2f3bb3d20b,
SE1/scripts/rules_combat.txt previously pinned at Phase678. That phase
already implements the three-tier exact/wildcard parser; Phase679
filesystem loading and Phase680 focus are also present. Do not duplicate.

Fresh server.dll SHA match and original 222d7ee0..222d8157 analysis:
reads each entity classname pointer +c (or original empty string
223d47a8 when present but no classname); missing entity contributes
zero. Fold each pointer by (p XOR p>>16)&ffff. NPC low16,
weapon next16, target high DWORD low16. Ordered tree lookup by
64-bit unsigned key. Existing entry +18 supplies vector directly,
no lookup of script, fallback logging or difficulty metric. A miss
uses helper 222d5470 exact/wildcard at each layer, fallback float
0x3e05a9a8 (0.13053), calls metric12 222d33e0, multiplies and stores
three equal floats via 222d7dc0 insertion. A collision of folded
identities aliases the same entry; do not compare strings on hit.
The CAI_BaseNPC focus override applies a transient multiplier AFTER
the cached base vector (Phase680). Original constructor 222d7e30
owns rules and cache; no invalidation on skill selection was seen in
this lookup. Full original object lifecycle remains host-owned.

New opt-in API accepts explicit 32-bit classname identities rather
than hashing the port's string bytes or retaining raw pointers.
Separate from existing live-query API, stores diagnostic base/fallback
in addition to original vector, uses 65536 entry port limit and
requires active campaign even on hit. Host must clear on original
rules-object destruction/replacement; no auto-clear on port skill or
script reload. Cached focus shares existing verified penalty helpers.
Tests prove key packing, collision alias, distinct entry, stale vector
after skill change, live API still recomputes, focus applied after hit,
absent vs empty identities, invalid identities, explicit clear and
optional actual owner SDK lookup. No script content redistributed.
