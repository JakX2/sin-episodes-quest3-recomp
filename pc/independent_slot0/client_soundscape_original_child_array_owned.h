#pragma once
// Three source-authenticated 2006 C_SoundscapeSystem child array
// destructor helpers, each EXACTLY 83 bytes (see auditor):
// +0x08/+0x1C child -> 0x240F3970
// +0xA0 child       -> 0x240F3A20
// +0xB4 child       -> 0x240F3AD0
//
// Reconstruct only their identical field-write/control-flow policy,
// not 2006 allocator calls, pointer arithmetic or engine object memory.
// Each helper checks source field +8 twice, zeros +C unconditionally,
// conditionally frees/zeros +0 and zeros +4, mirrors +0 to +10,
// then repeats its sentinel/ptr check. A valid +8 flag guarantees
// the second free is skipped in the modeled normal path.
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_independent::soundscape_original_child_arrays {
inline constexpr std::uint32_t kOriginalArrayAt08And1C=0x240F3970u;
inline constexpr std::uint32_t kOriginalArrayAtA0=0x240F3A20u;
inline constexpr std::uint32_t kOriginalArrayAtB4=0x240F3AD0u;
inline constexpr std::uint32_t kOriginalFree=0x2416F940u;
inline constexpr std::size_t kOriginalFunctionBytes=83u;
inline constexpr std::uint32_t kSentinel=0xFFFFFFFFu;
enum class Variant { Child08And1C, ChildA0, ChildB4 };
constexpr std::uint32_t OriginalVA(Variant v) noexcept {
    switch(v){
        case Variant::Child08And1C:return kOriginalArrayAt08And1C;
        case Variant::ChildA0:return kOriginalArrayAtA0;
        case Variant::ChildB4:return kOriginalArrayAtB4;
    }
    return 0u;
}
enum class Kind {
    CheckFlag8First,
    ZeroCountC,
    CheckPointer0First,
    FreePointer0First,
    ZeroPointer0First,
    ZeroCount4First,
    CheckFlag8Second,
    MirrorPointer0To10,
    CheckPointer0Second,
    ZeroCount4Second
};
struct Event final {
    Kind kind=Kind::CheckFlag8First;
    std::uint32_t offset=0u;
    std::uint32_t original_target=0u;
};
constexpr Event AsEvent(Kind k) noexcept {
    switch(k) {
        case Kind::CheckFlag8First:
        case Kind::CheckFlag8Second:return {k,8u,0u};
        case Kind::ZeroCountC:return {k,12u,0u};
        case Kind::CheckPointer0First:
        case Kind::CheckPointer0Second:
        case Kind::ZeroPointer0First:return {k,0u,0u};
        case Kind::FreePointer0First:return {k,0u,kOriginalFree};
        case Kind::ZeroCount4First:
        case Kind::ZeroCount4Second:return {k,4u,0u};
        case Kind::MirrorPointer0To10:return {k,16u,0u};
    }
    return {};
}
struct OwnedState final {
    std::uint32_t flag_8=kSentinel;
    bool pointer_0_present=false;
    bool mirror_10_present=false;
    std::uint32_t count_4=0u;
    std::uint32_t count_c=0u;
    std::uint64_t revision=0u;
};
struct Plan final {
    Variant variant=Variant::Child08And1C;
    std::uint64_t revision=0u;
    std::array<Event,10> actions{};
    std::size_t count=0u;
    unsigned original_allocator_free_count=0u;
};
enum class Result {Applied,Stale,Invalid};
class OwnedChildArray final {
public:
    const OwnedState& value() const noexcept {return state_;}
    void Set(const OwnedState& next) noexcept {
        const auto newRevision=state_.revision+1u;
        state_=next;
        state_.revision=newRevision;
    }
    void Reset() noexcept {
        const auto newRevision=state_.revision+1u;
        state_={};
        state_.revision=newRevision;
    }
    Plan Prepare(Variant variant) const noexcept {
        Plan out{};
        out.variant=variant;
        out.revision=state_.revision;
        auto add=[&out](Kind kind) noexcept {
            out.actions[out.count++]=AsEvent(kind);
        };
        add(Kind::CheckFlag8First);
        add(Kind::ZeroCountC);
        if(state_.flag_8!=kSentinel){
            add(Kind::CheckPointer0First);
            if(state_.pointer_0_present){
                add(Kind::FreePointer0First);
                add(Kind::ZeroPointer0First);
                ++out.original_allocator_free_count;
            }
            add(Kind::ZeroCount4First);
        }
        add(Kind::CheckFlag8Second);
        add(Kind::MirrorPointer0To10);
        if(state_.flag_8!=kSentinel){
            // Original reloads pointer after the first free/zero.
            // The pointer cannot remain non-null in a valid branch,
            // so the second allocator call is unreachable here.
            add(Kind::CheckPointer0Second);
            add(Kind::ZeroCount4Second);
        }
        return out;
    }
    Result SimulateComplete(const Plan& plan) noexcept {
        if(plan.revision!=state_.revision)return Result::Stale;
        if(OriginalVA(plan.variant)==0)return Result::Invalid;
        const Plan proof=Prepare(plan.variant);
        if(plan.count!=proof.count||
           plan.original_allocator_free_count!=proof.original_allocator_free_count)
            return Result::Invalid;
        for(std::size_t i=0;i<proof.count;++i){
            const auto& actual=plan.actions[i];
            const auto& expected=proof.actions[i];
            if(actual.kind!=expected.kind||actual.offset!=expected.offset||
               actual.original_target!=expected.original_target)
                return Result::Invalid;
        }
        // No original allocator is called: this mutates
        // independently owned *presence* and counter fields only.
        state_.count_c=0;
        if(state_.flag_8!=kSentinel){
            state_.pointer_0_present=false;
            state_.count_4=0;
        }
        state_.mirror_10_present=state_.pointer_0_present;
        ++state_.revision;
        return Result::Applied;
    }
private:
    OwnedState state_{};
};
static_assert(kOriginalFunctionBytes==83);
static_assert(OriginalVA(Variant::ChildA0)==0x240F3A20u);
} // namespace sin_independent::soundscape_original_child_arrays
