# Phase 733: original player damage dispatch and poison attachment

Reopened actual owner SDK and DLLs, hashes match previous pins.
Original CHL2_Player damage override 2239fa00..2239fb85.
Global state lookup 221f4680/4690, name gordon_invulnerable
224af48c; exact value 1 suppresses. Also suppress if +8c !=0.
Flag 20 checks +14b8 deadline strictly > now; if so clear and
return zero. Flag 10000000 with player +298 >2 checks actor
+298 via info +24 resolution; zero suppresses.
Amount info+2c strictly >0 sets player +14f4=now, resolves source
info+28 (22033770), if live resolves again and calls player
vslot224 (+380). Flag 4000 compares +9f4/+9f8: equal sound
Player.DrownStart, different Player.DrownContinue. Flag 8 emits
HL2Player.BurnPain. No inferred medical/physical behavior.

Original copy ctor 2202f480 copies exactly 0x48 bytes. If copied
flags +38 & 0x30000 (poison/radiation), resolves source +28,
RTTI cast CBaseEntity->CPoisonGas (2252a01c), if successful calls
handle setter 2218f420 at +14d8; return zero regardless of cast.
This directly connects Phase728 gas and Phase729 exposure source.
Else resolves source and actor from original info; if neither is
player, 221f8770 copies +2c into +34 then calls game-rules slot63
on mutable copy. Finally 2224c020 base damage receives copy and
returns original result. Host supplies existing game rules, virtual
effects, serial lookups and cast. No replacement base damage.

Port guards missing actor in original unsafe group branch, and
explicitly blocks uncertain callbacks, with existing health ack.
Positive-amount comparison handles unordered by skipping. No
original binary bytes copied. Native regression tests early gates,
actual source attachment then native poison inside query, wrong
cast/null source, drown/burn, mutable copy isolation, external
adjustment/self bypass and failure. Full engine radius-effect delivery
and reentrant periodic poison damage wiring remain host work.
