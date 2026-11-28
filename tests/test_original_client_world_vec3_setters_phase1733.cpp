#include "sin_compat/original_client_world_vec3_setters_phase1733.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
namespace v=sin_compat::original_client_world_vec3_setters_phase1733;
struct Context {
    unsigned calls=0u;
    std::uint8_t lastFlag=0u;
    bool allow=true;
    const std::uint8_t* owner=nullptr;
    v::Channel channel=v::Channel::First;
    v::Vec3Bits preCallOld{};
};
bool record(std::uint8_t sourceFlag,void* context) noexcept {
    auto& c=*static_cast<Context*>(context);
    assert(sourceFlag==v::OriginalDirtyFlag(c.channel));
    assert(v::ReadExisting(c.owner,c.channel).bits==c.preCallOld.bits);
    ++c.calls;c.lastFlag=sourceFlag;
    return c.allow;
}
int main() {
    static_assert(!v::CanPublishRetailWorld());
    assert(v::X87OrderedEqualBits(0u,0x80000000u));
    assert(v::X87OrderedEqualBits(0x3f800000u,0x3f800000u));
    assert(v::X87OrderedEqualBits(0x7f800000u,0x7f800000u));
    assert(v::X87OrderedEqualBits(0xff800000u,0xff800000u));
    assert(!v::X87OrderedEqualBits(0x7f800000u,0xff800000u));
    assert(!v::X87OrderedEqualBits(0x3f800000u,0xbf800000u));
    assert(!v::X87OrderedEqualBits(0x7fc01234u,0x7fc01234u));
    assert(!v::X87OrderedEqualBits(0x7fa00001u,0x7fa00001u));
    assert(!v::X87OrderedEqualBits(0x7fc00000u,0u));
    std::array<std::uint8_t,v::kOwnerBytes> owner{};
    const v::Vec3Bits initial{{0u,0x80000000u,0x7f800000u}};
    for(unsigned i=0;i<3;++i) v::Store32LE(owner.data()+0x220u+i*4u,initial.bits[i]);
    auto copy=owner;
    const auto equal=v::ApplyInertSourceVec3Setter(owner.data(),owner.size(),
        v::Channel::First,v::Vec3Bits{{0x80000000u,0u,0x7f800000u}});
    assert(equal.status==v::Status::OriginalOrderedEqualNoWrite);
    assert(equal.sourceDirtyFlag==0u);
    assert(owner==copy); // signed zeros compared equal but NOT normalized
    const v::Vec3Bits toChange{{0x3f800000u,0xbf800000u,0x7fc01234u}};
    auto unavailable=v::ApplyInertSourceVec3Setter(owner.data(),owner.size(),
        v::Channel::First,toChange);
    assert(unavailable.status==v::Status::RefusedDirtyPropagationNotPorted);
    assert(unavailable.sourceDirtyFlag==1u && owner==copy);
    Context context{};
    context.owner=owner.data();
    context.channel=v::Channel::First;
    context.preCallOld=initial;
    context.allow=false;
    auto refused=v::ApplyInertSourceVec3Setter(owner.data(),owner.size(),
        v::Channel::First,toChange,record,&context);
    assert(refused.status==v::Status::RefusedDirtyIntent);
    assert(context.calls==1u && owner==copy);
    context.allow=true;
    auto applied=v::ApplyInertSourceVec3Setter(owner.data(),owner.size(),
        v::Channel::First,toChange,record,&context);
    assert(applied.status==v::Status::ChangedInertSnapshotAndRecordedDirtyIntent);
    assert(applied.sourceDirtyFlag==1u && context.calls==2u);
    assert(v::ReadExisting(owner.data(),v::Channel::First).bits==toChange.bits);
    for(unsigned i=0;i<owner.size();++i)
        if(i<0x220u || i>=0x22cu)assert(owner[i]==copy[i]);
    // qNaN with identical payload is UNORDERED, so even the exact same
    // three DWORDs would request native invalidation once more.
    context.preCallOld=toChange;
    assert(v::ApplyInertSourceVec3Setter(owner.data(),owner.size(),
        v::Channel::First,toChange).status==
        v::Status::RefusedDirtyPropagationNotPorted);
    // The second original source method uses dirty-bit 2 and offset +0x25C.
    context.channel=v::Channel::Second;
    context.preCallOld={};
    v::Vec3Bits second{{0x80000000u,0x3f000000u,0x40000000u}};
    const auto secondApplied=v::ApplyInertSourceVec3Setter(owner.data(),owner.size(),
        v::Channel::Second,second,record,&context);
    assert(secondApplied.status==v::Status::ChangedInertSnapshotAndRecordedDirtyIntent);
    assert(context.lastFlag==2u && context.calls==3u);
    assert(v::ReadExisting(owner.data(),v::Channel::Second).bits==second.bits);
    assert(v::ApplyInertSourceVec3Setter(nullptr,owner.size(),v::Channel::First,
        initial).status==v::Status::RefusedNull);
    auto shortResult=v::ApplyInertSourceVec3Setter(owner.data(),owner.size()-1u,
        v::Channel::Second,second);
    assert(shortResult.status==v::Status::RefusedShort);
    std::puts("PASS Phase1733 dual 101B source CWorld x87 Vec3 setters: "
              "signed zero, NaN, infinity, dirty flags 1/2, denied/recorded "
              "host-only dirty intent, bit-preserved writes, no original engine call");
}
