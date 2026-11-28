// Phase1658: read-only original client cl_predict ConVar startup snapshot.
// The original prediction slot1 reads m_pParent at client RVA 0x374540,
// then m_pParent->m_nValue (+0x2C). Owner probe full-SHA-authenticates
// unchanged original client.dll BEFORE optional owner-controlled loading.
//
// Synthetic CI only uses invented DWORDs; no original ConVar is invoked.
// Never dereference an arbitrary m_pParent. If it is not the expected
// in-module self parent, report an alias and DO NOT follow that pointer.
#pragma once

#include "original_client_entitylist_readonly_snapshot.h"
#include <cstddef>
#include <cstdint>

namespace sin_independent {
namespace original_client_prediction_convar_snapshot {

namespace abi=original_client_entitylist_snapshot;
constexpr std::uintptr_t kClPredictRva=0x374528u;
constexpr std::uintptr_t kParentFieldOffset=0x18u;
constexpr std::uintptr_t kValueFieldOffset=0x2Cu;
constexpr std::uintptr_t kParentSlotRva=kClPredictRva+kParentFieldOffset;
constexpr std::uintptr_t kValueSlotRva=kClPredictRva+kValueFieldOffset;
static_assert(kParentSlotRva==0x374540u,
              "actual 2006 prediction ConVar m_pParent RVA changed");
static_assert(kValueSlotRva==0x374554u,
              "actual 2006 prediction ConVar integer value RVA changed");

enum class Status {
    VerifiedSelfParent,
    MissingReader,
    InvalidBase,
    ParentUnreadable,
    ParentNull,
    ParentAliasesExternal,
    NumericValueUnreadable,
};
struct Result {
    Status status=Status::MissingReader;
    unsigned successful_reads=0u;
    std::int32_t numeric_value=0;
    bool parent_is_expected_self=false;
    unsigned retail_virtual_invocations=0u;
};
inline const char* StatusName(Status status) noexcept {
    switch(status) {
    case Status::VerifiedSelfParent:return "pinned_original_convar_self_parent";
    case Status::MissingReader:return "missing_reader";
    case Status::InvalidBase:return "invalid_module_base";
    case Status::ParentUnreadable:return "cl_predict_parent_unreadable";
    case Status::ParentNull:return "cl_predict_null_parent";
    case Status::ParentAliasesExternal:return "cl_predict_external_or_unrecognized_parent";
    case Status::NumericValueUnreadable:return "cl_predict_numeric_value_unreadable";
    }
    return "unknown_status";
}
inline Result Observe(std::uintptr_t base,abi::Reader reader) noexcept {
    Result out{};
    if(!reader.read)return out;
    // Original PE32 uses 32-bit absolute pointers and a fixed object RVA.
    // Avoid unsigned wrap before even consulting the fake/owner reader.
    if(base==0u || base>0xFFFFFFFFu-kValueSlotRva-4u) {
        out.status=Status::InvalidBase;
        return out;
    }
    std::uint32_t parent=0u;
    if(!reader.read(reader.context,base+kParentSlotRva,&parent)) {
        out.status=Status::ParentUnreadable;return out;
    }
    ++out.successful_reads;
    if(parent==0u) {
        out.status=Status::ParentNull;return out;
    }
    if(parent!=static_cast<std::uint32_t>(base+kClPredictRva)) {
        // A registered ConVar alias could legitimately point elsewhere.
        // This stand-alone readback intentionally refuses to dereference it.
        out.status=Status::ParentAliasesExternal;return out;
    }
    out.parent_is_expected_self=true;
    std::uint32_t value=0u;
    if(!reader.read(reader.context,base+kValueSlotRva,&value)) {
        out.status=Status::NumericValueUnreadable;return out;
    }
    ++out.successful_reads;
    // Preserve all 32 input bits, including negative values; original Init
    // tests only whether the numeric word is zero.
    out.numeric_value=static_cast<std::int32_t>(value);
    out.status=Status::VerifiedSelfParent;
    return out;
}
} // namespace original_client_prediction_convar_snapshot
} // namespace sin_independent
