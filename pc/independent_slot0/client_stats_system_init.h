#pragma once
// Authentic 2006 C_StatsSystem::Init (0x241FCAA0) call sequencing.
// Original function's complete 114 bytes are SHA256-pinned by an
// owner-only audit; the four internal helpers are not reconstructed.
// All addresses below are metadata, never callable from rebuilt DLL.
#include <array>
#include <cstddef>
#include <cstdint>
namespace sin_independent::stats_system {
enum class StepKind {
    InitializeStaticData, InitializeDynamicData, InitializeOwner,
    LoadDisplayLayout, RegisterCallback
};
struct Step {
    StepKind kind;
    std::uint32_t original_callsite;
    std::uint32_t original_callee;
    std::uint32_t original_this_offset;
    const char* argument;
    std::uint32_t original_argument_va;
    std::uint32_t original_callback_va;
};
inline constexpr std::array<Step,7> kInitSteps{{
    {StepKind::InitializeStaticData,0x241FCAA7u,0x241FF420u,0x04u,nullptr,0,0},
    {StepKind::InitializeDynamicData,0x241FCAB0u,0x241FF8E0u,0x70u,nullptr,0,0},
    {StepKind::InitializeOwner,0x241FCABAu,0x241FC5A0u,0x00u,nullptr,0,0},
    {StepKind::LoadDisplayLayout,0x241FCACAu,0x24204F30u,0x98u,
        "scripts/stats/stats_display_layout.txt",0x242AE008u,0},
    {StepKind::RegisterCallback,0x241FCADFu,0x241CFF70u,0,
        "StaticStat",0x242AE114u,0x241FB7A0u},
    {StepKind::RegisterCallback,0x241FCAF4u,0x241CFF70u,0,
        "DynamicStat",0x242AE108u,0x241FB7E0u},
    {StepKind::RegisterCallback,0x241FCB09u,0x241CFF70u,0,
        "ResetHUD",0x2429B894u,0x241FCA60u}
}};
using Hook = void (*)(void*,const Step&) noexcept;
// Four private original helper implementations are deliberately opaque.
// Return a boolean ONLY when a host deliberately supplies the correct
// ordered operations. Original function unconditionally returns true.
inline bool Init(void* system,void* registry,Hook hook) noexcept {
    if(!system || !registry || !hook)return false;
    for(const auto& step:kInitSteps)hook(
        step.kind==StepKind::RegisterCallback?registry:system,step);
    return true;
}
} // namespace sin_independent::stats_system
