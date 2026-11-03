#pragma once
// SiN Episodes 2006 original x86 record dispatch and client shutdown reference.
// This does NOT parse cache record payloads or implement original vtables.
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace sin_compat::record_dispatch {
inline constexpr std::uint16_t invalid_record_index = 0xffffu;
inline constexpr std::size_t original_record_stride = 24;
inline constexpr std::size_t scene_new_record_object_bytes = 0x1c;
inline constexpr std::size_t model_sounds_new_record_object_bytes = 0x18;
struct RecordWords32 { std::uint32_t words[6]{}; };
static_assert(sizeof(RecordWords32)==original_record_stride);
inline constexpr std::size_t record_value_offset=0x0c;
inline constexpr std::size_t record_handler_index_offset=0x14;
inline constexpr std::size_t original_record_reader_view_offset=0x08;

// The original cache helpers return unsigned 16-bit record indices and
// use 0xffff as a missing-index sentinel. This host adapter only tracks
// that branch and preserves the 24-byte original in-memory slot layout.
template<class Lookup, class Insert>
std::uint16_t find_or_insert_record(Lookup&& lookup,Insert&& insert) {
    const std::uint16_t existing=lookup();
    return existing==invalid_record_index ? insert() : existing;
}

// Original reader starts from base+index*24+8, writes +4 relative
// to that view (= base+index*24+0x0C) and fetches handler index
// from view+0x0C (= base+index*24+0x14), matching the insert helper.
// Selected handler is called through original virtual byte offset +0x04.
// All pointers and ownership are host-owned; no x86 pointer cast to ARM64.
template<class LookupHandler, class Invoke>
void dispatch_record(RecordWords32& record,std::uint32_t value,
                     LookupHandler&& lookup_handler,Invoke&& invoke) {
    record.words[record_value_offset/4]=value;
    auto& handler=lookup_handler(record.words[record_handler_index_offset/4]);
    invoke(handler,record);
}
} // namespace sin_compat::record_dispatch

namespace sin_compat::client_shutdown {
struct LateShutdownCall {
    std::uint32_t original_global_va;
    std::uint8_t virtual_byte_offset;
};
inline constexpr std::array<LateShutdownCall,8> original_late_calls{{
    {0x2431dc90u,0x04}, {0x2431dea4u,0x04},
    {0x2431dd00u,0x04}, {0x2431de3cu,0x04},
    {0x2431ddd0u,0x04}, {0x2431dbecu,0x1c},
    {0x2431df40u,0x04}, {0x2431eebcu,0x04}
}};
inline constexpr std::uint32_t optional_interface_global_va=0x2437f8a0u;
inline constexpr std::uint8_t optional_virtual_byte_offset=0x0c;
inline constexpr std::uint8_t final_virtual_byte_offset=0x14;

// Mirrors the proven order in client.dll 0x241EE0C0. The pre-call
// 0x2422ECE0 and final interface-acquisition 0x24001060 are supplied
// as callbacks because their game semantics are not recovered.
template<class Pre, class Dispatch, class Optional, class Final>
void run_late_shutdown(Pre&& pre,Dispatch&& dispatch,
                       bool optional_present,Optional&& optional,Final&& final) {
    pre();
    for(const auto& call:original_late_calls)
        dispatch(call.original_global_va,call.virtual_byte_offset);
    if(optional_present)optional(optional_interface_global_va,optional_virtual_byte_offset);
    final(final_virtual_byte_offset);
}

template<class Release>
void release_optional_global(void*& object,Release&& release) {
    if(object)release(object,1);
    object=nullptr;
}
} // namespace sin_compat::client_shutdown
