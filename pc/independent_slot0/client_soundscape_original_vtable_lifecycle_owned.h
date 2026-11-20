#pragma once
// Original 2006 C_SoundscapeSystem vtable wrappers at:
//   slot 2: 0x240F3DF0, complete 22-byte SHA256
//   f585618e3c7645a7729ed7b27d96f10ede980d57f439eb6672308c1b1db21331
//   slot 11: 0x240F3E20, complete 76-byte SHA256
//   a46d83727243a32da487b7794d25b7fee70ef1ac8969a3d266dc9042e878e189
//
// Both bodies authenticated read-only from the owner's SHA-pinned
// original client.dll. This does NOT implement arbitrary game
// virtual calls or invoke the retail allocator. Source order only.
// Vtable slot2 invokes original virtual slot1 then virtual slot0,
// followed by the tail call at 0x240F37D0.
// Slot11 resets vptr, tears down four child subobjects, calls
// base dtor 0x24156820 and optionally operator delete 0x2416FD60.
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_independent::soundscape_original_vtable_lifecycle {
inline constexpr std::uint32_t kSlot2VA=0x240F3DF0u;
inline constexpr std::size_t kSlot2Bytes=22u;
inline constexpr std::uint32_t kSlot2TailVA=0x240F37D0u;
inline constexpr std::uint32_t kSlot11VA=0x240F3E20u;
inline constexpr std::size_t kSlot11Bytes=76u;
inline constexpr std::uint32_t kOriginalVtableVA=0x24293C34u;
inline constexpr std::uint32_t kOriginalChildB4VA=0x240F3AD0u;
inline constexpr std::uint32_t kOriginalChildA0VA=0x240F3A20u;
inline constexpr std::uint32_t kOriginalChildArrayVA=0x240F3970u;
inline constexpr std::uint32_t kOriginalBaseDestroyVA=0x24156820u;
inline constexpr std::uint32_t kOriginalOperatorDeleteVA=0x2416FD60u;
enum class Kind {
    Slot2VirtualShutdown1, // CALL [vptr+0x04]
    Slot2VirtualInit0,     // CALL [vptr+0x00]
    Slot2TailCall0x240F37D0,
    Slot11RestoreVtable,
    Slot11DestroyChildB4,
    Slot11DestroyChildA0,
    Slot11DestroyChild1C,
    Slot11DestroyChild08,
    Slot11DestroyBase,
    Slot11OptionalDelete,
    Slot11ReturnThis,
};
struct Event final {
    Kind kind=Kind::Slot2VirtualShutdown1;
    std::uint32_t offset=0;
    std::uint32_t original_target=0;
};
enum class Operation { Slot2OrderedChain, Slot11DeletingDestructor };
struct Plan final {
    Operation operation=Operation::Slot2OrderedChain;
    std::uint64_t revision=0;
    bool source_delete_flag_bit0=false;
    std::array<Event,8> actions{};
    std::size_t count=0;
};
enum class Result {Applied,Stale,Invalid,AlreadyDestroyed};
struct KnownState final {
    std::uint32_t object_vtable=kOriginalVtableVA;
    bool destroyed=false;
    bool child_b4_destroyed=false;
    bool child_a0_destroyed=false;
    bool child_1c_destroyed=false;
    bool child_08_destroyed=false;
    bool base_destroyed=false;
    bool storage_deallocated=false;
    std::uint64_t slot2_completed=0;
    std::uint64_t revision=0;
};
inline constexpr Event EventFor(Kind kind) noexcept {
    switch(kind) {
        case Kind::Slot2VirtualShutdown1:return {kind,0x04u,0u};
        case Kind::Slot2VirtualInit0:return {kind,0x00u,0u};
        case Kind::Slot2TailCall0x240F37D0:
            return {kind,0u,kSlot2TailVA};
        case Kind::Slot11RestoreVtable:return {kind,0u,kOriginalVtableVA};
        case Kind::Slot11DestroyChildB4:return {kind,0xB4u,kOriginalChildB4VA};
        case Kind::Slot11DestroyChildA0:return {kind,0xA0u,kOriginalChildA0VA};
        case Kind::Slot11DestroyChild1C:return {kind,0x1Cu,kOriginalChildArrayVA};
        case Kind::Slot11DestroyChild08:return {kind,0x08u,kOriginalChildArrayVA};
        case Kind::Slot11DestroyBase:return {kind,0u,kOriginalBaseDestroyVA};
        case Kind::Slot11OptionalDelete:
            return {kind,0u,kOriginalOperatorDeleteVA};
        case Kind::Slot11ReturnThis:return {kind,0u,0u};
    }
    return {};
}
class OwnedLifecycle final {
public:
    const KnownState& value() const noexcept {return state_;}
    void Reset() noexcept {state_={};}
    Plan PrepareSlot2() const noexcept {
        Plan plan{};
        plan.operation=Operation::Slot2OrderedChain;
        plan.revision=state_.revision;
        plan.actions[plan.count++]=EventFor(Kind::Slot2VirtualShutdown1);
        plan.actions[plan.count++]=EventFor(Kind::Slot2VirtualInit0);
        plan.actions[plan.count++]=EventFor(Kind::Slot2TailCall0x240F37D0);
        return plan;
    }
    Plan PrepareSlot11(unsigned sourceFlags) const noexcept {
        Plan plan{};
        plan.operation=Operation::Slot11DeletingDestructor;
        plan.revision=state_.revision;
        plan.source_delete_flag_bit0=(sourceFlags&1u)!=0u;
        plan.actions[plan.count++]=EventFor(Kind::Slot11RestoreVtable);
        plan.actions[plan.count++]=EventFor(Kind::Slot11DestroyChildB4);
        plan.actions[plan.count++]=EventFor(Kind::Slot11DestroyChildA0);
        plan.actions[plan.count++]=EventFor(Kind::Slot11DestroyChild1C);
        plan.actions[plan.count++]=EventFor(Kind::Slot11DestroyChild08);
        plan.actions[plan.count++]=EventFor(Kind::Slot11DestroyBase);
        if(plan.source_delete_flag_bit0)
            plan.actions[plan.count++]=EventFor(Kind::Slot11OptionalDelete);
        plan.actions[plan.count++]=EventFor(Kind::Slot11ReturnThis);
        return plan;
    }
    Result SimulateComplete(const Plan& incoming) noexcept {
        if(incoming.revision!=state_.revision)return Result::Stale;
        if(state_.destroyed)return Result::AlreadyDestroyed;
        const auto expected=incoming.operation==Operation::Slot2OrderedChain?
            PrepareSlot2():
            PrepareSlot11(incoming.source_delete_flag_bit0?1u:0u);
        if(incoming.count!=expected.count||
           incoming.source_delete_flag_bit0!=expected.source_delete_flag_bit0)
            return Result::Invalid;
        for(std::size_t i=0;i<expected.count;++i) {
            const auto& a=incoming.actions[i];
            const auto& b=expected.actions[i];
            if(a.kind!=b.kind||a.offset!=b.offset||
               a.original_target!=b.original_target)return Result::Invalid;
        }
        // Only a synthetic state transition. We never call
        // original virtual functions, children, base or allocator.
        if(incoming.operation==Operation::Slot2OrderedChain)
            ++state_.slot2_completed;
        else {
            state_.object_vtable=kOriginalVtableVA;
            state_.child_b4_destroyed=true;
            state_.child_a0_destroyed=true;
            state_.child_1c_destroyed=true;
            state_.child_08_destroyed=true;
            state_.base_destroyed=true;
            state_.storage_deallocated=incoming.source_delete_flag_bit0;
            state_.destroyed=true;
        }
        ++state_.revision;
        return Result::Applied;
    }
private:
    KnownState state_{};
};
static_assert(kSlot2Bytes==22u);
static_assert(kSlot11Bytes==76u);
static_assert(EventFor(Kind::Slot11DestroyChild1C).offset==0x1Cu);
} // namespace sin_independent::soundscape_original_vtable_lifecycle
