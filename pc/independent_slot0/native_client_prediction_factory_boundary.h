// Phase1686: the ACTUAL independent client.dll VClientPrediction001
// factory boundary, not a separately compiled standalone fixture.
//
// Original retail engine.dll calls client CreateInterface with
// "VClientPrediction001", then invokes native __thiscall slot1
// directly. Until now the production client.dll refused the name.
// This native owner lives in the shipping independent client.dll,
// but it is ONLY registered when a session-scoped owner opt-in has
// completed a verified VEngineCvar002 cl_predict source acquisition.
//
// The published ABI is conservative: 23 owned MSVC x86 vtable
// entries, source-authenticated slot1/slot2/slot7/slot15-17.
// Every other slot terminates through the host's nonreturning
// unsupported-callback handler. The production boundary is NOT
// a fully implemented Source IPrediction or playable client.
//
// The original 23-slot method addresses, native layout offsets and
// engine factory/slot1 calls are SHA-authenticated in Phase1658.
// No original binary instructions, executable bytes, or pointers
// to unknown game objects appear here.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1686 production prediction boundary requires native MSVC x86
#endif

namespace sin_independent::native_client_prediction_factory {
constexpr char kInterfaceName[]="VClientPrediction001";
constexpr unsigned kOriginalSlots=23u;
constexpr unsigned kOriginalEnabledOffset=0x10u;
constexpr unsigned kOriginalHighestOffset=0x24u;
constexpr unsigned kOriginalCounterAOffset=0x28u;
constexpr unsigned kOriginalCounterBOffset=0x2Cu;
constexpr unsigned kOriginalAnyReconciledOffset=0x30u;
constexpr unsigned kOriginalGetter8Offset=0x08u;
constexpr unsigned kOriginalGetter9Offset=0x09u;
constexpr unsigned kOriginalGetterCOffset=0x0Cu;
constexpr unsigned kOriginalConVarParentOffset=0x18u;
constexpr unsigned kOriginalConVarNumericOffset=0x2Cu;
static_assert(sizeof(void*)==4u,"original VClientPrediction001 PE32 pointer ABI");

using RefuseCallback=void (*)(const char*) noexcept;
// The source slot0 is a scalar deleting destructor with RET4.
// An engine delete attempt must terminate rather than free our
// process-lifetime factory singleton or impersonate original
// ownership. This class is placement-constructed and never
// destroyed during module detach.
class Boundary {
public:
    explicit Boundary(RefuseCallback refuse) noexcept:
        refuse_(refuse) {}

    virtual ~Boundary() noexcept {
        Refuse("VClientPrediction001.slot0-delete");
    }
    // Real original slot1 16B: cl_predict.m_pParent numeric DWORD
    // +0x2C -> Boolean this+0x10, no stack args / RET0.
    // In this independent module, snapshot_ is a validated and
    // COPIED engine value, not a retained engine pointer.
    virtual void __thiscall InitializeFromEngine() noexcept {
        if(!bound_)Refuse("VClientPrediction001.slot1-unbound");
        enabled_=snapshot_!=0?1u:0u;
    }
    // Original slot2 is an empty RET0.
    virtual void __thiscall ShutdownNoop() noexcept {}
#define SIN_PRED_GUARD(n) \
    virtual void __thiscall GuardSlot##n() noexcept { \
        Refuse("VClientPrediction001.slot" #n "-unsupported"); \
    }
    SIN_PRED_GUARD(3)
    SIN_PRED_GUARD(4)
    SIN_PRED_GUARD(5)
    SIN_PRED_GUARD(6)
    // Original slot7 RET0 writes 0 to this+0x28 and +0x2C,
    // and -1 to this+0x24. No engine calls required.
    virtual void __thiscall ResetBookkeeping() noexcept {
        counter_a_=0u;
        counter_b_=0u;
        highest_=-1;
    }
    SIN_PRED_GUARD(8)
    SIN_PRED_GUARD(9)
    SIN_PRED_GUARD(10)
    SIN_PRED_GUARD(11)
    SIN_PRED_GUARD(12)
    SIN_PRED_GUARD(13)
    SIN_PRED_GUARD(14)
    // Source-authenticated complete 4-byte RET0 native getters.
    virtual std::uint8_t __thiscall GetMember8() noexcept {
        return member8_;
    }
    virtual std::uint8_t __thiscall GetMember9() noexcept {
        return member9_;
    }
    virtual std::uint32_t __thiscall GetMemberC() noexcept {
        return member_c_;
    }
    SIN_PRED_GUARD(18)
    SIN_PRED_GUARD(19)
    SIN_PRED_GUARD(20)
    SIN_PRED_GUARD(21)
    SIN_PRED_GUARD(22)
#undef SIN_PRED_GUARD

    // Only the *real production VClient011.Init* path may bind a
    // copied source ConVar value, once all 21 factory dependencies
    // have been acquired. No public game factory mutator is added.
    void BindSourceNumeric(std::int32_t numeric) noexcept {
        snapshot_=numeric;
        bound_=true;
        enabled_=0u; // slot1 still performs original cached update
        ResetBookkeeping();
    }
    void Revoke() noexcept {
        bound_=false;
        snapshot_=0;
        enabled_=0u;
        ResetBookkeeping();
    }
    bool Bound() const noexcept {return bound_;}

    // Test-visible only through the factory's actual memory
    // layout. Preserve source offsets without assuming a real
    // C_BasePlayer or Source engine-owned entity layout.
private:
    [[noreturn]] void Refuse(const char* method) noexcept {
        if(refuse_)refuse_(method);
        std::abort(); // supplied callback MUST NOT return
    }

    // MSVC native vfptr occupies +0. The source-owned fields
    // below occupy exactly the same named offsets as original.
    std::uint32_t original_4_=0u;
    std::uint8_t member8_=0u;
    std::uint8_t member9_=0u;
    std::uint16_t original_a_=0u;
    std::uint32_t member_c_=0u;
    std::uint8_t enabled_=0u;
    std::uint8_t original_11_13_[3]{};
    std::uint32_t original_14_=0u;
    std::uint32_t original_18_=0u;
    std::uint32_t original_1c_=0u;
    std::uint32_t original_20_=0u;
    std::int32_t highest_=-1;
    std::uint32_t counter_a_=0u;
    std::uint32_t counter_b_=0u;
    std::uint32_t any_reconciled_=0u;
    // Host-only fields start AFTER the source-observable 0x34B.
    std::int32_t snapshot_=0;
    bool bound_=false;
    RefuseCallback refuse_=nullptr;
};
} // namespace sin_independent::native_client_prediction_factory
