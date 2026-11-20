#pragma once
// Phase 480: owned CClientShadowMgr::Init known side-effect ordering.
//
// Original SHA-authenticated SiN client.dll CClientShadowMgr::Init
// 0x24118880..0x24118911 (146 bytes) and its 2006 virtual object
// 0x24358280 were audited privately. This independently authored
// scaffold reproduces only the known top-level ordered actions and
// two known state fields. Render/shadow objects, virtual callees,
// engine event registration and conditional helper stay callback-injected.
// No original address is invoked or pointer dereferenced.
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_independent::client_shadow_init {
inline constexpr std::uint32_t kOriginalInitVA=0x24118880u;
inline constexpr std::uint32_t kOriginalMember14ConstructorVA=0x24167810u;
inline constexpr std::uint32_t kOriginalConditionalHelperVA=0x24118610u;
inline constexpr std::uint32_t kOriginalEngineCallbackVA=0x24115e70u;
inline constexpr std::uint32_t kOriginalDirectionSlotBytes=0x60u;
inline constexpr std::uint32_t kOriginalLengthSlotBytes=0x6cu;
inline constexpr std::uint32_t kOriginalBiasSlotBytes=0x70u;
inline constexpr std::uint32_t kOriginalEngineRegistrationSlotBytes=0x1d0u;
inline constexpr std::uint32_t kOriginalMember14Tag1=0x24297d14u;
inline constexpr std::uint32_t kOriginalMember14Tag2=0x2427d384u;
inline constexpr std::array<std::uint32_t,3> kDirectionFloatBits{{
    0x3dcccccdu, 0x3dcccccdu, 0xbf800000u
}};
inline constexpr std::uint32_t kLengthFloatBits=0x42480000u; // 50.0f
inline constexpr std::uint32_t kBiasFloatBits=0x3ba3d70au;   // approx 0.005f
inline constexpr std::uint32_t kOriginalMode=8u;
inline constexpr std::uint32_t kOriginalFlagCDOffset=0xcdu;
inline constexpr std::uint32_t kOriginalModeD8Offset=0xd8u;

// Only the two conclusively located fields are modeled. This is NOT
// an original CClientShadowMgr instance, vtable or binary-compatible ABI.
struct OwnedFlags final {
    std::uint8_t shadow_initialized=1;  // original instance+0xcd
    std::uint32_t mode=0;               // original instance+0xd8
};
using InitMember14 = void (*)(void*,std::uint32_t,std::uint32_t) noexcept;
using SetDirection = void (*)(void*,const std::array<std::uint32_t,3>&) noexcept;
using SetFloatBits = void (*)(void*,std::uint32_t) noexcept;
using ConditionalSetup = void (*)(void*) noexcept;
using RegisterEvent = void (*)(void*,std::uint32_t) noexcept;
struct Hooks final {
    void* context=nullptr;
    InitMember14 initialize_member14=nullptr;
    SetDirection set_direction_slot60=nullptr;
    SetFloatBits set_length_slot6c=nullptr;
    SetFloatBits set_bias_slot70=nullptr;
    ConditionalSetup initialize_conditional_shadows=nullptr;
    RegisterEvent register_engine_callback_slot1d0=nullptr;
    bool original_external_shadow_flag=false;
};
enum class Result {KnownSequenceInvoked,MissingRequiredHooks};
inline const char* ResultName(Result result) noexcept {
    return result==Result::KnownSequenceInvoked?
        "original-known-shadow-init-sequence-invoked":
        "shadow-init-hook-preflight-refused";
}
inline Result InvokeKnownSequence(OwnedFlags& state,const Hooks& hooks) noexcept {
    // Original method unconditionally returns true after these calls,
    // but an independent owned simulation must fail preflight rather
    // than invent effects for missing renderer/engine callbacks.
    if(!hooks.initialize_member14 || !hooks.set_direction_slot60 ||
       !hooks.set_length_slot6c || !hooks.set_bias_slot70 ||
       !hooks.register_engine_callback_slot1d0 ||
       (hooks.original_external_shadow_flag &&
        !hooks.initialize_conditional_shadows))
        return Result::MissingRequiredHooks;

    state.shadow_initialized=0u;
    hooks.initialize_member14(
        hooks.context,kOriginalMember14Tag1,kOriginalMember14Tag2);
    hooks.set_direction_slot60(hooks.context,kDirectionFloatBits);
    hooks.set_length_slot6c(hooks.context,kLengthFloatBits);
    hooks.set_bias_slot70(hooks.context,kBiasFloatBits);
    state.mode=kOriginalMode;
    if(hooks.original_external_shadow_flag)
        hooks.initialize_conditional_shadows(hooks.context);
    hooks.register_engine_callback_slot1d0(
        hooks.context,kOriginalEngineCallbackVA);
    return Result::KnownSequenceInvoked;
}
} // namespace sin_independent::client_shadow_init
