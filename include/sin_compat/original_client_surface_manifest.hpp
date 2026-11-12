#pragma once
// Phase 181, original SiN Episodes 2006 client.dll surface-property manifest
// bootstrap and KeyValues 32-bit helper layout. A testable host-neutral
// reconstruction, not a game-loadable engine module, native allocator, or
// original Source ABI implementation.
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace sin_compat::original_client_surface_manifest {

inline constexpr std::uint32_t kManifestFunctionVA = 0x241927A0u;
inline constexpr std::uint32_t kKeyValuesFirstVA = 0x24164B90u;
inline constexpr std::uint32_t kKeyValuesNextVA = 0x24164BA0u;
inline constexpr std::uint32_t kKeyValuesFindVA = 0x24164B20u;
inline constexpr std::uint32_t kKeyValuesNameVA = 0x24164770u;
inline constexpr std::uint32_t kAllocatorForwarderVA = 0x24164C60u;
inline constexpr std::uint32_t kKeyValuesReleaseVA = 0x24166050u;
inline constexpr const char* kManifestPath = "scripts/surfaceproperties_manifest.txt";
inline constexpr const char* kSearchPathId = "GAME";
inline constexpr const char* kManifestFileType = "file";

// Original compiled x86 KeyValues node offsets: +0 interned name symbol,
// +0x18 next-peer pointer, +0x1C first-child pointer. All are DWORDs.
struct NativeKeyValuesLinkLayout32 {
    std::uint32_t symbol_00;
    std::uint8_t opaque_04_to_17[20];
    std::uint32_t next_18;
    std::uint32_t first_child_1c;
};
static_assert(sizeof(NativeKeyValuesLinkLayout32) == 32, "original KeyValues record layout");
static_assert(offsetof(NativeKeyValuesLinkLayout32, next_18) == 0x18, "original peer link offset");
static_assert(offsetof(NativeKeyValuesLinkLayout32, first_child_1c) == 0x1C, "original first-subkey offset");
static_assert(std::is_standard_layout<NativeKeyValuesLinkLayout32>::value,"standard native layout");

// Portable record representation. Never reinterpret this as a native x86
// KeyValues instance or call its ABI; the host's pointer size may differ.
struct KeyNode {
    std::uint32_t name_symbol = 0;
    KeyNode* next = nullptr;
    KeyNode* first = nullptr;
};
inline KeyNode* first_subkey(KeyNode* node) noexcept { return node ? node->first : nullptr; }
inline KeyNode* next_key(KeyNode* node) noexcept { return node ? node->next : nullptr; }

// Original 0x24164B20: compares the original DWORD symbol in each child and
// follows +0x18 until match or null, without a native cycle guard. The host
// guard below is an explicit safety measure, not a recovered native branch.
inline KeyNode* find_subkey_by_symbol(KeyNode* parent, std::uint32_t symbol,
                                     std::size_t max_steps = 65536) {
    if (!parent) throw std::invalid_argument("original KeyValues this cannot be null");
    auto* node = parent->first;
    std::size_t steps=0;
    while (node) {
        if (steps++ == max_steps) throw std::length_error("cycle in host KeyValues fixture");
        if (node->name_symbol == symbol) return node;
        node = node->next;
    }
    return nullptr;
}

// These are x86 forwarding-boundary models. The underlying engine allocator
// and KeyValues::GetName implementation are not decompiled by this header.
template<class Allocator>
auto forward_alloc_0x24164c60(Allocator&& allocator, std::uint32_t bytes)
    -> decltype(allocator(bytes)) {
    return allocator(bytes);
}
template<class DestroyPayload, class FreeMemory>
void release_0x24166050(std::uintptr_t address, DestroyPayload&& payload,
                         FreeMemory&& free_memory) {
    if (!address) return;
    payload(address);
    free_memory(address);
}

struct ManifestStats {
    bool parsed = false;
    std::size_t visited = 0;
    std::size_t file_entries = 0;
    std::size_t unknown_entries = 0;
};

// Exact original 0x241927A0 high-level branch order. Opaque engine methods
// (allocator, construct, KeyValues parser, string comparison, surface file
// loading/logging, deallocation) are injected. Original native bytes prove
// the call/branch topology, NOT implementation of these host functions.
//
// Host contract:
// uintptr_t allocate_record(uint32_t bytes);
// uintptr_t construct_record(uintptr_t raw,const char* manifest_name);
// bool parse_manifest(uintptr_t keyvalues, uintptr_t file_system,
//                     const char* manifest_path,const char* search_path);
// uintptr_t first_key(uintptr_t keyvalues), next_key(uintptr_t key);
// const char* key_name(uintptr_t key);
// bool is_file_type(const char* candidate,const char* expected);
// void load_surface_file(uintptr_t surface_props,uintptr_t filesystem,uintptr_t key);
// void report_load_failure(const char* manifest_path);
// void report_unknown_type(const char* manifest_path,const char* type);
// void destroy_record(uintptr_t keyvalues);
//
// Like the original, parse failure and empty list terminate after cleanup.
// Unlike native x86 code, an invalid null record/name and cyclic list trigger
// a host diagnostic rather than executing an invalid pointer dereference.
template<class Host>
ManifestStats load_manifest(Host& host, std::uintptr_t surface_props,
                            std::uintptr_t filesystem,
                            std::size_t max_entries = 65536) {
    const std::uintptr_t raw = host.allocate_record(0x28u);
    if (!raw) throw std::runtime_error("native KeyValues allocator returned null");
    const std::uintptr_t record=host.construct_record(raw,kManifestPath);
    if (!record) throw std::runtime_error("native KeyValues constructor returned null");
    ManifestStats stats{};
    if (!host.parse_manifest(record,filesystem,kManifestPath,kSearchPathId)) {
        host.report_load_failure(kManifestPath);
        host.destroy_record(record);
        return stats;
    }
    stats.parsed = true;
    std::uintptr_t key=host.first_key(record);
    while(key) {
        if (stats.visited == max_entries)
            throw std::length_error("cycle or excessive host KeyValues children");
        ++stats.visited;
        const char* type = host.key_name(key);
        if (!type) throw std::invalid_argument("native comparison of null type name");
        if (host.is_file_type(type,kManifestFileType)) {
            ++stats.file_entries;
            host.load_surface_file(surface_props,filesystem,key);
        } else {
            ++stats.unknown_entries;
            host.report_unknown_type(kManifestPath,type);
        }
        key=host.next_key(key);
    }
    host.destroy_record(record);
    return stats;
}
} // namespace sin_compat::original_client_surface_manifest