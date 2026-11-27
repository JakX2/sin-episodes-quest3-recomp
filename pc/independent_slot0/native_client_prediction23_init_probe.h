// Phase 1658: original-free native Win32 x86 prediction slot1 ABI fixture.
// SHA-matched retail client.dll slot1 (0x24195F80) has NO stack arguments,
// reads the service pointer from global 0x24374540, tests its +0x2C DWORD,
// and sets byte this+0x10 to (nonzero?1:0), RET 0.
//
// The original does not check whether the global pointer is null. The
// independently owned diagnostic model instead refuses a missing provider.
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
constexpr std::uintptr_t kOriginalInitDependencyGlobalRva=0x374540u;
constexpr unsigned kOriginalInitDependencyMemberOffset=0x2Cu;
constexpr unsigned kOriginalPredictionEnabledOffset=0x10u;
constexpr unsigned kOriginalVirtualCount=23u;

// Only invented provider values are accepted. This mimics the *effect*
// of original initialized service+0x2C without exposing engine memory.
struct Provider {
    std::array<std::uint8_t,0x2Cu> opaque{};
    std::uint32_t flag=0u;
};
static_assert(offsetof(Provider,flag)==0x2Cu,
              "native synthetic engine service +0x2C offset changed");

struct Events {
    unsigned init_calls=0u;
    unsigned missing_provider_refusals=0u;
    unsigned destructor_calls=0u;
};
inline Events events{};

// The first virtual in MSVC Win32 x86 is a deleting destructor.
// The second is the only method signature authenticated in this fixture.
// All remaining slots are separate inert SENTINELS, not claims about
// the actual retail IPrediction methods or their parameters.
class Prediction23Fixture {
public:
    explicit Prediction23Fixture(const Provider* provider) noexcept
        : provider_(provider) {}
    virtual ~Prediction23Fixture() noexcept { ++events.destructor_calls; }

    virtual void __thiscall InitializeFromEngine() noexcept {
        ++events.init_calls;
        if(!provider_) {
            ++events.missing_provider_refusals;
            return;
        }
        enabled_=provider_->flag!=0u ? 1u : 0u;
    }
    virtual std::uint32_t __thiscall UnknownSlot02() noexcept {return 0xA002u;}
    virtual std::uint32_t __thiscall UnknownSlot03() noexcept {return 0xA003u;}
    virtual std::uint32_t __thiscall UnknownSlot04() noexcept {return 0xA004u;}
    virtual std::uint32_t __thiscall UnknownSlot05() noexcept {return 0xA005u;}
    virtual std::uint32_t __thiscall UnknownSlot06() noexcept {return 0xA006u;}
    virtual std::uint32_t __thiscall UnknownSlot07() noexcept {return 0xA007u;}
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
    void set_provider(const Provider* provider) noexcept {
        provider_=provider;
    }
private:
    // PE32 native vfptr occupies this+0 through this+3. This opaque
    // placeholder spans this+4..+0xF, and enabled_ is at +0x10.
    std::array<std::uint8_t,12> opaque_{};
    std::uint8_t enabled_=0u;
    const Provider* provider_=nullptr;
};

static_assert(sizeof(void*)==4u,"original 2006 PE32 pointer width");
static_assert(std::has_virtual_destructor<Prediction23Fixture>::value,
              "original prediction vtable slot0 is a deleting destructor");
using Slot1Signature=void (__thiscall Prediction23Fixture::*)() noexcept;
static_assert(std::is_same<
    decltype(&Prediction23Fixture::InitializeFromEngine),
    Slot1Signature>::value,
    "original prediction slot1 is parameterless MSVC thiscall");

} // namespace client_prediction23_diagnostic
} // namespace sin_independent
