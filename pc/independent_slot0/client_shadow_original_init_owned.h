#pragma once
// Phase 490: exact source-derived CClientShadowMgr::Init choreography.
// Original SiN Episodes 2006 SHA-pinned method 0x24118880, 146 bytes,
// SHA256 32dbbce9d4da9dce01522425c0531510640a5841a662c01abcec443df3d0d438.
// This is a pure owned EVENT CONTRACT: no engine virtual calls, renderer,
// callback registration, original material pointers or private DLL code.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_independent::shadow_original_init {
inline constexpr std::uint32_t kOriginalInitVA=0x24118880u;
inline constexpr std::size_t kOriginalInitBytes=146u;
inline constexpr std::uint32_t kOriginalVtableVA=0x24297C4Cu;
inline constexpr std::uint32_t kMaterialInitHelperVA=0x24167810u;
inline constexpr std::uint32_t kConditionalHelperVA=0x24118610u;
inline constexpr std::uint32_t kCallbackVA=0x24115E70u;
inline constexpr std::uint32_t kOriginalFieldCd=0xCDu;
inline constexpr std::uint32_t kOriginalFieldD8=0xD8u;
inline constexpr std::string_view kShadowMaterial="decals/simpleshadow";
inline constexpr std::string_view kShadowCategory="Decal textures";
inline constexpr std::uint32_t kVectorXBits=0x3DCCCCCDu; // 0.1f
inline constexpr std::uint32_t kVectorYBits=0x3DCCCCCDu; // 0.1f
inline constexpr std::uint32_t kVectorZBits=0xBF800000u; // -1.0f
inline constexpr std::uint32_t kSlot27FloatBits=0x42480000u; // 50.0f
inline constexpr std::uint32_t kSlot28FloatBits=0x3BA3D70Au; // 0.005f
inline constexpr std::uint32_t kFieldD8Value=8u;
inline constexpr std::size_t kOriginalVectorVirtualSlot=0x60u/4u;
inline constexpr std::size_t kOriginalSecondVirtualSlot=0x6Cu/4u;
inline constexpr std::size_t kOriginalThirdVirtualSlot=0x70u/4u;
inline constexpr std::size_t kOriginalCallbackVirtualSlot=0x1D0u/4u;
inline constexpr std::uint32_t kOriginalConditionalGlobal=0x243581C0u;
inline constexpr std::uint32_t kOriginalConditionOffset=0x2Cu;
inline constexpr std::uint32_t kOriginalCallbackGlobal=0x2433F69Cu;

enum class Operation {
    ClearObjectByteCd,
    InitializeOwnedMaterialRef,
    CallObjectVirtual24Vector,
    CallObjectVirtual27Float,
    CallObjectVirtual28Float,
    SetObjectDwordD8ToEight,
    ConditionalRefresh0x24118610,
    RegisterCallbackViaExternalVirtual116,
    OriginalReturnTrue
};
struct Action final {
    Operation operation=Operation::ClearObjectByteCd;
    std::uint32_t field_or_virtual=0;
    std::uint32_t arg0=0;
    std::uint32_t arg1=0;
    std::uint32_t arg2=0;
    std::string_view text0{};
    std::string_view text1{};
};
struct Plan final {
    std::array<Action,9> ordered{};
    std::size_t actions=0;
    bool optional_refresh_requested=false;
    bool original_returns_true=false;
};
constexpr bool ValidateEvidence() noexcept {
    return kOriginalInitVA==0x24118880u &&
           kOriginalInitBytes==146u &&
           kOriginalVectorVirtualSlot==24 &&
           kOriginalSecondVirtualSlot==27 &&
           kOriginalThirdVirtualSlot==28 &&
           kOriginalCallbackVirtualSlot==116 &&
           kVectorXBits==kVectorYBits &&
           kShadowMaterial=="decals/simpleshadow" &&
           kShadowCategory=="Decal textures";
}
static_assert(ValidateEvidence(),"Original 2006 shadow initialization evidence");
inline Plan MakePlan(bool external_condition_at_plus_2c) noexcept {
    Plan result{};
    auto append=[&](Action action) noexcept {
        result.ordered[result.actions++]=action;
    };
    // Original writes byte this+0xCD=0 and calls
    // 0x24167810 on the embedded material reference at this+0x14.
    append({Operation::ClearObjectByteCd,kOriginalFieldCd,0,0,0,{},{}});
    append({Operation::InitializeOwnedMaterialRef,0x14u,0,0,0,
            kShadowMaterial,kShadowCategory});
    // Three original THIS-vtable calls. Slot24 accepts an address
    // of three float32 values, slots27/28 each accept raw float bits.
    // These arguments are observed, but the virtual method names
    // and original implementation side effects are NOT invented.
    append({Operation::CallObjectVirtual24Vector,
            static_cast<std::uint32_t>(kOriginalVectorVirtualSlot),
            kVectorXBits,kVectorYBits,kVectorZBits,{},{}});
    append({Operation::CallObjectVirtual27Float,
            static_cast<std::uint32_t>(kOriginalSecondVirtualSlot),
            kSlot27FloatBits,0,0,{},{}});
    append({Operation::CallObjectVirtual28Float,
            static_cast<std::uint32_t>(kOriginalThirdVirtualSlot),
            kSlot28FloatBits,0,0,{},{}});
    append({Operation::SetObjectDwordD8ToEight,
            kOriginalFieldD8,kFieldD8Value,0,0,{},{}});
    // Original checks DWORD [[0x243581C0]+0x2C] before direct helper.
    if(external_condition_at_plus_2c)
        append({Operation::ConditionalRefresh0x24118610,
                kConditionalHelperVA,0,0,0,{},{}});
    // Original passes raw function VA 0x24115E70 into the method at
    // slot 116 of the external object stored at 0x2433F69C.
    append({Operation::RegisterCallbackViaExternalVirtual116,
            static_cast<std::uint32_t>(kOriginalCallbackVirtualSlot),
            kCallbackVA,0,0,{},{}});
    // Original sets AL=1 regardless of the return values of the
    // earlier direct/virtual calls, then releases stack and returns.
    append({Operation::OriginalReturnTrue,0,1,0,0,{},{}});
    result.optional_refresh_requested=external_condition_at_plus_2c;
    result.original_returns_true=true;
    return result;
}
struct OwnedModel final {
    std::uint8_t flag_cd=0xFFu;
    std::uint32_t value_d8=0;
    bool material_reference_described=false;
    bool vector_configuration_described=false;
    bool float27_described=false;
    bool float28_described=false;
    bool callback_registration_described=false;
    bool optional_refresh_described=false;
};
enum class Result {AppliedOwnedModel,PlanRefused};
inline Result ApplyPlanToOwnedModel(const Plan& plan,
                                  OwnedModel& output) noexcept {
    const Plan exact=MakePlan(plan.optional_refresh_requested);
    if(plan.actions!=exact.actions ||
       plan.original_returns_true!=exact.original_returns_true)
        return Result::PlanRefused;
    for(std::size_t i=0;i<exact.actions;++i){
        const auto& a=exact.ordered[i];
        const auto& b=plan.ordered[i];
        if(a.operation!=b.operation ||
           a.field_or_virtual!=b.field_or_virtual ||
           a.arg0!=b.arg0 ||a.arg1!=b.arg1 ||a.arg2!=b.arg2 ||
           a.text0!=b.text0 ||a.text1!=b.text1)
            return Result::PlanRefused;
    }
    OwnedModel next{};
    next.flag_cd=0;
    next.value_d8=kFieldD8Value;
    next.material_reference_described=true;
    next.vector_configuration_described=true;
    next.float27_described=true;
    next.float28_described=true;
    next.optional_refresh_described=plan.optional_refresh_requested;
    next.callback_registration_described=true;
    output=next;
    return Result::AppliedOwnedModel;
}
} // namespace sin_independent::shadow_original_init
