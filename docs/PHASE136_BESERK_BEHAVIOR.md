# Phase 136: embedded CAI_BeserkBehavior state reconstruction

## Evidence provenance

Based on the preserved Phase 18 static-analysis report for the owner-supplied original 2006 Windows x86 server.dll, SHA-256:

568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

The original DLL was not re-materialized or re-disassembled in this pass. No original code was executed. Addresses and offsets are specific to that PE32 build.

## Embedded object and fields

The original CNPC_GruntBase constructor at 0x22360D80 constructs an embedded RTTI-confirmed CAI_BeserkBehavior at grunt +0xDA0.

- Behavior +0x11 (grunt +0xDB1): active byte, initialized to zero.
- Behavior +0x14 (grunt +0xDB4): current-time deadline, registered as m_fBeserkTime.
- The embedded object's constructor initializes the deadline from gpGlobals->curtime.
- The original spelling in the RTTI and schedule strings is Beserk.

## Activation and expiration

Activation-like routine 0x22391D00:
1. Stores gpGlobals->curtime + 10.0 into the deadline.
2. If previously inactive, sets shared condition 0x22 (COND_SCHEDULE_DONE).
3. Calls an unresolved NPC operation with argument 3.
4. May write 3 to owning NPC field +0x858; the exact guard and semantics are unresolved.
5. Sets active byte +0x11 to one.

Expiration routine 0x22391CB0:
- Returns when inactive.
- Clears active only when current time is strictly greater than m_fBeserkTime.
- Equality does not expire the behavior.

The report found no static direct call or absolute function pointer to 0x22391D00. Therefore the activation trigger and reachability remain unverified. This code models the observed state transition only; it does not automatically activate the behavior in gameplay.

## Separate schedule byte

A different method at 0x22392700 checks behavior +0x18 and returns SCHED_BESERK ID 0x171 when set. This is not the active byte at +0x11. Its setter and activation conditions remain unknown, so the host supplies this separate request flag.

## Implementation

- Adds an inactive-by-default portable state object with the verified ten-second deadline.
- Repeats activation by refreshing the deadline without replaying first-activation side effects.
- Exposes the saved deadline field but does not invent a save format for the active byte.
- Keeps the unresolved NPC calls behind explicit host callbacks.
- Adds overloads so the Phase 135 relationship and condition-gather code consumes the actual behavior object's active state.
- Keeps the separate +0x18 schedule gate independent from active state.

## Verification

Regression tests cover constructor state, first activation callback order, repeated activation, strict expiration boundary, inactive update, separate schedule flag, and integration with the relationship/condition model.

Desktop CTest, Android NDK ARM64 compilation, and Android Bionic x86_64 mock-host execution are separate checks. None is SourceVR or physical Quest 3 gameplay.

## Remaining work

- Find the real caller/trigger of 0x22391D00.
- Decode behavior +0x18 setter and owning-NPC schedule integration.
- Identify the unresolved activation NPC routine and +0x858 state guard.
- Connect the embedded behavior to real engine NPC lifetime, save/restore and campaign encounters.

No proprietary DLL, APK, original ELF, BSP, model, map, script or game asset is included.
