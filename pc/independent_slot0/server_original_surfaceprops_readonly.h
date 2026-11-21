#pragma once
// Phase 521: SHA-source-authenticated 2006 server physics bootstrap's
// READ-ONLY VFileSystem012 manifest walk. No original engine registration,
// Source KeyValues allocation, vphysics body ownership or game initialization.
//
// Original server method 0x2220D910 (127 bytes) calls 0x22234060 after
// physics 3/3, using global original server filesystem 0x225721A4.
// Original 0x22234060 (247 bytes) reads manifest with GAME path ID;
// original 0x22233FD0 (136 bytes) opens each file rb/null.
// We reuse independently owned + synthetic-tested shared bridge. The
// original proprietary bytes and script texts are NOT committed.
//
// MSVC Win32 x86 VFileSystem012 virtual layout previously owner-observed
// through independent CLIENT filesystem stage; SERVER runtime not yet observed.
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Original SiN 2006 server physics bridge requires MSVC Win32 x86
#endif
#include "client_original_surfaceprops_engine_fs_bridge.h"
#include "server_original_physics_factory_probe.h"
#include <cstdint>
namespace sin_independent::server_original_surfaceprops_readonly {
namespace fac=sin_independent::server_original_physics_factory;
namespace fs=sin_independent::surfaceprops_engine_fs;
enum class Outcome : std::uint8_t {
    NoPhysics, NoFileSystem, InvalidSources, Verified
};
inline const char* Name(Outcome outcome) noexcept {
    switch(outcome) {
        case Outcome::NoPhysics:return "original-server-surfaceprops-physics-refused";
        case Outcome::NoFileSystem:return "original-server-surfaceprops-filesystem-refused";
        case Outcome::InvalidSources:return "original-server-surfaceprops-resources-refused";
        case Outcome::Verified:return "original-server-surfaceprops-read-verified";
    }
    return "original-server-surfaceprops-unknown";
}
struct Report final {
    Outcome outcome=Outcome::NoPhysics;
    fac::Result physics{};
    fs::Report resources{};
    bool verified() const noexcept {
        return outcome==Outcome::Verified && physics.successful() &&
               resources.verified() &&
               resources.owned_result.attempted==2 &&
               resources.owned_result.read==2 &&
               resources.owned_result.definitions==93 &&
               resources.owned_result.base_keys==71 &&
               resources.owned_result.repeated_names==0;
    }
};
// Includes the exact original server physics Factory ordering first.
// Once physics 3/3 has been proven, read the original manifest/scripts
// through bounded VFileSystem012 without calling ParseSurfaceData.
inline Report Inspect(fac::Factory physicsFactory,void* filesystem) noexcept {
    Report r{};
    r.physics=fac::ReadOnlyThree(physicsFactory);
    if(!r.physics.successful())return r;
    fs::Bridge bridge{};
    r.resources=bridge.ReadOnly(filesystem);
    if(r.resources.status==fs::Status::InvalidFileSystem)
        r.outcome=Outcome::NoFileSystem;
    else if(r.resources.verified())r.outcome=Outcome::Verified;
    else r.outcome=Outcome::InvalidSources;
    return r;
}
} // namespace sin_independent::server_original_surfaceprops_readonly
