# Phase 702: native DT_Local and FOV transitions

Owner originals SHA256 rechecked: server 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650, client 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d. Table constructor calls 0x2225818e / 0x240d389d, props 0x22578be4 / 0x2433936c, 48/49 slots. Client-only slot 12 m_vecClientBaseVelocity at +0x58 shifts the remaining slots by one. Original server initializer 0x22257ae0, client 0x240d32e0.

AreaBits Array3 constructors 0x22257b42/0x240d334d: 32 unsigned byte children, base +4, stride1. Parent name m_chAreaBits; server arrayProp points to owned prototype, client unlinked parent array_prop null. Byte callbacks preserve adjacent bytes.

| Server slot | Client slot | Property | Server offset | Client offset | Type/width | Send bits/flags |
|---:|---:|---|---|---|---|---|
|0|0|m_chAreaBits|0x4|0x4|Array3 32x1|8/1|
|1|1|m_iHideHUD|0x24|0x24|I|11/0x1|
|2|2|m_iFOV|0x28|0x28|I|9/0x0|
|3|3|m_flFOVRate|0x2c|0x2c|F|0/0x4|
|4|4|m_iDefaultFOV|0x30|0x38|I|9/0x0|
|5|5|m_bDucked|0x40|0x3c|B|1/0x1|
|6|6|m_bDucking|0x41|0x3d|B|1/0x1|
|7|7|m_bInDuckJump|0x42|0x3e|B|1/0x1|
|8|8|m_flDucktime|0x44|0x40|F|12/0x408|
|9|9|m_flDuckJumpTime|0x48|0x44|F|12/0x8|
|10|10|m_flJumpTime|0x4c|0x48|F|12/0x8|
|11|11|m_flFallVelocity|0x54|0x50|F|17/0x400|
|12|13|m_vecPunchAngle|0x5c|0x64|V|0/0x402|
|13|14|m_vecPunchAngleVel|0x68|0xa0|V|0/0x2|
|14|15|m_bDrawViewmodel|0x74|0xac|B|1/0x1|
|15|16|m_bWearingSuit|0x75|0xad|B|1/0x1|
|16|17|m_bPoisoned|0x76|0xae|B|1/0x1|
|17|18|m_flStepSize|0x78|0xb0|F|16/0x10|
|18|19|m_bAllowAutoMovement|0x7c|0xb4|B|1/0x1|
|19|20|m_skybox3d.scale|0x84|0xbc|I|12/0x0|
|20|21|m_skybox3d.origin|0x88|0xc0|V|0/0x2|
|21|22|m_skybox3d.area|0x94|0xcc|I|8/0x1|
|22|23|m_skybox3d.fog.enable|0x9c|0xd4|B|1/0x1|
|23|24|m_skybox3d.fog.blend|0x9d|0xd5|B|1/0x1|
|24|25|m_skybox3d.fog.dirPrimary|0xa0|0xd8|V|0/0x2|
|25|26|m_skybox3d.fog.colorPrimary|0xac|0xe4|I|32/0x1|
|26|27|m_skybox3d.fog.colorSecondary|0xb0|0xe8|I|32/0x1|
|27|28|m_skybox3d.fog.start|0xb4|0xec|F|0/0x4|
|28|29|m_skybox3d.fog.end|0xb8|0xf0|F|0/0x4|
|29|30|m_fog.enable|0xc4|0xfc|B|1/0x1|
|30|31|m_fog.blend|0xc5|0xfd|B|1/0x1|
|31|32|m_fog.dirPrimary|0xc8|0x100|V|0/0x2|
|32|33|m_fog.colorPrimary|0xd4|0x10c|I|32/0x1|
|33|34|m_fog.colorSecondary|0xd8|0x110|I|32/0x1|
|34|35|m_fog.start|0xdc|0x114|F|0/0x4|
|35|36|m_fog.end|0xe0|0x118|F|0/0x4|
|36|37|m_fog.farz|0xe4|0x11c|F|0/0x4|
|37|38|m_audio.localSound[0]|0xec|0x124|V|0/0x2|
|38|39|m_audio.localSound[1]|0xf8|0x130|V|0/0x2|
|39|40|m_audio.localSound[2]|0x104|0x13c|V|0/0x2|
|40|41|m_audio.localSound[3]|0x110|0x148|V|0/0x2|
|41|42|m_audio.localSound[4]|0x11c|0x154|V|0/0x2|
|42|43|m_audio.localSound[5]|0x128|0x160|V|0/0x2|
|43|44|m_audio.localSound[6]|0x134|0x16c|V|0/0x2|
|44|45|m_audio.localSound[7]|0x140|0x178|V|0/0x2|
|45|46|m_audio.soundscapeIndex|0x14c|0x184|I|17/0x0|
|46|47|m_audio.localBits|0x150|0x188|I|8/0x1|
|47|48|m_audio.ent|0x154|0x18c|H|21/0x5|

I=int32, B=byte, F=float, V=vector3, H=entity handle. COORD and NOSCALE unused ranges normalized; quantized duck timers 0..2047.5 multiplier2, step size low 128/65536 high128 multiplier512, fall velocity -4096..4096. No physics/model/prop implementation.

Original FOV proxy 0x240d2e80 compares +0x28; on change saves previous displayed local player FOV (vslot208, truncation), otherwise prior target or fallback cvar default when prior zero, into +0x30; current time word (global +0xc) goes to +0x34. Host sample explicitly supplies these inputs. Default FOV proxy 0x240d2590 compares +0x38; on change clears rate +0x2c then writes default. No local player global or fixed-address call. Original poison helper 0x2227eb40/0x241a8cc0 just delegates one-byte integer construction.

Paired native fixture checks every original slot and 32 area elements, exact offsets, role asymmetry, FOV transition/no-change/fallback/refusal/default reset and reverse destruction. Existing BasePlayer integration now uses a reconstructed DT_Local instead of a dummy. DT_BaseEntity remains a placeholder in that fixture. Native descriptors not registered with retail engine. No raw original binary data committed.
