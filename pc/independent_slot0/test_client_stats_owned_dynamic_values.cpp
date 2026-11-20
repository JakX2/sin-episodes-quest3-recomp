// Owner-original-free native Win32 proof of original DynamicStat's
// bounded "%s%i" name-key lookup, create/reuse and independent
// type-specific value update. No proprietary CUtlRBTree or game ABI.
#include "client_stats_owned_dynamic_values.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace d=sin_independent::stats_dynamic_values;
namespace w=sin_independent::stats_wire_values;
namespace b=sin_independent::stats_bitbuf;
namespace m=sin_independent::stats_messages;
static int Fail(int code) {
    std::fprintf(stderr,"owned dynamic named stat fixture failure=%d\n",code);
    return code;
}
template<std::size_t N>
void Put(std::array<std::uint8_t,N>& data,std::size_t offset,
         unsigned bits,std::uint32_t value) {
    for(unsigned i=0;i<bits;++i){
        const auto pos=offset+i;
        const auto mask=static_cast<std::uint8_t>(1u<<(pos&7u));
        if((value>>i)&1u)data[pos>>3]|=mask;
        else data[pos>>3]&=static_cast<std::uint8_t>(~mask);
    }
}
static w::Frame MakeDynamic(unsigned id,const char* name,
                            std::uint32_t raw=0x3f800000u) {
    w::Frame result{};
    const int type=sin_independent::stats_original_types::FactoryTypeForStatId(id);
    const auto* rule=w::RuleForType(static_cast<unsigned>(type));
    if(!rule)return result;
    result.message_id=28;
    result.stat_id=static_cast<int>(id);
    result.factory_type=static_cast<unsigned>(type);
    result.field_count=rule->fields;
    result.has_dynamic_text=true;
    const auto length=std::strlen(name);
    if(length<result.dynamic_text.size()){
        std::memcpy(result.dynamic_text.data(),name,length);
        result.dynamic_text_bytes=length;
        result.dynamic_text[length]=0;
    }
    result.fields[0]={rule->kind,raw};
    if(rule->fields==2)result.fields[1]={rule->kind,0x40000000u};
    return result;
}
int main(){
    if(d::kMinDynamicStatId!=55 || d::kMaxDynamicStatId!=65 ||
       d::kOriginalFormattedKeyBufferBytes!=256 ||
       d::kMaxDynamicNameLength!=253)return Fail(1);
    std::string key;
    if(!d::OwnedDynamicValues::MakeKey(55,"pistol",6,key) ||
       key!="pistol55" ||
       !d::OwnedDynamicValues::MakeKey(65,"",0,key) ||
       key!="65" ||
       d::OwnedDynamicValues::MakeKey(54,"pistol",6,key) ||
       d::OwnedDynamicValues::MakeKey(66,"pistol",6,key))
        return Fail(2);
    // Ensure Find accepts exactly three readable bytes with no fourth
    // terminator and never touches name[length] out of range.
    const char three[3]={'a','b','c'};
    if(!d::OwnedDynamicValues::MakeKey(55,three,3,key) ||
       key!="abc55")return Fail(3);

    d::OwnedDynamicValues owned;
    if(owned.size()!=0 || owned.Find(55,"pistol",6))
        return Fail(4);
    if(owned.Apply(MakeDynamic(55,"pistol"),false)!=
           d::Status::MissingAndCreationDisabled ||
       owned.size()!=0)return Fail(5);
    const auto first=MakeDynamic(55,"pistol");
    if(owned.Apply(first)!=d::Status::CreatedAndUpdated ||
       owned.size()!=1 || !owned.Find(55,"pistol",6) ||
       owned.Find(56,"pistol",6) ||
       owned.Find(55,"Pistol",6))return Fail(6);
    const auto original=owned.Find(55,"pistol",6);
    if(!original || original->factoryType()!=
        static_cast<unsigned>(
            sin_independent::stats_original_types::FactoryTypeForStatId(55)))
        return Fail(7);
    const auto previous=original->fieldBits(0);
    if(owned.Apply(MakeDynamic(55,"pistol",0x40000000u),false)!=
           d::Status::ReusedAndUpdated ||
       owned.size()!=1 || original->fieldBits(0)==previous)
        return Fail(8);
    if(owned.Apply(MakeDynamic(55,"Pistol"))!=
           d::Status::CreatedAndUpdated ||
       owned.size()!=2)return Fail(9);
    if(owned.Apply(MakeDynamic(55,""))!=
           d::Status::CreatedAndUpdated ||
       owned.size()!=3 || !owned.Find(55,"",0))return Fail(10);

    // All 11 original dynamic IDs are independently typed using the
    // compiled SHA-verified 66-stat registry, and each distinct
    // original name forms a separate named record.
    for(unsigned id=55;id<=65;++id){
        const auto name="weapon"+std::to_string(id);
        const auto frame=MakeDynamic(id,name.c_str(),0);
        if(owned.Apply(frame)!=d::Status::CreatedAndUpdated)return Fail(11);
        const auto* stat=owned.Find(id,name.c_str(),name.size());
        if(!stat || stat->factoryType()!=
             static_cast<unsigned>(
                sin_independent::stats_original_types::FactoryTypeForStatId(id)))
            return Fail(12);
        if(owned.Apply(frame)!=d::Status::ReusedAndUpdated ||
           owned.Find(id,name.c_str(),name.size())!=stat)
            return Fail(13);
    }
    if(owned.size()!=14)return Fail(14);

    // End-to-end actual wire format: DynamicStat message ID28,
    // 16-bit stat ID at non-byte offset, NUL name string,
    // authenticated original per-ID numeric field reader.
    m::OwnedRegistry hooks;
    for(unsigned i=0;i<3;++i)
        if(!hooks.RegisterStatsStep(
            sin_independent::stats_system::kInitSteps[4+i]))
            return Fail(15);
    for(unsigned start=0;start<8;++start){
        std::array<std::uint8_t,80> payload{};
        const unsigned id=60;
        const auto* kind=w::RuleForType(
            static_cast<unsigned>(
                sin_independent::stats_original_types::FactoryTypeForStatId(id)));
        Put(payload,start,16,id);
        Put(payload,start+16,8,'g');
        Put(payload,start+24,8,'u');
        Put(payload,start+32,8,'n');
        Put(payload,start+40,8,0);
        Put(payload,start+48,32,0x40a00000u);
        if(kind->fields==2)
            Put(payload,start+80,32,0x40000000u);
        const unsigned end=start+48+kind->fields*32;
        b::Cursor cursor(payload.data(),payload.size(),end,start);
        w::Frame parsed{};
        if(w::InspectFrameForRegisteredId(hooks,28,cursor,&parsed)!=
                w::Status::DecodedRawFieldsOnly ||
           parsed.stat_id!=60 || parsed.dynamic_text_bytes!=3 ||
           owned.Apply(parsed)!=(start==0?
                d::Status::CreatedAndUpdated:d::Status::ReusedAndUpdated) ||
           !owned.Find(60,"gun",3))return Fail(16);
    }
    // A 253-char name plus the two-digit original ID fills exactly
    // the original 256-byte snprintf buffer, including its trailing
    // NUL. 254 would truncate and is rejected by this safer port.
    const std::string longValid(253,'q'), longInvalid(254,'q');
    const auto longFrame=MakeDynamic(55,longValid.c_str());
    if(owned.Apply(longFrame)!=d::Status::CreatedAndUpdated ||
       !owned.Find(55,longValid.data(),longValid.size()))return Fail(17);
    const auto beforeLong=owned.size();
    if(owned.Apply(MakeDynamic(55,longInvalid.c_str()))!=
           d::Status::NameTooLongOrMalformed ||
       owned.size()!=beforeLong)return Fail(18);

    // Malformed decoded frames never insert a named record.
    auto invalid=MakeDynamic(55,"bad");
    invalid.has_dynamic_text=false;
    if(owned.Apply(invalid)!=d::Status::InvalidDynamicFrame ||
       owned.Find(55,"bad",3))return Fail(19);
    invalid=MakeDynamic(55,"bad");
    invalid.factory_type=16;
    if(owned.Apply(invalid)!=d::Status::BadOriginalType ||
       owned.Find(55,"bad",3))return Fail(20);
    invalid=MakeDynamic(55,"bad");
    invalid.dynamic_text[invalid.dynamic_text_bytes]='x';
    if(owned.Apply(invalid)!=d::Status::NameTooLongOrMalformed ||
       owned.Find(55,"bad",3))return Fail(21);
    invalid=MakeDynamic(55,"bad");
    invalid.field_count=0;
    if(owned.Apply(invalid)!=d::Status::InvalidDynamicFrame ||
       owned.Find(55,"bad",3))return Fail(22);
    invalid=MakeDynamic(55,"bad");
    invalid.dynamic_text_bytes=0;
    invalid.dynamic_text[0]='b';
    if(owned.Apply(invalid)!=d::Status::NameTooLongOrMalformed ||
       owned.Find(55,"bad",3))return Fail(23);

    // Bounded independent dynamic name count (the original tree's
    // allocator semantics and precise comparator remain unresolved).
    d::OwnedDynamicValues capacity;
    for(unsigned i=0;i<d::kConservativeMaxNames;++i){
        const auto name="n"+std::to_string(i);
        if(capacity.Apply(MakeDynamic(55,name.c_str()))!=
                d::Status::CreatedAndUpdated)
            return Fail(24);
    }
    if(capacity.size()!=d::kConservativeMaxNames ||
       capacity.Apply(MakeDynamic(55,"beyond"))!=d::Status::CapacityRefused ||
       capacity.size()!=d::kConservativeMaxNames ||
       capacity.Apply(MakeDynamic(55,"n0"))!=d::Status::ReusedAndUpdated)
        return Fail(25);
    capacity.Clear();
    if(capacity.size()!=0 || capacity.Find(55,"n0",2))
        return Fail(26);

    std::puts("PASS 11 dynamic IDs, bounded original name+ID lookup/create/reuse, 256-byte key safety and type-correct owned updates");
    return 0;
}
