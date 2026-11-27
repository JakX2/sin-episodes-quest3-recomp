// Phase 1658: read-only, SHA-gated owner-original client prediction
// singleton/vtable readback. Only this header's adapter is exercised in
// public CI, using invented in-process DWORDs. The real owner probe uses
// the same 24 read operations AFTER optional owner-confirmed original
// DllMain, never invokes ANY retail virtual or prediction Init.
#pragma once

#include "original_client_entitylist_readonly_snapshot.h"
#include <cstddef>
#include <cstdint>

namespace sin_independent {
namespace original_client_prediction23_readonly_snapshot {

namespace abi=original_client_entitylist_snapshot;
constexpr std::uintptr_t kSingletonRva=0x37FA00u;
constexpr std::uintptr_t kVtableRva=0x2A363Cu;
constexpr unsigned kVirtualCount=23u;
constexpr std::uint32_t kTargets[23]={
    0x195A50u, 0x195F80u, 0x195710u, 0x197210u,
    0x195F90u, 0x196040u, 0x1960E0u, 0x1957A0u,
    0x195DD0u, 0x195DF0u, 0x195E30u, 0x195E80u,
    0x195EC0u, 0x195F10u, 0x195F50u, 0x195970u,
    0x195980u, 0x195990u, 0x1968A0u, 0x1964F0u,
    0x196790u, 0x196AE0u, 0x1977B0u
};
static_assert(sizeof(kTargets)/sizeof(kTargets[0])==kVirtualCount,
              "original prediction has exactly 23 virtual entries");
static_assert(kSingletonRva>kVtableRva,
              "prediction singleton RVA must not be its own vtable");

enum class Status {
    Verified,
    MissingReader,
    InvalidBase,
    SingletonVptrUnreadable,
    SingletonVptrMismatch,
    VirtualSlotUnreadable,
    VirtualSlotMismatch,
};
struct Result {
    Status status=Status::MissingReader;
    unsigned matched_slots=0u;
    unsigned expected_slots=kVirtualCount;
    unsigned actual_method_invocations=0u;
};
inline const char* StatusName(Status status) noexcept {
    switch(status) {
    case Status::Verified:return "pinned_original_vptr_read";
    case Status::MissingReader:return "missing_reader";
    case Status::InvalidBase:return "invalid_module_base";
    case Status::SingletonVptrUnreadable:return "singleton_vptr_unreadable";
    case Status::SingletonVptrMismatch:return "singleton_vptr_mismatch";
    case Status::VirtualSlotUnreadable:return "prediction_slot_unreadable";
    case Status::VirtualSlotMismatch:return "prediction_slot_mismatch";
    }
    return "unknown_status";
}
inline Result Observe(std::uintptr_t module_base,abi::Reader reader) noexcept {
    Result out{};
    if(!reader.read)return out;
    // PE32 native owner mode only. No wraparound, host pointer alias
    // or raw engine pointer leaked through result fields.
    constexpr auto kMaxRequiredRva=kSingletonRva+4u;
    if(!module_base || module_base>0xFFFFFFFFu-kMaxRequiredRva) {
        out.status=Status::InvalidBase;
        return out;
    }
    std::uint32_t ptr=0u;
    if(!reader.read(reader.context,module_base+kSingletonRva,&ptr)) {
        out.status=Status::SingletonVptrUnreadable;
        return out;
    }
    if(ptr!=static_cast<std::uint32_t>(module_base+kVtableRva)) {
        out.status=Status::SingletonVptrMismatch;
        return out;
    }
    for(unsigned slot=0u;slot<kVirtualCount;++slot) {
        const auto address=module_base+kVtableRva+4u*slot;
        if(!reader.read(reader.context,address,&ptr)) {
            out.status=Status::VirtualSlotUnreadable;
            return out;
        }
        if(ptr!=static_cast<std::uint32_t>(
                 module_base+static_cast<std::uintptr_t>(kTargets[slot]))) {
            out.status=Status::VirtualSlotMismatch;
            return out;
        }
        ++out.matched_slots;
    }
    out.status=Status::Verified;
    return out;
}
} // namespace original_client_prediction23_readonly_snapshot
} // namespace sin_independent
