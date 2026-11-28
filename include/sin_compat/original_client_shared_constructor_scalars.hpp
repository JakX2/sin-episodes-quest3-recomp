#pragma once
// Phase1726: portable scalar-state projection of original constructor
// 0x24092BD0 (206 bytes), after its base constructor and excluding vtables.
// Whole source SHA256: 28e8f086a532a606b78304522eaaeb571f63922feaee80aaaeeacb7e8a03659a
// Original 5-byte helper 0x241E6ED0 returns AX=0xffff, irrespective of upper EAX.
// This is not a C++ object constructor, a native vtable, or a retail callback.
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "sin_compat/original_client_canonical_creator_bodies.hpp"

namespace sin_compat::original_client_shared_constructor_scalars {
namespace canonical=sin_compat::original_client_canonical_creator_bodies;
namespace creators=sin_compat::original_client_creator_source_map;
inline constexpr std::uint32_t kConstructorVA=0x24092BD0u;
inline constexpr std::uint32_t kBaseConstructorVA=0x240BD7C0u;
inline constexpr std::uint32_t kBaseOfBaseConstructorVA=0x240CCE30u;
inline constexpr std::uint32_t kWordHelperVA=0x241E6ED0u;
inline constexpr std::size_t kMinimumScalarExtent=0x820u;
inline constexpr std::size_t kDirectCanonicalUses=34u;
struct ScalarWrite final {
    std::size_t offset;
    std::uint8_t width;
    std::uint32_t value;
};
// Conditional same-value stores in the source are represented by their
// final byte values; no original address or vtable label is written.
inline constexpr std::array<ScalarWrite,16> kScalarWrites{{
    {0x788u,4u,0xffffffffu}, {0x80cu,4u,0xffffffffu},
    {0x7ecu,4u,0x42820000u}, {0x7f0u,4u,0x42820000u},
    {0x7f4u,4u,0x44800000u}, {0x7f8u,4u,0x44800000u},
    {0x7fcu,1u,0u}, {0x78cu,4u,0u}, {0x81cu,4u,0u},
    {0x7d0u,4u,0u}, {0x7e0u,4u,0xffffffffu},
    {0x7e4u,4u,0xffffffffu}, {0x7d8u,4u,0xffffffffu},
    {0x7dcu,4u,0xffffffffu}, {0x79cu,4u,0u},
    {0x810u,2u,0xffffu}
}};
constexpr bool ScalarLayoutValid() noexcept {
    for(std::size_t i=0;i<kScalarWrites.size();++i) {
        const auto w=kScalarWrites[i];
        if((w.width!=1u && w.width!=2u && w.width!=4u) ||
           w.offset<16u || w.offset+w.width>kMinimumScalarExtent)
            return false;
        for(std::size_t j=0;j<i;++j) {
            const auto v=kScalarWrites[j];
            if(w.offset<v.offset+v.width && v.offset<w.offset+w.width)
                return false;
        }
    }
    return true;
}
constexpr std::size_t DirectCanonicalUses() noexcept {
    std::size_t uses=0;
    for(const auto& item:canonical::k60ByteConstructors)
        if(item.originalConstructorVA==kConstructorVA)++uses;
    for(const auto& item:canonical::k86ByteConstructors)
        if(item.originalConstructorVA==kConstructorVA)++uses;
    return uses;
}
enum class Status : std::uint8_t {
    AppliedSyntheticScalarState, UnrecognizedSource, WrongStorageSize
};
inline Status ApplyToSyntheticStorage(std::size_t ordinal,
    std::uint32_t creatorVA,std::vector<std::uint8_t>& storage) noexcept {
    const auto plan=canonical::SourceCanonicalBodyPlan(ordinal,creatorVA);
    if(plan.family==canonical::Family::Unclassified ||
       plan.originalConstructorVA!=kConstructorVA)
        return Status::UnrecognizedSource;
    if(storage.size()!=plan.originalRequestedAllocationBytes ||
       storage.size()<kMinimumScalarExtent)
        return Status::WrongStorageSize;
    for(const auto w:kScalarWrites)
        for(std::size_t i=0;i<w.width;++i)
            storage[w.offset+i]=static_cast<std::uint8_t>(w.value>>(8u*i));
    return Status::AppliedSyntheticScalarState;
}
constexpr bool CanPublishRetailCreatorPointers() noexcept {return false;}
static_assert(ScalarLayoutValid());
static_assert(DirectCanonicalUses()==kDirectCanonicalUses);
static_assert(!CanPublishRetailCreatorPointers());
} // namespace sin_compat::original_client_shared_constructor_scalars
