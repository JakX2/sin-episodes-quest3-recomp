#pragma once
// 2006 SiN Episodes: Emergence, independently owned stat VALUE updates.
// Reconstructed from SHA256-pinned original client.dll and its 16-value
// factory + CStatReader virtual dispatch. Original addresses below are
// only identity witnesses, NEVER function pointers or executable jumps.
//
// Original 32-bit client SHA256:
// 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
//
// Critical: the original CStatReader methods choose vtable +0x08,
// +0x18, +0x2c, OR a direct write for factory type 7. Mutations below
// are NOT guessed from numeric field widths. They follow original
// setter bytecode audited separately. This is NOT the game's real
// object ABI and does NOT touch 2006 engine/client-owned memory.
#include "client_stats_wire_value_decoder.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace sin_independent::stats_value_mutation {
enum class PrimaryOperation : std::uint8_t {
    Integer32Store, Float32Store, DirectFloat32Store, Float32AuxStore,
    Float32Max, Float32Min
};
struct OriginalTypeUpdate final {
    unsigned type;
    std::uint32_t original_primary_method_va; // original vtable +0x18
    std::uint32_t original_secondary_method_va; // +0x2c
    PrimaryOperation operation;
    bool paired_field;
};
// Derive these from the ORIGINAL value-object vtables, not from a
// reinterpretation of wire reader vtables. Type 7's +0x18 is a NO-OP
// (0x24205760); its live reader writes field +0x0c directly.
// Factory types 2 and 3 use the SECOND vtable entry +0x2c.
inline constexpr std::array<OriginalTypeUpdate,16> kOriginalUpdates{{
    { 0,0x242053e0u,0x242052d0u,PrimaryOperation::Integer32Store,false},
    { 1,0x24205490u,0x242052d0u,PrimaryOperation::Float32Store,false},
    { 2,0x242053e0u,0x24205540u,PrimaryOperation::Integer32Store,true},
    { 3,0x24205490u,0x242055d0u,PrimaryOperation::Float32Store,true},
    { 4,0x24205490u,0x242055d0u,PrimaryOperation::Float32Store,false},
    { 5,0x24205490u,0x242055d0u,PrimaryOperation::Float32Store,false},
    { 6,0x24205490u,0x242055d0u,PrimaryOperation::Float32Store,false},
    { 7,0x24205760u,0x242052d0u,PrimaryOperation::DirectFloat32Store,false},
    { 8,0x242057f0u,0x242052d0u,PrimaryOperation::Float32AuxStore,false},
    { 9,0x242057f0u,0x242052d0u,PrimaryOperation::Float32AuxStore,false},
    {10,0x242057f0u,0x242052d0u,PrimaryOperation::Float32AuxStore,false},
    {11,0x242057f0u,0x24205890u,PrimaryOperation::Float32AuxStore,false},
    {12,0x24205930u,0x242052d0u,PrimaryOperation::Float32Max,false},
    {13,0x242059e0u,0x242052d0u,PrimaryOperation::Float32Min,false},
    {14,0x24205930u,0x24205ab0u,PrimaryOperation::Float32Max,false},
    {15,0x242059e0u,0x24205b60u,PrimaryOperation::Float32Min,false}
}};
constexpr bool OriginalUpdateContract() noexcept {
    for(unsigned i=0;i<kOriginalUpdates.size();++i){
        if(kOriginalUpdates[i].type!=i ||
           kOriginalUpdates[i].original_primary_method_va<0x24205000u ||
           kOriginalUpdates[i].original_secondary_method_va<0x24205000u)
            return false;
        const auto* reader=stats_wire_values::RuleForType(i);
        if(!reader || (reader->fields==2)!=kOriginalUpdates[i].paired_field)
            return false;
    }
    return kOriginalUpdates[7].original_primary_method_va==0x24205760u &&
           kOriginalUpdates[0].original_primary_method_va==0x242053e0u &&
           kOriginalUpdates[12].original_primary_method_va==0x24205930u &&
           kOriginalUpdates[13].original_primary_method_va==0x242059e0u;
}
static_assert(OriginalUpdateContract(),
              "16 original type update methods vs live CStatReader shape");

inline float Float32FromBits(std::uint32_t bits) noexcept {
    float value=0.0f;
    static_assert(sizeof(value)==sizeof(bits),"float32 required");
    std::memcpy(&value,&bits,sizeof(value));
    return value;
}
inline std::uint32_t BitsFromFloat32(float value) noexcept {
    std::uint32_t result=0;
    std::memcpy(&result,&value,sizeof(result));
    return result;
}
// Original Integer32Store path:
// bf_read ReadUBitLong(32) -> x87 FILD as signed int32 ->
// FSTP float32 -> value-object FISTP with CW rounding toward zero.
// The second FISTP returns x87's integer-indefinite 0x80000000 for
// out-of-range or invalid float values. Preserve the float32
// precision loss for integers above 2^24 instead of simply storing
// the original wire integer unchanged.
inline std::uint32_t OriginalIntStoreBits(std::uint32_t wireBits) noexcept {
    const auto signedInput=static_cast<std::int32_t>(wireBits);
    const float rounded=static_cast<float>(signedInput);
    const double integral=std::trunc(static_cast<double>(rounded));
    if(!std::isfinite(integral) || integral < -2147483648.0 ||
       integral>=2147483648.0)return 0x80000000u;
    const auto converted=static_cast<std::int32_t>(integral);
    return static_cast<std::uint32_t>(converted);
}
enum class Result {
    UpdatedOwnedValue, ExtremaDidNotAdvance, NotInitialized,
    WrongFactoryType, WrongFieldKind, WrongFieldCount
};
inline const char* ResultName(Result result) noexcept {
    switch(result){
        case Result::UpdatedOwnedValue:return "owned-value-updated-only";
        case Result::ExtremaDidNotAdvance:return "min-max-rejected-unmodified";
        case Result::NotInitialized:return "not-initialized";
        case Result::WrongFactoryType:return "wrong-original-factory-type";
        case Result::WrongFieldKind:return "wrong-original-reader-field-kind";
        case Result::WrongFieldCount:return "wrong-original-reader-field-count";
    }
    return "unknown";
}
class OwnedValue final {
public:
    // Fields are non-ABI data. f[0] describes original value+0x0c,
    // f[1] value+0x10, f[2] value+0x14, f[3] value+0x18 (constructor
    // configuration), f[4] value+0x1c. No original vtable pointer.
    bool Initialize(unsigned factoryType) noexcept {
        initialized_=false; type_=0; dirty_=false; fields_.fill(0);
        if(!stats_value_types::IsKnown(factoryType))return false;
        type_=factoryType;
        if(factoryType==7 || (factoryType>=8 && factoryType<=11))
            fields_[1]=0xbf800000u; // original -1.0f sentinel
        if(factoryType>=8 && factoryType<=11){
            fields_[3]=stats_value_types::kCases[factoryType].config_bits;
            dirty_=true; // original constructors explicitly set +0x08
        }
        if(factoryType==12 || factoryType==14){
            fields_[0]=0xff7fffffu; // -FLT_MAX sentinel
            dirty_=true;
        }else if(factoryType==13 || factoryType==15){
            fields_[0]=0x7f7fffffu; // +FLT_MAX sentinel
            dirty_=true;
        }
        initialized_=true;
        return true;
    }
    bool initialized() const noexcept{return initialized_;}
    unsigned factoryType() const noexcept{return type_;}
    bool dirty() const noexcept{return dirty_;}
    void ClearDirty() noexcept{dirty_=false;}
    std::uint32_t fieldBits(unsigned index) const noexcept {
        return index<fields_.size()?fields_[index]:0u;
    }
    Result Apply(const stats_wire_values::Frame& frame) noexcept {
        if(!initialized_)return Result::NotInitialized;
        if(frame.factory_type!=type_)return Result::WrongFactoryType;
        const auto* reader=stats_wire_values::RuleForType(type_);
        if(!reader || frame.field_count!=reader->fields)
            return Result::WrongFieldCount;
        for(unsigned i=0;i<reader->fields;++i)
            if(frame.fields[i].kind!=reader->kind)
                return Result::WrongFieldKind;
        // A bounded atomic value-state mutation; this object is
        // independent, no direct original engine callback.
        auto candidate=fields_;
        bool touched=true;
        const auto operation=kOriginalUpdates[type_].operation;
        switch(operation){
            case PrimaryOperation::Integer32Store:
                candidate[0]=OriginalIntStoreBits(frame.fields[0].bits);
                if(reader->fields==2)
                    candidate[1]=OriginalIntStoreBits(frame.fields[1].bits);
                break;
            case PrimaryOperation::Float32Store:
            case PrimaryOperation::DirectFloat32Store:
                candidate[0]=frame.fields[0].bits;
                if(reader->fields==2)candidate[1]=frame.fields[1].bits;
                break;
            case PrimaryOperation::Float32AuxStore:
                candidate[2]=frame.fields[0].bits;
                break;
            case PrimaryOperation::Float32Max:
                // x87 FCOMP+FNSTSW only updates for ordered new > old.
                touched=Float32FromBits(frame.fields[0].bits)>
                    Float32FromBits(fields_[0]);
                if(touched)candidate[0]=frame.fields[0].bits;
                break;
            case PrimaryOperation::Float32Min:
                touched=Float32FromBits(frame.fields[0].bits)<
                    Float32FromBits(fields_[0]);
                if(touched)candidate[0]=frame.fields[0].bits;
                break;
        }
        if(!touched)return Result::ExtremaDidNotAdvance;
        fields_=candidate;
        dirty_=true; // original object's vtable +0x48 mark
        return Result::UpdatedOwnedValue;
    }
private:
    std::array<std::uint32_t,5> fields_{};
    unsigned type_=0;
    bool initialized_=false, dirty_=false;
};

// One *static* value object per original stat ID 0..54. DynamicStat
// uses a separate original name-keyed allocation path, which remains
// uncharacterized. Do NOT conflate dynamic name strings with stat IDs
// or create a bogus global dynamic container.
class OwnedStaticValues final {
public:
    static constexpr unsigned kCount=55;
    bool Initialize() noexcept {
        ready_=false;
        for(unsigned i=0;i<kCount;++i){
            const int type=stats_original_types::FactoryTypeForStatId(i);
            if(type<0 || !objects_[i].Initialize(static_cast<unsigned>(type)))
                return false;
        }
        ready_=true;
        return true;
    }
    Result ApplyStatic(const stats_wire_values::Frame& frame) noexcept {
        if(!ready_)return Result::NotInitialized;
        if(frame.message_id!=27 || frame.stat_id<0 ||
           frame.stat_id>=static_cast<int>(kCount))
            return Result::WrongFactoryType;
        return objects_[static_cast<unsigned>(frame.stat_id)].Apply(frame);
    }
    const OwnedValue* at(unsigned id) const noexcept {
        return ready_ && id<kCount?&objects_[id]:nullptr;
    }
private:
    std::array<OwnedValue,kCount> objects_{};
    bool ready_=false;
};
} // namespace sin_independent::stats_value_mutation
