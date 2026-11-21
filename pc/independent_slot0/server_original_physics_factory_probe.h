#pragma once
// Original SiN Episodes 2006 server physics Factory sequence.
// SHA-authenticated owner server.dll, helper 0x2220D910, 127 bytes,
// exact SHA256 415f272c8f5fd8406c8a2a25e04ad99064f7476284e91fd9fa834acd58cbe838.
// IMPORTANT: differs from original client Init order.
// Original server: VPhysics030, VPhysicsCollision007, VPhysicsSurfaceProps001.
// The subsequent server 0x22234060 manifest loading and actual native
// surface registration are NOT invoked by this read-only component.
// Real-call-capable ONLY under separate opt-in; no engine pointers retained.
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Original 2006 SiN server physics ABI requires MSVC Windows x86
#endif
#include <cstdint>
#include <cstddef>
#include <cstring>
namespace sin_independent::server_original_physics_factory {
using Factory=void* (__cdecl *)(const char*,int*);
inline constexpr std::uint32_t kOriginalHelperVA=0x2220D910u;
inline constexpr std::size_t kOriginalHelperLength=127u;
inline constexpr std::uint32_t kOriginalPhysicsGlobal=0x22575390u;
inline constexpr std::uint32_t kOriginalCollisionGlobal=0x22575398u;
inline constexpr std::uint32_t kOriginalSurfaceGlobal=0x225753A0u;
inline constexpr std::uint32_t kOriginalSurfaceManifestHelper=0x22234060u;
inline constexpr const char* kNames[3]={
    "VPhysics030","VPhysicsCollision007","VPhysicsSurfaceProps001"
};
enum class Outcome:std::uint8_t{
    AllThreePresent,MissingFactory,MissingPhysics,
    MissingCollision,MissingSurfaceProps
};
inline const char* Name(Outcome o) noexcept {
    switch(o){
        case Outcome::AllThreePresent:return "original-server-physics-three-available";
        case Outcome::MissingFactory:return "original-server-physics-missing-factory";
        case Outcome::MissingPhysics:return "original-server-physics-missing-first";
        case Outcome::MissingCollision:return "original-server-physics-missing-second";
        case Outcome::MissingSurfaceProps:return "original-server-physics-missing-third";
    }
    return "original-server-physics-invalid-result";
}
struct Result final {
    Outcome outcome=Outcome::MissingFactory;
    unsigned attempted=0,found=0;
    bool original_query_gate=true;
    bool successful() const noexcept {
        return outcome==Outcome::AllThreePresent &&
               attempted==3 && found==3 && original_query_gate;
    }
};
inline Result ReadOnlyThree(Factory factory) noexcept {
    Result r{};
    r.original_query_gate=false;
    if(!factory)return r;
    for(unsigned i=0;i<3;++i){
        ++r.attempted;
        void* value=factory(kNames[i],nullptr);
        if(!value){
            r.outcome=i==0?Outcome::MissingPhysics:
                      i==1?Outcome::MissingCollision:Outcome::MissingSurfaceProps;
            return r;
        }
        ++r.found;
    }
    r.outcome=Outcome::AllThreePresent;
    r.original_query_gate=true;
    return r;
}
inline bool Consistent(Result r) noexcept {
    switch(r.outcome){
        case Outcome::AllThreePresent:return r.attempted==3&&r.found==3&&r.original_query_gate;
        case Outcome::MissingFactory:return r.attempted==0&&r.found==0&&!r.original_query_gate;
        case Outcome::MissingPhysics:return r.attempted==1&&r.found==0&&!r.original_query_gate;
        case Outcome::MissingCollision:return r.attempted==2&&r.found==1&&!r.original_query_gate;
        case Outcome::MissingSurfaceProps:return r.attempted==3&&r.found==2&&!r.original_query_gate;
    }
    return false;
}
static_assert(kOriginalHelperLength==127u);
} // namespace sin_independent::server_original_physics_factory
