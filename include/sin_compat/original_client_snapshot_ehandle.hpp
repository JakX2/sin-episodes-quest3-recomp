#pragma once
// Phase224: independent portable C++17 model of four original 2006 SiN
// client.dll type-13 EHandle snapshot helper behaviors. It is NOT a
// substitute CClientEntityList, game DLL, or complete Source engine ABI.
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "sin_compat/original_client_snapshot_field_copy.hpp"

namespace sin_compat::original_client_snapshot_ehandle {

using Bytes=std::vector<std::uint8_t>;
namespace copy=original_client_snapshot_field_copy;
inline constexpr std::uint32_t kCompareVA=0x24199060u;
inline constexpr std::uint32_t kCopyVA=0x24197E90u;
inline constexpr std::uint32_t kDiagnosticsVA=0x241994C0u;
inline constexpr std::uint32_t kPreviewVA=0x241995E0u;
inline constexpr std::uint32_t kCompareCall=0x24199A93u;
inline constexpr std::uint32_t kCopyCall=0x24199AA2u;
inline constexpr std::uint32_t kDiagnosticsCall=0x24199AB1u;
inline constexpr std::uint32_t kPreviewCall=0x24199AC0u;
inline constexpr std::uint32_t kOriginalEntityListGlobal=0x242FA734u;

inline constexpr std::uint32_t kInvalidHandle=0xFFFFFFFFu;
inline constexpr std::uint32_t kEntryIndexMask=0x00000FFFu;
inline constexpr std::uint32_t kSerialShift=12u;
inline constexpr std::size_t kEntryStrideBytes=16u;
inline constexpr std::size_t kEntryBaseOffset=4u;

struct OriginalFields {
    std::uint8_t enabled_14{};          // native BYTE(this+0x14)
    std::uint8_t descriptor_flags_13{}; // native BYTE([this+0x18]+0x13)
    std::uint8_t copy_enabled_2c{};     // native BYTE(this+0x2C)
    std::uint32_t descriptor_token_18{};
    std::uint32_t preview_token_34{};
};

// An independently supplied identity token replaces a native x86 entity
// pointer, without guessing an entity class layout or giving ownership.
// The original accesses slots at [global CClientEntityList + 4 + index*16]:
// DWORD+0 = pointer, DWORD+4 = serial. The global's base address comes
// from an original .data pointer at 0x242FA734.
struct OriginalEntitySlot {
    std::uint32_t entity_pointer{};
    std::uint32_t serial{};
};
struct IEntityListHost {
    virtual ~IEntityListHost()=default;
    virtual std::optional<OriginalEntitySlot> slot(std::uint32_t index) const=0;
    // Original first tries method 0x240C5590 to obtain a name. If it
    // returns an empty string, native calls 0x24248F05 and 0x24249896
    // to obtain a fallback. Those methods are unresolved and deliberately
    // exposed as separate, host-controlled callbacks.
    virtual std::string entity_name(std::uint32_t entity_pointer)=0;
    virtual std::string fallback_entity_name(std::uint32_t entity_pointer)=0;
};

inline std::uint32_t resolve_entity(std::uint32_t handle,
                                    const IEntityListHost& registry) {
    if(handle==kInvalidHandle)return 0u;
    const auto entry=registry.slot(handle & kEntryIndexMask);
    if(!entry || entry->serial!=(handle >> kSerialShift))
        return 0u;
    return entry->entity_pointer;
}
inline std::uint32_t load_u32(const Bytes& bytes,std::size_t off) {
    if(off>bytes.size() || bytes.size()-off<4u)
        throw std::out_of_range("host-only original EHandle pointer guard");
    return std::uint32_t(bytes[off]) |
        (std::uint32_t(bytes[off+1u])<<8u) |
        (std::uint32_t(bytes[off+2u])<<16u) |
        (std::uint32_t(bytes[off+3u])<<24u);
}

// Native 160-byte comparator 0x24199060, RET 12:
// disabled -> 0; descriptor flag 0x04 -> 1; signed count <=0 -> 1.
// Otherwise resolves both EHandle DWORDs in each record and compares
// entity pointer IDs. DIFFERENT raw handles resolving to the same
// entity pointer (even both null) compare EQUAL.
inline std::uint32_t compare_resolved_entities(
        const OriginalFields& fields,
        const Bytes& predicted,std::size_t predicted_off,
        const Bytes& net,std::size_t net_off,
        std::int32_t count,const IEntityListHost& registry) {
    if(fields.enabled_14==0u)return 0u;
    if((fields.descriptor_flags_13&0x04u)!=0u)return 1u;
    if(count<=0)return 1u;
    for(std::size_t i=0;i<static_cast<std::size_t>(count);++i) {
        if(i>std::numeric_limits<std::size_t>::max()/4u)
            throw std::overflow_error("host-only EHandle record index overflow");
        const auto delta=i*4u;
        if(predicted_off>std::numeric_limits<std::size_t>::max()-delta ||
           net_off>std::numeric_limits<std::size_t>::max()-delta)
            throw std::overflow_error("host-only EHandle pointer offset overflow");
        const auto pred=resolve_entity(
            load_u32(predicted,predicted_off+delta),registry);
        const auto source=resolve_entity(
            load_u32(net,net_off+delta),registry);
        if(pred!=source)return 0u;
    }
    return 1u;
}

// Native 49-byte copy at 0x24197E90, RET 16.
// It skips on !this+0x2C, operation == 1, or signed count <=0.
// The original copies one complete DWORD per forward iteration; it is
// not memmove and overlapping source/destination can cascade.
inline void copy_ehandles(const OriginalFields& fields,
        std::uint32_t status,const Bytes& net,std::size_t net_off,
        Bytes& predicted,std::size_t predicted_off,std::int32_t count) {
    if(!copy::should_copy({fields.copy_enabled_2c},status) || count<=0)
        return;
    const auto unsigned_count=static_cast<std::uint32_t>(count);
    if(unsigned_count>std::numeric_limits<std::uint32_t>::max()/4u)
        throw std::overflow_error("host-only original EHandle byte-count overflow");
    copy::forward_copy(net,net_off,predicted,predicted_off,
        static_cast<std::size_t>(unsigned_count)*4u);
}

enum class TraceKind {
    Difference, DiagnosticEntity, DiagnosticNull,
    PreviewEntity, PreviewNull
};
struct TraceEvent {
    TraceKind kind{};
    std::uint32_t status{};
    std::uint32_t net_pointer{};
    std::uint32_t predicted_pointer{};
    std::string entity_name{};
};
struct ITraceSink {
    virtual ~ITraceSink()=default;
    virtual void on_event(const TraceEvent& value)=0;
};
inline std::string display_name(IEntityListHost& registry,
                                std::uint32_t entity_ptr) {
    const auto primary=registry.entity_name(entity_ptr);
    if(!primary.empty())return primary;
    return registry.fallback_entity_name(entity_ptr);
}

// Native 288-byte diagnostics method 0x241994C0, RET 16.
// This+0x14 disabled -> no logging. Status zero emits the native
// "EHandles differ (net) 0x%p (pred) 0x%p\n" with resolved pointers,
// then in all enabled cases prints the first destination/predicted
// handle as either "EHandle (NULL)" or "EHandle (0x%p->%s)".
// Both diagnostics run AFTER the original router copy helper.
inline void diagnostics(const OriginalFields& fields,std::uint32_t status,
        const Bytes& predicted,std::size_t predicted_off,
        const Bytes& net,std::size_t net_off,
        IEntityListHost& registry,ITraceSink& sink) {
    if(fields.enabled_14==0u)return;
    const auto predicted_pointer=resolve_entity(
        load_u32(predicted,predicted_off),registry);
    if(status==0u) {
        const auto net_pointer=resolve_entity(
            load_u32(net,net_off),registry);
        sink.on_event({TraceKind::Difference,status,net_pointer,
                       predicted_pointer,{}});
    }
    if(predicted_pointer==0u) {
        sink.on_event({TraceKind::DiagnosticNull,status,0u,0u,{}});
        return;
    }
    sink.on_event({TraceKind::DiagnosticEntity,status,0u,
                   predicted_pointer,display_name(registry,predicted_pointer)});
}

// Native 175-byte preview at 0x241995E0, RET 16.
// Pointer-token identity, NOT enabled_14, gates the preview. Original
// resolved entity/name and fallback/null branches are preserved.
inline void preview(const OriginalFields& fields,
        const Bytes& predicted,std::size_t predicted_off,
        IEntityListHost& registry,ITraceSink& sink) {
    if(fields.preview_token_34!=fields.descriptor_token_18)return;
    const auto predicted_pointer=resolve_entity(
        load_u32(predicted,predicted_off),registry);
    if(predicted_pointer==0u) {
        sink.on_event({TraceKind::PreviewNull,0u,0u,0u,{}});
        return;
    }
    sink.on_event({TraceKind::PreviewEntity,0u,0u,
                   predicted_pointer,display_name(registry,predicted_pointer)});
}

// Original Phase212 type-13 case: EHandle pointer comparison → conditional
// forward DWORD copy → diagnostic → preview, with precisely these four
// native direct CALL target addresses. The pointer registry, original
// formatter receivers and Source engine lifecycle remain intentionally
// outside this clean-room reference.
inline std::uint32_t run_type13_direct_calls(const OriginalFields& fields,
        const Bytes& net,std::size_t net_off,
        Bytes& predicted,std::size_t predicted_off,
        std::int32_t signed_dword_count,
        IEntityListHost& registry,ITraceSink& sink) {
    const auto status=compare_resolved_entities(
        fields,predicted,predicted_off,net,net_off,
        signed_dword_count,registry);
    copy_ehandles(fields,status,net,net_off,predicted,predicted_off,
                  signed_dword_count);
    diagnostics(fields,status,predicted,predicted_off,
                net,net_off,registry,sink);
    preview(fields,predicted,predicted_off,registry,sink);
    return status;
}

} // namespace sin_compat::original_client_snapshot_ehandle
