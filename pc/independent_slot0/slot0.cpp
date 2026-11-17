// Minimal independent Win32 PE32 factory + slot-zero boundary. NOT a playable game DLL.
// Original SHA-authenticated Phase 285: client slot0 ret 12, server slot0 ret 16.
// Source SDK 2004 naming is a hypothesis, not a verified full retail ABI.
// Deliberately fail closed until actual engine factory roles and remaining slots are verified.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstring>
#if !defined(_M_IX86)
#error Win32 x86 only
#endif
namespace {
using Factory = void* (__cdecl *)(const char*, int*);
#if defined(SIN_ROLE_client)
constexpr char kName[] = "VClient011";
class Boundary {
public:
    virtual bool __thiscall Init(Factory app, Factory physics, Factory filesystem) noexcept {
        // Do not dereference unverified factories or claim initialization.
        seen_ = app != nullptr && physics != nullptr && filesystem != nullptr;
        return false;
    }
    bool seen_ = false;
};
#elif defined(SIN_ROLE_server)
constexpr char kName[] = "ServerGameDLL003";
class Boundary {
public:
    virtual bool __thiscall DLLInit(Factory engine, Factory physics, Factory filesystem, void* globals) noexcept {
        seen_ = engine != nullptr && physics != nullptr && filesystem != nullptr && globals != nullptr;
        return false;
    }
    bool seen_ = false;
};
#else
#error Choose one DLL role
#endif
Boundary instance;
}
extern "C" __declspec(dllexport) void* __cdecl CreateInterface(const char* name, int* status) noexcept {
    const bool found = name && std::strcmp(name, kName) == 0;
    if (status) *status = found ? 0 : 1;
    return found ? static_cast<void*>(&instance) : nullptr;
}
