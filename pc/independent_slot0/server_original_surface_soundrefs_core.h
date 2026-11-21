#pragma once
// Phase523: fully owned read-only reconstruction of 0x2220D9B0.
// Source-authenticated original SiN 2006 server code enumerates every
// VPhysicsSurfaceProps001 surface and checks the ten unsigned-short
// sound-name indices starting at surfacedata_t+0x2c. Each NONZERO index
// calls virtual slot6 GetString then helper 0x221B6170.
//
// Legacy Source SDK 2004 vphysics_interface.h contains the exact field
// layout (5 physical floats, 6 audio floats, then 10 sound IDs).
// This is a FAKE-object-only controlled pure enumerator: no engine
// pointers, no sound system, no native GetString or helper mutations.
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
namespace sin_independent::server_original_surface_soundrefs {
struct SoundIndices final {
    std::uint16_t stepleft=0;
    std::uint16_t stepright=0;
    std::uint16_t impactSoft=0;
    std::uint16_t impactHard=0;
    std::uint16_t scrapeSmooth=0;
    std::uint16_t scrapeRough=0;
    std::uint16_t bulletImpact=0;
    std::uint16_t rolling=0;
    std::uint16_t breakSound=0;
    std::uint16_t strainSound=0;
};
struct SurfaceView final {
    std::array<std::uint8_t,0x2c> originalPhysicsAndAudio{};
    SoundIndices sounds{};
};
static_assert(sizeof(SoundIndices)==20u);
static_assert(offsetof(SurfaceView,sounds)==0x2cu);
static_assert(offsetof(SurfaceView,sounds.stepleft)==0x2cu);
static_assert(offsetof(SurfaceView,sounds.stepright)==0x2eu);
static_assert(offsetof(SurfaceView,sounds.impactSoft)==0x30u);
static_assert(offsetof(SurfaceView,sounds.impactHard)==0x32u);
static_assert(offsetof(SurfaceView,sounds.scrapeSmooth)==0x34u);
static_assert(offsetof(SurfaceView,sounds.scrapeRough)==0x36u);
static_assert(offsetof(SurfaceView,sounds.bulletImpact)==0x38u);
static_assert(offsetof(SurfaceView,sounds.rolling)==0x3au);
static_assert(offsetof(SurfaceView,sounds.breakSound)==0x3cu);
static_assert(offsetof(SurfaceView,sounds.strainSound)==0x3eu);
inline constexpr std::size_t kSlotsPerSurface=10;
inline constexpr std::array<const char*,kSlotsPerSurface> kFieldNames{
    "stepleft","stepright","impactSoft","impactHard","scrapeSmooth",
    "scrapeRough","bulletImpact","rolling","breakSound","strainSound"
};
inline std::array<std::uint16_t,kSlotsPerSurface> Indices(const SoundIndices& s) noexcept{
    return {s.stepleft,s.stepright,s.impactSoft,s.impactHard,
            s.scrapeSmooth,s.scrapeRough,s.bulletImpact,s.rolling,
            s.breakSound,s.strainSound};
}
enum class Outcome : std::uint8_t {Complete,InvalidCount,MissingData,CallbackRefused};
struct Report final {
    Outcome outcome=Outcome::InvalidCount;
    unsigned surfaces=0,slots_examined=0,zero_ids=0,nonzero_ids=0,forwarded=0;
    bool complete() const noexcept {
        return outcome==Outcome::Complete &&
            slots_examined==surfaces*kSlotsPerSurface &&
            zero_ids+nonzero_ids==slots_examined &&
            forwarded==nonzero_ids;
    }
};
// The real original calls SurfacePropCount repeatedly at the loop head.
// This pure core uses a fixed prevalidated owner-provided count to avoid
// mixing nondeterministic engine state with sound registration. The
// caller supplies synthetic surface views and an INERT callback.
// Every nonzero ID is forwarded in source-authenticated field order.
template<class SurfaceAt,class OnIndex>
Report Scan(unsigned count,SurfaceAt surfaceAt,OnIndex onIndex) noexcept{
    Report report{};
    if(count>16384u)return report;
    for(unsigned surface=0;surface<count;++surface){
        const SoundIndices* row=surfaceAt(surface);
        if(!row){report.outcome=Outcome::MissingData;return report;}
        const auto fields=Indices(*row);
        ++report.surfaces;
        for(std::size_t i=0;i<fields.size();++i){
            ++report.slots_examined;
            if(fields[i]==0) {++report.zero_ids;continue;}
            ++report.nonzero_ids;
            if(!onIndex(surface,static_cast<unsigned>(i),fields[i])){
                report.outcome=Outcome::CallbackRefused;return report;
            }
            ++report.forwarded;
        }
    }
    report.outcome=Outcome::Complete;
    return report;
}
} // namespace sin_independent::server_original_surface_soundrefs
