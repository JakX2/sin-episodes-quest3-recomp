# Phase 123: original vehicle view-record permission byte

**Scope:** independently authored C++17 reconstruction of one additional binary-backed field in the original SiN Episodes: Emergence 2006 vehicle view record. This phase builds on draft PR #174 and does not add original binaries, game assets, maps, or engine code.

## Verified original layout

The preserved owner-only Phase 24 verifier `phase24_verify.py` records the original server/client SHA-256 identities and exact instruction witnesses. Its dataflow table establishes:

| Field / operation | Original evidence | Record/player offset |
|---|---|---:|
| View record stride | Original damage routine indexed at 100-byte stride | `0x64` bytes |
| `view_allow_weapons` constructor default | `server.dll:0x22332368`, byte zero store | record `+0x55` |
| `view_allow_weapons` read by `SetView` | `0x2232FACC`, `8A 4D 55 51` | record `+0x55` |
| Player permission setter | `0x22245410`, copies argument byte to player field | player `+0x9B4` |
| Player permission consumer | `0x2232EC3D`, compares player byte with zero | player `+0x9B4` |
| `view_damage_mod` constructor default | `0x2233236C`, 1.0f store | record `+0x60` |
| `view_damage_mod` config write | `0x223335DA` | record `+0x60` |
| Damage multiplier | `0x2232FA19` | record `+0x60` |

The selected-view index is stored at player `+0x9B0`; the permission byte is a separate field at player `+0x9B4`. The original `SetView` handler obtains the selected record's permission byte and passes it to the player setter. The player-side consumer tests the stored byte against zero. The portable representation therefore keeps `allow_weapons` as `std::uint8_t`, not `bool`, and preserves noncanonical nonzero byte values instead of normalizing them.

## Independent implementation

- Extend `decode_original_view_records()` to decode the verified raw permission byte at `+0x55` and the already reconstructed float at `+0x60`.
- Keep the parser bounded to the complete 100-byte record stride and transactional on truncated/null nonempty input.
- Leave all other record fields at conservative defaults; no unverified offsets are inferred.
- Change the portable host permission callback to accept `std::uint8_t`, retaining the original stored byte through the `SetView` path.
- Add regression coverage for zero and nonzero raw bytes and for decoded-record → `SetView` → player-host propagation.

## Evidence boundary

The Phase 24 verifier script and its instruction/field witness table are available in the user's Library. The raw `bin.zip` was not visible as a current conversation attachment or materialized Library artifact during this run, so this change relies on preserved, hash-pinned prior evidence and makes **no fresh instruction-level DLL audit claim**. The original DLLs remain read-only and are not committed.

This is not a SourceVR native vehicle entity, not complete vehicle damage or rendering, and not Quest 3 execution. It closes two view-record fields only. Remaining release blockers include the lawful engine host, real entity I/O and campaign transitions, vehicle physics/driver, client monitor rendering, NPC/combat integration, save/load and full campaign execution.
