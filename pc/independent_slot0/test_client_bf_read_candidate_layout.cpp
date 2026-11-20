// Phase 478: Win32 native candidate bf_read snapshot validation.
// All data are synthetic owned bytes; NO original game bf_read pointer.
// The private retail SHA audit is separate from this fixture.
#include "client_bf_read_candidate_layout.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

namespace a=sin_independent::bf_read_candidate;
namespace b=sin_independent::stats_bitbuf;
namespace d=sin_independent::stats_owned_dispatch;
namespace rt=sin_independent::stats_owned_messages;
static int Fail(int code){
    std::fprintf(stderr,"bf_read candidate layout fixture failed=%d\n",code);
    return code;
}
template<std::size_t N>
static void Put(std::array<std::uint8_t,N>& storage,std::size_t at,
                unsigned bits,std::uint32_t raw){
    for(unsigned i=0;i<bits;++i){
        const std::size_t bit=at+i;
        const auto mask=static_cast<std::uint8_t>(1u<<(bit&7u));
        if((raw>>i)&1u)storage[bit>>3]|=mask;
        else storage[bit>>3]&=static_cast<std::uint8_t>(~mask);
    }
}
struct Observer final {
    unsigned calls=0;
    unsigned expected_id=0;
    std::size_t expected_cursor=0;
    bool good=true;
};
static void Observe(void* obj,int id,b::Cursor& cursor) noexcept {
    auto& state=*static_cast<Observer*>(obj);
    std::uint32_t got=0;
    const unsigned width=id==5?8u:16u;
    if(cursor.position()!=state.expected_cursor ||
       !cursor.ReadUBits(width,got) || got!=state.expected_id)
        state.good=false;
    ++state.calls;
}
int main(){
#if defined(_WIN32) && defined(_M_IX86)
    if(sizeof(a::Layout)!=24 || offsetof(a::Layout,data_bits)!=8 ||
       offsetof(a::Layout,overflow)!=16 ||
       offsetof(a::Layout,debug_name)!=20)return Fail(1);
#endif
    d::Dispatcher owned;
    if(!owned.Initialize() || !owned.ready())return Fail(2);
    alignas(4) std::array<std::uint8_t,128> storage{};
    const a::TrustedBytes trusted{storage.data(),storage.size()};
    constexpr std::size_t start=5;
    Put(storage,start,16,2);
    Put(storage,start+16,32,0x40000000u);
    a::Layout native{};
    native.data=storage.data();
    native.data_bytes=static_cast<std::int32_t>(storage.size());
    native.data_bits=static_cast<std::int32_t>(start+48);
    native.current_bit=static_cast<std::int32_t>(start);
    native.debug_name=reinterpret_cast<const char*>(
        static_cast<std::uintptr_t>(0x00000001u));
    native.assert_on_overflow=1; // never dereferenced
    Observer first{0,2,start},second{0,2,start};
    if(!owned.AddObserver(27,&Observe,&first) ||
       !owned.AddObserver(27,&Observe,&second))return Fail(3);
    b::Cursor extracted;
    if(a::Inspect(native,trusted,&extracted)!=a::Status::Ready ||
       extracted.position()!=start || extracted.bitLength()!=start+48 ||
       extracted.remaining()!=48)return Fail(4);
    const auto applied=a::DispatchOwnedCandidate(owned,27,native,trusted);
    const auto* stat=owned.stats().staticValues().at(2);
    if(!applied.delivered() ||
       applied.delivery.stats!=rt::Result::StaticUpdated ||
       applied.delivery.observer_calls!=2 ||
       !stat || stat->fieldBits(0)!=0x40000000u ||
       first.calls!=1 || second.calls!=1 || !first.good || !second.good ||
       native.current_bit!=start)return Fail(5);

    // Every refusal must reset the out Cursor (not leak stale contents),
    // leave original fake layout unchanged and publish no additional
    // stat updates or callbacks.
    a::Layout bad=native;
    b::Cursor output=extracted;
    bad.data=reinterpret_cast<const std::uint8_t*>(
        static_cast<std::uintptr_t>(0xdeadbeefu));
    if(a::Inspect(bad,trusted,&output)!=a::Status::UntrustedPointer ||
       output.valid() || first.calls!=1)return Fail(6);
    bad=native;bad.data_bytes=-1;
    if(a::Inspect(bad,trusted)!=a::Status::NegativeOrOversize)return Fail(7);
    bad=native;bad.data_bytes=129;
    if(a::Inspect(bad,trusted)!=a::Status::NegativeOrOversize)return Fail(8);
    bad=native;bad.data_bits=-1;
    if(a::Inspect(bad,trusted)!=a::Status::NegativeOrOversize)return Fail(9);
    bad=native;bad.data_bits=static_cast<std::int32_t>(storage.size()*8+1);
    if(a::Inspect(bad,trusted)!=a::Status::InvalidBitExtent)return Fail(10);
    bad=native;bad.current_bit=-1;
    if(a::Inspect(bad,trusted)!=a::Status::NegativeOrOversize)return Fail(11);
    bad=native;bad.current_bit=native.data_bits+1;
    if(a::Inspect(bad,trusted)!=a::Status::InvalidCursor)return Fail(12);
    bad=native;bad.overflow=1;
    if(a::Inspect(bad,trusted)!=a::Status::ReaderOverflowed)return Fail(13);
    bad=native;bad.overflow=0xff;
    if(a::Inspect(bad,trusted)!=a::Status::ReaderOverflowed)return Fail(14);
    bad=native;bad.data=storage.data()+1;
    bad.data_bytes=127;bad.data_bits=60;
    if(a::Inspect(bad,{storage.data()+1,127})!=
       a::Status::UnalignedStorage)return Fail(15);
    if(a::Inspect(native,{nullptr,1})!=a::Status::InvalidTrustedSpan ||
       a::Inspect(native,{storage.data(),b::kMaxMessageBytes+1})!=
           a::Status::InvalidTrustedSpan)return Fail(16);

    // A coherent but truncated original-shaped bitstream can be
    // snapshotted: its *value* decoder then rejects the incomplete
    // frame, without changing state or notifying either observer.
    bad=native;bad.data_bits=static_cast<std::int32_t>(start+48-1);
    const auto truncated=a::DispatchOwnedCandidate(owned,27,bad,trusted);
    if(truncated.snapshot!=a::Status::Ready ||
       truncated.delivery.status!=d::Status::HeaderRefused ||
       truncated.delivery.observer_calls!=0 ||
       stat->fieldBits(0)!=0x40000000u ||
       first.calls!=1 || second.calls!=1)return Fail(17);

    // Valid reader layout but unsupported message: registry must refuse.
    const auto unsupported=a::DispatchOwnedCandidate(owned,38,native,trusted);
    if(unsupported.snapshot!=a::Status::Ready ||
       unsupported.delivery.status!=d::Status::UnboundMessage)return Fail(18);

    // DynamicStat 55, a name and its float32 wire value.
    storage.fill(0);
    std::size_t at=3;
    Put(storage,at,16,55);at+=16;
    for(unsigned char c:std::string("pistol")){
        Put(storage,at,8,c);at+=8;
    }
    Put(storage,at,8,0);at+=8;
    Put(storage,at,32,0x3f800000u);at+=32;
    native.current_bit=3;
    native.data_bits=static_cast<std::int32_t>(at);
    const auto dyn=a::DispatchOwnedCandidate(owned,28,native,trusted);
    const auto* pistol=owned.stats().dynamicValues().Find(55,"pistol",6);
    if(!dyn.delivered() ||
       dyn.delivery.stats!=rt::Result::DynamicCreated ||
       !pistol || pistol->fieldBits(0)!=0x3f800000u)return Fail(19);

    // ResetHUD discards the named record, clears the stat and flags reset.
    storage.fill(0);
    Put(storage,7,8,0xa5);
    native.current_bit=7;native.data_bits=15;
    const auto reset=a::DispatchOwnedCandidate(owned,5,native,trusted);
    if(!reset.delivered() ||
       reset.delivery.stats!=rt::Result::ResetApplied ||
       !owned.stats().resetDirty() ||
       owned.stats().dynamicValues().size()!=0 ||
       owned.stats().staticValues().at(2)->fieldBits(0)!=0)
        return Fail(20);

    // Empty bitstream is a valid *snapshot*, not a valid Stats message.
    bad=native;bad.current_bit=0;bad.data_bits=0;bad.data_bytes=0;
    if(a::Inspect(bad,trusted)!=a::Status::Ready ||
       a::DispatchOwnedCandidate(owned,27,bad,trusted).delivered())
        return Fail(21);
    std::puts("PASS SHA-grounded x86 bf_read candidate: 24-byte layout, offsets, owned trust-boundary, malformed snapshots, 27/28/5 isolated dispatch");
    return 0;
}
