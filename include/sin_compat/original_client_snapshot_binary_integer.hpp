#pragma once
// Phase217: independent C++17 reference for four verified 2006 SiN
// client.dll snapshot binary/integer helpers, NOT a Source engine module.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "sin_compat/original_client_snapshot_field_copy.hpp"

namespace sin_compat::original_client_snapshot_binary_integer {
using Bytes=std::vector<std::uint8_t>;
inline constexpr std::uint32_t kCompareVA=0x24197A90u;
inline constexpr std::uint32_t kBinaryDiagnosticVA=0x24197AD0u;
inline constexpr std::uint32_t kIntegerDiagnosticVA=0x24197B60u;
inline constexpr std::uint32_t kBinaryPreviewVA=0x24198360u;

struct OriginalFields {
    std::uint8_t enabled_14{};           // Native this+0x14
    std::uint8_t descriptor_flags_13{};  // Native this+0x18 -> +0x13
    std::uint8_t copy_enabled_2c{};      // Native this+0x2C, separate gate
    std::uint32_t descriptor_token_18{}; // Original object+0x18 DWORD
    std::uint32_t preview_token_34{};    // Original object+0x34 DWORD
};

// Original 56-byte shared case-8/case-9 comparator at 0x24197A90.
// If disabled return 0, if descriptor bit 4 set return 1; otherwise
// REPZ CMPSB compares EXACTLY native unsigned BYTE count forwards.
// An initial zero count returns 1. Compare exits at the first mismatch.
// Host-only checks protect reads that the native x86 would perform.
inline std::uint32_t compare_bytes(
        const OriginalFields& state,std::uint32_t count,
        const Bytes& predicted,std::size_t pred_off,
        const Bytes& net,std::size_t net_off) {
    if(state.enabled_14==0u) return 0u;
    if((state.descriptor_flags_13 & 0x04u)!=0u) return 1u;
    for(std::size_t i=0;i<static_cast<std::size_t>(count);++i) {
        if(pred_off>=predicted.size() || i>=predicted.size()-pred_off ||
           net_off>=net.size() || i>=net.size()-net_off)
            throw std::out_of_range("host-only original REPZ CMPSB bounds");
        if(predicted[pred_off+i]!=net[net_off+i])
            return 0u; // Native REPZ exits on the FIRST mismatching byte.
    }
    return 1u;
}

enum class TraceKind {
    BinaryDifference, BinaryStatus, BinaryPreview,
    IntegerDifference, IntegerSymbol, IntegerNumeric
};
struct TraceEvent {
    TraceKind kind{};
    std::uint32_t status{};
    std::uint32_t byte_count{};
    std::int32_t net{};
    std::int32_t predicted{};
    std::int32_t difference{};
    std::string symbol;
};
struct ITraceSink {
    virtual ~ITraceSink()=default;
    virtual void on_event(const TraceEvent& value)=0;
};
struct ISymbolHost {
    virtual ~ISymbolHost()=default;
    // Original calls a process-global registry's virtual +0x04 with the
    // first integer (predicted) and, on nonzero result, virtual +0x0C,
    // followed by vstdlib.dll!Q_FileBase into a 512-byte local buffer.
    // Mapping these exact engine objects is NOT part of this reference.
    virtual std::optional<std::string> find_name(std::int32_t value)=0;
    virtual std::string q_file_base(const std::string& name,
                                    std::size_t destination_capacity)=0;
};

// Original 60-byte binary diagnostic at 0x24197AD0. The second native
// argument is a BYTE count. The first is comparison status. Enabled,
// status-zero invokes "binary data differs (%i bytes)\\n" before the
// unconditional "binary (%i bytes)\\n" native formatter callback.
inline void binary_diagnostics(
        const OriginalFields& state,std::uint32_t status,
        std::uint32_t byte_count,ITraceSink& sink) {
    if(state.enabled_14==0u) return;
    if(status==0u)
        sink.on_event({TraceKind::BinaryDifference,status,byte_count});
    sink.on_event({TraceKind::BinaryStatus,status,byte_count});
}

// Original 30-byte shared preview at 0x24198360, original RET 16.
// When DWORD(this+0x34)==DWORD(this+0x18), forwards second stack DWORD
// as the decimal byte count to "binary (%i bytes)"; ignores enabled_14.
inline void binary_preview(const OriginalFields& state,
                           std::uint32_t byte_count,ITraceSink& sink) {
    if(state.preview_token_34!=state.descriptor_token_18) return;
    sink.on_event({TraceKind::BinaryPreview,0u,byte_count});
}

inline std::int32_t signed_32(std::uint32_t value) {
    std::int32_t signed_value{};
    static_assert(sizeof(value)==sizeof(signed_value),
                  "original i386 integer requires a 32-bit signed representation");
    std::memcpy(&signed_value,&value,sizeof(value));
    return signed_value;
}
inline std::int32_t signed_byte(std::uint8_t value) noexcept {
    return value<128u ? static_cast<std::int32_t>(value) :
                        static_cast<std::int32_t>(value)-256;
}
inline std::uint8_t first_byte(const Bytes& value,std::size_t offset) {
    if(offset>=value.size())
        throw std::out_of_range("host-only native signed first-byte read");
    return value[offset];
}

// Original 197-byte integer diagnostic at 0x24197B60. Native status-zero
// reports a 32-bit wrapping predicted-minus-net difference. All enabled
// paths then print the predicted integer. Descriptor bit 0x08 invokes a
// global symbol lookup (vtable +0x04/+0x0C) and Q_FileBase when found,
// otherwise falls back to numeric print. No fabricated registry identity.
inline void integer_diagnostics(
        const OriginalFields& state,std::uint32_t status,
        std::int32_t predicted,std::int32_t net,
        ITraceSink& sink,ISymbolHost* symbols) {
    if(state.enabled_14==0u) return;
    if(status==0u) {
        const auto diff=signed_32(static_cast<std::uint32_t>(predicted)-
                                  static_cast<std::uint32_t>(net));
        sink.on_event({TraceKind::IntegerDifference,status,0u,
                       net,predicted,diff});
    }
    if((state.descriptor_flags_13 & 0x08u)!=0u) {
        if(!symbols)
            throw std::runtime_error("host-only missing native symbol resolver");
        const auto name=symbols->find_name(predicted);
        if(name) {
            auto base=symbols->q_file_base(*name,512u);
            if(base.size()>=512u)
                throw std::length_error("host-only 512-byte Q_FileBase output bounds");
            sink.on_event({TraceKind::IntegerSymbol,status,0u,
                           0,predicted,0,std::move(base)});
            return;
        }
    }
    sink.on_event({TraceKind::IntegerNumeric,status,0u,0,predicted});
}

// Synthetic-only selected Phase212 case-9 direct branch composition.
// Original dword_count is multiplied by four in i386 modulo arithmetic;
// this reference REJECTS a wrap rather than concealing an unsafe copy.
// The actual owning record, allocator and game engine are not modeled.
inline std::uint32_t run_binary_case9_calls(
        const OriginalFields& state,std::uint32_t dword_count,
        const Bytes& net,std::size_t net_off,
        Bytes& predicted,std::size_t pred_off,ITraceSink& sink) {
    if(dword_count>std::numeric_limits<std::uint32_t>::max()/4u)
        throw std::overflow_error("host-only case9 count multiplication overflow");
    const auto bytes=dword_count*4u;
    const auto status=compare_bytes(state,bytes,predicted,pred_off,net,net_off);
    if(state.copy_enabled_2c!=0u && status!=1u) {
        original_client_snapshot_field_copy::forward_copy(
            net,net_off,predicted,pred_off,bytes);
    }
    binary_diagnostics(state,status,bytes,sink);
    binary_preview(state,bytes,sink);
    return status;
}

// Selected Phase212 case-8 direct branch composition. The native integer
// reporter receives sign-extended FIRST bytes of destination and source,
// read after the conditional inline copy, regardless of byte_count.
inline std::uint32_t run_integer_case8_calls(
        const OriginalFields& state,std::uint32_t byte_count,
        const Bytes& net,std::size_t net_off,
        Bytes& predicted,std::size_t pred_off,
        ITraceSink& sink,ISymbolHost* symbols) {
    const auto status=compare_bytes(
        state,byte_count,predicted,pred_off,net,net_off);
    if(state.copy_enabled_2c!=0u && status!=1u) {
        original_client_snapshot_field_copy::forward_copy(
            net,net_off,predicted,pred_off,byte_count);
    }
    const auto pred=signed_byte(first_byte(predicted,pred_off));
    const auto original=signed_byte(first_byte(net,net_off));
    integer_diagnostics(state,status,pred,original,sink,symbols);
    binary_preview(state,byte_count,sink);
    return status;
}

} // namespace sin_compat::original_client_snapshot_binary_integer
