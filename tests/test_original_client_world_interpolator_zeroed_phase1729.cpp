#include "sin_compat/original_client_world_interpolator_zeroed_phase1729.hpp"
#include "sin_compat/original_client_world_ctor_prefix_phase1728.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
namespace interp=sin_compat::original_client_world_interpolator_zeroed_phase1729;
namespace prefix=sin_compat::original_client_world_ctor_prefix_phase1728;
static std::uint32_t read32(const std::uint8_t* p) noexcept {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8u) |
           (static_cast<std::uint32_t>(p[2]) << 16u) |
           (static_cast<std::uint32_t>(p[3]) << 24u);
}
int main() {
    constexpr std::size_t n=interp::kOwnerBytes;
    std::array<std::uint8_t,n> owner{};
    const interp::BssTripleBits globals{{0x3F800000u,0xBF800000u,0x7FC01234u}};
    assert(interp::InitializeZeroedSourceEmbeddedSubobject(nullptr,n,globals).status==
           interp::Status::RefusedNull);
    assert(interp::InitializeZeroedSourceEmbeddedSubobject(owner.data(),n-1,globals).status==
           interp::Status::RefusedShortAllocation);
    assert(prefix::ApplyInertSourceConstructorPrefix(owner.data(),owner.size())==
           prefix::Status::ProducedInertPrefixSnapshot);
    const auto copyBefore=owner;
    const auto init=interp::InitializeZeroedSourceEmbeddedSubobject(owner.data(),n,globals);
    assert(init.status==interp::Status::InitializedInertZeroedSubobject);
    assert(init.sourceSubobjectSnapshot==owner.data()+0x16Cu);
    auto* sub=init.sourceSubobjectSnapshot;
    assert(read32(sub)==0x2429806Cu);
    assert(sub[0x26u]==0xFFu && sub[0x27u]==0xFFu);
    for(std::size_t r=0u;r<2u;++r)
        for(std::size_t i=0u;i<3u;++i)
            assert(read32(sub+0x4Cu+r*12u+i*4u)==globals.words[i]);
    // No original host pointers are called. Only +0 vtable label,
    // +0x26..27 sentinel and +0x4C..+0x63 BSS-vector copies are changed.
    std::size_t newBytes=0;
    for(std::size_t i=0;i<n;++i) {
        bool possibleChange=(i>=0x16Cu && i<0x170u) ||
            (i>=0x192u && i<0x194u) ||
            (i>=0x1B8u && i<0x1D0u);
        if(!possibleChange) assert(owner[i]==copyBefore[i]);
        if(owner[i]!=copyBefore[i])++newBytes;
    }
    assert(newBytes>=14u && newBytes<=30u);
    const auto after=owner;
    const auto refused=interp::InitializeZeroedSourceEmbeddedSubobject(owner.data(),n,globals);
    assert(refused.status==interp::Status::RefusedNotZeroed);
    assert(refused.sourceSubobjectSnapshot==nullptr);
    assert(owner==after);
    // Positive test for a different BSS snapshot, without float
    // conversion or NaN canonicalization of the source payload bits.
    std::array<std::uint8_t,n> independent{};
    const interp::BssTripleBits zeroOrigin{};
    const auto second=interp::InitializeZeroedSourceEmbeddedSubobject(
        independent.data(),n,zeroOrigin);
    assert(second.status==interp::Status::InitializedInertZeroedSubobject);
    for(std::size_t i=0x4Cu;i<0x64u;++i) assert(second.sourceSubobjectSnapshot[i]==0u);
    assert(!interp::CanPublishRetailWorld());
    std::puts("PASS Phase1729 source 26B embedded ctor + 210B virtual helper "
              "zeroed path, one-byte original virtual RET, copied exact BSS "
              "origin bits, inert x86 owner and no native engine callbacks");
}
