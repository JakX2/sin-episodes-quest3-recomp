// Phase1670: two original 2006 PE32 PostNetwork comparison helpers.
//
// The real game functions are SHA256 authenticated only in research.
// This file contains entirely NEW source built from observed ABI and
// field writes. Never loads or jumps into proprietary retail game code.
//
// 0x241978A0: __thiscall 10 stack DWORDs, RET40, initializes a
// 0x34-byte opaque comparison-argument object. Two flags derive
// from LOW BYTE of arg3 and arg5, not full DWORD truthiness.
//
// 0x24199BA0: __thiscall 3 stack DWORDs, RET12; increments a
// client global counter, optional preflight when arg3+0x10 BYTE=0,
// calls opaque core then opaque final, returns this+0x28 DWORD.
// Original opaque child routine behavior is NOT invented here.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sin_independent {
namespace original_postnetwork_compare_helpers {

constexpr std::uintptr_t kOriginalPrepareVa=0x241978A0u;
constexpr std::uintptr_t kOriginalWrapperVa=0x24199BA0u;
constexpr std::uintptr_t kOriginalCountGlobalVa=0x242FDB50u;
constexpr std::uintptr_t kOriginalPreflightVa=0x24197F50u;
constexpr std::uintptr_t kOriginalCoreVa=0x24199100u;
constexpr std::uintptr_t kOriginalFinalizeVa=0x24199B50u;
constexpr unsigned kPrepareArgumentDwords=10u;
constexpr unsigned kPrepareRetPopBytes=40u;
constexpr unsigned kWrapperArgumentDwords=3u;
constexpr unsigned kWrapperRetPopBytes=12u;
constexpr unsigned kPreparePayloadBytes=0x34u;
constexpr unsigned kInputPreflightFlagOffset=0x10u;
constexpr unsigned kWrapperResultOffset=0x28u;
static_assert(kPrepareArgumentDwords*4u==kPrepareRetPopBytes &&
              kWrapperArgumentDwords*4u==kWrapperRetPopBytes,
              "native MSVC x86 helper argument/stack cleanup drift");

// The original initializer is a NONVIRTUAL x86 member function.
// This fake object intentionally has no C++ vptr. Every payload
// byte is owned by the test. Unwritten padding remains untouched.
struct alignas(4) PreparePayload {
    std::array<std::uint8_t,kPreparePayloadBytes> raw{};

    std::uint32_t word(unsigned off) const noexcept {
        std::uint32_t value=0u;
        if(off+4u<=raw.size())
            std::memcpy(&value,raw.data()+off,4u);
        return value;
    }
    std::uint8_t byte(unsigned off) const noexcept {
        return off<raw.size()?raw[off]:0u;
    }
    void store_word(unsigned off,std::uint32_t value) noexcept {
        if(off+4u<=raw.size())
            std::memcpy(raw.data()+off,&value,4u);
    }

    // Exactly ten C++ stack DWORDs, ECX=this, __thiscall RET 0x28.
    __declspec(noinline) void __thiscall Initialize(
            std::uint32_t a1,std::uint32_t a2,
            std::uint32_t a3,std::uint32_t a4,
            std::uint32_t a5,std::uint32_t a6,
            std::uint32_t a7,std::uint32_t a8,
            std::uint32_t a9,std::uint32_t a10) noexcept {
        store_word(0x00u,a1);
        store_word(0x04u,a2);
        store_word(0x08u,a4);
        store_word(0x0Cu,(static_cast<std::uint8_t>(a3)!=0u)?1u:0u);
        store_word(0x10u,(static_cast<std::uint8_t>(a5)!=0u)?1u:0u);
        raw[0x14u]=static_cast<std::uint8_t>(a6);
        raw[0x15u]=static_cast<std::uint8_t>(a7);
        raw[0x16u]=static_cast<std::uint8_t>(a9);
        store_word(0x18u,0u);
        store_word(0x1Cu,0u);
        store_word(0x20u,0u);
        raw[0x24u]=0u;
        raw[0x25u]=0u;
        store_word(0x28u,0u);
        raw[0x2Cu]=static_cast<std::uint8_t>(a8);
        store_word(0x30u,a10);
    }
};
static_assert(sizeof(PreparePayload)==kPreparePayloadBytes,
              "original-free nonvirtual 0x34-byte argument layout changed");

struct ComparisonInput {
    std::array<std::uint8_t,kInputPreflightFlagOffset> opaque{};
    std::uint8_t preflight_suppressed=0u; // original input+0x10
};
static_assert(offsetof(ComparisonInput,preflight_suppressed)==
              kInputPreflightFlagOffset,
              "original preflight flag offset changed");

struct WrapperFixture;
struct WrapperOps {
    void* user=nullptr;
    std::uint32_t* counter=nullptr;
    void (*preflight)(void*,ComparisonInput*) noexcept=nullptr;
    void (*core)(void*,WrapperFixture*,std::uint32_t,std::uint32_t,
                 ComparisonInput*) noexcept=nullptr;
    void (*finalize)(void*,WrapperFixture*,std::uint32_t,
                     ComparisonInput*) noexcept=nullptr;
};
struct WrapperEvents {
    unsigned safe_refusals=0u;
    unsigned wrapper_calls=0u;
    unsigned conditional_preflights=0u;
    unsigned opaque_core_calls=0u;
    unsigned opaque_final_calls=0u;
};
inline WrapperEvents events{};

// The original wrapper returns [ECX+0x28] with RET 0x0C. The
// synthetic host callback registry lives AFTER the original
// 0x28 DWORD. The input is not an original C_BaseEntity.
struct alignas(4) WrapperFixture {
    std::array<std::uint8_t,kWrapperResultOffset> unknown{};
    std::uint32_t result_dword=0u;
    WrapperOps ops{};

    void set_ops(WrapperOps value) noexcept { ops=value; }
    void set_result(std::uint32_t v) noexcept {result_dword=v;}
    std::uintptr_t result_address() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&result_dword);
    }

    // Original code increments global counter before ANY child
    // callbacks, then only runs conditional preflight when
    // input+0x10 byte == 0. Core receives original a1, a2, input;
    // finalize receives incremented counter and input.
    // The safe synthetic stand-in refuses unowned null pointers
    // and missing opaque descendants before modifying anything.
    __declspec(noinline) std::uint32_t __thiscall CompareAndCount(
            std::uint32_t a1,std::uint32_t a2,
            ComparisonInput* input) noexcept {
        if(!input || !ops.counter || !ops.core || !ops.finalize ||
           (input->preflight_suppressed==0u && !ops.preflight)) {
            ++events.safe_refusals;
            return result_dword;
        }
        ++events.wrapper_calls;
        ++*ops.counter; // original unguarded wrapping DWORD increment
        if(input->preflight_suppressed==0u) {
            ++events.conditional_preflights;
            ops.preflight(ops.user,input);
        }
        ++events.opaque_core_calls;
        ops.core(ops.user,this,a1,a2,input);
        ++events.opaque_final_calls;
        ops.finalize(ops.user,this,*ops.counter,input);
        // Read AFTER finalize, as in original mov eax,[esi+0x28].
        return result_dword;
    }
};
static_assert(offsetof(WrapperFixture,result_dword)==
              kWrapperResultOffset,
              "original comparison wrapper result field +0x28 changed");

} // namespace original_postnetwork_compare_helpers
} // namespace sin_independent
