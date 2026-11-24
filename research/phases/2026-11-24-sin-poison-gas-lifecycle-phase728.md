# Phase 728: original poison gas gameplay lifecycle

SHA-matching original server. CPoisonGas (env_poison_gas), factory
222fe490 allocates 0x3bc, vtable 2246e81c, constructor 222fe0b0.
Leaf offsets: +394 enabled byte=1, +398 type=0, +39c amount=5,
+3a0 radius=300, +3a4 duration=20, +3a8 spawnrate=15,
+3ac last=0, +3b0 raw deadline=0, +3b4 visual pointer=null,
+3b8 sound byte=1. Factory helper 222fe1b0 selects default values
for negative arguments; zero is retained. Spawn 222ff590 clamps
type outside [0,3] to 0, base/precaching/setup services, sets
amount=5, starts if enabled. Port owns only post-setup leaf decisions.

Original four 20-byte variant records at 22529fc8:
flags 10000/400000/2/3 (hex), start color b43cea32/ffe58619/
ff11a4ed/ff808080 and end ff00ff00/ffe54f19/ff2864ef/ff808080;
gas.poison_spawn/loop, gas.antigen_spawn/loop, gas.poison_spawn/loop,
gas.inert_spawn/loop. Start 222feaf0 skips if child exists,
installs think 222fe510, schedules now+RandomFloat(0,.5),
samples clock again stores now+1, creates visual, sets pointer and
configures visual if present, emits spawn sound if sound byte.
Visual configuration is a borrowed engine service; no particle or
renderer reconstruction claimed. Enable 222fee20 checks flag,
sets and calls start. Disable 222fe190 clears flag and calls
222fe130 stop. Stop only acts if visual pointer nonnull: remove,
clear pointer, clear think, schedule -1, stop variant loop sound.
Kill 222fe350 calls stop then removes gas itself.

Think 222fe510: inert type 3 returns without scheduling.
If last==0 sets last=now. Strict elapsed >.25 gate; rounds elapsed
to f32, calls radius-effect helper 222a5e10 with amount*elapsed and
variant flags. Host owns existing radius query/effect. If duration
positive subtracts elapsed; <=0 stops/removes gas, no reschedule.
If still positive and antigen type 1 with visual child, stops visual,
reinstalls think and schedules random delay, then ordinary final
schedule overwrites it. Nonpositive duration instead emits loop
sound if enabled. Updated last on elapsed branch only. Finally
schedule freshly sampled now+.25, including equality skip. No
enable gate on the think method itself. Finite guards and callback
failure blocks are port differences. Test covers four types,
factory/spawn defaults, strict timing, duration removal, antigen
special path, indefinite loop, visual allocation failure, disable
and kill. No proprietary binary bytes or physics work.

Also recovered CMutagenBarrel constructor 22300770, shared 0x6a0
CPropContainer: flags 4, model models/containers/mutagen_barrel.mdl,
duration 12, count 5, raw parameter 1, loop gas.poison_loop.
Extends Phase727 enum without changing prior values or API size.
Original damage-triggered leak creation and player poison timer
integration remain separate.
