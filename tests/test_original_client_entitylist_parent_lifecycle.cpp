#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include "sin_compat/original_client_entitylist_parent_lifecycle.hpp"

namespace n=sin_compat::original_client_entitylist_parent_lifecycle;
using n::Bytes;

struct Host final:n::IHost {
    std::vector<std::string> events;
    std::uint32_t mirror_result{0xAAA00000u};
    std::uint32_t secondary_result{0xBBB00000u};
    std::uint32_t secondary_flags{0x400u};
    std::uint16_t aux_result{31u};
    Bytes* primary{};
    std::uint32_t primary_index{};
    std::uint32_t highest_at_unregister{};
    std::uint32_t occupied_at_unregister{};

    std::uint32_t entity_virtual(std::uint32_t ptr,
                                  std::uint32_t byte_offset) override {
        assert(ptr!=0u);
        if(byte_offset==0x10u) {
            events.push_back("virtual10");
            return mirror_result;
        }
        if(byte_offset==0x1Cu) {
            events.push_back("virtual1C");
            return secondary_result;
        }
        if(byte_offset==0xC4u) {
            events.push_back("virtualC4");
            assert(ptr==secondary_result);
            return secondary_flags;
        }
        assert(false);
        return 0u;
    }
    void registry_add_24108880(std::uint32_t entity) override {
        assert(entity==0xD00DF00Du);
        events.push_back("registry_add");
    }
    void registry_remove_24108910(std::uint32_t entity) override {
        assert(entity==0xD00DF00Du);
        events.push_back("registry_remove");
        if(primary!=nullptr) {
            highest_at_unregister=n::read_u32(*primary,n::kHighestSlotField);
            occupied_at_unregister=n::read_u32(*primary,4u+primary_index*16u);
            n::write_u32(*primary,4u+primary_index*16u,0u);
        }
    }
    std::uint16_t aux_acquire_24108680(std::uint32_t secondary) override {
        assert(secondary==secondary_result);
        events.push_back("aux_acquire");
        return aux_result;
    }
    void aux_release_24106C20(std::uint16_t index) override {
        assert(index==aux_result);
        events.push_back("aux_release");
    }
    void call_destructor_24107430(std::uint32_t parent) override {
        assert(parent==n::kParentObjectVA);
        events.push_back("destructor");
    }
    void free_object_2416FD60(std::uint32_t parent) override {
        assert(parent==n::kParentObjectVA);
        events.push_back("free");
    }
};

static Bytes parent() {
    Bytes b(0x18040u,0u);
    n::write_u32(b,n::kHighestSlotField,0xFFFFFFFFu);
    return b;
}
static void verify_native_parent_interface_not_the_embedded_vtable() {
    static_assert(n::kParentVtable==0x242973B8u);
    static_assert(n::kEmbeddedVtable==0x24297390u);
    static_assert(n::kParentObjectVA==0x2433FDC8u);
    static_assert(n::kParentVptrConstructorStore==0x241081A2u);
    static_assert(n::kIndexMask==0xFFFu);
    static_assert(n::kMirrorGuardExclusive==0x800u);
    static_assert(n::kMirrorStride==8u);
    static_assert(n::kMirrorPointerField==0x10028u);
    static_assert(n::kMirrorAuxField==0x1002Cu);
    static_assert(n::kLiveCountField==0x10018u);
    static_assert(n::kHighestSlotField==0x10024u);
    static_assert(n::kSecondaryFlagCountField==0x10020u);
    static_assert(n::kSecondaryFlagBit==0x400u);
    static_assert(n::kAuxStorageOffset==0x18028u);
    static_assert(n::kNewFullyReviewedMethods==3u);
    static_assert(n::kNewOriginalNativeBytes==348u);
    std::size_t total=0u;
    for(std::uint32_t i=0u;i<3u;i++) {
        const auto& m=n::kMethods[i];
        assert(m.slot==i);
        assert(m.stack_pop==(i==2?4u:8u));
        total+=m.size;
    }
    assert(n::kMethods[0].va==0x241089D0u);
    assert(n::kMethods[1].va==0x24108A70u);
    assert(n::kMethods[2].va==0x241082A0u);
    assert(total==348u);
}

static void register_then_unregister_preserves_native_event_order() {
    Bytes b=parent();
    Host h;
    // The 16-byte PRIMARY table differs from the mirrored 8-byte array.
    n::write_u32(b,4u+3u*16u,0xD00DF00Du);
    const auto handle=0xABC00003u;
    n::observe_add(b,0u,0xD00DF00Du,handle,h);
    assert(n::read_u32(b,n::kLiveCountField)==1u);
    assert(n::read_u32(b,n::kHighestSlotField)==3u);
    assert(n::read_u32(b,n::kMirrorPointerField+3u*8u)==0xAAA00000u);
    assert(n::read_u16(b,n::kMirrorAuxField+3u*8u)==31u);
    assert(n::read_u32(b,n::kSecondaryFlagCountField)==1u);
    assert((h.events==std::vector<std::string>{
        "virtual10","registry_add","virtual1C","aux_acquire","virtualC4"}));
    h.events.clear();
    h.primary=&b;
    h.primary_index=3u;
    n::observe_remove(b,0u,0xD00DF00Du,handle,h);
    assert((h.events==std::vector<std::string>{
        "registry_remove","virtual1C","virtualC4","aux_release"}));
    // Crucial observed native order: highest scan RUNS BEFORE
    // the real 0x24108910 helper removes primary table entry.
    assert(h.highest_at_unregister==3u);
    assert(h.occupied_at_unregister==0xD00DF00Du);
    assert(n::read_u32(b,4u+3u*16u)==0u);
    assert(n::read_u32(b,n::kHighestSlotField)==3u);
    assert(n::read_u32(b,n::kLiveCountField)==0u);
    assert(n::read_u32(b,n::kSecondaryFlagCountField)==0u);
    assert(n::read_u32(b,n::kMirrorPointerField+3u*8u)==0u);
    assert(n::read_u16(b,n::kMirrorAuxField+3u*8u)==0xFFFFu);
}
static void masked_indices_and_2048_guard_are_distinct() {
    Bytes b=parent();Host h;
    // 0xFFF is addressable in the 4096-entry mirror array, but is
    // outside the native 2048-entry live-counter guard.
    h.secondary_result=0u;
    n::observe_add(b,0u,0xD00DF00Du,0x123FFFFFu,h);
    assert(n::read_u32(b,n::kLiveCountField)==0u);
    assert(n::read_u32(b,n::kHighestSlotField)==0xFFFFFFFFu);
    assert(n::read_u16(b,n::kMirrorAuxField+0xFFFu*8u)==0xFFFFu);
    assert((h.events==std::vector<std::string>{"registry_add","virtual1C"}));
    h.events.clear();
    n::observe_remove(b,0u,0xD00DF00Du,0x123FFFFFu,h);
    assert((h.events==std::vector<std::string>{"registry_remove","virtual1C"}));
    assert(n::read_u32(b,n::kLiveCountField)==0u);
    assert(n::read_u16(b,n::kMirrorAuxField+0xFFFu*8u)==0xFFFFu);
    // Bit 10, NOT bit 2, drives optional secondary counter.
    b=parent();h.events.clear();
    h.secondary_result=0xBBB00000u;
    h.secondary_flags=0x4u;
    n::observe_add(b,0u,0xD00DF00Du,0x7000u,h);
    assert(n::read_u32(b,n::kSecondaryFlagCountField)==0u);
    assert(n::read_u32(b,n::kLiveCountField)==1u);
    assert(n::read_u32(b,n::kHighestSlotField)==0u);
    assert(n::read_u16(b,n::kMirrorAuxField)==31u);
}
static void parent_destructor_calls_are_conditional_and_ordered() {
    for(const auto flags: {0u,1u,2u,3u,0xFFFF0000u,0xFFFF0001u}) {
        Host h;
        assert(n::observe_deleting_destructor(n::kParentObjectVA,flags,h)==
               n::kParentObjectVA);
        if((flags&1u)!=0u)
            assert((h.events==std::vector<std::string>{"destructor","free"}));
        else
            assert((h.events==std::vector<std::string>{"destructor"}));
    }
}
static void invalid_host_memory_is_refused_without_faking_x86_crashes() {
    Host h;
    Bytes small(48u,0u);
    const auto before=small;
    bool rejected=false;
    try{n::observe_add(small,0u,0xD00DF00Du,1u,h);}
    catch(const std::out_of_range&){rejected=true;}
    assert(rejected && small==before && h.events.empty());
    rejected=false;
    try{n::observe_remove(small,0u,0xD00DF00Du,1u,h);}
    catch(const std::out_of_range&){rejected=true;}
    assert(rejected && small==before && h.events.empty());
    rejected=false;
    try{n::observe_add(small,0u,0u,1u,h);}
    catch(const std::invalid_argument&){rejected=true;}
    assert(rejected);
    rejected=false;
    try{n::observe_deleting_destructor(0u,1u,h);}
    catch(const std::invalid_argument&){rejected=true;}
    assert(rejected && h.events.empty());
    rejected=false;
    try{n::checked_offset(std::numeric_limits<std::size_t>::max(),1u);}
    catch(const std::overflow_error&){rejected=true;}
    assert(rejected);
}

int main() {
    verify_native_parent_interface_not_the_embedded_vtable();
    register_then_unregister_preserves_native_event_order();
    masked_indices_and_2048_guard_are_distinct();
    parent_destructor_calls_are_conditional_and_ordered();
    invalid_host_memory_is_refused_without_faking_x86_crashes();
}
