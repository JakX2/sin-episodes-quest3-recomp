#pragma once
// Source SDK 2004-guided, actual-engine-read-capable VPhysicsSurfaceProps001
// post-registration introspection. The original SiN 2006 module supplies
// this version and its native 0x24191EC0 calls ParseSurfaceData at vslot1.
// SDK 2004 and 2006 legacy VPhysics030 headers agree on the derived slots:
// vslot0 virtual destructor, vslot1 ParseSurfaceData,
// vslot2 SurfacePropCount, vslot3 GetSurfaceIndex.
// Original SiN client x86 additionally confirms vslot3 calls through
// global 0x243743A8: 0x240B7EC5, 0x24191E3C and 0x241A7389.
// Audit: audit_sin_original_surfaceprops_index_calls.py. Slot2 Count
// remains SDK-guided only, not directly authenticated in original DLL.
// Both slot2 and slot3 independent calls are still unobserved on owner
// retail engine; don't claim actual startup success on synthetic proof.
// This never modifies physics state, never retains interface pointers, and
// is opt-in ONLY AFTER the mutating ParseSurfaceData experiment.
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Source 2004 physics surfaceprops ABI is Win32 MSVC x86
#endif
#include "client_known_gamesystem_init.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_independent::original_surfaceprops_readback {
using Count=int (__thiscall *)(void*);
using Index=int (__thiscall *)(void*,const char*);
inline constexpr std::array<const char*,3> kNames{
    "default", "solidmetal", "metalvehicle"
};
// Names independently observed in original owner-private 2006 scripts:
// "default" and "solidmetal": surfaceproperties.txt;
// "metalvehicle": surfaceproperties_hl2.txt.
// The names are probe metadata only; no original script text is included.
enum class Outcome : std::uint8_t {
    InvalidObject, NoCountSlot, NoIndexSlot, CountInvalid,
    IndexInvalid, SomeNotFound, ThreeInRange
};
inline const char* Name(Outcome x) noexcept {
    switch(x){
        case Outcome::InvalidObject: return "original-physics-surface-readback-invalid-object";
        case Outcome::NoCountSlot: return "original-physics-surface-readback-missing-count-slot";
        case Outcome::NoIndexSlot: return "original-physics-surface-readback-missing-index-slot";
        case Outcome::CountInvalid: return "original-physics-surface-readback-count-invalid";
        case Outcome::IndexInvalid: return "original-physics-surface-readback-index-invalid";
        case Outcome::SomeNotFound: return "original-physics-surface-readback-some-unavailable";
        case Outcome::ThreeInRange: return "original-physics-surface-readback-three-in-range";
    }
    return "original-physics-surface-readback-unrecognized";
}
struct Report final {
    Outcome outcome=Outcome::InvalidObject;
    int count=-1;
    std::array<int,3> indices{-1,-1,-1};
    bool all_found() const noexcept {
        if(outcome!=Outcome::ThreeInRange||count<1||count>16384)return false;
        for(int value:indices)
            if(value<0||value>=count)return false;
        return true;
    }
};
inline Report Query(void* surfaceProps) noexcept {
    Report r{};
    if(!surfaceProps)return r;
    using sin_independent::known_game_system::CallResult;
    CallResult refused=CallResult::InvalidObject;
    void* countSlot=sin_independent::known_game_system::VerifiedMethod(
        surfaceProps,2,refused);
    if(!countSlot){
        r.outcome=Outcome::NoCountSlot;
        return r;
    }
    void* indexSlot=sin_independent::known_game_system::VerifiedMethod(
        surfaceProps,3,refused);
    if(!indexSlot){
        r.outcome=Outcome::NoIndexSlot;
        return r;
    }
    auto count=reinterpret_cast<Count>(countSlot);
    auto index=reinterpret_cast<Index>(indexSlot);
    r.count=count(surfaceProps);
    // Bound the untrusted ABI results before issuing index queries.
    // The lower bound allows a real empty database to be reported
    // without erroneously claiming that the interface is invalid.
    if(r.count<0||r.count>16384){
        r.outcome=Outcome::CountInvalid;
        return r;
    }
    bool absent=false;
    for(std::size_t i=0;i<kNames.size();++i) {
        r.indices[i]=index(surfaceProps,kNames[i]);
        if(r.indices[i]==-1)absent=true;
        else if(r.indices[i]<-1||r.indices[i]>=r.count){
            r.outcome=Outcome::IndexInvalid;
            return r;
        }
    }
    r.outcome=absent?Outcome::SomeNotFound:Outcome::ThreeInRange;
    return r;
}
} // namespace sin_independent::original_surfaceprops_readback
