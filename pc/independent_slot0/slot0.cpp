// Independently authored Win32 PE32 engine-interface initialization diagnostic.
// Native, fail-closed diagnostic. No original DLL forwarding; NOT playable.
// The original VClient011 first twenty-one factory queries and server
// ServerGameDLL003 DLLInit first sixteen cross-factory queries are now reviewed
// against SHA-pinned owner originals. All bodies remain fail-closed.
// These methods deliberately fail closed after a bounded early factory probe.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <type_traits>
#if defined(SIN_SYNTHETIC_SOUND_CONNECT) || defined(SIN_ROLE_server)
#include "sound_emitter_connect_core.h"
#endif
#if defined(SIN_ROLE_server)
#include "server_gameents_abi.h"
#include "server_tick_interval_core.h"
#include "sin_user_message_catalog.h"
#include "server_post_connect_core.h"
#include "server_single_cvar_probe.h"
#include <new>
#include <intrin.h>
#endif

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
// Remaining initial app-system factory dependencies authenticated in
// original VClient011 Init (RVA 0xFE0E0), before sound-emitter Connect.
constexpr const char* kRemainingClientVersions[] = {
    "VEngineRenderView011",
    "VDebugOverlay003",
    "VMaterialSystem076",
    "VEngineCache001",
    "VModelInfoClient003",
    "VEngineVGui001",
    "VEngineClientStringTable001",
    "SpatialPartition001",
    "VEngineShadowMgr002",
    "StaticPropMgrClient004",
    "IEngineSoundClient003",
    "VFileSystem012",
    "VEngineRandom001",
    "VENGINE_GAMEUIFUNCS_VERSION004",
    "GAMEEVENTSMANAGER002",
    "VSoundEmitter001",
};
#elif defined(SIN_ROLE_server)
constexpr char kName[] = "ServerGameDLL003";
constexpr char kRole[] = "server";
constexpr char kFirstEngineVersion[] = "VEngineServer021";
constexpr char kSecondEngineVersion[] = "VoiceServer001";
constexpr char kThirdEngineVersion[] = "VEngineCvar002";
// Next seven names and order were recovered from the SHA-pinned original
// server.dll ServerGameDLL003 slot-zero DLLInit. All ten use engineFactory.
constexpr char kFourthEngineVersion[] = "VEngineServerStringTable001";
constexpr char kFifthEngineVersion[] = "StaticPropMgrServer002";
constexpr char kSixthEngineVersion[] = "VEngineRandom001";
constexpr char kSeventhEngineVersion[] = "IEngineSoundServer003";
constexpr char kEighthEngineVersion[] = "SpatialPartition001";
constexpr char kNinthEngineVersion[] = "VModelInfoServer001";
constexpr char kTenthEngineVersion[] = "EngineTraceServer003";
// Original DLLInit next requests VFileSystem012 from fileSystemFactory,
// then these five additional interface names from engineFactory.
constexpr char kFileSystemVersion[] = "VFileSystem012";
constexpr char kEleventhEngineVersion[] = "GAMEEVENTSMANAGER002";
constexpr char kTwelfthEngineVersion[] = "VEngineCache001";
constexpr char kThirteenthEngineVersion[] = "VSoundEmitter001";
constexpr char kFourteenthEngineVersion[] = "ServerUploadGameStats001";
constexpr char kFifteenthEngineVersion[] = "ISERVERPLUGINHELPERS001";
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

#if defined(SIN_ROLE_server)
// Every unsupported server callback terminates ONLY the current game
// process with a recognizable diagnostic exit code. No invented return
// value, dereference, or fallback to a commercial DLL. These boundaries
// are diagnostic surfaces, NOT implementations of gameplay APIs.
[[noreturn]] static void stopUnsupported(const char* method) noexcept {
    diagnostic("unsupported-callback", method);
    TerminateProcess(GetCurrentProcess(), 0x53494E45u);
    __fastfail(7);
}
static bool extraFactoriesEnabled() noexcept {
    char value[2] = {};
    if (GetEnvironmentVariableA("SIN_INDEPENDENT_SERVER_FACTORY_PROBE", value, 2) != 1 ||
        value[0] != '1') return false;
    if (GetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_FILE", value, 2) != 1 ||
        value[0] != '1') return false;
    char session[33] = {};
    if (GetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_SESSION", session, 33) != 32)
        return false;
    for (unsigned i=0;i<32;++i)
        if (!((session[i]>='0' && session[i]<='9') ||
              (session[i]>='a' && session[i]<='f'))) return false;
    return true;
}
// Slot-index-only traps have no reconstructed parameter/return signatures.
// They are NEVER allowed to return (so no guessed x86 callee stack cleanup).
// This covers every authenticated original table entry, not arbitrary
// positions beyond that table. Do not treat these traps as ABI parity.
#define SIN_GUARD_SLOT(n, prefix) \
    virtual void __thiscall GuardSlot##n() noexcept { stopUnsupported(prefix ".slot" #n); }
#endif

// Acquire pointers without dereferencing. The sound pointer is retained
// locally only for the explicitly selected first post-factory operation.
static bool require(Factory factory, const char* name,
                    void** captured = nullptr) noexcept {
    if (captured) *captured = nullptr;
    if (factory == nullptr) {
        diagnostic("missing-factory", name);
        return false;
    }
    diagnostic("request", name);
    void* value = factory(name, nullptr);
    if (!value) {
        diagnostic("missing-interface", name);
        return false;
    }
    if (captured) *captured = value;
    diagnostic("found-interface", name);
    return true;
}

#if defined(SIN_SYNTHETIC_SOUND_CONNECT) || defined(SIN_ROLE_server)
// Shared typed first post-acquisition callback. Default owner builds
// still refuse before calling it unless separately opted in.
static bool connectSound(void* sound, Factory factory) noexcept {
    diagnostic("sound-connect-entered", "VSoundEmitter001");
    using sin_independent::SoundConnectResult;
    const auto result=sin_independent::TryConnectSoundEmitter(sound,factory);
    const bool connected=result==SoundConnectResult::Connected;
#if defined(SIN_ROLE_server) && !defined(SIN_SYNTHETIC_SOUND_CONNECT)
    if (!connected && result!=SoundConnectResult::Rejected)
        stopUnsupported("VSoundEmitter001.Connect.missing-abi-input");
#endif
    diagnostic(connected ? "sound-connect-succeeded" : "sound-connect-rejected",
               "VSoundEmitter001");
    return connected;
}
#endif

#if defined(SIN_ROLE_server) && !defined(SIN_SYNTHETIC_SOUND_CONNECT)
static bool retailSoundProbeEnabled() noexcept {
    char enabled[2]={};
    return extraFactoriesEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_SERVER_SOUND_PROBE",enabled,2)==1 &&
        enabled[0]=='1';
}
static bool retailEventProbeEnabled() noexcept {
    char enabled[2]={};
    return retailSoundProbeEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_SERVER_EVENT_PROBE",enabled,2)==1 &&
        enabled[0]=='1';
}
static bool retailCvarProbeEnabled() noexcept {
    char enabled[2]={};
    return retailEventProbeEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_SERVER_CVAR_PROBE",enabled,2)==1 &&
        enabled[0]=='1';
}
static void traceCvar(const char* phase) noexcept {
    diagnostic(phase,sin_independent::console_abi::kProbeVarName);
}
[[noreturn]] static void stopAfterCvarProbe(void* cvar) noexcept {
    using namespace sin_independent::console_abi;
    // The engine may retain this object. Never destroy or unload it
    // while the engine is running; the bounded probe terminates instead.
    static Variable variable;
    const auto result=RegisterSingleProbeVariable(cvar,variable,&traceCvar);
    diagnostic("cvar-probe-result",ResultName(result));
    diagnostic("cvar-probe-terminal-stop",kProbeVarName);
    TerminateProcess(GetCurrentProcess(),result==SingleCvarResult::Verified?0x53494E49u:0x53494E4Au);
    __fastfail(7);
}
// Keep the original Phase 330 sound-only termination unless a second
// opt-in explicitly allows the post-Connect event boundary.
[[noreturn]] static void stopAfterSoundProbe(bool connected) noexcept {
    diagnostic("sound-probe-terminal-stop","VSoundEmitter001");
    TerminateProcess(GetCurrentProcess(), connected ? 0x53494E46u : 0x53494E47u);
    __fastfail(7);
}
[[noreturn]] static void stopAfterEventProbe(
    void* events, void* cvar, CGlobalVars* globals, Factory engine, Factory physics,
    Factory filesystem) noexcept {
    static sin_independent::ServerPostConnectState state;
    if(!state.Initialize(globals,engine,physics,filesystem))
        stopUnsupported("ServerPostConnect.Initialize");
    diagnostic("math-tables-initialized","server-scalar");
    diagnostic("factory-state-cached","server");
    diagnostic("event-load-entered","resource/gameevents.res");
    int count=0;
    if(!state.LoadInitialEvents(events,count))
        stopUnsupported("GAMEEVENTSMANAGER002.LoadEventsFromFile.missing-abi-input");
    char result[80]={};
    std::snprintf(result,sizeof(result),"resource/gameevents.res:%d",count);
    diagnostic("event-load-returned",result);
    if(count>0 && retailCvarProbeEnabled())stopAfterCvarProbe(cvar);
    diagnostic("event-probe-terminal-stop","resource/gameevents.res");
    TerminateProcess(GetCurrentProcess(),0x53494E48u);
    __fastfail(7);
}
#endif

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
        // All 21 initial names/order are authenticated from the original
        // VClient011 Init slot. Never dereference engine pointers or
        // advertise successful initialization. Stop at first missing input.
        if (!require(appSystemFactory, kFirstEngineVersion) ||
            !require(appSystemFactory, kSecondEngineVersion) ||
            !require(appSystemFactory, kThirdEngineVersion) ||
            !require(appSystemFactory, kFourthEngineVersion) ||
            !require(appSystemFactory, kFifthEngineVersion)) return 0;
        void* sound=nullptr;
        for (const char* version : kRemainingClientVersions)
            if (!require(appSystemFactory, version,
                         std::strcmp(version,"VSoundEmitter001")==0 ? &sound : nullptr))
                return 0;
#if defined(SIN_SYNTHETIC_SOUND_CONNECT)
        if (!connectSound(sound,appSystemFactory)) return 0;
#endif
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
        // First SIXTEEN original DLLInit interface dependencies are now
        // authenticated against the pinned server. The filesystem request
        // uses the THIRD factory argument; the surrounding 15 use the
        // engine factory. Stop at the first unavailable pointer, never
        // dereference or retain it, and NEVER report successful Init.
        void* sound=nullptr;
        void* events=nullptr;
        void* cvar=nullptr;
        if (!require(engineFactory, kFirstEngineVersion) ||
            !require(engineFactory, kSecondEngineVersion) ||
            !require(engineFactory, kThirdEngineVersion, &cvar) ||
            !require(engineFactory, kFourthEngineVersion) ||
            !require(engineFactory, kFifthEngineVersion) ||
            !require(engineFactory, kSixthEngineVersion) ||
            !require(engineFactory, kSeventhEngineVersion) ||
            !require(engineFactory, kEighthEngineVersion) ||
            !require(engineFactory, kNinthEngineVersion) ||
            !require(engineFactory, kTenthEngineVersion) ||
            !require(fileSystemFactory, kFileSystemVersion) ||
            !require(engineFactory, kEleventhEngineVersion, &events) ||
            !require(engineFactory, kTwelfthEngineVersion) ||
            !require(engineFactory, kThirteenthEngineVersion, &sound) ||
            !require(engineFactory, kFourteenthEngineVersion) ||
            !require(engineFactory, kFifteenthEngineVersion)) return false;
#if defined(SIN_SYNTHETIC_SOUND_CONNECT)
        if (!connectSound(sound,engineFactory)) return false;
#else
        if (retailSoundProbeEnabled()) {
            const bool connected=connectSound(sound,engineFactory);
            if(connected && retailEventProbeEnabled())
                stopAfterEventProbe(events,cvar,globals,engineFactory,physicsFactory,fileSystemFactory);
            stopAfterSoundProbe(connected);
        }
#endif
        diagnostic("sdk2004-preflight-only", nullptr);
        seen_ = true;
        return false;
    }
    virtual bool __thiscall GameInit() noexcept {
        diagnostic("gameinit-rejected", nullptr);
        return false;
    }
    // Original ServerGameDLL003 has 31 entries, not the SDK's 29.
    // Unimplemented slots remain terminal guards; verified methods are typed.
    SIN_GUARD_SLOT(2, "ServerGameDLL003")
    SIN_GUARD_SLOT(3, "ServerGameDLL003")
    SIN_GUARD_SLOT(4, "ServerGameDLL003")
    SIN_GUARD_SLOT(5, "ServerGameDLL003")
    SIN_GUARD_SLOT(6, "ServerGameDLL003")
    SIN_GUARD_SLOT(7, "ServerGameDLL003")
    SIN_GUARD_SLOT(8, "ServerGameDLL003")
    // Original slot 9 (RVA 0x1E7A90). The engine owns the command-line
    // singleton; resolve its existing vstdlib module, never load a new one.
    virtual float __thiscall GetTickInterval() const noexcept {
        auto module = GetModuleHandleA("vstdlib.dll");
        if (!module) stopUnsupported("ServerGameDLL003.GetTickInterval.missing-commandline");
        auto commandLine = reinterpret_cast<sin_independent::CommandLineProvider>(
            GetProcAddress(module,"CommandLine"));
        float interval = 0;
        if (!sin_independent::TryGetTickInterval(commandLine, interval))
            stopUnsupported("ServerGameDLL003.GetTickInterval.missing-commandline");
        diagnostic("diagnostic-callback","ServerGameDLL003.GetTickInterval");
        return interval;
    }
    // Original slot 10 returns the registered server-class linked-list
    // head. Do not give the engine an empty or metadata-only send schema.
    virtual struct ServerClass* __thiscall GetAllServerClasses() noexcept {
        stopUnsupported("ServerGameDLL003.GetAllServerClasses");
    }
    // Original slot 11, RVA 0x1e7c40, delegates to the description
    // helper at RVA 0x2f0cc0. Before a game-rules object exists, that
    // helper returns the process-lifetime "SiN Episodes" string.
    // This DLL never initializes game rules, so only the authenticated
    // pre-initialization branch is reachable here.
    virtual const char* __thiscall GetGameDescription() noexcept {
        diagnostic("diagnostic-callback", "ServerGameDLL003.GetGameDescription");
        return "SiN Episodes";
    }
    SIN_GUARD_SLOT(12, "ServerGameDLL003")
    SIN_GUARD_SLOT(13, "ServerGameDLL003")
    SIN_GUARD_SLOT(14, "ServerGameDLL003")
    SIN_GUARD_SLOT(15, "ServerGameDLL003")
    SIN_GUARD_SLOT(16, "ServerGameDLL003")
    SIN_GUARD_SLOT(17, "ServerGameDLL003")
    SIN_GUARD_SLOT(18, "ServerGameDLL003")
    SIN_GUARD_SLOT(19, "ServerGameDLL003")
    SIN_GUARD_SLOT(20, "ServerGameDLL003")
    SIN_GUARD_SLOT(21, "ServerGameDLL003")
    SIN_GUARD_SLOT(22, "ServerGameDLL003")
    SIN_GUARD_SLOT(23, "ServerGameDLL003")
    SIN_GUARD_SLOT(24, "ServerGameDLL003")
    SIN_GUARD_SLOT(25, "ServerGameDLL003")
    SIN_GUARD_SLOT(26, "ServerGameDLL003")
    SIN_GUARD_SLOT(27, "ServerGameDLL003")
    // Original slot 28, RVA 0x1e7c80: bounded name and size lookup
    // from the shared message catalog registered on game DLL load.
    virtual bool __thiscall GetUserMessageInfo(
        int index, char* name, int capacity, int& bytes) noexcept {
        diagnostic("diagnostic-callback","ServerGameDLL003.GetUserMessageInfo");
        return sin_independent::GetUserMessageInfo(index,name,capacity,bytes);
    }
    SIN_GUARD_SLOT(29, "ServerGameDLL003")
    SIN_GUARD_SLOT(30, "ServerGameDLL003")
    bool seen_ = false;
};
using ExpectedInit = bool (__thiscall Boundary::*)(
    Factory, Factory, Factory, CGlobalVars*) noexcept;
using ExpectedSecond = bool (__thiscall Boundary::*)() noexcept;
static_assert(std::is_same<decltype(&Boundary::DLLInit), ExpectedInit>::value,
              "SDK2004 server DLLInit signature mismatch");
static_assert(std::is_same<decltype(&Boundary::GameInit), ExpectedSecond>::value,
              "SDK2004 server GameInit signature mismatch");

class EntityFactoryProbe final : public sin_independent::ServerGameEntsAbi {
public:
    ~EntityFactoryProbe() noexcept override {
        stopUnsupported("ServerGameEnts001.destructor");
    }
    void __thiscall SetDebugEdictBase(edict_t* base) noexcept override {
        // Verified original behavior: save the pointer, never dereference.
        debugBase_ = base;
        diagnostic("diagnostic-callback","ServerGameEnts001.SetDebugEdictBase");
    }
    void __thiscall MarkEntitiesAsTouching(edict_t*,edict_t*) noexcept override {
        stopUnsupported("ServerGameEnts001.MarkEntitiesAsTouching");
    }
    void __thiscall FreeContainingEntity(edict_t*) noexcept override {
        stopUnsupported("ServerGameEnts001.FreeContainingEntity");
    }
    edict_t* __thiscall BaseEntityToEdict(CBaseEntity* entity) noexcept override {
        if (!entity) return nullptr;
        stopUnsupported("ServerGameEnts001.BaseEntityToEdict");
    }
    CBaseEntity* __thiscall EdictToBaseEntity(edict_t* edict) noexcept override {
        if (!edict) return nullptr;
        stopUnsupported("ServerGameEnts001.EdictToBaseEntity");
    }
    void __thiscall CheckTransmit(CCheckTransmitInfo*, const unsigned short*, int) noexcept override {
        stopUnsupported("ServerGameEnts001.CheckTransmit");
    }
private:
    edict_t* debugBase_ = nullptr;
};
// Process-lifetime singleton with no automatic destructor invocation on
// DLL detach. A caller attempting to delete it reaches the terminal guard.
static EntityFactoryProbe* entityFactoryProbe() noexcept {
    alignas(EntityFactoryProbe) static unsigned char storage[sizeof(EntityFactoryProbe)];
    static auto* const object = new (storage) EntityFactoryProbe;
    return object;
}

class ClientsFactoryProbe final {
public:
    // Original slot 0 RVA 0x1E63F0 writes exactly min=1,max=32,default=1.
    virtual void __thiscall GetPlayerLimits(int& minPlayers,int& maxPlayers,
                                           int& defaultPlayers) const noexcept {
        minPlayers=1;
        maxPlayers=32;
        defaultPlayers=1;
        diagnostic("diagnostic-callback","ServerGameClients003.GetPlayerLimits");
    }
    SIN_GUARD_SLOT(1, "ServerGameClients003")
    SIN_GUARD_SLOT(2, "ServerGameClients003")
    SIN_GUARD_SLOT(3, "ServerGameClients003")
    SIN_GUARD_SLOT(4, "ServerGameClients003")
    SIN_GUARD_SLOT(5, "ServerGameClients003")
    SIN_GUARD_SLOT(6, "ServerGameClients003")
    SIN_GUARD_SLOT(7, "ServerGameClients003")
    SIN_GUARD_SLOT(8, "ServerGameClients003")
    SIN_GUARD_SLOT(9, "ServerGameClients003")
    SIN_GUARD_SLOT(10, "ServerGameClients003")
    SIN_GUARD_SLOT(11, "ServerGameClients003")
    SIN_GUARD_SLOT(12, "ServerGameClients003")
    SIN_GUARD_SLOT(13, "ServerGameClients003")
    SIN_GUARD_SLOT(14, "ServerGameClients003")
};
ClientsFactoryProbe clientsFactoryProbe;
#undef SIN_GUARD_SLOT
#endif
Boundary instance;
} // namespace

extern "C" __declspec(dllexport) void* __cdecl CreateInterface(
    const char* name, int* status) noexcept {
    void* result = name && std::strcmp(name, kName) == 0
        ? static_cast<void*>(&instance) : nullptr;
#if defined(SIN_ROLE_server)
    if (!result && name && extraFactoriesEnabled()) {
        if (std::strcmp(name,"ServerGameEnts001")==0) result=entityFactoryProbe();
        else if (std::strcmp(name,"ServerGameClients003")==0) result=&clientsFactoryProbe;
    }
#endif
    const bool found = result != nullptr;
    if (status) *status = found ? 0 : 1;
    // The retail engine may reject our candidate version before it ever
    // calls slot zero. This is a factory negotiation observation ONLY.
    // A rejected probe does not prove the engine considers it fatal.
    diagnostic(found ? "factory-accepted" : "factory-rejected", name);
    return result;
}
