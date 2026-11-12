#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include "sin_compat/original_server_cache_record_identity.hpp"
namespace rec=sin_compat::original_server_cache_record_identity;

struct Host {
    std::vector<std::string> trace;
    std::uint16_t existing=rec::kMissingIndex;
    std::uint16_t next_index=19;
    std::uintptr_t block=0x1234;
    std::uintptr_t payload=0x5678;
    std::uint32_t vtable=0,allocation=0,seen_key=0;

    std::uint32_t opaque_read(std::uintptr_t handle,std::uint32_t slot) {
        assert(handle==0x42 && slot==0xD4);
        trace.emplace_back("read");
        return 0x12345678;
    }
    std::uint16_t find_existing(const rec::Config& cfg,std::uint32_t key) {
        assert(cfg.record_vtable_va==vtable && key==0x12345678);
        trace.emplace_back("find");seen_key=key;
        return existing;
    }
    std::uintptr_t alloc_record(std::uint32_t n) {
        assert(n==allocation);trace.emplace_back("alloc");return block;
    }
    void construct_record(std::uintptr_t ptr,std::uint32_t vt,std::uint32_t n) {
        assert(ptr==block && vt==vtable && n==allocation);
        trace.emplace_back("construct");
    }
    std::uintptr_t append_payload(const rec::Config&,std::uintptr_t ptr) {
        assert(ptr==block);trace.emplace_back("append");return payload;
    }
    std::uint16_t insert_key(const rec::Config&,std::uint32_t key) {
        assert(key==seen_key);trace.emplace_back("insert");return next_index;
    }
    void publish_payload(const rec::Config&,std::uint16_t idx,std::uintptr_t value,
                         std::uint32_t stride,std::uint32_t offset) {
        assert(idx==next_index && value==payload && stride==24 && offset==0x14);
        trace.emplace_back("publish");
    }
};
int main() {
    static_assert(rec::kScene.native_entry_va==0x221ACFD0u,"scene native");
    static_assert(rec::kModelSounds.native_entry_va==0x22149770u,"model native");
    static_assert(rec::kScene.allocation_size_bytes==28u,"scene allocation");
    static_assert(rec::kModelSounds.allocation_size_bytes==24u,"model allocation");
    for (const auto cfg:{rec::kScene,rec::kModelSounds}) {
        Host h;h.vtable=cfg.record_vtable_va;h.allocation=cfg.allocation_size_bytes;
        assert(rec::lookup_or_insert(cfg,0x42,h)==19);
        assert((h.trace==std::vector<std::string>{"read","find","alloc","construct","append","insert","publish"}));
        h.trace.clear();h.existing=7;
        assert(rec::lookup_or_insert(cfg,0x42,h)==7);
        assert((h.trace==std::vector<std::string>{"read","find"}));
        h.trace.clear();h.existing=rec::kMissingIndex;h.block=0;
        assert(rec::lookup_or_insert(cfg,0x42,h)==19);
        assert((h.trace==std::vector<std::string>{"read","find","alloc","append","insert","publish"}));
    }
}
