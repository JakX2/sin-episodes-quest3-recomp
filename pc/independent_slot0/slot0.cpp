// Independently authored Win32 PE32 interface-boundary diagnostic.
// NOT a playable game: only first two virtual slots are implemented.
// SDK 2004 method type signatures are hypotheses for the original SiN DLLs.
// Phase 285 proved x86 stack cleanup shapes of slot 0, NOT full method ABI.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstring>
#include <type_traits>

#if !defined(_M_IX86)
#error Native MSVC x86 PE32 only
#endif

// Opaque: the diagnostic does not dereference engine global structures.
struct CGlobalVarsBase;
struct CGlobalVars;

namespace {
using Factory = void* (__cdecl *)(const char*, int*);

#if defined(SIN_ROLE_client)
constexpr char kName[] = "VClient011";

class Boundary final {
public:
    // Source SDK 2004 IBaseClientDLL::Init, *candidate* SiN slot 0.
    // The third parameter is a globals DATA pointer, not a third factory.
    virtual int __thiscall Init(
        Factory appSystemFactory, Factory physicsFactory,
        CGlobalVarsBase* globals) noexcept {
        seen_ = appSystemFactory != nullptr &&
                physicsFactory != nullptr && globals != nullptr;
        // Never claim successful engine initialization.
        return 0;
    }

    // SDK2004 candidate slot 1; safe to call after a failed Init.
    virtual void __thiscall Shutdown() noexcept {
        seen_ = false;
    }

    bool seen_ = false;
};

using ExpectedInit = int (__thiscall Boundary::*)(
    Factory, Factory, CGlobalVarsBase*) noexcept;
using ExpectedShutdown = void (__thiscall Boundary::*)() noexcept;
static_assert(std::is_same<decltype(&Boundary::Init), ExpectedInit>::value,
              "Client Init must use SDK 2004 x86 calling convention and globals pointer");
static_assert(std::is_same<decltype(&Boundary::Shutdown), ExpectedShutdown>::value,
              "Client Shutdown prototype differs from SDK 2004");

#elif defined(SIN_ROLE_server)
constexpr char kName[] = "ServerGameDLL003";

class Boundary final {
public:
    // Source SDK 2004 IServerGameDLL::DLLInit, *candidate* SiN slot 0.
    virtual bool __thiscall DLLInit(
        Factory engineFactory, Factory physicsFactory,
        Factory fileSystemFactory, CGlobalVars* globals) noexcept {
        seen_ = engineFactory != nullptr && physicsFactory != nullptr &&
                fileSystemFactory != nullptr && globals != nullptr;
        return false;
    }

    // SDK2004 candidate slot 1; do not start the game after failed DLLInit.
    virtual bool __thiscall GameInit() noexcept {
        return false;
    }

    bool seen_ = false;
};

using ExpectedDllInit = bool (__thiscall Boundary::*)(
    Factory, Factory, Factory, CGlobalVars*) noexcept;
using ExpectedGameInit = bool (__thiscall Boundary::*)() noexcept;
static_assert(std::is_same<decltype(&Boundary::DLLInit), ExpectedDllInit>::value,
              "Server DLLInit must use SDK2004 method prototype");
static_assert(std::is_same<decltype(&Boundary::GameInit), ExpectedGameInit>::value,
              "Server GameInit prototype differs from SDK2004");
#else
#error Choose exactly one DLL role
#endif

Boundary instance;
}

extern "C" __declspec(dllexport) void* __cdecl CreateInterface(
    const char* name, int* status) noexcept {
    const bool found = name && std::strcmp(name, kName) == 0;
    if (status) *status = found ? 0 : 1;
    return found ? static_cast<void*>(&instance) : nullptr;
}
