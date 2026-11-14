#pragma once
// Phase220: independently authored portable type-5 native helper reference
// for 2006 SiN Episodes client.dll. Exact 2006 function entries were reviewed;
// no original function bodies, SDK source or original game code are reproduced.
// This is NOT an ABI-compatible client.dll replacement.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>

#include "sin_compat/original_client_snapshot_field_copy.hpp"
#include "sin_compat/original_client_snapshot_binary_integer.hpp"

namespace sin_compat::original_client_snapshot_type5 {
namespace binary = original_client_snapshot_binary_integer;
namespace copy = original_client_snapshot_field_copy;

using OriginalFields=binary::OriginalFields;
using Bytes=binary::Bytes;
using TraceKind=binary::TraceKind;
using TraceEvent=binary::TraceEvent;
using ITraceSink=binary::ITraceSink;
using ISymbolHost=binary::ISymbolHost;

inline constexpr std::uint32_t kCompareVA=0x24197DB0u;
inline constexpr std::uint32_t kCopyVA=0x24198A10u; // Phase213 reviewed.
inline constexpr std::uint32_t kDiagnosticsVA=0x24197B60u; // Phase217 reviewed.
inline constexpr std::uint32_t kPreviewVA=0x241983B0u;
inline constexpr std::uint32_t kCompareCaller=0x241999BCu;
inline constexpr std::uint32_t kCopyCaller=0x241999CBu;
inline constexpr std::uint32_t kDiagnosticsCaller=0x241999D6u;
inline constexpr std::uint32_t kPreviewCaller=0x241999E5u;

// Exact little-endian DWORD word read; sign conversion follows native
// 32-bit two's-complement bit representation, without host alignment UB.
inline std::int32_t read_signed_dword(const Bytes& data,std::size_t offset) {
    static_assert(sizeof(std::int32_t)==4u,"native i386 word size");
    if(offset>data.size() || data.size()-offset<4u)
        throw std::out_of_range("host-only original type5 DWORD bounds");
    const std::uint32_t bits=static_cast<std::uint32_t>(data[offset]) |
        (static_cast<std::uint32_t>(data[offset+1u])<<8u) |
        (static_cast<std::uint32_t>(data[offset+2u])<<16u) |
        (static_cast<std::uint32_t>(data[offset+3u])<<24u);
    return binary::signed_32(bits);
}

// Original 125-byte native x86 function 0x24197DB0, RET 12.
// disabled this+0x14 -> return 0; descriptor flag +0x13 bit 4 ->
// return 1; signed DWORD count <= 0 -> return 1.
// Otherwise compares DWORD values in order and, on FIRST mismatch,
// invokes original formatter at 0x24197910 with actual values and
// 32-bit wrapping (predicted - net), BEFORE returning 0.
// The subsequent type5 diagnostic method may log another mismatch
// AFTER the original conditional copy. Keep both events separate.
inline std::uint32_t compare_dwords_with_immediate_diagnostic(
        const OriginalFields& state,
        const Bytes& predicted,std::size_t predicted_offset,
        const Bytes& net,std::size_t net_offset,
        std::int32_t signed_dword_count,
        ITraceSink& sink) {
    if(state.enabled_14==0u)return 0u;
    if((state.descriptor_flags_13 & 0x04u)!=0u)return 1u;
    if(signed_dword_count<=0)return 1u;
    if(predicted_offset>predicted.size() || net_offset>net.size())
        throw std::out_of_range("host-only native type5 starting pointers");

    for(std::size_t index=0;index<static_cast<std::size_t>(signed_dword_count);++index) {
        const std::size_t pred_available=(predicted.size()-predicted_offset)/4u;
        const std::size_t net_available=(net.size()-net_offset)/4u;
        if(index>=pred_available || index>=net_available)
            throw std::out_of_range("host-only native type5 compared DWORD count");
        const auto predicted_value=read_signed_dword(
            predicted,predicted_offset+index*4u);
        const auto net_value=read_signed_dword(net,net_offset+index*4u);
        if(predicted_value==net_value)continue;

        const auto difference=binary::signed_32(
            static_cast<std::uint32_t>(predicted_value)-
            static_cast<std::uint32_t>(net_value));
        sink.on_event({TraceKind::IntegerDifference,0u,0u,
                       net_value,predicted_value,difference,{}});
        return 0u;
    }
    return 1u;
}

// Original 144-byte native preview 0x241983B0, RET 16, followed
// IMMEDIATELY by the next function at 0x24198440 (zero INT3 pad).
// Only if pointer-token this+0x34 == this+0x18, it displays the
// FIRST destination DWORD. When descriptor bit 0x08 is set,
// original global registry at 0x2433F6AC uses virtual slots +4/+C
// and vstdlib.dll!Q_FileBase into a 512-byte local buffer.
// A missing lookup falls back to integer (%i). It is NOT gated
// by this+0x14, and does not inspect the comparison status.
inline void preview(const OriginalFields& state,
                    const Bytes& predicted,std::size_t predicted_offset,
                    ITraceSink& sink,ISymbolHost* symbols) {
    if(state.preview_token_34!=state.descriptor_token_18)return;
    const auto value=read_signed_dword(predicted,predicted_offset);
    if((state.descriptor_flags_13 & 0x08u)!=0u) {
        if(!symbols)
            throw std::runtime_error("host-only missing original integer symbol registry");
        const auto name=symbols->find_name(value);
        if(name) {
            auto leaf=symbols->q_file_base(*name,512u);
            if(leaf.size()>=512u)
                throw std::length_error("host-only Q_FileBase 512-byte output bound");
            sink.on_event({TraceKind::IntegerSymbol,0u,0u,
                           0,value,0,std::move(leaf)});
            return;
        }
    }
    sink.on_event({TraceKind::IntegerNumeric,0u,0u,0,value,0,{}});
}

// Four original direct Phase212 router CALLs for type 5:
// 0x24197DB0 compare + immediate mismatch report ->
// 0x24198A10 Phase213 conditional DWORD copy ->
// 0x24197B60 Phase217 integer diagnostic and name resolution ->
// 0x241983B0 symbol-aware preview.
// The later two observe the post-copy FIRST destination DWORD.
// Retains separate comparison and copy gates; the host-only copy
// guard refuses raw count multiplication overflow.
inline std::uint32_t run_type5_direct_calls(
        const OriginalFields& state,
        const Bytes& net,std::size_t net_offset,
        Bytes& predicted,std::size_t predicted_offset,
        std::uint32_t native_dword_count,
        ITraceSink& sink,ISymbolHost* symbols) {
    const auto operation=compare_dwords_with_immediate_diagnostic(
        state,predicted,predicted_offset,net,net_offset,
        binary::signed_32(native_dword_count),sink);
    copy::copy_dwords_type5(
        {state.copy_enabled_2c},operation,
        net,net_offset,predicted,predicted_offset,native_dword_count);
    // Native diagnostics reads the FIRST DWORD only, even if count == 0.
    // It also performs separate status-zero and optional registry paths.
    if(state.enabled_14!=0u) {
        const auto predicted_first=read_signed_dword(predicted,predicted_offset);
        const auto net_first=read_signed_dword(net,net_offset);
        binary::integer_diagnostics(
            state,operation,predicted_first,net_first,sink,symbols);
    }
    preview(state,predicted,predicted_offset,sink,symbols);
    return operation;
}

} // namespace sin_compat::original_client_snapshot_type5
