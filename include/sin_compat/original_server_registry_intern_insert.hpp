#pragma once
// Phase173: original 2006 SiN Episodes server.dll registration-table insertion,
// free-list slot acquisition, pool selection and string interning.
// Source MSVC x86 ABI/allocators remain external host callbacks.
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>

namespace sin_compat::original_server_registry_intern_insert {
inline constexpr std::uint16_t kMissingSlot=0xffffu;
inline constexpr std::uint32_t kMinStringPoolCapacity=0x800u;
inline constexpr std::uint32_t kNativeChunkAllocationOverhead=11u;
inline constexpr std::uint32_t kRegistryRecordStride=16u;

struct SlotState {
    std::uint16_t free_head_14=kMissingSlot;
    std::uint16_t used_count_16=0;
    std::uint32_t capacity_08=0;
    std::uint16_t insertions_12=0;
};

// Original 0x2228A3B0 (98 bytes): FIFO/count is not assumed, free-list
// pointer follows record+0x02. The original only grows when count==capacity,
// writes 0xffff to record+0x08, caches storage base at object+0x18.
template<class Host>
std::uint16_t original_allocate_registry_slot(SlotState& self,Host& host) {
    std::uint16_t slot;
    if (self.free_head_14==kMissingSlot) {
        if (self.capacity_08==static_cast<std::uint32_t>(self.used_count_16))
            host.expand_registry_capacity(self,1u); // 0x22288390
        slot=self.used_count_16;
        self.used_count_16=static_cast<std::uint16_t>(self.used_count_16+1u);
    } else {
        slot=self.free_head_14;
        self.free_head_14=host.next_recycled_slot(slot); // 0x22288300
    }
    host.reset_record_word_at_plus8(slot,kMissingSlot);
    host.capture_slot_storage_base();
    return slot;
}

struct RegistryKeyWords {
    std::uint16_t first_word=0;
    std::uint32_t word_at_plus4=0;
};
struct PreparedInsert {
    std::uint16_t first=0;
    std::uint16_t second=0;
};

// Original 0x2228B0A0 (102 bytes). The native pre-insert helper
// 0x2228A280 is opaque and NOT labelled a successful find. Native code
// calls slot allocator 0x2228A3B0 unconditionally after that helper.
// On the normal native path, writes key[0] to record+8 and key[4] to +12.
template<class Host>
std::uint16_t original_insert_registry_record(SlotState& state,
        const RegistryKeyWords& key,std::uint32_t argument2,Host& host) {
    const PreparedInsert prep=host.prepare_table_insertion(key,argument2);
    const std::uint16_t slot=original_allocate_registry_slot(state,host);
    host.link_new_registry_slot(slot,prep.first,prep.second); // 0x2228AF00
    state.insertions_12=static_cast<std::uint16_t>(state.insertions_12+1u);
    host.write_key_to_record_plus8(slot,key.first_word,key.word_at_plus4);
    return slot;
}

struct StringPoolChunk {
    std::uint32_t capacity=0;
    std::uint32_t used=0;
};

// Original 0x2228EA30 (49 bytes): first 32-bit signed free>=requested.
// Native subtraction uses 32-bit modular arithmetic and signed comparison.
inline std::int32_t native_signed32(std::uint32_t bits) noexcept {
    std::int32_t out=0;
    static_assert(sizeof(out)==sizeof(bits),"native signed 32-bit x86 layout");
    std::memcpy(&out,&bits,sizeof(out));
    return out;
}
inline std::int32_t original_first_pool_with_space(
    const std::vector<StringPoolChunk>& chunks,std::uint32_t requested) noexcept {
    const auto wanted=native_signed32(requested);
    for (std::size_t i=0;i<chunks.size();++i) {
        const auto free=native_signed32(chunks[i].capacity-chunks[i].used);
        if (free>=wanted) return static_cast<std::int32_t>(i);
    }
    return -1;
}

// Original 0x2228F7A0 (261 bytes). Calls 0x2228EB40 to find the symbol
// before strlen; missed names are inserted into a pool, with at least
// 2048-byte capacity and an allocation of capacity+11 bytes.
// The actual native string-chunk metadata allocator 0x2228ED80,
// entry-index function 0x2228F710 and pre-intern lookup 0x2228EB40
// remain external. All callbacks preserve native observable call order.
template<class Host>
std::uint16_t original_intern_classname(const char* name,Host& host) {
    if (name==nullptr) return kMissingSlot;
    host.set_pending_intern_name(name); // this+0x30 scratch
    const std::uint16_t found=host.find_interned_name(name); // 0x2228EB40
    if (found!=kMissingSlot) return found;

    // Original x86 byte scan includes the terminating NUL in copied bytes.
    const std::uint32_t needed=static_cast<std::uint32_t>(std::strlen(name)+1u);
    std::int32_t index=host.first_pool_with_space(needed); // 0x2228EA30
    if (index==-1) {
        const std::uint32_t capacity=needed>kMinStringPoolCapacity
            ? needed:kMinStringPoolCapacity;
        index=host.allocate_pool_with_header(capacity,
                capacity+kNativeChunkAllocationOverhead); // alloc + 0x2228ED80
    }
    const auto old_used=host.pool_bytes_used(static_cast<std::uint32_t>(index));
    const auto offset16=static_cast<std::uint16_t>(old_used);
    host.copy_name_into_pool(static_cast<std::uint32_t>(index),old_used,
                             name,needed);
    host.advance_pool_used(static_cast<std::uint32_t>(index),needed);
    return host.register_interned_locator(
        static_cast<std::uint16_t>(index),offset16); // 0x2228F710
}
} // namespace sin_compat::original_server_registry_intern_insert