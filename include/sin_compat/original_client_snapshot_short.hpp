#pragma once
// Phase218 independent C++17 model of four fully SHA-reviewed native x86
// functions in original 2006 SiN client.dll prediction snapshot type 7.
// This is NOT the game's owning class, serializer, or game-engine module.
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include "sin_compat/original_client_snapshot_field_copy.hpp"

namespace sin_compat::original_client_snapshot_short {
using Bytes = std::vector<std::uint8_t>;

inline constexpr std::uint32_t kCompareVA=0x24197D50u;
inline constexpr std::uint32_t kWordCopyVA=0x241989E0u;
inline constexpr std::uint32_t kDiagnosticsVA=0x24197B10u;
inline constexpr std::uint32_t kPreviewVA=0x24198380u;
inline constexpr std::uint32_t kCompareCaller=0x241999F4u;
inline constexpr std::uint32_t kCopyCaller=0x24199A05u;
inline constexpr std::uint32_t kDiagnosticsCaller=0x24199A10u;
inline constexpr std::uint32_t kPreviewCaller=0x24199A1Fu;

struct OriginalFields {
    std::uint8_t enabled_14{};         // Original this+0x14
    std::uint8_t descriptor_flags_13{}; // Original descriptor[this+0x18]+0x13
    std::uint8_t copy_enabled_2c{};    // Separate copy gate at this+0x2C
    std::uint32_t descriptor_token_18{}; // Pointer identity as 32-bit value
    std::uint32_t preview_token_34{}; // Pointer identity as 32-bit value
};

// Native integer sign extension is explicit, independent of host char sign
// or implementation-defined uint16_t-to-int16_t casts.
inline std::int32_t read_signed_short(const Bytes& bytes, std::size_t off) {
    if (off >= bytes.size() || bytes.size()-off < 2u)
        throw std::out_of_range("host-only signed short pointer bounds");
    const std::uint32_t raw=std::uint32_t(bytes[off]) |
        (std::uint32_t(bytes[off+1u])<<8u);
    return raw<0x8000u ? static_cast<std::int32_t>(raw) :
                         static_cast<std::int32_t>(raw)-0x10000;
}

// Original 84-byte x86 method 0x24197D50, native RET 12.
// Disabled this+0x14 -> 0, descriptor bit 0x04 -> 1,
// signed word_count<=0 -> 1, otherwise compare EXACT 16-bit words
// in forward order, exit on the first mismatch (0), else return 1.
// Host validation occurs per accessed word: unused suffixes are not scanned.
inline std::uint32_t compare_words(const OriginalFields& state,
        const Bytes& predicted,std::size_t pred_off,
        const Bytes& net,std::size_t net_off,
        std::int32_t word_count) {
    if(state.enabled_14==0u) return 0u;
    if((state.descriptor_flags_13&0x04u)!=0u) return 1u;
    if(word_count<=0) return 1u;
    if(pred_off>predicted.size() || net_off>net.size())
        throw std::out_of_range("host-only original short compare offsets");
    for(std::size_t i=0u;i<static_cast<std::size_t>(word_count);++i) {
        if(i>=(predicted.size()-pred_off)/2u ||
           i>=(net.size()-net_off)/2u)
            throw std::out_of_range("host-only original short comparison bounds");
        const auto pred=pred_off+i*2u;
        const auto src=net_off+i*2u;
        if(predicted[pred]!=net[src] || predicted[pred+1u]!=net[src+1u])
            return 0u;
    }
    return 1u;
}

// Native 48-byte 0x241989E0: original first stack arg ==1 or this+0x2C
// zero skips without touching pointers or the count; else unsigned
// DWORD word_count is multiplied by 2 with i386 wrap and copied forwards
// as REP MOVSD (whole DWORD at a time), then REP MOVSB for 2-byte tail.
// Portable model refuses overflow instead of silently wrapping.
inline void copy_words(const OriginalFields& state,std::uint32_t status,
        const Bytes& net,std::size_t net_off,
        Bytes& predicted,std::size_t pred_off,std::uint32_t word_count) {
    if(!original_client_snapshot_field_copy::should_copy(
        {state.copy_enabled_2c},status)) return;
    if(word_count>std::numeric_limits<std::uint32_t>::max()/2u)
        throw std::overflow_error("host-only original word count * 2 overflow");
    original_client_snapshot_field_copy::forward_copy(
        net,net_off,predicted,pred_off,static_cast<std::size_t>(word_count)*2u);
}

enum class TraceKind { Difference, Status, Preview };
struct TraceEvent {
    TraceKind kind{};
    std::uint32_t status{};
    std::int32_t net{};
    std::int32_t predicted{};
    std::int32_t predicted_minus_net{};
};
struct ITraceSink {
    virtual ~ITraceSink()=default;
    virtual void on_event(const TraceEvent& event)=0;
};

// Original 79-byte method 0x24197B10, RET 16.
// If enabled and status==0, original formatter receives the FIRST signed
// short of net and predicted and (predicted - net). The status formatter
// always follows it when enabled, even when status != 0. Both are called
// after the original word-copy helper in the router's type-7 case.
inline void diagnostics(const OriginalFields& state,std::uint32_t status,
        const Bytes& predicted,std::size_t pred_off,
        const Bytes& net,std::size_t net_off,ITraceSink& sink) {
    if(state.enabled_14==0u) return;
    if(status==0u) {
        const auto pred=read_signed_short(predicted,pred_off);
        const auto src=read_signed_short(net,net_off);
        sink.on_event({TraceKind::Difference,status,src,pred,pred-src});
    }
    sink.on_event({TraceKind::Status,status,0,
                   read_signed_short(predicted,pred_off),0});
}

// Original 33-byte 0x24198380, RET 16. Independently tests the DWORD
// pointer-token equality this+0x34 == this+0x18 and forwards the first
// signed 16-bit destination value to the original "short (%i)" callback.
// Does NOT require this+0x14 to be enabled.
inline void preview(const OriginalFields& state,
        const Bytes& predicted,std::size_t pred_off,ITraceSink& sink) {
    if(state.preview_token_34!=state.descriptor_token_18) return;
    sink.on_event({TraceKind::Preview,0u,0,
                   read_signed_short(predicted,pred_off),0});
}

// Selected Phase212 native type-7 four-direct-CALL order:
// compare -> copy -> diagnostics -> preview. Entire host object and
// serializer/networked prediction lifecycle remain explicitly opaque.
// Negative counts can lead to 32-bit unsigned copy overflow when disabled;
// guard that host-only situation without inventing original x86 behavior.
inline std::uint32_t run_type7_direct_calls(
        const OriginalFields& state,const Bytes& net,std::size_t net_off,
        Bytes& predicted,std::size_t pred_off,std::int32_t word_count,
        ITraceSink& sink) {
    const auto status=compare_words(
        state,predicted,pred_off,net,net_off,word_count);
    if(word_count<0 && original_client_snapshot_field_copy::should_copy(
        {state.copy_enabled_2c},status))
        throw std::overflow_error("host-only negative short count copy refusal");
    copy_words(state,status,net,net_off,predicted,pred_off,
               static_cast<std::uint32_t>(word_count));
    diagnostics(state,status,predicted,pred_off,net,net_off,sink);
    preview(state,predicted,pred_off,sink);
    return status;
}

} // namespace sin_compat::original_client_snapshot_short
