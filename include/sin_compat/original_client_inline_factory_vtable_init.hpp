#pragma once
// Phase1723: verify original 2006 PE32 50 inline-vtable ClientClass
// creators SHARE one original native network initialization vtable
// method at primary vtable BYTE+0x94, source VA 0x240C8FE0.
// FULL original 75-byte native method SHA256:
// fc701a745f638b46ebb9bacae97369d2a46545dd9bef4351e500ecc8d20bbe92
// Followed by 5 INT3 bytes, no original code bytes committed.
// All 50 original SECONDARY vtables' method at byte+0x00
// resolves to native 4-byte thunk at 0x24054EC0: LEA EAX,[ECX-4]; RET.
// Full native 4-byte thunk SHA256:
// b8bcfc02fb79817e275fdfb8a7c0c73dc6dff41e2b950288a2668b7d0ff6e92d
// Followed by 12 INT3 bytes.
// Original 50 primary/secondary vtable-pointer DWORD pairs SHA256:
// 42db228cc0ea0cb4c213d46e1140656d0dea885e0cd8d56994ff472dbc4dc02e
// Original 50 source virtual target triple DWORDs SHA256:
// ccfbb162a58b55ea5be781e55cef7b386a06327ab06b8b4950707a9d4b725413
// Original whole owner PE32 client.dll SHA256:
// 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
//
// The original 75B initializer itself calls the object's +4 interface
// vtable method +0, engine global at 0x242FA738, source helper
// 0x241280A0, method on object+0x16C at 0x24119570, first vtable
// byte+0x44, helper 0x240C8E90, stores first input at owner+0x50,
// returns AL=1 and RET 8. This is a true runtime side-effectful
// native engine function, NOT a host pointer-free pure calculation.
// This HEADER PRESERVES ONLY SOURCE CODE IDENTITY + DEPENDENCY PLAN.
// No x86 vtables, process globals or callback pointers are executed.
// Original 20B ClientClass +0/+4 callbacks NULL, retail slot5 blocked.
#include <array>
#include <cstddef>
#include <cstdint>
#include "sin_compat/original_client_canonical_creator_bodies.hpp"

namespace sin_compat::original_client_inline_factory_vtable_init {
namespace canonical=sin_compat::original_client_canonical_creator_bodies;
namespace creators=sin_compat::original_client_creator_source_map;
inline constexpr std::size_t kInlineNativeCreators=50u;
inline constexpr std::size_t kDistinctSourceVirtual44Targets=3u;
inline constexpr std::uint32_t kSourcePrimaryVirtual94Target=0x240C8FE0u;
inline constexpr std::uint32_t kSourceSecondaryVirtual0Target=0x24054EC0u;
inline constexpr std::uint32_t kNativeVirtualInitializerBytes=75u;
inline constexpr std::uint32_t kNativeSecondaryAdjustorBytes=4u;
inline constexpr std::uint32_t kSourceSecondaryInterfaceOffset=4u;
inline constexpr std::uint32_t kSourceNetworkableReturnOffset=8u;
inline constexpr std::uint32_t kSourceOwnerField50=0x50u;
inline constexpr std::uint32_t kOriginalEngineStateGlobal=0x242FA738u;
inline constexpr std::uint32_t kNativeInitInterfaceHelper=0x241280A0u;
inline constexpr std::uint32_t kNativeInitOwnerFieldHelper=0x24119570u;
inline constexpr std::uint32_t kNativeInitFollowupHelper=0x240C8E90u;
struct OriginalSourceVirtual44Route final {
    std::uint16_t originalClassOrdinal;
    std::uint32_t originalPrimaryVirtual44TargetVA;
};
inline constexpr std::array<OriginalSourceVirtual44Route,kInlineNativeCreators>
kOriginalSourceVirtual44Routes{{
    {0u, 0x24062070u},
    {1u, 0x24062070u},
    {2u, 0x24062070u},
    {3u, 0x24062070u},
    {4u, 0x24062070u},
    {5u, 0x24062070u},
    {6u, 0x24062070u},
    {7u, 0x24062070u},
    {8u, 0x24062070u},
    {10u, 0x24062070u},
    {11u, 0x24062070u},
    {12u, 0x24062070u},
    {13u, 0x24062070u},
    {14u, 0x24062070u},
    {15u, 0x24062070u},
    {16u, 0x24062070u},
    {17u, 0x24062070u},
    {18u, 0x24062070u},
    {19u, 0x24062070u},
    {20u, 0x24062070u},
    {21u, 0x24062070u},
    {22u, 0x24062070u},
    {23u, 0x24062070u},
    {24u, 0x24062070u},
    {25u, 0x24062070u},
    {26u, 0x24062070u},
    {27u, 0x24062070u},
    {28u, 0x24062070u},
    {29u, 0x24062070u},
    {39u, 0x24062070u},
    {42u, 0x240982d0u},
    {56u, 0x24054e80u},
    {57u, 0x24062070u},
    {58u, 0x24062070u},
    {59u, 0x24062070u},
    {60u, 0x240982d0u},
    {64u, 0x24054e80u},
    {65u, 0x24062070u},
    {72u, 0x24054e80u},
    {76u, 0x24054e80u},
    {85u, 0x24054e80u},
    {97u, 0x24054e80u},
    {100u, 0x24054e80u},
    {102u, 0x24054e80u},
    {103u, 0x24054e80u},
    {109u, 0x24054e80u},
    {112u, 0x24054e80u},
    {124u, 0x24054e80u},
    {127u, 0x240982d0u},
    {186u, 0x24054e80u}
}};
enum class Status : std::uint8_t {
    RecognizedSourceDependency, NotAnAuthenticatedInlineSourceCreator
};
struct OriginalVirtualInitDependency final {
    Status status;
    std::uint32_t originalPrimaryVirtual94VA;
    std::uint32_t originalSecondaryVirtual0VA;
    std::uint32_t originalPrimaryVirtual44VA;
    std::uint32_t secondaryThisObjectOffset;
    std::uint32_t returnedObjectSubobjectOffset;
    std::uint32_t mutatedOwnerFieldOffset;
};
constexpr OriginalVirtualInitDependency SourceVirtualInitDependencyFor(
    std::size_t classOrdinal,std::uint32_t originalCreatorVA) noexcept {
    const auto plan=canonical::SourceCanonicalBodyPlan(
        classOrdinal,originalCreatorVA);
    if(plan.family!=canonical::Family::InlineFourVtables86B)
        return {Status::NotAnAuthenticatedInlineSourceCreator,
                0u,0u,0u,0u,0u,0u};
    for(const auto& route:kOriginalSourceVirtual44Routes)
        if(route.originalClassOrdinal==classOrdinal)
            return {Status::RecognizedSourceDependency,
                    kSourcePrimaryVirtual94Target,
                    kSourceSecondaryVirtual0Target,
                    route.originalPrimaryVirtual44TargetVA,
                    kSourceSecondaryInterfaceOffset,
                    kSourceNetworkableReturnOffset,kSourceOwnerField50};
    return {Status::NotAnAuthenticatedInlineSourceCreator,
            0u,0u,0u,0u,0u,0u};
}
constexpr bool SourceInlineVtableRoutesComplete() noexcept {
    unsigned group1=0u,group2=0u,group3=0u;
    std::size_t found=0u;
    for(std::size_t i=0u;i<kOriginalSourceVirtual44Routes.size();++i) {
        const auto& route=kOriginalSourceVirtual44Routes[i];
        if(route.originalClassOrdinal>=creators::kOriginalClasses ||
           creators::SourceCreatorIsTemp(route.originalClassOrdinal))
            return false;
        if(route.originalPrimaryVirtual44TargetVA==0x24062070u)++group1;
        else if(route.originalPrimaryVirtual44TargetVA==0x24054E80u)++group2;
        else if(route.originalPrimaryVirtual44TargetVA==0x240982D0u)++group3;
        else return false;
        for(std::size_t j=0u;j<i;++j)
            if(kOriginalSourceVirtual44Routes[j].originalClassOrdinal==
                route.originalClassOrdinal)
                return false;
        const auto va=creators::SourceCreatorVAForOrdinal(
            route.originalClassOrdinal);
        const auto plan=canonical::SourceCanonicalBodyPlan(
            route.originalClassOrdinal,va);
        if(plan.family!=canonical::Family::InlineFourVtables86B)return false;
        ++found;
    }
    return found==kInlineNativeCreators &&
           group1==34u && group2==13u && group3==3u;
}
inline constexpr bool CanInvokeRetailVirtualMethods() noexcept {return false;}
static_assert(SourceInlineVtableRoutesComplete());
static_assert(!CanInvokeRetailVirtualMethods());
} // namespace sin_compat::original_client_inline_factory_vtable_init
