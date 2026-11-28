#include "sin_compat/original_client_world_pre_vector_stage_phase1730.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
namespace w=sin_compat::original_client_world_pre_vector_stage_phase1730;
static std::uint32_t read32(const std::uint8_t* p) noexcept {
    return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8u)|
           (std::uint32_t(p[2])<<16u)|(std::uint32_t(p[3])<<24u);
}
struct CallbackContext final {
    std::uint8_t* owner=nullptr;
    unsigned calls=0;
    std::array<std::uint32_t,2> identities{};
};
void record(std::uint32_t sourceVA,void* context) noexcept {
    auto& c=*static_cast<CallbackContext*>(context);
    assert(c.calls<2u);
    c.identities[c.calls]=sourceVA;
    if(c.calls==0u){
        assert(sourceVA==0x24271DA0u);
        assert(read32(c.owner+0x22Cu)==0x24278694u);
        assert(read32(c.owner+0x268u)==0u);
    }else{
        assert(sourceVA==0x24272B10u);
        assert(read32(c.owner+0x22Cu)==0x24278AA4u);
        assert(read32(c.owner+0x268u)==0x242893F0u);
    }
    ++c.calls;
}
int main() {
    std::array<std::uint8_t,w::kOwnerBytes> owner{};
    w::lazy::HostOwnedStatic first{},second{};
    CallbackContext context{owner.data()};
    const w::embedded::BssTripleBits bss{{0x3F800000u,0x80000000u,0x7FC0A5A5u}};
    const auto null=w::MakeInertPreVectorSnapshot(nullptr,owner.size(),
                                                    first,second,record,&context,bss);
    assert(null.status==w::Status::RefusedNull);
    const auto shortResult=w::MakeInertPreVectorSnapshot(owner.data(),owner.size()-1u,
                                                        first,second,record,&context,bss);
    assert(shortResult.status==w::Status::RefusedShort);
    const auto noRegistrar=w::MakeInertPreVectorSnapshot(owner.data(),owner.size(),
                                                         first,second,nullptr,&context,bss);
    assert(noRegistrar.status==w::Status::RefusedNoHostRegistrar);
    assert(context.calls==0u && owner[0u]==0u);
    owner[0x470u]=1u;
    const auto dirty=w::MakeInertPreVectorSnapshot(owner.data(),owner.size(),
                                                   first,second,record,&context,bss);
    assert(dirty.status==w::Status::RefusedNonzeroOriginalAllocation);
    owner[0x470u]=0u;
    const auto result=w::MakeInertPreVectorSnapshot(owner.data(),owner.size(),
                                                    first,second,record,&context,bss);
    assert(result.status==w::Status::ProducedInertPreVectorSnapshot);
    assert(result.nextBlockedSourceCall==0x240CCFBEu);
    assert(result.nextBlockedTarget==0x24061220u);
    assert(context.calls==2u);
    assert(context.identities[0]==0x24271DA0u && context.identities[1]==0x24272B10u);
    assert((first.nativeModeledGuard&1u) && (second.nativeModeledGuard&1u));
    assert(read32(owner.data()+0x16Cu)==0x24288E3Cu);
    assert(read32(owner.data()+0x16Cu+0x4Cu)==bss.words[0]);
    assert(read32(owner.data()+0x16Cu+0x58u)==bss.words[0]);
    assert(read32(owner.data()+0x1D8u)==0xFFFFFFFFu);
    assert(read32(owner.data()+0x1E0u)==0xFFFFFFFFu);
    assert(read32(owner.data()+0x22Cu)==0x24278AA4u);
    assert(read32(owner.data()+0x230u)==0u);
    assert(read32(owner.data()+0x244u)==1u);
    assert(read32(owner.data()+0x250u)==1u);
    assert(read32(owner.data()+0x258u)==0x24289524u);
    assert(read32(owner.data()+0x268u)==0x24289418u);
    assert(read32(owner.data()+0x280u)==1u);
    assert(read32(owner.data()+0x28Cu)==1u);
    assert(read32(owner.data()+0x294u)==0x24289510u);
    for(std::size_t i=0x460u;i<=0x468u;i+=4u)
        assert(read32(owner.data()+i)==0xFFFFFFFFu);
    // The next original call allocates; the source vector remains untouched.
    assert(read32(owner.data()+0x10u)==0u);
    assert(read32(owner.data()+0x14u)==0u);
    assert(read32(owner.data()+0x1Cu)==0u);
    assert(read32(owner.data()+0x220u)==0u);
    assert(read32(owner.data()+0x25Cu)==0u);
    assert(!w::CanPublishRetailWorld());
    std::puts("PASS Phase1730: 398-byte source original CWorld prefix through "
              "two lazy statics and zeroed interpolator; stops before "
              "first 12B vector insertion/allocation, no retail callbacks");
}
