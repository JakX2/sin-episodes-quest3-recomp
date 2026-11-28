#pragma once
// Phase1725: source-only 2006 PE32 CWorld singleton native lifecycle.
// Original CWorld ClientClass creator (ordinal 75) does NOT allocate;
// it calls the already-bound global original 0x2433F3F4 and returns +8.
// Original 2006 PE32 client initialization CALL at 0x240FE590 invokes
// source 0x240FCDD0: 74B allocator+world constructor and global store.
// Original client shutdown CALL at 0x240FF3D4 invokes source 0x240FC900:
// 27B virtual destructor + clear original global.
// Original simple getter 0x240FCD40 is 6B MOV EAX,[GLOBAL];RET.
//
// Whole executable original function SHA256, source only:
// init 0x240FCDD0 74B: 8a9a82e9e7a8bbd9eb61c80802270817f00c8053bff3b21ba941e9c46311d289
// shutdown 0x240FC900 27B: 71721cdd2db6d9210f6b335faca52209973d792a0aa0b802c0c7306241303e88
// getter 0x240FCD40 6B: b07f64b7fbb5a63cfd4816fcc0481ce9f61be29ae5eae77d80f758bd319a0d06
// CWorld Class creator 0x240FC920 40B:
// 8b06decbe2712135eef0780b69446b5147fbf52c618fa78e7c46aa3fcffde274
// Whole owner client.dll SHA256:
// 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
//
// The original startup requests 0x4B0 (1200) ZEROED bytes via
// 0x240C4FF0, constructor 0x240CCE30, then four original native
// CWorld RTTI vtables at +0,+4,+8,+12, stores singleton global.
// CWorld primary +0x94 target is 0x240FCB10, a FULL 44-byte wrapper
// (SHA256 305044e8173fc81a50952318532ec1a5638e4c956dc056285e048ef92448b35d).
// It clears original owner+0x480, calls original 0x24085750 and
// 0x24129480, then calls shared 75B initializer 0x240C8FE0.
// Source wrapper has RET8 and four trailing INT3 bytes.
// It is NOT the same vtable target as generic inline factory,
// but delegates to that method AFTER world-only side effects.
// Secondary+0 is same 4-byte
// 0x24054EC0 this-minus-four interface adjustor (Phase1723).
//
// No code here dereferences the old x86 globals/vtables, invokes
// original constructors, destructors, engine methods or allocators.
// This models ONLY a fail-closed synthetic object BINDING PLAN.
// Retail VClient011.slot5 and native +0/+4 callbacks remain BLOCKED.
#include <cstddef>
#include <cstdint>
#include "sin_compat/original_client_seven_creator_exceptions.hpp"
#include "sin_compat/original_client_inline_factory_vtable_init.hpp"

namespace sin_compat::original_client_world_singleton_lifecycle {
namespace special=sin_compat::original_client_seven_creator_exceptions;
namespace creators=sin_compat::original_client_creator_source_map;
namespace inlinevt=sin_compat::original_client_inline_factory_vtable_init;
inline constexpr std::size_t kWorldClassOrdinal=75u;
inline constexpr std::uint32_t kWorldOriginalCreatorVA=0x240FC920u;
inline constexpr std::uint32_t kWorldNativeSingletonGlobalVA=0x2433F3F4u;
inline constexpr std::uint32_t kNativeWorldStartupCallerVA=0x240FE590u;
inline constexpr std::uint32_t kNativeWorldStartupVA=0x240FCDD0u;
inline constexpr std::uint32_t kWorldStartupExecutableBytes=74u;
inline constexpr std::uint32_t kNativeWorldShutdownCallerVA=0x240FF3D4u;
inline constexpr std::uint32_t kNativeWorldShutdownVA=0x240FC900u;
inline constexpr std::uint32_t kWorldShutdownExecutableBytes=27u;
inline constexpr std::uint32_t kNativeWorldSourceGetterVA=0x240FCD40u;
inline constexpr std::uint32_t kWorldGetterExecutableBytes=6u;
inline constexpr std::uint32_t kNativeZeroingAllocatorVA=0x240C4FF0u;
inline constexpr std::uint32_t kNativeWorldConstructorVA=0x240CCE30u;
inline constexpr std::uint32_t kNativeWorldRequestBytes=0x4B0u;
inline constexpr std::uint32_t kNativeWorldReturnNetworkableOffset=8u;
inline constexpr std::uint32_t kNativeWorldPrimaryVirtual94VA=0x240FCB10u;
inline constexpr std::uint32_t kNativeWorldNetworkVirtualWrapperBytes=44u;
inline constexpr std::uint32_t kNativeWorldInitClearsFieldOffset=0x480u;
inline constexpr std::uint32_t kNativeWorldPreSharedInitHelperOneVA=0x24085750u;
inline constexpr std::uint32_t kNativeWorldPreSharedInitHelperTwoVA=0x24129480u;
inline constexpr std::uint32_t kNativeSharedNetworkInitializerVA=0x240C8FE0u;
inline constexpr std::uint32_t kNativeWorldPrimaryVirtual44VA=0x24054E80u;
inline constexpr std::uint32_t kNativeWorldSecondaryVirtual0VA=0x24054EC0u;
inline constexpr std::uint32_t kOriginalHostTokenBase=0x10000000u;
inline constexpr std::uint32_t kOriginalHostTokenLimit=0x20000000u;

enum class Status : std::uint8_t {
    RequiresOriginalAllocationAndConstructor,
    RefusedUnboundWorldObject,
    RefusedUnsafeSyntheticWorldToken,
    RequiresOriginalWorldNetworkInitialization,
    RequiresOriginalVirtualDestroy,
    WorldGlobalWouldBeClearedWithoutDestroy
};
struct HostOwnedWorldBinding final {
    bool hasAuthenticatedWorldObject=false;
    // The SOURCE original singleton is not available in a new process.
    // These are NOT physical 2006 x86 pointers, only opaque test tokens.
    std::uint32_t syntheticWorldObjectToken=0u;
};
constexpr bool SafeSyntheticWorldToken(std::uint32_t token) noexcept {
    return token>=kOriginalHostTokenBase+0x1000u &&
           token<kOriginalHostTokenLimit && (token&3u)==0u;
}
inline constexpr Status PlanNativeWorldStartup() noexcept {
    // Requires original CWorld constructor 0x240CCE30 and engine
    // side-effectful vtables. No partial fake entity is published.
    return Status::RequiresOriginalAllocationAndConstructor;
}
constexpr Status PlanSourceCWorldClientClassCreator(
    HostOwnedWorldBinding binding) noexcept {
    // Source original 40B creator dereferences singleton BEFORE any
    // null test. Never emulate its null-dereference side effect.
    if(!binding.hasAuthenticatedWorldObject)
        return Status::RefusedUnboundWorldObject;
    if(!SafeSyntheticWorldToken(binding.syntheticWorldObjectToken))
        return Status::RefusedUnsafeSyntheticWorldToken;
    return Status::RequiresOriginalWorldNetworkInitialization;
}
constexpr Status PlanNativeWorldShutdown(
    HostOwnedWorldBinding binding) noexcept {
    if(!binding.hasAuthenticatedWorldObject)
        return Status::WorldGlobalWouldBeClearedWithoutDestroy;
    if(!SafeSyntheticWorldToken(binding.syntheticWorldObjectToken))
        return Status::RefusedUnsafeSyntheticWorldToken;
    return Status::RequiresOriginalVirtualDestroy;
}
inline constexpr bool CanPublishRetailCWorldCreator() noexcept {return false;}
static_assert(creators::SourceCreatorVAForOrdinal(kWorldClassOrdinal)==
              kWorldOriginalCreatorVA);
static_assert(special::SourcePlanForEntity(
                  kWorldClassOrdinal,kWorldOriginalCreatorVA).path==
              special::CreatorPath::ExistingGlobalSingleton);
static_assert(kNativeWorldSecondaryVirtual0VA==
              inlinevt::kSourceSecondaryVirtual0Target);
static_assert(kNativeWorldPrimaryVirtual94VA!=
              inlinevt::kSourcePrimaryVirtual94Target);
static_assert(kNativeSharedNetworkInitializerVA==
              inlinevt::kSourcePrimaryVirtual94Target);
static_assert(kNativeWorldInitClearsFieldOffset<
              kNativeWorldRequestBytes);
static_assert(!SafeSyntheticWorldToken(kWorldNativeSingletonGlobalVA));
static_assert(!CanPublishRetailCWorldCreator());
} // namespace sin_compat::original_client_world_singleton_lifecycle
