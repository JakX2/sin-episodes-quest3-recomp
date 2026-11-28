// Phase1690: actual production PE32 VCENTERPRINT002 eight-virtual
// cold-state wrapper. No proprietary executable bytes are linked.
//
// Original source: client.dll factory thunk 0x241EC540 returns
// process object 0x2431DBF0, two DWORDs: virtual ptr at +0,
// nullable internal centerprint service at +4. Constructor
// 0x241EC5E0 stores vtable 0x242ABC4C and clears +4.
//
// Original vtable (full original method bodies whole-SHA audited):
//   0 0x241EC430 -> 4 stack args, RET16
//   1 0x241EC480 -> 1 stack arg, RET4
//   2 0x241EC450 -> 1 stack arg, RET4
//   3 0x241EC4D0 -> 5 stack args, RET20
//   4 0x241EC4B0 -> 5 stack args, RET20
//   5 0x241EC4F0 -> 0 stack args, RET0
//   6 0x241EC760 -> 1 stack arg, RET4
//   7 0x241EC510 -> 0 stack args, RET0
//
// In the original +4 == nullptr state, methods0–5 and7 perform
// no engine action, forwarding only when a backing object exists.
// Slot6, however, allocates a 0xF8-byte service with another
// GUI-dependent constructor 0x241EC5F0. We intentionally refuse
// slot6 until that service and ownership are reconstructed.
// This is NOT a functional centerprint renderer or UI service.
//
// Factory is available ONLY in the owner explicitly authorized
// diagnostic process after prediction AND entity-list factories
// were installed. Ordinary game clients never see the interface.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1690 original VCENTERPRINT002 ABI requires native MSVC x86
#endif

namespace sin_independent::native_original_client_centerprint {
inline constexpr char kInterfaceName[]="VCENTERPRINT002";
inline constexpr unsigned kOriginalSlots=8u;
inline constexpr unsigned kOriginalBackingOffset=0x04u;
using RefuseCallback=void (*)(const char*) noexcept;

class ColdWrapper final {
public:
    explicit ColdWrapper(RefuseCallback refuse) noexcept:
        refuse_(refuse) {}

    // Source original 0x241EC430, tail forwards four arguments to
    // service vtable+0x2B8; absent service is native RET16.
    virtual void __thiscall Slot0Four(
        std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t) noexcept {
        MustBeCold();
    }
    // Source original 0x241EC480, adds 4 full-FF color words
    // before forwarding to backend+0x2C4; absent service RET4.
    virtual void __thiscall Slot1One(std::uint32_t) noexcept {
        MustBeCold();
    }
    // Source original 0x241EC450, as above to backend+0x2C8.
    virtual void __thiscall Slot2One(std::uint32_t) noexcept {
        MustBeCold();
    }
    // Source original 0x241EC4D0, backend+0x2C4, else RET20.
    virtual void __thiscall Slot3Five(
        std::uint32_t,std::uint32_t,std::uint32_t,
        std::uint32_t,std::uint32_t) noexcept {
        MustBeCold();
    }
    // Source original 0x241EC4B0, backend+0x2C8, else RET20.
    virtual void __thiscall Slot4Five(
        std::uint32_t,std::uint32_t,std::uint32_t,
        std::uint32_t,std::uint32_t) noexcept {
        MustBeCold();
    }
    // Engine.dll source 0x200AA539, 0x200AA630 and
    // 0x20150D56 invoke slot5 without arguments.
    virtual void __thiscall Slot5Noarg() noexcept {
        MustBeCold();
    }
    // Source original slot6 0x241EC760 allocates 0xF8 bytes
    // and calls the still-unimplemented service constructor
    // 0x241EC5F0. NEVER report successful UI initialization.
    virtual void* __thiscall Slot6Initialize(
            void*) noexcept {
        Refuse("VCENTERPRINT002.slot6-backing-service-unimplemented");
    }
    // Source original 0x241EC510 releases +4 service if present
    // and always zeros it. Since no service exists, null RET0.
    virtual void __thiscall Slot7Shutdown() noexcept {
        MustBeCold();
        backing_=nullptr;
    }

    void BindCold() noexcept {
        backing_=nullptr;
        active_=true;
    }
    void Revoke() noexcept {
        backing_=nullptr;
        active_=false;
    }
    bool Active() const noexcept {return active_;}

private:
    void MustBeCold() noexcept {
        if(!active_) Refuse("VCENTERPRINT002.callback-revoked");
        if(backing_!=nullptr)
            Refuse("VCENTERPRINT002.unexpected-nonnull-backing");
    }
    [[noreturn]] void Refuse(const char* label) noexcept {
        if(refuse_)refuse_(label);
        std::abort();
    }

    // Native MSVC x86 vfptr +0, service +4 (authenticated original).
    void* backing_=nullptr;
    // Host-only metadata at +8 onwards is never used by source
    // engine interfaces (which only interact through eight slots).
    bool active_=false;
    RefuseCallback refuse_=nullptr;
};
static_assert(sizeof(void*)==4u,"original 2006 PE32 pointer width");
} // namespace sin_independent::native_original_client_centerprint
