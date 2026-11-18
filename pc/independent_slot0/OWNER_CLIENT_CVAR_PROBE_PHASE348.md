# Phase 348: bounded native client console registration

Actual Phase 344 owner run returned 1920x1080 and verified Gaussian
attachment (0x53494E53), preserving both original DLLs. Phase 347
recovered 533 original client console declaration sites and its
client-specific registration accessor. This diagnostic extends only
the authenticated startup prefix; no successful VClient011 Init
or rendering is claimed.

The SHA-verified original client contains the following selected
three-argument ConVar declarations (constructor RVA 0x121640).
All have flags zero, no bounds, help or change callback.

| Name | Default | Call RVA | Object RVA |
| --- | --- | --- | --- |
| cl_starfield_distance | 256.0 | 0x26F031 | 0x37B108 |
| cl_starfield_diameter | 128.0 | 0x26F001 | 0x37B0C0 |
| cl_show_bloodspray | 1 | 0x268E01 | 0x361978 |

The SHA-verified owner server has none of the exact NUL-terminated
names; this does not exclude live engine/plugin collisions.
The metadata inspector verifies selection against the original
candidate set. The selected descending call order models prepend
order for this subset only, not the full original static order.

The client-only terminal diagnostic captures VEngineCvar002 and
requires every earlier sound, material, screen and Gaussian gate
to pass. It preflights all name lookups and the bounded command list
before creating three owned native objects. It implements original
client accessor 0xFE000: AddFlags(8), detach next, register via slot
zero, mark registered. Unlike the server it does not query or write
command-line overrides or replicated defaults. Unexpected root
relinking, mutation, or lookup mismatch is refused. Final verification
checks all three objects again. If already registered, the objects
remain alive until process exit; there is no engine unregister or
guessed rollback. The diagnostic terminates the process, never
continues into client object, particle, or VGUI initialization.

EnableClientCvarProbe requires EnableClientGaussianProbe and all
other Client-only prerequisites. Exit 0x53494E55 means verified 3/3
including final consistency; 0x53494E56 means refusal. The classifier
requires exact ordered per-object evidence and a valid earlier
Gaussian prefix. Native distributed DLL tests exercise all-success,
preflight existing variable/command conflicts, partial relink,
late first-value mutation, and refusal after failed Gaussian attach.
The original engine has now verified the three-variable probe (see below).
No original game binary or implementation body committed.

## Actual owner engine verification (2026-11-18)

Owner-provided Phase 348 report from the CI bundle built at merge commit
`8abb78cd1163479e66537cb40c1f1647ad184a97` (PR #412's initial build).
The reconstructed client was the only staged DLL; the original server
was retained.

Observed results (sanitized; no local paths, session identifiers or
commercial binaries published):

- VClient011 accepted and Init entered; all 21 required interfaces found.
- Sound Connect succeeded, optional material stub and required hardware
  configuration acquired, screen query returned 1920:1080 and Gaussian
  attachment verified.
- Per-variable registration verified for cl_starfield_distance,
  cl_starfield_diameter and cl_show_bloodspray in that order.
- Final client-cvar-probe-result: verified:3/3.
- Actual process exit: 0x53494E55
  (INDEPENDENT_CLIENT_CVAR_VERIFIED_DIAGNOSTIC_STOP).
- Client original restored and retained server unchanged; restoration
  VERIFIED_COMPLETE.
- Classifier: CLIENT_CVAR_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF.

The module snapshot did not observe client.dll, but the session trace
contains explicit client factory and Init entry plus the terminal result.
This is owner-observed retail registration evidence, not independent
execution of the entire original Init. It does not verify the remaining
console catalog, deferred object list, particle allocator, VGUI or rendering.

Next gate: integrate an explicitly bounded deferred-object initialization
diagnostic with the verified console prefix; establish authentic required
object constructors and ownership before claiming retail parity.
