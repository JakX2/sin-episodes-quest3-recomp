// Independently authored Win32 PE32 engine-interface initialization diagnostic.
// TWO vtable slots ONLY. No original DLL forwarding and NOT playable SiN.
// The original VClient011 first five factory queries are now reviewed
// against owner-provided SHA-pinned retail client.dll. Server names are
// still SDK2004 hypotheses pending original server DLLInit verification.
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
// These next three names and query order were reviewed directly from the
// owner's SHA-pinned original retail client.dll VClient011 slot-zero Init.
// We only inspect returned non-nullness; NO interface methods are invoked.
constexpr char kThirdEngineVersion[] = "VEngineEffects001";
constexpr char kFourthEngineVersion[] = "VEngineCvar002";
constexpr char kFifthEngineVersion[] = "EngineTraceClient003";
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
    // A per-launch, validated lowercase hex token prevents confusing old
    // or concurrent diagnostics with an owner's retail startup attempt.
    char session[33] = {};
    const DWORD sessionLength =
        GetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_SESSION", session,
                                static_cast<DWORD>(sizeof(session)));
    if (sessionLength && sessionLength != 32) return;
    for (DWORD i = 0; i < sessionLength; ++i) {
        if (!((session[i] >= '0' && session[i] <= '9') ||
              (session[i] >= 'a' && session[i] <= 'f'))) return;
    }
    char path[MAX_PATH] = {};
    int n = sessionLength == 32
        ? std::snprintf(path, sizeof(path),
                        "%ssin-independent-%s-%lu.log", temp, session,
                        static_cast<unsigned long>(GetCurrentProcessId()))
        : std::snprintf(path, sizeof(path),
                        "%ssin-independent-%lu.log", temp,
                        static_cast<unsigned long>(GetCurrentProcessId()));
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
        diagnostic("init-entered", nullptr);
        if (!appSystemFactory || !physicsFactory || !globals) {
            diagnostic("invalid-init-arguments", nullptr);
            return 0;
        }
        // The FIRST FIVE names/order were confirmed from the authenticated
        // original retail VClient011 Init slot. We never dereference engine
        // pointers or advertise successful initialization. Stop at first
        // unavailable interface so each owner probe finds a precise blocker.
        if (!require(appSystemFactory, kFirstEngineVersion) ||
            !require(appSystemFactory, kSecondEngineVersion) ||
            !require(appSystemFactory, kThirdEngineVersion) ||
            !require(appSystemFactory, kFourthEngineVersion) ||
            !require(appSystemFactory, kFifthEngineVersion)) return 0;
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
        diagnostic("init-entered", nullptr);
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
    // The retail engine may reject our candidate version before it ever
    // calls slot zero. This is a factory negotiation observation ONLY.
    // A rejected probe does not prove the engine considers it fatal.
    diagnostic(found ? "factory-accepted" : "factory-rejected", name);
    return found ? static_cast<void*>(&instance) : nullptr;
}
