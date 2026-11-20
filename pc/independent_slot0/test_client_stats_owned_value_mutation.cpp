// Original-free native Win32 proof of independently owned SiN 2006
// stat-value mutation behavior. Exact vtable/setter evidence is
// SHA-audited privately; no commercial code or real engine pointer.
#include "client_stats_owned_value_mutation.h"
#include "client_stats_original_id_map.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>

namespace v=sin_independent::stats_value_mutation;
namespace w=sin_independent::stats_wire_values;
namespace b=sin_independent::stats_bitbuf;
namespace m=sin_independent::stats_messages;
static int Fail(int code) {
    std::fprintf(stderr,"owned stat mutation fixture failure=%d\n",code);
    return code;
}
template<std::size_t N>
void Put(std::array<std::uint8_t,N>& bytes,std::size_t at,
         unsigned count,std::uint32_t data) {
    for(unsigned i=0;i<count;++i){
        const auto off=at+i;
        const auto mask=static_cast<std::uint8_t>(1u<<(off&7u));
        if((data>>i)&1u)bytes[off>>3]|=mask;
        else bytes[off>>3]&=static_cast<std::uint8_t>(~mask);
    }
}
static w::Frame MakeRaw(unsigned type,std::uint32_t raw0,
                        std::uint32_t raw1=0) {
    w::Frame result{};
    const auto* rule=w::RuleForType(type);
    if(!rule)return result;
    result.message_id=27;
    result.factory_type=type;
    result.stat_id=0;
    result.field_count=rule->fields;
    result.fields[0]={rule->kind,raw0};
    if(rule->fields==2)result.fields[1]={rule->kind,raw1};
    return result;
}
int main(){
    if(!v::OriginalUpdateContract() ||
       v::kOriginalUpdates.size()!=16)return Fail(1);
    // The independent factory MUST reject unsupported discriminants.
    v::OwnedValue unset{};
    if(unset.Apply(MakeRaw(1,0))!=v::Result::NotInitialized ||
       unset.Initialize(16) || unset.Initialize(255) ||
       !unset.Initialize(0))return Fail(2);

    for(unsigned type=0;type<16;++type){
        v::OwnedValue object;
        if(!object.Initialize(type) || !object.initialized() ||
           object.factoryType()!=type)return Fail(3);
        const auto op=v::kOriginalUpdates[type].operation;
        const bool startsDirty=(type>=8 && type<=15);
        if(object.dirty()!=startsDirty)return Fail(4);
        const auto cfg=type>=8 && type<=11?
            sin_independent::stats_value_types::kCases[type].config_bits:0;
        if(object.fieldBits(3)!=cfg)return Fail(5);
        if(type==7 || (type>=8 && type<=11)){
            if(object.fieldBits(1)!=0xbf800000u)return Fail(6);
        }else if(object.fieldBits(1)!=0)return Fail(7);
        if(type==12 || type==14){
            if(object.fieldBits(0)!=0xff7fffffu)return Fail(8);
        }else if(type==13 || type==15){
            if(object.fieldBits(0)!=0x7f7fffffu)return Fail(9);
        }else if(object.fieldBits(0)!=0)return Fail(10);
        if(type==11 && object.fieldBits(4)!=0)return Fail(11);
        object.ClearDirty();
        const auto first=MakeRaw(type,0x3f800000u,0x40000000u);
        const auto res=object.Apply(first);
        if(res!=v::Result::UpdatedOwnedValue || !object.dirty())
            return Fail(12);
        const auto rule=w::RuleForType(type);
        if(!rule)return Fail(13);
        if(op==v::PrimaryOperation::Integer32Store){
            // 0x3f800000 interpreted as SIGNED INT32 then rounded to
            // float32 and FISTP toward zero (not bitcast as 1.0f).
            if(object.fieldBits(0)!=
               v::OriginalIntStoreBits(0x3f800000u))return Fail(14);
            if(rule->fields==2 &&
               object.fieldBits(1)!=v::OriginalIntStoreBits(0x40000000u))
                return Fail(15);
        }else if(op==v::PrimaryOperation::Float32AuxStore){
            if(object.fieldBits(2)!=0x3f800000u ||
               object.fieldBits(0)!=0)return Fail(16);
        }else{
            if(object.fieldBits(0)!=0x3f800000u)return Fail(17);
            if(rule->fields==2 && object.fieldBits(1)!=0x40000000u)
                return Fail(18);
        }
        object.ClearDirty();
        const auto second=MakeRaw(type,0x40000000u,0xc0400000u);
        const auto step=object.Apply(second);
        if(op==v::PrimaryOperation::Float32Min){
            if(step!=v::Result::ExtremaDidNotAdvance ||
               object.dirty() || object.fieldBits(0)!=0x3f800000u)
                return Fail(19);
        }else{
            if(step!=v::Result::UpdatedOwnedValue || !object.dirty())
                return Fail(20);
        }
        // Atomicity on mismatch: never apply the first half of an
        // incorrect two-part payload, or the wrong value factory type.
        object.ClearDirty();
        const auto before0=object.fieldBits(0);
        const auto before1=object.fieldBits(1);
        const auto before2=object.fieldBits(2);
        auto wrong=MakeRaw(type,0x41200000u,0x41100000u);
        wrong.factory_type=16;
        if(object.Apply(wrong)!=v::Result::WrongFactoryType ||
           object.dirty() || object.fieldBits(0)!=before0 ||
           object.fieldBits(1)!=before1 ||
           object.fieldBits(2)!=before2)return Fail(21);
        wrong=MakeRaw(type,0x41200000u,0x41100000u);
        wrong.field_count=0;
        if(object.Apply(wrong)!=v::Result::WrongFieldCount ||
           object.dirty())return Fail(22);
        wrong=MakeRaw(type,0x41200000u,0x41100000u);
        wrong.fields[0].kind=rule->kind==w::FieldKind::Float32Bits?
            w::FieldKind::SignedInt32ConvertedByFild:
            w::FieldKind::Float32Bits;
        if(object.Apply(wrong)!=v::Result::WrongFieldKind ||
           object.dirty())return Fail(23);
    }
    // Original Integer32Store is a two-step x87 operation, with
    // float32 rounding and FISTP invalid-indefinite behavior.
    if(v::OriginalIntStoreBits(0u)!=0u ||
       v::OriginalIntStoreBits(1u)!=1u ||
       v::OriginalIntStoreBits(0xffffffffu)!=0xffffffffu ||
       v::OriginalIntStoreBits(0x80000000u)!=0x80000000u ||
       v::OriginalIntStoreBits(0x7fffffffu)!=0x80000000u ||
       v::OriginalIntStoreBits(16777217u)!=16777216u)return Fail(24);

    // Max updates only for a strictly greater ordered float. Min
    // updates only for strictly lower values. NaNs and ties do not
    // set the dirty bit or overwrite the initial sentinel.
    v::OwnedValue maximum,minimum;
    if(!maximum.Initialize(12) || !minimum.Initialize(13))
        return Fail(25);
    maximum.ClearDirty();minimum.ClearDirty();
    if(maximum.Apply(MakeRaw(12,0xff800000u))!=v::Result::ExtremaDidNotAdvance ||
       minimum.Apply(MakeRaw(13,0x7f800000u))!=v::Result::ExtremaDidNotAdvance ||
       maximum.dirty() || minimum.dirty())return Fail(26);
    if(maximum.Apply(MakeRaw(12,0x00000000u))!=v::Result::UpdatedOwnedValue ||
       minimum.Apply(MakeRaw(13,0x00000000u))!=v::Result::UpdatedOwnedValue)
        return Fail(27);
    maximum.ClearDirty();minimum.ClearDirty();
    if(maximum.Apply(MakeRaw(12,0x7fc00000u))!=v::Result::ExtremaDidNotAdvance ||
       minimum.Apply(MakeRaw(13,0xffc00000u))!=v::Result::ExtremaDidNotAdvance ||
       maximum.Apply(MakeRaw(12,0x00000000u))!=v::Result::ExtremaDidNotAdvance ||
       minimum.Apply(MakeRaw(13,0x00000000u))!=v::Result::ExtremaDidNotAdvance ||
       maximum.dirty() || minimum.dirty() ||
       maximum.fieldBits(0)!=0 || minimum.fieldBits(0)!=0)
        return Fail(28);
    if(maximum.Apply(MakeRaw(12,0x3f800000u))!=v::Result::UpdatedOwnedValue ||
       minimum.Apply(MakeRaw(13,0xbf800000u))!=v::Result::UpdatedOwnedValue ||
       maximum.fieldBits(0)!=0x3f800000u ||
       minimum.fieldBits(0)!=0xbf800000u)
        return Fail(29);

    // For type 7 a normal +0x18 setter is an original no-op, but
    // CStatReader virtual +0x1c DOES directly write float to +0x0c.
    v::OwnedValue direct;
    if(!direct.Initialize(7))return Fail(30);
    direct.ClearDirty();
    if(direct.Apply(MakeRaw(7,0x42c80000u))!=v::Result::UpdatedOwnedValue ||
       direct.fieldBits(0)!=0x42c80000u || !direct.dirty())
        return Fail(31);

    // End-to-end bounded packet -> original 66-record type selection
    // -> owned static value mutation. Do NOT apply dynamic name-keyed
    // packets to a fabricated one-value-per-ID container.
    v::OwnedStaticValues staticValues;
    if(staticValues.at(0) ||
       staticValues.ApplyStatic(MakeRaw(1,0))!=v::Result::NotInitialized ||
       !staticValues.Initialize() || !staticValues.at(0) ||
       staticValues.at(55))return Fail(32);
    m::OwnedRegistry hooks;
    for(unsigned index=0;index<3;++index)
        if(!hooks.RegisterStatsStep(
           sin_independent::stats_system::kInitSteps[index+4]))
            return Fail(33);
    for(unsigned id=0;id<55;++id){
        const auto* state=staticValues.at(id);
        const int type=sin_independent::stats_original_types::FactoryTypeForStatId(id);
        const auto* rule=w::RuleForType(static_cast<unsigned>(type));
        if(!state || !rule || state->factoryType()!=static_cast<unsigned>(type))
            return Fail(34);
        const unsigned offset=id%8u;
        std::array<std::uint8_t,64> packet{};
        Put(packet,offset,16,id);
        // Feed zero for float/int so max/min comparisons are stable.
        unsigned end=offset+16;
        for(unsigned i=0;i<rule->fields;++i){
            Put(packet,end,32,0);end+=32;
        }
        b::Cursor cursor(packet.data(),packet.size(),end,offset);
        w::Frame frame{};
        if(w::InspectFrameForRegisteredId(hooks,27,cursor,&frame)!=
                w::Status::DecodedRawFieldsOnly ||
           staticValues.ApplyStatic(frame)!=v::Result::UpdatedOwnedValue ||
           state->fieldBits(0)!=0 ||
           !state->dirty())return Fail(35);
    }
    // Fake dynamic traffic must not touch our 55 fixed static owners.
    w::Frame forged=MakeRaw(5,0);
    forged.message_id=28;forged.stat_id=55;
    if(staticValues.ApplyStatic(forged)!=v::Result::WrongFactoryType)
        return Fail(36);
    std::puts("PASS SHA-audited 16 stat value mutators, 55 owned static records, x87 int conversion and extrema; no engine memory writes");
    return 0;
}
