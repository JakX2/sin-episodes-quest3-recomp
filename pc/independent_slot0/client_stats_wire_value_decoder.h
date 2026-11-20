#pragma once
// SiN 2006 original CStatReader (vtable 0x242AE06C) wire-value decoder,
// independently reconstructed from SHA-verified DLL reader-method calls.
// STATIC path: usermsg 27 -> stat ID u16 -> value object's vtable +0x4c.
// DYNAMIC path: usermsg 28 -> ID u16 -> NUL string <= 2047 -> +0x4c.
// The +0x4c type object forwards to one of 9 CStatReader slots.
// All supported slots call bf_read 0x240A5630 (float32) or
// 0x240A5620 (int32 word treated by x87 as signed), either once or twice.
//
// The 16 factory type discriminants and 66 stat IDs are NOT the same.
// Until the original statID->factoryType association is authenticated,
// consumers MUST pass a trusted type explicitly. This implementation
// never invokes original methods or mutates real stats; it merely
// returns raw 32-bit value fields and a copy of dynamic text.
#include "client_stats_bitbuf_cursor.h"
#include "client_stats_value_factory_contract.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_independent::stats_wire_values {
enum class FieldKind : std::uint8_t {
    Float32Bits, SignedInt32ConvertedByFild
};
struct ReaderSlot final {
    std::uint8_t offset;
    std::uint32_t original_wire_method_va;
    FieldKind kind;
    std::uint8_t fields;
};
inline constexpr std::uint32_t kOriginalCStatReaderVtableVA=0x242ae06cu;
inline constexpr std::array<ReaderSlot,9> kWireReaderSlots{{
    {0x04,0x24203c30u,FieldKind::Float32Bits,1},
    {0x08,0x24203c10u,FieldKind::Float32Bits,1},
    {0x0c,0x24203bf0u,FieldKind::Float32Bits,1},
    {0x10,0x24203bb0u,FieldKind::Float32Bits,2},
    {0x14,0x24203b60u,FieldKind::SignedInt32ConvertedByFild,2},
    {0x18,0x24203b40u,FieldKind::Float32Bits,1},
    {0x1c,0x24203b10u,FieldKind::Float32Bits,1},
    {0x20,0x24203af0u,FieldKind::Float32Bits,1},
    {0x24,0x24203ac0u,FieldKind::SignedInt32ConvertedByFild,1}
}};
constexpr const ReaderSlot* RuleForSlot(std::uint8_t offset) noexcept {
    for(const auto& slot:kWireReaderSlots)
        if(slot.offset==offset)return &slot;
    return nullptr;
}
constexpr const ReaderSlot* RuleForType(unsigned originalType) noexcept {
    const auto* type=stats_value_types::TryGet(originalType);
    return type?RuleForSlot(type->reader_slot_offset):nullptr;
}
constexpr bool RulesComplete() noexcept {
    for(unsigned i=0;i<stats_value_types::kTypeCount;++i){
        const auto* rule=RuleForType(i);
        if(!rule || rule->fields<1 || rule->fields>2)return false;
    }
    return RuleForType(2)->fields==2 &&
           RuleForType(3)->fields==2 &&
           RuleForType(8)->fields==1 &&
           RuleForType(0)->kind==FieldKind::SignedInt32ConvertedByFild &&
           RuleForType(1)->kind==FieldKind::Float32Bits;
}
static_assert(RulesComplete(),"16 stat types must have SHA-grounded wire read operations");

struct RawField final {
    FieldKind kind=FieldKind::Float32Bits;
    std::uint32_t bits=0; // raw bit pattern; no unsafe numerical mutation
};
struct Frame final {
    int message_id=-1;
    int stat_id=-1;
    unsigned factory_type=0;
    std::array<RawField,2> fields{};
    unsigned field_count=0;
    bool has_dynamic_text=false;
    std::array<char,stats_bitbuf::kDynamicTextBufferBytes> dynamic_text{};
    std::size_t dynamic_text_bytes=0;
    std::size_t opaque_start_bit=0;
    std::size_t opaque_remaining_bits=0;
};
enum class Status {
    DecodedRawFieldsOnly, UnverifiedFactoryType, UnsupportedMessage,
    RejectedHeader, RejectedDynamicString, TruncatedValue
};
inline const char* StatusName(Status s) noexcept {
    switch(s){
        case Status::DecodedRawFieldsOnly:return "raw-fields-only-no-stat-mutation";
        case Status::UnverifiedFactoryType:return "unknown-value-factory-type";
        case Status::UnsupportedMessage:return "unsupported-message";
        case Status::RejectedHeader:return "refused-stat-id-header";
        case Status::RejectedDynamicString:return "refused-dynamic-string";
        case Status::TruncatedValue:return "truncated-32bit-value";
    }
    return "unknown";
}
// Caller MUST obtain originalType from a separately authenticated
// statID/type registry, not from arbitrary packet content. The current
// project has not yet proven all 66 mappings. This function never
// silently chooses or invents a type based on stat ID.
inline Status InspectFrame(
    const stats_messages::OwnedRegistry& hooks,int messageId,
    unsigned originalType,stats_bitbuf::Cursor& source,
    Frame* output=nullptr) noexcept {
    if(output)*output={};
    const ReaderSlot* rule=RuleForType(originalType);
    if(!rule)return Status::UnverifiedFactoryType;
    if(messageId!=27 && messageId!=28)
        return Status::UnsupportedMessage;
    stats_bitbuf::Cursor pending=source;
    Frame staged{};
    staged.message_id=messageId;
    staged.factory_type=originalType;
    if(messageId==27){
        stats_bitbuf::ParsedHeader header{};
        if(stats_bitbuf::InspectBitHeader(hooks,27,pending,&header)!=
                stats_messages::HeaderStatus::KnownHeaderOnly)
            return Status::RejectedHeader;
        staged.stat_id=header.stat_id;
    }else{
        stats_bitbuf::DynamicText dynamic{};
        if(stats_bitbuf::InspectDynamicText(hooks,pending,&dynamic)!=
                stats_bitbuf::DynamicTextStatus::DecodedHeaderAndStringOnly)
            return Status::RejectedDynamicString;
        staged.stat_id=dynamic.stat_id;
        staged.has_dynamic_text=true;
        staged.dynamic_text=dynamic.text;
        staged.dynamic_text_bytes=dynamic.text_bytes;
    }
    if(pending.remaining()<static_cast<std::size_t>(rule->fields)*32u)
        return Status::TruncatedValue;
    for(unsigned i=0;i<rule->fields;++i){
        std::uint32_t bits=0;
        if(!pending.ReadUBits(32,bits))
            return Status::TruncatedValue;
        staged.fields[i]={rule->kind,bits};
    }
    staged.field_count=rule->fields;
    staged.opaque_start_bit=pending.position();
    staged.opaque_remaining_bits=pending.remaining();
    if(output)*output=staged;
    source=pending;
    return Status::DecodedRawFieldsOnly;
}
} // namespace sin_independent::stats_wire_values
