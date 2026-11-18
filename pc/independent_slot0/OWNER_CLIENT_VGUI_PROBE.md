# Original Windows VGUI acquisition probe

Baseline: owner-confirmed 0x53494E57, PR #413. Prior Phase 87 confirms the
two interface names and original CLIENT module check, and Phase 98 verifies
the original vgui2.dll provides the exact factory, with 71 input and 33
system methods. This new probe queries the existing loaded Windows provider
using its CreateInterface export; it does not reproduce the original CLIENT
module helper or any of the later registration calls, and does not load a
module if absent. It does not dereference the returned interfaces. It
requires successful owned lifecycle cleanup and separate opt-in.

Native actual-DLL child tests use a fixture-only vgui2.dll and cover
success, missing module, missing CreateInterface export, missing input,
missing system, aliased returns and
prior console refusal. The original module is not distributed. Success
0x53494E59, refusal 0x53494E5A. Both are terminal diagnostic stops, not
successful Init or gameplay. No original code bodies committed.

## Actual owner-run verification, 2026-11-18

Owner provided the Phase 414 CLI output and session trace for the CI
bundle at merge commit `c7c181de019ec4f92d0b703fe6d7c91a5bc61fe3`
(PR #414 head `627aed353fbf83a0cf53969654b1e3f74a63add4`). This is original-game process execution,
not a synthetic trace, and the report is sanitized below.

- Explicit Client-only diagnostic; independent client staged, original
  server remained installed and SHA-verified.
- VClient011 factory accepted and its Init entry observed.
- All 21 initial required engine factory requests found, sound connected,
  material hardware configuration acquired, 1920:1080 screen size and
  Gaussian attachment verified.
- All three original-metadata client variables registered and verified.
- Owned independent deferred object and 2048 x 96-byte particle pool
  lifecycle returned verified, with cleanup verified.
- The already loaded original `vgui2.dll` exposed `CreateInterface`.
- `VGUI_InputInternal001` and `VGUI_System010` both returned non-null,
  distinct pointers; ordered trace `vgui-probe-result=acquired`.
- Exit code: `0x53494E59` / 1397313113.
- Verdict: `CLIENT_VGUI_FACTORY_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF`.
- Original client restored, original server retained unchanged, with
  `OWNER_INDEPENDENT_RESTORATION: VERIFIED_COMPLETE`.

This confirms actual legacy VGUI factory interoperability at this
limited acquisition boundary. It does NOT verify the returned vtable
layouts, module ownership of virtual methods, CLIENT registration,
the one-time original helper, the two subsequent GUI helpers, rendering,
or a successful return from VClient011::Init.
