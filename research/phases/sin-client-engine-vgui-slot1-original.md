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
