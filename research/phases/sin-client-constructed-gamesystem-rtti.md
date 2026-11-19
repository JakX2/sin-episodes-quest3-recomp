# Original constructor-created 2006 AutoGameSystem identities

Private read-only analysis of original SHA256-authenticated client.dll
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
The previous PR #434 identified four initialized static C++ objects.
This phase also pins **eight** original C++ constructor vtable writes
for game systems whose object memory is in BSS (not raw PE data).
The auditor checks exact x86 vptr stores, MSVC RTTI complete object locators
and type descriptors, virtual slot zero, and SHA256 of the first 16
original method bytes.

| Registration index | RTTI class name | Object VA | Registered subobject offset | Init method VA |
| --- | --- | --- | --- | --- |
| 1 | CGameStringPool | 0x2436212C | 0 | 0x240566C0 |
| 3 | CClientLeafSystem | 0x24357F30 | +4 | 0x2410CA00 |
| 4 | CDetailObjectSystem | 0x243589A8 | 0 | 0x24126300 |
| 6 | CClientEffectPrecacheSystem | 0x2433FD9C | 0 | 0x241055A0 |
| 7 | CClientShadowMgr | 0x24358280 | 0 | 0x24118880 |
| 8 | C_StatsSystem | 0x24378DE8 | 0 | 0x241FCAA0 |
| 9 | C_DynamicDifficulty | 0x24378D18 | 0 | 0x241FAC90 |
| 11 | C_SoundscapeSystem | 0x2433E4F8 | 0 | 0x240F4160 |

The third system is especially important: original client Init obtains
pointer `[0x242FA79C]`, adds four bytes, then registers this
**secondary IGameSystem subobject**. Its constructor `0x2410C790`
sets a secondary vptr to `0x24297534`, with original RTTI object offset
4, rather than using the primary `CClientLeafSystem` vtable.
Treating that subobject as primary would call the wrong virtual method.

The first, third and sixth entries have complete, identical
three-byte `Init` functions that always return true
(`mov al,1; ret`). The other five have nontrivial bodies; only
their entry prefixes and virtual method locations are authenticated
at this stage.

Together with the four original raw-PE RTTI systems in PR #434,
we now have original class and virtual Init identities for **all
twelve direct-registered systems**. This is a crucial ABI/map milestone
but not a game startup: constructors must still be independently
implemented, including sound, physics, client shadows, stats,
dynamic difficulty, spatial leaf and soundscape subsystems.

Tool:
`python research/tools/audit_sin_client_constructed_gamesystems.py
/path/to/original/client.dll`
checks everything against the privately held original and
does not publish original code bytes.
