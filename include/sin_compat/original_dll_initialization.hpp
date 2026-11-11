#pragma once
// Phase179: address-precise extension of Phase85 original Init negotiation
// prefix from server CServerGameDLL::DLLInit and client CHLClient::Init.
// This is a partial, host-neutral branch model, NOT complete x86 ABI or
// a game-loadable client/server DLL implementation.
#include <array>
#include <cstdint>
#include <cstddef>

namespace sin_compat::original_dll_initialization {
enum class Supplier { EngineFactory, FilesystemFactory, DerivedVirtual };
struct Request {
    const char* version;
    std::uint32_t native_request_va;
    std::uint32_t native_string_va;
    std::uint32_t native_storage_va;
    Supplier supplier;
    bool required;
};

inline constexpr std::array<Request, 16> kServerPrefixRequests{{
    {"VEngineServer021", 0x221e7649u, 0x22442a48u, 0x22572198u, Supplier::EngineFactory, true},
    {"VoiceServer001", 0x221e7662u, 0x22442a38u, 0x2257219cu, Supplier::EngineFactory, true},
    {"VEngineCvar002", 0x221e767bu, 0x22442a28u, 0x225721a0u, Supplier::EngineFactory, true},
    {"VEngineServerStringTable001", 0x221e7694u, 0x22442a0cu, 0x225721a8u, Supplier::EngineFactory, true},
    {"StaticPropMgrServer002", 0x221e76adu, 0x224429f4u, 0x225721acu, Supplier::EngineFactory, true},
    {"VEngineRandom001", 0x221e76c6u, 0x224429e0u, 0x225721b0u, Supplier::EngineFactory, true},
    {"IEngineSoundServer003", 0x221e76dfu, 0x224429c8u, 0x225721b4u, Supplier::EngineFactory, true},
    {"SpatialPartition001", 0x221e76f8u, 0x224429b4u, 0x225721b8u, Supplier::EngineFactory, true},
    {"VModelInfoServer001", 0x221e7711u, 0x224429a0u, 0x225721bcu, Supplier::EngineFactory, true},
    {"EngineTraceServer003", 0x221e772au, 0x22442988u, 0x225721c0u, Supplier::EngineFactory, true},
    {"VFileSystem012", 0x221e7747u, 0x22442978u, 0x225721a4u, Supplier::FilesystemFactory, true},
    {"GAMEEVENTSMANAGER002", 0x221e7760u, 0x22442960u, 0x225721c4u, Supplier::EngineFactory, true},
    {"VEngineCache001", 0x221e7779u, 0x22442950u, 0x225721c8u, Supplier::EngineFactory, true},
    {"VSoundEmitter001", 0x221e7792u, 0x2244293cu, 0x225721d0u, Supplier::EngineFactory, true},
    {"ServerUploadGameStats001", 0x221e77abu, 0x22442920u, 0x22572194u, Supplier::EngineFactory, true},
    {"ISERVERPLUGINHELPERS001", 0x221e77c4u, 0x22442908u, 0x225721d4u, Supplier::EngineFactory, true},
}};

inline constexpr std::array<Request, 1> kServerLateRequests{{
    {"VDebugOverlay003", 0x221e7942u, 0x224428b8u, 0x225721ccu, Supplier::EngineFactory, false},
}};

inline constexpr std::array<Request, 21> kClientPrefixRequests{{
    {"VEngineClient012", 0x240fe109u, 0x24296aecu, 0x2433f684u, Supplier::EngineFactory, true},
    {"VEngineModel012", 0x240fe127u, 0x24296adcu, 0x2433f688u, Supplier::EngineFactory, true},
    {"VEngineEffects001", 0x240fe140u, 0x24296ac8u, 0x2433f68cu, Supplier::EngineFactory, true},
    {"VEngineCvar002", 0x240fe159u, 0x24296ab8u, 0x2433f690u, Supplier::EngineFactory, true},
    {"EngineTraceClient003", 0x240fe172u, 0x24296aa0u, 0x2433f6d8u, Supplier::EngineFactory, true},
    {"VEngineRenderView011", 0x240fe18bu, 0x24296a88u, 0x2433f694u, Supplier::EngineFactory, true},
    {"VDebugOverlay003", 0x240fe1a4u, 0x24296a74u, 0x2433f698u, Supplier::EngineFactory, true},
    {"VMaterialSystem076", 0x240fe1bdu, 0x24296a60u, 0x2433f69cu, Supplier::EngineFactory, true},
    {"VEngineCache001", 0x240fe1d6u, 0x24296a50u, 0x2433f6a8u, Supplier::EngineFactory, true},
    {"VModelInfoClient003", 0x240fe1efu, 0x24296a3cu, 0x2433f6acu, Supplier::EngineFactory, true},
    {"VEngineVGui001", 0x240fe208u, 0x24296a2cu, 0x2433f6b0u, Supplier::EngineFactory, true},
    {"VEngineClientStringTable001", 0x240fe221u, 0x24296a10u, 0x2433f6b4u, Supplier::EngineFactory, true},
    {"SpatialPartition001", 0x240fe23au, 0x242969fcu, 0x2433f6b8u, Supplier::EngineFactory, true},
    {"VEngineShadowMgr002", 0x240fe253u, 0x242969e8u, 0x2433f6c0u, Supplier::EngineFactory, true},
    {"StaticPropMgrClient004", 0x240fe26cu, 0x242969d0u, 0x2433f6c4u, Supplier::EngineFactory, true},
    {"IEngineSoundClient003", 0x240fe285u, 0x242969b8u, 0x2433f6c8u, Supplier::EngineFactory, true},
    {"VFileSystem012", 0x240fe29eu, 0x242d328cu, 0x2433f6bcu, Supplier::EngineFactory, true},
    {"VEngineRandom001", 0x240fe2b7u, 0x242969a4u, 0x2433f6ccu, Supplier::EngineFactory, true},
    {"VENGINE_GAMEUIFUNCS_VERSION004", 0x240fe2d0u, 0x24296984u, 0x2433f6dcu, Supplier::EngineFactory, true},
    {"GAMEEVENTSMANAGER002", 0x240fe2e9u, 0x2429696cu, 0x2433f6e0u, Supplier::EngineFactory, true},
    {"VSoundEmitter001", 0x240fe302u, 0x24296958u, 0x2433f6e4u, Supplier::EngineFactory, true},
}};

inline constexpr std::array<Request, 3> kClientLateRequests{{
    {"VMaterialSystemStub001", 0x240fe347u, 0x24296934u, 0x2433f6a0u, Supplier::EngineFactory, false},
    {"MaterialSystemHardwareConfig012", 0x240fe362u, 0x24296914u, 0x2433f6a4u, Supplier::DerivedVirtual, true},
    {"MatSystemSurface003", 0x240fe3e1u, 0x24296900u, 0x2433f6d0u, Supplier::DerivedVirtual, true},
}};

// Reconstructs only the early, contiguous mandatory lookup prefix.
// The original stores even null results before branching to the shared
// false return. A caller-supplied host must perform the original factory
// callback or a test replacement; no native pointers are dereferenced here.
// Host: uintptr_t query(Supplier,const char*); void publish(uint32_t,uintptr_t).
template<class Host, std::size_t N>
bool query_mandatory_prefix(Host& host, const std::array<Request,N>& rows) {
    for (const Request& row : rows) {
        const std::uintptr_t result = host.query(row.supplier, row.version);
        host.publish(row.native_storage_va, result);
        if (result == 0) return false;
    }
    return true; // only the PREFIX succeeded, not entire DLL Init!
}

// Later requests occur after additional unported native initialization.
// This helper models a single late lookup, NOT the intervening control flow.
template<class Host>
bool query_late_request(Host& host, const Request& row) {
    const std::uintptr_t result = host.query(row.supplier, row.version);
    host.publish(row.native_storage_va, result);
    return !row.required || result != 0;
}

inline constexpr std::uint32_t kServerInitVA = 0x221e7640u;
inline constexpr std::uint32_t kClientInitVA = 0x240fe0e0u;
inline constexpr std::uint32_t kServerFailureReturnVA = 0x221e79a1u;
inline constexpr std::uint32_t kClientFailureReturnVA = 0x240fe59eu;
inline constexpr std::size_t kServerInitOriginalLength = 875;
inline constexpr std::size_t kClientInitOriginalLength = 1220;
} // namespace sin_compat::original_dll_initialization