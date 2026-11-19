# Original SiN GUI animation texture converter: independently owned core

Original 2006 client.dll SHA256:
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.
Original one-time helper 0x24009340 (101 bytes) is already SHA-pinned
in research/tools/audit_sin_client_engine_vgui_slot1.py:
a0851e6f97bc087734723394de37dfb925db41c1764087f4c9e94ede1a1b384e.

Corroboration: Source SDK 2004 `cl_dll/vgui_int.cpp`, published by
Valve's Source SDK archives. It declares CHudTextureHandleProperty as
vgui::IPanelAnimationPropertyConverter, with three original member
operations: GetData, SetData and InitFromDefault. These use the
PanelAnimationMapEntry lookup callback to obtain the handle, convert
between HUD icon names and CHudTextureHandle pointers via gHUD.GetIcon,
and represent missing/empty texture names as a null icon handle.
`VGui_OneTimeInit` sets a process-static initialized flag before it
calls `vgui::Panel::AddPropertyConverter("CHudTextureHandle",...)`.

Code in `client_hud_texture_converter_core.h` reconstructs these
behaviors with injected callbacks representing original panel, entry,
KeyValues, HUD icon lookups, and property registry. The independent
OneTimeOwner owns its converter and retains it for its own lifetime,
sets the registration guard before invoking the registrar, and blocks
duplicate/reentrant registration. It does NOT pretend to know the
original SiN `PanelAnimationMapEntry` object layout or the static
`vgui::Panel::AddPropertyConverter` 2006 Windows linkage.

The native test covers all three conversion operations, empty/unknown
texture names, callback failures and reentrant one-time registration.
This core is currently a **build-tested** reconstruction component,
not a claim that the original 2006 VGUI animation registry was
modified in a running game. The genuine Windows owner checkpoint is
0x53494E61 (MatSystemSurface003 interface acquired). Further work must
authenticate live Panel::AddPropertyConverter ABI and lifecycle before
invoking it, then reconstruct root-panel construction and later Init.
