# Original SiN client MatSystemSurface003 post-VGUI boundary

Source evidence: original owner 2006 client.dll, SHA256
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.
Original virtual callsite 0x240FE3D7 - 0x240FE3F5 inclusive
is 31 bytes, SHA256
d9f987e6679cac9e491de40a996293b9fada35e5047bc5561a588f1376281cb0.

After original VGui_Startup returns true, original VClient011::Init:
- Calls 0x24001040, a tiny accessor returning the VGUI_Surface028
  interface pointer cached by original 0x240010D0 VGUI controls setup.
- Adjusts the returned interface pointer by +4 bytes to its
  SECONDARY IAppSystem base class subobject.
- Reads that subobject's vtable and dispatches **virtual slot 2**,
  passing string literal "MatSystemSurface003".
- Saves the returned interface pointer, refusing Init if null.

Source SDK 2004 public/vgui/ISurface.h confirms ISurface inherits
IBaseInterface and IAppSystem; appframework/IAppSystem.h confirms
IAppSystem slot 2 = QueryInterface(const char*). That identifies
the original call as IAppSystem::QueryInterface("MatSystemSurface003"),
NOT a direct CreateInterface call on a DLL.

Original 0x24001040 accessor is 6 bytes, SHA256
e5099f1323cd1754c2a4c0968d0165ffe11561ba66a6540ee5717ddc3e58dd03.
The version text at original 0x24296900 is authenticated.

The independent module implements only this exact ABI call, with
VirtualQuery checks and loaded-module ownership of the method pointer.
It does not construct an ISurface, render panels or invent a gameplay
startup return. The preceding owner-confirmed milestone is original
IVGui.Init + GetPanel(PANEL_CLIENTDLL), exit 0x53494E5F.

## Actual original Windows owner result, 19 November 2026

The owner ran the PR #429 independently built PE32 `client.dll` in a
disposable original SiN Episodes: Emergence Windows installation, using the
reversible launcher and bundled scripts. Two uploaded outputs agree:
`Pasted text(6).txt` and `sin-mat-surface-probe.txt`.
Bundle tested: merge commit `98e5fc0fb1b49102d2a145a2b9b8c1dff17f642c`
(PR #429 feature head `b378cd122c8650a507bf9c0c7a08ed94ae14f92a`).

- Original engine accepted `VClient011` factory and invoked `Init`.
- All 21 original engine factory dependencies returned non-null; sound
  Connect, materials, 1920x1080 screen query, Gaussian attachment,
  native 3/3 console vars and the independent owned-object/particle
  preflight completed with their earlier success trace markers.
- Original `vgui2.dll` accepted `VGUI_InputInternal001` and
  `VGUI_System010`; vtable address checks passed.
- All eight `VGui_InitInterfacesList` names were found, followed by
  `VGUI_ivgui006::Init` returning true.
- `IEngineVGui::GetPanel(PANEL_CLIENTDLL)` returned nonzero.
- **New original engine operation:** `VGUI_Surface028` secondary
  `IAppSystem::QueryInterface("MatSystemSurface003")` returned non-null.
  Ordered actual engine trace:
  `mat-surface-query-entered VGUI_Surface028:IAppSystem+4`,
  `mat-surface-query-result found`,
  `mat-surface-query-terminal-stop original-abi`.
- Exact exit `0x53494E61` (decimal 1397313121), diagnostic
  `INDEPENDENT_CLIENT_MAT_SURFACE_QUERY_VERIFIED_DIAGNOSTIC_STOP`.
  Owner classifier:
  `CLIENT_MAT_SURFACE_QUERY_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF`.
- `RESTORED_VERIFIED` original `client.dll`,
  `RETAINED_ORIGINAL_UNCHANGED` original `server.dll`,
  `OWNER_INDEPENDENT_RESTORATION: VERIFIED_COMPLETE`.
- The module sampler reported `client.dll` not observed in 15 polling
  samples, despite positive factory, ordered trace, method invocation and
  terminal exit proof; module sampling is not a complete temporal survey.

This authenticates one further real virtual ABI interoperability step.
It does **not** prove that `CHudTextureHandle` converter registration,
client HUD/root-panel construction, full client `Init` return, map load,
or gameplay is functional. The DLL was intentionally stopped at this
boundary; no retail engine startup beyond this point was attempted.
