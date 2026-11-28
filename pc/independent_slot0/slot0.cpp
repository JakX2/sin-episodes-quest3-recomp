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
#include <new>
#if defined(SIN_ROLE_client) || defined(SIN_ROLE_server)
#include "sound_emitter_connect_core.h"
#endif
#if defined(SIN_ROLE_client)
#include "client_material_startup_core.h"
#include "native_client_prediction_factory_boundary.h"
#include "native_client_prediction_convar_registration.h"
#include "native_original_client_entitylist_empty_factory.h"
#include "native_original_client_centerprint_cold_factory.h"
#include "client_screen_random_core.h"
#include "client_cvar_probe.h"
#include "client_owned_startup_probe.h"
#include "client_vgui_vtable_probe.h"
#include "client_engine_vgui_slot1_probe.h"
#include "client_vgui_controls_bootstrap.h"
#include "client_mat_surface_query.h"
#include "client_known_gamesystem_init.h"
#include "client_stats_live_asset_probe.h"
#include "client_engine_filesystem_read.h"
#include "client_soundscape_retail_gate.h"
#include "client_soundscape_original_convar_cache.h"
#include "client_original_physics_factory_probe.h"
#include "client_original_surfaceprops_engine_fs_bridge.h"
#include "client_original_surfaceprops_registration.h"
#include "client_original_surfaceprops_readback.h"
#include "client_original_surfacephysics_data_probe.h"
#include "client_soundscape_diagnostic_formatter.h"
#if defined(SIN_SYNTHETIC_SLOT33_OWNED)
#include "client_bf_read_candidate_layout.h"
#endif
#endif
#if defined(SIN_ROLE_server)
#include "server_gameents_abi.h"
#include "server_original_physics_factory_probe.h"
#include "server_original_surfaceprops_readonly.h"
#include "server_tick_interval_core.h"
#include "sin_user_message_catalog.h"
#include "server_post_connect_core.h"
#include "server_single_cvar_probe.h"
#include "server_batch_cvar_probe.h"
#if defined(SIN_NATIVE_GAME_FRAME)
#include "native_server_game_frame.h"
#include "native_server_startup.h"
#include "native_server_level.h"
extern "C" const sin_port::native_level::Api* __cdecl SiN_GetServerLevelApi(std::uint32_t,std::uint32_t) noexcept;
int __cdecl SinLevelInitWithEventsInternal(sin_port::native_level::State*,const sin_port::native_level::LevelArgs*,
    const sin_port::native_level::Bindings*,sin_port::native_level::Result*,void*) noexcept;
int __cdecl SinLevelInitWithFiltersInternal(sin_port::native_level::State*,const sin_port::native_level::LevelArgs*,
    const sin_port::native_level::Bindings*,sin_port::native_level::Result*,void*,const sin_port::native_map::CampaignFilterFactoryBindings*) noexcept;
int __cdecl SinLevelInitWithSequencesInternal(sin_port::native_level::State*,const sin_port::native_level::LevelArgs*,
    const sin_port::native_level::Bindings*,sin_port::native_level::Result*,void*,const sin_port::native_map::ScriptedSequenceMapBindings*) noexcept;
int __cdecl SinServerActivateWithSequencesInternal(sin_port::native_level::State*,const sin_port::native_level::Bindings*,
    sin_port::native_level::Result*,void*,const sin_port::native_level::ScriptedSequenceActivationRoute*,sin_port::native_level::ScriptedSequenceActivationSummary*) noexcept;
int __cdecl SinResolveLocalPlayerInternal(void*,void*,void**) noexcept;
bool __cdecl SinReadCampaignContactFieldsInternal(const void*,unsigned,unsigned*,unsigned char*) noexcept;
extern "C" int __cdecl SiN_StartCampaignTriggerOutput(void*,unsigned,void*,unsigned,const sin_port::native_map::CampaignStartTouchBindings*,
    sin_port::native_map::CampaignStartTouchResult*) noexcept;
extern "C" int __cdecl SiN_StartCampaignTriggerOutputWithFilter(void*,unsigned,void*,unsigned,const sin_port::native_map::CampaignStartTouchBindings*,
    sin_port::native_map::CampaignStartTouchResult*,const sin_port::native_map::CampaignFilterBindings*,sin_port::native_map::CampaignFilterResult*) noexcept;
int __cdecl SinServerActivateWithEventsInternal(sin_port::native_level::State*,const sin_port::native_level::Bindings*,
    sin_port::native_level::Result*,void*) noexcept;
int __cdecl SinServerActivateWithCampaignFiltersInternal(sin_port::native_level::State*,const sin_port::native_level::Bindings*,
    sin_port::native_level::Result*,void*,const sin_port::native_map::CampaignFilterActivationBindings*,sin_port::native_level::CampaignFilterActivationSummary*) noexcept;
int __cdecl SinTouchCampaignTriggerFromStartupInternal(void*,unsigned,void*,const sin_port::native_map::CampaignTriggerBindings*,
    void*,sin_port::native_map::CampaignTriggerResult*,int*,void*,void*) noexcept;
extern "C" const sin_port::native_server_startup::Api* __cdecl SiN_GetServerStartupApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" const sin_port::native_game_frame::Api* __cdecl SiN_GetServerGameFrameApi(std::uint32_t,std::uint32_t) noexcept;
int __cdecl SinGameFrameWithSequencesInternal(bool,const sin_port::native_game_frame::State*,const sin_port::native_game_frame::Bindings*,sin_port::native_game_frame::Result*,
    const sin_port::native_game_frame::ScriptedSequenceFrameBindings*,sin_port::native_game_frame::ScriptedSequenceFrameResult*) noexcept;
int __cdecl SinGameFrameWithSequenceRemovalInternal(bool,const sin_port::native_game_frame::State*,const sin_port::native_game_frame::Bindings*,sin_port::native_game_frame::Result*,
    const sin_port::native_game_frame::ScriptedSequenceFrameBindings*,sin_port::native_game_frame::ScriptedSequenceFrameResult*,
    const sin_port::native_map::ScriptedSequenceRemovalThinkBindings*,sin_port::native_game_frame::ScriptedSequenceFrameRemovalResult*) noexcept;
int __cdecl SinGameFrameWithFiltersInternal(bool,const sin_port::native_game_frame::State*,const sin_port::native_game_frame::Bindings*,
    sin_port::native_game_frame::Result*,const sin_port::native_map::CampaignFilterInputBindings*,sin_port::native_weapon_tables::CampaignFilterServiceSummary*) noexcept;
#endif
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

#if defined(SIN_ROLE_client) && defined(SIN_SYNTHETIC_SLOT33_OWNED)
// Fixture-only state: never compiled into the published client.dll.
// Installed reader must be the address of THIS owned test object,
// not an arbitrary engine-supplied bf_read*. Unknown pointers are refused
// before dereference, and only explicitly installed test storage is read.
static sin_independent::stats_owned_dispatch::Dispatcher g_slot33Owned{};
static sin_independent::bf_read_candidate::Layout g_slot33Reader{};
static sin_independent::bf_read_candidate::TrustedBytes g_slot33Trusted{};
static bool g_slot33Ready=false;
#endif

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

#if defined(SIN_ROLE_client)
// Phase1686: this is an opt-in PRODUCTION client.dll factory boundary,
// not the original game implementation. The actual engine first asks
// client CreateInterface("VClientPrediction001"), then calls vtable[1].
// The original full-SHA engine.dll evidence is Phase1658.
//
// The owner diagnostic gate is per-process AND per-launch scoped;
// ordinary VClient011 Init still returns failure and does not publish
// partially reconstructed IPrediction methods.
static bool clientPredictionFactoryProbeEnabled() noexcept {
    char flag[2]={};
    return validTraceSession() &&
        GetEnvironmentVariableA(
            "SIN_INDEPENDENT_CLIENT_PREDICTION_FACTORY_PROBE",
            flag,sizeof(flag))==1 && flag[0]=='1';
}
namespace pred_factory=sin_independent::native_client_prediction_factory;
static pred_factory::Boundary* g_predictionOwner=nullptr;
static bool g_predictionOwnerReady=false;
namespace owned_predict=
    sin_independent::native_client_prediction_convar_registration;
static owned_predict::Owner* g_predictionConVarOwner=nullptr;
// Phase1689: source-verified embedded entity-list factory is a
// separate explicit diagnostic gate layered AFTER prediction.
// Original engine.dll requests VClientEntityList003 at
// 0x20023D24 immediately after Prediction slot1 at 0x20023D21.
// This provides an authentically placed 3+9 native virtual
// interface on a genuinely empty owner, NOT live entities.
namespace native_entitylist=
    sin_independent::native_original_client_entitylist_factory;
static native_entitylist::Owner* g_entityListOwner=nullptr;
static bool g_entityListReady=false;
static bool clientEntityListFactoryProbeEnabled() noexcept {
    char flag[2]={};
    return clientPredictionFactoryProbeEnabled() &&
        GetEnvironmentVariableA(
            "SIN_INDEPENDENT_CLIENT_ENTITYLIST_FACTORY_PROBE",
            flag,sizeof(flag))==1 && flag[0]=='1';
}
static bool bindEmptyNativeEntityList() noexcept {
    if(!clientEntityListFactoryProbeEnabled())return false;
    HMODULE retainedModule=nullptr;
    // The engine may retain the embedded interface pointer after
    // its original factory request; never unmap its virtual methods.
    if(!GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|
            GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCSTR>(&bindEmptyNativeEntityList),
            &retainedModule) || !retainedModule)return false;
    if(!g_entityListOwner)
        g_entityListOwner=new(std::nothrow)
            native_entitylist::Owner(&stopUnsupported);
    if(!g_entityListOwner)return false;
    auto* const selected=g_entityListOwner->BindEmpty();
    const auto parent=reinterpret_cast<std::uintptr_t>(g_entityListOwner);
    const auto child=reinterpret_cast<std::uintptr_t>(selected);
    const auto raw=reinterpret_cast<std::uintptr_t>(
        g_entityListOwner->RawAddress());
    if(child-parent!=native_entitylist::kOriginalSecondaryOffset ||
       raw-parent!=native_entitylist::kOriginalRawOffset ||
       !selected->Active()) {
        g_entityListOwner->Revoke();
        return false;
    }
    g_entityListReady=true;
    diagnostic("client-entitylist-empty-bound",
               "secondary-plus-0x10014:9-slots");
    return true;
}
// Phase1690: original engine makes an unconditional NEXT factory
// request for VCENTERPRINT002 after the entity-list interface.
// Source wrapper has eight native virtual methods, nullable
// service pointer at +4, and source-correct empty-cold behavior.
// Real centerprint GUI service construction (slot6) is terminal.
namespace native_centerprint=
    sin_independent::native_original_client_centerprint;
static native_centerprint::ColdWrapper* g_centerprintOwner=nullptr;
static bool g_centerprintReady=false;
static bool clientCenterprintFactoryProbeEnabled() noexcept {
    char flag[2]={};
    return clientEntityListFactoryProbeEnabled() &&
        GetEnvironmentVariableA(
            "SIN_INDEPENDENT_CLIENT_CENTERPRINT_FACTORY_PROBE",
            flag,sizeof(flag))==1 && flag[0]=='1';
}
static bool bindColdClientCenterprint() noexcept {
    if(!clientCenterprintFactoryProbeEnabled() ||
       !g_entityListReady || !g_entityListOwner ||
       !g_entityListOwner->Active())return false;
    HMODULE retainedModule=nullptr;
    // The engine captures this vptr after CreateInterface and may
    // retain it beyond the client module's temporary loader ref.
    if(!GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|
            GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCSTR>(&bindColdClientCenterprint),
            &retainedModule) || !retainedModule)return false;
    if(!g_centerprintOwner)
        g_centerprintOwner=new(std::nothrow)
            native_centerprint::ColdWrapper(&stopUnsupported,&diagnostic);
    if(!g_centerprintOwner)return false;
    g_centerprintOwner->BindCold();
    g_centerprintReady=true;
    diagnostic("client-centerprint-cold-bound",
               "nullable-service:8-slots");
    return true;
}
// Phase1691: unlike name-only VSoundEmitter001 factory acquisition,
// the source original VClient011.Init at 0x240FE31B actually invokes
// the emitter's first virtual Connect(appSystemFactory) and tests the
// native bool at 0x240FE322..0x240FE324. A false result branches to
// original Init failure at 0x240FE59E. Reuse the already authenticated
// native sound-emitter adapter, but ONLY in a new explicit owner
// continuation gate. Existing standalone sound probe remains terminal.
static bool clientSoundConnectContinueProbeEnabled() noexcept {
    char flag[2]={};
    return clientPredictionFactoryProbeEnabled() &&
        GetEnvironmentVariableA(
            "SIN_INDEPENDENT_CLIENT_SOUND_CONNECT_CONTINUE_PROBE",
            flag,sizeof(flag))==1 && flag[0]=='1';
}
static bool clientPredictionCvarRegistrationEnabled() noexcept {
    char flag[2]={};
    return clientPredictionFactoryProbeEnabled() &&
        GetEnvironmentVariableA(
            "SIN_INDEPENDENT_CLIENT_PREDICTION_CVAR_REGISTER_PROBE",
            flag,sizeof(flag))==1 && flag[0]=='1';
}
static pred_factory::Boundary* ensurePredictionOwner() noexcept {
    if(!g_predictionOwner)
        g_predictionOwner=new(std::nothrow)
            pred_factory::Boundary(&stopUnsupported);
    return g_predictionOwner;
}
// Host-only bounded read: do not infer safety from a nonnull pointer.
// The actual 2006 engine ConVar is expected to remain mapped; this
// helper proves only accessible Win32 pages, not original ownership.
static bool readablePredictionSource(const void* pointer,
                                    std::size_t length) noexcept {
    if(!pointer || !length)return false;
    MEMORY_BASIC_INFORMATION info{};
    if(VirtualQuery(pointer,&info,sizeof(info))!=sizeof(info) ||
       info.State!=MEM_COMMIT ||
       (info.Protect&(PAGE_NOACCESS|PAGE_GUARD)))return false;
    const DWORD access=info.Protect&0xffu;
    if(access!=PAGE_READONLY && access!=PAGE_READWRITE &&
       access!=PAGE_WRITECOPY && access!=PAGE_EXECUTE_READ &&
       access!=PAGE_EXECUTE_READWRITE &&
       access!=PAGE_EXECUTE_WRITECOPY)return false;
    const auto at=reinterpret_cast<std::uintptr_t>(pointer);
    const auto base=reinterpret_cast<std::uintptr_t>(info.BaseAddress);
    return at>=base &&
           (at-base)<=info.RegionSize &&
           length<=info.RegionSize-(at-base);
}
// Phase1691: reject bogus app-system factory pointers BEFORE invoking
// source VSoundEmitter001 virtual slot0 in an owner diagnostic run.
// Readable/executable page boundaries alone cannot authenticate that an
// engine pointer is genuinely an ISoundEmitterSystemBase; this is a
// conservative missing/forged pointer check, not a C++ RTTI proof.
// The real source code may dereference a bad object and crash; the
// independent DLL deliberately returns Init failure instead.
static bool validOwnerSoundConnectSource(void* sound) noexcept {
    if(!readablePredictionSource(sound,sizeof(void*)))return false;
    const void* const* table=nullptr;
    std::memcpy(&table,sound,sizeof(table));
    if(!readablePredictionSource(table,sizeof(void*)))return false;
    const void* entry=nullptr;
    std::memcpy(&entry,table,sizeof(entry));
    if(!entry)return false;
    MEMORY_BASIC_INFORMATION info{};
    if(VirtualQuery(entry,&info,sizeof(info))!=sizeof(info) ||
       info.State!=MEM_COMMIT ||
       (info.Protect&(PAGE_NOACCESS|PAGE_GUARD)))return false;
    const DWORD access=info.Protect&0xffu;
    return access==PAGE_EXECUTE ||
           access==PAGE_EXECUTE_READ ||
           access==PAGE_EXECUTE_READWRITE ||
           access==PAGE_EXECUTE_WRITECOPY;
}
// Query only the already-acquired real VEngineCvar002 virtual slot3,
// source-authenticated via the existing console_abi ABI. Do not write
// or register any ConVar. The original slot1 checks cl_predict's
// m_pParent->m_nValue; our native owner SNAPSHOTS that number rather
// than retaining an untrusted game-engine ConVar pointer.
// Perform only owner-gated engine registration when the original
// cl_predict is truly absent. The native ConVar is independently
// owned and never freed after any external registration attempt.
// This is a timing adaptation from original static constructor to
// client Init, and only the owner diagnostic launch can enable it.
static bool registerMissingPredictionConVar(void* engineCvar) noexcept {
    if(!clientPredictionCvarRegistrationEnabled() ||
       !readablePredictionSource(engineCvar,sizeof(void*)))
        return false;
    const void* const* table=nullptr;
    std::memcpy(&table,engineCvar,sizeof(table));
    if(!readablePredictionSource(table,4u*sizeof(void*)))
        return false;
    void* reg=nullptr;
    void* find=nullptr;
    std::memcpy(&reg,table+0u,sizeof(reg));
    std::memcpy(&find,table+3u,sizeof(find));
    if(!reg || !find)return false;
    // Once VEngineCvar002 has received a native C++ ConVar
    // vptr, it may retain that pointer beyond a client shutdown.
    // Pin this independent client.dll for the remainder of the
    // explicitly requested diagnostic process, so a later
    // FreeLibrary cannot invalidate engine-visible virtual code.
    HMODULE retainedModule=nullptr;
    if(!GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|
        GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCSTR>(&registerMissingPredictionConVar),
        &retainedModule) || !retainedModule)
        return false;
    if(!g_predictionConVarOwner)
        g_predictionConVarOwner=new(std::nothrow)
            owned_predict::Owner();
    if(!g_predictionConVarOwner)return false;
    const bool registered=
        g_predictionConVarOwner->RegisterMissing(
           engineCvar,
           reinterpret_cast<owned_predict::Owner::Register>(reg),
           reinterpret_cast<owned_predict::Owner::Find>(find));
    diagnostic(registered?
        "prediction-convar-registered":"prediction-convar-register-refused",
        owned_predict::kOriginalName);
    return registered;
}
static bool readPredictionNumeric(void* engineCvar,
                                  std::int32_t& result) noexcept {
    result=0;
    if(!readablePredictionSource(engineCvar,sizeof(void*)))
        return false;
    const void* const* vptr=nullptr;
    std::memcpy(&vptr,engineCvar,sizeof(vptr));
    if(!readablePredictionSource(vptr,4u*sizeof(void*)))
        return false;
    void* fn=nullptr;
    std::memcpy(&fn,vptr+3u,sizeof(fn));
    if(!fn)return false;
    using FindConstVar=const void* (__thiscall *)(
        const void*,const char*) noexcept;
    const auto selected=
        reinterpret_cast<FindConstVar>(fn)(engineCvar,"cl_predict");
    if(!readablePredictionSource(selected,
         pred_factory::kOriginalConVarParentOffset+sizeof(void*)))
        return false;
    const auto* source=static_cast<const std::uint8_t*>(selected);
    const void* parent=nullptr;
    std::memcpy(&parent,source+
       pred_factory::kOriginalConVarParentOffset,sizeof(parent));
    if(!parent)return false;
    const auto* parentBytes=static_cast<const std::uint8_t*>(parent);
    if(!readablePredictionSource(parentBytes,
       pred_factory::kOriginalConVarNumericOffset+
       sizeof(std::int32_t)))return false;
    std::memcpy(&result,parentBytes+
        pred_factory::kOriginalConVarNumericOffset,sizeof(result));
    return true;
}
#endif

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
static bool retailServerPhysicsFactoryProbeEnabled() noexcept {
    char enabled[2]={};
    // A distinct owner terminal diagnostic, never enabled by normal
    // ServerGameDLL003::DLLInit, client probe, or server sound probe.
    return extraFactoriesEnabled() &&
        GetEnvironmentVariableA(
            "SIN_INDEPENDENT_SERVER_PHYSICS_FACTORY_PROBE",
            enabled,2)==1 && enabled[0]=='1';
}
static bool retailServerPhysicsResourcesEnabled() noexcept {
    char enabled[2]={};
    return retailServerPhysicsFactoryProbeEnabled() &&
        GetEnvironmentVariableA(
            "SIN_INDEPENDENT_SERVER_PHYSICS_RESOURCES_PROBE",
            enabled,2)==1 && enabled[0]=='1';
}
[[noreturn]] static void stopAfterServerPhysicsFactoryProbe(
    Factory physicsFactory,void* filesystem) noexcept {
    diagnostic("server-physics-entered",
               "original-server-second-factory:read-only");
    const auto r=sin_independent::server_original_physics_factory::
        ReadOnlyThree(physicsFactory);
    char bounded[128]={};
    std::snprintf(bounded,sizeof(bounded),"%s:%u:%u:%u",
        sin_independent::server_original_physics_factory::Name(r.outcome),
        r.attempted,r.found,static_cast<unsigned>(r.original_query_gate));
    diagnostic("server-physics-result",
        sin_independent::server_original_physics_factory::Consistent(r)?
            bounded:"original-server-physics-inconsistent:0:0:0");
    diagnostic("server-physics-terminal-stop",
               "no-manifest-coupling-no-server-init");
    if(r.successful() && retailServerPhysicsResourcesEnabled()) {
        // The server original at 0x22234060 reads the SAME surface
        // manifest rb/GAME and its two per-file sources rb/null scope.
        // Do not perform the server's original slot1 mutating parse yet.
        diagnostic("server-surfaceprops-entered",
                   "VFileSystem012:server-original:rb-dual-scope");
        const auto resources=
            sin_independent::server_original_surfaceprops::ReadOnlyAfterFactory(
                r,filesystem);
        char status[192]={};
        std::snprintf(status,sizeof(status),"%s:%u:%u:%u:%u:%u",
            sin_independent::server_original_surfaceprops::Name(resources.outcome),
            resources.attempted,resources.read,resources.definitions,
            resources.base_keys,resources.repeated_names);
        diagnostic("server-surfaceprops-result",status);
        diagnostic("server-surfaceprops-terminal-stop",
                   "read-only-manifest-no-server-init");
        TerminateProcess(GetCurrentProcess(),
            resources.verified()?0x53494E7Au:0x53494E7Bu);
        __fastfail(7);
    }
    TerminateProcess(GetCurrentProcess(),
        r.successful()?0x53494E78u:0x53494E79u);
    __fastfail(7);
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
static void* g_clientFileSystem=nullptr;
// Acquired as original fourth app-system interface; retained only while
// the opt-in diagnostic is running, never dereferenced by normal Init.
static void* g_clientCvar=nullptr;
// The original VClient011 Init argument #2 is the PHYSICS factory.
// Retained only for the short, opt-in terminal diagnostic while
// Init is still on its original engine-invoked call stack.
static Factory g_clientPhysicsFactory=nullptr;
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
static bool clientEngineFsProbeEnabled() noexcept {
    char enabled[2]={};
    return clientSoundSystemInitProbeEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_ENGINE_FS_PROBE",enabled,2)==1 &&
        enabled[0]=='1';
}
static bool clientEngineFsReadProbeEnabled() noexcept {
    char enabled[2]={};
    return clientEngineFsProbeEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_ENGINE_FS_READ_PROBE",enabled,2)==1 &&
        enabled[0]=='1';
}
static bool clientEngineFsStagedProbeEnabled() noexcept {
    char enabled[2]={};
    return clientEngineFsReadProbeEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_ENGINE_FS_STAGED_PROBE",enabled,2)==1 &&
        enabled[0]=='1';
}
static bool clientEngineFsHookProofEnabled() noexcept {
    char enabled[2]={};
    return clientEngineFsStagedProbeEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_ENGINE_FS_HOOK_PROBE",enabled,2)==1 &&
        enabled[0]=='1';
}
// Fourth optional read-only retail milestone. Requires ALL earlier
// session-bound 21-factory/VGUI/sound/Stats hook gates to have succeeded.
// Default independent client behavior remains unchanged.
static bool clientSoundscapeCatalogProbeEnabled() noexcept {
    char enabled[2]={};
    return clientEngineFsHookProofEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_SOUNDSCAPE_CATALOG_PROBE",
                                enabled,2)==1 &&
        enabled[0]=='1';
}
// Phase 497: opt-in ORIGINAL 2006 soundscape ICvar::FindVar source
// virtual-slot3 probe. Requires all previous session-bound safety gates
// AND the existing soundscape diagnostic. It cannot change exit codes.
static bool clientSoundscapeCvarLookupProbeEnabled() noexcept {
    char enabled[2]={};
    return clientSoundscapeCatalogProbeEnabled() &&
        GetEnvironmentVariableA(
            "SIN_INDEPENDENT_CLIENT_SOUNDSCAPE_CVAR_LOOKUP_PROBE",
            enabled,2)==1 && enabled[0]=='1';
}
// Independent source-authenticated 2006 CClientDLL physics triple:
 // only after the previously proven 7-resource and CVar diagnostic,
 // only with an additional explicit process opt-in.
static bool clientPhysicsFactoryProbeEnabled() noexcept {
    char enabled[2]={};
    return clientSoundscapeCvarLookupProbeEnabled() &&
        GetEnvironmentVariableA(
            "SIN_INDEPENDENT_CLIENT_PHYSICS_FACTORY_PROBE",
            enabled,2)==1 && enabled[0]=='1';
}
static bool clientSurfacePropsReadProbeEnabled() noexcept {
    char enabled[2]={};
    return clientPhysicsFactoryProbeEnabled() &&
        GetEnvironmentVariableA(
            "SIN_INDEPENDENT_CLIENT_SURFACEPROPS_READ_PROBE",
            enabled,2)==1 && enabled[0]=='1';
}
// Phase522: vslot5 GetSurfaceData, actual 2006 engine read-only material
// snapshot. Unlike Phase515, does NOT require or call ParseSurfaceData.
static bool clientSurfacePhysicsDataProbeEnabled() noexcept {
    char enabled[2]={};
    return clientSurfacePropsReadProbeEnabled() &&
        GetEnvironmentVariableA(
            "SIN_INDEPENDENT_CLIENT_SURFACEPHYSICS_DATA_PROBE",
            enabled,2)==1 && enabled[0]=='1';
}
// Phase 515: actual engine mutation requires an independent, separate opt-in.
// No old filesystem/physics/soundscape diagnostic can enable this implicitly.
static bool clientSurfacePropsRegisterProbeEnabled() noexcept {
    char enabled[2]={};
    return clientSurfacePropsReadProbeEnabled() &&
        GetEnvironmentVariableA(
            "SIN_INDEPENDENT_CLIENT_SURFACEPROPS_REGISTER_PROBE",
            enabled,2)==1 && enabled[0]=='1';
}
// SDK2004-guided slots 2/3 are distinct and unproven on this retail
// engine. Extra explicit opt-in only AFTER actual registration returns.
static bool clientSurfacePropsReadbackProbeEnabled() noexcept {
    char enabled[2]={};
    return clientSurfacePropsRegisterProbeEnabled() &&
        GetEnvironmentVariableA(
            "SIN_INDEPENDENT_CLIENT_SURFACEPROPS_READBACK_PROBE",
            enabled,2)==1 && enabled[0]=='1';
}
[[noreturn]] static void stopAfterClientSoundscapeCatalogProbe() noexcept {
    using namespace sin_independent::soundscape_retail_gate;
    diagnostic("engine-fs-soundscape-entered","rb-GAME:read-only");
    const Report result=Verify(g_clientFileSystem);
    char totals[96]={};
    std::snprintf(totals,sizeof(totals),"%u:%u:%u:%u",
        static_cast<unsigned>(result.counts.files),
        static_cast<unsigned>(result.counts.definitions),
        static_cast<unsigned>(result.counts.nested_references),
        static_cast<unsigned>(result.counts.unresolved_references));
    diagnostic("engine-fs-soundscape-counts",totals);
    diagnostic("engine-fs-soundscape-result",StatusName(result.status));
    // Format is fixed-size, path-free and carries only source-neutral
    // refusal categories, not the user's original scripts or binary data.
    char detail[256]={};
    const bool formatted=
        sin_independent::soundscape_diagnostic::Format(
            result,detail,sizeof(detail));
    diagnostic("engine-fs-soundscape-detail",
               formatted?detail:"diagnostic-format-refused");
    bool cvarBothFound=false;
    if(clientSoundscapeCvarLookupProbeEnabled()) {
        // Source vtable slot3 0x240F3C40 resolves only two ConVar
        // pointers. We log only bounded name-free status/count fields,
        // never real pointers, ConVar values or game file contents.
        diagnostic("engine-fs-soundscape-cvar-entered",
                   "VEngineCvar002:slot3:read-only");
        static sin_independent::soundscape_original_cvars::OwnedCache cache{};
        const auto outcome=cache.Resolve(g_clientCvar);
        cvarBothFound=outcome.mixer_cached&&outcome.dsp_cached;
        char summary[128]={};
        std::snprintf(summary,sizeof(summary),"%s:%u:%u:%u:%u",
            sin_independent::soundscape_original_cvars::Name(outcome.result),
            outcome.requested,outcome.found_this_call,
            static_cast<unsigned>(outcome.mixer_cached),
            static_cast<unsigned>(outcome.dsp_cached));
        diagnostic("engine-fs-soundscape-cvar-result",summary);
        diagnostic("engine-fs-soundscape-cvar-terminal-stop",
                   "no-convar-writes-no-game-init");
    }
    if(clientPhysicsFactoryProbeEnabled()) {
        // Original 0x24182C60 makes three cdecl Factory queries from
        // the VClient011::Init *second* factory argument. It would
        // publish returned pointers to 3 globals and call a separate
        // coupling helper after success. This stage ONLY queries and
        // logs bounded result counts, never publishes pointer values
        // or calls the unreconstructed coupling function 0x241927A0.
        diagnostic("engine-fs-physics-entered",
                   "physics-factory:three-original:read-only");
        const auto report=
            sin_independent::physics_factory_probe::ReadOnlyQueryThree(
                g_clientPhysicsFactory);
        char resultLine[128]={};
        std::snprintf(resultLine,sizeof(resultLine),"%s:%u:%u:%u",
            sin_independent::physics_factory_probe::Name(report.outcome),
            report.attempted,report.found,
            static_cast<unsigned>(report.would_pass_original_three_query_gate));
        diagnostic("engine-fs-physics-result",
                   sin_independent::physics_factory_probe::Consistent(report)?
                   resultLine:"original-physics-inconsistent:0:0:0");
        diagnostic("engine-fs-physics-terminal-stop",
                   "no-physics-coupling-no-game-init");
        if(clientSurfacePropsReadProbeEnabled() &&
           report.would_pass_original_three_query_gate &&
           cvarBothFound && result.verified()) {
            // Original post-physics helper 0x241927A0 reads the
            // surfaceproperties manifest rb/GAME; helper 0x24191EC0
            // opens each individual surface script rb/nullptr then
            // invokes VPhysicsSurfaceProps001 slot1. This stage
            // does ONLY the independent read/shape validation,
            // never the native registration virtual method.
            diagnostic("engine-fs-surfaceprops-entered",
                       "VFileSystem012:surfaceproperties:rb-dual-scope");
            sin_independent::surfaceprops_engine_fs::Bridge bridge{};
            const auto resources=bridge.ReadOnly(g_clientFileSystem);
            char shape[192]={};
            std::snprintf(shape,sizeof(shape),"%s:%u:%u:%u:%u:%u",
                sin_independent::surfaceprops_engine_fs::Name(
                    resources.status),
                resources.owned_result.attempted,
                resources.owned_result.read,
                resources.owned_result.definitions,
                resources.owned_result.base_keys,
                resources.owned_result.repeated_names);
            diagnostic("engine-fs-surfaceprops-result",shape);
            diagnostic("engine-fs-surfaceprops-terminal-stop",
                       "no-physics-surface-registration-no-game-init");
            if(resources.verified() &&
               clientSurfacePhysicsDataProbeEnabled()) {
                diagnostic("engine-fs-material-entered",
                           "VPhysicsSurfaceProps001:slots2-3-5:read-only");
                void* props=g_clientPhysicsFactory?
                    g_clientPhysicsFactory("VPhysicsSurfaceProps001",nullptr):
                    nullptr;
                const auto materials=
                    sin_independent::original_surfacephysics_data::ReadOnly(props);
                char report[144]={};
                std::snprintf(report,sizeof(report),"%s:%d:%u",
                    sin_independent::original_surfacephysics_data::Name(materials.outcome),
                    materials.count,materials.sampled);
                diagnostic("engine-fs-material-result",report);
                if(materials.verified()){
                    for(unsigned i=0;i<3;++i){
                        const auto p=materials.values[i];
                        char line[174]={};
                        std::snprintf(line,sizeof(line),"%s:%d:%.4g:%.4g:%.6g:%.4g:%.4g",
                            sin_independent::original_surfaceprops_readback::kNames[i],
                            materials.indices[i],
                            static_cast<double>(p.friction),
                            static_cast<double>(p.elasticity),
                            static_cast<double>(p.density),
                            static_cast<double>(p.thickness),
                            static_cast<double>(p.dampening));
                        diagnostic("engine-fs-material-sample",line);
                    }
                }
                diagnostic("engine-fs-material-terminal-stop",
                           "read-only-physics-fields-no-game-init");
            }
            if(resources.verified() &&
               clientSurfacePropsRegisterProbeEnabled()) {
                // Original 0x24191EC0 closes both source files before
                // calling VPhysicsSurfaceProps001 virtual slot1. Re-query
                // this interface ONLY inside the explicit opt-in stage.
                diagnostic("engine-fs-registration-entered",
                           "VPhysicsSurfaceProps001:slot1:explicit-opt-in");
                void* surfaceProps=g_clientPhysicsFactory?
                    g_clientPhysicsFactory("VPhysicsSurfaceProps001",nullptr):
                    nullptr;
                // PHASE 519: the real owner-side engine returned 94
                // surface entries and 0:1:85 indices after two Parse calls,
                // but the initial table size was not observed. Capture it
                // BEFORE any mutation so later results can establish whether
                // this attempt changed the engine's surface table at all.
                // Readback is already separately explicitly opted in.
                if(clientSurfacePropsReadbackProbeEnabled()) {
                    diagnostic("engine-fs-preparse-entered",
                               "VPhysicsSurfaceProps001:slots2-3:before-parse");
                    const auto before=
                        sin_independent::original_surfaceprops_readback::Query(
                            surfaceProps);
                    char baseline[192]={};
                    std::snprintf(baseline,sizeof(baseline),"%s:%d:%d:%d:%d",
                        sin_independent::original_surfaceprops_readback::Name(
                            before.outcome),
                        before.count,before.indices[0],
                        before.indices[1],before.indices[2]);
                    diagnostic("engine-fs-preparse-result",baseline);
                    diagnostic("engine-fs-preparse-terminal-stop",
                               "read-only-before-original-surface-parse");
                }
                const auto registration=
                    sin_independent::original_surfaceprops_registration::Attempt(
                        g_clientFileSystem,surfaceProps);
                char registered[192]={};
                std::snprintf(registered,sizeof(registered),
                    "%s:%u:%u:%d:%d",
                    sin_independent::original_surfaceprops_registration::Name(
                        registration.outcome),
                    registration.source_reads,registration.calls_returned,
                    registration.first_return,registration.second_return);
                diagnostic("engine-fs-registration-result",registered);
                diagnostic("engine-fs-registration-terminal-stop",
                           "two-engine-call-returns-not-game-init");
                if(registration.completed_calls() &&
                   clientSurfacePropsReadbackProbeEnabled()) {
                    diagnostic("engine-fs-readback-entered",
                               "VPhysicsSurfaceProps001:slots2-3:read-only");
                    const auto state=
                        sin_independent::original_surfaceprops_readback::Query(
                            surfaceProps);
                    char status[192]={};
                    std::snprintf(status,sizeof(status),"%s:%d:%d:%d:%d",
                        sin_independent::original_surfaceprops_readback::Name(
                            state.outcome),
                        state.count,state.indices[0],
                        state.indices[1],state.indices[2]);
                    diagnostic("engine-fs-readback-result",status);
                    diagnostic("engine-fs-readback-terminal-stop",
                               "SDK2004-count-and-indices-not-game-init");
                }
            }
        }
    }
    diagnostic("engine-fs-soundscape-terminal-stop",
               "owner-original-files-no-game-init");
    TerminateProcess(GetCurrentProcess(),
        result.verified()?0x53494E76u:0x53494E77u);
    __fastfail(7);
}
[[noreturn]] static void stopAfterClientEngineFsProbe() noexcept {
    using namespace sin_independent::engine_fs;
    diagnostic("engine-fs-presence-entered","VFileSystem012:source2004");
    auto status=Presence(g_clientFileSystem);
    diagnostic("engine-fs-presence-result",Name(status));
    DWORD code=status==Status::BothPresent?0x53494E68u:
               status==Status::InvalidAbi?0x53494E6Au:0x53494E69u;
    if(status==Status::BothPresent && clientEngineFsReadProbeEnabled()) {
        diagnostic("engine-fs-read-entered","VFileSystem012:read-only");
        Counts counts{};
        status=ReadAndValidate(g_clientFileSystem,&counts);
        char summary[96]={};
        std::snprintf(summary,sizeof(summary),"%u:%u:%u:%u",
            static_cast<unsigned>(counts.definitions),
            static_cast<unsigned>(counts.refs),
            static_cast<unsigned>(counts.pairs),
            static_cast<unsigned>(counts.dividers));
        diagnostic("engine-fs-read-counts",summary);
        diagnostic("engine-fs-read-result",Name(status));
        code=status==Status::ParsedAndVerified?0x53494E6Bu:
             status==Status::UnexpectedCounts?0x53494E6Du:0x53494E6Cu;
        if(status==Status::ParsedAndVerified &&
           clientEngineFsStagedProbeEnabled()) {
            diagnostic("engine-fs-staged-entered","rb-GAME:rt-null:owned-only");
            StagedCounts staged{};
            const auto outcome=ReadBindAndValidateStaged(g_clientFileSystem,&staged);
            char stagedCounts[96]={};
            std::snprintf(stagedCounts,sizeof(stagedCounts),"%u:%u:%u:%u:%u",
                static_cast<unsigned>(staged.catalog),
                static_cast<unsigned>(staged.definitions),
                static_cast<unsigned>(staged.refs),
                static_cast<unsigned>(staged.pairs),
                static_cast<unsigned>(staged.dividers));
            diagnostic("engine-fs-staged-counts",stagedCounts);
            char stages[48]={};
            std::snprintf(stages,sizeof(stages),"%u:%u",
                static_cast<unsigned>(staged.completed),
                static_cast<unsigned>(staged.attempted));
            diagnostic("engine-fs-staged-stage",stages);
            diagnostic("engine-fs-staged-result",StagedName(outcome));
            code=outcome==StagedStatus::OwnedStagedVerified?0x53494E70u:
                 outcome==StagedStatus::DefinitionsRefused?0x53494E71u:
                 outcome==StagedStatus::LayoutRefused?0x53494E72u:
                 0x53494E73u;
            diagnostic("engine-fs-staged-terminal-stop","original-modes-owned-only");
            if(clientEngineFsHookProofEnabled()) {
                // This independent-only probe cannot call original hooks
                // or claim network message delivery. It verifies the
                // identities installed in OUR rebuilt Stats owner after
                // actual retail engine staged file reads.
                diagnostic("engine-fs-hooks-entered","CUserMessages:owned-only");
                char hookIds[48]={};
                std::snprintf(hookIds,sizeof(hookIds),"%d:%d:%d",
                    staged.hook_ids[0],staged.hook_ids[1],staged.hook_ids[2]);
                diagnostic("engine-fs-hooks-ids",hookIds);
                char count[24]={};
                std::snprintf(count,sizeof(count),"%u",
                    static_cast<unsigned>(staged.owned_user_message_hooks));
                diagnostic("engine-fs-hooks-count",count);
                const bool verified=outcome==StagedStatus::OwnedStagedVerified &&
                    staged.owned_user_message_hooks==3 &&
                    staged.hook_ids[0]==27 && staged.hook_ids[1]==28 &&
                    staged.hook_ids[2]==5 && staged.completed==7 &&
                    staged.attempted==7;
                diagnostic("engine-fs-hooks-result",
                    verified?"three-owned-hooks-bound-not-dispatched":"refused");
                code=verified?0x53494E74u:0x53494E75u;
                diagnostic("engine-fs-hooks-terminal-stop","independent-only");
                if(verified && clientSoundscapeCatalogProbeEnabled())
                    stopAfterClientSoundscapeCatalogProbe();
            }
            TerminateProcess(GetCurrentProcess(),code);
            __fastfail(7);
        }
    }
    diagnostic("engine-fs-terminal-stop","read-only-diagnostic");
    TerminateProcess(GetCurrentProcess(),code);
    __fastfail(7);
}
static bool clientStatsAssetProbeEnabled() noexcept {
    char enabled[2]={};
    return clientSoundSystemInitProbeEnabled() &&
        GetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_STATS_ASSET_PROBE",enabled,2)==1 &&
        enabled[0]=='1';
}
[[noreturn]] static void stopAfterClientStatsAssetProbe() noexcept {
    diagnostic("stats-asset-probe-entered","scripts/stats");
    wchar_t contentRoot[1024]={};
    const DWORD length=GetEnvironmentVariableW(
        L"SIN_INDEPENDENT_CLIENT_STATS_ROOT",contentRoot,1024);
    sin_independent::stats_live_probe::Counts counts{};
    auto result=sin_independent::stats_live_probe::Result::MissingAbsoluteRoot;
    if(length>0 && length<1024) {
        try {
            const std::filesystem::path root(contentRoot);
            result=sin_independent::stats_live_probe::Validate(&root,&counts);
        } catch(...) {
            result=sin_independent::stats_live_probe::Result::ReadOrParseRefused;
        }
    }
    char summary[96]={};
    std::snprintf(summary,sizeof(summary),"%u:%u:%u:%u",
        static_cast<unsigned>(counts.definitions),
        static_cast<unsigned>(counts.percentage_refs),
        static_cast<unsigned>(counts.display_pairs),
        static_cast<unsigned>(counts.dividers));
    diagnostic("stats-asset-probe-counts",summary);
    diagnostic("stats-asset-probe-result",
        sin_independent::stats_live_probe::Name(result));
    const DWORD code=result==sin_independent::stats_live_probe::Result::Verified?
        0x53494E65u:
        result==sin_independent::stats_live_probe::Result::UnexpectedRetailCounts?
        0x53494E67u:0x53494E66u;
    diagnostic("stats-asset-probe-terminal-stop","independent-content-io");
    TerminateProcess(GetCurrentProcess(),code);
    __fastfail(7);
}
[[noreturn]] static void stopAfterClientSoundSystemInit() noexcept {
    diagnostic("sound-system-init-entered","CSoundEmitterSystem:VSoundEmitter001+1");
    const auto result=sin_independent::known_game_system::InitSoundEmitter(g_clientSoundEmitter);
    diagnostic("sound-system-init-result",sin_independent::known_game_system::Name(result));
    if(result==sin_independent::known_game_system::CallResult::CalledTrue &&
       clientEngineFsProbeEnabled())stopAfterClientEngineFsProbe();
    if(result==sin_independent::known_game_system::CallResult::CalledTrue &&
       clientStatsAssetProbeEnabled())stopAfterClientStatsAssetProbe();
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
        g_clientFileSystem=nullptr;
        g_clientCvar=nullptr;
        g_clientPhysicsFactory=physicsFactory;
        g_predictionOwnerReady=false;
        if(g_predictionOwner)g_predictionOwner->Revoke();
        g_entityListReady=false;
        if(g_entityListOwner)g_entityListOwner->Revoke();
        g_centerprintReady=false;
        if(g_centerprintOwner)g_centerprintOwner->Revoke();
        if (!require(appSystemFactory, kFirstEngineVersion, &engine) ||
            !require(appSystemFactory, kSecondEngineVersion) ||
            !require(appSystemFactory, kThirdEngineVersion) ||
            !require(appSystemFactory, kFourthEngineVersion, &cvar) ||
            !require(appSystemFactory, kFifthEngineVersion)) return 0;
        g_clientCvar=cvar; // non-owning pointer, only used by opt-in stage
        void* sound=nullptr;
        void* materials=nullptr;
        void* uniform=nullptr;
        for (const char* version : kRemainingClientVersions)
            if (!require(appSystemFactory, version,
                         std::strcmp(version,"VSoundEmitter001")==0 ? &sound :
                         std::strcmp(version,"VMaterialSystem076")==0 ? &materials :
                         std::strcmp(version,"VEngineRandom001")==0 ? &uniform :
                         std::strcmp(version,"VEngineVGui001")==0 ? &g_engineVgui :
                         std::strcmp(version,"VFileSystem012")==0 ? &g_clientFileSystem : nullptr))
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
        // Original 2006 PE32 Init source invokes the sound emitter
        // Connect callback immediately after the twenty-first
        // required factory acquisition, and can fail before any
        // prediction or entity-list interfaces are requested.
        // Do not assume a returned VSoundEmitter001 pointer alone
        // means a successfully connected sound app system.
        if(clientSoundConnectContinueProbeEnabled()) {
            if(!validOwnerSoundConnectSource(sound)) {
                diagnostic("client-sound-connect-continue-refused",
                           "invalid-sound-vtable");
                return 0;
            }
            if(!connectSound(sound,appSystemFactory)) {
                diagnostic("client-sound-connect-continue-refused",
                           "VSoundEmitter001");
                return 0;
            }
            diagnostic("client-sound-connect-continued",
                       "VSoundEmitter001");
        }
#endif
        // Explicit owner diagnostic only. This advances the same
        // independently compiled VClient011 factory through its
        // actual slot0 so the 2006 engine can request the actual
        // VClientPrediction001 factory after all known dependencies.
        // No other gameplay callbacks are silently implemented.
        if(clientPredictionFactoryProbeEnabled()) {
            std::int32_t numeric=0;
            auto* owner=ensurePredictionOwner();
            bool gotNumeric=owner && readPredictionNumeric(cvar,numeric);
            if(owner && !gotNumeric &&
               registerMissingPredictionConVar(cvar))
                gotNumeric=readPredictionNumeric(cvar,numeric);
            if(owner && gotNumeric) {
                owner->BindSourceNumeric(numeric);
                g_predictionOwnerReady=true;
                if(clientEntityListFactoryProbeEnabled() &&
                   !bindEmptyNativeEntityList()) {
                    g_predictionOwnerReady=false;
                    owner->Revoke();
                    diagnostic("client-entitylist-empty-refused",
                               "owner-layout-or-lifetime");
                    return 0;
                }
                if(clientCenterprintFactoryProbeEnabled() &&
                   !bindColdClientCenterprint()) {
                    g_centerprintReady=false;
                    if(g_centerprintOwner)g_centerprintOwner->Revoke();
                    g_entityListReady=false;
                    if(g_entityListOwner)g_entityListOwner->Revoke();
                    g_predictionOwnerReady=false;
                    owner->Revoke();
                    diagnostic("client-centerprint-cold-refused",
                               "entitylist-or-owner-lifetime");
                    return 0;
                }
                seen_=true;
                diagnostic("prediction-bound","cl_predict-parent-value");
                return 1; // Explicit opt-in; remaining callbacks guard.
            }
            diagnostic("prediction-bound-refused",
                       "cl_predict-unavailable-or-unsafe");
            return 0;
        }
        diagnostic("sdk2004-preflight-only", nullptr);
        seen_ = true;
        return 0; // Default still never advertises startup.
    }
    virtual void __thiscall Shutdown() noexcept {
        seen_ = false;
        g_clientPhysicsFactory=nullptr;
        g_predictionOwnerReady=false;
        if(g_predictionOwner)g_predictionOwner->Revoke();
        g_entityListReady=false;
        if(g_entityListOwner)g_entityListOwner->Revoke();
        g_centerprintReady=false;
        if(g_centerprintOwner)g_centerprintOwner->Revoke();
#if defined(SIN_SYNTHETIC_SLOT33_OWNED)
        g_slot33Ready=false;
        g_slot33Reader={};
        g_slot33Trusted={};
        g_slot33Owned.Disable();
#endif
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
    // Original 2006 VClient011 vtable starts at 0x24296C6C: entry 33
    // is 0x240FDCD0 and forwards to CUserMessages::DispatchUserMessage
    // (0x241D0000). It takes (messageIndex,bf_read*) and returns bool,
    // with thiscall ret 8. The full handler graph is not rebuilt yet.
    virtual bool __thiscall DispatchUserMessage(
        int messageIndex, void* bitReader) noexcept {
#if defined(SIN_SYNTHETIC_SLOT33_OWNED)
        // Only the explicitly installed, internally owned fixture
        // reader is accepted. NO retail game pointer is dereferenced.
        if(!g_slot33Ready || bitReader!=static_cast<void*>(&g_slot33Reader))
            return false;
        const auto outcome=sin_independent::bf_read_candidate::
            DispatchOwnedCandidate(
                g_slot33Owned,messageIndex,g_slot33Reader,g_slot33Trusted);
        return outcome.delivered();
#else
        const bool known=messageIndex>=0 &&
            messageIndex<static_cast<int>(sin_independent::kUserMessageCount);
        diagnostic("client-user-message-dispatch-refused",
                   known && bitReader?"known-message-unbound":
                   "invalid-message-or-reader");
        return false; // No fabricated network message delivery.
#endif
    }
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
#if defined(SIN_NATIVE_GAME_FRAME)
struct FrameInvocation {
    const sin_port::native_game_frame::State* state;
    const sin_port::native_game_frame::Bindings* bindings;
    sin_port::native_game_frame::Result* result;
    bool entered=false;
    int success=0;
    const sin_port::native_map::CampaignFilterInputBindings* filters=nullptr;
    sin_port::native_weapon_tables::CampaignFilterServiceSummary* filter_summary=nullptr;
    const sin_port::native_game_frame::ScriptedSequenceFrameBindings* sequences=nullptr;
    sin_port::native_game_frame::ScriptedSequenceFrameResult* sequence_result=nullptr;
    const sin_port::native_map::ScriptedSequenceRemovalThinkBindings* sequence_removal=nullptr;
    sin_port::native_game_frame::ScriptedSequenceFrameRemovalResult* sequence_removed=nullptr;
};
static thread_local FrameInvocation* g_frame_invocation=nullptr;
struct StartupInvocation {
    sin_port::native_server_startup::State* state;
    const sin_port::native_server_startup::Bindings* bindings;
    sin_port::native_server_startup::BoundaryResult* result;
    sin_port::native_server_startup::EngineEventSession* events=nullptr;
};
static thread_local StartupInvocation* g_startup_invocation=nullptr;
struct GameInitInvocation {sin_port::native_server_startup::State* state;const sin_port::native_server_startup::GameInitBindings* bindings;sin_port::native_server_startup::GameInitResult* result;bool entered=false;};
static thread_local GameInitInvocation* g_gameinit_invocation=nullptr;
struct LevelInvocation {
    sin_port::native_level::State* state;
    const sin_port::native_level::Bindings* bindings;
    sin_port::native_level::Result* result;
    bool activating,entered=false;int success=0;
    void* event_manager=nullptr;
    const sin_port::native_map::CampaignFilterActivationBindings* filters=nullptr;
    sin_port::native_level::CampaignFilterActivationSummary* filter_summary=nullptr;
    const sin_port::native_map::CampaignFilterFactoryBindings* filter_factories=nullptr;
    const sin_port::native_map::ScriptedSequenceMapBindings* sequence_map=nullptr;
    const sin_port::native_level::ScriptedSequenceActivationRoute* sequence_activation=nullptr;
    sin_port::native_level::ScriptedSequenceActivationSummary* sequence_summary=nullptr;
};
static thread_local LevelInvocation* g_level_invocation=nullptr;
#endif
class Boundary final {
public:
    virtual bool __thiscall DLLInit(
        Factory engineFactory, Factory physicsFactory,
        Factory fileSystemFactory, CGlobalVars* globals) noexcept {
        diagnostic("init-entered", nullptr);
#if defined(SIN_NATIVE_GAME_FRAME)
        auto* invocation=g_startup_invocation;
        if(invocation){
            if(invocation->result->entered)return false;
            invocation->result->entered=1;
        }
#endif
        if (!engineFactory || !physicsFactory || !fileSystemFactory || !globals) {
            diagnostic("invalid-init-arguments", nullptr);
            return false;
        }
        // First SIXTEEN original DLLInit interface dependencies are now
        // authenticated against the pinned server. The filesystem request
        // uses the THIRD factory argument; the surrounding 15 use the
        // engine factory. Stop at the first unavailable pointer, never
        // dereference or retain it, and NEVER report successful Init.
        void* engineServer=nullptr;
        void* sound=nullptr;
        void* events=nullptr;
        void* cvar=nullptr;
        void* filesystem=nullptr;
        if (!require(engineFactory, kFirstEngineVersion, &engineServer) ||
            !require(engineFactory, kSecondEngineVersion) ||
            !require(engineFactory, kThirdEngineVersion, &cvar) ||
            !require(engineFactory, kFourthEngineVersion) ||
            !require(engineFactory, kFifthEngineVersion) ||
            !require(engineFactory, kSixthEngineVersion) ||
            !require(engineFactory, kSeventhEngineVersion) ||
            !require(engineFactory, kEighthEngineVersion) ||
            !require(engineFactory, kNinthEngineVersion) ||
            !require(engineFactory, kTenthEngineVersion) ||
            !require(fileSystemFactory, kFileSystemVersion, &filesystem) ||
            !require(engineFactory, kEleventhEngineVersion, &events) ||
            !require(engineFactory, kTwelfthEngineVersion) ||
            !require(engineFactory, kThirteenthEngineVersion, &sound) ||
            !require(engineFactory, kFourteenthEngineVersion) ||
            !require(engineFactory, kFifteenthEngineVersion)) return false;
#if defined(SIN_NATIVE_GAME_FRAME)
        if(invocation){
            auto& out=*invocation->result;out.acquired=16;
            if(sin_independent::TryConnectSoundEmitter(sound,engineFactory)!=sin_independent::SoundConnectResult::Connected)return false;
            out.sound_connected=1;
            const auto* api=SiN_GetServerStartupApi(1,sizeof(sin_port::native_server_startup::Api));
            if(!api)return false;
            auto bound=*invocation->bindings;
            // Trust the actual acquired interface pointers, not the
            // optional copies in the owner's binding.
            bound.globals=globals;bound.engine=engineFactory;bound.physics=physicsFactory;
            bound.filesystem=fileSystemFactory;bound.event_manager=events;bound.cvar_interface=cvar;bound.engine_server=engineServer;
            const bool ok=api->post_connect(invocation->state,&bound,&out.tail)!=0;
            if(ok&&invocation->events)
                *invocation->events={invocation->state,events,GetCurrentThreadId()};
            return ok;
        }
#endif
#if defined(SIN_SYNTHETIC_SOUND_CONNECT)
        if (!connectSound(sound,engineFactory)) return false;
#else
        if (retailServerPhysicsFactoryProbeEnabled())
            stopAfterServerPhysicsFactoryProbe(physicsFactory,filesystem);
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
#if defined(SIN_NATIVE_GAME_FRAME)
        if(auto* invocation=g_gameinit_invocation){
            if(invocation->entered)return false;invocation->entered=true;
            const auto* api=SiN_GetServerStartupApi(1,sizeof(sin_port::native_server_startup::Api));
            return api&&api->game_init(invocation->state,invocation->bindings,invocation->result);
        }
#endif
        diagnostic("gameinit-rejected", nullptr);
        return false;
    }
    // Original ServerGameDLL003 has 31 entries, not the SDK's 29.
    // Unimplemented slots remain terminal guards; verified methods are typed.
    // Original slot2 RET18, slot3 RET0c.
    virtual bool __thiscall LevelInit(const char* map,const char* entities,const char* old_level,const char* landmark,bool load_game,bool background) noexcept {
#if defined(SIN_NATIVE_GAME_FRAME)
        if(auto* invocation=g_level_invocation){
            if(invocation->entered||invocation->activating)return false;invocation->entered=true;
            const auto* api=SiN_GetServerLevelApi(1,sizeof(sin_port::native_level::Api));
            const sin_port::native_level::LevelArgs args{map,entities,old_level,landmark,load_game,background};
            if(invocation->sequence_map)
                return SinLevelInitWithSequencesInternal(invocation->state,&args,invocation->bindings,invocation->result,invocation->event_manager,invocation->sequence_map)!=0;
            if(invocation->event_manager&&invocation->filter_factories)
                return SinLevelInitWithFiltersInternal(invocation->state,&args,invocation->bindings,invocation->result,invocation->event_manager,invocation->filter_factories)!=0;
            if(invocation->event_manager)
                return SinLevelInitWithEventsInternal(invocation->state,&args,invocation->bindings,invocation->result,invocation->event_manager)!=0;
            return api&&api->level_init(invocation->state,&args,invocation->bindings,invocation->result);
        }
#endif
        stopUnsupported("ServerGameDLL003.slot2");
    }
    virtual void __thiscall ServerActivate(edict_t*,int,int) noexcept {
#if defined(SIN_NATIVE_GAME_FRAME)
        if(auto* invocation=g_level_invocation){
            if(!invocation->entered&&invocation->activating){
                invocation->entered=true;
                const auto* api=SiN_GetServerLevelApi(1,sizeof(sin_port::native_level::Api));
                if(invocation->sequence_activation)
                    invocation->success=SinServerActivateWithSequencesInternal(invocation->state,invocation->bindings,invocation->result,invocation->event_manager,invocation->sequence_activation,invocation->sequence_summary);
                else if(invocation->event_manager&&invocation->filters)
                    invocation->success=SinServerActivateWithCampaignFiltersInternal(invocation->state,invocation->bindings,invocation->result,invocation->event_manager,invocation->filters,invocation->filter_summary);
                else if(invocation->event_manager)
                    invocation->success=SinServerActivateWithEventsInternal(invocation->state,invocation->bindings,invocation->result,invocation->event_manager);
                else if(api)invocation->success=api->server_activate(invocation->state,invocation->bindings,invocation->result);
                return;
            }
        }
#endif
        stopUnsupported("ServerGameDLL003.slot3");
    }
    // Authenticated original 221e9080, slot4, RET 4. Typed on every
    // build; only the full independent server has the one-shot adapter.
    virtual void __thiscall GameFrame(bool simulating) noexcept {
#if defined(SIN_NATIVE_GAME_FRAME)
        auto* invocation=g_frame_invocation;
        if(invocation&&!invocation->entered){
            invocation->entered=true;
            const auto* api=SiN_GetServerGameFrameApi(1,sizeof(sin_port::native_game_frame::Api));
            if(invocation->sequence_removal)
                invocation->success=SinGameFrameWithSequenceRemovalInternal(simulating,invocation->state,invocation->bindings,invocation->result,invocation->sequences,invocation->sequence_result,invocation->sequence_removal,invocation->sequence_removed);
            else if(invocation->sequences)
                invocation->success=SinGameFrameWithSequencesInternal(simulating,invocation->state,invocation->bindings,invocation->result,invocation->sequences,invocation->sequence_result);
            else if(invocation->filters)
                invocation->success=SinGameFrameWithFiltersInternal(simulating,invocation->state,invocation->bindings,invocation->result,invocation->filters,invocation->filter_summary);
            else if(api)invocation->success=api->run(simulating,invocation->state,invocation->bindings,invocation->result);
            return;
        }
#endif
        stopUnsupported("ServerGameDLL003.slot4");
    }
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
using ExpectedLevelInit=bool (__thiscall Boundary::*)(const char*,const char*,const char*,const char*,bool,bool) noexcept;
using ExpectedServerActivate=void (__thiscall Boundary::*)(edict_t*,int,int) noexcept;
static_assert(std::is_same<decltype(&Boundary::LevelInit),ExpectedLevelInit>::value,"original LevelInit x86 ABI");
static_assert(std::is_same<decltype(&Boundary::ServerActivate),ExpectedServerActivate>::value,"original ServerActivate x86 ABI");
using ExpectedGameFrame=void (__thiscall Boundary::*)(bool) noexcept;
static_assert(std::is_same<decltype(&Boundary::GameFrame),ExpectedGameFrame>::value,"original server GameFrame x86 ABI");

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

#if defined(SIN_ROLE_server) && defined(SIN_NATIVE_GAME_FRAME)
#pragma comment(linker,"/EXPORT:SiN_InvokeServerLevelInitBoundary=_SiN_InvokeServerLevelInitBoundary")
extern "C" __declspec(dllexport) int __cdecl SiN_InvokeServerLevelInitBoundary(sin_port::native_level::State* state,const sin_port::native_level::LevelArgs* args,const sin_port::native_level::Bindings* bindings,sin_port::native_level::Result* out) noexcept {
    if(!out||g_level_invocation||g_startup_invocation||g_gameinit_invocation||g_frame_invocation)return 0;*out={};
    if(!state||!args||!bindings)return 0;
    LevelInvocation invocation{state,bindings,out,false};g_level_invocation=&invocation;
    using Slot=bool (__thiscall *)(void*,const char*,const char*,const char*,const char*,bool,bool) noexcept;
    auto** table=*reinterpret_cast<void***>(&instance);
    const bool success=reinterpret_cast<Slot>(table[2])(&instance,args->map,args->entities,args->old_level,args->landmark,args->load_game,args->background);
    g_level_invocation=nullptr;return invocation.entered&&success;
}
#pragma comment(linker,"/EXPORT:SiN_InvokeServerActivateBoundary=_SiN_InvokeServerActivateBoundary")
extern "C" __declspec(dllexport) int __cdecl SiN_InvokeServerActivateBoundary(sin_port::native_level::State* state,void* edicts,int count,int clients,const sin_port::native_level::Bindings* bindings,sin_port::native_level::Result* out) noexcept {
    if(!out||g_level_invocation||g_startup_invocation||g_gameinit_invocation||g_frame_invocation)return 0;*out={};
    if(!state||!bindings)return 0;
    LevelInvocation invocation{state,bindings,out,true};g_level_invocation=&invocation;
    using Slot=void (__thiscall *)(void*,edict_t*,int,int) noexcept;
    auto** table=*reinterpret_cast<void***>(&instance);
    reinterpret_cast<Slot>(table[3])(&instance,static_cast<edict_t*>(edicts),count,clients);
    g_level_invocation=nullptr;return invocation.entered&&invocation.success;
}
#pragma comment(linker,"/EXPORT:SiN_InvokeServerGameInitBoundary=_SiN_InvokeServerGameInitBoundary")
extern "C" __declspec(dllexport) int __cdecl SiN_InvokeServerGameInitBoundary(sin_port::native_server_startup::State* state,const sin_port::native_server_startup::GameInitBindings* bindings,sin_port::native_server_startup::GameInitResult* out) noexcept {
    if(!out||g_gameinit_invocation||g_startup_invocation||g_frame_invocation||g_level_invocation)return 0;*out={};
    if(!state||!bindings||!state->complete||state->game_init_attempted||state->busy)return 0;
    GameInitInvocation invocation{state,bindings,out};g_gameinit_invocation=&invocation;
    using Slot=bool (__thiscall *)(void*) noexcept;
    auto** table=*reinterpret_cast<void***>(&instance);
    const bool success=reinterpret_cast<Slot>(table[1])(&instance);
    g_gameinit_invocation=nullptr;return invocation.entered&&success;
}
#pragma comment(linker,"/EXPORT:SiN_InvokeServerDllInitBoundary=_SiN_InvokeServerDllInitBoundary")
extern "C" __declspec(dllexport) int __cdecl SiN_InvokeServerDllInitBoundary(sin_port::native_server_startup::State* state,const sin_port::native_server_startup::Bindings* bindings,sin_port::native_server_startup::BoundaryResult* out) noexcept {
    if(!out||g_startup_invocation||g_frame_invocation||g_gameinit_invocation||g_level_invocation)return 0;*out={};
    if(!state||!bindings||state->attempted||state->busy)return 0;
    StartupInvocation invocation{state,bindings,out};g_startup_invocation=&invocation;
    using Slot=bool (__thiscall *)(void*,Factory,Factory,Factory,CGlobalVars*) noexcept;
    auto** table=*reinterpret_cast<void***>(&instance);
    out->original_return=reinterpret_cast<Slot>(table[0])(&instance,bindings->engine,bindings->physics,bindings->filesystem,static_cast<CGlobalVars*>(bindings->globals));
    g_startup_invocation=nullptr;return out->entered&&out->original_return;
}
// The same actual slot-zero interface is invoked, with a session output
// populated only after successful native startup.
#pragma comment(linker,"/EXPORT:SiN_InvokeServerDllInitWithEngineEvents=_SiN_InvokeServerDllInitWithEngineEvents")
extern "C" int __cdecl SiN_InvokeServerDllInitWithEngineEvents(
    sin_port::native_server_startup::State* state,const sin_port::native_server_startup::Bindings* bindings,
    sin_port::native_server_startup::BoundaryResult* out,sin_port::native_server_startup::EngineEventSession* session) noexcept {
    if(!out||!session||g_startup_invocation||g_frame_invocation||g_gameinit_invocation||g_level_invocation)return 0;
    *out={};*session={};
    if(!state||!bindings||state->attempted||state->busy)return 0;
    StartupInvocation invocation{state,bindings,out,session};g_startup_invocation=&invocation;
    using Slot=bool (__thiscall *)(void*,Factory,Factory,Factory,CGlobalVars*) noexcept;
    auto** table=*reinterpret_cast<void***>(&instance);
    out->original_return=reinterpret_cast<Slot>(table[0])(&instance,bindings->engine,bindings->physics,bindings->filesystem,static_cast<CGlobalVars*>(bindings->globals));
    g_startup_invocation=nullptr;return out->entered&&out->original_return;
}
static bool eventSessionReady(const sin_port::native_server_startup::EngineEventSession* session) noexcept {
    return session&&session->startup&&session->manager&&session->thread==GetCurrentThreadId()&&
        session->startup->complete&&session->startup->game_init_complete&&!session->startup->busy;
}
#pragma comment(linker,"/EXPORT:SiN_ResolveStartedLocalPlayer=_SiN_ResolveStartedLocalPlayer")
extern "C" int __cdecl SiN_ResolveStartedLocalPlayer(
    const sin_port::native_server_startup::EngineEventSession* session,void** out) noexcept {
    if(!out)return 0;*out=nullptr;
    if(!eventSessionReady(session)||g_startup_invocation||g_gameinit_invocation||g_level_invocation)return 0;
    return SinResolveLocalPlayerInternal(session->startup->post.globals,session->startup->engine_server,out);
}
#pragma comment(linker,"/EXPORT:SiN_InvokeServerLevelInitWithEngineEvents=_SiN_InvokeServerLevelInitWithEngineEvents")
extern "C" int __cdecl SiN_InvokeServerLevelInitWithEngineEvents(
    sin_port::native_level::State* state,const sin_port::native_level::LevelArgs* args,
    const sin_port::native_level::Bindings* bindings,sin_port::native_level::Result* out,
    const sin_port::native_server_startup::EngineEventSession* session) noexcept {
    if(!out||g_level_invocation||g_startup_invocation||g_gameinit_invocation||g_frame_invocation)return 0;*out={};
    if(!state||!args||!bindings||!eventSessionReady(session)||state->startup!=session->startup||
       bindings->parse_entities||!bindings->native_map_pipeline)return 0;
    LevelInvocation invocation{state,bindings,out,false};invocation.event_manager=session->manager;g_level_invocation=&invocation;
    using Slot=bool (__thiscall *)(void*,const char*,const char*,const char*,const char*,bool,bool) noexcept;
    auto** table=*reinterpret_cast<void***>(&instance);
    const bool success=reinterpret_cast<Slot>(table[2])(&instance,args->map,args->entities,args->old_level,args->landmark,args->load_game,args->background);
    g_level_invocation=nullptr;return invocation.entered&&success;
}
#pragma comment(linker,"/EXPORT:SiN_InvokeServerLevelInitWithCampaignFilters=_SiN_InvokeServerLevelInitWithCampaignFilters")
extern "C" int __cdecl SiN_InvokeServerLevelInitWithCampaignFilters(
    sin_port::native_level::State* state,const sin_port::native_level::LevelArgs* args,
    const sin_port::native_level::Bindings* bindings,sin_port::native_level::Result* out,
    const sin_port::native_server_startup::EngineEventSession* session,const sin_port::native_map::CampaignFilterFactoryBindings* factories) noexcept {
    if(!out||g_level_invocation||g_startup_invocation||g_gameinit_invocation||g_frame_invocation)return 0;*out={};
    if(!state||!args||!bindings||!factories||!factories->owner||!eventSessionReady(session)||state->startup!=session->startup||
       bindings->parse_entities||!bindings->native_map_pipeline||!bindings->native_map_pipeline->parse)return 0;
    const auto* parse=bindings->native_map_pipeline->parse;
    if(parse->create||parse->key_value||!parse->native_factory||!parse->native_keys)return 0;
    LevelInvocation invocation{state,bindings,out,false};invocation.event_manager=session->manager;invocation.filter_factories=factories;g_level_invocation=&invocation;
    using Slot=bool (__thiscall *)(void*,const char*,const char*,const char*,const char*,bool,bool) noexcept;
    auto** table=*reinterpret_cast<void***>(&instance);
    const bool success=reinterpret_cast<Slot>(table[2])(&instance,args->map,args->entities,args->old_level,args->landmark,args->load_game,args->background);
    g_level_invocation=nullptr;return invocation.entered&&success;
}
#pragma comment(linker,"/EXPORT:SiN_InvokeServerActivateWithEngineEvents=_SiN_InvokeServerActivateWithEngineEvents")
extern "C" int __cdecl SiN_InvokeServerActivateWithEngineEvents(
    sin_port::native_level::State* state,void* edicts,int count,int clients,
    const sin_port::native_level::Bindings* bindings,sin_port::native_level::Result* out,
    const sin_port::native_server_startup::EngineEventSession* session) noexcept {
    if(!out||g_level_invocation||g_startup_invocation||g_gameinit_invocation||g_frame_invocation)return 0;*out={};
    if(!state||!bindings||!eventSessionReady(session)||state->startup!=session->startup)return 0;
    LevelInvocation invocation{state,bindings,out,true};invocation.event_manager=session->manager;g_level_invocation=&invocation;
    using Slot=void (__thiscall *)(void*,edict_t*,int,int) noexcept;
    auto** table=*reinterpret_cast<void***>(&instance);
    reinterpret_cast<Slot>(table[3])(&instance,static_cast<edict_t*>(edicts),count,clients);
    g_level_invocation=nullptr;return invocation.entered&&invocation.success;
}
#pragma comment(linker,"/EXPORT:SiN_InvokeServerActivateWithCampaignFilters=_SiN_InvokeServerActivateWithCampaignFilters")
extern "C" int __cdecl SiN_InvokeServerActivateWithCampaignFilters(
    sin_port::native_level::State* state,void* edicts,int count,int clients,
    const sin_port::native_level::Bindings* bindings,sin_port::native_level::Result* out,
    const sin_port::native_server_startup::EngineEventSession* session,
    const sin_port::native_map::CampaignFilterActivationBindings* filters,sin_port::native_level::CampaignFilterActivationSummary* summary) noexcept {
    if(summary)*summary={};
    if(!out||g_level_invocation||g_startup_invocation||g_gameinit_invocation||g_frame_invocation)return 0;*out={};
    if(!state||!bindings||!filters||!summary||!eventSessionReady(session)||state->startup!=session->startup)return 0;
    LevelInvocation invocation{state,bindings,out,true};invocation.event_manager=session->manager;
    invocation.filters=filters;invocation.filter_summary=summary;g_level_invocation=&invocation;
    using Slot=void (__thiscall *)(void*,edict_t*,int,int) noexcept;
    auto** table=*reinterpret_cast<void***>(&instance);
    reinterpret_cast<Slot>(table[3])(&instance,static_cast<edict_t*>(edicts),count,clients);
    g_level_invocation=nullptr;return invocation.entered&&invocation.success;
}
// Optional session retains the started-engine event path; null uses the
// existing owner event callbacks, as in the original boundary adapter.
#pragma comment(linker,"/EXPORT:SiN_InvokeServerLevelInitWithScriptedSequences=_SiN_InvokeServerLevelInitWithScriptedSequences")
extern "C" int __cdecl SiN_InvokeServerLevelInitWithScriptedSequences(
    sin_port::native_level::State* state,const sin_port::native_level::LevelArgs* args,
    const sin_port::native_level::Bindings* bindings,sin_port::native_level::Result* out,
    const sin_port::native_server_startup::EngineEventSession* session,const sin_port::native_map::ScriptedSequenceMapBindings* sequences) noexcept {
    if(!out||g_level_invocation||g_startup_invocation||g_gameinit_invocation||g_frame_invocation)return 0;*out={};
    if(!state||!args||!bindings||!sequences||!sequences->factory||!sequences->spawn||
       (session&&(!eventSessionReady(session)||state->startup!=session->startup))||
       bindings->parse_entities||!bindings->native_map_pipeline)return 0;
    const auto* pipeline=bindings->native_map_pipeline;
    if(!pipeline->parse||!pipeline->spawn||!pipeline->spawn->native_dispatch||
       pipeline->parse->create||pipeline->parse->key_value||!pipeline->parse->native_factory||!pipeline->parse->native_keys)return 0;
    LevelInvocation invocation{state,bindings,out,false};invocation.event_manager=session?session->manager:nullptr;
    invocation.sequence_map=sequences;g_level_invocation=&invocation;
    using Slot=bool (__thiscall *)(void*,const char*,const char*,const char*,const char*,bool,bool) noexcept;
    auto** table=*reinterpret_cast<void***>(&instance);
    const bool success=reinterpret_cast<Slot>(table[2])(&instance,args->map,args->entities,args->old_level,args->landmark,args->load_game,args->background);
    g_level_invocation=nullptr;return invocation.entered&&success;
}
#pragma comment(linker,"/EXPORT:SiN_InvokeServerActivateWithScriptedSequences=_SiN_InvokeServerActivateWithScriptedSequences")
extern "C" int __cdecl SiN_InvokeServerActivateWithScriptedSequences(
    sin_port::native_level::State* state,void* edicts,int count,int clients,
    const sin_port::native_level::Bindings* bindings,sin_port::native_level::Result* out,
    const sin_port::native_server_startup::EngineEventSession* session,
    const sin_port::native_level::ScriptedSequenceActivationRoute* route,sin_port::native_level::ScriptedSequenceActivationSummary* summary) noexcept {
    if(summary)*summary={};
    if(!out||g_level_invocation||g_startup_invocation||g_gameinit_invocation||g_frame_invocation)return 0;*out={};
    if(!state||!bindings||!route||!route->sequence||!summary||(route->filters&&!route->filter_summary)||
       (session&&(!eventSessionReady(session)||state->startup!=session->startup)))return 0;
    LevelInvocation invocation{state,bindings,out,true};invocation.event_manager=session?session->manager:nullptr;
    invocation.sequence_activation=route;invocation.sequence_summary=summary;g_level_invocation=&invocation;
    using Slot=void (__thiscall *)(void*,edict_t*,int,int) noexcept;
    auto** table=*reinterpret_cast<void***>(&instance);
    reinterpret_cast<Slot>(table[3])(&instance,static_cast<edict_t*>(edicts),count,clients);
    g_level_invocation=nullptr;return invocation.entered&&invocation.success;
}
#pragma comment(linker,"/EXPORT:SiN_TouchStartedCampaignTrigger=_SiN_TouchStartedCampaignTrigger")
extern "C" int __cdecl SiN_TouchStartedCampaignTrigger(
    const sin_port::native_server_startup::EngineEventSession* session,void* actor,unsigned bytes,void* other,
    const sin_port::native_map::CampaignTriggerBindings* bindings,sin_port::native_map::CampaignTriggerResult* result,int* publication) noexcept {
    if(result)*result={};
    if(publication)*publication=sin_port::native_map::engine_event_not_attempted;
    static thread_local bool touching=false;
    if(touching||!eventSessionReady(session)||g_startup_invocation||g_gameinit_invocation||g_level_invocation)return 0;
    struct Guard {bool& active;~Guard(){active=false;}} guard{touching};touching=true;
    // Touch is permitted during the ordinary game-frame boundary.
    // The acquired startup owner replaces the hosted owner-word callback.
    return SinTouchCampaignTriggerFromStartupInternal(actor,bytes,other,bindings,session->manager,result,publication,
        session->startup->post.globals,session->startup->engine_server);
}
// 2221e740 and 2221e770: check both deletion bytes once, then
// notify virtual +164 or +160 followed by +164. Do not generate contacts.
static int dispatchCampaignContactImpl(
    const sin_port::native_server_startup::EngineEventSession* session,void* actor,unsigned bytes,void* other,unsigned other_bytes,
    sin_port::native_level::CampaignContactKind kind,const sin_port::native_level::CampaignContactBindings* b,
    sin_port::native_level::CampaignContactResult* o,const sin_port::native_map::CampaignStartTouchBindings* native_start,
    sin_port::native_map::CampaignStartTouchResult* start_result,
    const sin_port::native_map::CampaignFilterBindings* native_filter=nullptr,
    sin_port::native_map::CampaignFilterResult* filter_result=nullptr) noexcept {
    if(start_result)*start_result={};
    if(filter_result)*filter_result={};
    using namespace sin_port::native_level;
    namespace nm=sin_port::native_map;
    if(!o)return 0;*o={};o->publication=nm::engine_event_not_attempted;
    static thread_local bool dispatching=false;
    if(dispatching||!eventSessionReady(session)||g_startup_invocation||g_gameinit_invocation||g_level_invocation||!b||!b->campaign||
       (kind!=CampaignContactKind::Continuing&&kind!=CampaignContactKind::Starting))return 0;
    unsigned marker=0;unsigned char flags=0;
    if(!SinReadCampaignContactFieldsInternal(actor,bytes,&marker,&flags))return 0;
    const unsigned extent=marker==nm::dynamic_trigger_vtable?nm::dynamic_trigger_extent:
        marker==nm::secret_trigger_vtable?nm::secret_trigger_extent:0;
    if(!extent||bytes<extent)return 0;
    if(!other){o->missing_other=o->completed=1;return 1;}
    // Original gate reads actor first and can skip without reading other.
    if(flags&1){o->marked_actor=o->completed=1;return 1;}
    unsigned other_marker=0;unsigned char other_flags=0;
    if(!SinReadCampaignContactFieldsInternal(other,other_bytes,&other_marker,&other_flags))return 0;
    if(other_flags&1){o->marked_other=o->completed=1;return 1;}
    struct Guard {bool& flag;~Guard(){flag=false;}} guard{dispatching};dispatching=true;
    if(kind==CampaignContactKind::Starting){
        if(native_start){
            if(!start_result)return 0;
            o->start_called=1;
            if(native_filter){
                if(!filter_result||!SiN_StartCampaignTriggerOutputWithFilter(actor,bytes,other,other_bytes,native_start,start_result,native_filter,filter_result))return 0;
            }else if(!SiN_StartCampaignTriggerOutput(actor,bytes,other,other_bytes,native_start,start_result))return 0;
        }else{
            if(!b->start_touch)return 0;
            o->start_called=1;
            if(!b->start_touch(b->context,actor,other))return 0;
        }
        // Original does not recheck deletion bits after StartTouch.
        // Validate that storage/class is still supported; owners defer free.
        unsigned after=0;unsigned char ignored=0;
        if(!SinReadCampaignContactFieldsInternal(actor,bytes,&after,&ignored)||after!=marker)return 0;
    }
    o->touch_called=1;
    if(!SiN_TouchStartedCampaignTrigger(session,actor,bytes,other,b->campaign,&o->trigger,&o->publication))return 0;
    o->completed=1;return 1;
}
#pragma comment(linker,"/EXPORT:SiN_DispatchStartedCampaignContact=_SiN_DispatchStartedCampaignContact")
extern "C" int __cdecl SiN_DispatchStartedCampaignContact(
    const sin_port::native_server_startup::EngineEventSession* session,void* actor,unsigned bytes,void* other,unsigned other_bytes,
    sin_port::native_level::CampaignContactKind kind,const sin_port::native_level::CampaignContactBindings* b,
    sin_port::native_level::CampaignContactResult* o) noexcept {
    return dispatchCampaignContactImpl(session,actor,bytes,other,other_bytes,kind,b,o,nullptr,nullptr);
}
#pragma comment(linker,"/EXPORT:SiN_DispatchStartedCampaignContactWithOutput=_SiN_DispatchStartedCampaignContactWithOutput")
extern "C" int __cdecl SiN_DispatchStartedCampaignContactWithOutput(
    const sin_port::native_server_startup::EngineEventSession* session,void* actor,unsigned bytes,void* other,unsigned other_bytes,
    sin_port::native_level::CampaignContactKind kind,const sin_port::native_level::CampaignContactBindings* b,
    sin_port::native_level::CampaignContactResult* o,const sin_port::native_map::CampaignStartTouchBindings* native_start,
    sin_port::native_map::CampaignStartTouchResult* start_result) noexcept {
    if(!native_start||!start_result){if(o)*o={};return 0;}
    return dispatchCampaignContactImpl(session,actor,bytes,other,other_bytes,kind,b,o,native_start,start_result);
}
#pragma comment(linker,"/EXPORT:SiN_DispatchStartedCampaignContactWithFilter=_SiN_DispatchStartedCampaignContactWithFilter")
extern "C" int __cdecl SiN_DispatchStartedCampaignContactWithFilter(
    const sin_port::native_server_startup::EngineEventSession* session,void* actor,unsigned bytes,void* other,unsigned other_bytes,
    sin_port::native_level::CampaignContactKind kind,const sin_port::native_level::CampaignContactBindings* b,
    sin_port::native_level::CampaignContactResult* o,const sin_port::native_map::CampaignStartTouchBindings* native_start,
    sin_port::native_map::CampaignStartTouchResult* start_result,const sin_port::native_map::CampaignFilterBindings* filter,
    sin_port::native_map::CampaignFilterResult* fr) noexcept {
    if(!native_start||!start_result||!filter||!fr){if(o)*o={};return 0;}
    return dispatchCampaignContactImpl(session,actor,bytes,other,other_bytes,kind,b,o,native_start,start_result,filter,fr);
}
#pragma comment(linker,"/EXPORT:SiN_InvokeServerGameFrameBoundary=_SiN_InvokeServerGameFrameBoundary")
extern "C" __declspec(dllexport) int __cdecl SiN_InvokeServerGameFrameBoundary(bool simulating,const sin_port::native_game_frame::State* state,const sin_port::native_game_frame::Bindings* bindings,sin_port::native_game_frame::Result* result) noexcept {
    if(!result||g_frame_invocation||g_startup_invocation||g_gameinit_invocation||g_level_invocation)return 0;*result={};
    if(!state||!bindings)return 0;
    FrameInvocation invocation{state,bindings,result};
    g_frame_invocation=&invocation;
    using Slot=void (__thiscall *)(void*,bool) noexcept;
    // Use the actual interface object and slot, not a second fake vtable.
    auto** table=*reinterpret_cast<void***>(&instance);
    reinterpret_cast<Slot>(table[sin_port::native_game_frame::original_slot])(&instance,simulating);
    g_frame_invocation=nullptr;
    return invocation.entered&&invocation.success;
}
#pragma comment(linker,"/EXPORT:SiN_InvokeServerGameFrameWithCampaignFilters=_SiN_InvokeServerGameFrameWithCampaignFilters")
extern "C" int __cdecl SiN_InvokeServerGameFrameWithCampaignFilters(bool simulating,const sin_port::native_game_frame::State* state,
    const sin_port::native_game_frame::Bindings* bindings,sin_port::native_game_frame::Result* result,
    const sin_port::native_map::CampaignFilterInputBindings* filters,sin_port::native_weapon_tables::CampaignFilterServiceSummary* summary) noexcept {
    if(result)*result={};if(summary)*summary={};
    if(!state||!bindings||!result||!filters||!summary||g_frame_invocation||g_startup_invocation||g_gameinit_invocation||g_level_invocation)return 0;
    FrameInvocation invocation{state,bindings,result};invocation.filters=filters;invocation.filter_summary=summary;g_frame_invocation=&invocation;
    using Slot=void (__thiscall *)(void*,bool) noexcept;
    auto** table=*reinterpret_cast<void***>(&instance);
    reinterpret_cast<Slot>(table[sin_port::native_game_frame::original_slot])(&instance,simulating);
    g_frame_invocation=nullptr;return invocation.entered&&invocation.success;
}
#pragma comment(linker,"/EXPORT:SiN_InvokeServerGameFrameWithScriptedSequences=_SiN_InvokeServerGameFrameWithScriptedSequences")
extern "C" int __cdecl SiN_InvokeServerGameFrameWithScriptedSequences(bool simulating,const sin_port::native_game_frame::State* state,
    const sin_port::native_game_frame::Bindings* bindings,sin_port::native_game_frame::Result* result,
    const sin_port::native_game_frame::ScriptedSequenceFrameBindings* sequences,sin_port::native_game_frame::ScriptedSequenceFrameResult* sr) noexcept {
    if(result)*result={};if(sr)*sr={};
    if(!state||!bindings||!result||!sequences||!sr||g_frame_invocation||g_startup_invocation||g_gameinit_invocation||g_level_invocation)return 0;
    FrameInvocation invocation{state,bindings,result};invocation.sequences=sequences;invocation.sequence_result=sr;g_frame_invocation=&invocation;
    using Slot=void (__thiscall *)(void*,bool) noexcept;
    auto** table=*reinterpret_cast<void***>(&instance);
    reinterpret_cast<Slot>(table[sin_port::native_game_frame::original_slot])(&instance,simulating);
    g_frame_invocation=nullptr;return invocation.entered&&invocation.success;
}
#pragma comment(linker,"/EXPORT:SiN_InvokeServerGameFrameWithScriptedSequenceRemoval=_SiN_InvokeServerGameFrameWithScriptedSequenceRemoval")
extern "C" int __cdecl SiN_InvokeServerGameFrameWithScriptedSequenceRemoval(bool simulating,const sin_port::native_game_frame::State* state,
    const sin_port::native_game_frame::Bindings* bindings,sin_port::native_game_frame::Result* result,
    const sin_port::native_game_frame::ScriptedSequenceFrameBindings* sequences,sin_port::native_game_frame::ScriptedSequenceFrameResult* sr,
    const sin_port::native_map::ScriptedSequenceRemovalThinkBindings* removal,sin_port::native_game_frame::ScriptedSequenceFrameRemovalResult* removed) noexcept {
    if(result)*result={};if(sr)*sr={};if(removed)*removed={};
    if(!state||!bindings||!result||!sequences||!sr||!removal||!removed||g_frame_invocation||g_startup_invocation||g_gameinit_invocation||g_level_invocation)return 0;
    FrameInvocation invocation{state,bindings,result};invocation.sequences=sequences;invocation.sequence_result=sr;
    invocation.sequence_removal=removal;invocation.sequence_removed=removed;g_frame_invocation=&invocation;
    using Slot=void (__thiscall *)(void*,bool) noexcept;
    auto** table=*reinterpret_cast<void***>(&instance);
    reinterpret_cast<Slot>(table[sin_port::native_game_frame::original_slot])(&instance,simulating);
    g_frame_invocation=nullptr;return invocation.entered&&invocation.success;
}
#endif

extern "C" __declspec(dllexport) void* __cdecl CreateInterface(
    const char* name, int* status) noexcept {
    void* result = name && std::strcmp(name, kName) == 0
        ? static_cast<void*>(&instance) : nullptr;
#if defined(SIN_ROLE_client)
    if(!result && name &&
       std::strcmp(name,pred_factory::kInterfaceName)==0 &&
       clientPredictionFactoryProbeEnabled() &&
       g_predictionOwnerReady && g_predictionOwner &&
       g_predictionOwner->Bound())
        result=static_cast<void*>(g_predictionOwner);
    if(!result && name &&
       std::strcmp(name,native_entitylist::kInterfaceName)==0 &&
       clientEntityListFactoryProbeEnabled() &&
       g_entityListReady && g_entityListOwner &&
       g_entityListOwner->Active())
        result=static_cast<void*>(g_entityListOwner->Interface());
    if(!result && name &&
       std::strcmp(name,native_centerprint::kInterfaceName)==0 &&
       clientCenterprintFactoryProbeEnabled() &&
       g_centerprintReady && g_centerprintOwner &&
       g_centerprintOwner->Active() &&
       g_entityListReady && g_entityListOwner &&
       g_entityListOwner->Active())
        result=static_cast<void*>(g_centerprintOwner);
#endif
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

// The following exports only exist in the SEPARATE synthetic test DLL.
// They never appear in the independently distributed client.dll.
#if defined(SIN_ROLE_client) && defined(SIN_SYNTHETIC_SLOT33_OWNED)
extern "C" __declspec(dllexport)
bool __cdecl SiNTestInitializeOwnedMessageRuntime() noexcept {
    g_slot33Reader={};
    g_slot33Trusted={};
    g_slot33Ready=g_slot33Owned.Initialize();
    return g_slot33Ready;
}
extern "C" __declspec(dllexport)
void* __cdecl SiNTestInstallOwnedReader(
    const sin_independent::bf_read_candidate::Layout* reader,
    const std::uint8_t* trusted,std::size_t accessible) noexcept {
    if(!g_slot33Ready || !reader || !trusted ||
       accessible==0 || accessible>sin_independent::stats_bitbuf::kMaxMessageBytes)
        return nullptr;
    // Only caller-owned test memory explicitly provided through the
    // fixture export is subsequently permitted as the buffer source.
    g_slot33Reader=*reader;
    g_slot33Trusted={trusted,accessible};
    return &g_slot33Reader;
}
extern "C" __declspec(dllexport)
std::uint32_t __cdecl SiNTestStaticFieldBits(unsigned id,unsigned field) noexcept {
    const auto* value=g_slot33Owned.stats().staticValues().at(id);
    return g_slot33Ready && value?value->fieldBits(field):0u;
}
extern "C" __declspec(dllexport)
std::uint32_t __cdecl SiNTestDynamicFieldBits(
    unsigned id,const char* name,std::size_t length) noexcept {
    const auto* value=g_slot33Owned.stats().dynamicValues().Find(id,name,length);
    return g_slot33Ready && value?value->fieldBits(0):0u;
}
extern "C" __declspec(dllexport)
std::size_t __cdecl SiNTestDynamicCount() noexcept {
    return g_slot33Ready?g_slot33Owned.stats().dynamicValues().size():0u;
}
extern "C" __declspec(dllexport)
std::size_t __cdecl SiNTestResetCount() noexcept {
    return g_slot33Ready?g_slot33Owned.stats().resetCount():0u;
}
#endif
