#pragma once
// Original 2006 C_DynamicDifficulty::Init exact registration sequence.
// Method at 0x241FAC90, 234 bytes: eleven registry insert calls, then
// "mov al,1; ret". The eleven callback addresses are original metadata,
// NOT callable pointers inside our independently rebuilt client.dll.
#include <array>
#include <cstdint>
#include <cstddef>
namespace sin_independent::dynamic_difficulty {
struct CallbackBinding {
    const char* name;
    std::uint32_t original_name_va;
    std::uint32_t original_callback_va;
    std::uint32_t original_callsite_va;
};
inline constexpr std::array<CallbackBinding,11> kInitBindings{{
    {"GamePlayVarValue",0x242ADFF4u,0x241F9960u,0x241FACA0u},
    {"Progression",0x242ADFE8u,0x241F9970u,0x241FACB5u},
    {"DDInfo",0x242ADFE0u,0x241F9F80u,0x241FACCAu},
    {"DDAdvisorChannels",0x242ADFCCu,0x241FAC40u,0x241FACDFu},
    {"DDAdvisorScores",0x242ADFBCu,0x241FAC50u,0x241FACF4u},
    {"DDDecisionMade",0x242ADFACu,0x241FAC60u,0x241FAD09u},
    {"DDDecisionsPerf",0x242ADF9Cu,0x241F9F90u,0x241FAD1Eu},
    {"DDSuccessModifiers",0x242ADF88u,0x241F9990u,0x241FAD33u},
    {"ArenaMode",0x242ADF7Cu,0x241F9FA0u,0x241FAD48u},
    {"NewGameInfo",0x242ADF70u,0x241F9FB0u,0x241FAD5Du},
    {"Cheating",0x242ADF64u,0x241F9FC0u,0x241FAD72u}
}};
using RegisterFn=void (*)(void* context,const CallbackBinding& binding) noexcept;
// The original stores no per-call result and always returns true.
// The missing registry object is an independent fail-closed condition.
inline bool Init(void* registry,RegisterFn registerCallback) noexcept {
    if(!registry || !registerCallback)return false;
    for(const auto& binding:kInitBindings)
        registerCallback(registry,binding);
    return true;
}
} // namespace sin_independent::dynamic_difficulty
