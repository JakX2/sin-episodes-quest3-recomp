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
#include <intrin.h>
#if defined(SIN_ROLE_client) || defined(SIN_ROLE_server)
#include "sound_emitter_connect_core.h"
#endif
#if defined(SIN_ROLE_client)
#include "client_material_startup_core.h"
#include "client_screen_random_core.h"
#include "client_cvar_probe.h"
#include "client_owned_startup_probe.h"
#include "client_vgui_vtable_probe.h"
#include "client_engine_vgui_slot1_probe.h"
#include "client_vgui_controls_bootstrap.h"
#include "client_mat_surface_query.h"
#include "client_known_gamesystem_init.h"
#endif
#if defined(SIN_ROLE_server)
#include "server_gameents_abi.h"
#include "server_tick_interval_core.h"
#include "sin_user_message_catalog.h"
#include "server_post_connect_core.h"
#include "server_single_cvar_probe.h"
#include "server_batch_cvar_probe.h"
#include <new>
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

// Every unsupported callback terminates ONLY the current game
// process with a recognizable diagnostic exit code. No invented return
// value, dereference, or fallback to a commercial DLL. These boundaries
// are diagnostic surfaces, NOT implementations of gameplay APIs.
[[noreturn]] static void stopUnsupported(const char* method) noexcept {
    diagnostic("unsupported-callback", method);
    TerminateProcess(GetCurrentProcess(), 0x53494E45u);
    __fastfail(7);
}
static bool validTraceSession() noexcept {
    char value[2]={};
    if(GetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_FILE",value,2)!=1 ||
       value[0]!='1')return false;
    char session[33]={};
    if(GetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_SESSION",session,33)!=32)
        return false;
    for(unsigned i=0;i<32;++i)
        if(!((session[i]>='0' && session[i]<='9') ||
             (session[i]>='a' && session[i]<='f')))return false;
    return true;
}
#if defined(SIN_ROLE_server)
static bool extraFactoriesEnabled() noexcept {
    char value[2] = {};
    if (GetEnvironmentVariableA("SIN_INDEPENDENT_SERVER_FACTORY_PROBE", value, 2) != 1 ||
        value[0] != '1') return false;
    return validTraceSession();
}
#endif
// Slot-index-only traps have no reconstructed parameter/return signatures.
// They are NEVER allowed to return (so no guessed x86 callee stack cleanup).
// This covers every authenticated original table entry, not arbitrary
// positions beyond that table. Do not treat these traps as ABI parity.
#define SIN_GUARD_SLOT(n, prefix) \
    virtual void __thiscall GuardSlot##n() noexcept { stopUnsupported(prefix ".slot" #n); }

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

#if defined(SIN_ROLE_client) || defined(SIN_ROLE_server)
// Shared typed first post-acquisition callback. Default owner builds
// still refuse before calling it unless separately opted in.
static bool connectSound(void* sound, Factory factory) noexcept {
    diagnostic("sound-connect-entered", "VSoundEmitter001");
    using sin_independent::SoundConnectResult;
    const auto result=sin_independent::TryConnectSoundEmitter(sound,factory);
    const bool connected=result==SoundConnectResult::Connected;
#if !defined(SIN_SYNTHETIC_SOUND_CONNECT)
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
static bool retailBatchCvarProbeEnabled() noexcept {
    char enabled[2]={};
    return retailCvarProbeEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_SERVER_CVAR_BATCH_PROBE",enabled,2)==1 &&
        enabled[0]=='1';
}
[[noreturn]] static void stopAfterBatchCvarProbe(void* cvar) noexcept {
    using namespace sin_independent::console_abi;
    static BatchVariables variables;
    const auto result=variables.Register(cvar,&diagnostic);
    char outcome[96]={};
    std::snprintf(outcome,sizeof(outcome),"%s:%u/%u",
        ResultName(result.result),static_cast<unsigned>(result.verified),
        static_cast<unsigned>(kBatchCount));
    diagnostic("cvar-batch-result",outcome);
    diagnostic("cvar-batch-terminal-stop","24");
    TerminateProcess(GetCurrentProcess(),result.result==SingleCvarResult::Verified?
        0x53494E4Du:0x53494E4Eu);
    __fastfail(7);
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
    if(count>0 && retailBatchCvarProbeEnabled())stopAfterBatchCvarProbe(cvar);
    if(count>0 && retailCvarProbeEnabled())stopAfterCvarProbe(cvar);
    diagnostic("event-probe-terminal-stop","resource/gameevents.res");
    TerminateProcess(GetCurrentProcess(),0x53494E48u);
    __fastfail(7);
}
#endif

#if defined(SIN_ROLE_client)
// Retain the original engine interface for both actual and synthetic Init paths.
static void* g_engineVgui=nullptr;
static Factory g_clientAppFactory=nullptr;
static void* g_clientVGuiSurface=nullptr;
static void* g_clientSoundEmitter=nullptr;
#if !defined(SIN_SYNTHETIC_SOUND_CONNECT)
static bool clientSoundProbeEnabled() noexcept {
    char enabled[2]={};
    return validTraceSession() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_SOUND_PROBE",enabled,2)==1 &&
        enabled[0]=='1';
}
static bool clientMaterialProbeEnabled() noexcept {
    char enabled[2]={};
    return clientSoundProbeEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_MATERIAL_PROBE",enabled,2)==1 &&
        enabled[0]=='1';
}
static bool clientScreenProbeEnabled() noexcept {
    char enabled[2]={};
    return clientMaterialProbeEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_SCREEN_PROBE",enabled,2)==1 &&
        enabled[0]=='1';
}
static bool clientGaussianProbeEnabled() noexcept {
    char enabled[2]={};
    return clientScreenProbeEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_GAUSSIAN_PROBE",enabled,2)==1 &&
        enabled[0]=='1';
}
static bool clientCvarProbeEnabled() noexcept {
    char enabled[2]={};
    return clientGaussianProbeEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_CVAR_PROBE",enabled,2)==1 &&
        enabled[0]=='1';
}
static bool clientOwnedStartupProbeEnabled() noexcept {
    char enabled[2]={};
    return clientCvarProbeEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_OWNED_STARTUP_PROBE",enabled,2)==1 &&
        enabled[0]=='1';
}
static bool clientVGuiProbeEnabled() noexcept {
    char enabled[2]={};
    return clientOwnedStartupProbeEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_VGUI_PROBE",enabled,2)==1 &&
        enabled[0]=='1';
}
static bool clientVGuiLayoutProbeEnabled() noexcept {
    char value[2]={};
    return clientVGuiProbeEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_VGUI_LAYOUT_PROBE",value,2)==1 &&
        value[0]=='1';
}
static bool clientEngineVGuiSlot1ProbeEnabled() noexcept {
    char value[2]={};
    return clientVGuiLayoutProbeEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_ENGINE_VGUI_SLOT1_PROBE",value,2)==1 &&
        value[0]=='1';
}
static bool clientVGuiControlsBootstrapProbeEnabled() noexcept {
    char value[2]={};
    return clientEngineVGuiSlot1ProbeEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_VGUI_CONTROLS_BOOTSTRAP_PROBE",value,2)==1 &&
        value[0]=='1';
}
static bool clientMatSurfaceProbeEnabled() noexcept {
    char enabled[2]={};
    return clientVGuiControlsBootstrapProbeEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_MAT_SURFACE_PROBE",enabled,2)==1 &&
        enabled[0]=='1';
}
static bool clientSoundSystemInitProbeEnabled() noexcept {
    char enabled[2]={};
    return clientMatSurfaceProbeEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_SOUND_SYSTEM_INIT_PROBE",enabled,2)==1 &&
        enabled[0]=='1';
}
[[noreturn]] static void stopAfterClientSoundSystemInit() noexcept {
    diagnostic("sound-system-init-entered","CSoundEmitterSystem:VSoundEmitter001+1");
    const auto result=sin_independent::known_game_system::InitSoundEmitter(g_clientSoundEmitter);
    diagnostic("sound-system-init-result",sin_independent::known_game_system::Name(result));
    diagnostic("sound-system-init-terminal-stop","original-slot1");
    TerminateProcess(GetCurrentProcess(),
        result==sin_independent::known_game_system::CallResult::CalledTrue?
        0x53494E63u:0x53494E64u);
    __fastfail(7);
}
[[noreturn]] static void stopAfterClientMatSurfaceQuery() noexcept {
    diagnostic("mat-surface-query-entered","VGUI_Surface028:IAppSystem+4");
    void* obtained=nullptr;
    const auto result=sin_independent::mat_surface::Query(g_clientVGuiSurface,&obtained);
    diagnostic("mat-surface-query-result",sin_independent::mat_surface::Name(result));
    if(result==sin_independent::mat_surface::Result::Found &&
       clientSoundSystemInitProbeEnabled())
        stopAfterClientSoundSystemInit();
    diagnostic("mat-surface-query-terminal-stop","original-abi");
    TerminateProcess(GetCurrentProcess(),result==sin_independent::mat_surface::Result::Found?
        0x53494E61u:0x53494E62u);
    __fastfail(7);
}
[[noreturn]] static void stopAfterClientEngineVGuiSlot1Probe() noexcept {
    const bool bootstrap=clientVGuiControlsBootstrapProbeEnabled();
    if(bootstrap) {
        diagnostic("vgui-controls-bootstrap-entered","CLIENT:factory-count-1");
        const auto result=sin_independent::vgui_controls::Start(g_clientAppFactory,&diagnostic,&g_clientVGuiSurface);
        diagnostic("vgui-controls-bootstrap-result",sin_independent::vgui_controls::Name(result));
        if(!sin_independent::vgui_controls::Completed(result)) {
            diagnostic("vgui-controls-bootstrap-terminal-stop","source2004");
            TerminateProcess(GetCurrentProcess(),0x53494E60u);
            __fastfail(7);
        }
    }
    diagnostic("engine-vgui-slot1-entered","VEngineVGui001:arg2");
    const auto result=sin_independent::engine_vgui::CallSlot1Arg2(g_engineVgui);
    diagnostic("engine-vgui-slot1-result",sin_independent::engine_vgui::Name(result));
    diagnostic("engine-vgui-slot1-terminal-stop","original-slot1");
    if(bootstrap && clientMatSurfaceProbeEnabled() &&
       result==sin_independent::engine_vgui::Slot1Result::Nonzero)
        stopAfterClientMatSurfaceQuery();
    if(bootstrap)diagnostic("vgui-controls-bootstrap-terminal-stop","source2004");
    TerminateProcess(GetCurrentProcess(),bootstrap?
        (result==sin_independent::engine_vgui::Slot1Result::Nonzero?0x53494E5Fu:0x53494E60u):
        (result==sin_independent::engine_vgui::Slot1Result::Nonzero?0x53494E5Du:0x53494E5Eu));
    __fastfail(7);
}
[[noreturn]] static void stopAfterClientVGuiLayoutProbe(
    HMODULE module,void* input,void* system) noexcept {
    diagnostic("vgui-layout-entered","input:0+56,system:0+24");
    const char* result="verified";
    if(!sin_independent::vgui_layout::Verify(module,input,56)) {
        result="invalid-input";
    } else {
        diagnostic("vgui-layout-input-verified","0+56");
        if(!sin_independent::vgui_layout::Verify(module,system,24))
            result="invalid-system";
        else diagnostic("vgui-layout-system-verified","0+24");
    }
    diagnostic("vgui-layout-probe-result",result);
    if(std::strcmp(result,"verified")==0 && clientEngineVGuiSlot1ProbeEnabled())
        stopAfterClientEngineVGuiSlot1Probe();
    diagnostic("vgui-layout-terminal-stop","read-only");
    TerminateProcess(GetCurrentProcess(),std::strcmp(result,"verified")==0?0x53494E5Bu:0x53494E5Cu);
    __fastfail(7);
}
// Query only the already-loaded engine provider. Do not load a new DLL,
// invoke virtual methods, or claim the original CLIENT helper ran.
[[noreturn]] static void stopAfterClientVGuiProbe() noexcept {
    diagnostic("vgui-probe-entered","vgui2.dll");
    const auto module=GetModuleHandleA("vgui2.dll");
    const auto factory=module?reinterpret_cast<Factory>(
        GetProcAddress(module,"CreateInterface")):nullptr;
    const char* result=module?"missing-factory":"missing-module";
    static void* input=nullptr;
    static void* system=nullptr;
    if(factory) {
        diagnostic("vgui-provider-found","vgui2.dll");
        diagnostic("vgui-query-entered","VGUI_InputInternal001");
        input=factory("VGUI_InputInternal001",nullptr);
        if(input) {
            diagnostic("vgui-query-found","VGUI_InputInternal001");
            diagnostic("vgui-query-entered","VGUI_System010");
            system=factory("VGUI_System010",nullptr);
            if(system)diagnostic("vgui-query-found","VGUI_System010");
        }
        result=!input?"missing-input":!system?"missing-system":
            input==system?"aliased-interfaces":"acquired";
    }
    diagnostic("vgui-probe-result",result);
    if(std::strcmp(result,"acquired")==0 && clientVGuiLayoutProbeEnabled())
        stopAfterClientVGuiLayoutProbe(module,input,system);
    diagnostic("vgui-probe-terminal-stop","factory-only");
    TerminateProcess(GetCurrentProcess(),std::strcmp(result,"acquired")==0?0x53494E59u:0x53494E5Au);
    __fastfail(7);
}
[[noreturn]] static void stopAfterClientOwnedStartupProbe() noexcept {
    const bool verified=sin_independent::RunOwnedClientStartup(&diagnostic);
    diagnostic("owned-startup-probe-result",verified?"verified":"refused");
    if(verified && clientVGuiProbeEnabled())stopAfterClientVGuiProbe();
    diagnostic("owned-startup-terminal-stop","independent-only");
    TerminateProcess(GetCurrentProcess(),verified?0x53494E57u:0x53494E58u);
    __fastfail(7);
}
[[noreturn]] static void stopAfterClientCvarProbe(void* cvar) noexcept {
    using namespace sin_independent::console_abi;
    static ClientProbeVariables owned;
    const auto result=owned.Register(cvar,&diagnostic);
    char outcome[96]={};
    std::snprintf(outcome,sizeof(outcome),"%s:%u/%u",ResultName(result.result),
        static_cast<unsigned>(result.verified),static_cast<unsigned>(kClientSpecCount));
    diagnostic("client-cvar-probe-result",outcome);
    if(result.result==SingleCvarResult::Verified && clientOwnedStartupProbeEnabled())
        stopAfterClientOwnedStartupProbe();
    diagnostic("client-cvar-probe-terminal-stop","3");
    TerminateProcess(GetCurrentProcess(),result.result==SingleCvarResult::Verified?0x53494E55u:0x53494E56u);
    __fastfail(7);
}
[[noreturn]] static void stopAfterClientGaussianProbe(
    sin_independent::ClientScreenRandomState& state,void* uniform,void* cvar) noexcept {
    using namespace sin_independent;
    const auto module=GetModuleHandleA("vstdlib.dll");
    const auto ctor=module?reinterpret_cast<GaussianCtor>(
        GetProcAddress(module,"??0CGaussianRandomStream@@QAE@PAVIUniformRandomStream@@@Z")):nullptr;
    const auto attach=module?reinterpret_cast<GaussianAttach>(
        GetProcAddress(module,"?AttachToStream@CGaussianRandomStream@@QAEXPAVIUniformRandomStream@@@Z")):nullptr;
    const bool available=ctor && attach && uniform;
    if(available)diagnostic("gaussian-attach-entered","VEngineRandom001");
    const bool attached=available && state.Attach(uniform,ctor,attach);
    if(attached)diagnostic("gaussian-attach-verified","VEngineRandom001");
    diagnostic("gaussian-probe-result",attached?"attached":available?"layout-or-attach-refused":"missing-abi-input");
    if(attached && clientCvarProbeEnabled())stopAfterClientCvarProbe(cvar);
    diagnostic("gaussian-probe-terminal-stop","VEngineRandom001");
    TerminateProcess(GetCurrentProcess(),attached?0x53494E53u:0x53494E54u);
    __fastfail(7);
}
[[noreturn]] static void stopAfterClientScreenProbe(void* engine,void* uniform,void* cvar) noexcept {
    static sin_independent::ClientScreenRandomState state;
    auto& screen=state.screen;
    diagnostic("screen-query-entered","VEngineClient012.GetScreenSize");
    const bool queried=screen.Query(engine);
    if(queried) {
        char dimensions[80]={};
        std::snprintf(dimensions,sizeof(dimensions),"%d:%d",screen.width,screen.height);
        diagnostic("screen-query-returned",dimensions);
    }
    diagnostic("screen-probe-result",queried?"returned":"missing-abi-input");
    if(queried && clientGaussianProbeEnabled())stopAfterClientGaussianProbe(state,uniform,cvar);
    diagnostic("screen-probe-terminal-stop","VEngineClient012.GetScreenSize");
    TerminateProcess(GetCurrentProcess(),queried?0x53494E51u:0x53494E52u);
    __fastfail(7);
}
[[noreturn]] static void stopAfterClientMaterialProbe(Factory app,void* materials,void* engine,void* uniform,void* cvar) noexcept {
    using namespace sin_independent;
    const auto module=GetModuleHandleA("vstdlib.dll");
    const auto provider=module?reinterpret_cast<ClientCommandLine>(
        GetProcAddress(module,"CommandLine")):nullptr;
    static ClientMaterialState state;
    const auto result=AcquireClientMaterialState(provider,app,materials,state,&diagnostic);
    diagnostic("material-probe-result",ClientMaterialResultName(result));
    if(result==ClientMaterialResult::Acquired && clientScreenProbeEnabled())
        stopAfterClientScreenProbe(engine,uniform,cvar);
    diagnostic("material-probe-terminal-stop","MaterialSystemHardwareConfig012");
    TerminateProcess(GetCurrentProcess(),result==ClientMaterialResult::Acquired?0x53494E4Fu:0x53494E50u);
    __fastfail(7);
}
[[noreturn]] static void stopAfterClientSoundProbe(bool connected) noexcept {
    diagnostic("sound-probe-terminal-stop","VSoundEmitter001");
    TerminateProcess(GetCurrentProcess(),connected?0x53494E4Bu:0x53494E4Cu);
    __fastfail(7);
}
#endif
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
        void* engine=nullptr;
        void* cvar=nullptr;
        g_engineVgui=nullptr;
        g_clientAppFactory=appSystemFactory;
        g_clientVGuiSurface=nullptr;
        g_clientSoundEmitter=nullptr;
        if (!require(appSystemFactory, kFirstEngineVersion, &engine) ||
            !require(appSystemFactory, kSecondEngineVersion) ||
            !require(appSystemFactory, kThirdEngineVersion) ||
            !require(appSystemFactory, kFourthEngineVersion, &cvar) ||
            !require(appSystemFactory, kFifthEngineVersion)) return 0;
        void* sound=nullptr;
        void* materials=nullptr;
        void* uniform=nullptr;
        for (const char* version : kRemainingClientVersions)
            if (!require(appSystemFactory, version,
                         std::strcmp(version,"VSoundEmitter001")==0 ? &sound :
                         std::strcmp(version,"VMaterialSystem076")==0 ? &materials :
                         std::strcmp(version,"VEngineRandom001")==0 ? &uniform :
                         std::strcmp(version,"VEngineVGui001")==0 ? &g_engineVgui : nullptr))
                return 0;
        g_clientSoundEmitter=sound;
#if defined(SIN_SYNTHETIC_SOUND_CONNECT)
        if (!connectSound(sound,appSystemFactory)) return 0;
#else
        if(clientSoundProbeEnabled()) {
            const bool connected=connectSound(sound,appSystemFactory);
            if(connected && clientMaterialProbeEnabled())
                stopAfterClientMaterialProbe(appSystemFactory,materials,engine,uniform,cvar);
            stopAfterClientSoundProbe(connected);
        }
#endif
        diagnostic("sdk2004-preflight-only", nullptr);
        seen_ = true;
        return 0; // Never advertise successful independent startup.
    }
    virtual void __thiscall Shutdown() noexcept {
        seen_ = false;
        diagnostic("shutdown", nullptr);
    }
    // Original VClient011 contains 48 contiguous code entries. The
    // remaining positions terminate rather than inventing result types
    // or callee cleanup. These are NOT recovered gameplay methods.
    SIN_GUARD_SLOT(2, "VClient011")
    SIN_GUARD_SLOT(3, "VClient011")
    SIN_GUARD_SLOT(4, "VClient011")
    SIN_GUARD_SLOT(5, "VClient011")
    SIN_GUARD_SLOT(6, "VClient011")
    SIN_GUARD_SLOT(7, "VClient011")
    SIN_GUARD_SLOT(8, "VClient011")
    SIN_GUARD_SLOT(9, "VClient011")
    SIN_GUARD_SLOT(10, "VClient011")
    SIN_GUARD_SLOT(11, "VClient011")
    SIN_GUARD_SLOT(12, "VClient011")
    SIN_GUARD_SLOT(13, "VClient011")
    SIN_GUARD_SLOT(14, "VClient011")
    SIN_GUARD_SLOT(15, "VClient011")
    SIN_GUARD_SLOT(16, "VClient011")
    SIN_GUARD_SLOT(17, "VClient011")
    SIN_GUARD_SLOT(18, "VClient011")
    SIN_GUARD_SLOT(19, "VClient011")
    SIN_GUARD_SLOT(20, "VClient011")
    SIN_GUARD_SLOT(21, "VClient011")
    SIN_GUARD_SLOT(22, "VClient011")
    SIN_GUARD_SLOT(23, "VClient011")
    SIN_GUARD_SLOT(24, "VClient011")
    SIN_GUARD_SLOT(25, "VClient011")
    SIN_GUARD_SLOT(26, "VClient011")
    SIN_GUARD_SLOT(27, "VClient011")
    SIN_GUARD_SLOT(28, "VClient011")
    SIN_GUARD_SLOT(29, "VClient011")
    SIN_GUARD_SLOT(30, "VClient011")
    SIN_GUARD_SLOT(31, "VClient011")
    SIN_GUARD_SLOT(32, "VClient011")
    SIN_GUARD_SLOT(33, "VClient011")
    SIN_GUARD_SLOT(34, "VClient011")
    SIN_GUARD_SLOT(35, "VClient011")
    SIN_GUARD_SLOT(36, "VClient011")
    SIN_GUARD_SLOT(37, "VClient011")
    SIN_GUARD_SLOT(38, "VClient011")
    SIN_GUARD_SLOT(39, "VClient011")
    SIN_GUARD_SLOT(40, "VClient011")
    SIN_GUARD_SLOT(41, "VClient011")
    SIN_GUARD_SLOT(42, "VClient011")
    SIN_GUARD_SLOT(43, "VClient011")
    SIN_GUARD_SLOT(44, "VClient011")
    SIN_GUARD_SLOT(45, "VClient011")
    SIN_GUARD_SLOT(46, "VClient011")
    SIN_GUARD_SLOT(47, "VClient011")
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
