// Phase1728 inert prefix snapshot test. No native x86 execution, vtables,
// client factories, engine callbacks or object registration.
#include "sin_compat/original_client_world_ctor_prefix_phase1728.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace p=sin_compat::original_client_world_ctor_prefix_phase1728;
static std::uint32_t read32(const std::uint8_t* p) noexcept {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8u) |
           (static_cast<std::uint32_t>(p[2]) << 16u) |
           (static_cast<std::uint32_t>(p[3]) << 24u);
}
int main() {
    constexpr std::uint8_t poison=0x5Au;
    std::array<std::uint8_t,p::kOwnerBytes> original{};
    original.fill(poison);
    assert(p::ApplyInertSourceConstructorPrefix(nullptr,p::kOwnerBytes)==
           p::Status::RefusedNullBuffer);
    assert(p::ApplyInertSourceConstructorPrefix(original.data(),0)==
           p::Status::RefusedTruncatedAllocation);
    assert(p::ApplyInertSourceConstructorPrefix(original.data(),p::kOwnerBytes-1)==
           p::Status::RefusedTruncatedAllocation);
    for(const auto value:original) assert(value==poison);
    assert(p::ApplyInertSourceConstructorPrefix(original.data(),original.size())==
           p::Status::ProducedInertPrefixSnapshot);
    // Independent final state witnesses, not derived solely from the
    // implementation's kOrderedOriginalWrites table.
    assert(read32(&original[0x000u])==0x242895ECu);
    assert(read32(&original[0x004u])==0x24289584u);
    assert(read32(&original[0x008u])==0x24289550u);
    assert(read32(&original[0x00Cu])==0x24289538u);
    for(std::size_t i=0x10u;i<=0x20u;i+=4u)assert(read32(&original[i])==0u);
    assert(original[0x24u]==0u && read32(&original[0x28u])==0u);
    assert(read32(&original[0xA4u])==0u);
    assert(read32(&original[0xB4u])==0xFFFFFFFFu);
    for(std::size_t i=0xC8u;i<=0xD8u;i+=4u)assert(read32(&original[i])==0u);
    for(std::size_t i=0x154u;i<=0x164u;i+=4u)
        assert(read32(&original[i])==0xFFFFFFFFu);
    assert(read32(&original[0x14Cu])==0xFFFFFFFFu);
    std::array<bool,p::kOwnerBytes> written{};
    for(const auto& step:p::kOrderedOriginalWrites) {
        const auto count=static_cast<std::size_t>(step.size);
        for(std::size_t k=0;k<count;++k) written[step.nativeObjectOffset+k]=true;
    }
    std::size_t count=0u;
    for(std::size_t i=0;i<original.size();++i) {
        if(written[i]) ++count;
        else assert(original[i]==poison);
    }
    assert(count==93u); // Unique bytes (some initial vtable stores overwritten).
    assert(p::kNativeConstructorVA==0x240CCE30u);
    assert(p::kPrefixEndsBeforeCallVA==0x240CCEDEu);
    assert(p::kBlockedEmbeddedCtorVA==0x2411AEB0u);
    assert(!p::CanPublishRetailCWorld());
    std::puts("PASS Phase1728: 174 source bytes before first unported embedded "
              "constructor, 27 ordered writes incl +0xA4, inert host-only "
              "1200B CWorld snapshot, no runtime vtables/retail callbacks");
}
