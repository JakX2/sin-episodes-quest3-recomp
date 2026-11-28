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
constexpr unsigned kOriginalGetterSlot15Offset=0x08u;
constexpr unsigned kOriginalGetterSlot16Offset=0x09u;
constexpr unsigned kOriginalGetterSlot17Offset=0x0Cu;
constexpr unsigned kOriginalHostFlagSlot=8u;
constexpr unsigned kOriginalHostViewSlot=13u;
constexpr unsigned kOriginalHostFlagOffset=0x1DCu;
constexpr unsigned kOriginalHostViewOffset=0xF04u;
constexpr unsigned kOriginalPlayerGetterA=9u;
constexpr unsigned kOriginalPlayerSetterA=10u;
constexpr unsigned kOriginalPlayerGetterB=11u;
constexpr unsigned kOriginalPlayerSetterB=12u;
constexpr unsigned kOriginalPlayerVcallGetA=0xE4u;
constexpr unsigned kOriginalPlayerVcallSetA=0xE8u;
constexpr unsigned kOriginalPlayerVcallGetB=0xECu;
constexpr unsigned kOriginalPlayerVcallSetB=0xF0u;
constexpr unsigned kOriginalPlayerMirrorA=0x2C8u;
constexpr unsigned kOriginalPlayerMirrorB=0x2D4u;
constexpr unsigned kOriginalPlayerNotifierA=0x22Cu;
constexpr unsigned kOriginalPlayerNotifierB=0x268u;
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

// The retail prediction host pointer is a separate original client
// global at RVA 0x338CF0, not CPrediction.this. For original-free tests,
// construct a fully owned memory image with the two directly read fields.
struct ViewBits3 {std::uint32_t x=0u,y=0u,z=0u;};
static_assert(sizeof(ViewBits3)==12u,"3 original view DWORDs");
struct HostSnapshot {
    std::array<std::uint8_t,0x1DCu> reserved_first{};
    std::uint32_t field1dc=0u;
    std::array<std::uint8_t,0xF04u-0x1E0u> reserved_between{};
    ViewBits3 view{};
};
static_assert(offsetof(HostSnapshot,field1dc)==kOriginalHostFlagOffset,
              "native prediction host slot8 field offset drift");
static_assert(offsetof(HostSnapshot,view)==kOriginalHostViewOffset,
              "native prediction host slot13 vector offset drift");

// Artificial callbacks represent only the original C_BasePlayer virtual
// methods and embedded notifier at authentic byte offsets. They are NOT
// original player objects or claims about the engine's object layouts.
// The original setter callback executes BEFORE mirror fields are copied,
// and the notifier callback executes AFTER that copy.
struct OrientationOps {
    void* user=nullptr;
    using Getter=const ViewBits3* (*)(void*) noexcept;
    using Setter=void (*)(void*,ViewBits3*) noexcept;
    using Mirror=void (*)(void*,const ViewBits3*) noexcept;
    using Notify=void (*)(void*) noexcept;
    Getter get_a=nullptr;
    Setter set_a=nullptr;
    Getter get_b=nullptr;
    Setter set_b=nullptr;
    Mirror mirror_a=nullptr;
    Mirror mirror_b=nullptr;
    Notify notify_a=nullptr;
    Notify notify_b=nullptr;
};

struct Events {
    unsigned init_calls=0u;
    unsigned missing_provider_refusals=0u;
    unsigned destructor_calls=0u;
    unsigned missing_output_refusals=0u;
    unsigned missing_orientation_callbacks=0u;
    unsigned missing_orientation_inputs=0u;
};
inline Events events{};

// MSVC slot0 deletes, slot1 Init copies cl_predict effective numeric
// value, slot2 on engine shutdown is an empty RET and slot7 resets three
// prediction state DWORDs. Slots15/16/17 are tiny native field getters;
// the other slots remain inert sentinels. This fixture is not a
// registerable implementation of IPrediction.
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
    // Retail slot8 reads client-global host +0x1DC, or returns zero
    // when the global host is absent. No arguments, RET0.
    virtual std::uint32_t __thiscall GetHostField1DC() noexcept {
        return view_host_?view_host_->field1dc:0u;
    }
    // Retail getter slots9/11: if no active C_BasePlayer, zero three
    // output DWORDs. Otherwise call player vptr +0xE4/+0xEC for the
    // returned pointer and copy three words into caller's OUT buffer.
    // The safe synthetic fixture refuses NULL output or returned vector.
    virtual void __thiscall CopyOrientationA(ViewBits3* out) noexcept {
        if(!out) {++events.missing_output_refusals;return;}
        *out=ViewBits3{};
        if(!orientation_.user)return;
        if(!orientation_.get_a) {
            ++events.missing_orientation_callbacks;return;
        }
        const auto* src=orientation_.get_a(orientation_.user);
        if(!src) {
            ++events.missing_orientation_callbacks;return;
        }
        *out=*src;
    }
    // Retail setter slot10: if player present, call player vptr +0xE8
    // FIRST, copy three (possibly mutated) caller DWORDs into player
    // +0x2C8/+0x2CC/+0x2D0, THEN invoke embedded player+0x22C
    // notifier's vptr +0x0C. Caller stack cleanup is RET4.
    virtual void __thiscall SetOrientationA(ViewBits3* in) noexcept {
        if(!orientation_.user)return;
        if(!in) {++events.missing_orientation_inputs;return;}
        if(!orientation_.set_a || !orientation_.mirror_a ||
           !orientation_.notify_a) {
            ++events.missing_orientation_callbacks;return;
        }
        orientation_.set_a(orientation_.user,in);
        orientation_.mirror_a(orientation_.user,in);
        orientation_.notify_a(orientation_.user);
    }
    virtual void __thiscall CopyOrientationB(ViewBits3* out) noexcept {
        if(!out) {++events.missing_output_refusals;return;}
        *out=ViewBits3{};
        if(!orientation_.user)return;
        if(!orientation_.get_b) {
            ++events.missing_orientation_callbacks;return;
        }
        const auto* src=orientation_.get_b(orientation_.user);
        if(!src) {
            ++events.missing_orientation_callbacks;return;
        }
        *out=*src;
    }
    // Retail setter slot12: player vptr +0xF0; 3 mirrored DWORDs
    // player+0x2D4/+0x2D8/+0x2DC; notifier at player+0x268.
    virtual void __thiscall SetOrientationB(ViewBits3* in) noexcept {
        if(!orientation_.user)return;
        if(!in) {++events.missing_orientation_inputs;return;}
        if(!orientation_.set_b || !orientation_.mirror_b ||
           !orientation_.notify_b) {
            ++events.missing_orientation_callbacks;return;
        }
        orientation_.set_b(orientation_.user,in);
        orientation_.mirror_b(orientation_.user,in);
        orientation_.notify_b(orientation_.user);
    }
    // Retail slot13 writes 12 bytes into its single caller-provided
    // output pointer; when host global is missing, writes all zero.
    // The independent fixture rejects an absent output pointer instead
    // of reproducing the retail null-destination crash.
    virtual void __thiscall CopyHostView(ViewBits3* out) noexcept {
        if(!out) {
            ++events.missing_output_refusals;
            return;
        }
        *out=view_host_?view_host_->view:ViewBits3{};
    }
    virtual std::uint32_t __thiscall UnknownSlot14() noexcept {return 0xA00Eu;}
    // Retail slots15/16 read individual BYTEs at +8/+9 via AL,
    // and slot17 reads a DWORD at +0xC via EAX. All are RET0.
    virtual std::uint8_t __thiscall GetMember8() noexcept {return member8_;}
    virtual std::uint8_t __thiscall GetMember9() noexcept {return member9_;}
    virtual std::uint32_t __thiscall GetMemberC() noexcept {return memberC_;}
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
    void set_orientation_ops(OrientationOps ops) noexcept {
        orientation_=ops;
    }
    void set_view_host(const HostSnapshot* host) noexcept {
        view_host_=host;
    }
    void prime_early_fields(std::uint8_t a,std::uint8_t b,
                            std::uint32_t c) noexcept {
        member8_=a;
        member9_=b;
        memberC_=c;
    }
    std::uintptr_t member8_address() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&member8_);
    }
    std::uintptr_t member9_address() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&member9_);
    }
    std::uintptr_t memberC_address() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&memberC_);
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
    // PE32 native vfptr occupies this+0..+3. The next 12 bytes
    // preserve actual getter member positions at +8/+9/+0xC.
    std::array<std::uint8_t,4> opaque4to7_{};
    std::uint8_t member8_=0u;
    std::uint8_t member9_=0u;
    std::array<std::uint8_t,2> opaqueAtoB_{};
    std::uint32_t memberC_=0u;
    std::uint8_t enabled_=0u;
    const ConVarView* cl_predict_=nullptr; // PE32 native this+0x14
    std::array<std::uint8_t,12> unknown_prediction_fields_{};
    std::int32_t highest_=-1;
    std::uint32_t cleared_a_=0u;
    std::uint32_t cleared_b_=0u;
    const HostSnapshot* view_host_=nullptr; // diagnostic host pointer only
    OrientationOps orientation_{}; // synthetic callback contract only
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
using ByteGetter=std::uint8_t (__thiscall Prediction23Fixture::*)() noexcept;
using DwordGetter=std::uint32_t (__thiscall Prediction23Fixture::*)() noexcept;
static_assert(std::is_same<decltype(&Prediction23Fixture::GetMember8),
              ByteGetter>::value &&
              std::is_same<decltype(&Prediction23Fixture::GetMember9),
              ByteGetter>::value &&
              std::is_same<decltype(&Prediction23Fixture::GetMemberC),
              DwordGetter>::value,
              "native prediction slots15/16/17 byte/word getter signatures");
using HostFlagGetter=std::uint32_t (__thiscall Prediction23Fixture::*)() noexcept;
using HostVectorGetter=void (__thiscall Prediction23Fixture::*)(ViewBits3*) noexcept;
static_assert(std::is_same<decltype(&Prediction23Fixture::GetHostField1DC),
              HostFlagGetter>::value &&
              std::is_same<decltype(&Prediction23Fixture::CopyHostView),
              HostVectorGetter>::value,
              "original prediction slot8 RET0 / slot13 RET4 x86 contracts");
using OrientationRef=void (__thiscall Prediction23Fixture::*)(ViewBits3*) noexcept;
static_assert(std::is_same<decltype(&Prediction23Fixture::CopyOrientationA),
              OrientationRef>::value &&
              std::is_same<decltype(&Prediction23Fixture::SetOrientationA),
              OrientationRef>::value &&
              std::is_same<decltype(&Prediction23Fixture::CopyOrientationB),
              OrientationRef>::value &&
              std::is_same<decltype(&Prediction23Fixture::SetOrientationB),
              OrientationRef>::value,
              "native prediction slots9-12 each take one Vec3 pointer RET4");

} // namespace client_prediction23_diagnostic
} // namespace sin_independent
