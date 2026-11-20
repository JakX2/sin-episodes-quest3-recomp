// Native MSVC 32-bit original-free simulation of the recovered SiN
// StaticStat(27), DynamicStat(28) and ResetHUD(5) message sequence.
// All data and state independently owned; no game-owned bf_read, VGUI,
// user-message dispatch or original 2006 engine objects are accessed.
#include "client_stats_owned_message_runtime.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace rt=sin_independent::stats_owned_messages;
namespace b=sin_independent::stats_bitbuf;
namespace w=sin_independent::stats_wire_values;
static int Fail(int code) {
    std::fprintf(stderr,"owned Stats reset/runtime failure=%d\n",code);
    return code;
}
template<std::size_t N>
void Put(std::array<std::uint8_t,N>& bytes,std::size_t at,
         unsigned count,std::uint32_t bits) {
    for(unsigned i=0;i<count;++i){
        const auto pos=at+i;
        const std::uint8_t mask=static_cast<std::uint8_t>(1u<<(pos&7u));
        if((bits>>i)&1u)bytes[pos>>3]|=mask;
        else bytes[pos>>3]&=static_cast<std::uint8_t>(~mask);
    }
}
template<std::size_t N>
std::size_t MakeStatic(std::array<std::uint8_t,N>& bytes,
                       std::size_t start,unsigned id,
                       std::uint32_t raw) {
    bytes.fill(0);
    Put(bytes,start,16,id);
    std::size_t end=start+16;
    const auto* rule=w::RuleForType(static_cast<unsigned>(
        sin_independent::stats_original_types::FactoryTypeForStatId(id)));
    if(!rule)return end;
    for(unsigned i=0;i<rule->fields;++i){
        Put(bytes,end,32,i?0x40000000u:raw);
        end+=32;
    }
    return end;
}
template<std::size_t N>
std::size_t MakeDynamic(std::array<std::uint8_t,N>& bytes,
                        std::size_t start,unsigned id,
                        const std::string& name,std::uint32_t raw) {
    bytes.fill(0);
    Put(bytes,start,16,id);
    std::size_t end=start+16;
    for(unsigned char ch:name) {
        Put(bytes,end,8,ch);end+=8;
    }
    Put(bytes,end,8,0);
    end+=8;
    const auto* rule=w::RuleForType(static_cast<unsigned>(
        sin_independent::stats_original_types::FactoryTypeForStatId(id)));
    if(!rule)return end;
    for(unsigned i=0;i<rule->fields;++i){
        Put(bytes,end,32,i?0x40000000u:raw);
        end+=32;
    }
    return end;
}
int main() {
    rt::OwnedStatsRuntime runtime;
    std::array<std::uint8_t,400> bytes{};
    std::size_t end=MakeStatic(bytes,5,2,0x40000000u); // known type1
    b::Cursor beforeInit(bytes.data(),bytes.size(),end,5);
    if(runtime.Handle(27,beforeInit)!=rt::Result::NotInitialized ||
       beforeInit.position()!=5 || runtime.hooks().count()!=0)
        return Fail(1);
    if(!runtime.Initialize() || !runtime.initialized() ||
       !runtime.hooks().Complete() || !runtime.resetDirty() ||
       runtime.resetCount()!=0 || runtime.dynamicValues().size()!=0)
        return Fail(2);
    runtime.ClearResetDirty();

    // An original StaticStat packet, unaligned 5 bits into the byte,
    // updates independent static ID2; does not set the global RESET
    // flag (only per-value dirty is set).
    b::Cursor staticPacket(bytes.data(),bytes.size(),end,5);
    if(runtime.Handle(27,staticPacket)!=rt::Result::StaticUpdated ||
       staticPacket.position()!=end || runtime.resetDirty())
        return Fail(3);
    const auto* stat2=runtime.staticValues().at(2);
    if(!stat2 || stat2->factoryType()!=1 ||
       stat2->fieldBits(0)!=0x40000000u || !stat2->dirty())
        return Fail(4);

    // DynamicStat packs a 16bit ID, NUL string and type-specific raw
    // value. Each distinct name is independently stored.
    end=MakeDynamic(bytes,7,55,"pistol",0x3f800000u);
    b::Cursor firstDynamic(bytes.data(),bytes.size(),end,7);
    if(runtime.Handle(28,firstDynamic)!=rt::Result::DynamicCreated ||
       firstDynamic.position()!=end ||
       runtime.dynamicValues().size()!=1 || runtime.resetDirty())
        return Fail(5);
    const auto* gun=runtime.dynamicValues().Find(55,"pistol",6);
    if(!gun || gun->factoryType()!=5 ||
       gun->fieldBits(0)!=0x3f800000u)
        return Fail(6);
    end=MakeDynamic(bytes,2,55,"pistol",0x40800000u);
    b::Cursor updateDynamic(bytes.data(),bytes.size(),end,2);
    if(runtime.Handle(28,updateDynamic)!=rt::Result::DynamicUpdated ||
       runtime.dynamicValues().size()!=1 || gun->fieldBits(0)!=0x40800000u)
        return Fail(7);
    end=MakeDynamic(bytes,0,65,"rifle",0x40a00000u);
    b::Cursor differentType(bytes.data(),bytes.size(),end);
    if(runtime.Handle(28,differentType)!=rt::Result::DynamicCreated ||
       runtime.dynamicValues().size()!=2 ||
       !runtime.dynamicValues().Find(65,"rifle",5))
        return Fail(8);

    // Message 5 ResetHUD has a fixed one-byte envelope. The actual
    // original C++ callback ignores that byte, and in a strict order:
    //   destroy old static values / secondary owner and dynamic tree;
    //   re-create 55 stat values and 11 dynamic type buckets;
    //   mark owner reset flag.
    bytes.fill(0);
    Put(bytes,3,8,0xa5);
    b::Cursor reset(bytes.data(),bytes.size(),11,3);
    if(runtime.Handle(5,reset)!=rt::Result::ResetApplied ||
       reset.position()!=11 || runtime.resetCount()!=1 ||
       !runtime.resetDirty() || runtime.dynamicValues().size()!=0 ||
       runtime.dynamicValues().Find(55,"pistol",6))
        return Fail(9);
    const auto* restored=runtime.staticValues().at(2);
    if(!restored || restored->factoryType()!=1 ||
       restored->fieldBits(0)!=0 ||
       restored->fieldBits(1)!=0 ||
       restored->dirty())
        return Fail(10);
    for(unsigned id=0;id<55;++id) {
        const auto* value=runtime.staticValues().at(id);
        if(!value || value->factoryType()!=static_cast<unsigned>(
               sin_independent::stats_original_types::FactoryTypeForStatId(id)))
            return Fail(11);
    }

    // After reset, the same static stat value can update again and
    // the same named dynamic key MUST be treated as absent/created.
    runtime.ClearResetDirty();
    end=MakeStatic(bytes,0,2,0x40400000u);
    b::Cursor again(bytes.data(),bytes.size(),end);
    if(runtime.Handle(27,again)!=rt::Result::StaticUpdated ||
       runtime.staticValues().at(2)->fieldBits(0)!=0x40400000u ||
       runtime.resetDirty())
        return Fail(12);
    end=MakeDynamic(bytes,1,55,"pistol",0x3f800000u);
    b::Cursor freshDynamic(bytes.data(),bytes.size(),end,1);
    if(runtime.Handle(28,freshDynamic)!=rt::Result::DynamicCreated ||
       runtime.dynamicValues().size()!=1 ||
       runtime.resetCount()!=1)return Fail(13);

    // Unknown messages/invalid one-byte reset envelope must NEVER
    // advance the logical bit cursor or modify owned records.
    bytes.fill(0);
    Put(bytes,4,8,0xff);
    b::Cursor badResetShort(bytes.data(),bytes.size(),4+7,4);
    const auto beforeBadReset=runtime.resetCount();
    if(runtime.Handle(5,badResetShort)!=rt::Result::MessageHeaderRefused ||
       badResetShort.position()!=4 || runtime.resetCount()!=beforeBadReset ||
       !runtime.dynamicValues().Find(55,"pistol",6))
        return Fail(14);
    b::Cursor badResetLong(bytes.data(),bytes.size(),4+9,4);
    if(runtime.Handle(5,badResetLong)!=rt::Result::MessageHeaderRefused ||
       badResetLong.position()!=4 ||
       runtime.dynamicValues().size()!=1)return Fail(15);
    b::Cursor unknown(bytes.data(),bytes.size(),12,4);
    if(runtime.Handle(42,unknown)!=rt::Result::UnknownMessage ||
       runtime.Handle(0,unknown)!=rt::Result::UnknownMessage ||
       unknown.position()!=4)return Fail(16);

    // A valid stat ID with an incomplete raw value MUST NOT mutate
    // either the destination object or the original read cursor.
    end=MakeStatic(bytes,2,2,0x40400000u);
    b::Cursor truncated(bytes.data(),bytes.size(),end-1,2);
    if(runtime.Handle(27,truncated)!=rt::Result::MessageHeaderRefused ||
       truncated.position()!=2 ||
       runtime.staticValues().at(2)->fieldBits(0)!=0x40400000u)
        return Fail(17);

    // Dynamic name >=254 would be truncated by original %s%i in 256
    // bytes, so independent creation must refuse with NO partial
    // decoder cursor consumption and NO change to stored name count.
    const std::string excessive(254,'x');
    end=MakeDynamic(bytes,6,55,excessive,0x3f800000u);
    b::Cursor rejectedLong(bytes.data(),bytes.size(),end,6);
    if(runtime.Handle(28,rejectedLong)!=rt::Result::ValueRefused ||
       rejectedLong.position()!=6 ||
       runtime.dynamicValues().size()!=1)return Fail(18);

    // Two repeated resets should leave the 55 static owner values
    // initialized, no dynamic records and the reset flag set.
    bytes.fill(0);Put(bytes,7,8,0);
    b::Cursor resetAgain(bytes.data(),bytes.size(),15,7);
    if(runtime.Handle(5,resetAgain)!=rt::Result::ResetApplied ||
       runtime.resetCount()!=2 || !runtime.resetDirty() ||
       runtime.dynamicValues().size()!=0)return Fail(19);
    b::Cursor third(bytes.data(),bytes.size(),8,0);
    if(runtime.Handle(5,third)!=rt::Result::ResetApplied ||
       runtime.resetCount()!=3 ||
       runtime.dynamicValues().size()!=0)return Fail(20);
    if(!runtime.Initialize() || runtime.resetCount()!=0 ||
       !runtime.resetDirty() || runtime.dynamicValues().size()!=0)
        return Fail(21);

    std::puts("PASS independent StaticStat->DynamicStat->ResetHUD->recreate->update, bounded reader rollback and 55+11 reset shapes");
    return 0;
}
