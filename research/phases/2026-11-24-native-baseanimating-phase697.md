# Phase 697: native BaseAnimating graph

Both owner originals were freshly SHA-256 matched (client 6535f13e...1841d, server 568a0086...9650). Original constructor regions: server 0x220ec770 and client 0x240b249c (18 properties); ServerAnimationData server 0x220ec47f and client 0x240b21b7 (1 property). BaseEntity pointers resolve to 0x225587f8 / 0x24337974. Parent is required externally with 24 declared properties.

| Property | Server slot / offset | Client slot / offset | Server factory args |
|---|---|---|---|
| baseclass | 0 / 0 | 0 / 0 | direct |
| m_nForceBone | 1 / 3a0 | 2 / 510 | int8 signed |
| m_vecForce | 2 / 3a4 | 3 / 504 | NOSCALE vector |
| m_nSkin | 3 / 3b0 | 4 / 4a0 | signed 10 |
| m_nBody | 4 / 3b4 | 5 / 4a4 | signed 32 |
| m_nHitboxSet | 5 / 3b8 | 6 / 4a8 | unsigned 2 |
| m_flModelWidthScale | 6 / 3bc | 7 / 544 | 6 bits, ROUNDUP, 0..1 |
| m_flPoseParameter | 7 / 3f0 | 8 / 548 | Array3 24 x float 11 bits |
| m_nSequence | 8 / 3ec | 1 / 6e0 | unsigned 10 |
| m_flPlaybackRate | 9 / 3c0 | 9 / 4c8 | 8 bits, ROUNDUP, -4..12 |
| m_flEncodedController | 10 / 450 | 10 / 66c | Array3 4 x float 11 bits ROUNDDOWN |
| m_bClientSideAnimation | 11 / 460 | 11 / 6b0 | unsigned byte 1 |
| m_bClientSideFrameReset | 12 / 461 | 12 / 4f4 | unsigned byte 1 |
| m_nNewSequenceParity | 13 / 464 | 13 / 6b4 | 3 bits, flags 0x401 |
| m_nResetEventsParity | 14 / 468 | 14 / 6b8 | 3 bits, flags 0x401 |
| m_nMuzzleFlashParity | 15 / 46c | 15 / 784 | byte 2 bits flags 0x401 |
| m_hLightingOrigin | 16 / 470 | 16 / 780 | handle helper flags 5 |
| serveranimdata | 17 / 0 | 17 / 0 | server 0x2213eb50, client direct |
| m_flCycle (nested) | 0 / 3e8 | 0 / 6e4 | 15 bits, ROUNDDOWN, 0..1 |

Offsets hexadecimal. Float factory 0x2225e8c0 adjusts bounds by one 2^-bits step before computing multiplier 0x2225e5c0. For these binary fractional ranges the scale is exact. NOSCALE vector range/multiplier normalized. Integer helper 0x2225ebf0 chooses byte zero extension for unsigned one-byte inputs; client 0x24126df0 selects byte write for size one. Lighting handle helper 0x2227eb70 invokes 0x2227ec60; client 0x241a8db0. Port keeps existing strict handle index validation. Server animation proxy calls vslot49 then checks byte +0x460; returns data only for non-client-side animation and leaves recipient mask unchanged.

New role-specific graph API owns all four tables including 28 numbered child descriptors. Paired fixture checks original order divergence, endpoint metadata, raw scalar/vector/array callbacks, byte canaries, handle reconstruction, cycle gate, lifecycle and weapon graph composition. Not a wire codec, not a live entity or model/prop implementation. BaseEntity remains external; Overlay and BaseFlex remain. Engine registration not enabled. No original executable bytes committed.
