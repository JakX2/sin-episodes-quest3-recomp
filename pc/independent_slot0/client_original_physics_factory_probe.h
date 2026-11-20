#pragma once
// 2006 SiN Episodes client physics-factory read-only diagnostic.
// Original CClientDLL::Init helper 0x24182C60, complete 97-byte
// SHA256 46e079ea76e0cfee721efe3a421d68920cf980079aea4d76d7542f6f9b5281f2.
//
// Original code queries VPhysics030, VPhysicsSurfaceProps001 and
// VPhysicsCollision007 IN THIS ORDER from the *physics* Factory
// (second VClient011::Init argument). Each query passes status=nullptr,
// stores a global and exits false on the first null. Only after all
// three succeed does it call 0x241927A0 with surface props and the
// existing game filesystem. That final coupling method is deliberately
// NOT invoked by this diagnostic.
//
// This class makes three genuine typed Factory calls in an opt-in
// original-engine diagnostic, but never dereferences, stores across
// frames, invokes virtual methods on, or exposes any returned pointers.
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Original SiN 2006 runtime client ABI only verified for MSVC x86
#endif
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace sin_independent::physics_factory_probe {
using Factory = void* (__cdecl *)(const char*, int*);
inline constexpr std::uint32_t kOriginalPhysicsHelperVA=0x24182C60u;
inline constexpr std::size_t kOriginalPhysicsHelperBytes=97u;
inline constexpr std::uint32_t kOriginalPhysicsCouplingVA=0x241927A0u;
inline constexpr std::uint32_t kOriginalPhysicsGlobalVA=0x24374398u;
inline constexpr std::uint32_t kOriginalSurfaceGlobalVA=0x243743A8u;
inline constexpr std::uint32_t kOriginalCollisionGlobalVA=0x243743A0u;
inline constexpr const char* kOriginalNames[3]={
    "VPhysics030","VPhysicsSurfaceProps001","VPhysicsCollision007"
};
enum class Outcome : std::uint8_t {
    AllPresent, MissingFactory, FirstUnavailable,
    SecondUnavailable, ThirdUnavailable
};
inline const char* Name(Outcome x) noexcept {
    switch(x){
        case Outcome::AllPresent: return "original-physics-three-available";
        case Outcome::MissingFactory:return "original-physics-no-factory";
        case Outcome::FirstUnavailable:return "original-physics-missing-first";
        case Outcome::SecondUnavailable:return "original-physics-missing-second";
        case Outcome::ThirdUnavailable:return "original-physics-missing-third";
    }
    return "original-physics-invalid-result";
}
struct Result final {
    Outcome outcome=Outcome::MissingFactory;
    std::uint32_t attempted=0u;
    std::uint32_t found=0u;
    bool would_pass_original_three_query_gate=false;
    // No interface addresses or lifecycle ownership are retained.
};
inline Result ReadOnlyQueryThree(Factory factory) noexcept {
    Result report{};
    if(!factory)return report;
    for(std::size_t i=0;i<3u;++i){
        ++report.attempted;
        // The 2006 binary uses one cdecl Factory callback with a null
        // return-code pointer. Never fabricate an int storage argument.
        void* const resolved=factory(kOriginalNames[i],nullptr);
        if(!resolved){
            report.outcome=i==0u?Outcome::FirstUnavailable:
                           i==1u?Outcome::SecondUnavailable:
                                  Outcome::ThirdUnavailable;
            return report;
        }
        ++report.found;
    }
    report.outcome=Outcome::AllPresent;
    report.would_pass_original_three_query_gate=true;
    return report;
}
inline bool Consistent(const Result& r) noexcept {
    switch(r.outcome){
        case Outcome::AllPresent:
            return r.attempted==3u&&r.found==3u&&
                   r.would_pass_original_three_query_gate;
        case Outcome::MissingFactory:
            return r.attempted==0u&&r.found==0u&&
                   !r.would_pass_original_three_query_gate;
        case Outcome::FirstUnavailable:
            return r.attempted==1u&&r.found==0u&&
                   !r.would_pass_original_three_query_gate;
        case Outcome::SecondUnavailable:
            return r.attempted==2u&&r.found==1u&&
                   !r.would_pass_original_three_query_gate;
        case Outcome::ThirdUnavailable:
            return r.attempted==3u&&r.found==2u&&
                   !r.would_pass_original_three_query_gate;
    }
    return false;
}
static_assert(kOriginalPhysicsHelperBytes==97u);
static_assert(kOriginalPhysicsCouplingVA==0x241927A0u);
} // namespace sin_independent::physics_factory_probe
