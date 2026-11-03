#pragma once
// Phase98: original 2006 SoundEmitterSystem.dll & vgui2.dll versus actual
// SourceVR 0.5.5 ARM64. Verified metadata; NOT a callable native adapter.
#include <array>
#include <cstdint>
#include <string_view>
namespace sin_compat::phase98 {
enum class ABIStatus : std::uint8_t {
  ProvenSlotMismatch, FunctionalCandidateNeedsABIVerification,
  ProvenBehaviorMismatch, MethodUnknown, ProvenABICompatible
};
struct SoundMapCandidate {
  std::string_view operation;
  std::uint32_t old_index, native_index, old_x86_va, native_aarch64_va;
  ABIStatus status;
};
inline constexpr std::array<SoundMapCandidate,2> sound_candidates{{
  {"name_to_token",3u,7u,0x2c00a830u,0x133a0u,ABIStatus::FunctionalCandidateNeedsABIVerification},
  {"token_to_name",6u,10u,0x2c008fc0u,0x13570u,ABIStatus::FunctionalCandidateNeedsABIVerification}
}};
inline constexpr std::uint32_t original_emitter_vtable=0x2c019c88u;
inline constexpr std::uint32_t original_input_vtable=0x100472acu;
inline constexpr std::uint32_t original_system_vtable=0x1004b95cu;
inline constexpr std::uint32_t native_emitter_vtable=0x41f98u;
inline constexpr std::uint32_t native_input_vtable=0x670c8u;
inline constexpr std::uint32_t native_system_vtable=0x67cb0u;
inline constexpr std::uint32_t invalid_token=0xffffffffu;
inline constexpr std::uint32_t largest_candidate_token=0xfffeu;
struct TokenResult {std::uint32_t token;bool valid;};
// This only normalizes the native slot-7 candidate's observed return shape.
// Do not assume token namespace or index identity matches the old engine.
constexpr TokenResult normalize_candidate(std::int32_t x) noexcept {
  if(x<0 || static_cast<std::uint32_t>(x)>largest_candidate_token)
    return {invalid_token,false};
  return {static_cast<std::uint32_t>(x),true};
}
constexpr bool candidate_token_range(std::uint32_t x) noexcept {
  return x<=largest_candidate_token;
}
// Proven by both REAL old engine and REAL native implementations:
inline constexpr ABIStatus vgui_input_slot56=ABIStatus::ProvenBehaviorMismatch;
inline constexpr ABIStatus vgui_system_slot24=ABIStatus::ProvenBehaviorMismatch;
inline constexpr std::uint32_t old_input_slot56_va=0x10007d90u;
inline constexpr std::uint32_t native_input_slot56_va=0x25ca0u;
inline constexpr std::uint32_t old_system_slot24_va=0x1001cb60u;
inline constexpr std::uint32_t native_system_slot24_va=0x3de30u;
constexpr bool native_call_permitted(ABIStatus status) noexcept {
  return status==ABIStatus::ProvenABICompatible;
}
static_assert(sound_candidates[0].native_index==7u);
static_assert(sound_candidates[1].native_index==10u);
static_assert(!native_call_permitted(sound_candidates[0].status));
static_assert(!native_call_permitted(vgui_input_slot56));
static_assert(!native_call_permitted(vgui_system_slot24));
} // namespace sin_compat::phase98
