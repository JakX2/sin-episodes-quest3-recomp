#pragma once
// Phase 481: independently authored last VClient011::Init call-order oracle.
//
// Source-authenticated original 2006 client.dll VClient011::Init
// after the 12 direct AutoGameSystem registrations:
//   VA 0x240FE486..0x240FE5A3 (286 bytes, SHA pinned privately).
// This metadata models the *observable call ordering* and the TWO
// explicit bool-checked failure gates only. It does not reconstruct
// the unknown callees, engine-owned object pointers or normal Init.
// The production client.dll still returns 0 after diagnostic probing.
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_independent::client_init_tail {
inline constexpr std::uint32_t kTailBeginVA=0x240fe486u;
inline constexpr std::uint32_t kAutoSystemsGateVA=0x240fe4b3u;
inline constexpr std::uint32_t kPhysicsGateVA=0x240fe540u;
inline constexpr std::uint32_t kSharedFailureVA=0x240fe59eu;
inline constexpr std::uint32_t kOriginalSuccessReturnVA=0x240fe595u;
enum class Kind : std::uint8_t { VirtualCall, DirectCall };
enum class Gate : std::uint8_t {
    Unchecked, AutoGameSystemsInitialized, PhysicsInterfacesAcquired
};
struct Step final {
    std::uint32_t callsite_va;
    std::uint32_t direct_target_va; // 0 for virtual calls
    std::uint16_t virtual_slot_byte_offset; // 0 for direct calls
    Kind kind;
    Gate gate;
};
// Calls inside the bounded original tail, in precisely the
// authenticated order. The addresses are DATA only, never invoked.
inline constexpr std::array<Step,26> kOriginalTailCalls{{
    {0x240fe491u,0u,0x00,Kind::VirtualCall,Gate::Unchecked},
    {0x240fe49bu,0u,0x04,Kind::VirtualCall,Gate::Unchecked},
    {0x240fe4a3u,0x24144d30u,0,Kind::DirectCall,Gate::Unchecked},
    {0x240fe4b0u,0u,0x08,Kind::VirtualCall,Gate::Unchecked},
    {0x240fe4b3u,0x24156860u,0,Kind::DirectCall,Gate::AutoGameSystemsInitialized},
    {0x240fe4c8u,0u,0x14,Kind::VirtualCall,Gate::Unchecked},
    {0x240fe4e2u,0u,0x00,Kind::VirtualCall,Gate::Unchecked},
    {0x240fe4ecu,0u,0x00,Kind::VirtualCall,Gate::Unchecked},
    {0x240fe4eeu,0x24053a90u,0,Kind::DirectCall,Gate::Unchecked},
    {0x240fe4fbu,0u,0x00,Kind::VirtualCall,Gate::Unchecked},
    {0x240fe4fdu,0x241ee030u,0,Kind::DirectCall,Gate::Unchecked},
    {0x240fe502u,0x24085330u,0,Kind::DirectCall,Gate::Unchecked},
    {0x240fe507u,0x240f85d0u,0,Kind::DirectCall,Gate::Unchecked},
    {0x240fe50cu,0x241e5330u,0,Kind::DirectCall,Gate::Unchecked},
    {0x240fe512u,0x241cd170u,0,Kind::DirectCall,Gate::Unchecked},
    {0x240fe524u,0u,0x04,Kind::VirtualCall,Gate::Unchecked},
    {0x240fe529u,0x241e5c00u,0,Kind::DirectCall,Gate::Unchecked},
    {0x240fe538u,0u,0x04,Kind::VirtualCall,Gate::Unchecked},
    {0x240fe540u,0x24182c60u,0,Kind::DirectCall,Gate::PhysicsInterfacesAcquired},
    {0x240fe554u,0x241acf50u,0,Kind::DirectCall,Gate::Unchecked},
    {0x240fe560u,0u,0x24,Kind::VirtualCall,Gate::Unchecked},
    {0x240fe56bu,0x2418d1c0u,0,Kind::DirectCall,Gate::Unchecked},
    {0x240fe577u,0u,0x24,Kind::VirtualCall,Gate::Unchecked},
    {0x240fe581u,0x241dab30u,0,Kind::DirectCall,Gate::Unchecked},
    {0x240fe58du,0u,0x24,Kind::VirtualCall,Gate::Unchecked},
    {0x240fe590u,0x240fcdd0u,0,Kind::DirectCall,Gate::Unchecked},
}};
constexpr bool ValidMetadata() noexcept {
    if(kOriginalTailCalls.size()!=26)return false;
    std::size_t gateCount=0;
    for(std::size_t i=0;i<kOriginalTailCalls.size();++i) {
        const auto& x=kOriginalTailCalls[i];
        if(x.callsite_va<kTailBeginVA || x.callsite_va>=kOriginalSuccessReturnVA ||
           (i && x.callsite_va<=kOriginalTailCalls[i-1].callsite_va) ||
           (x.kind==Kind::DirectCall && x.direct_target_va==0) ||
           (x.kind==Kind::VirtualCall && x.direct_target_va!=0))
            return false;
        if(x.gate!=Gate::Unchecked){
            ++gateCount;
            if(x.kind!=Kind::DirectCall)return false;
        }
    }
    return gateCount==2 &&
           kOriginalTailCalls[4].callsite_va==kAutoSystemsGateVA &&
           kOriginalTailCalls[18].callsite_va==kPhysicsGateVA &&
           kOriginalTailCalls[25].callsite_va==0x240fe590u;
}
static_assert(ValidMetadata(),"source-authenticated client Init tail metadata");

using OwnedCallback = bool (*)(void*,const Step&) noexcept;
enum class Outcome {
    KnownOrderingCompleted, MissingInjectedCallback,
    AutoSystemGateRejected, PhysicsGateRejected
};
struct Result final {
    Outcome outcome=Outcome::MissingInjectedCallback;
    std::size_t calls=0;
    std::size_t checked_gates_passed=0;
    std::uint32_t last_callsite_va=0;
    std::uint32_t original_exit_va=0;
    bool accepted() const noexcept {
        return outcome==Outcome::KnownOrderingCompleted;
    }
};
// Callback is a test/port-owned substitute for each unknown engine call.
// Return values from original *unchecked* calls are ignored, matching
// original Init's visible branch behavior. Real side effects remain
// unknown and are NOT simulated or claimed by this oracle.
inline Result WalkKnownTail(OwnedCallback call,void* context) noexcept {
    Result result{};
    if(!call)return result;
    for(const auto& step:kOriginalTailCalls){
        const bool invoked=call(context,step);
        ++result.calls;
        result.last_callsite_va=step.callsite_va;
        if(step.gate==Gate::Unchecked)continue;
        if(!invoked){
            result.outcome=step.gate==Gate::AutoGameSystemsInitialized?
                Outcome::AutoSystemGateRejected:Outcome::PhysicsGateRejected;
            result.original_exit_va=kSharedFailureVA;
            return result;
        }
        ++result.checked_gates_passed;
    }
    // This is only the control-flow destination the ORIGINAL binary
    // would reach, IF all injected side effects were actually performed.
    result.original_exit_va=kOriginalSuccessReturnVA;
    result.outcome=Outcome::KnownOrderingCompleted;
    return result;
}
} // namespace sin_independent::client_init_tail
