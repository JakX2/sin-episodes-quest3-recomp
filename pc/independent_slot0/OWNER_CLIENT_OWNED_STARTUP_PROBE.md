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
