#pragma once
// Phase190 original 2006 server.dll cache-record identity flow.
// Original native addresses and branch order are verified; host callbacks
// replace all opaque engine, allocator and record-key helper implementations.
#include <cstdint>

namespace sin_compat::original_server_cache_record_identity {
inline constexpr std::uint32_t kFileSystemGlobalVA=0x225721A4u;
inline constexpr std::uint32_t kOpaqueFileReadVirtualOffset=0xD4u;
inline constexpr std::uint16_t kMissingIndex=0xFFFFu;
inline constexpr std::uint32_t kSlotStride=24u;
inline constexpr std::uint32_t kSlotValueOffset=0x14u;

struct Config {
    std::uint32_t native_entry_va;
    std::uint32_t record_vtable_va;
    std::uint32_t allocation_size_bytes;
    std::uint32_t find_existing_va;
    std::uint32_t append_payload_va;
    std::uint32_t insert_key_va;
};
inline constexpr Config kScene{0x221ACFD0u,0x2242DCA0u,28u,
                               0x221A7FB0u,0x221A6620u,0x221ACD00u};
inline constexpr Config kModelSounds{0x22149770u,0x2240ABC8u,24u,
                                     0x22142670u,0x22140810u,0x22146EF0u};

// Original ECX=this, one file-handle stack argument and RET 4. The method
// reads one opaque value through filesystem vtable +0xD4, calls a
// repository-specific key lookup and returns existing AX unless ==0xFFFF.
// On insertion it allocates 28/24 bytes, conditionally initializes non-null
// memory, uses original-specific payload insertion helpers, then writes a
// payload pointer at [slot_base + 24*new_index + 0x14].
// This host-neutral approximation deliberately does not invent key formats
// or memory ownership. The original still invokes append when alloc returns 0.
// Required Host callbacks: opaque_read, find_existing, alloc_record,
// construct_record, append_payload, insert_key, publish_payload.
template<class Host>
std::uint16_t lookup_or_insert(const Config& cfg,std::uintptr_t file_handle,Host& h) {
    const std::uint32_t opaque=h.opaque_read(file_handle,kOpaqueFileReadVirtualOffset);
    const std::uint16_t existing=h.find_existing(cfg,opaque);
    if(existing!=kMissingIndex) return existing;
    const std::uintptr_t block=h.alloc_record(cfg.allocation_size_bytes);
    if(block) h.construct_record(block,cfg.record_vtable_va,cfg.allocation_size_bytes);
    const std::uintptr_t payload=h.append_payload(cfg,block);
    const std::uint16_t index=h.insert_key(cfg,opaque);
    h.publish_payload(cfg,index,payload,kSlotStride,kSlotValueOffset);
    return index;
}
} // namespace sin_compat::original_server_cache_record_identity
