#pragma once
// Phase216: independently authored portable reference for original 2006
// SiN client.dll snapshot type-1 float specialist chain. No original
// engine ownership or compatible drop-in game module is claimed.
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

#include "sin_compat/original_client_snapshot_field_copy.hpp"

namespace sin_compat::original_client_snapshot_type1 {

inline constexpr std::uint32_t kComparisonVA=0x24198AA0u;
inline constexpr std::uint32_t kDiagnosticsVA=0x24197CA0u;
inline constexpr std::uint32_t kPreviewVA=0x24198470u;

struct OriginalFields {
    std::uint8_t enabled_14{};         // Native this+0x14 BYTE
    std::uint8_t descriptor_flags_13{}; // Native this+0x18 -> descriptor+0x13
    std::uint8_t copy_enabled_2c{};    // Distinct Phase213 copy gate
    float descriptor_tolerance_30{};   // Native descriptor+0x30 float
    std::uint32_t descriptor_token_18{}; // Native this+0x18 DWORD identity
    std::uint32_t preview_token_34{}; // Native this+0x34 DWORD identity
};
using Bytes=std::vector<std::uint8_t>;

inline float load_f32_le(const Bytes& bytes, std::size_t offset) {
    static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559,
                  "original x87 input requires IEEE754 binary32");
    if(offset>bytes.size() || bytes.size()-offset<4u)
        throw std::out_of_range("host-only native float input bounds");
    const std::uint32_t bits=
        std::uint32_t(bytes[offset]) |
        (std::uint32_t(bytes[offset+1u])<<8u) |
        (std::uint32_t(bytes[offset+2u])<<16u) |
        (std::uint32_t(bytes[offset+3u])<<24u);
    float value{};
    std::memcpy(&value,&bits,sizeof(value));
    return value;
}

// Full 152-byte original comparator at 0x24198AA0.
// Status 0: disabled or unequal beyond tolerance.
// Status 1: exact numeric equality, forced descriptor gate, or count<=0.
// Status 2: at least one different float, but every mismatch is within
// a strictly-positive descriptor tolerance. NaNs/unordered never pass.
// x87 loads two binary32 inputs as extended, subtracts and fabs before
// comparing with binary32 tolerance. A binary64 subtraction of two finite
// binary32 inputs has enough precision for the same comparison outcome.
inline std::uint32_t compare_status(const OriginalFields& fields,
                                    const Bytes& predicted,std::size_t predicted_off,
                                    const Bytes& net,std::size_t net_off,
                                    std::int32_t count) {
    if(fields.enabled_14==0u) return 0u;
    if((fields.descriptor_flags_13 & 0x04u)!=0u) return 1u;
    if(count<=0) return 1u;
    const std::size_t n=static_cast<std::size_t>(count);
    if(predicted_off>predicted.size() || net_off>net.size() ||
       n>(predicted.size()-predicted_off)/4u ||
       n>(net.size()-net_off)/4u)
        throw std::out_of_range("host-only native float-array count bounds");
    const bool tolerance_enabled=fields.descriptor_tolerance_30>0.0f;
    std::uint32_t status=1u;
    for(std::size_t i=0;i<n;++i) {
        const float pred=load_f32_le(predicted,predicted_off+i*4u);
        const float source=load_f32_le(net,net_off+i*4u);
        if(pred==source) continue;
        if(!tolerance_enabled) return 0u;
        const double delta=std::fabs(static_cast<double>(pred)-
                                     static_cast<double>(source));
        // An unordered x87 comparison (NaN) returns mismatch, not status 2.
        if(!(delta<=static_cast<double>(fields.descriptor_tolerance_30)))
            return 0u;
        status=2u;
    }
    return status;
}

enum class TraceKind { Difference, Status, Preview };
struct TraceEvent {
    TraceKind kind{};
    std::uint32_t operation{};
    double net{};
    double predicted{};
    double predicted_minus_net{};
};
struct ITraceSink {
    virtual ~ITraceSink()=default;
    virtual void on_event(const TraceEvent& event)=0;
};

// Original 92-byte diagnostics method 0x24197CA0, native RET 16.
// If enabled and operation==0, formats "float differs (net %f pred %f)
// diff(%f)\\n"; then always formats "float (%f)\\n" with the prediction
// value. These are callbacks, NOT recovered tier0/engine console ownership.
inline void diagnostics(const OriginalFields& fields,std::uint32_t operation,
                        const Bytes& predicted,std::size_t predicted_off,
                        const Bytes& net,std::size_t net_off,
                        ITraceSink& sink) {
    if(fields.enabled_14==0u) return;
    if(operation==0u) {
        const double predicted_value=load_f32_le(predicted,predicted_off);
        const double net_value=load_f32_le(net,net_off);
        sink.on_event({TraceKind::Difference,operation,net_value,
                       predicted_value,predicted_value-net_value});
    }
    const double predicted_value=load_f32_le(predicted,predicted_off);
    sink.on_event({TraceKind::Status,operation,0.0,
                   predicted_value,0.0});
}

// Original 37-byte preview method 0x24198470, native RET 16.
// Conditional on DWORD(this+0x34)==DWORD(this+0x18), not enabled_14.
inline void preview(const OriginalFields& fields,
                    const Bytes& predicted,std::size_t predicted_off,
                    ITraceSink& sink) {
    if(fields.preview_token_34!=fields.descriptor_token_18)return;
    const double value=load_f32_le(predicted,predicted_off);
    sink.on_event({TraceKind::Preview,0u,0.0,value,0.0});
}

// Composes the Phase212 case-1 original direct CALL ORDER:
// compare -> Phase213 DWORD copy -> diagnostics -> preview.
// This is a host-only reference, not the whole serializer, engine
// allocator, data-map, writer mode or native object layout.
inline std::uint32_t run_direct_type1_calls(
        const OriginalFields& fields,const Bytes& net,std::size_t net_off,
        Bytes& predicted,std::size_t predicted_off,
        std::int32_t count,ITraceSink& sink) {
    if(count<0)
        throw std::invalid_argument("host-only refusal of negative DWORD copy count");
    const auto status=compare_status(
        fields,predicted,predicted_off,net,net_off,count);
    original_client_snapshot_field_copy::copy_dwords_type1(
        {fields.copy_enabled_2c},status,net,net_off,
        predicted,predicted_off,static_cast<std::uint32_t>(count));
    diagnostics(fields,status,predicted,predicted_off,net,net_off,sink);
    preview(fields,predicted,predicted_off,sink);
    return status;
}

} // namespace sin_compat::original_client_snapshot_type1
