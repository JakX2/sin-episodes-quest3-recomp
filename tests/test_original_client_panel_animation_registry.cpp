#include "sin_compat/original_client_panel_animation_registry.hpp"
#include "sin_compat/original_client_timer_property_registrars.hpp"
#include "sin_compat/original_client_property_descriptor_array.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace reg=sin_compat::original_client_panel_registry;
namespace props=sin_compat::original_client_timer_properties;
namespace vector24=sin_compat::original_client_property_descriptor_array;
namespace {
void test_original_prefix_strip_q_strnicmp_only() {
    using reg::normalize_original_class_name;
    static_assert(reg::has_original_vgui_prefix("vgui::CHudBreathTimer"),"prefix");
    static_assert(reg::has_original_vgui_prefix("VGUI::CHudBreathTimer"),"case");
    static_assert(!reg::has_original_vgui_prefix("vgui:Panel"),"partial");
    const std::array<std::string_view,5> matching{{
        "vgui::Panel","VGUI::Panel","VgUi::Panel","vGuI::Panel","VGUi::Panel"}};
    for(auto original:matching) {
        const auto result=normalize_original_class_name(original);
        assert(result.prefix_removed);
        assert(result.original_symbol_key=="Panel");
    }
    auto no_prefix=normalize_original_class_name("CHudPoisonTimer");
    assert(!no_prefix.prefix_removed);
    assert(no_prefix.original_symbol_key=="CHudPoisonTimer");
    assert(normalize_original_class_name("vgui::").original_symbol_key.empty());
    assert(!normalize_original_class_name("vgui:").prefix_removed);
    const auto suffix=normalize_original_class_name("VGUI::PaneL");
    assert(suffix.original_symbol_key=="PaneL"); // suffix case unmodified
}
void test_lazy_singleton_sets_flag_before_callback() {
    reg::SingletonState state{};
    int calls=0;
    auto callback=[&](std::uint32_t original_va) {
        ++calls;
        assert(original_va==reg::kRegistrySingletonVA);
        assert((state.initialization_bits&1u)!=0);
        // Original OR flag before initialization permits recursion.
        auto recursive=reg::original_lazy_registry(state,
            [&](std::uint32_t){assert(false);});
        assert(recursive==reg::kRegistrySingletonVA);
    };
    assert(reg::original_lazy_registry(state,callback)==reg::kRegistrySingletonVA);
    assert(reg::original_lazy_registry(state,callback)==reg::kRegistrySingletonVA);
    assert(calls==1);
    reg::SingletonState high_bits{};
    high_bits.initialization_bits=0x80u;
    assert(reg::original_lazy_registry(high_bits,callback)==reg::kRegistrySingletonVA);
    assert(high_bits.initialization_bits==0x81u);
    assert(calls==2);
}
void test_existing_owner_short_circuits_allocator() {
    std::vector<std::string> sequence;
    reg::SingletonState singleton{};
    auto result=reg::original_class_registry_entry(
        singleton,"VGUI::CHudBreathTimer",
        [&](std::uint32_t va){assert(va==reg::kRegistrySingletonVA);sequence.emplace_back("singleton");},
        [&](std::string_view name){
            assert(name=="CHudBreathTimer");
            sequence.emplace_back("find");
            return 0x12345000u;
        },
        [&]{sequence.emplace_back("setup");},
        [&](std::uint32_t)->std::uint32_t{assert(false);return 0;},
        [&](std::string_view,std::uint32_t,const reg::EmptyOwnerWords&){assert(false);});
    assert(result.found_existing);
    assert(!result.insertion_attempted && result.allocation_succeeded);
    assert(result.owner_handle==0x12345000u);
    assert(result.canonical_key=="CHudBreathTimer");
    assert((sequence==std::vector<std::string>{"singleton","find"}));
}
void test_missing_owner_normalizes_and_zeroes_28_bytes() {
    std::vector<std::string> sequence;
    reg::SingletonState singleton{};
    const auto result=reg::original_class_registry_entry(
        singleton,"vgui::CHudPoisonTimer",
        [&](std::uint32_t){sequence.emplace_back("singleton");},
        [&](std::string_view name)->std::uint32_t{
            sequence.emplace_back("find");
            assert(name=="CHudPoisonTimer");return 0;
        },
        [&]{sequence.emplace_back("setup");},
        [&](std::uint32_t bytes)->std::uint32_t{
            sequence.emplace_back("allocate");
            assert(bytes==28u);return 0x43210000u;
        },
        [&](std::string_view name,std::uint32_t owner,const reg::EmptyOwnerWords& fields) {
            sequence.emplace_back("insert");
            assert(name=="CHudPoisonTimer" && owner==0x43210000u);
            for(auto word:fields)assert(word==0);
        });
    assert(!result.found_existing && result.insertion_attempted);
    assert(result.allocation_succeeded && result.owner_handle==0x43210000u);
    assert((sequence==std::vector<std::string>{
        "singleton","find","setup","allocate","insert"}));
    assert(singleton.initialization_bits==1);
}
void test_original_allocation_failure_still_attempts_map_insert() {
    int count=0;
    const auto r=reg::original_find_or_create(
        "NoSuchPanel",
        [](std::string_view)->std::uint32_t{return 0;},
        [&]{++count;},
        [&](std::uint32_t bytes)->std::uint32_t{++count;assert(bytes==28);return 0;},
        [&](std::string_view,std::uint32_t owner,const reg::EmptyOwnerWords&){
            assert(owner==0);++count;
        });
    assert(count==3);
    assert(!r.found_existing && !r.allocation_succeeded);
    assert(r.insertion_attempted && r.owner_handle==0);
}
void test_true_registrar_owner_bridge_to_phase163_seven_descriptors() {
    // Test host callbacks stand in for original 28-byte owner allocator,
    // native string-symbol map and original property descriptor storage.
    // This test does NOT invent actual native engine pointer addresses.
    reg::SingletonState singleton{};
    std::map<std::string,std::uint32_t> owner_by_key;
    std::map<std::uint32_t,vector24::NativeDescriptorArray> property_arrays;
    std::uint32_t next_owner=0x10000000u;
    int singleton_creates=0,owner_creates=0,lookups=0,setup_calls=0;
    auto resolve=[&](std::string_view original_name) {
        return reg::original_class_registry_entry(
            singleton,original_name,
            [&](std::uint32_t){++singleton_creates;},
            [&](std::string_view key)->std::uint32_t{
                ++lookups;
                const auto it=owner_by_key.find(std::string(key));
                return it==owner_by_key.end()?0u:it->second;
            },
            [&]{++setup_calls;},
            [&](std::uint32_t bytes)->std::uint32_t{
                assert(bytes==28);++owner_creates;return next_owner+=28u;
            },
            [&](std::string_view key,std::uint32_t owner,
                const reg::EmptyOwnerWords& words) {
                for(auto word:words)assert(word==0);
                owner_by_key[std::string(key)]=owner;
            });
    };
    auto first=resolve("vgui::CHudBreathTimer");
    auto second=resolve("VGUI::CHudBreathTimer");
    auto poison=resolve("CHudPoisonTimer");
    assert(first.owner_handle==second.owner_handle);
    assert(second.found_existing && !second.insertion_attempted);
    assert(poison.owner_handle!=first.owner_handle && !poison.found_existing);
    assert(owner_creates==2 && setup_calls==2 && singleton_creates==1);
    assert(lookups==3);

    props::RegistryState registration_flags{};
    auto store_property=[&](const props::Property& property) {
        const auto owner=resolve(property.owner_name);
        auto& arr=property_arrays[owner.owner_handle];
        vector24::NativeDescriptor24 descriptor{};
        descriptor.raw_words={{
            property.registration_function_va,
            property.native_field_accessor_va,
            property.registration_guard_va,
            property.complete_object_field_offset,
            static_cast<std::uint32_t>(property.native_kind_byte),
            0xDEADCAFEu
        }};
        assert(vector24::append_original_descriptor(arr,descriptor));
    };
    assert(props::register_original_class_properties(
        registration_flags,props::TimerClass::Breath,store_property)==3);
    assert(props::register_original_class_properties(
        registration_flags,props::TimerClass::Poison,store_property)==4);
    const auto& breath_arr=property_arrays.at(first.owner_handle);
    const auto& poison_arr=property_arrays.at(poison.owner_handle);
    assert(breath_arr.records.size()==3 && breath_arr.capacity==4);
    assert(poison_arr.records.size()==4 && poison_arr.capacity==4);
    assert(breath_arr.records[0].raw_words[0]==0x24230DE0u);
    assert(poison_arr.records[0].raw_words[0]==0x24236450u);
    assert(poison_arr.records[3].raw_words[0]==0x242365A0u);
    assert(poison_arr.records[3].raw_words[5]==0xDEADCAFEu);
    assert(owner_creates==2 && singleton_creates==1);
}
}
int main() {
    test_original_prefix_strip_q_strnicmp_only();
    test_lazy_singleton_sets_flag_before_callback();
    test_existing_owner_short_circuits_allocator();
    test_missing_owner_normalizes_and_zeroes_28_bytes();
    test_original_allocation_failure_still_attempts_map_insert();
    test_true_registrar_owner_bridge_to_phase163_seven_descriptors();
}
