// Test-only loaded vgui2.dll lacking the public factory export.
// The real Windows game DLL is never packaged or modified by this fixture.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
extern "C" __declspec(dllexport) int __cdecl VGuiMissingFactorySentinel() noexcept {return 1;}
