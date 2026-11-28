// Phase1669: source-authentic bounded C_BaseEntity PostNetwork
// reconciliation Boolean contract, actual original VA 0x240C6900.
//
// The original 2006 PE32 method is 192 native bytes, one stack
// DWORD, __thiscall RET4, and is WHOLE-SHA pinned by the matching
// Python auditor. It is called for predictable entities from
// CPrediction::PostNetworkDataReceived slot6 (Phase1668).
//
// This is NOT a complete implementation of the original method's
// child helpers 0x240C5960, 0x241978A0, 0x24199BA0 or its
// negative-debug ConVar and embedded-subobject virtual paths.
// It only implements verified frame>0 gating, ring selection,
// primary virtual vptr+0x40 and result>0 => AL=1 through
// strictly host-owned callbacks. No original DLL code executes.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_independent {
namespace original_postnetwork_entity_compare {

constexpr std::uintptr_t kOriginalMethodVa=0x240C6900u;
constexpr unsigned kOriginalNativeMethodBytes=192u;
constexpr unsigned kOriginalStackPopBytes=4u;
constexpr unsigned kOriginalPrimaryEntityVirtualSlot=16u;
constexpr unsigned kOriginalPrimaryEntityVirtualOffset=0x40u;
constexpr unsigned kOriginalHistoryStartOffset=0x2ECu;
constexpr unsigned kOriginalHistoryActiveOffset=0x454u;
constexpr unsigned kOriginalHistorySlots=90u;
constexpr unsigned kOriginalPredictableFieldOffset=0x2E8u;
constexpr std::uintptr_t kOriginalOpaqueTagHelper=0x240C5960u;
constexpr std::uintptr_t kOriginalOpaquePrepareHelper=0x241978A0u;
constexpr std::uintptr_t kOriginalOpaqueDiffHelper=0x24199BA0u;

static_assert(kOriginalHistoryStartOffset+
              kOriginalHistorySlots*4u==kOriginalHistoryActiveOffset,
              "native 90-DWORD ring spans C_BaseEntity +0x2EC..+0x450");

struct Ops {
    void* user=nullptr;
    // The following objects represent ONLY original-free C_BaseEntity
    // storage. No borrowed pointers into live retail engine objects.
    const std::array<std::uint32_t,kOriginalHistorySlots>* history=nullptr;
    bool history_active=false;
    // Calls original 0x240C5960-like opaque tag before frame>0 check.
    void (*on_tag)(void*,void*,std::int32_t) noexcept=nullptr;
    // Original 0x241978A0 full call signature/effects unresolved.
    void (*on_prepare)(void*,void*,std::int32_t,std::uint32_t) noexcept=nullptr;
    // Must execute host-generated C_BaseEntity vtable[16] in test.
    const void* (*get_virtual16)(void*,void*) noexcept=nullptr;
    // Represents opaque 0x24199BA0 result, NOT reconstructed body.
    std::int32_t (*difference_count)(void*,void*,const void*) noexcept=nullptr;
};
enum class Status {
    MissingEntity,
    MissingTag,
    MissingComparisonBackend,
    Complete,
};
struct Result {
    Status status=Status::MissingEntity;
    std::uint8_t return_al=0u;
    std::uint32_t history_word=0u;
    unsigned history_index=0u;
    unsigned tagged=0u;
    unsigned prepared=0u;
    unsigned virtual16_calls=0u;
    unsigned difference_calls=0u;
    std::int32_t observed_difference_count=0;
    unsigned retail_native_method_invocations=0u;
};
inline Result Run(void* entity,std::int32_t frame,
                  const Ops& ops) noexcept {
    Result out{};
    if(!entity)return out;
    if(!ops.on_tag) {
        out.status=Status::MissingTag;
        return out;
    }
    ops.on_tag(ops.user,entity,frame);
    ++out.tagged;
    // Original performs a signed frame>0 check, not unsigned.
    // The expensive comparison does not run for frame==0 or <0.
    if(frame<=0) {
        out.status=Status::Complete;
        return out;
    }
    if(!ops.on_prepare || !ops.get_virtual16 ||
       !ops.difference_count) {
        out.status=Status::MissingComparisonBackend;
        return out; // synthetic-only safety refusal
    }
    out.history_index=static_cast<unsigned>((frame-1)%
                              static_cast<std::int32_t>(
                                  kOriginalHistorySlots));
    if(ops.history_active) {
        if(!ops.history) {
            out.status=Status::MissingComparisonBackend;
            return out; // cannot follow absent invented buffer
        }
        out.history_word=(*ops.history)[out.history_index];
    }
    ops.on_prepare(ops.user,entity,frame,out.history_word);
    ++out.prepared;
    const auto* source=ops.get_virtual16(ops.user,entity);
    ++out.virtual16_calls;
    // Original passes this source to a second helper. It does not
    // check source for null. We refuse it in the safe synthetic model.
    if(!source) {
        out.status=Status::MissingComparisonBackend;
        return out;
    }
    out.observed_difference_count=
        ops.difference_count(ops.user,entity,source);
    ++out.difference_calls;
    // Original x86 compares signed EAX to zero and sets AL to one
    // only when the opaque difference helper returns EAX > 0.
    out.return_al=out.observed_difference_count>0?1u:0u;
    out.status=Status::Complete;
    return out;
}
} // namespace original_postnetwork_entity_compare
} // namespace sin_independent
