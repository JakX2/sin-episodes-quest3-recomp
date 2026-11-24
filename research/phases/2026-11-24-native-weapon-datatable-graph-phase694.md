# Phase 694: native Win32 weapon table graphs

Phase693 all seven workflows passed. Fresh SHA256 owner binaries
server 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
client 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

Original table constructor call addresses, server / client:
DT_LocalWeaponData 222acb10 / 24091076 (5)
DT_LocalActiveWeaponData 222acfea / 24090f9e (4)
DT_BaseCombatWeapon 222ade36 / 24091196 (7)
DT_BaseHLCombatWeapon 222cbda5 / 2420d659 (1)
DT_HLMachineGun 2230b425 / 24215f49 (1)
DT_HLSelectFireMachineGun 2230b225 / 24215fe9 (1)
DT_WeaponMagnum 22316a95 / 2422a5e9 (1)
DT_WeaponAssaultRifle 22313b95 / 2422a6b9 (1)
DT_WeaponScatterGun 2231bce5 / 2422aa59 (1).
Direct original table-global pointer resolution confirms BaseCombat
inherits DT_BaseAnimating; BaseHL -> BaseCombat; MachineGun -> BaseHL;
SelectFire -> MachineGun; Magnum/ScatterGun -> BaseHL;
AssaultRifle -> SelectFire. Server globals 224e88b4,2251f80c,
22522e6c,2252c084,2252c0a0; client 242f7420,242f6278,2431efdc,
2431f75c,2431f760. All inheritance offsets zero. Both local tables
nested in BaseCombat. No extra direct weapon-specific properties.

Rechecked original server SendPropInt factory 2225ebf0, standard
32-bit raw copy proxies 2225e540/2225e570, float copy 2225e420,
clip add-one 2227eb10, handle 2227ec60, timer wrapper 2227ebd0.
Client RecvPropDataTable factory 24126b50, scalar factories
24126df0/24126d80, direct table 24126ce0, raw copies 24126c90/
24126c40, clip 241a8c20, handle 241a8db0. SDK2004 dt_common.h,
dt_recv.h corroborate 16-byte variant, 28-byte recv proxy data,
60-byte RecvProp, 20-byte RecvTable. Existing verified SendProp
76-byte and SendTable 20-byte layout reused. Known scalar field
offsets, widths and flags come from Phase692; client flags zero.
Unused NOSCALE quantization fields normalized to zero, not byte
parity with all original unobserved/irrelevant fields.

Actual independent Win32 server and client DLLs now expose separate
role-specific graph creation/destruction APIs. Caller must provide
a compatible nonempty DT_BaseAnimating dependency; no placeholder
base graph built in the DLL. Server requires both host owner-only
table callbacks and live handle packing callback. Built-in scalar
callbacks implement raw DWORD and clip plus-one/minus-one, receive
handle conversion. Original offsets apply ONLY to a host's verified
compatible storage, never the existing port MapEntity object.
Graph owns descriptor strings and table/prop lifetimes, retains
explicit borrowed base and callback dependencies. Destroy refuses
if engine-owned table flags or precalc/decoder are still present;
caller must detach all references before teardown.

Actual PE32 paired loader walks all nine tables in both modules,
compares metadata, invokes 13 property callbacks against padded
owned fixtures, tests owner recipient mask, invalid dependencies,
role-specific exports, and teardown guards. Not full serializer:
scalar callbacks do not implement bitstream, flattened graph or
encoder limits. No engine table registration, actual entity creation,
BaseAnimating implementation, retail networking or startup claim.
No proprietary bytes or assets committed.
