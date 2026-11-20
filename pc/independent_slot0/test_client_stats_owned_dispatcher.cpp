// Phase 477: native original-free owned Stats user-message dispatch.
// Exercises recovered message IDs and reader-copy fanout semantics,
// NOT retail bf_read memory layout or actual Source engine delivery.
#include "client_stats_owned_dispatcher.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

namespace d=sin_independent::stats_owned_dispatch;
namespace b=sin_independent::stats_bitbuf;
namespace rt=sin_independent::stats_owned_messages;
static int Fail(int test) {
    std::fprintf(stderr,"owned message dispatch failure=%d\n",test);
    return test;
}
template<std::size_t N>
static void Put(std::array<std::uint8_t,N>& bytes,std::size_t bit,
                unsigned count,std::uint32_t value){
    for(unsigned i=0;i<count;++i){
        const auto where=bit+i;
        const auto mask=static_cast<std::uint8_t>(1u<<(where&7u));
        if((value>>i)&1u)bytes[where/8u]|=mask;
        else bytes[where/8u]&=static_cast<std::uint8_t>(~mask);
    }
}
template<std::size_t N>
static std::size_t MakeStatic(std::array<std::uint8_t,N>& bytes,
                              std::size_t start,unsigned id,std::uint32_t value){
    bytes.fill(0);
    Put(bytes,start,16,id);
    Put(bytes,start+16,32,value);
    return start+48;
}
template<std::size_t N>
static std::size_t MakeDynamic(std::array<std::uint8_t,N>& bytes,
                               std::size_t start,unsigned id,
                               const std::string& name,std::uint32_t value){
    bytes.fill(0);
    Put(bytes,start,16,id);
    std::size_t at=start+16;
    for(unsigned char c:name){Put(bytes,at,8,c);at+=8;}
    Put(bytes,at,8,0);
    at+=8;
    Put(bytes,at,32,value);
    return at+32;
}
struct Tap final {
    int expectedMessage=0;
    std::uint32_t expectedHeader=0;
    std::size_t expectedPosition=0;
    int ordinal=0;
    int* sequence=nullptr;
    unsigned calls=0;
    bool good=true;
};
static void Observe(void* opaque,int messageId,b::Cursor& stream) noexcept {
    auto& t=*static_cast<Tap*>(opaque);
    std::uint32_t header=0;
    const unsigned width=messageId==5?8u:16u;
    if(messageId!=t.expectedMessage ||
       stream.position()!=t.expectedPosition ||
       !stream.ReadUBits(width,header) ||
       header!=t.expectedHeader ||
       !t.sequence || (*t.sequence)++!=t.ordinal)t.good=false;
    // Deliberately exhaust this observer's private cursor. This must
    // never affect other hooks or the original caller's input reader.
    std::uint32_t discard=0;
    while(stream.remaining()>=32u)stream.ReadUBits(32u,discard);
    ++t.calls;
}
int main(){
    d::Dispatcher owned;
    std::array<std::uint8_t,400> bytes{};
    const auto end=MakeStatic(bytes,5,2,0x40000000u);
    b::Cursor input(bytes.data(),bytes.size(),end,5);
    if(owned.ready() ||
       owned.Dispatch(27,input).status!=d::Status::NotInitialized ||
       owned.AddObserver(27,&Observe,nullptr))return Fail(1);
    if(!owned.Initialize() || !owned.ready() ||
       !owned.stats().initialized() || !owned.stats().hooks().Complete())
        return Fail(2);
    if(owned.AddObserver(0,&Observe,nullptr) ||
       owned.AddObserver(27,nullptr,nullptr) ||
       owned.ObserverCount(0)!=0)return Fail(3);

    int sequence=0;
    Tap first{27,2,5,0,&sequence},second{27,2,5,1,&sequence};
    if(!owned.AddObserver(27,&Observe,&first) ||
       !owned.AddObserver(27,&Observe,&second) ||
       owned.ObserverCount(27)!=2)return Fail(4);
    const auto stat=owned.Dispatch(27,input);
    const auto* value=owned.stats().staticValues().at(2);
    if(!stat.delivered() || stat.stats!=rt::Result::StaticUpdated ||
       stat.observer_calls!=2 || input.position()!=5 ||
       sequence!=2 || first.calls!=1 || second.calls!=1 ||
       !first.good || !second.good || !value ||
       value->fieldBits(0)!=0x40000000u)return Fail(5);

    // A bad packet should not notify observers or mutate owned state.
    b::Cursor clipped(bytes.data(),bytes.size(),end-2,5);
    const auto truncated=owned.Dispatch(27,clipped);
    if(truncated.status!=d::Status::HeaderRefused ||
       clipped.position()!=5 || sequence!=2 ||
       value->fieldBits(0)!=0x40000000u)return Fail(6);
    b::Cursor bad(nullptr,1,8);
    if(owned.Dispatch(27,bad).status!=d::Status::InvalidCursor ||
       owned.Dispatch(3,input).status!=d::Status::UnboundMessage)
        return Fail(7);
    // Each additional dispatch has a fresh per-listener cursor.
    sequence=0;
    if(owned.Dispatch(27,input).observer_calls!=2 ||
       input.position()!=5 || sequence!=2 ||
       first.calls!=2 || second.calls!=2 ||
       !first.good || !second.good)return Fail(8);
    sequence=0;

    // DynamicStat ID55, its original name field and one 32-bit value.
    const auto dynEnd=MakeDynamic(bytes,7,55,"pistol",0x3f800000u);
    b::Cursor dyn(bytes.data(),bytes.size(),dynEnd,7);
    Tap dynamic{28,55,7,0,&sequence};
    if(!owned.AddObserver(28,&Observe,&dynamic))return Fail(9);
    const auto newStat=owned.Dispatch(28,dyn);
    const auto* pistol=owned.stats().dynamicValues().Find(55,"pistol",6);
    if(!newStat.delivered() || newStat.stats!=rt::Result::DynamicCreated ||
       newStat.observer_calls!=1 || !pistol ||
       pistol->fieldBits(0)!=0x3f800000u || dyn.position()!=7 ||
       dynamic.calls!=1 || !dynamic.good)return Fail(10);

    // ResetHUD 5 consumes exactly eight logical bits in the independently
    // owned cursor. Neither network-side meaning nor UI side effects
    // of the payload byte are invented.
    bytes.fill(0);
    Put(bytes,3,8,0xa5);
    b::Cursor reset(bytes.data(),bytes.size(),11,3);
    sequence=0;
    Tap resetTap{5,0xa5,3,0,&sequence};
    if(!owned.AddObserver(5,&Observe,&resetTap))return Fail(11);
    const auto resetReport=owned.Dispatch(5,reset);
    if(!resetReport.delivered() ||
       resetReport.stats!=rt::Result::ResetApplied ||
       resetReport.observer_calls!=1 || !resetTap.good ||
       reset.position()!=3 || owned.stats().resetCount()!=1 ||
       owned.stats().dynamicValues().size()!=0 ||
       owned.stats().staticValues().at(2)->fieldBits(0)!=0)
        return Fail(12);
    b::Cursor shortReset(bytes.data(),bytes.size(),10,3);
    if(owned.Dispatch(5,shortReset).status!=d::Status::HeaderRefused ||
       resetTap.calls!=1 || owned.stats().resetCount()!=1)return Fail(13);

    // Bounded independent hook vector: cannot overwrite the fifth slot.
    Tap filler{5,0xa5,3,0,nullptr};
    if(!owned.AddObserver(5,&Observe,&filler) ||
       !owned.AddObserver(5,&Observe,&filler) ||
       !owned.AddObserver(5,&Observe,&filler) ||
       owned.AddObserver(5,&Observe,&filler) ||
       owned.ObserverCount(5)!=4)return Fail(14);

    // Disable and Initialize are deterministic owned-session boundaries:
    // no listener retained and no stat values survive reinitialization.
    owned.Disable();
    if(owned.ready() || owned.ObserverCount(27)!=0 ||
       owned.Dispatch(27,input).status!=d::Status::NotInitialized)
        return Fail(15);
    if(!owned.Initialize() || owned.ObserverCount(5)!=0 ||
       owned.ObserverCount(27)!=0 ||
       owned.stats().resetCount()!=0 ||
       owned.stats().dynamicValues().size()!=0 ||
       owned.stats().staticValues().at(2)->fieldBits(0)!=0)
        return Fail(16);
    std::puts("PASS owned user-message dispatch: 27/28/5, copied independent cursors, bounded hook fanout, malformed rollback, ResetHUD, reinit");
    return 0;
}
