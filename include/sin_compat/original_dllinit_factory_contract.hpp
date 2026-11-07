#pragma once
// Independently reconstructed, original 2006 SiN client/server DLL startup
// interface-acquisition contract. Original game files stay private.
// Evidence: research/tools/audit_original_sin_dllinit_factories.py, SHA-pinned
// original PE32 x86 call/test/store/branch instruction windows.
// This is a prerequisite preflight, NOT a complete DLLInit/Init or a valid
// SourceVR/Quest game-module implementation.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_compat::original_dllinit {

enum class FactoryProvider : std::uint8_t {
    Engine, FileSystem, MaterialSystemMethod, SurfaceMethod
};

struct FactoryRequirement {
    std::string_view name;
    FactoryProvider provider;
    std::uint32_t original_push_va;
};

// CServerGameDLL::DLLInit: all 16 of these are null-checked against a
// shared original failure exit, in the exact order seen in server.dll.
inline constexpr std::array<FactoryRequirement,16> server_required{{
    {"VEngineServer021", FactoryProvider::Engine, 0x221e764b},
    {"VoiceServer001", FactoryProvider::Engine, 0x221e7664},
    {"VEngineCvar002", FactoryProvider::Engine, 0x221e767d},
    {"VEngineServerStringTable001", FactoryProvider::Engine, 0x221e7696},
    {"StaticPropMgrServer002", FactoryProvider::Engine, 0x221e76af},
    {"VEngineRandom001", FactoryProvider::Engine, 0x221e76c8},
    {"IEngineSoundServer003", FactoryProvider::Engine, 0x221e76e1},
    {"SpatialPartition001", FactoryProvider::Engine, 0x221e76fa},
    {"VModelInfoServer001", FactoryProvider::Engine, 0x221e7713},
    {"EngineTraceServer003", FactoryProvider::Engine, 0x221e772c},
    {"VFileSystem012", FactoryProvider::FileSystem, 0x221e7749},
    {"GAMEEVENTSMANAGER002", FactoryProvider::Engine, 0x221e7762},
    {"VEngineCache001", FactoryProvider::Engine, 0x221e777b},
    {"VSoundEmitter001", FactoryProvider::Engine, 0x221e7794},
    {"ServerUploadGameStats001", FactoryProvider::Engine, 0x221e77ad},
    {"ISERVERPLUGINHELPERS001", FactoryProvider::Engine, 0x221e77c6},
}};

// CHLClient::Init: 21 required direct engine-factory calls.
inline constexpr std::array<FactoryRequirement,21> client_direct_required{{
    {"VEngineClient012", FactoryProvider::Engine, 0x240fe10b},
    {"VEngineModel012", FactoryProvider::Engine, 0x240fe129},
    {"VEngineEffects001", FactoryProvider::Engine, 0x240fe142},
    {"VEngineCvar002", FactoryProvider::Engine, 0x240fe15b},
    {"EngineTraceClient003", FactoryProvider::Engine, 0x240fe174},
    {"VEngineRenderView011", FactoryProvider::Engine, 0x240fe18d},
    {"VDebugOverlay003", FactoryProvider::Engine, 0x240fe1a6},
    {"VMaterialSystem076", FactoryProvider::Engine, 0x240fe1bf},
    {"VEngineCache001", FactoryProvider::Engine, 0x240fe1d8},
    {"VModelInfoClient003", FactoryProvider::Engine, 0x240fe1f1},
    {"VEngineVGui001", FactoryProvider::Engine, 0x240fe20a},
    {"VEngineClientStringTable001", FactoryProvider::Engine, 0x240fe223},
    {"SpatialPartition001", FactoryProvider::Engine, 0x240fe23c},
    {"VEngineShadowMgr002", FactoryProvider::Engine, 0x240fe255},
    {"StaticPropMgrClient004", FactoryProvider::Engine, 0x240fe26e},
    {"IEngineSoundClient003", FactoryProvider::Engine, 0x240fe287},
    {"VFileSystem012", FactoryProvider::Engine, 0x240fe2a0},
    {"VEngineRandom001", FactoryProvider::Engine, 0x240fe2b9},
    {"VENGINE_GAMEUIFUNCS_VERSION004", FactoryProvider::Engine, 0x240fe2d2},
    {"GAMEEVENTSMANAGER002", FactoryProvider::Engine, 0x240fe2eb},
    {"VSoundEmitter001", FactoryProvider::Engine, 0x240fe304},
}};

// Two more null-checked client dependencies obtained from material/surface
// vtable providers AFTER other initialization, not from the engine factory.
inline constexpr std::array<FactoryRequirement,2> client_derived_required{{
    {"MaterialSystemHardwareConfig012", FactoryProvider::MaterialSystemMethod, 0x240fe362},
    {"MatSystemSurface003", FactoryProvider::SurfaceMethod, 0x240fe3e1},
}};

// Missing these does not take the direct common-failure exit at the
// observed original request sites. They must not be treated as required.
inline constexpr std::array<FactoryRequirement,2> optional_probes{{
    {"VDebugOverlay003", FactoryProvider::Engine, 0x221e7944},
    {"VMaterialSystemStub001", FactoryProvider::Engine, 0x240fe349},
}};

struct RequirementReport {
    bool present;
    std::size_t checked;
    std::string_view missing_name;
    FactoryProvider missing_provider;
    std::uint32_t missing_original_va;
};

// The template host returns a pointer or boolean convertible to bool.
// It does NOT invoke C++ game interfaces, create an engine object, or
// imply that succeeding factory checks will make the full DLLInit succeed.
template<typename Engine,typename FileSystem>
RequirementReport check_original_server_interfaces(Engine&& engine,
                                                    FileSystem&& filesystem) {
    for (std::size_t i=0;i<server_required.size();++i) {
        const auto& r=server_required[i];
        const bool ok=r.provider==FactoryProvider::FileSystem
            ? static_cast<bool>(filesystem(r.name))
            : static_cast<bool>(engine(r.name));
        if (!ok) return {false,i+1,r.name,r.provider,r.original_push_va};
    }
    return {true,server_required.size(),{},FactoryProvider::Engine,0};
}

template<typename Engine,typename MaterialSystem,typename Surface>
RequirementReport check_original_client_interfaces(Engine&& engine,
                                                     MaterialSystem&& materials,
                                                     Surface&& surface) {
    for (std::size_t i=0;i<client_direct_required.size();++i) {
        const auto& r=client_direct_required[i];
        if (!static_cast<bool>(engine(r.name)))
            return {false,i+1,r.name,r.provider,r.original_push_va};
    }
    // The genuine Init performs other setup, including an optional material
    // stub lookup, between these stages. This helper probes dependencies
    // only; it must not be called a full faithful execution of Init.
    const auto& hardware=client_derived_required[0];
    if (!static_cast<bool>(materials(hardware.name)))
        return {false,client_direct_required.size()+1,hardware.name,
                hardware.provider,hardware.original_push_va};
    const auto& draw=client_derived_required[1];
    if (!static_cast<bool>(surface(draw.name)))
        return {false,client_direct_required.size()+2,draw.name,
                draw.provider,draw.original_push_va};
    return {true,client_direct_required.size()+client_derived_required.size(),
            {},FactoryProvider::Engine,0};
}

static_assert(server_required.size()==16);
static_assert(client_direct_required.size()==21);
static_assert(client_derived_required.size()==2);
static_assert(optional_probes.size()==2);
static_assert(server_required[10].provider==FactoryProvider::FileSystem);
static_assert(server_required[13].name=="VSoundEmitter001");
static_assert(client_direct_required[20].name=="VSoundEmitter001");
} // namespace sin_compat::original_dllinit
