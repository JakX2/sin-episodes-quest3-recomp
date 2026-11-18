# Original SiN client VGUI controls startup ABI

Original original-owner 2006 client.dll SHA-256:
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

The 586-byte original VGui_InitInterfacesList function (VA 0x240010D0)
has SHA256 73c405920189d4c1c0eeefb994849544d8ff41cf6fc20b79d420e19d52fae64b.
It was reviewed read-only from the owner's original; no original code is committed.

It accepts module name CLIENT and an array containing one app-system
factory. It searches in this exact order for:
1. VGUI_ivgui006
2. VGUI_Panel009
3. VGUI_Surface028
4. VGUI_Scheme010
5. VGUI_System010
6. VGUI_Input005
7. VGUI_Localize003
8. VFileSystem012

The original searches all eight, even if a request fails, and then
invokes VGUI_ivgui006 virtual slot 1 (vtable +4), with factory array
pointer and count=1, if that primary interface exists. It ignores the
virtual call return and reports success only if all eight interfaces
were available. Source SDK 2004 vgui2/controls/controls.cpp independently
calls this method IVGui::Init(CreateInterfaceFn*, int).
Original VGui_InitInterfacesList also checks the controls allocator
override and sets locale categories; our initial independent boundary
does not reproduce those prerequisites or full GUI integration.

The original VGui_Startup then obtains VGUI_InputInternal001 and
VGUI_System010, one-time registers CHudTextureHandle property converter,
and calls IEngineVGui::GetPanel(PANEL_CLIENTDLL). The owner has already
verified the latter original-engine call returned a nonzero handle,
using the independent client at diagnostic 0x53494E5D.

Next: reconstruct these eight queries and IVGui.Init under a separate
opt-in gate, verify with native x86 synthetic fixtures, and only
then attempt the retail game startup sequence. This is not proof of a
normal VClient011 Init return, game HUD, or gameplay.

## Owner-engine verification: 2026-11-18 (PR #428)

The owner ran the independently compiled Windows x86 client from
bundle merge commit `6a2b47ba4924f01f1fedecf4cb1cd714ded5df74`
(PR #428 head `9abeb8e96570fea659721bd74e021829072eae95`) in a disposable copy of the actual 2006
SiN Episodes engine. Uploaded `sin-vgui-controls-probe.txt` and
matching pasted console log agree on the ordered result:

- `VClient011` factory was accepted, then real Init entered.
- All 21 required engine factory interfaces were found; sound, material,
  screen, Gaussian, console (3/3) and independent deferred/pool stages
  reached their existing accepted checkpoints.
- Original loaded `vgui2.dll` supplied `VGUI_InputInternal001`,
  `VGUI_System010` and the checked vtable anchors.
- At `vgui-controls-bootstrap-entered CLIENT:factory-count-1`,
  the engine factory returned non-null pointers for **all eight** named
  VGUI controls interfaces in the expected original order.
- `vgui-controls-init-entered VGUI_ivgui006:factory-count-1`,
  `vgui-controls-init-returned true`,
  `vgui-controls-bootstrap-result init-true`.
- Subsequent `VEngineVGui001` virtual slot1 (SDK2004
  `GetPanel(PANEL_CLIENTDLL)`) returned **nonzero** again.
- Exact diagnostic exit `0x53494E5F` (decimal 1397313119) was
  intentional; classifier:
  `CLIENT_VGUI_CONTROLS_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF`.
- `RESTORED_VERIFIED` original client and
  `RETAINED_ORIGINAL_UNCHANGED` original server:
  `OWNER_INDEPENDENT_RESTORATION: VERIFIED_COMPLETE`.

This is original retail-engine execution of our new client bootstrap
ABI, not just synthetic CI tests. It proves the return of the
`IVGui::Init` call, **not** complete original VGUI startup equivalence,
`CHudTextureHandle` property converter setup, root HUD panel creation,
a normal `VClient011::Init` return, or a playable game.
The sampler reported no observed client module in 15 polling samples
despite the factory, trace, and exit evidence; this is not treated
as an exhaustive module survey.
