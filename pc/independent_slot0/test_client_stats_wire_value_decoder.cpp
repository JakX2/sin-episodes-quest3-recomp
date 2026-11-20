// Original-free native MSVC fixtures: 16 SHA-grounded client Stats
// factory value-type codes, CStatReader vtable method shapes and
// independent bounded LSB-first raw 32-bit numeric frame extraction.
#include "client_stats_wire_value_decoder.h"
#include <array>
#include <cstdint>
#include <cstdio>

namespace m=sin_independent::stats_messages;
namespace b=sin_independent::stats_bitbuf;
namespace w=sin_independent::stats_wire_values;
static int Fail(int code) {
    std::fprintf(stderr,"Stats wire value native failure %d\n",code);
    return code;
}
template<std::size_t N>
void Bits(std::array<std::uint8_t,N>& buffer,std::size_t first,
          unsigned count,std::uint32_t value) {
    for(unsigned i=0;i<count;++i){
        const auto offset=first+i;
        const std::uint8_t mask=static_cast<std::uint8_t>(1u<<(offset&7u));
        if((value>>i)&1u)buffer[offset>>3]|=mask;
        else buffer[offset>>3]&=static_cast<std::uint8_t>(~mask);
    }
}
int main(){
    if(!w::RulesComplete() ||
       w::kOriginalCStatReaderVtableVA!=0x242ae06cu ||
       w::kWireReaderSlots.size()!=9 ||
       w::RuleForSlot(0x3c)!=nullptr)return Fail(1);
    m::OwnedRegistry hooks;
    for(unsigned i=0;i<3;++i)
        if(!hooks.RegisterStatsStep(
            sin_independent::stats_system::kInitSteps[4+i]))return Fail(2);
    for(unsigned type=0;type<16;++type){
        const auto* spec=w::RuleForType(type);
        const auto* factory=sin_independent::stats_value_types::TryGet(type);
        if(!spec || !factory || spec->fields<1 || spec->fields>2 ||
           spec->offset!=factory->reader_slot_offset)return Fail(3);
        for(unsigned offset=0;offset<9;++offset){
            std::array<std::uint8_t,64> packet{};
            Bits(packet,offset,16,54);
            Bits(packet,offset+16,32,0x80000001u);
            if(spec->fields==2)
                Bits(packet,offset+48,32,0x7f800000u);
            const unsigned bits=16u+spec->fields*32u;
            Bits(packet,offset+bits,5,0x1d);
            b::Cursor cursor(packet.data(),packet.size(),offset+bits+5u,offset);
            w::Frame frame{};
            const auto result=w::InspectFrame(hooks,27,type,cursor,&frame);
            if(result!=w::Status::DecodedRawFieldsOnly ||
               frame.message_id!=27 || frame.stat_id!=54 ||
               frame.factory_type!=type || frame.has_dynamic_text ||
               frame.field_count!=spec->fields ||
               frame.fields[0].bits!=0x80000001u ||
               frame.fields[0].kind!=spec->kind ||
               frame.opaque_remaining_bits!=5 ||
               frame.opaque_start_bit!=offset+bits ||
               cursor.position()!=offset+bits)
                return Fail(4);
            if(spec->fields==2 &&
               (frame.fields[1].bits!=0x7f800000u ||
                frame.fields[1].kind!=spec->kind))
                return Fail(5);
            std::uint32_t trailing=0;
            if(!cursor.ReadUBits(5,trailing) || trailing!=0x1d)
                return Fail(6);
        }
    }
    // Exact recovered CStatReader source-method metadata:
    // type 0 reads 32-bit integer converted to float via x87 FILD,
    // type 1 reads float32, type 2 reads two integers, type 3 two floats.
    if(w::RuleForType(0)->kind!=w::FieldKind::SignedInt32ConvertedByFild ||
       w::RuleForType(1)->kind!=w::FieldKind::Float32Bits ||
       w::RuleForType(2)->fields!=2 ||
       w::RuleForType(2)->kind!=w::FieldKind::SignedInt32ConvertedByFild ||
       w::RuleForType(3)->fields!=2 ||
       w::RuleForType(3)->kind!=w::FieldKind::Float32Bits ||
       w::RuleForType(8)->fields!=1 ||
       w::RuleForType(10)->fields!=1)return Fail(7);

    // Real DynamicStat reads u16 statId, terminated string, then one
    // or two source wire primitives according to the actual value
    // object's type. Confirm unaligned starts for both field classes.
    for(const unsigned type:{0u,2u,3u,8u,11u,15u}){
        const auto* rule=w::RuleForType(type);
        for(unsigned offset=0;offset<8;++offset){
            std::array<std::uint8_t,64> packet{};
            Bits(packet,offset,16,55);
            Bits(packet,offset+16,8,'O');
            Bits(packet,offset+24,8,'K');
            Bits(packet,offset+32,8,0);
            Bits(packet,offset+40,32,0xdeadbeefu);
            if(rule->fields==2)
                Bits(packet,offset+72,32,0x01020304u);
            const auto end=offset+40u+32u*rule->fields;
            b::Cursor cursor(packet.data(),packet.size(),end,offset);
            w::Frame frame{};
            if(w::InspectFrame(hooks,28,type,cursor,&frame)!=
                   w::Status::DecodedRawFieldsOnly ||
               frame.message_id!=28 || frame.stat_id!=55 ||
               frame.factory_type!=type || !frame.has_dynamic_text ||
               frame.dynamic_text_bytes!=2 ||
               frame.dynamic_text[0]!='O' ||
               frame.dynamic_text[1]!='K' ||
               frame.dynamic_text[2]!=0 ||
               frame.field_count!=rule->fields ||
               frame.fields[0].bits!=0xdeadbeefu ||
               frame.opaque_remaining_bits!=0 ||
               cursor.position()!=end)
                return Fail(8);
            if(rule->fields==2 &&
               frame.fields[1].bits!=0x01020304u)
                return Fail(9);
        }
    }
    // Refuse unknown discriminator before altering original stream.
    std::array<std::uint8_t,20> invalid{};
    Bits(invalid,3,16,54);
    Bits(invalid,19,32,0x3f800000u);
    b::Cursor unknown(invalid.data(),invalid.size(),51,3);
    w::Frame output{};
    output.stat_id=1234;
    if(w::InspectFrame(hooks,27,16,unknown,&output)!=
           w::Status::UnverifiedFactoryType ||
       unknown.position()!=3 || output.stat_id!=-1 ||
       w::InspectFrame(hooks,5,0,unknown)!=
           w::Status::UnsupportedMessage ||
       unknown.position()!=3)return Fail(10);
    // Type 2 must consume 64 wire bits. One bit missing refuses
    // transactionally, without accidentally storing a half-value.
    b::Cursor shortValue(invalid.data(),invalid.size(),3+16+63,3);
    if(w::InspectFrame(hooks,27,2,shortValue,&output)!=
           w::Status::TruncatedValue ||
       shortValue.position()!=3 || output.stat_id!=-1)
        return Fail(11);
    Bits(invalid,3,16,55); // not a StaticStat index
    b::Cursor wrongClass(invalid.data(),invalid.size(),51,3);
    if(w::InspectFrame(hooks,27,1,wrongClass)!=
           w::Status::RejectedHeader ||
       wrongClass.position()!=3)return Fail(12);
    // Missing terminator is an explicit refusal before numeric reads.
    std::array<std::uint8_t,32> dynamicBad{};
    Bits(dynamicBad,2,16,60);
    Bits(dynamicBad,18,8,'x');
    b::Cursor unterminated(dynamicBad.data(),dynamicBad.size(),26,2);
    if(w::InspectFrame(hooks,28,1,unterminated)!=
           w::Status::RejectedDynamicString ||
       unterminated.position()!=2)return Fail(13);
    // Per-listener independent copies, never mutate the base cursor
    // when the same message has multiple callbacks.
    std::array<std::uint8_t,32> isolated{};
    Bits(isolated,7,16,0);
    Bits(isolated,23,32,0x3eaaaaabu);
    b::Cursor original(isolated.data(),isolated.size(),55,7);
    b::Cursor a=original,c=original;
    if(w::InspectFrame(hooks,27,1,a)!=w::Status::DecodedRawFieldsOnly ||
       w::InspectFrame(hooks,27,1,c)!=w::Status::DecodedRawFieldsOnly ||
       original.position()!=7 || a.position()!=55 || c.position()!=55)
        return Fail(14);
    hooks.Clear();
    b::Cursor unbound(isolated.data(),isolated.size(),55,7);
    if(w::InspectFrame(hooks,27,1,unbound)!=w::Status::RejectedHeader ||
       unbound.position()!=7)return Fail(15);
    std::puts("PASS 16 factory types, 9 SHA-derived CStatReader slots, unaligned static/dynamic 32/64-bit raw numeric frames; no mutation");
    return 0;
}
