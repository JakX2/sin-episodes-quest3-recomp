#include "sin_compat/original_client_world_pre_finalizer_phase1732.hpp"
#include "sin_compat/original_client_world_pre_vector_stage_phase1730.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
namespace tail=sin_compat::original_client_world_pre_finalizer_phase1732;
namespace pre=sin_compat::original_client_world_pre_vector_stage_phase1730;
namespace vec=tail::vector;
struct Context {
    unsigned cleanupCalls=0u;
    unsigned allocationCalls=0u;
};
void cleanup(std::uint32_t va,void* context) noexcept {
    auto& c=*static_cast<Context*>(context);
    assert(va==0x24271DA0u || va==0x24272B10u);
    ++c.cleanupCalls;
}
bool allocate(std::size_t n,void* context) noexcept {
    auto& c=*static_cast<Context*>(context);
    assert(n==36u);
    ++c.allocationCalls;
    return true;
}
int main() {
    std::array<std::uint8_t,tail::kOwnerBytes> owner{};
    pre::lazy::HostOwnedStatic f{},s{};
    vec::HostOwnedSidecar sidecar{};
    Context context{};
    const pre::embedded::BssTripleBits bss{{0x3f800000u,0x7fc0dead,0u}};
    const auto prefix=pre::MakeInertPreVectorSnapshot(owner.data(),owner.size(),
                                                      f,s,cleanup,&context,bss);
    assert(prefix.status==pre::Status::ProducedInertPreVectorSnapshot);
    assert(context.cleanupCalls==2u);
    constexpr std::uint32_t ownerToken=0x10002000u;
    constexpr std::uint32_t vectorToken=0x10010000u;
    const auto vectorResult=vec::MakeOriginalTwoInsertions(owner.data(),owner.size(),
        sidecar,ownerToken,vectorToken,allocate,&context);
    assert(vectorResult.status==vec::Status::ProducedTwoInsertSnapshot);
    assert(context.allocationCalls==1u);
    const auto before=owner;
    assert(tail::ApplyInertPreFinalizerTail(nullptr,owner.size(),sidecar,vectorToken).status==
           tail::Status::RefusedNull);
    assert(tail::ApplyInertPreFinalizerTail(owner.data(),owner.size()-1u,sidecar,
                                           vectorToken).status==tail::Status::RefusedShort);
    assert(tail::ApplyInertPreFinalizerTail(owner.data(),owner.size(),sidecar,
                                           vectorToken+0x100u).status==
           tail::Status::RefusedUnexpectedVectorMetadata);
    vec::HostOwnedSidecar uninitialized{};
    assert(tail::ApplyInertPreFinalizerTail(owner.data(),owner.size(),uninitialized,
                                           vectorToken).status==
           tail::Status::RefusedVectorNotInitialized);
    assert(owner==before);
    const auto r=tail::ApplyInertPreFinalizerTail(owner.data(),owner.size(),
                                                  sidecar,vectorToken);
    assert(r.status==tail::Status::ProducedInertPreFinalizerSnapshot);
    assert(r.nextBlockedSourceCall==0x240CD05Eu &&
           r.nextBlockedTarget==0x240C9F40u);
    for(const auto& step:tail::kOriginalWrites) {
        for(std::size_t i=0;i<static_cast<std::size_t>(step.size);++i)
            assert(owner[step.offset+i]==std::uint8_t((step.value>>(8u*i))&0xFFu));
    }
    std::array<bool,tail::kOwnerBytes> changed{};
    for(const auto& step:tail::kOriginalWrites)
        for(std::size_t i=0;i<static_cast<std::size_t>(step.size);++i)
            changed[step.offset+i]=true;
    for(std::size_t i=0;i<owner.size();++i)
        if(!changed[i])assert(owner[i]==before[i]);
    assert(vec::LoadWordLE(owner.data()+0x10u)==vectorToken);
    assert(vec::LoadWordLE(owner.data()+0x1Cu)==2u);
    assert(sidecar.words[0][0]==2u && sidecar.words[1][0]==2u);
    const auto after=owner;
    assert(tail::ApplyInertPreFinalizerTail(owner.data(),owner.size(),sidecar,
                                           vectorToken).status==
           tail::Status::RefusedAlreadyApplied);
    assert(owner==after);
    assert(!tail::CanPublishRetailWorld());
    std::puts("PASS Phase1732 CWorld zeroed constructor inert source "
              "558-byte prefix: two 12B vector entries and 16 final field "
              "writes, stops before native engine/virtual finalizer");
}
