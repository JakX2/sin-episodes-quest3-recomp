# Owned client startup diagnostic after retail-verified Phase 348

Phase 348 owner result is recorded in OWNER_CLIENT_CVAR_PROBE_PHASE348.md.
This explicitly opt-in terminal probe exercises one independently owned
deferred object (the concrete particle pool backend), then the existing
48-byte bucket boundary and a 2048-record trial pool with 96-byte records.
It verifies allocation, isolation, reuse, stale-handle rejection, and
cleanup of both the bucket and deferred object in the actual built DLL.
The test fixture loads that DLL with synthetic engine interfaces and
checks that failed console or Gaussian registration prevents entry.

This is an independent owned lifecycle test after a real console gate,
NOT proof of the original deferred-object registry population, original
particle ABI/capacity, VGUI, rendering or a successful Init return.
No original code bodies or binaries are included.

Owner switch (only after CI passes): EnableClientOwnedStartupProbe, in
addition to the existing five client switches. Success 0x53494E57,
refusal 0x53494E58. The existing Phase 348 mode is unchanged by default.

## Actual owner runtime verification (2026-11-18)

The owner ran the CI-built diagnostic bundle at merge commit
`18fde9fd41a199de5d11dedc0b9a5d7cfcad8b06`, corresponding to
PR #413 head `26f829f81677447eb4f729084a3c51ff82c9a50b`.
Only the independent client was staged; the original server was retained.

Observed ordered checkpoints (sanitized):
- VClient011 factory accepted; Init entered; all 21 initial required interfaces found.
- Sound connected, material configuration acquired, screen 1920:1080, Gaussian attached.
- All three client console variables registered and verified again.
- owned-startup-entered: independent-only
- owned-startup-objects-ready: 1
- owned-startup-pool-ready: 2048x96
- owned-startup-pool-verified: allocate-reuse-release
- owned-startup-cleanup-verified: pool-objects
- owned-startup-probe-result: verified
- owned-startup-terminal-stop: independent-only
- Process exit: 0x53494E57 (1397313111).
- Classifier: CLIENT_OWNED_STARTUP_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF.
- Original client restored, original server unchanged; VERIFIED_COMPLETE.

This is owner-observed execution of the independent owned lifecycle
inside the actual game process, not original particle allocator parity.
The original registry population and VGUI helpers remain unresolved.
No rendering, simulation, or successful VClient011 Init return is claimed.
