# Next 2006 VGUI startup instruction: original engine VGui slot 1

Primary evidence: original owner `client.dll` SHA-256
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`,
retrieved privately from the owner's Drive `bin.zip`. No original code or
DLL is committed.

Read-only PE32 audit:
`python research/tools/audit_sin_client_engine_vgui_slot1.py <client.dll>`

Exact original facts, verified by five bounded-region SHA checks:

1. During `VClient011::Init`, a request for `VEngineVGui001` stores the
   returned pointer at original global `0x2433F6B0`.
2. Original VGUI setup at `0x241EDFB0` obtains the exact
   `VGUI_InputInternal001` and `VGUI_System010` interfaces, then performs
   a one-time registration with the string `CHudTextureHandle`.
   Its helper `0x2422ECD0` is a genuine one-instruction no-op.
3. The final helper `0x2422ECF0` reads the saved
   `VEngineVGui001` pointer, loads its vtable, invokes virtual **slot 1**
   (offset +4) with integer argument **2**, then returns the 32-bit result.
   `0x241EDFB0` converts that result into a boolean. No other GUI
   method or complete Init semantics is inferred from this call.

Prior retail checkpoints: owner verified 0x53494E55 (console), 0x53494E57
(owned particle lifecycle), 0x53494E59 (GUI factory), 0x53494E5B
(read-only GUI vtable anchors). These are terminal diagnostics, not
successful `VClient011::Init` or gameplay.

The next Windows candidate is a *separately gated* call into the original
engine's already-provided `VEngineVGui001` object using the exactly
observed slot and argument; the result is recorded only as zero/nonzero
without implementing unrelated engine methods.

## Corroboration from the owner's historical Source SDK 2004

The owner's `source-sdk-2004-master.zip` contains
`src_mod/public/ienginevgui.h`. It declares `IEngineVGui` with a virtual
destructor (slot 0), then `vgui::VPANEL GetPanel(VGuiPanel_t type)`
(slot 1). Its `VGuiPanel_t` enumerator `PANEL_CLIENTDLL` has value 2.
The matching `src_mod/cl_dll/vgui_int.cpp` calls
`enginevgui->GetPanel(PANEL_CLIENTDLL)` and separately
`enginevgui->GetPanel(PANEL_TOOLS)` when constructing GUI panels.

**Interpretation:** original SHA-pinned slot1(arg2) is the 2004
`IEngineVGui::GetPanel(PANEL_CLIENTDLL)` ABI and returns an opaque
`vgui::VPANEL` handle. The original binary proves the dispatched slot
and argument; SDK2004 supplies the symbolic method/enum names. We do
not dereference the returned handle or claim GUI panel creation.


## Owner-original engine result: 18 November 2026

This is an actual owner-side retail engine process test, using the
independent PE32 client from PR #426 at CI diagnostic merge commit
`07eda44132ed61049b365a3e708574849b984ccc`.

Owner supplied both the console paste (150 lines) and
`sin-engine-vgui-panel-probe.txt` (UTF-16, 126 log lines).
Both independent sources agree:

- Owner launched a disposable, SHA-pinned original SiN Episodes copy.
  Only `client.dll` was staged; original `server.dll` was unchanged.
- `VClient011` factory accepted; actual Init entered.
- All 21 required engine interfaces found, including
  `VEngineVGui001`; SoundEmitter connected, material hardware
  configuration obtained, screen queried, Gaussian attached and
  three client console variables registered.
- The independent owned object and particle lifecycle reported verified.
- The loaded original `vgui2.dll` provided both VGUI interface
  pointers; input and system anchor slots passed memory inspection.
- `engine-vgui-slot1-entered version=VEngineVGui001:arg2` followed by
  `engine-vgui-slot1-result version=nonzero`.
- Exact exit `0x53494E5D` / decimal `1397313117`, classified
  `INDEPENDENT_CLIENT_ENGINE_VGUI_SLOT1_NONZERO_DIAGNOSTIC_STOP`.
- Classifier:
  `CLIENT_ENGINE_VGUI_SLOT1_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF`.
- `RESTORED_VERIFIED: ...client.dll`,
  `RETAINED_ORIGINAL_UNCHANGED: ...server.dll` and
  `OWNER_INDEPENDENT_RESTORATION: VERIFIED_COMPLETE`.
- Module sampling noted `client.dll` not observed despite 34
  successful polling samples; positive factory and trace evidence
  establishes native DLL execution, but sampling is not exhaustive.

Interpretation: the original engine returned a **nonzero**
`IEngineVGui::GetPanel(PANEL_CLIENTDLL)` VPANEL handle for this
independently built client. The diagnostic deliberately terminated
before a normal `Init` return. This does NOT show that any GUI panels
were constructed/registered, that in-game HUD or rendering works,
that the original one-time `CHudTextureHandle` converter exists, or
that `VClient011::Init` completed.
