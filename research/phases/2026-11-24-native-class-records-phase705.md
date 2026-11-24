# Phase 705: native PE32 class-list records

Fresh read-only inspection of SHA-matching original owner DLLs:
server 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650;
client 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

ServerGameDLL003 slot 10 getter 0x221e7c30 returns global 0x22589050.
Constructor 0x22001290 writes name +0, SendTable +4, next +8,
instance baseline +16 = 0xffff; class ID +12 starts zero in static storage.
It inserts in case-insensitive order (comparator 0x223a5574).
164 unique server registrations were recovered by matching calls.
VClient011 vtable 0x24296c6c slot 5 getter 0x240fd990 returns global
0x2433fd90. ClientClass constructor 0x24053c40 writes create +0,
event create +4, network name +8, RecvTable +12, next +16, prepends.
Class ID +20 starts zero in static storage. Most constructors are inlined;
191 distinct registrations recovered from head stores, including 42 event
creators and 149 entity creators. All server network names occur on client;
27 client-only names. One matched name (CWorld) differs in table-name case.
CKeypad's table constructor pattern is not captured by the simple table
scanner, but both class registrations are present.

15 already reconstructed class/table pairs, in ascending client
registration-code address order (the subset list prepends in this order):

| Network name | Server class storage | Client class storage | Client creator |
|---|---|---|---|
| CBaseCombatWeapon | 22591a24 | 2432b94c | 24094110 |
| CBaseAnimating | 2254d108 | 24336290 | 240bdbf0 |
| CBaseAnimatingOverlay | 2254d590 | 24337494 | 240c0c50 |
| CBaseCombatCharacter | 22590f58 | 242f7d64 | 240c39a0 |
| CBaseEntity | 225587e4 | 2433792c | 240cd470 |
| CBaseFlex | 2254d97c | 24338a0c | 240d1570 |
| CBasePlayer | 225765bc | 24338ea4 | 240d6160 |
| CBaseHLCombatWeapon | 22592a30 | 2437902c | 2420d5a0 |
| CHLMachineGun | 22597fd4 | 2431f768 | 242161f0 |
| CHLSelectFireMachineGun | 22598004 | 2437a92c | 24216250 |
| CPlayerHand | 2259e45c | 2431f810 | 24216540 |
| CHL2_Player | 2259eb80 | 2437abd0 | 242168a0 |
| CWeaponMagnum | 225986ec | 2437f548 | 2422b240 |
| CWeaponAssaultRifle | 225984a0 | 2437f580 | 2422b620 |
| CWeaponScatterGun | 22598dc8 | 2437f5f0 | 2422a9a0 |

Addresses hex, evidence only; original function addresses never invoked.
Native role-specific ABI exports construct exact class record layouts for
this subset using actual reconstructed graph instances and borrowed client
creators. Validate class/table names, counts, unique identity, and shared
baseclass table pointers. Server sorted insertion and client prepend are
tested independently. Class ID/baseline and engine-owned table flags guard
teardown; callers must detach all graph/list references before release.

Actual retail class getters remain guarded: incomplete subset, external
collision schema, no original entity constructors, no complete registry
or array preprocessing/engine initialization. Test uses an explicit fixture
creator, not gameplay construction. No original bytes committed.
