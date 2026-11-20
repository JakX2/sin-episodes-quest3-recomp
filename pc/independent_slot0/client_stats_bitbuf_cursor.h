#pragma once
// Source 2004-compatible independent *bitstream* cursor for the recovered
// SiN Stats user-message ID headers. Source bf_read packs bits LSB-first;
// this implementation is completely owned and never casts or touches the
// 2006 engine's 24-byte bf_read object. Only bit-field HEADER selection
// is reconstructed; per-type statistic values remain opaque.
// Reference: public Valve SDK tier1/bitbuf.h (ReadUBitLong).
#include "client_stats_user_message_hooks.h"
#include <cstddef>
#include <cstdint>
#include <array>
#include <cstring>

namespace sin_independent::stats_bitbuf {
inline constexpr std::size_t kMaxMessageBytes=256u*1024u;

// Independent value type. NOT a Source ABI-compatible bf_read layout.
// The original CUserMessages dispatcher copies the 24-byte native reader
// for each handler. Copying this value instead provides the same logical
// cursor isolation without relying on unknown private memory fields.
class Cursor final {
public:
    Cursor()=default;
    Cursor(const std::uint8_t* data,std::size_t byteLength,
           std::size_t bitLength,std::size_t firstBit=0) noexcept {
        Reset(data,byteLength,bitLength,firstBit);
    }
    bool Reset(const std::uint8_t* data,std::size_t byteLength,
               std::size_t bitLength,std::size_t firstBit=0) noexcept {
        data_=nullptr;
        capacity_bytes_=0;
        bit_length_=position_=0;
        overflow_=false;
        valid_=false;
        if(byteLength>kMaxMessageBytes || (byteLength && !data) ||
           bitLength>byteLength*8u || firstBit>bitLength)return false;
        data_=data;
        capacity_bytes_=byteLength;
        bit_length_=bitLength;
        position_=firstBit;
        valid_=true;
        return true;
    }
    bool valid() const noexcept{return valid_;}
    bool overflow() const noexcept{return overflow_;}
    std::size_t position() const noexcept{return position_;}
    std::size_t bitLength() const noexcept{return bit_length_;}
    std::size_t remaining() const noexcept {
        return valid_ && position_<=bit_length_?
            bit_length_-position_:0;
    }
    bool ReadUBits(unsigned count,std::uint32_t& result) noexcept {
        result=0;
        if(!valid_ || count==0 || count>32)return false;
        if(static_cast<std::size_t>(count)>remaining()) {
            // Valve's 2004 ReadUBitLong clamps to the end and marks
            // overflow on an attempted read beyond m_nDataBits.
            position_=bit_length_;
            overflow_=true;
            return false;
        }
        std::uint32_t value=0;
        for(unsigned bit=0;bit<count;++bit) {
            const std::size_t offset=position_+bit;
            const auto read=static_cast<std::uint32_t>(
                (data_[offset >> 3] >> (offset & 7u)) & 1u);
            value|=(read << bit);
        }
        position_+=count;
        result=value;
        return true;
    }
private:
    const std::uint8_t* data_=nullptr;
    std::size_t capacity_bytes_=0;
    std::size_t bit_length_=0;
    std::size_t position_=0;
    bool overflow_=false,valid_=false;
};

struct ParsedHeader final {
    int message_id=-1;
    int stat_id=-1;
    std::size_t opaque_start_bit=0;
    std::size_t opaque_bits=0;
};
// Fail-closed, *transactional* header analysis: malformed messages do
// not consume the caller's owned cursor. This deliberately differs from
// a raw Source bf_read overrun, where overflow is set and the position
// clamps to the end. A successful parse advances through the known
// 16-bit ID field, leaving the unknown value bits entirely untouched.
inline stats_messages::HeaderStatus InspectBitHeader(
    const stats_messages::OwnedRegistry& registry,
    int messageId,Cursor& stream,ParsedHeader* output=nullptr) noexcept {
    using stats_messages::HeaderStatus;
    if(output)*output={};
    if(!stream.valid() || messageId<0 ||
       messageId>=static_cast<int>(kUserMessageCount))
        return HeaderStatus::InvalidMessage;
    const stats_messages::HookSpec* hook=nullptr;
    for(std::size_t i=0;i<stats_messages::kOriginalStatsHooks.size();++i){
        if(stats_messages::kOriginalStatsHooks[i].message_id==messageId){
            if(registry.at(i)!=messageId)return HeaderStatus::MissingHook;
            hook=&stats_messages::kOriginalStatsHooks[i];
            break;
        }
    }
    if(!hook)return HeaderStatus::InvalidMessage;
    Cursor pending=stream; // per-listener copy; never dereference engine bf_read
    unsigned readBits=hook->kind==stats_messages::HookKind::ResetHUD?8u:16u;
    if(hook->kind==stats_messages::HookKind::ResetHUD &&
       pending.remaining()!=8)
        return HeaderStatus::ResetLengthRefused;
    if(pending.remaining()<readBits)return HeaderStatus::PayloadTruncated;
    std::uint32_t value=0;
    if(!pending.ReadUBits(readBits,value))return HeaderStatus::PayloadTruncated;
    if(hook->kind==stats_messages::HookKind::StaticStat && value>=55)
        return HeaderStatus::StaticIdRefused;
    if(hook->kind==stats_messages::HookKind::DynamicStat &&
       (value<55 || value>=66))
        return HeaderStatus::DynamicIdRefused;
    if(output){
        output->message_id=messageId;
        output->stat_id=hook->kind==stats_messages::HookKind::ResetHUD?
            -1:static_cast<int>(value);
        output->opaque_start_bit=pending.position();
        output->opaque_bits=pending.remaining();
    }
    stream=pending; // commit only verified header consumption
    return HeaderStatus::KnownHeaderOnly;
}

// Original private 2006 client.dll SHA-pinned function 0x241FB370 reads
// UBitLong(16) and then calls helper 0x240A5740 to read a NUL-terminated
// string through a 0x800 (2048) byte temporary buffer. The helper is
// the engine's ReadString path, not a fixed-length float or integer.
// This is an independently authored *bounded* parser of those TWO
// recovered fields; the later per-type dynamic stat action is opaque.
inline constexpr std::size_t kDynamicTextBufferBytes=0x800;
enum class DynamicTextStatus {
    DecodedHeaderAndStringOnly, InvalidHeader, TruncatedText,
    MissingTerminatorWithinLimit
};
inline const char* DynamicTextName(DynamicTextStatus status) noexcept {
    switch(status){
        case DynamicTextStatus::DecodedHeaderAndStringOnly:
            return "dynamic-id-and-null-string-only";
        case DynamicTextStatus::InvalidHeader:return "invalid-dynamic-header";
        case DynamicTextStatus::TruncatedText:return "truncated-dynamic-text";
        case DynamicTextStatus::MissingTerminatorWithinLimit:
            return "unterminated-dynamic-text";
    }
    return "unknown";
}
struct DynamicText final {
    int stat_id=-1;
    std::array<char,kDynamicTextBufferBytes> text{};
    std::size_t text_bytes=0;
    std::size_t opaque_start_bit=0;
    std::size_t opaque_bits=0;
};

// Only commit cursor+output if both the 16-bit ID and bounded
// NUL-terminated text are valid. Do not accept an incomplete message
// or silently truncate a string, which may differ from the original
// engine behavior but is safer for a stand-alone port.
inline DynamicTextStatus InspectDynamicText(
    const stats_messages::OwnedRegistry& registry,
    Cursor& source,DynamicText* output=nullptr) noexcept {
    if(output)*output={};
    Cursor pending=source;
    ParsedHeader header{};
    if(InspectBitHeader(registry,28,pending,&header)!=
            stats_messages::HeaderStatus::KnownHeaderOnly)
        return DynamicTextStatus::InvalidHeader;
    std::array<char,kDynamicTextBufferBytes> scratch{};
    for(std::size_t i=0;i<scratch.size();++i){
        if(pending.remaining()<8u)
            return DynamicTextStatus::TruncatedText;
        std::uint32_t letter=0;
        if(!pending.ReadUBits(8u,letter))
            return DynamicTextStatus::TruncatedText;
        scratch[i]=static_cast<char>(letter);
        if(letter==0){
            if(output){
                output->stat_id=header.stat_id;
                output->text=scratch;
                output->text_bytes=i;
                output->opaque_start_bit=pending.position();
                output->opaque_bits=pending.remaining();
            }
            source=pending;
            return DynamicTextStatus::DecodedHeaderAndStringOnly;
        }
    }
    return DynamicTextStatus::MissingTerminatorWithinLimit;
}
} // namespace sin_independent::stats_bitbuf
