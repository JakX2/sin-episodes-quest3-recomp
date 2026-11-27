// Phase 1658: original-free native Win32 x86 prediction slot1 ABI fixture.
// SHA-matched retail client.dll slot1 (0x24195F80) has NO stack arguments,
// reads cl_predict's m_pParent pointer from 0x24374540
// (cl_predict ConVar object 0x24374528 + 0x18), tests that parent's
// numeric ConVar m_nValue DWORD at +0x2C, and writes prediction
// this+0x10 = (nonzero?1:0), RET 0.
//
// The original does not check whether the ConVar parent pointer is null.
// The independently owned diagnostic model refuses a missing ConVar view.
// This fixture is NOT registered in CreateInterface or an original engine.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1658 prediction test requires native MSVC Windows x86
#endif

namespace sin_independent {
namespace client_prediction23_diagnostic {

constexpr std::uintptr_t kOriginalSingletonRva=0x37FA00u;
constexpr std::uintptr_t kOriginalVtableRva=0x2A363Cu;
constexpr std::uintptr_t kOriginalInitMethodRva=0x195F80u;
constexpr std::uintptr_t kOriginalClPredictConVarRva=0x374528u;
constexpr std::uintptr_t kOriginalClPredictParentSlotRva=0x374540u;
constexpr unsigned kOriginalConVarParentFieldOffset=0x18u;
constexpr unsigned kOriginalConVarNumericValueOffset=0x2Cu;
constexpr unsigned kOriginalPredictionEnabledOffset=0x10u;
constexpr unsigned kOriginalShutdownSlot=2u;
constexpr unsigned kOriginalResetSlot=7u;
constexpr unsigned kOriginalResetIndexOffset=0x24u;
constexpr unsigned kOriginalResetCountAOffset=0x28u;
constexpr unsigned kOriginalResetCountBOffset=0x2Cu;
constexpr unsigned kOriginalVirtualCount=23u;

// Only invented ConVar storage is accepted. Original ConVar constructor
// sets this+0x18 (m_pParent) to self; the integer m_nValue is +0x2C.
struct ConVarView {
    std::array<std::uint8_t,0x18u> opaque_prefix{};
    const ConVarView* parent=nullptr;
    std::array<std::uint8_t,0x10u> opaque_middle{};
    std::int32_t numeric_value=0;
};
static_assert(offsetof(ConVarView,parent)==0x18u,
              "synthetic cl_predict ConVar parent pointer offset drift");
static_assert(offsetof(ConVarView,numeric_value)==0x2Cu,
              "synthetic cl_predict integer m_nValue offset drift");
static_assert(kOriginalClPredictConVarRva+
              kOriginalConVarParentFieldOffset==
              kOriginalClPredictParentSlotRva,
              "original cl_predict pointer RVA and parent offset drift");

struct Events {
    unsigned init_calls=0u;
    unsigned missing_provider_refusals=0u;
    unsigned destructor_calls=0u;
};
inline Events events{};

// MSVC slot0 deletes, slot1 Init copies cl_predict effective numeric
// value, slot2 on engine shutdown is an empty RET and slot7 resets three
// prediction state DWORDs. Other slots remain inert sentinels. This
// test fixture is not a registerable implementation of IPrediction.
class Prediction23Fixture {
public:
    explicit Prediction23Fixture(const ConVarView* cl_predict) noexcept
        : cl_predict_(cl_predict) {}
    virtual ~Prediction23Fixture() noexcept { ++events.destructor_calls; }

    virtual void __thiscall InitializeFromEngine() noexcept {
        ++events.init_calls;
        if(!cl_predict_ || !cl_predict_->parent) {
            ++events.missing_provider_refusals;
            return;
        }
        enabled_=cl_predict_->parent->numeric_value!=0 ? 1u : 0u;
    }
    // Retail slot2 is a literal RET with no side effects; engine invokes
    // it during client shutdown through [vptr+8].
    virtual void __thiscall ShutdownNoop() noexcept {}
    virtual std::uint32_t __thiscall UnknownSlot03() noexcept {return 0xA003u;}
    virtual std::uint32_t __thiscall UnknownSlot04() noexcept {return 0xA004u;}
    virtual std::uint32_t __thiscall UnknownSlot05() noexcept {return 0xA005u;}
    virtual std::uint32_t __thiscall UnknownSlot06() noexcept {return 0xA006u;}
    // Retail slot7 is 16 bytes, zeroing [+0x28] and [+0x2C] and
    // setting [+0x24] to -1. No arguments, RET 0.
    virtual void __thiscall ResetBookkeeping() noexcept {
        cleared_a_=0u;
        cleared_b_=0u;
        highest_=-1;
    }
    virtual std::uint32_t __thiscall UnknownSlot08() noexcept {return 0xA008u;}
    virtual std::uint32_t __thiscall UnknownSlot09() noexcept {return 0xA009u;}
    virtual std::uint32_t __thiscall UnknownSlot10() noexcept {return 0xA00Au;}
    virtual std::uint32_t __thiscall UnknownSlot11() noexcept {return 0xA00Bu;}
    virtual std::uint32_t __thiscall UnknownSlot12() noexcept {return 0xA00Cu;}
    virtual std::uint32_t __thiscall UnknownSlot13() noexcept {return 0xA00Du;}
    virtual std::uint32_t __thiscall UnknownSlot14() noexcept {return 0xA00Eu;}
    virtual std::uint32_t __thiscall UnknownSlot15() noexcept {return 0xA00Fu;}
    virtual std::uint32_t __thiscall UnknownSlot16() noexcept {return 0xA010u;}
    virtual std::uint32_t __thiscall UnknownSlot17() noexcept {return 0xA011u;}
    virtual std::uint32_t __thiscall UnknownSlot18() noexcept {return 0xA012u;}
    virtual std::uint32_t __thiscall UnknownSlot19() noexcept {return 0xA013u;}
    virtual std::uint32_t __thiscall UnknownSlot20() noexcept {return 0xA014u;}
    virtual std::uint32_t __thiscall UnknownSlot21() noexcept {return 0xA015u;}
    virtual std::uint32_t __thiscall UnknownSlot22() noexcept {return 0xA016u;}

    std::uint8_t enabled() const noexcept {return enabled_;}
    std::uintptr_t enabled_address() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&enabled_);
    }
    void set_cl_predict(const ConVarView* cl_predict) noexcept {
        cl_predict_=cl_predict;
    }
    void prime_bookkeeping(std::int32_t highest,
                           std::uint32_t a,std::uint32_t b) noexcept {
        highest_=highest;
        cleared_a_=a;
        cleared_b_=b;
    }
    std::int32_t highest() const noexcept {return highest_;}
    std::uint32_t value_a() const noexcept {return cleared_a_;}
    std::uint32_t value_b() const noexcept {return cleared_b_;}
    std::uintptr_t highest_address() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&highest_);
    }
    std::uintptr_t value_a_address() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&cleared_a_);
    }
    std::uintptr_t value_b_address() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&cleared_b_);
    }
private:
    // PE32 native vfptr occupies this+0 through this+3. This opaque
    // placeholder spans this+4..+0xF, and enabled_ is at +0x10.
    std::array<std::uint8_t,12> opaque_{};
    std::uint8_t enabled_=0u;
    const ConVarView* cl_predict_=nullptr; // PE32 native this+0x14
    std::array<std::uint8_t,12> unknown_prediction_fields_{};
    std::int32_t highest_=-1;
    std::uint32_t cleared_a_=0u;
    std::uint32_t cleared_b_=0u;
};

static_assert(sizeof(void*)==4u,"original 2006 PE32 pointer width");
static_assert(std::has_virtual_destructor<Prediction23Fixture>::value,
              "original prediction vtable slot0 is a deleting destructor");
using Slot1Signature=void (__thiscall Prediction23Fixture::*)() noexcept;
static_assert(std::is_same<
    decltype(&Prediction23Fixture::InitializeFromEngine),
    Slot1Signature>::value,
    "original prediction slot1 is parameterless MSVC thiscall");
using NoArgVoid=void (__thiscall Prediction23Fixture::*)() noexcept;
static_assert(std::is_same<
    decltype(&Prediction23Fixture::ShutdownNoop),NoArgVoid>::value,
    "original prediction slot2 shutdown is parameterless RET0");
static_assert(std::is_same<
    decltype(&Prediction23Fixture::ResetBookkeeping),NoArgVoid>::value,
    "original prediction slot7 reset is parameterless RET0");

} // namespace client_prediction23_diagnostic
} // namespace sin_independent
