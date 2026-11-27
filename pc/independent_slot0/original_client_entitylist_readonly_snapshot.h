// Phase 1656: bounded, read-only original client entity-list ABI snapshot.
// This is a header-only *observation* contract: no original DLL code runs
// in the synthetic tests. The owner probe may separately load SHA-matched
// original DLLs after explicit consent, but NEVER calls these virtuals.
#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>

namespace sin_independent {
namespace original_client_entitylist_snapshot {

constexpr std::uintptr_t kParentObjectRva=0x33FDC8u;
constexpr std::uintptr_t kSecondaryObjectRva=0x34FDDCu;
constexpr std::uintptr_t kSecondaryOffset=0x10014u;
constexpr std::uintptr_t kParentVtableRva=0x2973B8u;
constexpr std::uintptr_t kSecondaryVtableRva=0x297390u;
constexpr std::uint32_t kSecondaryMethodRvas[9]={
    0x1056F0u,0x105BB0u,0x1074D0u,0x105AF0u,0x105BE0u,
    0x105700u,0x105740u,0x105720u,0x105730u
};
constexpr std::uint32_t kParentMethodRvas[3]={
    0x1089D0u,0x108A70u,0x1082A0u
};
static_assert(kSecondaryObjectRva-kParentObjectRva==kSecondaryOffset,
              "native factory secondary object does not match offset");
static_assert(kParentVtableRva-kSecondaryVtableRva==9u*4u+4u,
              "nine secondary methods, four inter-table bytes, then parent");
static_assert(sizeof(kSecondaryMethodRvas)/sizeof(kSecondaryMethodRvas[0])==9,
              "nine original entity-list secondary virtual slots");

using ReadDword=bool (*)(void*,std::uintptr_t,std::uint32_t*) noexcept;
struct Reader {
    void* context=nullptr;
    ReadDword read=nullptr;
};
enum class Status {
    Verified,
    MissingReader,
    InvalidBase,
    ParentVptrUnreadable,
    ParentVptrMismatch,
    SecondaryVptrUnreadable,
    SecondaryVptrMismatch,
    SecondarySlotUnreadable,
    SecondarySlotMismatch,
    ParentSlotUnreadable,
    ParentSlotMismatch,
};
struct Result {
    Status status=Status::MissingReader;
    unsigned secondary_slots_matched=0u;
    unsigned parent_slots_matched=0u;
};

inline const char* StatusName(Status s) noexcept {
    switch(s) {
    case Status::Verified:return "pinned_original_vptr_read";
    case Status::MissingReader:return "missing_reader";
    case Status::InvalidBase:return "invalid_module_base";
    case Status::ParentVptrUnreadable:return "parent_vptr_unreadable";
    case Status::ParentVptrMismatch:return "parent_vptr_mismatch";
    case Status::SecondaryVptrUnreadable:return "secondary_vptr_unreadable";
    case Status::SecondaryVptrMismatch:return "secondary_vptr_mismatch";
    case Status::SecondarySlotUnreadable:return "secondary_slot_unreadable";
    case Status::SecondarySlotMismatch:return "secondary_slot_mismatch";
    case Status::ParentSlotUnreadable:return "parent_slot_unreadable";
    case Status::ParentSlotMismatch:return "parent_slot_mismatch";
    }
    return "unknown_status";
}

inline Result Observe(std::uintptr_t base,Reader reader) noexcept {
    Result out{};
    if(!reader.read) return out;
    // Probe addresses must remain within one 32-bit module address space.
    // No implicit wrap and no disassembly or instruction execution.
    constexpr auto maxRva=kSecondaryObjectRva+4u;
    if(base==0u || base>static_cast<std::uintptr_t>(
            std::numeric_limits<std::uint32_t>::max()-maxRva)) {
        out.status=Status::InvalidBase;
        return out;
    }
    std::uint32_t actual=0;
    if(!reader.read(reader.context,base+kParentObjectRva,&actual)) {
        out.status=Status::ParentVptrUnreadable;
        return out;
    }
    if(actual!=static_cast<std::uint32_t>(base+kParentVtableRva)) {
        out.status=Status::ParentVptrMismatch;
        return out;
    }
    if(!reader.read(reader.context,base+kSecondaryObjectRva,&actual)) {
        out.status=Status::SecondaryVptrUnreadable;
        return out;
    }
    if(actual!=static_cast<std::uint32_t>(base+kSecondaryVtableRva)) {
        out.status=Status::SecondaryVptrMismatch;
        return out;
    }
    for(unsigned i=0u;i<9u;++i) {
        if(!reader.read(reader.context,base+kSecondaryVtableRva+4u*i,&actual)) {
            out.status=Status::SecondarySlotUnreadable;
            return out;
        }
        if(actual!=static_cast<std::uint32_t>(base+kSecondaryMethodRvas[i])) {
            out.status=Status::SecondarySlotMismatch;
            return out;
        }
        ++out.secondary_slots_matched;
    }
    for(unsigned i=0u;i<3u;++i) {
        if(!reader.read(reader.context,base+kParentVtableRva+4u*i,&actual)) {
            out.status=Status::ParentSlotUnreadable;
            return out;
        }
        if(actual!=static_cast<std::uint32_t>(base+kParentMethodRvas[i])) {
            out.status=Status::ParentSlotMismatch;
            return out;
        }
        ++out.parent_slots_matched;
    }
    out.status=Status::Verified;
    return out;
}
} // namespace original_client_entitylist_snapshot
} // namespace sin_independent
