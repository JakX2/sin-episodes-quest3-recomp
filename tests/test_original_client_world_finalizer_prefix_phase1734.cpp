#include "sin_compat/original_client_world_finalizer_prefix_phase1734.hpp"
#include "sin_compat/original_client_world_pre_vector_stage_phase1730.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
namespace f=sin_compat::original_client_world_finalizer_prefix_phase1734;
namespace pre=sin_compat::original_client_world_pre_vector_stage_phase1730;
namespace v=f::v;
namespace vec=f::vec;
struct Context {
    unsigned cleanupCalls=0u;
    unsigned allocationCalls=0u;
};
void record(std::uint32_t sourceVA,void* context) noexcept {
    auto& c=*static_cast<Context*>(context);
    assert(sourceVA==0x24271DA0u || sourceVA==0x24272B10u);
    ++c.cleanupCalls;
}
bool allocate(std::size_t amount,void* context) noexcept {
    auto& c=*static_cast<Context*>(context);
    assert(amount==36u);
    ++c.allocationCalls;
    return true;
}
int main() {
    std::array<std::uint8_t,f::kOwnerBytes> owner{};
    pre::lazy::HostOwnedStatic fst{},snd{};
    vec::HostOwnedSidecar sidecar{};
    Context context{};
    constexpr std::uint32_t ownerToken=0x10002000u;
    constexpr std::uint32_t vectorToken=0x10010000u;
    // First original BSS vector may contain signed negative zero; the
    // original first CWorld setter treats it equal to zero and never copies.
    const pre::embedded::BssTripleBits baseBss{{0x80000000u,0u,0x80000000u}};
    const v::Vec3Bits firstSource{{0x80000000u,0u,0x80000000u}};
    const v::Vec3Bits secondSource{{0u,0x80000000u,0u}};
    assert(pre::MakeInertPreVectorSnapshot(owner.data(),owner.size(),
        fst,snd,record,&context,baseBss).status==
        pre::Status::ProducedInertPreVectorSnapshot);
    assert(vec::MakeOriginalTwoInsertions(owner.data(),owner.size(),sidecar,
        ownerToken,vectorToken,allocate,&context).status==
        vec::Status::ProducedTwoInsertSnapshot);
    assert(f::pre::ApplyInertPreFinalizerTail(owner.data(),owner.size(),
        sidecar,vectorToken).status==
        f::pre::Status::ProducedInertPreFinalizerSnapshot);
    assert(context.cleanupCalls==2u && context.allocationCalls==1u);
    const auto copy=owner;
    assert(f::ExtendZeroedCWorldFinalizerPrefix(nullptr,owner.size(),sidecar,
        ownerToken,firstSource,secondSource).status==f::Status::RefusedNull);
    assert(f::ExtendZeroedCWorldFinalizerPrefix(owner.data(),owner.size()-1u,
        sidecar,ownerToken,firstSource,secondSource).status==f::Status::RefusedShort);
    assert(f::ExtendZeroedCWorldFinalizerPrefix(owner.data(),owner.size(),sidecar,
        0x240C9F40u,firstSource,secondSource).status==
        f::Status::RefusedInvalidOwnerToken);
    const v::Vec3Bits changed{{0x3f800000u,0u,0u}};
    assert(f::ExtendZeroedCWorldFinalizerPrefix(owner.data(),owner.size(),sidecar,
        ownerToken,changed,secondSource).status==
        f::Status::RefusedRequiresOriginalDirtyPropagation);
    const v::Vec3Bits nan{{0x7fc01234u,0u,0u}};
    assert(f::ExtendZeroedCWorldFinalizerPrefix(owner.data(),owner.size(),sidecar,
        ownerToken,nan,secondSource).status==
        f::Status::RefusedRequiresOriginalDirtyPropagation);
    assert(owner==copy); // all refusals must occur BEFORE source writes
    const auto result=f::ExtendZeroedCWorldFinalizerPrefix(owner.data(),owner.size(),
        sidecar,ownerToken,firstSource,secondSource);
    assert(result.status==f::Status::ProducedInertPrefixBeforeEngineGlobalRead);
    assert(result.nextBlockedInstructionVA==0x240CA0A5u &&
           result.nextBlockedEngineGlobalVA==0x242FA458u);
    assert(v::Load32LE(owner.data()+0x16Cu+4u)==ownerToken);
    assert(v::Load32LE(owner.data()+0x16Cu)==0x24288E3Cu);
    for(std::size_t r=0u;r<2u;++r)
        for(std::size_t i=0u;i<3u;++i)
            assert(v::Load32LE(owner.data()+0x16Cu+0x4Cu+12u*r+4u*i)==
                   firstSource.bits[i]);
    // x87 compares signed zero equal, preserving the original +0 DWORDs.
    for(const auto channel:{v::Channel::First,v::Channel::Second})
        for(const auto bits:v::ReadExisting(owner.data(),channel).bits)assert(bits==0u);
    assert(owner[0x119u]==1u);
    assert(v::Load32LE(owner.data()+0xB4u)==0xFFFFFFFFu);
    assert(owner[0x134u]==0xFFu && owner[0x135u]==0xFFu);
    assert(owner[0x136u]==0xFFu && owner[0x137u]==0xFFu);
    assert(owner[0xB8u]==0xFFu && owner[0xB9u]==0xFFu);
    assert(v::Load32LE(owner.data()+0x13Cu)==0x0000FFFFu);
    assert(v::Load32LE(owner.data()+0x478u)==0xFFFFFFFFu);
    assert(v::Load32LE(owner.data()+0x50u)==0xFFFFFFFFu);
    assert(v::Load32LE(owner.data()+0x5Cu)==0xFFFFFFFFu);
    assert(v::Load32LE(owner.data()+0x1D8u)==0xFFFFFFFFu);
    assert(v::Load32LE(owner.data()+0x2E0u)==0u);
    assert((owner[0x143u]&1u)==0u);
    assert(v::Load32LE(owner.data()+0x10u)==vectorToken);
    assert(v::Load32LE(owner.data()+0x1Cu)==2u);
    assert(sidecar.words[0][1]==ownerToken+0x220u &&
           sidecar.words[1][1]==ownerToken+0x25Cu);
    const auto after=owner;
    assert(f::ExtendZeroedCWorldFinalizerPrefix(owner.data(),owner.size(),sidecar,
        ownerToken,firstSource,secondSource).status==f::Status::RefusedAlreadyApplied);
    assert(owner==after);
    assert(!f::CanPublishRetailWorld());
    std::puts("PASS Phase1734 zeroed CWorld finalizer 357B source path: "
              "two interpolator resets, both +E8/+F0 ordered-equal setters, "
              "zero-state virtual/leaf paths, 0x240CA0A5 engine global "
              "read blocked; no engine or proprietary x86 execution");
}
