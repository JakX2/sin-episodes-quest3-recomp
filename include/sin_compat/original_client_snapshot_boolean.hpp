#pragma once
// Phase219: independent C++17 host reference for authentic 2006 SiN
// client.dll type-6 Boolean prediction snapshot helpers.
// NOT a rebuilt engine ABI, retail gameplay client.dll, or complete serializer.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "sin_compat/original_client_snapshot_field_copy.hpp"

namespace sin_compat::original_client_snapshot_boolean {

using Bytes=std::vector<std::uint8_t>;
inline constexpr std::uint32_t kCompareVA=0x24197E30u;
inline constexpr std::uint32_t kCopyVA=0x24198A40u; // Phase213, not a new method.
inline constexpr std::uint32_t kDiagnosticsVA=0x24197C30u;
inline constexpr std::uint32_t kPreviewVA=0x24198440u;
inline constexpr std::uint32_t kCompareCaller=0x24199982u;
inline constexpr std::uint32_t kCopyCaller=0x24199993u;
inline constexpr std::uint32_t kDiagnosticsCaller=0x2419999Eu;
inline constexpr std::uint32_t kPreviewCaller=0x241999ADu;

struct OriginalFields {
    std::uint8_t enabled_14{};          // Original this+0x14
    std::uint8_t descriptor_flags_13{};  // Original [this+0x18]+0x13
    std::uint8_t copy_enabled_2c{};     // Original this+0x2C
    std::uint32_t descriptor_token_18{};
    std::uint32_t preview_token_34{};
};

inline std::uint8_t first_byte(const Bytes& b,std::size_t off) {
    if(off>=b.size())
        throw std::out_of_range("host-only Boolean pointer guard");
    return b[off];
}
inline const char* boolean_text(std::uint8_t byte) noexcept {
    // Original tests BYTE [ptr] against zero, not against the value 1.
    return byte ? "true" : "false";
}
inline std::int32_t signed_count_from_u32(std::uint32_t raw) noexcept {
    static_assert(sizeof(std::uint32_t)==sizeof(std::int32_t),
                  "native DWORD and signed count must be 32 bits");
    std::int32_t signed_value{};
    std::memcpy(&signed_value,&raw,sizeof(raw));
    return signed_value;
}

// Original 82-byte 0x24197E30 native comparison, RET 12:
// enabled=false -> status 0; descriptor bit 4 -> status 1;
// signed count <=0 -> status 1; else compare actual BYTES, stopping at
// first different byte. Difference between 1 and 0xFF matters here
// even though both are printed "true" by the original formatter.
inline std::uint32_t compare(const OriginalFields& state,
        const Bytes& net,std::size_t net_off,
        const Bytes& predicted,std::size_t predicted_off,
        std::int32_t signed_count) {
    if(state.enabled_14==0u) return 0u;
    if((state.descriptor_flags_13&0x04u)!=0u) return 1u;
    if(signed_count<=0) return 1u;
    const auto count=static_cast<std::size_t>(signed_count);
    for(std::size_t i=0;i<count;++i) {
        if(net_off>=net.size() || predicted_off>=predicted.size() ||
           i>=net.size()-net_off || i>=predicted.size()-predicted_off)
            throw std::out_of_range("host-only Boolean compare buffer bounds");
        if(net[net_off+i]!=predicted[predicted_off+i])
            return 0u;
    }
    return 1u;
}

enum class TraceKind { Difference, Status, Preview };
struct TraceEvent {
    TraceKind kind{};
    std::uint32_t operation{};
    std::string net;
    std::string predicted;
};
struct ITraceSink {
    virtual ~ITraceSink()=default;
    virtual void on_event(const TraceEvent& event)=0;
};

// Original 110-byte 0x24197C30 formatter, RET 16.
// Enabled and status == 0: sends "bool differs (net %s pred %s)\n".
// All enabled statuses: sends "bool (%s)\n" for destination.
// This uses the first BYTE of each pointer AFTER the original copy call.
inline void diagnostics(const OriginalFields& state,std::uint32_t operation,
        const Bytes& net,std::size_t net_off,
        const Bytes& predicted,std::size_t predicted_off,
        ITraceSink& sink) {
    if(state.enabled_14==0u) return;
    if(operation==0u) {
        const auto predicted_text=boolean_text(first_byte(predicted,predicted_off));
        const auto net_text=boolean_text(first_byte(net,net_off));
        sink.on_event({TraceKind::Difference,operation,net_text,predicted_text});
    }
    sink.on_event({TraceKind::Status,operation,"",
                   boolean_text(first_byte(predicted,predicted_off))});
}

// Original 45-byte 0x24198440 preview, RET 16. Independent of
// enabled_14, guarded by full 32-bit original pointer-token equality.
inline void preview(const OriginalFields& state,
        const Bytes& predicted,std::size_t predicted_off,ITraceSink& sink) {
    if(state.preview_token_34!=state.descriptor_token_18)return;
    sink.on_event({TraceKind::Preview,0u,"",
                   boolean_text(first_byte(predicted,predicted_off))});
}

// Original Phase212 type-6 direct CALL order:
// byte comparison -> Phase213 exact BYTE-count copy -> diagnostics -> preview.
// The comparison sees the ORIGINAL pre-copy destination; the formatters
// observe destination after the optional copy, preserving original behavior.
// An unsigned count whose sign bit is set is accepted when the copy skips,
// but refused as unsafe if the native code would try a huge unsigned copy.
inline std::uint32_t run_type6_direct_calls(
        const OriginalFields& state,const Bytes& net,std::size_t net_off,
        Bytes& predicted,std::size_t predicted_off,
        std::uint32_t original_byte_count,ITraceSink& sink) {
    const auto operation=compare(state,net,net_off,predicted,predicted_off,
                                 signed_count_from_u32(original_byte_count));
    if(original_byte_count>0x7fffffffu &&
       original_client_snapshot_field_copy::should_copy(
           {state.copy_enabled_2c},operation))
        throw std::overflow_error("host-only unsafe signed/unsigned Boolean count");
    original_client_snapshot_field_copy::copy_bytes_type6(
        {state.copy_enabled_2c},operation,
        net,net_off,predicted,predicted_off,original_byte_count);
    diagnostics(state,operation,net,net_off,predicted,predicted_off,sink);
    preview(state,predicted,predicted_off,sink);
    return operation;
}

} // namespace sin_compat::original_client_snapshot_boolean
