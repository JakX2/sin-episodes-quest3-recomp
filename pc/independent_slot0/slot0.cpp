// Independently authored Win32 PE32 engine-interface initialization diagnostic.
// TWO vtable slots ONLY. No original DLL forwarding and NOT playable SiN.
// Source SDK 2004 names/order are hypotheses for the SiN retail engine.
// These methods deliberately fail closed after a bounded early factory probe.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <type_traits>

#if !defined(_M_IX86)
#error Native MSVC x86 PE32 only
#endif

// Opaque SDK 2004 data-pointer types: never dereferenced by this diagnostic.
struct CGlobalVarsBase;
struct CGlobalVars;

namespace {
using Factory = void* (__cdecl *)(const char*, int*);

#if defined(SIN_ROLE_client)
constexpr char kName[] = "VClient011";
constexpr char kRole[] = "client";
constexpr char kFirstEngineVersion[] = "VEngineClient012";
constexpr char kSecondEngineVersion[] = "VEngineModel012";
#elif defined(SIN_ROLE_server)
constexpr char kName[] = "ServerGameDLL003";
constexpr char kRole[] = "server";
constexpr char kFirstEngineVersion[] = "VEngineServer021";
constexpr char kSecondEngineVersion[] = "VoiceServer001";
constexpr char kThirdEngineVersion[] = "VEngineCvar002";
#else
#error Choose exactly one DLL role
#endif

// Opt-in owner debugging. No writes in DllMain (none is provided), no
// original code/addresses, no third-party interface pointer values.
static void diagnostic(const char* phase, const char* version) noexcept {
    char flag[2] = {};
    if (GetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_FILE", flag, 2) != 1 ||
        flag[0] != '1') return;
    char temp[MAX_PATH] = {};
    DWORD used = GetTempPathA(MAX_PATH, temp);
    if (!used || used >= MAX_PATH) return;
    char path[MAX_PATH] = {};
    int n = std::snprintf(path, sizeof(path),
                          "%ssin-independent-%lu.log",
                          temp, static_cast<unsigned long>(GetCurrentProcessId()));
    if (n <= 0 || static_cast<size_t>(n) >= sizeof(path)) return;
    char line[240] = {};
    n = std::snprintf(line, sizeof(line),
                      "[SiN independent] role=%s phase=%s version=%s\n",
                      kRole, phase, version ? version : "<none>");
    if (n <= 0 || static_cast<size_t>(n) >= sizeof(line)) return;
    HANDLE file = CreateFileA(path, FILE_APPEND_DATA, FILE_SHARE_READ|FILE_SHARE_WRITE,
                              nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    (void)WriteFile(file, line, static_cast<DWORD>(n), &written, nullptr);
    CloseHandle(file);
}

// Probe strictly an SDK2004 candidate interface name. The engine-owned
// factory may be called as a cdecl callback, but its returned pointer is
// NEVER dereferenced or retained by this experimental diagnostic.
static bool require(Factory factory, const char* name) noexcept {
    if (factory == nullptr) {
        diagnostic("missing-factory", name);
        return false;
    }
    diagnostic("request", name);
    if (factory(name, nullptr) == nullptr) {
        diagnostic("missing-interface", name);
        return false;
    }
    diagnostic("found-interface", name);
    return true;
}

#if defined(SIN_ROLE_client)
class Boundary final {
public:
    virtual int __thiscall Init(
        Factory appSystemFactory, Factory physicsFactory,
        CGlobalVarsBase* globals) noexcept {
        if (!appSystemFactory || !physicsFactory || !globals) {
            diagnostic("invalid-init-arguments", nullptr);
            return 0;
        }
        // These are the FIRST TWO SDK2004 client app-system queries, not a
        // claim that SiN uses identical versions or initialization order.
        if (!require(appSystemFactory, kFirstEngineVersion) ||
            !require(appSystemFactory, kSecondEngineVersion)) return 0;
        diagnostic("sdk2004-preflight-only", nullptr);
        seen_ = true;
        return 0; // Never advertise successful independent startup.
    }
    virtual void __thiscall Shutdown() noexcept {
        seen_ = false;
        diagnostic("shutdown", nullptr);
    }
    bool seen_ = false;
};
using ExpectedInit = int (__thiscall Boundary::*)(
    Factory, Factory, CGlobalVarsBase*) noexcept;
using ExpectedSecond = void (__thiscall Boundary::*)() noexcept;
static_assert(std::is_same<decltype(&Boundary::Init), ExpectedInit>::value,
              "SDK2004 client Init signature / global pointer mismatch");
static_assert(std::is_same<decltype(&Boundary::Shutdown), ExpectedSecond>::value,
              "SDK2004 client Shutdown signature mismatch");
#elif defined(SIN_ROLE_server)
class Boundary final {
public:
    virtual bool __thiscall DLLInit(
        Factory engineFactory, Factory physicsFactory,
        Factory fileSystemFactory, CGlobalVars* globals) noexcept {
        if (!engineFactory || !physicsFactory || !fileSystemFactory || !globals) {
            diagnostic("invalid-init-arguments", nullptr);
            return false;
        }
        // These are the FIRST THREE SDK2004 game-server engine requests.
        if (!require(engineFactory, kFirstEngineVersion) ||
            !require(engineFactory, kSecondEngineVersion) ||
            !require(engineFactory, kThirdEngineVersion)) return false;
        diagnostic("sdk2004-preflight-only", nullptr);
        seen_ = true;
        return false;
    }
    virtual bool __thiscall GameInit() noexcept {
        diagnostic("gameinit-rejected", nullptr);
        return false;
    }
    bool seen_ = false;
};
using ExpectedInit = bool (__thiscall Boundary::*)(
    Factory, Factory, Factory, CGlobalVars*) noexcept;
using ExpectedSecond = bool (__thiscall Boundary::*)() noexcept;
static_assert(std::is_same<decltype(&Boundary::DLLInit), ExpectedInit>::value,
              "SDK2004 server DLLInit signature mismatch");
static_assert(std::is_same<decltype(&Boundary::GameInit), ExpectedSecond>::value,
              "SDK2004 server GameInit signature mismatch");
#endif
Boundary instance;
} // namespace

extern "C" __declspec(dllexport) void* __cdecl CreateInterface(
    const char* name, int* status) noexcept {
    const bool found = name && std::strcmp(name, kName) == 0;
    if (status) *status = found ? 0 : 1;
    return found ? static_cast<void*>(&instance) : nullptr;
}
