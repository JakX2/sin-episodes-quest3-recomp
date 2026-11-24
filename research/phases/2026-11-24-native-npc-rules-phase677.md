# Phase 677: original NPC rule consumers and engine RNG

Direct inspection of the original SHA-matching 2006 server.dll
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
identified original CSE1Rules vtable base 0x2245f74c (slot63 at
0x2245f848). Original methods: slot97 health drop 0x222d4940,
slot98 secondary fire 0x222d4980, slot100 spawn count delta
0x222d4a00, slots101..104 helmet, helmet shot off, cover, mobility
at 0x222d4a80/0x222d4ad0/0x222d4b20/0x222d4b70 respectively.
All nonzero probability paths call the original VEngineRandom001
global 0x225721b0 virtual +4 with float bounds 0,1. Zero thresholds
skip RNG. FCOMPP tests C0 (abs(threshold) < random), so equality
succeeds. The earlier simplified spawn count reference used strict
less-than; this native implementation follows the actual branch.
Mobility multiplies metric13 by the caller factor and literal 0.2f
at 0x223d9538 before f32 storage, without clamping above 1.
Health drop uses generic metric4 roll at 0x222d3920 when forced
metric3 is zero. Fixed profiles keep metric3 zero; nonzero
forced-counter mutation is deliberately outside this API.

Crucial correction: original attack-spread lookup 0x222d7ee0
(631 bytes, SHA256 e3189ca6fd7ef55db8d3ed4e2bcd35602c13599f32b51a074062bdd96cf2c7c8)
first returns cached three-component spread on cache hit. The miss
path looks up class_names, weapon_class_names, target_class_names,
or logs "Attack spread Rule not found..." and uses 0.13053f.
At 0x222d8107 it multiplies resolved base spread by metric12
(PERCENT_NPC_ACCURATE), then copies that value to x/y/z.
Therefore the old fixed profile inverted actual enemy spread:
Easy=0.5, Hard=1.0 was backwards. Corrected to Easy=1.0,
Normal=0.75 (unchanged exact original), Hard=0.5. These remain
new fixed policy choices, not original selectable difficulty levels.

New native DLL NpcRulesApi consumes the single shared campaign
profile. Portable caller-provided RNG callback and Win32 direct
VEngineRandom001 slot1 export use the same rule implementation.
Zero threshold does not resolve or invoke RNG. Nonzero threshold
always draws exactly once even if above one. Invalid samples,
exceptions and reentrant campaign mutation are refused. The
spread API accepts the caller-resolved uncached base value; it
does not invent the KeyValues rule/cache implementation or
rescale cache hits. Existing Phase663 root regression updated
to match authenticated spread direction. Native loaded-module
tests check all supported decisions, equality, zero RNG, >1,
actual synthetic Win32 vslot1 dispatch and corrected spread.
No actual engine-owned NPC callbacks or retail gameplay claimed.
