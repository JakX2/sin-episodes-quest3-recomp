#pragma once
// Phase184, original 2006 SiN Episodes client.dll KeyValues lifecycle.
// Engine-neutral x86 field evidence and an independently testable callback
// model. The original memory/symbol service is NOT implemented here.
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <unordered_set>

namespace sin_compat::original_client_keyvalues_lifetime {

inline constexpr std::uint32_t kCtorVA = 0x24165010u;
inline constexpr std::uint32_t kDeletingDtorVA = 0x24165050u;
inline constexpr std::uint32_t kDestroyContentsVA = 0x24166620u;
inline constexpr std::uint32_t kKeyValuesSystemProviderIAT = 0x24276230u;
inline constexpr std::uint32_t kKeyValuesFreeVA = 0x2416FD60u;

// A PE32 snapshot layout, not a host-pointer overlay. The original ctor
// clears the listed fields but does not touch unknown_14 or the bytes at
// offsets 0x25 through 0x27. It interns name using KeyValuesSystem slot +0xC.
struct NativeKeyValues40 {
    std::uint32_t name_00;
    std::uint32_t text_04;
    std::uint32_t wide_08;
    std::uint32_t number_0c;
    std::uint32_t type_10;
    std::uint32_t unknown_14;
    std::uint32_t next_18;
    std::uint32_t child_1c;
    std::uint32_t fallback_20;
    std::uint8_t escape_24;
    std::uint8_t untouched_25_27[3];
};
static_assert(sizeof(NativeKeyValues40) == 0x28, "original KeyValues allocation is 0x28 bytes");
static_assert(offsetof(NativeKeyValues40, unknown_14) == 0x14, "unobserved field boundary");
static_assert(offsetof(NativeKeyValues40, next_18) == 0x18, "peer link offset");
static_assert(offsetof(NativeKeyValues40, child_1c) == 0x1c, "child link offset");
static_assert(offsetof(NativeKeyValues40, fallback_20) == 0x20, "fallback link offset");
static_assert(offsetof(NativeKeyValues40, escape_24) == 0x24, "escape flag offset");

template<class Intern>
NativeKeyValues40* construct_native_snapshot(NativeKeyValues40& node,
                                              const char* name, Intern&& intern) {
    node.name_00 = 0xffffffffu;
    node.type_10 = 0;
    node.child_1c = 0;
    node.next_18 = 0;
    node.fallback_20 = 0;
    node.text_04 = 0;
    node.wide_08 = 0;
    node.number_0c = 0;
    node.escape_24 = 0;
    // Original x86: indirect KeyValuesSystem provider then virtual slot +0xC.
    node.name_00 = static_cast<std::uint32_t>(intern(name));
    return &node;
}

// Portable pointer graph: never cast it to the native 40-byte x86 struct.
struct Node {
    std::uint32_t name_symbol = 0xffffffffu;
    std::uintptr_t text = 0;
    std::uintptr_t wide = 0;
    std::uint32_t number = 0;
    std::uint32_t type = 0;
    Node* next = nullptr;
    Node* first_child = nullptr;
    Node* fallback = nullptr;
    bool escape = false;
};

// Original 0x24166620 first tears down every child, then peer nodes until
// reaching self (special native guard), and finally frees +4/+8 buffers.
// Host interface: release_node(Node*), release_buffer(uintptr_t). The actual
// KeyValuesSystem +0x08 release and CRT free are external ABI callbacks.
// Explicit safety departure: detects repeated nodes, severs host links after
// traversal, and bounds recursion to avoid original invalid-input hangs.
template<class Host>
void destroy_contents_impl(Node& node, Host& host,
                           std::unordered_set<Node*>& encountered,
                           std::size_t depth, std::size_t depth_limit) {
    if (depth >= depth_limit) throw std::length_error("KeyValues hierarchy depth limit");
    auto release_chain = [&](Node* first, bool stop_at_self) {
        Node* current = first;
        while (current) {
            if (stop_at_self && current == &node) break;
            if (!encountered.insert(current).second)
                throw std::length_error("cycle/shared ownership in KeyValues hierarchy");
            Node* following = current->next;
            current->next = nullptr; // original clears +0x18 on each owned node
            destroy_contents_impl(*current, host, encountered, depth + 1, depth_limit);
            host.release_node(current);
            current = following;
        }
    };
    release_chain(node.first_child, false);
    release_chain(node.next, true);
    // Host-only safety: original leaves root child/peer DWORDs untouched.
    node.first_child = nullptr;
    node.next = nullptr;
    host.release_buffer(node.text);  // native invokes free even for null
    host.release_buffer(node.wide);
    node.text = 0;
    node.wide = 0;
}

template<class Host>
void destroy_contents(Node& node, Host& host, std::size_t depth_limit = 65536) {
    std::unordered_set<Node*> visited;
    visited.insert(&node);
    destroy_contents_impl(node, host, visited, 0, depth_limit);
}

// Original 0x24165050: call 0x24166620 unconditionally, then free the
// instance via KeyValuesSystem slot +0x08 if (deletion_flags & 1) != 0.
// It returns the original instance pointer, even if memory was freed.
template<class Host>
Node* scalar_deleting_destructor(Node& node, std::uint32_t deletion_flags,
                                 Host& host) {
    destroy_contents(node, host);
    if ((deletion_flags & 1u) != 0) host.release_node(&node);
    return &node;
}

} // namespace sin_compat::original_client_keyvalues_lifetime