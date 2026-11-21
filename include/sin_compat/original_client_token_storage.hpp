#pragma once
// Independently authored bounded reference for 2006 client.dll token storage.
// Original virtual code is not copied, linked, called or shipped here.
// Exact original call/offset evidence: research Phase547 and its SHA auditor.
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace sin_compat::original_client_string_tokens::storage {

inline constexpr std::uint16_t kMissingToken=0xFFFF;
inline constexpr std::uint32_t kNodeStride=12;
inline constexpr std::uint32_t kPagePointerStride=4;
inline constexpr std::uint32_t kInitialNodeCapacity=3;
inline constexpr std::uint32_t kInitialPointerCapacity=8;

// These describe native 32-bit array-container fields, not host pointers.
struct NativeArrayHeader {
    std::uint32_t data_address_native32=0; // original container +0x00
    std::int32_t capacity=0;              // +0x04
    std::int32_t grow_increment=0;        // +0x08, -1 suppresses allocation
};
static_assert(sizeof(NativeArrayHeader)==12);
static_assert(offsetof(NativeArrayHeader,capacity)==4);
static_assert(offsetof(NativeArrayHeader,grow_increment)==8);

// This original helper checks the missing node BEFORE touching the native
// backing array. Return an offset, not a simulated native pointer.
struct NodeLocation {
    bool global_missing_sentinel=false;
    std::uint32_t array_byte_offset=0;
};
constexpr NodeLocation node_location(std::uint16_t index) noexcept {
    return index==kMissingToken
        ? NodeLocation{true,0}
        : NodeLocation{false,static_cast<std::uint32_t>(index)*kNodeStride};
}

enum class PlanStatus:std::uint8_t {
    Valid,                 // next_capacity / bytes support a host allocation
    FixedStorage,          // the original -1 grow mode early-exits
    InvalidInput,          // host-side guard; not original behavior
    Overflow               // host-side guard; not original behavior
};
struct GrowthPlan {
    PlanStatus status=PlanStatus::InvalidInput;
    std::uint32_t next_capacity=0;
    std::uint32_t allocation_bytes=0;
    bool realloc_existing=false;
    bool growth_needed=false;
};

// Models the observed *capacity selection*, not native malloc/realloc failure
// handling. The original may still invoke its allocator even when the target
// fits, so callers should not use `growth_needed` as a drop-in call contract.
constexpr GrowthPlan plan_capacity(std::int32_t existing_capacity,
                                   std::int32_t growth_increment,
                                   std::uint32_t additional_elements,
                                   std::uint32_t initial_capacity,
                                   std::uint32_t element_stride,
                                   bool native_buffer_present) noexcept {
    GrowthPlan out{};
    if(growth_increment==-1){out.status=PlanStatus::FixedStorage;return out;}
    if(existing_capacity<0 || growth_increment<0 || initial_capacity==0 ||
       element_stride==0){return out;}
    constexpr auto limit=static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
    const auto cap=static_cast<std::uint32_t>(existing_capacity);
    if(additional_elements>limit-cap){out.status=PlanStatus::Overflow;return out;}
    const auto requested=cap+additional_elements;
    auto next=cap;
    bool grew=false;
    // The original uses signed x86 comparisons and unchecked 32-bit add and
    // multiply. Fail closed in the portable reference instead of wrapping.
    while(next<requested){
        grew=true;
        std::uint32_t step = next==0 ? initial_capacity
                            : growth_increment==0 ? next
                            : static_cast<std::uint32_t>(growth_increment);
        if(step==0 || step>limit-next){out.status=PlanStatus::Overflow;return out;}
        next+=step;
    }
    if(next>std::numeric_limits<std::uint32_t>::max()/element_stride){
        out.status=PlanStatus::Overflow;return out;
    }
    out.status=PlanStatus::Valid;
    out.next_capacity=next;
    out.allocation_bytes=next*element_stride;
    out.realloc_existing=native_buffer_present;
    out.growth_needed=grew;
    return out;
}

constexpr GrowthPlan node_reserve(std::int32_t current,std::int32_t grow,
                                  std::uint32_t additional,bool native_buffer) noexcept {
    return plan_capacity(current,grow,additional,kInitialNodeCapacity,
                         kNodeStride,native_buffer);
}
constexpr GrowthPlan page_pointer_reserve(std::int32_t current,std::int32_t grow,
                                          std::uint32_t additional,bool native_buffer) noexcept {
    return plan_capacity(current,grow,additional,kInitialPointerCapacity,
                         kPagePointerStride,native_buffer);
}

// At original page+0: capacity; page+4: used bytes. The original scans
// `pages[0..count)` and returns first `capacity-used >= required`.
struct PageHeader {
    std::uint32_t total_bytes=0;
    std::uint32_t used_bytes=0;
};
static_assert(sizeof(PageHeader)==8);
inline std::optional<std::size_t> first_page_with_space(
    const std::vector<PageHeader>& pages,std::uint32_t requested) noexcept {
    for(std::size_t i=0;i<pages.size();++i){
        const auto& p=pages[i];
        // Corrupt-page rejection is independent host safety.
        if(p.used_bytes>p.total_bytes)continue;
        if(p.total_bytes-p.used_bytes>=requested)return i;
    }
    return std::nullopt;
}

} // namespace sin_compat::original_client_string_tokens::storage