#pragma once
// Phase 500: SHA-authenticated original 2006 soundscape auxiliary
// cleanup helper 0x240F3D10 (223 bytes; entire body SHA256
// 41ca09e9768fe5a683d57fdbe8ccac0e157ce557efc101e8e103aec3c804cfee).
//
// Original vtable slot5 at 0x240F3E10 is a 5-byte direct JMP
// to this helper. It separately handles two packed EHANDLE-like
// fields and clears/frees two pointer-backed arrays.
// This *owned model* never invokes original subobject vtable slot0,
// dereferences a retail handle table, or frees engine pointers.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_independent::soundscape_aux_cleanup {
inline constexpr std::uint32_t kOriginalHelperVA=0x240F3D10u;
inline constexpr std::size_t kOriginalHelperBytes=223u;
inline constexpr std::uint32_t kOriginalVtableSlot5VA=0x240F3E10u;
inline constexpr std::size_t kOriginalSlot5ThunkBytes=5u;
inline constexpr std::uint32_t kOriginalHandleTableGlobalVA=0x242FA734u;
inline constexpr std::uint32_t kOriginalFreeVA=0x2416F940u;
inline constexpr std::uint32_t kHandleIndexMask=0x00000FFFu;
inline constexpr unsigned kHandleGenerationShift=12u;
inline constexpr std::uint32_t kInvalid=0xFFFFFFFFu;
struct State final {
    std::uint32_t handle_9c=kInvalid;
    std::uint32_t handle_94=kInvalid;
    bool handle_9c_generation_matches=false;
    bool handle_9c_target_present=false;
    // The original reads +0xA8/+0xBC sentinel fields but does
    // not overwrite them. They are NOT array lengths.
    std::uint32_t vector_flag_a8=kInvalid;
    std::uint32_t vector_flag_bc=kInvalid;
    bool buffer_a0_present=false;
    bool buffer_b4_present=false;
    bool mirror_b0_present=false;
    bool mirror_c4_present=false;
    std::uint32_t count_a4=0;
    std::uint32_t count_ac=0;
    std::uint32_t count_b8=0;
    std::uint32_t count_c0=0;
};
enum class EventKind {
    ReleaseHandle9cViaEmbeddedVirtual0,
    ReleaseHandle94ViaEmbeddedVirtual0,
    ClearCountAc,
    FreeBufferA0Via0x2416F940,
    ClearBufferA0,
    ClearCountA4,
    MirrorB0FromA0,
    ClearCountC0,
    FreeBufferB4Via0x2416F940,
    ClearBufferB4,
    ClearCountB8,
    MirrorC4FromB4
};
struct Event final {
    EventKind kind=EventKind::ClearCountAc;
    std::uint32_t handle=0;
    bool expected_buffer_present=false;
};
struct Plan final {
    std::uint64_t source_revision=0;
    std::array<Event,12> actions{};
    std::size_t count=0;
    unsigned handle_releases=0;
    unsigned buffer_releases=0;
};
enum class Result {Ready,StalePlan,ModifiedPlan};
inline std::string_view ResultName(Result result) noexcept {
    switch(result){
        case Result::Ready:return "owned-soundscape-aux-cleanup-ready";
        case Result::StalePlan:return "owned-soundscape-aux-cleanup-stale";
        case Result::ModifiedPlan:return "owned-soundscape-aux-cleanup-forged";
    }
    return "unknown";
}
inline constexpr std::uint32_t HandleIndex(std::uint32_t raw) noexcept {
    return raw&kHandleIndexMask;
}
inline constexpr std::uint32_t HandleGeneration(std::uint32_t raw) noexcept {
    return raw>>kHandleGenerationShift;
}
class OwnedCleanup final {
public:
    const State& value() const noexcept{return state_;}
    std::uint64_t revision() const noexcept{return revision_;}
    void Set(const State& next) noexcept {state_=next;++revision_;}
    void Clear() noexcept {state_={};++revision_;}
    Plan Prepare() const noexcept {
        Plan plan{};
        plan.source_revision=revision_;
        const auto add=[&](Event e) noexcept{
            plan.actions[plan.count++]=e;
        };
        // Original checks handle+0x9C against -1, then generation
        // and target validity through its original global table.
        // The caller's independent flags model those *outcomes*,
        // never a real table or engine pointer.
        if(state_.handle_9c!=kInvalid &&
           state_.handle_9c_generation_matches &&
           state_.handle_9c_target_present){
            add({EventKind::ReleaseHandle9cViaEmbeddedVirtual0,
                 state_.handle_9c,false});
            ++plan.handle_releases;
        }
        // Unlike +0x9C, +0x94 tests only the -1 sentinel in
        // this original method before invoking the subobject.
        if(state_.handle_94!=kInvalid){
            add({EventKind::ReleaseHandle94ViaEmbeddedVirtual0,
                 state_.handle_94,false});
            ++plan.handle_releases;
        }
        add({EventKind::ClearCountAc});
        if(state_.vector_flag_a8!=kInvalid){
            if(state_.buffer_a0_present){
                add({EventKind::FreeBufferA0Via0x2416F940,0,true});
                ++plan.buffer_releases;
            }
            add({EventKind::ClearBufferA0,0,state_.buffer_a0_present});
            add({EventKind::ClearCountA4});
        }
        add({EventKind::MirrorB0FromA0,0,
             state_.vector_flag_a8==kInvalid&&state_.buffer_a0_present});
        add({EventKind::ClearCountC0});
        if(state_.vector_flag_bc!=kInvalid){
            if(state_.buffer_b4_present){
                add({EventKind::FreeBufferB4Via0x2416F940,0,true});
                ++plan.buffer_releases;
            }
            add({EventKind::ClearBufferB4,0,state_.buffer_b4_present});
            add({EventKind::ClearCountB8});
        }
        add({EventKind::MirrorC4FromB4,0,
             state_.vector_flag_bc==kInvalid&&state_.buffer_b4_present});
        return plan;
    }
    Result SimulateComplete(const Plan& claim) noexcept {
        if(claim.source_revision!=revision_)return Result::StalePlan;
        const Plan proof=Prepare();
        if(claim.count!=proof.count ||
           claim.handle_releases!=proof.handle_releases ||
           claim.buffer_releases!=proof.buffer_releases)
            return Result::ModifiedPlan;
        for(std::size_t i=0;i<proof.count;++i){
            const auto& a=claim.actions[i];
            const auto& b=proof.actions[i];
            if(a.kind!=b.kind||a.handle!=b.handle||
               a.expected_buffer_present!=b.expected_buffer_present)
                return Result::ModifiedPlan;
        }
        // Source ordering: conditional 9C invalidation, unconditional
        // (non-sentinel) 94 invalidation, global AC/C0 count clears,
        // optional array frees, and mirror pointers *after* frees.
        State next=state_;
        if(next.handle_9c!=kInvalid &&
           next.handle_9c_generation_matches &&
           next.handle_9c_target_present)
            next.handle_9c=kInvalid;
        if(next.handle_94!=kInvalid)next.handle_94=kInvalid;
        next.count_ac=0;
        if(next.vector_flag_a8!=kInvalid){
            next.buffer_a0_present=false;
            next.count_a4=0;
        }
        next.mirror_b0_present=next.buffer_a0_present;
        next.count_c0=0;
        if(next.vector_flag_bc!=kInvalid){
            next.buffer_b4_present=false;
            next.count_b8=0;
        }
        next.mirror_c4_present=next.buffer_b4_present;
        state_=next;
        ++revision_;
        return Result::Ready;
    }
private:
    State state_{};
    std::uint64_t revision_=0;
};
static_assert(HandleIndex(0xAABBCCDDu)==0xCDDu);
static_assert(HandleGeneration(0xAABBCCDDu)==0xAABBCu);
} // namespace sin_independent::soundscape_aux_cleanup
