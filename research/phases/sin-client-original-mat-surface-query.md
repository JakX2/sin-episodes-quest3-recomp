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
