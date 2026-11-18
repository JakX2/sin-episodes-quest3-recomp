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
success, missing module, missing input, missing system, aliased returns and
prior console refusal. The original module is not distributed. Success
0x53494E59, refusal 0x53494E5A. Both are terminal diagnostic stops, not
successful Init or gameplay. No original code bodies committed.
