#pragma once
// Phase223 independent C++17 host model for original 2006 client.dll
// type-4 quaternion fields. NO proprietary original code, and NO claim
// of complete original x87 stack-scratch or Source engine integration.
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

#include "sin_compat/original_client_snapshot_vector_copy.hpp"

namespace sin_compat::original_client_snapshot_quaternion {
using Bytes=std::vector<std::uint8_t>;
using Quaternion=std::array<float,4>;
namespace copy=original_client_snapshot_vector_copy;
inline constexpr std::uint32_t kCompareVA=0x24198F50u;
inline constexpr std::uint32_t kCopyVA=0x24198DE0u; // Phase221, not new
inline constexpr std::uint32_t kDiagnosticsVA=0x241987C0u;
inline constexpr std::uint32_t kPreviewVA=0x241989A0u;
inline constexpr std::uint32_t kCompareCall=0x241998ECu;
inline constexpr std::uint32_t kCopyCall=0x241998FDu;
inline constexpr std::uint32_t kDiagnosticsCall=0x24199908u;
inline constexpr std::uint32_t kPreviewCall=0x24199917u;

struct OriginalFields {
    std::uint8_t enabled_14{};
    std::uint8_t descriptor_flags_13{};
    std::uint8_t copy_enabled_2c{};
    float descriptor_tolerance_30{};
    std::uint32_t descriptor_token_18{};
    std::uint32_t preview_token_34{};
};
inline float f32(const Bytes& bytes,std::size_t off) {
    static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559,
                  "original quaternion uses IEEE754 binary32");
    if(off>bytes.size() || bytes.size()-off<4)
        throw std::out_of_range("host-only native float pointer guard");
    const std::uint32_t bits=std::uint32_t(bytes[off]) |
        (std::uint32_t(bytes[off+1])<<8u) |
        (std::uint32_t(bytes[off+2])<<16u) |
        (std::uint32_t(bytes[off+3])<<24u);
    float val{};
    std::memcpy(&val,&bits,sizeof(val));
    return val;
}
inline Quaternion read_quat(const Bytes& bytes,std::size_t off) {
    if(off>bytes.size() || bytes.size()-off<16)
        throw std::out_of_range("host-only native quaternion pointer guard");
    return {f32(bytes,off),f32(bytes,off+4u),
            f32(bytes,off+8u),f32(bytes,off+12u)};
}
inline float rounded_delta(float a,float b) {
    const volatile float result=static_cast<float>(
        static_cast<long double>(a)-static_cast<long double>(b));
    return result;
}

// An explicit seed for FOUR native local stack floats. Original 258-byte
// method reads slots [esp+0x18/0x1C/0x20/0x24], but on each nonexact
// record writes only [esp+0x18 + record_index*4], from the FOURTH float.
// Other slots were NOT initialized by the method; no safe default exists.
// Caller-provided values are a diagnostic model, not reconstructed x86
// stack-frame contents. For record_index >=4 the native write escapes
// the four slots: the portable host refuses to emulate that corruption.
struct ExplicitScratch {
    std::array<float,4> slots;
};
struct MissingOriginalScratch : std::runtime_error {
    MissingOriginalScratch()
        :std::runtime_error("original type4 x87 nonexact comparison reads uninitialized stack slots") {}
};

// Original FUCOMPP; FNSTSW AX; TEST AH,0x44; JP:
// ordered equal has only C3 (bit 0x40): odd parity, JP not taken;
// ordered unequal has neither: even parity, JP taken;
// unordered has BOTH C3/C2 (0x44): even parity, JP taken.
// With masked x87 invalid exceptions, NaN is NONEXACT.
inline bool native_ordered_equal(float a,float b) {
    return a==b;
}

// Original 258-byte method at 0x24198F50; returns 0, 1, 2.
// Exact/forced/disabled/empty paths are fully determined. For a
// nonexact record with positive tolerance, an explicit scratch model is
// REQUIRED; never silently substitute four componentwise deltas.
inline std::uint32_t compare(const OriginalFields& state,
        const Bytes& predicted,std::size_t pred_off,
        const Bytes& net,std::size_t net_off,
        std::int32_t record_count,ExplicitScratch* scratch=nullptr) {
    if(state.enabled_14==0u)return 0u;
    if((state.descriptor_flags_13&0x04u)!=0u)return 1u;
    if(record_count<=0)return 1u;
    std::uint32_t status=1u;
    for(std::size_t record=0;record<static_cast<std::size_t>(record_count);++record) {
        if(record>std::numeric_limits<std::size_t>::max()/16u)
            throw std::overflow_error("host-only native quaternion record overflow");
        const auto advance=record*16u;
        if(pred_off>std::numeric_limits<std::size_t>::max()-advance ||
           net_off>std::numeric_limits<std::size_t>::max()-advance)
            throw std::overflow_error("host-only original quaternion pointer overflow");
        const auto pred=read_quat(predicted,pred_off+advance);
        const auto src=read_quat(net,net_off+advance);
        bool nonexact=false;
        for(std::size_t axis=0;axis<4;axis++) {
            if(!native_ordered_equal(pred[axis],src[axis])) {
                nonexact=true;
                break;
            }
        }
        if(!nonexact)continue;
        // The native fourth-component subtraction and FSTP happens
        // BEFORE checking the strictly-positive tolerance.
        const float fourth_delta=rounded_delta(pred[3],src[3]);
        if(!(state.descriptor_tolerance_30>0.0f))return 0u;
        if(!scratch)throw MissingOriginalScratch{};
        if(record>=scratch->slots.size())
            throw std::overflow_error("host refuses original out-of-frame scratch write");
        scratch->slots[record]=fourth_delta;
        for(const float slot:scratch->slots) {
            if(!(std::fabs(slot)<=state.descriptor_tolerance_30))return 0u;
        }
        status=2u;
    }
    return status;
}

enum class TraceKind { Difference, Status, Preview };
struct TraceEvent {
    TraceKind kind{};
    std::uint32_t status{};
    Quaternion net{};
    Quaternion predicted{};
    float verified_fourth_component_delta{};
    bool native_format_argument_order_fully_reconstructed{};
};
struct ITraceSink {
    virtual ~ITraceSink()=default;
    virtual void on_event(const TraceEvent& event)=0;
};

// Original 406-byte diagnostic method. The enabled flag and status=0
// select the original "quaternion[] differs ..." formatter. Native stack
// setup is unusually complex; host only reports independently observed
// first-record values and the fourth-component subtraction, NOT guessed
// three other delta-format arguments. The generic status formatter follows.
inline void diagnostics(const OriginalFields& state,std::uint32_t status,
        const Bytes& predicted,std::size_t pred_off,
        const Bytes& net,std::size_t net_off,ITraceSink& sink) {
    if(state.enabled_14==0u)return;
    const auto pred=read_quat(predicted,pred_off);
    if(status==0u) {
        const auto src=read_quat(net,net_off);
        sink.on_event({TraceKind::Difference,status,src,pred,
                       rounded_delta(pred[3],src[3]),false});
    }
    sink.on_event({TraceKind::Status,status,{},pred,0.0f,false});
}

// Original 58-byte preview method at 0x241989A0, RET 16.
// Pointer-token identity, not enabled flag, decides whether to forward
// FOUR float values to the opaque original formatter.
inline void preview(const OriginalFields& state,
        const Bytes& predicted,std::size_t pred_off,ITraceSink& sink) {
    if(state.preview_token_34!=state.descriptor_token_18)return;
    sink.on_event({TraceKind::Preview,0u,{},read_quat(predicted,pred_off),0.0f,false});
}

// All four original direct type4 CALLs, preserving compare → Phase221
// 16-byte stride copy → diagnostics → preview. Host-only bounds guards.
// ExplicitScratch is needed only for nonexact positive-tolerance branch.
inline std::uint32_t run_direct_type4_calls(const OriginalFields& state,
        const Bytes& net,std::size_t net_off,
        Bytes& predicted,std::size_t pred_off,
        std::int32_t record_count,ITraceSink& sink,
        ExplicitScratch* scratch=nullptr) {
    const auto status=compare(state,predicted,pred_off,net,net_off,
                              record_count,scratch);
    if(record_count<0 &&
       original_client_snapshot_field_copy::should_copy(
           {state.copy_enabled_2c},status))
        throw std::overflow_error("host-only negative native quaternion copy count");
    copy::copy_type4({state.copy_enabled_2c},status,
        net,net_off,predicted,pred_off,static_cast<std::uint32_t>(record_count));
    diagnostics(state,status,predicted,pred_off,net,net_off,sink);
    preview(state,predicted,pred_off,sink);
    return status;
}

} // namespace sin_compat::original_client_snapshot_quaternion
