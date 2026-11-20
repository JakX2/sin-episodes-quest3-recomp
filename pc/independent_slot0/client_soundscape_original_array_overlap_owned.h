#pragma once
// Original SHA-pinned 2006 C_SoundscapeSystem source-owned field alias
// contract across 223B auxiliary cleanup (0x240F3D10) and the
// 83B child destructor helpers (0x240F3A20 at +A0,
// 0x240F3AD0 at +B4).
//
// These methods touch IDENTICAL parent fields through different
// this-relative bases. This adapter copies only independent DWORD
// flags/counters and pointer-PRESENCE booleans between owners,
// never actual retail allocator pointers or original objects.
#include "client_soundscape_original_aux_cleanup_owned.h"
#include "client_soundscape_original_child_array_owned.h"
#include "client_soundscape_original_object_ctor.h"
#include <cstdint>
namespace sin_independent::soundscape_original_array_overlap {
namespace aux=soundscape_aux_cleanup;
namespace child=soundscape_original_child_arrays;
namespace ctor=soundscape_original_object;

enum class Region { ArrayA0,ArrayB4 };
struct OriginalOffsets final {
    std::uint32_t base;
    std::uint32_t pointer;
    std::uint32_t count4;
    std::uint32_t flag8;
    std::uint32_t countC;
    std::uint32_t mirror10;
    child::Variant helper;
};
constexpr OriginalOffsets Offsets(Region r) noexcept {
    switch(r){
        case Region::ArrayA0:
            return {0xA0u,0xA0u,0xA4u,0xA8u,0xACu,0xB0u,
                    child::Variant::ChildA0};
        case Region::ArrayB4:
            return {0xB4u,0xB4u,0xB8u,0xBCu,0xC0u,0xC4u,
                    child::Variant::ChildB4};
    }
    return {0u,0u,0u,0u,0u,0u,
            child::Variant::Child08And1C};
}
static_assert(Offsets(Region::ArrayA0).flag8==0xA8u);
static_assert(Offsets(Region::ArrayA0).countC==0xACu);
static_assert(Offsets(Region::ArrayA0).mirror10==0xB0u);
static_assert(Offsets(Region::ArrayB4).flag8==0xBCu);
static_assert(Offsets(Region::ArrayB4).countC==0xC0u);
static_assert(Offsets(Region::ArrayB4).mirror10==0xC4u);
// Original 120B constructor zeroes both +A8 and +BC;
// its exact-known-DWORD model is separate from opaque storage.
constexpr bool ConstructorInitializesBothArrayFlagsToZero(
    const ctor::KnownDwordModel& constructed) noexcept {
    return constructed.At(0xA8u)==0u && constructed.At(0xBCu)==0u;
}
inline child::OwnedState Project(const aux::State& state,
                                 Region region) noexcept {
    child::OwnedState out{};
    if(region==Region::ArrayA0){
        out.flag_8=state.vector_flag_a8;
        out.pointer_0_present=state.buffer_a0_present;
        out.mirror_10_present=state.mirror_b0_present;
        out.count_4=state.count_a4;
        out.count_c=state.count_ac;
    }else{
        out.flag_8=state.vector_flag_bc;
        out.pointer_0_present=state.buffer_b4_present;
        out.mirror_10_present=state.mirror_c4_present;
        out.count_4=state.count_b8;
        out.count_c=state.count_c0;
    }
    return out;
}
inline aux::State ProjectBack(aux::State out,
                              const child::OwnedState& value,
                              Region region) noexcept {
    if(region==Region::ArrayA0){
        out.vector_flag_a8=value.flag_8;
        out.buffer_a0_present=value.pointer_0_present;
        out.mirror_b0_present=value.mirror_10_present;
        out.count_a4=value.count_4;
        out.count_ac=value.count_c;
    }else{
        out.vector_flag_bc=value.flag_8;
        out.buffer_b4_present=value.pointer_0_present;
        out.mirror_c4_present=value.mirror_10_present;
        out.count_b8=value.count_4;
        out.count_c0=value.count_c;
    }
    return out;
}
} // namespace sin_independent::soundscape_original_array_overlap
