#pragma once
// Phase163: original 2006 SiN Episodes client.dll 0x2400A300 (24-byte
// metadata descriptor insertion) and 0x2400B140 (capacity growth).
// These are clean C++17 *valid-input* behavioral reconstructions of two
// verified 32-bit MSVC functions, NOT native original binary ABI classes.
// Original out-of-bounds indices / invalid capacity/growth can write past
// allocated memory; unlike the x86 source this safe reference refuses them.
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace sin_compat::original_client_property_descriptor_array {

// Six raw DWORDs are copied in native code, including any opaque padding
// in the fifth word. Do not normalize to Color/string or drop bytes.
// Native function 0x2400A300 copies exactly 0x18 bytes via six MOVs.
struct NativeDescriptor24 {
    std::array<std::uint32_t, 6> raw_words{};
};
static_assert(sizeof(NativeDescriptor24)==24,
              "2006 original native property record is six DWORDs");

// Original 0x2400B140 (growth thiscall) works with:
//   [this+0x00] pointer, [this+0x04] capacity,
//   [this+0x08] growth amount (0 = double, -1 = no reallocation).
// The original receives ADDITIONAL capacity requested, and derives
// required_capacity = current capacity + requested_delta.
// For a normal metadata insertion this equals old_count + 1.
// We constrain signed overflow and nonprogressing negative growth safely.
inline std::optional<std::int32_t> plan_original_capacity_growth(
    std::int32_t current_capacity,
    std::int32_t additional_capacity,
    std::int32_t original_grow_amount) noexcept {
    if(current_capacity<0 || additional_capacity<0) return std::nullopt;
    const std::int64_t needed =
        static_cast<std::int64_t>(current_capacity)+additional_capacity;
    if(needed > std::numeric_limits<std::int32_t>::max())
        return std::nullopt;
    if(original_grow_amount==-1 || needed<=current_capacity)
        return current_capacity; // original -1 sentinel exits unconditionally
    if(original_grow_amount < -1) return std::nullopt;
    std::int64_t capacity=current_capacity;
    while(capacity<needed) {
        if(capacity==0) capacity=2;
        else if(original_grow_amount!=0) capacity+=original_grow_amount;
        else capacity*=2;
        if(capacity>std::numeric_limits<std::int32_t>::max())
            return std::nullopt;
    }
    return static_cast<std::int32_t>(capacity);
}

// The native dynamic-array state also has [this+0x0C] count and
// [this+0x10] cached element-storage pointer. The latter gets refreshed
// after each insertion and cannot be represented as an original x86 pointer
// on a portable host, so cache_refreshed records that observable store.
// The portable records vector stores only the logical live 24-byte entries.
struct NativeDescriptorArray {
    std::int32_t capacity=0;
    std::int32_t original_grow_amount=0;
    std::vector<NativeDescriptor24> records{};
    bool cache_refreshed=false;
};
struct InsertionOutcome {
    std::int32_t returned_index=0;
    bool allocated_or_reallocated=false;
    bool shifted_existing_records=false;
};
// Models original 0x2400A300 valid indices 0..count:
//   if (count+1 > capacity) call 0x2400B140 with count+1-capacity,
//   ++count, refresh cached pointer, move (old_count-index)*24 bytes
//   upward by 24 via original 0x24248B50, then copy six descriptor DWORDs.
// Unlike native original code, invalid index, negative states, nonprogressing
// growth and externally owned -1 storage with no room are rejected, never
// subjected to undefined memory writes. Source records must not alias this
// array's live storage (the native routine can observe shifted aliases).
template <class ObserveAllocation>
std::optional<InsertionOutcome> insert_original_descriptor(
    NativeDescriptorArray& array,
    std::int32_t index,
    const NativeDescriptor24& descriptor,
    ObserveAllocation&& on_reallocate) {
    const auto count=static_cast<std::int64_t>(array.records.size());
    if(index<0 || index>count ||
       count>=std::numeric_limits<std::int32_t>::max() ||
       array.capacity<0 ||
       count>array.capacity)
        return std::nullopt;
    const auto needed=static_cast<std::int32_t>(count+1);
    auto new_capacity=array.capacity;
    if(needed>array.capacity) {
        const auto capacity=plan_original_capacity_growth(
            array.capacity,needed-array.capacity,array.original_grow_amount);
        if(!capacity || *capacity<needed) return std::nullopt;
        new_capacity=*capacity;
    }
    const bool grew=new_capacity!=array.capacity;
    if(grew) {
        // Original 0x2400B140 invokes malloc/realloc, before the move
        // and before writing the new descriptor at the insertion slot.
        on_reallocate(array.capacity,new_capacity);
        array.capacity=new_capacity;
    }
    const bool shifted=index<count;
    array.records.insert(array.records.begin()+index,descriptor);
    array.cache_refreshed=true;
    return InsertionOutcome{index,grew,shifted};
}
inline std::optional<InsertionOutcome> insert_original_descriptor(
    NativeDescriptorArray& array,std::int32_t index,
    const NativeDescriptor24& descriptor) {
    return insert_original_descriptor(array,index,descriptor,
                                       [](std::int32_t,std::int32_t){});
}
inline std::optional<InsertionOutcome> append_original_descriptor(
    NativeDescriptorArray& array,const NativeDescriptor24& descriptor) {
    if(array.records.size()>=static_cast<std::size_t>(
            std::numeric_limits<std::int32_t>::max()))
        return std::nullopt;
    return insert_original_descriptor(
        array,static_cast<std::int32_t>(array.records.size()),descriptor);
}
} // namespace sin_compat::original_client_property_descriptor_array
