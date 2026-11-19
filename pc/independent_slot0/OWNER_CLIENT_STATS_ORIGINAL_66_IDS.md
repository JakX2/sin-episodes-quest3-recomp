# SHA-pinned original SiN 2006 stats name-to-ID registry

Original client.dll SHA256:
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

## Owner-original evidence

The game function VA 0x241FFB60 loops over exactly 66 pointers
from VA 0x2431E428 (RVA 0x31E428). It increments EBX once per
registration, stores that index in the registry item at +0x0C
and ends at 0x42 (66). The table was decoded from the exact
SHA-matched 2006 original, not guessed from game script order.

Pointer-block SHA256 (264 original bytes):
0c91373338d5d085702cc9dfce22a03151d41c26cd86660a34034548180ae0ec

Joined ordered-66-names SHA256:
8913a9a523ffcfaf6baf621f6b3d0c82786346c510afa8d56e6b2fe30909a1ce

Original stats_definitions.txt SHA256:
a18afd2e8fba54653a9bd173c3eacf440fe016b3659b74578f2ecd1f37a156a8

The original script defines 62 distinct names, all of which map to
the compiled table. The other four compiled-only ID names are:

- 51 STAT_NUM_DD_TRIGGERS
- 52 STAT_NUM_ITEMS_PICKED_UP
- 53 STAT_NUM_ITEMS_USED_GENERIC
- 54 STAT_NUM_FLAILS

Script order must not be used as internal identifier order.
For example, STAT_EXPECTED_TIME is original compiled ID 2, but
it is located at position 34 in the definitions file.

## New independent host implementation

client_stats_original_id_map.h: original canonical 66 identifiers,
exact spelling lookup and explicit unknown-name refusal.

client_stats_original_catalog_binding.h: atomic projection of up
to 66 parsed definitions into separate, independently owned text
entries. The original 12-byte catalog record cannot store host
std::string pointers. Therefore this code does NOT populate the
record's original 32-bit first/second pointer fields; it only writes
the validated percentage_of target index into its +8 integer field.
This matches one observed behavior of original 0x241FBD20 without
pretending the full object ABI is reconstructed.

Missing allocation, duplicate/unknown stat names or unresolved
percentage references refuse the entire new projection without
modifying the previous good snapshot. Retry may recover after a
catalog rebuild. The projection is separate from retail event hookup.

test_client_stats_original_id_map.cpp: native MSVC x86 synthetic
regression over all 66 IDs and 62 source definitions, four
compiled-only names, bad percentage reference, duplicate names,
failed allocation and safe retry.

audit_sin_original_stats_registry.py: owner-only original SHA check,
registered table and count, and optional owner definitions SHA
cross-check. Synthetic --self-test constructs a fabricated PE and
rejects seven table/code mutation cases without using game bytes.

## Owner-private reproduction

From pc/independent_slot0, run:

    python audit_sin_original_stats_registry.py <path-to-original-client.dll> --definitions <path-to-original-stats_definitions.txt>

Synthetic-only regression:

    python audit_sin_original_stats_registry.py --self-test

This is original static metadata and independent host behavior.
It is not a retail KeyValues initialization, event hookup,
successful normal VClient011 Init, map load or playable game boot.
