// Native original-free end-to-end proof of the SHA-pinned compiled
// 66-stat ID -> original 16-case value type association and the
// independent bit-accurate CStatReader raw payload decoder.
// This does NOT dispatch into real engine bf_read or mutate game stats.
#include "client_stats_wire_value_decoder.h"
#include "client_stats_original_id_map.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
namespace n=sin_independent;
namespace t=n::stats_original_types;
namespace b=n::stats_bitbuf;
namespace d=n::stats_wire_values;
namespace m=n::stats_messages;
static int Fail(int code) {
    std::fprintf(stderr,"original stat ID/type wire registry failure=%d\n",code);
    return code;
}
template<std::size_t N>
void Put(std::array<std::uint8_t,N>& packet,std::size_t pos,
         unsigned count,std::uint32_t value){
    for(unsigned i=0;i<count;++i){
        const auto bit=pos+i;
        const std::uint8_t mask=static_cast<std::uint8_t>(1u<<(bit&7u));
        if((value>>i)&1u)packet[bit>>3]|=mask;
        else packet[bit>>3]&=static_cast<std::uint8_t>(~mask);
    }
}
int main(){
    if(t::kStatCount!=66 || n::stat_name_ids::kCount!=66 ||
       t::kOriginalRecordBytes!=12 ||
       t::kOriginalMapVA!=0x2431e660u ||
       t::RegistryFNV1a64()!=0xfb1f4af74099b337ULL)
        return Fail(1);
    std::array<unsigned,16> typeCounts{};
    for(unsigned id=0;id<66;++id){
        const auto* rec=t::TryGet(id);
        if(!rec || t::FactoryTypeForStatId(id)!=rec->factory_type ||
           !d::RuleForType(rec->factory_type) ||
           rec->raw_meta_1>3 || rec->raw_meta_2<1 ||
           rec->raw_meta_2>2 ||
           n::stat_name_ids::NameForId(id).empty())
            return Fail(2);
        ++typeCounts[rec->factory_type];
    }
    if(typeCounts!=std::array<unsigned,16>{{
        38,3,3,0,0,4,1,5,3,3,1,3,0,0,2,0
    }})return Fail(3);
    if(t::TryGet(66)||t::TryGet(65535) ||
       t::FactoryTypeForStatId(66)!=-1 ||
       t::FactoryTypeForStatId(9999999u)!=-1)return Fail(4);
    m::OwnedRegistry hooks;
    for(unsigned i=0;i<3;++i)
        if(!hooks.RegisterStatsStep(
            n::stats_system::kInitSteps[4+i]))return Fail(5);

    // Test EVERY single original stat ID, not just convenient type
    // fixture IDs. The original registry decides type and number of
    // 32-bit value fields. Dynamic stat IDs also carry a string.
    for(unsigned id=0;id<66;++id){
        const auto* desc=t::TryGet(id);
        const auto* rule=d::RuleForType(desc->factory_type);
        const int messageId=id<55?27:28;
        const unsigned offset=id%9u;
        std::array<std::uint8_t,64> data{};
        Put(data,offset,16,id);
        unsigned pos=offset+16;
        if(messageId==28){
            Put(data,pos,8,'x');pos+=8;
            Put(data,pos,8,0);pos+=8;
        }
        Put(data,pos,32,0xa0f00000u|id);
        if(rule->fields==2)
            Put(data,pos+32,32,0x3f800000u|id);
        pos+=rule->fields*32u;
        Put(data,pos,3,5);
        b::Cursor cursor(data.data(),data.size(),pos+3,offset);
        d::Frame output{};
        if(d::InspectFrameForRegisteredId(hooks,messageId,cursor,&output)!=
               d::Status::DecodedRawFieldsOnly ||
           output.message_id!=messageId ||
           output.stat_id!=static_cast<int>(id) ||
           output.factory_type!=desc->factory_type ||
           output.field_count!=rule->fields ||
           output.fields[0].bits!=(0xa0f00000u|id) ||
           output.fields[0].kind!=rule->kind ||
           output.has_dynamic_text!=(messageId==28) ||
           output.opaque_start_bit!=pos ||
           output.opaque_remaining_bits!=3 ||
           cursor.position()!=pos)
            return Fail(6);
        if(rule->fields==2 &&
           output.fields[1].bits!=(0x3f800000u|id))
            return Fail(7);
        if(messageId==28 &&
           (output.dynamic_text_bytes!=1 ||
            output.dynamic_text[0]!='x' ||
            output.dynamic_text[1]!=0))
            return Fail(8);
        std::uint32_t suffix=0;
        if(!cursor.ReadUBits(3,suffix) || suffix!=5 ||
           cursor.remaining()!=0)
            return Fail(9);
    }

    // Original registry entry #46 has type 2 and consumes two 32-bit
    // fields. A one-bit truncation MUST refuse without partial mutation.
    std::array<std::uint8_t,32> shortValue{};
    Put(shortValue,3,16,46);
    Put(shortValue,19,32,0xffffffffu);
    Put(shortValue,51,31,0x7fffffffu);
    b::Cursor trunc(shortValue.data(),shortValue.size(),3+16+63,3);
    d::Frame frame{};frame.stat_id=777;
    if(d::InspectFrameForRegisteredId(hooks,27,trunc,&frame)!=
           d::Status::TruncatedValue ||
       trunc.position()!=3 || frame.stat_id!=-1)
        return Fail(10);
    // Type map must never be guessed for malformed IDs 66 or 55 in
    // static and 54 in dynamic messages.
    std::array<std::uint8_t,16> invalid{};
    Put(invalid,2,16,66);
    b::Cursor wrong(invalid.data(),invalid.size(),18,2);
    if(d::InspectFrameForRegisteredId(hooks,28,wrong)!=
           d::Status::RejectedHeader || wrong.position()!=2)
        return Fail(11);
    Put(invalid,2,16,55);
    b::Cursor badStatic(invalid.data(),invalid.size(),18,2);
    if(d::InspectFrameForRegisteredId(hooks,27,badStatic)!=
           d::Status::RejectedHeader || badStatic.position()!=2)
        return Fail(12);
    Put(invalid,2,16,54);
    b::Cursor badDynamic(invalid.data(),invalid.size(),18,2);
    if(d::InspectFrameForRegisteredId(hooks,28,badDynamic)!=
           d::Status::RejectedHeader || badDynamic.position()!=2)
        return Fail(13);
    if(d::InspectFrameForRegisteredId(hooks,5,badStatic)!=
           d::Status::UnsupportedMessage)
        return Fail(14);
    // Missing hooks fail closed before mapping or reading payloads.
    hooks.Clear();
    b::Cursor unbound(invalid.data(),invalid.size(),18,2);
    if(d::InspectFrameForRegisteredId(hooks,27,unbound)!=
           d::Status::RejectedHeader || unbound.position()!=2)
        return Fail(15);
    std::puts("PASS SHA-grounded all 66 original stat IDs -> 16 type factory -> 9 live reader methods -> raw bit-accurate payloads");
    return 0;
}
