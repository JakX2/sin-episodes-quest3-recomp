#pragma once
// Phase522: read-only physics material parameters already loaded by the
// original SiN Episodes 2006 engine. Owner 2026-11-21 pre/post traces prove
// the engine table is already 94 entries BEFORE independent ParseSurfaceData.
// No engine mutation is necessary to obtain current material data.
//
// Physics object ABI: VPhysicsSurfaceProps001 vslot2 Count, vslot3 Index,
// vslot5 GetSurfaceData(index). Original server calls +0x14 at 0x2220D9CF,
// original client does the same at 0x24184F62. SDK 2004/2006 specifies
// surfacedata_t starts with five IEEE754 surfacephysicsparams_t floats:
// friction, elasticity, density, thickness, dampening. Copy ONLY 20 bytes
// from a VirtualQuery-readable pointer, never publish original script text.
// This stage never calls slot1 ParseSurfaceData or changes engine state.
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Original SiN 2006 surfacephysics ABI requires MSVC Windows x86
#endif
#include "client_original_surfaceprops_readback.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <cstdint>

namespace sin_independent::original_surfacephysics_data {
namespace index=sin_independent::original_surfaceprops_readback;
struct Physics final {
    float friction=0,elasticity=0,density=0,thickness=0,dampening=0;
};
static_assert(sizeof(Physics)==20,"Source2004 physics properties are 5 floats");
using GetSurfaceData=const Physics* (__thiscall *)(void*,int);
enum class Outcome : std::uint8_t {
    InvalidSurface,IndexRefused,NoDataSlot,NullData,UnreadableData,
    NonFiniteData,ThreePhysicsSamples
};
inline const char* Name(Outcome o) noexcept {
    switch(o){
        case Outcome::InvalidSurface:return "original-physics-material-invalid-object";
        case Outcome::IndexRefused:return "original-physics-material-index-refused";
        case Outcome::NoDataSlot:return "original-physics-material-no-data-slot";
        case Outcome::NullData:return "original-physics-material-null-data";
        case Outcome::UnreadableData:return "original-physics-material-unreadable-data";
        case Outcome::NonFiniteData:return "original-physics-material-nonfinite-data";
        case Outcome::ThreePhysicsSamples:return "original-physics-material-three-samples";
    }
    return "original-physics-material-unknown";
}
struct Report final {
    Outcome outcome=Outcome::InvalidSurface;
    int count=-1;
    std::array<int,3> indices{-1,-1,-1};
    std::array<Physics,3> values{};
    unsigned sampled=0;
    bool verified() const noexcept {
        return outcome==Outcome::ThreePhysicsSamples && count>=3 &&
            count<=16384 && sampled==3;
    }
};
inline bool Finite(Physics p) noexcept {
    const float values[5]={p.friction,p.elasticity,p.density,
                           p.thickness,p.dampening};
    for(float f:values)
        if(!std::isfinite(f) || std::fabs(f)>1000000000.0f)return false;
    return true;
}
inline Report ReadOnly(void* props) noexcept {
    Report r{};
    if(!props)return r;
    const auto idx=index::Query(props);
    r.count=idx.count;
    r.indices=idx.indices;
    if(!idx.all_found()){
        r.outcome=Outcome::IndexRefused;
        return r;
    }
    using sin_independent::known_game_system::CallResult;
    CallResult refused=CallResult::InvalidObject;
    void* raw=sin_independent::known_game_system::VerifiedMethod(
        props,5,refused);
    if(!raw){
        r.outcome=Outcome::NoDataSlot;
        return r;
    }
    const auto getData=reinterpret_cast<GetSurfaceData>(raw);
    for(std::size_t i=0;i<r.indices.size();++i) {
        // GetSurfaceData returns an engine-owned pointer. The underlying
        // struct layout has to match the original (the code pin confirms
        // the slot, Source2004 confirms first 5 x86 floats).
        const Physics* rawData=getData(props,r.indices[i]);
        if(!rawData){r.outcome=Outcome::NullData;return r;}
        if(!sin_independent::vgui_layout::Readable(rawData,sizeof(Physics))) {
            r.outcome=Outcome::UnreadableData;return r;
        }
        Physics owned{};
        std::memcpy(&owned,rawData,sizeof(owned));
        if(!Finite(owned)){r.outcome=Outcome::NonFiniteData;return r;}
        r.values[i]=owned;
        ++r.sampled;
    }
    r.outcome=Outcome::ThreePhysicsSamples;
    return r;
}
} // namespace sin_independent::original_surfacephysics_data
