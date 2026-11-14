#pragma once
// Phase222: independent C++17 reference for original 2006 SiN client.dll
// snapshot type-3 vec3 x87 compare, diagnostics, and preview. No original
// proprietary code or claim of a complete gameplay/engine ABI is included.
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

#include "sin_compat/original_client_snapshot_vector_copy.hpp"

namespace sin_compat::original_client_snapshot_vec3 {
using Bytes=std::vector<std::uint8_t>;
using Vec3=std::array<float,3>;
namespace vector_copy=original_client_snapshot_vector_copy;
inline constexpr std::uint32_t kCompareVA=0x24198E20u;
inline constexpr std::uint32_t kCopyVA=0x24198DA0u; // Phase221: already counted
inline constexpr std::uint32_t kDiagnosticsVA=0x241985A0u;
inline constexpr std::uint32_t kPreviewVA=0x241986C0u;
inline constexpr std::uint32_t kCompareCall=0x241998B2u;
inline constexpr std::uint32_t kCopyCall=0x241998C3u;
inline constexpr std::uint32_t kDiagnosticsCall=0x241998CEu;
inline constexpr std::uint32_t kPreviewCall=0x241998DDu;

struct OriginalFields {
    std::uint8_t enabled_14{};
    std::uint8_t descriptor_flags_13{};
    std::uint8_t copy_enabled_2c{};
    float descriptor_tolerance_30{};
    std::uint32_t descriptor_token_18{};
    std::uint32_t preview_token_34{};
};

inline float load_f32_le(const Bytes& bytes,std::size_t off) {
    static_assert(sizeof(float)==4u && std::numeric_limits<float>::is_iec559,
                  "2006 x87 client uses IEEE754 binary32 fields");
    if(off>bytes.size() || bytes.size()-off<4u)
        throw std::out_of_range("host-only original vec3 float pointer bounds");
    const std::uint32_t bits=std::uint32_t(bytes[off]) |
        (std::uint32_t(bytes[off+1u])<<8u) |
        (std::uint32_t(bytes[off+2u])<<16u) |
        (std::uint32_t(bytes[off+3u])<<24u);
    float value{};
    std::memcpy(&value,&bits,sizeof(value));
    return value;
}
inline Vec3 read_vec3(const Bytes& bytes,std::size_t off) {
    if(off>bytes.size() || bytes.size()-off<12u)
        throw std::out_of_range("host-only original 12-byte vec3 bounds");
    return {load_f32_le(bytes,off),load_f32_le(bytes,off+4u),
            load_f32_le(bytes,off+8u)};
}

// Native x87 subtracts binary32 fields in its register precision then
// FSTP DWORD rounds each component to a binary32 temporary BEFORE the
// tolerance test. Long double models extended arithmetic on platforms
// where supported; exact x87 control-word/trap behavior is not emulated.
inline float single_rounded_delta(float predicted,float net) {
    const volatile float rounded=static_cast<float>(
        static_cast<long double>(predicted)-static_cast<long double>(net));
    return rounded;
}
inline Vec3 delta_pred_minus_net(const Vec3& predicted,const Vec3& net) {
    return {single_rounded_delta(predicted[0],net[0]),
            single_rounded_delta(predicted[1],net[1]),
            single_rounded_delta(predicted[2],net[2])};
}

// Complete 289-byte original comparator at 0x24198E20, RET 12.
// Return 0 if disabled, or a vec3 difference fails tolerance.
// Return 1 if forced descriptor bit 4, count <= 0, or all vectors
// are numerically equal. Return 2 if at least one vector differs,
// but all three of its single-rounded delta components are within
// an ordered strictly-positive tolerance. FUCOMPP treats NaN as
// unordered/nonexact; +0/-0 and equal infinities are exactly equal.
inline std::uint32_t compare(const OriginalFields& state,
        const Bytes& predicted,std::size_t pred_off,
        const Bytes& net,std::size_t net_off,
        std::int32_t signed_record_count) {
    if(state.enabled_14==0u)return 0u;
    if((state.descriptor_flags_13&0x04u)!=0u)return 1u;
    if(signed_record_count<=0)return 1u;
    std::uint32_t status=1u;
    for(std::size_t n=0;n<static_cast<std::size_t>(signed_record_count);++n) {
        if(n>std::numeric_limits<std::size_t>::max()/12u)
            throw std::overflow_error("host-only original vec3 record offset overflow");
        const auto advance=n*12u;
        if(pred_off>std::numeric_limits<std::size_t>::max()-advance ||
           net_off>std::numeric_limits<std::size_t>::max()-advance)
            throw std::overflow_error("host-only original vec3 base offset overflow");
        const Vec3 pred=read_vec3(predicted,pred_off+advance);
        const Vec3 source=read_vec3(net,net_off+advance);
        if(pred[0]==source[0] && pred[1]==source[1] &&
           pred[2]==source[2])continue;
        if(!(state.descriptor_tolerance_30>0.0f))return 0u;
        const Vec3 differences=delta_pred_minus_net(pred,source);
        for(const auto value:differences) {
            if(!(std::fabs(value)<=state.descriptor_tolerance_30))
                return 0u;
        }
        status=2u;
    }
    return status;
}

enum class TraceKind { Difference, Status, Preview };
struct TraceEvent {
    TraceKind kind{};
    std::uint32_t status{};
    Vec3 net{};
    Vec3 predicted{};
    Vec3 delta_pred_minus_net{};
};
struct ITraceSink {
    virtual ~ITraceSink()=default;
    virtual void on_event(const TraceEvent& event)=0;
};

// Original 219-byte diagnostic helper 0x241985A0, RET 16.
// Enabled + status0: native formatter reports the FIRST three net/pred
// values and three single-rounded predicted-minus-net deltas.
// Every enabled status emits "vector (%f %f %f)\n" from the first
// destination/predicted vec3, after the optional native copy.
inline void diagnostics(const OriginalFields& state,std::uint32_t status,
        const Bytes& predicted,std::size_t pred_off,
        const Bytes& net,std::size_t net_off,ITraceSink& sink) {
    if(state.enabled_14==0u)return;
    const Vec3 pred=read_vec3(predicted,pred_off);
    if(status==0u) {
        const Vec3 source=read_vec3(net,net_off);
        sink.on_event({TraceKind::Difference,status,source,pred,
                       delta_pred_minus_net(pred,source)});
    }
    sink.on_event({TraceKind::Status,status,{},pred,{}});
}

// Original 51-byte preview 0x241986C0, RET 16, does NOT read enabled_14.
// Pointer-token equality gates "vector (%f %f %f)" preview.
inline void preview(const OriginalFields& state,
        const Bytes& predicted,std::size_t pred_off,ITraceSink& sink) {
    if(state.preview_token_34!=state.descriptor_token_18)return;
    sink.on_event({TraceKind::Preview,0u,{},
                   read_vec3(predicted,pred_off),{}});
}

// Original Phase212 type-3 four-direct-CALL order:
// 0x24198E20 compare -> Phase221 0x24198DA0 forward copy ->
// 0x241985A0 diagnostics -> 0x241986C0 preview.
// Signed count is used by comparison, but original copy uses an
// UNSIGNED record count; host refuses dangerous negative-count copies.
inline std::uint32_t run_type3_direct_calls(const OriginalFields& state,
        const Bytes& net,std::size_t net_off,
        Bytes& predicted,std::size_t pred_off,
        std::int32_t signed_record_count,ITraceSink& sink) {
    const auto status=compare(state,predicted,pred_off,
                              net,net_off,signed_record_count);
    if(signed_record_count<0 &&
       original_client_snapshot_field_copy::should_copy(
           {state.copy_enabled_2c},status))
        throw std::overflow_error("host-only negative native vec3 copy count");
    vector_copy::copy_type3({state.copy_enabled_2c},status,
        net,net_off,predicted,pred_off,
        static_cast<std::uint32_t>(signed_record_count));
    diagnostics(state,status,predicted,pred_off,net,net_off,sink);
    preview(state,predicted,pred_off,sink);
    return status;
}

} // namespace sin_compat::original_client_snapshot_vec3
