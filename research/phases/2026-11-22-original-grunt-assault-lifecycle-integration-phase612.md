# Gameplay Phase612: source-backed Grunt Assault cross-phase lifecycle integration

## Why this integration exists

The owner's 2006 original SiN Episodes: Emergence server/client retail DLL pair has been independently SHA-authenticated throughout Phases535–611. This Phase612 combines three previously source-verified, separately implemented pieces of original Grunt behavior:

1. **Phase606 selector:** walks the real eight-class Grunt registration vector in original order, calls each behavior eligibility virtual+0x18 and owner acceptance+0x77C, then passes the first winner to the original behavior transition system.
2. **Phase610 Assault eligibility:** original complete native function 0x220CEAF0 (255 bytes, already counted in Phase607) with owner gates, serial-checked pair of handles, original finite-time x87 comparisons, byte+0x26 state and condition 0x18 signaling.
3. **Phase611 Assault deactivation:** original vtable+0x20 -> 0x220CEBF0 (105 bytes), plus original linked-owner release helper 0x220CE4A0 (115 bytes), conditionally unlinks only an invalid/stale relationship or one belonging to the departing Grunt owner.

**No new original native method is claimed in Phase612.** This is an independent C++17 integration regression for source-grounded behavior selection, safe lifecycle transitions and ownership cleanup. It does not replace the Source engine's own entity/AI objects or implement a native server.dll ABI.

## Integrated host test

File: tests/test_original_grunt_assault_priority_lifecycle_phase612.cpp

The host implements the three existing independent interfaces:
- sin_compat::grunt_behavior_priority_phase606::Host
- sin_compat::grunt_assault_eligibility_phase610::Host (composes Phase609 serial-handle checks)
- sin_compat::grunt_assault_deactivate_phase611::Host

It uses the real original first-eligible order with only Rappel and Assault eligible in this bounded scenario. Engine-owned entity identity and owner relationship are opaque test tokens rather than copied raw 2006 pointers.

### Cases verified

- With earlier behaviors ineligible, Assault becomes the first eligible candidate and is selected at the actual original registration position **5** (zero-based).
- Selecting the same active Assault again must not rerun lifecycle callbacks: Phase604's original current-pointer setter checks old versus new.
- If Rappel becomes eligible before Assault, **Rappel wins immediately**, replacing active Assault. The host checks the Phase604 lifecycle sequence: new behavior+0x1C, new behavior+0x24, old Assault+0x20 shutdown, then owner notification+0x778.
- On leaving Assault, bytes +0x24/+0x25 clear. Its original linked entity+0x3BC clears if the link resolves to the SAME owner.
- If the entity+0x3BC link is valid but belongs to a DIFFERENT owner, deactivation leaves it unchanged.
- When no behavior is eligible after Assault was active, old Assault receives deactivation/cleanup and the owner receives notification; the active pointer becomes null.
- At the original finite-time **4.0 second boundary** with a zero marker timestamp+0x28, Assault may become ineligible and set byte+0x26=1, so no candidate is chosen.

The test does not invent results for all eight AI predicates; only Rappel is configurable and Assault is sourced from the recovered finite-time model. Other behaviors remain deliberately ineligible in this controlled host scenario.

## Engineering boundaries

- Host callback names are independent port abstractions, not claimed original 2006 C++ symbols.
- Lifecycle notifications are simulated against an event log, not the real Source engine's AI schedule manager.
- Native phase610 exceptional x87/NaN paths are not modeled. Missing/stale second entity pointers are safely refused rather than used as null dereferences.
- Other original behaviors and their detailed scheduling heuristics remain unimplemented; initial pointers and test flags are hand-created host inputs, NOT a real NPC object allocation.
- No user-prioritized generic HL2 physics, models, props, surface properties or rendering work is included.

## Reproduce

The synthetic host code was compiled and run locally with GCC C++17 and flags -Wall -Wextra -Wpedantic -Werror and passed all integrated cases. Root CMake registers sin_original_grunt_assault_lifecycle_phase612_tests for GitHub C++ reference regression. This test links no proprietary original binary.

**Phase612 adds zero native method bodies or bytes. Cumulative Phase601–612 remains 40 exact original method addresses / 11,224 whole-method SHA-authenticated original machine bytes.** This is a practical port-integration test that reduces cross-phase logical drift, not a claim that Windows/Android/Quest 3 now boot a complete NPC gameplay module.
