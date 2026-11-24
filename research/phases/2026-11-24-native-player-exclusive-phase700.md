# Phase 700: native DT_LocalPlayerExclusive

Fresh SHA-matching owner originals, server table constructor 0x22249d9c / client 0x240d3c1c, 27 declared slots both. Original nested DT_Local at 0x22579aa8 (48 server properties) / 0x24338e74 (49 client properties) remains externally supplied, not inferred from modern SDK. Slot 5 borrows the Phase696 ammo descriptor and numbered graph.

| Slot | Name | Server offset | Client offset | Original server encoding |
|---|---|---|---|---|
| 0 | m_Local | 794 | d68 | nested direct |
| 1–2 | m_vecViewOffset[0–1] | 37c/380 | e0/e4 | float 8 -32..32 |
| 3 | m_vecViewOffset[2] | 384 | e8 | float 10 flags 0x400 0..128 |
| 4 | m_flFriction | 2a8 | 1ec | float 8 ROUNDDOWN 0..4 |
| 5 | m_iAmmo | 64c | c00 | Array3 32 |
| 6–7 | m_fOnTarget / m_bOnUseTarget | 94c/94d | f10/f11 | byte unsigned 2 |
| 8 | m_nTickBase | 1274 | 102c | signed 32 flags 0x400 |
| 9 | m_nNextThinkTick | 74 | 84 | signed 32 |
| 10–11 | m_hLastWeapon / m_hGroundEntity | 1280/238 | 1040/1e0 | handle helper |
| 12–14 | m_vecVelocity[0–2] | 2c8/2cc/2d0 | ec/f0/f4 | float 20/20/16 flags 0x400 -2048..2048 |
| 15 | m_vecBaseVelocity | 244 | 124 | vector 20 -1000..1000 |
| 16 | m_hViewModel / m_hViewModel[0] | bdc | 1058 | handle element |
| 17 | m_hViewModel | 0 | 0 | DPT_Array count 2 stride 4 |
| 18 | m_hConstraintEntity | 12a8 | f48 | handle helper |
| 19 | m_vecConstraintCenter | 12ac | f4c | NOSCALE vector |
| 20–23 | constraint radius/width/speed factor/view constraint | 12b8/12bc/12c0/12c4 | f58/f5c/f60/f64 | NOSCALE float |
| 24 | m_flDeathTime | 9d8 | f74 | NOSCALE float |
| 25 | m_nWaterLevel | 298 | 1dc | unsigned 2 DWORD |
| 26 | m_flLaggedMovementValue | 12c8 | 1174 | NOSCALE float |

Hex offsets. Factory 0x2227eb70 forwards its fourth argument (4) OR 1 as SendPropInt flags (5) and its fifth argument as size, even where input fifth is 0x400; explicit handle proxy at 0x2227ec60. Original integer width -1 is normalized by 0x2225ebf0. NOSCALE unused ranges normalized. DPT_Array factories 0x2225ee70 / 0x24126bd0 construct unlinked array records adjacent to element; engine preprocessing must link them before actual encoding. The native server no-op table callback returns null (original bare ret, undefined return register). No false Array3 substitution.

Client custom velocity component callbacks 0x240d2f00/2f50/2fa0 compare incoming float numerically with the corresponding current component and call 0x240c5340 on change. That setter compares whole vector, calls 0x24096290 with flag 4 before storing all three words. Port-owned proxy preserves comparison and call order, delegates actual recursive invalidation to host callback and refuses writes when it fails. The host guarantees writable entity storage. No model/prop/physics implementation or retail registration claim.

Paired native DLL fixture checks all 27 slots, byte/handle/vector raw proxy transfers, existing ammo graph composition, velocity change/no-change/signed-zero and refusal, array descriptor shape and lifecycle. This is pre-initialization schema, not an actual wire codec. No original binary bytes committed.
