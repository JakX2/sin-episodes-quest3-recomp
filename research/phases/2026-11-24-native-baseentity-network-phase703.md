# Phase 703: native BaseEntity network and time/prediction callbacks

SHA256 matched owner originals: server 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650, client 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d. DT_BaseEntity constructor calls 0x2213f0f5 / 0x240c7631, 24 direct slots each; DT_AnimTimeMustBeFirst 0x2213ec27 / 0x240c4495 (1 property); DT_PredictableId 0x2213af2a / 0x240c450b (2 properties). Borrowed DT_CollisionProperty 8 slots (original 0x222000e3 / 0x2411b470); no recovery of HL2-equivalent collision or physics internals.

| Property | Server slot/offset | Client slot/offset | Encoding |
|---|---|---|---|
| AnimTimeMustBeFirst |0/0|0/0|conditional animating table|
| m_flSimulationTime |1/30|1/6c|8-bit relative ticks flags 0x401|
| m_vecOrigin |2/360|2/2c8|COORD vector flags 0x402|
| m_nModelIndex |3/38|4/8c|signed 11|
| m_Collision |4/fc|19/16c|borrowed direct table|
| m_nRenderFX |5/1c|7/54|unsigned 8|
| m_nRenderMode |6/20|6/7c|unsigned 8|
| m_fEffects |7/78|5/78|unsigned 8|
| m_clrRender |8/24|8/5c|unsigned 32|
| m_iTeamNum |9/22c|9/a0|signed 6|
| m_CollisionGroup |10/214|10/2e4|unsigned 5|
| m_flElasticity |11/21c|11/1d0|COORD float|
| m_flShadowCastDistance |12/220|12/1d4|float 12 flag 1 range 0..4096|
| m_hOwnerEntity |13/20c|13/464|handle|
| m_hEffectEntity |14/210|14/468|handle|
| moveparent |15/f0|15/160|handle|
| m_iParentAttachment |16/ec|16/150|byte unsigned 6|
| movetype |17/0|17/0|custom source +204, destination +144|
| movecollide |18/0|18/0|custom source +208, destination +148|
| m_angRotation |19/36c|3/2d4|angle vector 13 bits flag 0x400|
| m_iTextureFrameIndex |20/2d4|20/46c|unsigned 8|
| predictable_id |21/0|21/0|owner-filtered nested table|
| m_bSimulatedEveryTick |22/2d8|22/470|byte unsigned 1|
| m_bAnimatedEveryTick |23/2d9|23/471|byte unsigned 1|
| m_flAnimTime (nested) |0/2c|0/64|8-bit relative ticks|
| m_PredictableID (nested) |0/3c|0/a4|unsigned 31|
| m_bIsPlayerSimulated (nested) |1/388|1/458|byte unsigned 1|

Hex offsets. Original time encoders 0x2213adc0/ae40 use tick base (global tick /100)*100 and trunc(time / interval + 0.5), animation accepts ticks >= base-100; simulation >= base. Client 0x240c42d0/4360 adds encoded byte to base, first adjusts upward to tick-127 then downward to tick+127, including asymmetric +/-128 case. Port uses bounded 64-bit arithmetic for valid clock/time and rejects nonfinite or invalid clock sample. x87 conversion to int32 modeled for valid numeric inputs; no claim of bit-identical behavior at extreme floating-point rounding boundaries.

Original 0x2213afa0 predictable proxy checks ID +3c nonzero, sets one recipient bit at (ID>>1)&31 (helpers 0x2226fb80/fbb0); returns null and leaves mask on zero ID. Raw ID helper 0x2227ec10/0x2226fc60; client 0x241a8d60/0x24195390. Movement proxies 0x2213eb80/eba0 read actual fields despite descriptor offset zero, client 0x240c7330 clears movecollide on movetype write; 0x240c61e0 writes movecollide. Client effects 0x240c4420/0x24094260 notifies after changed value; tick flags 0x241a8cf0 notify interpolation only on changed bool. Host supplies actual side effects, not fake engine services.

Origin and rotation callbacks 0x2213ec40/ecb0 choose entity state or alternate step-simulation state depending on original global, movement and eligibility. Host provides exact selected transform, rather than reimplement HL2 physics. Rotation proxy then wraps all components through trunc(angle*(65536/360)) & 0xffff and multiplies by 360/65536 before separate 13-bit wire encoding. Quantized metadata and COORD unused fields normalized. No wire codec or retail class registration claim.

Native paired fixture checks time threshold/wrap, animation gate, recipient filter, movement side effects, transform selection callback, angle normalization, entity handles, effects/tick notifications, descriptor ordering and lifecycle. Integrated BasePlayer fixture now uses constructed BaseEntity and DT_Local graphs; only borrowed CollisionProperty schema and host operations remain external in this graph. Original binaries not committed.
