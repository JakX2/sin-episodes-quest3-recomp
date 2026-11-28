// Phase1684: original-free native Win32 x86 reconstruction of the
// complete 126-byte client.dll entity reinitialization guard at
// 0x240CC2D0, called by 110-byte ReinitPredictables 0x24195C20.
//
// REAL original ABI: ECX=this, one C++ pointer stack argument,
// __thiscall RET4, not the parent's no-argument RET0.
// The original does not dereference the argument in this routine.
// Source reads parent cl_predict m_nValue, then this+0xC0 latch.
// When the latch is zero it tests *this+0xA4 nonzero (original
// entire 7B helper 0x241952A0), requires external virtual+0x30
// returned index MINUS ONE to equal ((*this+0xA4 >> 1)&31)
// (original entire 8B helper 0x241952D0), and sets latch=1.
// It then requires virtual+0x1A0 AL==0 and virtual+0x1BC AL!=0,
// checks this+0x454 DWORD==0, then calls still-opaque 0x240CAF40.
//
// All unknown engine pointers/virtual methods are injected
// host-owned callbacks. This never loads or runs retail DLL code.
// Safety-only missing-backend guards deliberately diverge from
// the original's unguarded pointer dereferences.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1684 requires genuine native MSVC x86
#endif
namespace sin_independent {
namespace original_prediction_entity_reinit_guard {

constexpr std::uintptr_t kOriginalMethodVa=0x240CC2D0u;
constexpr unsigned kOriginalNativeMethodBytes=126u;
constexpr unsigned kOriginalArgDwords=1u;
constexpr unsigned kOriginalRetPopBytes=4u;
constexpr std::uintptr_t kOriginalHasEncodedStateVa=0x241952A0u;
constexpr unsigned kOriginalHasEncodedStateBytes=7u;
constexpr std::uintptr_t kOriginalStateIndexVa=0x241952D0u;
constexpr unsigned kOriginalStateIndexBytes=8u;
constexpr std::uintptr_t kOriginalFinalOpaqueVa=0x240CAF40u;
constexpr std::uintptr_t kOriginalClPredictParentGlobal=0x24374540u;
constexpr std::uintptr_t kOriginalModeProviderGlobal=0x2433F684u;
constexpr unsigned kOriginalModeProviderVptrOffset=0x30u;
constexpr unsigned kOriginalNegativeVirtualOffset=0x1A0u;
constexpr unsigned kOriginalPositiveVirtualOffset=0x1BCu;
constexpr unsigned kOriginalStateDwordOffset=0xA4u;
constexpr unsigned kOriginalFlagByteOffset=0xC0u;
constexpr unsigned kOriginalFinalGateDwordOffset=0x454u;
constexpr unsigned kOriginalSourceStateIndexMask=0x1Fu;
static_assert(sizeof(void*)==4u &&
              kOriginalArgDwords*4u==kOriginalRetPopBytes,
              "native retail entity reinit calling convention drift");

struct Fixture;
struct Ops {
    void* user=nullptr;
    std::int32_t cl_predict_numeric=0;
    // Original separate global 0x2433F684 virtual+0x30.
    std::uint32_t (*mode_provider)(void*) noexcept=nullptr;
    // Source's unknown entity virtual call effects are NOT guessed.
    std::uint8_t (*predicate_virtual_1a0)(void*,Fixture*) noexcept=nullptr;
    std::uint8_t (*predicate_virtual_1bc)(void*,Fixture*) noexcept=nullptr;
    // Still-opaque original 0x240CAF40. No fake gameplay effects.
    void (*on_final_opaque)(void*,Fixture*) noexcept=nullptr;
};
struct Events {
    unsigned called=0u;
    unsigned cl_predict_off_skips=0u;
    unsigned state_zero_skips=0u;
    unsigned mode_provider_calls=0u;
    unsigned mode_index_mismatch_skips=0u;
    unsigned newly_latched=0u;
    unsigned previously_latched=0u;
    unsigned negative_virtual_calls=0u;
    unsigned negative_virtual_blocked=0u;
    unsigned positive_virtual_calls=0u;
    unsigned positive_virtual_blocked=0u;
    unsigned final_gate_blocked=0u;
    unsigned final_helper_calls=0u;
    unsigned missing_backend_refusals=0u;
};

// Nonvirtual native object retaining original +0xA4/+0xC0/+0x454
// at real offsets, with host-only Ops and Events AFTER +0x458.
// Original game object layout beyond these offsets isn't claimed.
struct alignas(4) Fixture {
    std::array<std::uint8_t,kOriginalStateDwordOffset> opaque_prefix{};
    std::uint32_t embedded_state=0u;
    std::array<std::uint8_t,
        kOriginalFlagByteOffset-kOriginalStateDwordOffset-4u> opaque_mid{};
    std::uint8_t latched_c0=0u;
    std::array<std::uint8_t,
        kOriginalFinalGateDwordOffset-kOriginalFlagByteOffset-1u>
        opaque_tail{};
    std::uint32_t final_gate_454=0u;
    Ops ops{};
    Events events{};

    // Exact real ABI: __thiscall ECX=this and one stack pointer,
    // native RET 4. Original does not read the reason argument.
    __declspec(noinline) void __thiscall ReinitEntity(
            const char* original_reason) noexcept {
        (void)original_reason;
        ++events.called;
        if(ops.cl_predict_numeric==0) {
            ++events.cl_predict_off_skips;
            return;
        }
        if(latched_c0==0u) {
            // Complete 7B source helper: bool(*DWORD(this+0xA4)).
            if(embedded_state==0u) {
                ++events.state_zero_skips;
                return;
            }
            if(!ops.mode_provider) {
                ++events.missing_backend_refusals;
                return;
            }
            ++events.mode_provider_calls;
            const auto external_index=ops.mode_provider(ops.user);
            // Complete 8B source helper: (state >> 1)&0x1F.
            const auto encoded_index=(embedded_state>>1u)&
                                     kOriginalSourceStateIndexMask;
            // Original SUB EBX,1 wraps modulo 2^32, not signed.
            if(external_index-1u!=encoded_index) {
                ++events.mode_index_mismatch_skips;
                return;
            }
            latched_c0=1u;
            ++events.newly_latched;
        } else {
            ++events.previously_latched;
        }
        if(!ops.predicate_virtual_1a0) {
            ++events.missing_backend_refusals;
            return;
        }
        ++events.negative_virtual_calls;
        if(ops.predicate_virtual_1a0(ops.user,this)!=0u) {
            ++events.negative_virtual_blocked;
            return;
        }
        if(!ops.predicate_virtual_1bc) {
            ++events.missing_backend_refusals;
            return;
        }
        ++events.positive_virtual_calls;
        if(ops.predicate_virtual_1bc(ops.user,this)==0u) {
            ++events.positive_virtual_blocked;
            return;
        }
        if(final_gate_454!=0u) {
            ++events.final_gate_blocked;
            return;
        }
        if(!ops.on_final_opaque) {
            ++events.missing_backend_refusals;
            return;
        }
        ++events.final_helper_calls;
        ops.on_final_opaque(ops.user,this);
    }
};
static_assert(offsetof(Fixture,embedded_state)==
              kOriginalStateDwordOffset &&
              offsetof(Fixture,latched_c0)==
              kOriginalFlagByteOffset &&
              offsetof(Fixture,final_gate_454)==
              kOriginalFinalGateDwordOffset,
              "synthetic source-owned entity layout shifted");
static_assert(offsetof(Fixture,ops)>=
              kOriginalFinalGateDwordOffset+sizeof(std::uint32_t),
              "host callbacks overlap authenticated retail fields");

} // namespace original_prediction_entity_reinit_guard
} // namespace sin_independent
