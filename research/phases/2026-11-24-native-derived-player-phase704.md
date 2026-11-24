# Phase 704: SiN-specific derived player and PlayerHand networking

Original owner binaries rechecked against pinned hashes. DT_HL2_Player (6) server ctor 0x2239f625 / client 0x242164b0, parent pointers 0x22518654 -> 0x225765d0 and 0x242f8868 -> 0x24338ebc (DT_BasePlayer). DT_HL2Local (13) server 0x223a2987 / client 0x2421ad7e. DT_PlayerHand (1) server 0x2239b275 / client 0x24216379: baseclass parent pointers 0x224f0300 -> 0x225587f8 and 0x242f7ecc -> 0x24337974 (DT_BaseEntity, NOT BaseAnimating).

| DT_HL2_Player property | Server offset | Client offset | Encoding |
|---|---|---|---|
| baseclass | 0 | 0 | DT_BasePlayer |
| m_HL2Local | 137c | 11c8 | DT_HL2Local; server owner-ID proxy 0x22258250 |
| m_fIsSprinting | 13d9 | 1224 | unsigned byte 1 |
| m_flFlashTime | 1510 | 123c | NOSCALE float |
| m_flFlashFadeTime | 1514 | 1240 | NOSCALE float |
| m_flFlashStartAlpha | 1518 | 1244 | NOSCALE float |

| DT_HL2Local property | Both offsets | Encoding |
|---|---|---|
| m_fFlashlightPower | 4 | float 10 flags 0x11 (ROUNDUP), 0..100 |
| m_fSprintPower | 8 | same |
| m_fAccuracy | c | NOSCALE float |
| m_bZooming | 10 | unsigned byte 1 |
| m_bZoomScope | 11 | unsigned byte 1 |
| m_bitsActiveDevices | 14 | unsigned DWORD 3 |
| m_iSquadMemberCount | 18 | signed DWORD 32 |
| m_iSquadMedicCount | 1c | signed DWORD 32 |
| m_fSquadInFollowMode | 20 | unsigned byte 1 |
| m_bWeaponLowered | 21 | unsigned byte 1 |
| m_poisonTimer | 24 | NOSCALE float |
| m_breathTimer | 28 | NOSCALE float |
| m_hLadder | 2c | handle helper 21 bits flags5 |

Hex offsets. Flashlight/sprint original factory args flag 0x11, 10 bits, range 0..100; ROUNDUP adjusts lower endpoint 100/1024 and multiplier 1024/100. Original 32-bit float constructor automatically adds NOSCALE flag; bits normalize to zero. Reuses original scalar, byte and validated handle proxies and existing object-ID recipient callback.

The native pair fixture now constructs the actual previously implemented BasePlayer and BaseEntity graphs and derives these three tables from them, transfers every direct scalar through callbacks (raw payload, not real bitstream), checks parent offset and private recipient routing, and destroys in dependency order. No collision/physics implementation, live entity construction, original engine registration or gameplay claim. No original binary bytes committed.
