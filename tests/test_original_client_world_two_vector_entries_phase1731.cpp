#include "sin_compat/original_client_world_two_vector_entries_phase1731.hpp"
#include "sin_compat/original_client_world_pre_vector_stage_phase1730.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
namespace v=sin_compat::original_client_world_two_vector_entries_phase1731;
namespace pre=sin_compat::original_client_world_pre_vector_stage_phase1730;
struct State {
    unsigned allocations=0;
    unsigned cleanupRegistrations=0;
    bool allowAllocation=true;
    v::HostOwnedSidecar* backing=nullptr;
    std::uint8_t* owner=nullptr;
};
void recordCleanup(std::uint32_t sourceVA,void* context) noexcept {
    auto& s=*static_cast<State*>(context);
    assert(sourceVA==0x24271DA0u || sourceVA==0x24272B10u);
    ++s.cleanupRegistrations;
}
bool allocate(std::size_t bytes,void* context) noexcept {
    auto& s=*static_cast<State*>(context);
    assert(bytes==36u);
    assert(s.backing && s.owner);
    assert(s.backing->state==v::HostOwnedSidecar::State::Allocating);
    assert(v::LoadWordLE(s.owner+0x10u)==0u);
    ++s.allocations;
    return s.allowAllocation;
}
int main() {
    std::array<std::uint8_t,pre::kOwnerBytes> owner{};
    pre::lazy::HostOwnedStatic f{},s{};
    v::HostOwnedSidecar storage{};
    State state{};
    state.backing=&storage;state.owner=owner.data();
    const pre::embedded::BssTripleBits bss{{0x3F800000u,0x80000000u,0x7FC0A5A5u}};
    const auto prepared=pre::MakeInertPreVectorSnapshot(owner.data(),owner.size(),
                                                        f,s,recordCleanup,&state,bss);
    assert(prepared.status==pre::Status::ProducedInertPreVectorSnapshot);
    assert(state.cleanupRegistrations==2u);
    const auto prefixBytes=owner;
    constexpr std::uint32_t ownerToken=0x10002000u;
    constexpr std::uint32_t storageToken=0x10010000u;
    const auto call=[&](std::uint32_t objectToken,std::uint32_t allocToken,
                        v::HostAllocationRequest cb) {
        return v::MakeOriginalTwoInsertions(owner.data(),owner.size(),
                                            storage,objectToken,allocToken,cb,&state);
    };
    assert(v::MakeOriginalTwoInsertions(nullptr,owner.size(),storage,
                                       ownerToken,storageToken,allocate,&state).status==
           v::Status::RefusedNull);
    assert(v::MakeOriginalTwoInsertions(owner.data(),owner.size()-1u,storage,
                                       ownerToken,storageToken,allocate,&state).status==
           v::Status::RefusedShort);
    assert(call(0x240CCE30u,storageToken,allocate).status==
           v::Status::RefusedInvalidOwnerToken);
    assert(call(ownerToken,0x24061220u,allocate).status==
           v::Status::RefusedInvalidAllocationToken);
    assert(call(ownerToken,ownerToken+0x100u,allocate).status==
           v::Status::RefusedOverlappingTokenRanges);
    assert(call(ownerToken,storageToken,nullptr).status==
           v::Status::RefusedNoHostAllocator);
    assert(state.allocations==0u);
    state.allowAllocation=false;
    const auto denied=call(ownerToken,storageToken,allocate);
    assert(denied.status==v::Status::RefusedHostAllocation);
    assert(state.allocations==1u);
    assert(storage.state==v::HostOwnedSidecar::State::Empty);
    assert(owner==prefixBytes);
    state.allowAllocation=true;
    const auto ok=call(ownerToken,storageToken,allocate);
    assert(ok.status==v::Status::ProducedTwoInsertSnapshot);
    assert(ok.firstSourceInsertionIndex==0u && ok.secondSourceInsertionIndex==1u);
    assert(ok.sourceCapacity==3u && ok.sourceAllocationBytes==36u);
    assert(state.allocations==2u && state.cleanupRegistrations==2u);
    assert(storage.state==v::HostOwnedSidecar::State::Initialized);
    assert(storage.words[0]==(std::array<std::uint32_t,3>{{2u,ownerToken+0x220u,ownerToken+0x22Cu}}));
    assert(storage.words[1]==(std::array<std::uint32_t,3>{{2u,ownerToken+0x25Cu,ownerToken+0x268u}}));
    assert(v::LoadWordLE(owner.data()+0x10u)==storageToken);
    assert(v::LoadWordLE(owner.data()+0x14u)==3u);
    assert(v::LoadWordLE(owner.data()+0x18u)==0u);
    assert(v::LoadWordLE(owner.data()+0x1Cu)==2u);
    assert(v::LoadWordLE(owner.data()+0x20u)==storageToken);
    for(std::size_t i=0u;i<owner.size();++i) {
        if(i>=0x10u && i<0x24u)continue;
        assert(owner[i]==prefixBytes[i]);
    }
    const auto after=owner;
    assert(call(ownerToken,storageToken,allocate).status==
           v::Status::RefusedUninitializedVectorExpected);
    assert(owner==after && state.allocations==2u);
    assert(!v::CanPublishRetailWorld());
    std::puts("PASS Phase1731: original 121B 12-byte vector two append slots, "
              "one malloc(36) request, capacity 3, size 2, source-token pairs, "
              "no memmove, safe allocation denial and zero pointer execution");
}
