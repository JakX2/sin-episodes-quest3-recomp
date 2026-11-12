#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include "sin_compat/original_client_memalloc_bridge.hpp"
#include "sin_compat/original_client_keyvalues_lifetime.hpp"
namespace original = sin_compat::original_client_memalloc_bridge;
struct FakeTier0 {
    struct Event { std::string op; std::uintptr_t a,b; std::uint32_t c; };
    std::vector<Event> events;
    std::uintptr_t debug_owner=0;
    std::size_t resolves=0;
    std::uintptr_t resolve_debug_owner() { ++resolves; events.push_back({"resolve",0,0,0});return debug_owner; }
    std::uintptr_t ordinary_alloc(std::uint32_t n) { events.push_back({"plain",n,0,0});return n+0x1000u; }
    std::uintptr_t debug_alloc(std::uint32_t n,std::uintptr_t f,std::uint32_t line) {
        events.push_back({"debug",n,f,line});return n+0x2000u;
    }
    void free_block(std::uintptr_t p) {events.push_back({"free",p,0,0});}
};
struct KeyValuesToTier0 {
    using Node=sin_compat::original_client_keyvalues_lifetime::Node;
    FakeTier0& tier0;
    std::vector<Node*> released_nodes;
    void release_node(Node* node) { released_nodes.push_back(node); }
    void release_buffer(std::uintptr_t raw) { original::AllocationBridge::free(tier0,raw); }
};
int main() {
    static_assert(sizeof(original::NativeDispatchOffsets32)==12);
    static_assert(sizeof(original::NativeCachedDebugState32)==8);
    assert(original::kTier0MemAllocIAT==0x242761CCu);
    assert(original::kVstdlibKeyValuesSystemIAT==0x24276230u);
    assert(!original::allocator_present(0));
    assert(original::allocator_present(0x1234));
    // Both native allocator wrappers use the same process-global cached owner.
    FakeTier0 no_debug;
    original::AllocationBridge a;
    assert(a.allocate(no_debug,32)==0x1020u);
    assert(a.allocate(no_debug,50)==0x1032u);
    assert(no_debug.resolves==1);
    assert(no_debug.events.size()==3);
    assert(no_debug.events[1].op=="plain");
    assert(a.cached_state().bit0_flags==1u);
    assert(a.cached_state().opaque_debug_owner==0u);
    // Debug owner points to opaque information, never to an in-module copy.
    FakeTier0 with_debug;with_debug.debug_owner=0x00DAD001u;
    original::AllocationBridge b;
    assert(b.allocate(with_debug,42)==0x202Au);
    assert(b.allocate(with_debug,18)==0x2012u);
    assert(with_debug.resolves==1);
    assert(with_debug.events.size()==3);
    assert(with_debug.events[1].op=="debug");
    assert(with_debug.events[1].a==42u);
    assert(with_debug.events[1].b==0x00DAD001u);
    assert(with_debug.events[1].c==0u);
    // Original checks only bit0; preserve unrelated DWORD flag bits.
    FakeTier0 prepopulated; prepopulated.debug_owner=0x88776655u;
    original::AllocationBridge c({0xA5u, 0x11223344u});
    assert(c.allocate(prepopulated,1)==0x2001u);
    assert(prepopulated.resolves==0);
    assert(c.cached_state().bit0_flags==0xA5u);
    assert(prepopulated.events[0].b==0x11223344u);
    // A cache with other bits but no bit0 must still resolve once.
    original::AllocationBridge d({0xA0u, 0x800u});
    assert(d.allocate(prepopulated,7)==0x2007u);
    assert(prepopulated.resolves==1);
    assert(d.cached_state().bit0_flags==0xA1u);
    // Direct x86 debug-allocate forwarding does not consult cached state.
    FakeTier0 direct;
    assert(original::AllocationBridge::allocate_explicit_debug(direct,55,0x222u,17)==0x2037u);
    assert(direct.resolves==0);
    assert(direct.events[0].op=="debug" && direct.events[0].b==0x222u && direct.events[0].c==17u);
    original::AllocationBridge::free(direct,0x12345678u);
    original::AllocationBridge::free(direct,0u); // native forwards null unchanged
    assert(direct.events[1].op=="free" && direct.events[1].a==0x12345678u);
    assert(direct.events[2].op=="free" && direct.events[2].a==0);
    // Full x86 DWORD size is passed through, not truncated to 16 bits.
    (void)original::AllocationBridge().allocate(no_debug,0xFFFFFFFEu);
    assert(no_debug.events.back().a==0xFFFFFFFEu);
    // Phase184 -> Phase185 integration: KeyValues content destruction frees
    // narrow and wide buffers through this exact tier0 Free forwarding layer.
    namespace kv=sin_compat::original_client_keyvalues_lifetime;
    kv::Node root, child;
    root.first_child=&child;root.text=0xABCDu;root.wide=0;
    child.text=0x00120000u;child.wide=0xAAu;
    FakeTier0 memory;KeyValuesToTier0 bridge{memory,{}};
    kv::Node* out=kv::scalar_deleting_destructor(root,1u,bridge);
    assert(out==&root);
    assert(bridge.released_nodes.size()==2);
    assert(bridge.released_nodes[0]==&child && bridge.released_nodes[1]==&root);
    assert(memory.events.size()==4);
    assert(memory.events[0].a==0x00120000u);
    assert(memory.events[1].a==0xAAu);
    assert(memory.events[2].a==0xABCDu);
    assert(memory.events[3].a==0u);
    for(const auto& event: memory.events) assert(event.op=="free");
    return 0;
}