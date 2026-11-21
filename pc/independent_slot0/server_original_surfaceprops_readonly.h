#pragma once
// Phase520: original SHA-pinned 2006 server physics resource bootstrap,
// strictly READ ONLY. Server original 0x2220D910 queries physics, collision,
// then surface-props. Its 0x22234060 manifest and 0x22233FD0 per-file loader
// use VFileSystem012, scripts/surfaceproperties_manifest.txt (rb/GAME),
// then two source scripts (rb/null). NEVER call original ParseSurfaceData.
// The independently compiled client bridge is role-neutral despite its name;
// both original DLLs use the same VFileSystem012 method ABI and resource shape.
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Original SiN physics resources are Windows MSVC x86-only
#endif
#include "server_original_physics_factory_probe.h"
#include "client_original_surfaceprops_engine_fs_bridge.h"
#include <cstdint>

namespace sin_independent::server_original_surfaceprops {
namespace physics=sin_independent::server_original_physics_factory;
namespace bridge=sin_independent::surfaceprops_engine_fs;

enum class Outcome : std::uint8_t {
    FactoryNotVerified, InvalidFileSystem, ResourceRefused, TwoSourcesVerified
};
inline const char* Name(Outcome o) noexcept {
    switch(o){
        case Outcome::FactoryNotVerified: return "original-server-surfaceprops-factory-refused";
        case Outcome::InvalidFileSystem: return "original-server-surfaceprops-invalid-filesystem";
        case Outcome::ResourceRefused: return "original-server-surfaceprops-resource-refused";
        case Outcome::TwoSourcesVerified: return "original-server-surfaceprops-two-sources-verified";
    }
    return "original-server-surfaceprops-unknown";
}
struct Report final {
    Outcome outcome=Outcome::FactoryNotVerified;
    unsigned attempted=0,read=0,definitions=0,base_keys=0,repeated_names=0;
    bool verified() const noexcept {
        return outcome==Outcome::TwoSourcesVerified && attempted==2u &&
            read==2u && definitions==93u && base_keys==71u &&
            repeated_names==0u;
    }
};
inline Report ReadOnlyAfterFactory(physics::Result factory,void* filesystem) noexcept {
    Report r{};
    if(!physics::Consistent(factory) || !factory.successful())return r;
    bridge::Bridge own{};
    const auto result=own.ReadOnly(filesystem);
    r.attempted=result.owned_result.attempted;
    r.read=result.owned_result.read;
    r.definitions=result.owned_result.definitions;
    r.base_keys=result.owned_result.base_keys;
    r.repeated_names=result.owned_result.repeated_names;
    r.outcome=result.status==bridge::Status::InvalidFileSystem?
        Outcome::InvalidFileSystem:
        result.verified()?Outcome::TwoSourcesVerified:Outcome::ResourceRefused;
    return r;
}
} // namespace sin_independent::server_original_surfaceprops
