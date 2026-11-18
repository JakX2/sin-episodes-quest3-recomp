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
