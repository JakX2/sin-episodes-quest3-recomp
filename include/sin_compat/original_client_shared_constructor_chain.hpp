#pragma once
// Phase1723: three original 2006 PE32 COMMON constructor target functions
// form an explicit source call chain, used by 60 of 142 authenticated
// whole native ClientClass entity creator methods (Phase1722).
//
// Source original SHA-pinned unchanged client.dll:
// 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
//
// Entry 0x24092BD0: complete 206 executable bytes, SHA256
// 28e8f086a532a606b78304522eaaeb571f63922feaee80aaaeeacb7e8a03659a
// Exact CALL at +3 to 0x240BD7C0. 34 original entity creators
// directly invoke it. Native method ends RET, 2 INT3 bytes.
//
// Entry 0x240BD7C0: complete 861 executable bytes, SHA256
// 194b3ef35fa66ab726f06f1ce37539c6901ced7a3d22995a228f00aa04e62db0
// Exact CALL at +7 to 0x240CCE30. 3 INT3 bytes after RET.
// No direct source creator points here in Phase1722, but the
// 34-creator upstream family reaches it.
//
// Entry 0x240CCE30: complete 573 executable bytes, SHA256
// 63fc9f1bbd6c8fe318f7faa1cd4aef448dd19553a6dbdc0d182b768855b34eca
// 26 original entity creators directly invoke it. 3 INT3 after RET.
// Its own first decoded internal CALL targets 0x24195260 (yet another
// dependency, not reconstructed). This is a source call graph, not
// a portable factory or finished constructor chain!
//
// Concatenated complete 206+861+573 original native bytes SHA256:
// 3a02d8d7ff3c50af8c1096bcc00be731e02426a0b4f06112138e72cb99ec70ec
//
// These original source VA labels are NEVER executable host functions;
// no native original bytecode, original vtables or Win32 pointers are
// committed, dereferenced or mapped as Android callbacks.
// All native32 ClientClass +0/+4 callbacks NULL, slot5 guarded.
#include <cstddef>
#include <cstdint>
#include <array>
#include "sin_compat/original_client_full_entity_creator_bodies.hpp"
namespace sin_compat::original_client_shared_constructor_chain {
namespace creators=sin_compat::original_client_full_entity_creator_bodies;
inline constexpr std::uint32_t kNativeSharedConstructorA=0x24092BD0u;
inline constexpr std::uint32_t kNativeSharedConstructorB=0x240BD7C0u;
inline constexpr std::uint32_t kNativeSharedConstructorC=0x240CCE30u;
inline constexpr std::uint32_t kUnreconstructedNestedConstructorVA=0x24195260u;
inline constexpr std::size_t kNativeSourceConstructorCount=3u;
inline constexpr std::size_t kNativeAExecutableBytes=206u;
inline constexpr std::size_t kNativeBExecutableBytes=861u;
inline constexpr std::size_t kNativeCExecutableBytes=573u;
inline constexpr std::size_t kTotalOriginalExecutableBytes=1640u;
inline constexpr std::size_t kNativeDirectAClassCreators=34u;
inline constexpr std::size_t kNativeDirectCClassCreators=26u;
inline constexpr std::size_t kNativeOtherConstructorTargets=82u;
inline constexpr std::size_t kReachThisChainCreators=60u;

enum class SourceConstructorRoute : std::uint8_t {
    UnsupportedOriginalTarget,
    DirectAThroughBToC,
    DirectC
};
constexpr SourceConstructorRoute RouteForSourceConstructorTarget(
    std::uint32_t targetVA) noexcept {
    return targetVA==kNativeSharedConstructorA?
        SourceConstructorRoute::DirectAThroughBToC:
        targetVA==kNativeSharedConstructorC?
        SourceConstructorRoute::DirectC:
        SourceConstructorRoute::UnsupportedOriginalTarget;
}
struct NativeSourceFunction final {
    std::uint32_t originalFunctionVA;
    std::uint16_t completeExecutableBytes;
    std::uint8_t trailingInt3Bytes;
    std::uint16_t firstCallOffset;
    std::uint32_t firstCallTargetVA;
};
inline constexpr std::array<NativeSourceFunction,kNativeSourceConstructorCount>
kOriginalFunctionChain{{
    {kNativeSharedConstructorA,206u,2u,3u,kNativeSharedConstructorB},
    {kNativeSharedConstructorB,861u,3u,7u,kNativeSharedConstructorC},
    {kNativeSharedConstructorC,573u,3u,86u,kUnreconstructedNestedConstructorVA}
}};
constexpr bool SourceChainInventoryConsistent() noexcept {
    std::size_t directA=0u,directC=0u,other=0u;
    std::size_t total=0u;
    for(const auto& function:kOriginalFunctionChain) {
        if(function.completeExecutableBytes==0u ||
           function.completeExecutableBytes>1000u ||
           function.firstCallOffset+5u>=function.completeExecutableBytes ||
           function.trailingInt3Bytes>15u ||
           function.originalFunctionVA<0x24001000u ||
           function.originalFunctionVA>=0x24275FBAu)
            return false;
        total+=function.completeExecutableBytes;
    }
    if(kOriginalFunctionChain[0].firstCallTargetVA!=kNativeSharedConstructorB ||
       kOriginalFunctionChain[1].firstCallTargetVA!=kNativeSharedConstructorC ||
       kOriginalFunctionChain[2].firstCallTargetVA!=kUnreconstructedNestedConstructorVA)
        return false;
    for(const auto& record:creators::kSourceCompleteCreatorBodies) {
        const auto group=RouteForSourceConstructorTarget(
            record.originalConstructorTargetVA);
        if(group==SourceConstructorRoute::DirectAThroughBToC)++directA;
        else if(group==SourceConstructorRoute::DirectC)++directC;
        else ++other;
    }
    return directA==kNativeDirectAClassCreators &&
           directC==kNativeDirectCClassCreators &&
           other==kNativeOtherConstructorTargets &&
           directA+directC==kReachThisChainCreators &&
           total==kTotalOriginalExecutableBytes &&
           creators::SourceFullBodyInventoryConsistent();
}
inline constexpr bool CanRunOriginalConstructorChain() noexcept {return false;}
static_assert(SourceChainInventoryConsistent());
static_assert(!CanRunOriginalConstructorChain());
} // namespace sin_compat::original_client_shared_constructor_chain
