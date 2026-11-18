# Phase 343: retail client screen-size query diagnostic

Actual owner Phase 341 returned 0x53494E4F. The initial 21 factory
requests, sound Connect, absence of -textmode, optional material stub
availability and required hardware configuration were observed in the
retail engine. The original client was restored and the original
server remained unchanged. The exact observed trace is preserved in
the classifier regression. This does not establish rendering.

Phase 342 passed all four workflows. Its owned ScreenState adapter
is now compiled in the distributed independent client. The original
SHA-authenticated client helper at RVA 0xFFAB0 invokes the acquired
VEngineClient012 slot 5, passing height and width addresses according
to the original x86 calling convention. The implementation calls the
same slot with independent zero-initialized width/height storage and
records the actual signed integers returned, including zero.

The new terminal diagnostic requires EnableClientScreenProbe in
addition to EnableClientMaterialProbe and EnableClientSoundProbe
in explicit Client-only mode on a disposable copy. It only enters
the screen query following successful hardware configuration.
Exit 0x53494E51 means that the callback returned, not that any drawing
occurred or even that the dimensions are nonzero. Exit 0x53494E52
means the necessary ABI input was missing. The prior material-only
and sound-only gates remain unchanged. Classifier requires ordered
material acquisition, query entry, signed dimensions, result and
terminal stop; refuses a bare success result.

The actual distributed client native child fixture exercises 1920x1080
and 0x0 output references, validates calling order, and verifies refusal
to call the screen slot after rejected sound or unavailable hardware.
The Phase 342 standalone fixture also covers missing slots. The next
original operation is the Gaussian stream attachment (0xFE388).
No Gaussian provider call is enabled by this phase. Further original
console registration, particle system / VGUI and game systems
remain incomplete. No original binary or code body is committed.
