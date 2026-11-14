#pragma once
// Phase220: clean-room host reference for two newly authenticated original
// type5 client.dll DWORD specialists, composing Phase213/217 helpers.
// No original engine type layout or Windows gameplay DLL is implied.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include "sin_compat/original_client_snapshot_field_copy.hpp"
#include "sin_compat/original_client_snapshot_binary_integer.hpp"

namespace sin_compat::original_client_snapshot_dword {
namespace copy=original_client_snapshot_field_copy;
namespace integer=original_client_snapshot_binary_integer;
using Bytes=std::vector<std::uint8_t>;

inline constexpr std::uint32_t kCompareVA=0x24197DB0u;
inline constexpr std::uint32_t kCopyVA=0x24198A10u; // Phase213
inline constexpr std::uint32_t kDiagnosticVA=0x24197B60u; // Phase217
inline constexpr std::uint32_t kPreviewVA=0x241983B0u;
inline constexpr std::uint32_t kCompareCaller=0x241999BCu;
inline constexpr std::uint32_t kCopyCaller=0x241999CBu;
inline constexpr std::uint32_t kDiagnosticCaller=0x241999D6u;
inline constexpr std::uint32_t kPreviewCaller=0x241999E5u;

struct OriginalFields {
    std::uint8_t enabled_14{};
    std::uint8_t descriptor_flags_13{};
    std::uint8_t copy_enabled_2c{};
    std::uint32_t descriptor_token_18{};
    std::uint32_t preview_token_34{};
};
inline std::uint32_t read_u32(const Bytes& bytes,std::size_t off) {
    if(off>bytes.size() || bytes.size()-off<4u)
        throw std::out_of_range("host-only native DWORD pointer bounds");
    return std::uint32_t(bytes[off]) |
           (std::uint32_t(bytes[off+1u])<<8u) |
           (std::uint32_t(bytes[off+2u])<<16u) |
           (std::uint32_t(bytes[off+3u])<<24u);
}
struct FirstMismatch {
    std::size_t index{};
    std::int32_t net{};
    std::int32_t predicted{};
    std::int32_t diff{};
};
struct IMismatchSink {
    virtual ~IMismatchSink()=default;
    // Original 0x24197DB0 calls 0x24197910 INSIDE the comparator, before
    // the router's separate Phase213 forward-copy function.
    virtual void first_mismatch(const FirstMismatch& value)=0;
};

// Original 125-byte function 0x24197DB0, RET 12.
// enabled=0 -> status0; descriptor flag0x04 -> status1;
// signed count<=0 -> status1; else 32-bit exact element comparison.
// On the FIRST different DWORD, emit exactly one original diagnostic
// with 32-bit wrapping predicted-minus-net and return 0.
inline std::uint32_t compare(const OriginalFields& state,
        const Bytes& predicted,std::size_t pred_off,
        const Bytes& net,std::size_t net_off,
        std::int32_t signed_count,IMismatchSink& sink) {
    if(state.enabled_14==0u)return 0u;
    if((state.descriptor_flags_13&0x04u)!=0u)return 1u;
    if(signed_count<=0)return 1u;
    for(std::size_t i=0;i<static_cast<std::size_t>(signed_count);++i) {
        if(i>(static_cast<std::size_t>(-1)/4u))
            throw std::overflow_error("host-only DWORD index overflow");
        const auto a=read_u32(predicted,pred_off+i*4u);
        const auto b=read_u32(net,net_off+i*4u);
        if(a!=b) {
            sink.first_mismatch({i,integer::signed_32(b),
                integer::signed_32(a),integer::signed_32(a-b)});
            return 0u;
        }
    }
    return 1u;
}

enum class PreviewKind { Numeric, Symbol };
struct PreviewEvent {
    PreviewKind kind{};
    std::int32_t predicted{};
    std::string symbol;
};
struct IPreviewSink {
    virtual ~IPreviewSink()=default;
    virtual void on_preview(const PreviewEvent& value)=0;
};

// Original 144-byte function 0x241983B0, RET 16, with NO padding
// before original Phase219 preview at 0x24198440.
// If full pointer-token identity differs, no pointer data is read.
// If descriptor bit 0x08 is set, the original tries the same global
// vtable lookup and Q_FileBase(512) used by Phase217. Missing lookup
// falls back to "integer (%i)"; otherwise "integer (%i->%s)".
// Actual resolver class and formatter are intentionally host-supplied.
inline void preview(const OriginalFields& state,const Bytes& predicted,
        std::size_t pred_off, IPreviewSink& sink,
        integer::ISymbolHost* symbols) {
    if(state.preview_token_34!=state.descriptor_token_18)return;
    const auto value=integer::signed_32(read_u32(predicted,pred_off));
    if((state.descriptor_flags_13&0x08u)!=0u) {
        if(!symbols)
            throw std::runtime_error("host-only unresolved original symbol registry");
        const auto name=symbols->find_name(value);
        if(name) {
            auto base=symbols->q_file_base(*name,512u);
            if(base.size()>=512u)
                throw std::length_error("host-only 512-byte basename buffer limit");
            sink.on_preview({PreviewKind::Symbol,value,base});
            return;
        }
    }
    sink.on_preview({PreviewKind::Numeric,value,{}});
}

// All four exact direct Phase212 type5 CALLs are now individually
// SHA-reviewed (two new functions plus pre-existing Phase213/217).
// Effects: comparison may emit a PRE-copy mismatch diagnostic; copy
// happens second; Phase217 integer diagnostics and this preview follow.
inline std::uint32_t run_type5_direct_calls(const OriginalFields& state,
        const Bytes& net,std::size_t net_off,
        Bytes& predicted,std::size_t pred_off,
        std::int32_t signed_count,
        IMismatchSink& mismatches,integer::ITraceSink& diagnostics,
        IPreviewSink& previews,integer::ISymbolHost* symbols) {
    const auto status=compare(state,predicted,pred_off,net,net_off,
                              signed_count,mismatches);
    const bool needs_copy=copy::should_copy({state.copy_enabled_2c},status);
    if(signed_count<0 && needs_copy)
        throw std::overflow_error("host-only negative DWORD count copy refusal");
    copy::copy_dwords_type5({state.copy_enabled_2c},status,
        net,net_off,predicted,pred_off,
        static_cast<std::uint32_t>(signed_count));
    // Phase217's original integer diagnostics reads the first whole DWORD,
    // and is called AFTER any native copy. No formatter is fabricated.
    const integer::OriginalFields int_state{
        state.enabled_14,state.descriptor_flags_13,state.copy_enabled_2c,
        state.descriptor_token_18,state.preview_token_34};
    if(state.enabled_14!=0u) {
        const auto pred=integer::signed_32(read_u32(predicted,pred_off));
        const auto original=integer::signed_32(read_u32(net,net_off));
        integer::integer_diagnostics(
            int_state,status,pred,original,diagnostics,symbols);
    }
    preview(state,predicted,pred_off,previews,symbols);
    return status;
}
} // namespace sin_compat::original_client_snapshot_dword
