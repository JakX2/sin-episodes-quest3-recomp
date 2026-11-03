#pragma once
// Original 2006 SiN server cache header + client final cleanup.
// Host-only C++17. Not a full on-disk cache decoder or SourceVR ARM64 ABI.
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace sin_compat::lifecycle {
inline constexpr std::uint32_t cache_header_magic=2u;
inline constexpr std::uint32_t scene_cache_format=3u;
inline constexpr std::uint32_t model_sounds_cache_format=2u;
inline constexpr std::size_t cache_header_bytes=16;

struct CacheHeader {
    std::uint32_t magic=0, format=0, source_key=0;
    std::int32_t record_count=0;
};
inline std::uint32_t little_endian_word(const std::uint8_t* p) noexcept {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1])<<8u)
         | (std::uint32_t(p[2])<<16u) | (std::uint32_t(p[3])<<24u);
}
inline bool read_cache_header(const std::uint8_t* data,std::size_t n,CacheHeader& h) noexcept {
    if(!data || n<cache_header_bytes)return false;
    h.magic=little_endian_word(data);
    h.format=little_endian_word(data+4);
    h.source_key=little_endian_word(data+8);
    h.record_count=static_cast<std::int32_t>(little_endian_word(data+12));
    return true;
}
enum class CacheHeaderStatus {InsufficientData,WrongMagic,WrongFormat,
                              ChangedSourceKey,HeaderMatches};
struct CacheHeaderResult {
    CacheHeaderStatus status;
    bool may_parse_payload;
    bool mark_dirty_and_continue;
    std::int32_t record_count;
};
inline CacheHeaderResult validate_cache_header(const std::uint8_t* data,std::size_t n,
                                                std::uint32_t expected_format,
                                                std::uint32_t expected_source_key) noexcept {
    CacheHeader h{};
    if(!read_cache_header(data,n,h))
        return {CacheHeaderStatus::InsufficientData,false,true,0};
    if(h.magic!=cache_header_magic)
        return {CacheHeaderStatus::WrongMagic,false,true,h.record_count};
    if(h.format!=expected_format)
        return {CacheHeaderStatus::WrongFormat,false,true,h.record_count};
    // Original x86 skips this comparison if callback-produced key is zero.
    if(expected_source_key!=0 && h.source_key!=expected_source_key)
        return {CacheHeaderStatus::ChangedSourceKey,false,true,h.record_count};
    return {CacheHeaderStatus::HeaderMatches,true,false,h.record_count};
}
inline CacheHeaderResult scene_header(const std::uint8_t* data,std::size_t n,
                                      std::uint32_t key) noexcept {
    return validate_cache_header(data,n,scene_cache_format,key);
}
inline CacheHeaderResult model_sounds_header(const std::uint8_t* data,std::size_t n,
                                             std::uint32_t key) noexcept {
    return validate_cache_header(data,n,model_sounds_cache_format,key);
}
// Original client 0x241E5050 releases optional global via virtual +0(1)
// then zeros it. Host controls the lifetime and actual release.
template<class Release>
void release_client_global(void*& ptr,Release&& release) {
    if(ptr)release(ptr,1);
    ptr=nullptr;
}
struct ClientPointerArrayState {
    std::vector<void*> pointers;
    std::int32_t active_count=0;
    std::int32_t allocation_marker=-1;
    bool heap_buffer_attached=false;
    std::int32_t auxiliary_count=0;
    bool auxiliary_flag=false;
};
// Original 0x24143E80: reverse release virtual +0(1), count=0;
// free allocated array only if marker +0x20 differs from -1.
// Malformed out-of-range counts are safely bounded by host vector size.
template<class Release,class HeapFree>
void cleanup_client_pointer_array(ClientPointerArrayState& s,
                                  Release&& release,HeapFree&& heap_free) {
    std::size_t count=s.active_count>0?static_cast<std::size_t>(s.active_count):0u;
    if(count>s.pointers.size())count=s.pointers.size();
    while(count>0) {
        void* value=s.pointers[--count];
        if(value)release(value,1);
    }
    s.active_count=0;
    if(s.allocation_marker!=-1) {
        if(s.heap_buffer_attached)heap_free();
        s.heap_buffer_attached=false;
        s.auxiliary_count=0;
    }
    s.auxiliary_flag=false;
}
} // namespace sin_compat::lifecycle
