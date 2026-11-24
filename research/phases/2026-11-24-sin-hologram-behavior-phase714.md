# Phase 714: original npc_hologram behavior

Fresh SHA-matching binary inspection. Server class CNPC_Hologram primary
vtable 0x22496734, allocation size 0x101c. DT_NPC_Hologram fields state
+0x1014, float deadline +0x1018. Client size 0xd88, fields +0xd7c,
+0xd80. Datamap 0x22535804 and input names HologramOn / HologramOff
(0x2249669c / 0x2249667c) route via 0x22361110 / 0x22361120
to virtuals +0x7c4/+0x7c8; wrappers 0x22361790/0x223617a0.

| Original method | Observed behavior |
|---|---|
| 0x22361480 | base spawn then state=0, deadline=0, then separate collision/base setup |
| 0x22361540 | state=1, current time+2, raw +0x3b0=0, emit On, stop Loop |
| 0x22361620 | state=2, emit Loop (no timer update) |
| 0x22361670 | state=3, current time+1, raw +0x3b0=0, stop Loop, emit Off |
| 0x223617b0 | only if state=2: state=4, current time+argument, +0x3b0=1, +0x1c=13 |
| 0x223618a0 | state=2, current time, +0x3b0=0, +0x1c=0 |
| 0x22361980 | if deadline < now, state=2, emit Loop |
| 0x223619e0 | if deadline < now, state=0 |
| 0x22361a80 | base think pointer +8 first, then state 1/3/4 checks |
| 0x223611a0 | base Activate first, emit Loop only if state=2 |

Clock constants 0x224965e4=2.0f, 0x224965e8=1.0f.
Pulse wrappers 0x22361a40 and 0x22361a50 pass 0.5f and 0.3f.
Sound pointer globals 0x22535764/68/6c resolve to Hologram.On,
Hologram.Off, Hologram.Loop. Emit helper 0x221b77b0; stop 0x221b6a80.
Tracked changes mark dirty before store; unchanged fields skip dirty,
but repeated On/Off/StartLoop still execute sound operations. Original
time comparison uses x87 extended intermediate; port double matches
normal game clocks, not an exact claim at extreme rounding boundaries.
Unordered deadline does not trigger Think. No automatic repair/replay
after failed host action: explicit reconciliation required.

Client 0x2421cef0: states other than 1/3 return state!=0. State 1
progress=1-(deadline-now)*0.5, state 3 progress=1-(deadline-now).
19 float thresholds at 0x242b7478: .10,.15,.25,.26,.27,.28,.30,.39,
.40,.45,.46,.50,.51,.60,.70,.85,.90,.95,.99. Original per-object
cursor +0xd74 advances at most one per invocation on >= threshold.
Global visibility byte 0x2431fd3c toggles, shared across holograms.
No reset observed in the examined method; constructor initializes cursor.
Port host owns shared visibility state, refuses negative cursor.

Server native method dispatch operates on actual original-layout fields,
with explicitly supplied dirty, clock, sound and base hooks. Reset is
only the state/timer portion after base spawn; no new collision setup.
Client native visibility consumes Phase713's actual received fields.
Paired fixture uses the real send/recv callbacks for state/timer, tests
exact audio/dirty ordering, strict deadline, pulse gating and second
clock read, shared flicker across objects, single threshold advancement,
partial failure/reconciliation, repeated inputs and malformed cursor.
Rendering, sound delivery, map dispatch and full retail class creation
remain host dependencies; no physics/model work or copied original code.
