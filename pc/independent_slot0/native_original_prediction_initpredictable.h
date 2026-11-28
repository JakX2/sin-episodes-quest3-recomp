// Phase1685: original-free native Win32 x86 reconstruction of the
// complete 90B 2006 PE32 InitPredictable helper 0x240CAF40,
// called ONLY after the original Phase1684 entity guard.
//
// Source native __thiscall no-stack-argument RET0, marking entity
// BYTE+0x2E8 = 1, then in exact order: virtual+0x44 -> helper
// 0x240C8F00 with returned value, helper 0x240C8720, global
// 0x242F7F78 callback with copied entity DWORD+0xB4,
// reconcile 0x240C6900 with arg0=0, then exactly 90 calls
// to still-opaque 0x240C5960("InitPredictable", i, 0) for i=0..89.
//
// All downstream engine methods and game effects remain opaque
// injected callbacks. Source fields +0xB4/+0x2E8 are stored
// inside the SAME native Phase1684 owner, not parallel records.
// No original entity pointer or retail code executes in CI.
#pragma once
#include "native_original_prediction_entity_reinit_guard.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1685 requires native MSVC x86
#endif
namespace sin_independent {
namespace original_prediction_initpredictable {
namespace guard=original_prediction_entity_reinit_guard;

constexpr std::uintptr_t kOriginalMethodVa=0x240CAF40u;
constexpr unsigned kOriginalNativeBytes=90u;
constexpr unsigned kOriginalRetPopBytes=0u;
constexpr unsigned kOriginalStackArgs=0u;
constexpr unsigned kOriginalVcall44Offset=0x44u;
constexpr unsigned kOriginalEntityHandleOffset=0xB4u;
constexpr unsigned kOriginalPredictableFlagOffset=0x2E8u;
constexpr std::uintptr_t kOriginalOpaqueApplyVcallVa=0x240C8F00u;
constexpr std::uintptr_t kOriginalOpaqueNextVa=0x240C8720u;
constexpr std::uintptr_t kOriginalGlobalHandleVa=0x242F7F78u;
constexpr std::uintptr_t kOriginalOpaqueHandleVa=0x240CAC00u;
constexpr std::uintptr_t kOriginalOpaqueReconcileVa=0x240C6900u;
constexpr std::uintptr_t kOriginalOpaquePerFieldVa=0x240C5960u;
constexpr unsigned kOriginalFieldCount=0x5Au;
constexpr char kOriginalFieldLabel[]="InitPredictable";
static_assert(sizeof(void*)==4u &&
              kOriginalRetPopBytes==0u &&
              offsetof(guard::Fixture,opaque_mid)==0xA8u &&
              offsetof(guard::Fixture,opaque_tail)==0xC1u &&
              kOriginalEntityHandleOffset>=0xA8u &&
              kOriginalEntityHandleOffset+4u<=0xC0u &&
              kOriginalPredictableFlagOffset>=0xC1u &&
              kOriginalPredictableFlagOffset<0x454u,
              "native original init source offsets or member ABI drift");

struct Fixture;
struct Ops {
    void* user=nullptr;
    // Original virtual method entity+vtable 0x44 returns a DWORD.
    std::uint32_t (*virtual_44)(void*,Fixture*) noexcept=nullptr;
    // Opaque original 0x240C8F00 receives the virtual return.
    void (*apply_virtual_result)(void*,Fixture*,
                                 std::uint32_t) noexcept=nullptr;
    // Opaque original 0x240C8720, ECX=this, no stack args.
    void (*secondary_init)(void*,Fixture*) noexcept=nullptr;
    // Global handler object 0x242F7F78 receives entity+0xB4
    // raw DWORD as one stack word, not a dereferenced owner.
    void (*notify_global_handle)(void*,std::uint32_t) noexcept=nullptr;
    // Opaque original 0x240C6900 receives zero stack DWORD.
    void (*reconcile_zero)(void*,Fixture*,std::uint32_t) noexcept=nullptr;
    // Original 0x240C5960 has three stack DWORD args:
    // literal, i=0..89, zero; its effects remain opaque.
    void (*initialize_field)(void*,Fixture*,const char*,
                             std::uint32_t,std::uint32_t) noexcept=nullptr;
};
struct Events {
    unsigned called=0u;
    unsigned safe_refusals=0u;
    unsigned flag_written=0u;
    unsigned virtual_calls=0u;
    unsigned result_helper_calls=0u;
    unsigned secondary_calls=0u;
    unsigned global_handle_calls=0u;
    unsigned reconcile_calls=0u;
    unsigned per_field_calls=0u;
    unsigned completed=0u;
};

// C++ base contributes original source-layout fields 0..0x457;
// ALL additional host-only callback state stays after that region.
struct Fixture : guard::Fixture {
    Ops init_ops{};
    Events init_events{};

    void set_owner_handle(std::uint32_t value) noexcept {
        std::memcpy(opaque_mid.data()+
                    (kOriginalEntityHandleOffset-0xA8u),
                    &value,sizeof(value));
    }
    std::uint32_t owner_handle() const noexcept {
        std::uint32_t value=0u;
        std::memcpy(&value,opaque_mid.data()+
                    (kOriginalEntityHandleOffset-0xA8u),
                    sizeof(value));
        return value;
    }
    std::uint8_t predictable_flag() const noexcept {
        return opaque_tail[
            kOriginalPredictableFlagOffset-0xC1u];
    }
    void set_predictable_flag(std::uint8_t value) noexcept {
        opaque_tail[
            kOriginalPredictableFlagOffset-0xC1u]=value;
    }

    // True compiler-generated native __thiscall no stack args,
    // plain RET0, no vptr invented for the original entity.
    __declspec(noinline) void __thiscall InitializePredictable() noexcept {
        ++init_events.called;
        // The original writes this flag before ALL child calls.
        set_predictable_flag(1u);
        ++init_events.flag_written;
        if(!init_ops.virtual_44) {
            ++init_events.safe_refusals;
            return;
        }
        ++init_events.virtual_calls;
        const auto v=init_ops.virtual_44(init_ops.user,this);
        if(!init_ops.apply_virtual_result) {
            ++init_events.safe_refusals;
            return;
        }
        ++init_events.result_helper_calls;
        init_ops.apply_virtual_result(init_ops.user,this,v);
        if(!init_ops.secondary_init) {
            ++init_events.safe_refusals;
            return;
        }
        ++init_events.secondary_calls;
        init_ops.secondary_init(init_ops.user,this);
        if(!init_ops.notify_global_handle) {
            ++init_events.safe_refusals;
            return;
        }
        ++init_events.global_handle_calls;
        init_ops.notify_global_handle(init_ops.user,owner_handle());
        if(!init_ops.reconcile_zero) {
            ++init_events.safe_refusals;
            return;
        }
        ++init_events.reconcile_calls;
        init_ops.reconcile_zero(init_ops.user,this,0u);
        if(!init_ops.initialize_field) {
            ++init_events.safe_refusals;
            return;
        }
        for(std::uint32_t i=0u;i<kOriginalFieldCount;++i) {
            ++init_events.per_field_calls;
            init_ops.initialize_field(init_ops.user,this,
                                      kOriginalFieldLabel,i,0u);
        }
        ++init_events.completed;
    }
};
static_assert(offsetof(Fixture,init_ops)>=
              kOriginalPredictableFlagOffset+1u,
              "host-only callbacks overlap source entity prediction flag");

} // namespace original_prediction_initpredictable
} // namespace sin_independent
