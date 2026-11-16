#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include "sin_compat/original_client_entitylist_base_register.hpp"

namespace reg=sin_compat::original_client_entitylist_base_register;
namespace base=reg::base;

struct SerialSpy final:reg::SerialSource {
    std::uint32_t calls{};
    std::uint32_t random_int(std::uint32_t lo,
                             std::uint32_t hi) override {
        assert(lo==0u&&hi==32767u);
        return (calls++*37u)^0xABCD0000u;
    }
};
struct Events final:reg::BindCallbacks,base::Callbacks {
    std::vector<std::string> items{};
    std::vector<std::uint32_t> binding_handles{};
    std::vector<std::uint32_t> removal_handles{};
    bool modify_binding_handle{};

    // Original same native entity virtual slot1 invoked both by
    // registrations (packed handle) and removals (initial 0xFFFFFFFF).
    void entity_virtual_slot1(std::uint32_t entity,
                              std::uint32_t& inout) override {
        if(inout==0xFFFFFFFFu) {
            items.push_back("unbind_entity:"+std::to_string(entity));
        } else {
            items.push_back("bind_entity:"+std::to_string(entity));
            binding_handles.push_back(inout);
            if(modify_binding_handle)inout^=0x10000000u;
        }
    }
    void parent_virtual_slot0(std::uint32_t entity,
                              std::uint32_t handle) override {
        items.push_back("bind_parent:"+std::to_string(entity));
        binding_handles.push_back(handle);
    }
    void parent_virtual_slot1(std::uint32_t entity,
                              std::uint32_t handle) override {
        items.push_back("unbind_parent:"+std::to_string(entity));
        removal_handles.push_back(handle);
    }
};
static void test_verified_native_entrypoints_and_constants() {
    static_assert(reg::kBaseCtorVA==0x24127E80u);
    static_assert(reg::kRegisterExplicitVA==0x24127F50u);
    static_assert(reg::kRegisterForwardVA==0x241280A0u);
    static_assert(reg::kRegisterNonNetworkedVA==0x241280C0u);
    static_assert(reg::kRandomSerialWrapperVA==0x241A8B90u);
    static_assert(reg::kOriginalVstdlibRandomIntIAT==0x24276278u);
    static_assert(reg::kOriginalTier0ErrorIAT==0x242761FCu);
    static_assert(reg::kFirstInitialHighFreeSlot==2049u);
    static_assert(reg::kLastInitialHighFreeSlot==4095u);
    static_assert(reg::kInitialHighFreeSlotCount==2047u);
    static_assert(reg::kKeepOriginalSerial==0xFFFFFFFFu);
    static_assert(reg::kNewWholeOriginalMethods==5u);
    static_assert(reg::kNewWholeNativeCodeBytes==536u);
    static_assert(base::kSlotStride==16u);
    static_assert(base::kSlotCount==4096u);
}
static void test_constructor_initializes_all_4096_and_exact_free_chain() {
    base::State state;
    SerialSpy random;
    reg::construct_base(state,random);
    assert(random.calls==4096u);
    assert(state.vtable_token==base::kNativeBaseVtableVA);
    assert(state.active.head==base::kMissing);
    assert(state.active.tail==base::kMissing);
    assert(state.high_free.head==2049u);
    assert(state.high_free.tail==4095u);
    assert(base::checked(state,0u).serial==0u);
    assert(base::checked(state,1u).serial==37u);
    assert(base::checked(state,4095u).serial==((4095u*37u)&0x7FFFu));
    for(std::size_t i=0;i<state.slots.size();++i) {
        const auto id=static_cast<std::uint16_t>(i);
        const auto& s=base::checked(state,id);
        assert(s.entity_token==0u);
        assert(s.serial==((static_cast<std::uint32_t>(i)*37u)&0x7FFFu));
        if(i<2049u) {
            assert(s.previous==id&&s.next==id);
            assert(!s.in_high_free);
        } else {
            assert(s.in_high_free);
            assert(s.previous==(i==2049u?base::kMissing:i-1u));
            assert(s.next==(i==4095u?base::kMissing:i+1u));
        }
    }
    // Critical original distinction: index 2048 starts self-linked,
    // although Phase247 removed slot 2048 gets added to this free list.
    assert(!base::checked(state,2048u).in_high_free);
    reg::construct_base(state,random); // original constructor resets all fields
    assert(random.calls==8192u);
    assert(state.high_free.head==2049u&&state.high_free.tail==4095u);
}
static void test_explicit_registration_keeps_or_overrides_raw_serial() {
    base::State state;SerialSpy rng;reg::construct_base(state,rng);
    Events events;
    std::uint32_t retained=0u;
    reg::register_explicit(state,retained,77u,12u,reg::kKeepOriginalSerial,events);
    assert(retained==((12u*37u)<<12|12u));
    assert(base::checked(state,12u).serial==12u*37u);
    assert(state.active.head==12u&&state.active.tail==12u);
    assert(base::checked(state,12u).entity_token==77u);
    std::uint32_t overridden=0u;
    reg::register_explicit(state,overridden,99u,2049u,0xFEDC1234u,events);
    assert(base::checked(state,2049u).serial==0xFEDC1234u);
    assert(overridden==((0xFEDC1234u<<12)|2049u));
    assert(state.active.head==12u&&state.active.tail==2049u);
    assert(base::checked(state,12u).next==2049u);
    assert(base::checked(state,2049u).previous==12u);
    assert(state.high_free.head==2050u&&state.high_free.tail==4095u);
    assert(!base::checked(state,2049u).in_high_free);
    assert((events.items==std::vector<std::string>{
        "bind_entity:77","bind_parent:77",
        "bind_entity:99","bind_parent:99"
    }));
    assert((events.binding_handles==std::vector<std::uint32_t>{
        retained,retained,overridden,overridden
    }));
}
static void test_forwarder_returns_same_output_pointer_and_callback_can_edit_handle() {
    base::State state;SerialSpy rng;reg::construct_base(state,rng);
    Events events;events.modify_binding_handle=true;
    std::uint32_t output=0u;
    auto* result=reg::register_explicit_forward(
        state,&output,400u,4u,9u,events);
    assert(result==&output);
    assert(output==(((9u<<12)|4u)^0x10000000u));
    assert(base::checked(state,4u).entity_token==400u);
    assert((events.binding_handles==std::vector<std::uint32_t>{
        (9u<<12)|4u,output
    }));
    bool refused=false;
    try {(void)reg::register_explicit_forward(
        state,nullptr,401u,5u,10u,events);}
    catch(const std::invalid_argument&){refused=true;}
    assert(refused&&state.active.head==4u);
}
static void test_non_networked_draws_exact_first_high_slot_and_removal_requeues() {
    base::State state;SerialSpy rng;reg::construct_base(state,rng);
    Events events;
    std::uint32_t first=0u;
    reg::register_non_networked(state,first,501u,events);
    const auto expected_serial=(2049u*37u)&0x7FFFu;
    assert(first==((expected_serial<<12)|2049u));
    assert(state.active.head==2049u&&state.active.tail==2049u);
    assert(state.high_free.head==2050u&&state.high_free.tail==4095u);
    assert(!base::checked(state,2049u).in_high_free);
    assert(base::checked(state,2049u).entity_token==501u);
    assert(base::remove_packed_handle(state,first,events));
    assert(state.active.head==base::kMissing&&state.active.tail==base::kMissing);
    assert(state.high_free.head==2050u&&state.high_free.tail==2049u);
    assert(base::checked(state,2049u).serial==((expected_serial+1u)&0x7FFFu));
    assert(base::checked(state,2049u).in_high_free);
    assert(events.removal_handles.size()==1u);
    assert(events.removal_handles[0]==first);
    assert((events.items==std::vector<std::string>{
        "bind_entity:501","bind_parent:501",
        "unbind_entity:501","unbind_parent:501"
    }));
    std::uint32_t second=0u;
    reg::register_non_networked(state,second,502u,events);
    assert((second&base::kIndexMask)==2050u);
    assert(state.high_free.head==2051u);
    // Releasing and re-registering a high slot with explicit index must
    // remove it from high_free and retain native serial if sentinel.
    std::uint32_t explicit_handle=0u;
    reg::register_explicit(state,explicit_handle,503u,2049u,
                           reg::kKeepOriginalSerial,events);
    assert((explicit_handle&base::kIndexMask)==2049u);
    assert(state.high_free.tail==4095u);
    assert(state.active.head==2050u&&state.active.tail==2049u);
}
static void test_exhaustion_and_invalid_registration_are_safe_host_failures() {
    base::State state;SerialSpy rng;reg::construct_base(state,rng);
    Events events;
    std::uint32_t output=0xAA55AA55u;
    state.high_free={}; // synthetic test of original fatal Error condition
    bool rejected=false;
    try{reg::register_non_networked(state,output,123u,events);}
    catch(const std::runtime_error&){rejected=true;}
    assert(rejected&&output==0xAA55AA55u);
    assert(events.items.empty()&&state.active.head==base::kMissing);
    rejected=false;
    try {reg::register_explicit(state,output,0u,7u,0u,events);}
    catch(const std::invalid_argument&){rejected=true;}
    assert(rejected);
    rejected=false;
    volatile std::uint16_t invalid_index=4096u;
    try{reg::register_explicit(state,output,44u,invalid_index,0u,events);}
    catch(const std::out_of_range&){rejected=true;}
    assert(rejected);
    reg::register_explicit(state,output,44u,7u,0u,events);
    rejected=false;
    try{reg::register_explicit(state,output,45u,7u,0u,events);}
    catch(const std::logic_error&){rejected=true;}
    assert(rejected);
    base::State incomplete(8u);
    rejected=false;
    try{reg::construct_base(incomplete,rng);}
    catch(const std::logic_error&){rejected=true;}
    assert(rejected);
}
int main() {
    test_verified_native_entrypoints_and_constants();
    test_constructor_initializes_all_4096_and_exact_free_chain();
    test_explicit_registration_keeps_or_overrides_raw_serial();
    test_forwarder_returns_same_output_pointer_and_callback_can_edit_handle();
    test_non_networked_draws_exact_first_high_slot_and_removal_requeues();
    test_exhaustion_and_invalid_registration_are_safe_host_failures();
    return 0;
}
