#pragma once
// Phase175: original 2006 SiN server.dll 12-byte symbol-tree storage helpers.
// Exact native branches; original MSVC x86 allocator, tree balancing and
// engine-owned memory remain host callbacks rather than guessed SDK code.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace sin_compat::original_server_symbol_tree_storage {
inline constexpr std::uint16_t kInvalidSlot=0xffffu;
inline constexpr std::uint32_t kRecordStride=12u;

// Fields identified from original 0x2228F460. Record offsets are
// +0 left child, +2 right child, +4 parent, +6 zeroed metadata, +8 payload.
struct Node12 {
    std::uint16_t left= kInvalidSlot;
    std::uint16_t right=kInvalidSlot;
    std::uint16_t parent=kInvalidSlot;
    std::uint16_t metadata=0;
    std::uint32_t payload=0;
};
static_assert(sizeof(Node12)==12, "original 12-byte symbol record");
static_assert(offsetof(Node12,right)==2 && offsetof(Node12,parent)==4 &&
              offsetof(Node12,metadata)==6 && offsetof(Node12,payload)==8,
              "original symbol record offsets");

// 0x2228E0E0, 17 native bytes. Native returns [this+4]+12*WORD(index)
// with no bounds check, even when the index is 0xFFFF. The reference
// exposes the exact byte offset, leaving address mapping to the host.
inline constexpr std::uint32_t original_node_byte_offset(
    std::uint16_t index) noexcept {
    return static_cast<std::uint32_t>(index)*kRecordStride;
}

// 0x2228E450, 24 bytes: [this+4]+12*index+2 <- raw 16-bit child.
// The host vector uses bounds-checking, a deliberate safety difference
// from the original unchecked pointer store.
inline void original_set_right_link(std::vector<Node12>& nodes,
                                    std::uint16_t index,
                                    std::uint16_t right_child) {
    nodes.at(index).right=right_child;
}

struct SymbolSlots {
    std::uint16_t free_head=kInvalidSlot;  // native object +0x14
    std::uint16_t used_count=0;            // +0x16, wraps in uint16_t
    std::uint32_t capacity=0;              // +0x08
    std::uintptr_t allocation_token=0;    // pointer at +0x04
    std::uintptr_t cached_allocation=0;   // pointer at +0x18
};

// Original 0x2228E620, 86 native bytes. The fresh branch checks capacity
// against the current zero-extended WORD count, calls grow(+1) if equal,
// then caches the current buffer pointer and increments the WORD counter.
// The recycle branch uses the old head, reads WORD at record+2 via native
// 0x2228E100, updates cached buffer, then replaces the free-list head.
// Neither branch resets the returned node (linking does this later).
template<class Host>
std::uint16_t original_take_symbol_slot(SymbolSlots& slots,Host& host) {
    if(slots.free_head == kInvalidSlot) {
        if(slots.capacity == static_cast<std::uint32_t>(slots.used_count))
            host.grow_slot_storage(slots,1u); // original 0x2228E200
        const std::uint16_t slot=slots.used_count;
        slots.cached_allocation=slots.allocation_token;
        slots.used_count=static_cast<std::uint16_t>(slot+1u);
        return slot;
    }
    const std::uint16_t slot=slots.free_head;
    const std::uint16_t next=host.read_recycled_right_link(slots,slot);
    slots.cached_allocation=slots.allocation_token;
    slots.free_head=next;
    return slot;
}

// 0x2228F460, 109 native bytes. Entry receives a child slot, parent slot,
// and truthy 8-bit direction. Initializes all four WORD metadata fields,
// routes child as root/left/right, and always invokes original 0x2228EDF0
// rebalance callback *after* wiring the links. Payload DWORD at node+8
// is deliberately untouched, matching the native code.
template<class Host>
void original_link_symbol_node(std::vector<Node12>& nodes,
                              std::uint16_t& root,
                              std::uint16_t child,
                              std::uint16_t parent,
                              std::uint8_t direction,
                              Host& host) {
    Node12& node=nodes.at(child);
    node.parent=parent;
    node.left=kInvalidSlot;
    node.right=kInvalidSlot;
    node.metadata=0;
    if(parent==kInvalidSlot) {
        root=child;
    } else if(direction!=0) {
        nodes.at(parent).left=child;
    } else {
        nodes.at(parent).right=child;
    }
    host.rebalance_after_link(child); // original 0x2228EDF0
}

struct AppendStorage {
    std::uint32_t used=0;              // +0x0C
    std::uint32_t capacity=0;          // +0x04
    std::uintptr_t allocation_token=0; // +0x00
    std::uintptr_t cached_allocation=0; // +0x10
};
// Read uint32 as signed two's-complement exactly as x86 JLE does.
inline std::int32_t native_signed32(std::uint32_t bits) noexcept {
    std::int32_t result{};
    static_assert(sizeof(result)==sizeof(bits));
    std::memcpy(&result,&bits,sizeof(result));
    return result;
}

// 0x2228E680, 46 bytes. Compute used+requested with DWORD wrap and
// compare to capacity *signed*. If greater, reserve (used-capacity+req)
// additional DWORD units using 0x2228E280. Then add req to used with
// wrap and copy current allocation pointer into cached pointer. No
// original overflow guard is fabricated, including negative-as-signed req.
template<class Host>
void original_append_reserve(AppendStorage& storage,
                             std::uint32_t requested,Host& host) {
    const std::uint32_t future=storage.used+requested;
    if(native_signed32(future)>native_signed32(storage.capacity)) {
        const std::uint32_t extra=storage.used-storage.capacity+requested;
        host.reserve_extra(storage,extra); // original 0x2228E280
    }
    storage.used+=requested;
    storage.cached_allocation=storage.allocation_token;
}
} // namespace sin_compat::original_server_symbol_tree_storage