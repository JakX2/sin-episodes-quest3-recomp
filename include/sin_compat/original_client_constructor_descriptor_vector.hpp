#pragma once
// Phase1727: valid-input behavior of the original 12-byte descriptor
// insertion 0x24061220 and capacity growth 0x24060870. No native pointers.
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace sin_compat::original_client_constructor_descriptor_vector {
using Descriptor=std::array<std::uint32_t,3>;
static_assert(sizeof(Descriptor)==12u);
inline constexpr std::int32_t kMaxRecords=std::numeric_limits<std::int32_t>::max()/12;
struct State final {
    std::int32_t capacity=0;
    std::int32_t growBy=0;
    std::vector<Descriptor> records;
    bool cacheRefreshed=false;
};
struct Result final {
    std::int32_t index;
    bool grew;
    bool shifted;
};
// Original starts at three 12-byte records, then doubles or adds the
// positive grow amount. Negative one means external storage/no growth.
// Signed overflow and unsafe native byte counts are deliberately rejected.
inline std::optional<std::int32_t> PlanCapacity(
    std::int32_t capacity,std::int32_t required,std::int32_t growBy) noexcept {
    if(capacity<0 || required<0 || capacity>kMaxRecords ||
       required>kMaxRecords || growBy< -1)return std::nullopt;
    if(required<=capacity)return capacity;
    if(growBy== -1)return std::nullopt;
    std::int64_t next=capacity==0?3:capacity;
    if(next<required && growBy>0) {
        const auto delta=static_cast<std::int64_t>(required)-next;
        next+=((delta+growBy-1)/growBy)*growBy;
    }else while(next<required)next*=2;
    if(next>kMaxRecords)return std::nullopt;
    return static_cast<std::int32_t>(next);
}
// All three DWORDs are opaque, including possible source object tokens.
// We snapshot the input by value rather than reproducing aliasing UB.
// std::vector owns storage; no original allocator or pointer is invoked.
inline std::optional<Result> Insert(State& state,std::int32_t index,Descriptor value) {
    if(state.records.size()>=static_cast<std::size_t>(kMaxRecords) ||
       state.capacity<0 || state.records.size()>static_cast<std::size_t>(state.capacity) ||
       index<0 || static_cast<std::size_t>(index)>state.records.size())return std::nullopt;
    const auto count=static_cast<std::int32_t>(state.records.size());
    const auto capacity=PlanCapacity(state.capacity,count+1,state.growBy);
    if(!capacity)return std::nullopt;
    const bool grew=*capacity!=state.capacity;
    state.records.insert(state.records.begin()+index,value);
    state.capacity=*capacity;
    state.cacheRefreshed=true;
    return Result{index,grew,index<count};
}
} // namespace sin_compat::original_client_constructor_descriptor_vector
