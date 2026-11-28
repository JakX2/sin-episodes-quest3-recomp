// Phase1662: source-authenticated ORIGINAL C_BasePlayer primary virtual
// getter/setter methods at slots57-60. This is a new entirely invented
// MSVC Win32 x86 fixture, NOT retail game code or a real player engine.
//
// Original 2006 PE32 client.dll vtable 0x2428AAE4:
//   slot57 +0xE4 -> 0x240C5190: this+0x220, RET0
//   slot58 +0xE8 -> 0x240C51B0: x87 3-float compare, dirty(1), RET4
//   slot59 +0xEC -> 0x240C5260: this+0x25C, RET0
//   slot60 +0xF0 -> 0x240C5280: x87 3-float compare, dirty(2), RET4
// SHA gates and sparse opcode witnesses live in the Phase1662 auditor.
//
// This independent model intentionally refuses NULL setter inputs,
// whereas the original methods dereference them. It does not implement
// the original 0x24096290 dirty-helper external engine side effects.
#pragma once
#include "native_client_prediction23_init_probe.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1662 C_BasePlayer native fixture requires MSVC x86
#endif
namespace sin_independent {
namespace original_client_player_orientation_61 {

using Vec3Bits=client_prediction23_diagnostic::ViewBits3;
constexpr unsigned kGetterSlotA=57u;
constexpr unsigned kSetterSlotA=58u;
constexpr unsigned kGetterSlotB=59u;
constexpr unsigned kSetterSlotB=60u;
constexpr std::uintptr_t kPlayerFieldA=0x220u;
constexpr std::uintptr_t kPlayerFieldB=0x25Cu;
constexpr std::uintptr_t kNotifierA=0x22Cu;
constexpr std::uintptr_t kNotifierB=0x268u;
constexpr std::uintptr_t kMirrorA=0x2C8u;
constexpr std::uintptr_t kMirrorB=0x2D4u;
constexpr std::uintptr_t kDirtyBits=0x140u;
constexpr unsigned kDirtyFlagA=1u;
constexpr unsigned kDirtyFlagB=2u;

// x87 FUCOMPP compares IEEE-754 floating *values*, not raw DWORDs.
// All NaNs compare unordered, even identical bit patterns.
// The two signed zeroes compare equal, and so produce no dirty write.
// Numeric non-NaN equality otherwise equals bitwise equality in binary32.
inline bool OriginalFloatEqualBits(std::uint32_t a,
                                   std::uint32_t b) noexcept {
    const auto aa=a & 0x7FFFFFFFu;
    const auto bb=b & 0x7FFFFFFFu;
    if(aa>0x7F800000u || bb>0x7F800000u)return false;
    if(aa==0u && bb==0u)return true;
    return a==b;
}
inline bool OriginalAnyOrientationChange(const Vec3Bits& a,
                                          const Vec3Bits& b) noexcept {
    return !OriginalFloatEqualBits(a.x,b.x) ||
           !OriginalFloatEqualBits(a.y,b.y) ||
           !OriginalFloatEqualBits(a.z,b.z);
}

struct DirtyHook {
    void* context=nullptr;
    void (*notify)(void*,unsigned) noexcept=nullptr;
};

// This synthetic class has four actual compiler-generated vtable slots,
// including notifier slot3 at BYTE OFFSET 0x0C. The original C_BasePlayer
// embedded notifier class has not been independently reconstructed.
struct Notifier4 {
    virtual std::uint32_t __thiscall Slot0() noexcept {return 0xA001u;}
    virtual std::uint32_t __thiscall Slot1() noexcept {return 0xA002u;}
    virtual std::uint32_t __thiscall Slot2() noexcept {return 0xA003u;}
    virtual void __thiscall Notify() noexcept {
        if(on_notify)on_notify(context,id);
    }
    void* context=nullptr;
    void (*on_notify)(void*,unsigned) noexcept=nullptr;
    unsigned id=0u;
};
static_assert(sizeof(Notifier4)==16u,
              "native subobject must use 32-bit pointer-sized fields");

// 57 explicitly inert early vtable slots preserve the exact native
// indices of the four observed 2006 player orientation operations.
// Unknown earlier player virtual methods are NOT implemented/registered.
class CBasePlayer61Fixture {
public:
#define SIN_PLAYER_INERT_SLOT(N) \
    virtual std::uint32_t __thiscall UnknownSlot##N() noexcept { \
        return 0xA1000000u+static_cast<std::uint32_t>(N); \
    }
    SIN_PLAYER_INERT_SLOT(0)
    SIN_PLAYER_INERT_SLOT(1)
    SIN_PLAYER_INERT_SLOT(2)
    SIN_PLAYER_INERT_SLOT(3)
    SIN_PLAYER_INERT_SLOT(4)
    SIN_PLAYER_INERT_SLOT(5)
    SIN_PLAYER_INERT_SLOT(6)
    SIN_PLAYER_INERT_SLOT(7)
    SIN_PLAYER_INERT_SLOT(8)
    SIN_PLAYER_INERT_SLOT(9)
    SIN_PLAYER_INERT_SLOT(10)
    SIN_PLAYER_INERT_SLOT(11)
    SIN_PLAYER_INERT_SLOT(12)
    SIN_PLAYER_INERT_SLOT(13)
    SIN_PLAYER_INERT_SLOT(14)
    SIN_PLAYER_INERT_SLOT(15)
    SIN_PLAYER_INERT_SLOT(16)
    SIN_PLAYER_INERT_SLOT(17)
    SIN_PLAYER_INERT_SLOT(18)
    SIN_PLAYER_INERT_SLOT(19)
    SIN_PLAYER_INERT_SLOT(20)
    SIN_PLAYER_INERT_SLOT(21)
    SIN_PLAYER_INERT_SLOT(22)
    SIN_PLAYER_INERT_SLOT(23)
    SIN_PLAYER_INERT_SLOT(24)
    SIN_PLAYER_INERT_SLOT(25)
    SIN_PLAYER_INERT_SLOT(26)
    SIN_PLAYER_INERT_SLOT(27)
    SIN_PLAYER_INERT_SLOT(28)
    SIN_PLAYER_INERT_SLOT(29)
    SIN_PLAYER_INERT_SLOT(30)
    SIN_PLAYER_INERT_SLOT(31)
    SIN_PLAYER_INERT_SLOT(32)
    SIN_PLAYER_INERT_SLOT(33)
    SIN_PLAYER_INERT_SLOT(34)
    SIN_PLAYER_INERT_SLOT(35)
    SIN_PLAYER_INERT_SLOT(36)
    SIN_PLAYER_INERT_SLOT(37)
    SIN_PLAYER_INERT_SLOT(38)
    SIN_PLAYER_INERT_SLOT(39)
    SIN_PLAYER_INERT_SLOT(40)
    SIN_PLAYER_INERT_SLOT(41)
    SIN_PLAYER_INERT_SLOT(42)
    SIN_PLAYER_INERT_SLOT(43)
    SIN_PLAYER_INERT_SLOT(44)
    SIN_PLAYER_INERT_SLOT(45)
    SIN_PLAYER_INERT_SLOT(46)
    SIN_PLAYER_INERT_SLOT(47)
    SIN_PLAYER_INERT_SLOT(48)
    SIN_PLAYER_INERT_SLOT(49)
    SIN_PLAYER_INERT_SLOT(50)
    SIN_PLAYER_INERT_SLOT(51)
    SIN_PLAYER_INERT_SLOT(52)
    SIN_PLAYER_INERT_SLOT(53)
    SIN_PLAYER_INERT_SLOT(54)
    SIN_PLAYER_INERT_SLOT(55)
    SIN_PLAYER_INERT_SLOT(56)
#undef SIN_PLAYER_INERT_SLOT

    // Exactly original native slots57..60 (vptr +0xE4..+0xF0).
    virtual const Vec3Bits* __thiscall GetOrientationA() noexcept {
        return &a_;
    }
    virtual void __thiscall SetOrientationA(const Vec3Bits* src) noexcept {
        if(!src) {++null_input_refusals_;return;}
        if(!OriginalAnyOrientationChange(a_,*src))return;
        if(dirty_.notify)dirty_.notify(dirty_.context,kDirtyFlagA);
        // Original reloads caller's raw DWORDs AFTER the dirty helper.
        a_=*src;
    }
    virtual const Vec3Bits* __thiscall GetOrientationB() noexcept {
        return &b_;
    }
    virtual void __thiscall SetOrientationB(const Vec3Bits* src) noexcept {
        if(!src) {++null_input_refusals_;return;}
        if(!OriginalAnyOrientationChange(b_,*src))return;
        if(dirty_.notify)dirty_.notify(dirty_.context,kDirtyFlagB);
        b_=*src;
    }
    CBasePlayer61Fixture() noexcept =default;
    void set_dirty_hook(DirtyHook h) noexcept {dirty_=h;}
    void set_notifier_hooks(void* ctx,
                            void (*sink)(void*,unsigned) noexcept) noexcept {
        notify_a_.context=ctx;notify_a_.on_notify=sink;notify_a_.id=1u;
        notify_b_.context=ctx;notify_b_.on_notify=sink;notify_b_.id=2u;
    }
    const Vec3Bits* a() const noexcept {return &a_;}
    const Vec3Bits* b() const noexcept {return &b_;}
    const Vec3Bits* mirrored_a() const noexcept {return &mirror_a_;}
    const Vec3Bits* mirrored_b() const noexcept {return &mirror_b_;}
    void mirror_a(const Vec3Bits* v) noexcept {if(v)mirror_a_=*v;}
    void mirror_b(const Vec3Bits* v) noexcept {if(v)mirror_b_=*v;}
    Notifier4* notifier_a() noexcept {return &notify_a_;}
    Notifier4* notifier_b() noexcept {return &notify_b_;}
    unsigned refused_null_inputs() const noexcept {return null_input_refusals_;}
    std::uint32_t* dirty_flags_for_test_only() noexcept {return &dirty_flags_;}
    // Layout tests use runtime address differences because a class with
    // virtual methods is not standard-layout; offsetof would be UB.
private:
    std::array<std::uint8_t,0x13Cu> padding04to140_{};
    std::uint32_t dirty_flags_=0u;
    std::array<std::uint8_t,0xDCu> padding144to220_{};
    Vec3Bits a_{};
    Notifier4 notify_a_{};
    std::array<std::uint8_t,0x20u> padding23Cto25C_{};
    Vec3Bits b_{};
    Notifier4 notify_b_{};
    std::array<std::uint8_t,0x50u> padding278to2C8_{};
    Vec3Bits mirror_a_{};
    Vec3Bits mirror_b_{};
    DirtyHook dirty_{};
    unsigned null_input_refusals_=0u;
};

using GetterA=const Vec3Bits* (__thiscall CBasePlayer61Fixture::*)() noexcept;
using SetterA=void (__thiscall CBasePlayer61Fixture::*)(const Vec3Bits*) noexcept;
static_assert(std::is_same<decltype(&CBasePlayer61Fixture::GetOrientationA),
                           GetterA>::value &&
              std::is_same<decltype(&CBasePlayer61Fixture::GetOrientationB),
                           GetterA>::value &&
              std::is_same<decltype(&CBasePlayer61Fixture::SetOrientationA),
                           SetterA>::value &&
              std::is_same<decltype(&CBasePlayer61Fixture::SetOrientationB),
                           SetterA>::value,
              "native player orientation slot57/59 RET0, 58/60 RET4");

} // namespace original_client_player_orientation_61
} // namespace sin_independent
