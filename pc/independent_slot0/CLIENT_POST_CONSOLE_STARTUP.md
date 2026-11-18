# Combined client post-console startup fixture

Stacked on PR #409 (all four CI workflows green when checked). Uses the actual
native independently implemented ClientInitializers and ParticleBucketOwner
components and the earlier VGUI startup reference instead of disconnected mocks.

## Source evidence

Phase 87, research/phases/sin-original-client-server-subsystem-startup.md:
- 0x240FE39B calls the original deferred object initializer.
- 0x240FE3B9 calls particle initialization.
- 0x240FE3C7 calls VGUI initialization.
- VGUI requires CLIENT module check, VGUI_InputInternal001,
  VGUI_System010, one-time registration, and two helper calls.
- The original VGUI helper meanings and teardown contract are not yet known.

The native fixture now verifies one successful ordered path and seven distinct
failure gates: required object, allocation, construction, module, input,
system and final VGUI helper. It also rejects repeated or reentrant startup
and verifies that an externally initialized object list is not freed.

The reverse-stage cleanup and VGUI ReleaseFixture callback are independent
fixture ownership rules, not reconstructed original shutdown. No original
runtime pointer is invoked, no actual Source VGUI factory or rendering is
connected, and no successful VClient011 Init is claimed. The retail Phase
348 cvar probe remains the next owner runtime test.
