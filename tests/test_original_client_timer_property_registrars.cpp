#include "sin_compat/original_client_timer_property_registrars.hpp"
#include "sin_compat/original_client_timer_constructor_contract.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
namespace m=sin_compat::original_client_timer_properties;
namespace ctor=sin_compat::original_client_timer_constructors;
namespace {
void test_original_names_defaults_and_native_kind_bytes() {
    using m::PropertyKind;
    const auto& p=m::kOriginalProperties;
    assert(p.size()==7U);
    assert(p[0].external_name=="BreathTimerColor");
    assert(p[0].member_name=="m_breathTimerColor");
    assert(p[0].native_type=="Color" && p[0].native_default=="0 0 0 0");
    assert(p[0].native_kind_byte==PropertyKind::Color);
    assert(p[3].external_name=="PoisonTimerColor");
    assert(p[3].member_name=="m_poisonTimerColor");
    assert(p[3].native_default=="255 255 255 0");
    assert(p[3].native_kind_byte==PropertyKind::Color);
    assert(p[4].external_name=="MutagenBarForeground");
    assert(p[5].external_name=="MutagenBarBackground");
    assert(p[6].external_name=="MutagenBarInactive");
    assert(p[6].member_name=="m_mutagenBarInactiveTextureName");
    assert(p[6].native_default=="Mutagen_bar_inactive");
    for(std::size_t i=0;i<p.size();++i) {
        assert(p[i].complete_object_field_offset==
               p[i].panel_relative_field_offset+0x14U);
        assert(p[i].native_field_accessor_va!=0U);
        assert(p[i].registration_guard_va!=0U);
        if(i!=0U && i!=3U) {
            assert(p[i].native_type=="string");
            assert(p[i].native_kind_byte==PropertyKind::String);
        }
    }
}
void test_native_constructor_order_and_one_time_behavior() {
    m::RegistryState state{};
    std::vector<std::string> events;
    auto record=[&](const m::Property& p){
        events.emplace_back(p.external_name);
    };
    assert(m::register_original_class_properties(state,m::TimerClass::Breath,record)==3U);
    assert((events==std::vector<std::string>{"BreathTimerColor",
        "BreathBarForeground","BreathBarBackground"}));
    assert(m::register_original_class_properties(state,m::TimerClass::Breath,record)==0U);
    assert(m::register_original_class_properties(state,m::TimerClass::Poison,record)==4U);
    assert((events==std::vector<std::string>{
        "BreathTimerColor","BreathBarForeground","BreathBarBackground",
        "PoisonTimerColor","MutagenBarForeground","MutagenBarBackground",
        "MutagenBarInactive"}));
    assert(m::register_original_class_properties(state,m::TimerClass::Poison,record)==0U);
    for(auto flag:state.one_time_flags)assert(flag==1U);
}
void test_original_guard_is_set_before_native_registration_callback() {
    m::RegistryState state{};
    int called=0;
    auto recursive=[&](const m::Property& p){
        ++called;
        assert(p.external_name=="PoisonTimerColor");
        assert(state.one_time_flags[3]==1);
        // Old guarded function returns during reentrant registration.
        assert(!m::register_original_once(state,3,[](const m::Property&){
            assert(false);
        }));
    };
    assert(m::register_original_once(state,3,recursive));
    assert(called==1);
    assert(!m::register_original_once(state,3,recursive));
    assert(called==1);
    assert(!m::register_original_once(state,999U,recursive));
}
void test_per_property_guards_are_independent() {
    m::RegistryState state{};
    state.one_time_flags[1]=0xFFU;
    std::vector<std::string> names;
    assert(m::register_original_class_properties(
        state,m::TimerClass::Breath,
        [&](const m::Property& p){names.emplace_back(p.external_name);})==2U);
    assert((names==std::vector<std::string>{
        "BreathTimerColor","BreathBarBackground"}));
    assert(state.one_time_flags[1]==0xFFU);
    assert(state.one_time_flags[0]==1U);
    assert(state.one_time_flags[3]==0U);
}
void test_real_object_offsets_and_resolver_fallthrough() {
    constexpr auto& p=m::kOriginalProperties;
    assert(p[0].complete_object_field_offset==ctor::kOriginalCompleteFirstByteOffset);
    assert(p[1].complete_object_field_offset==ctor::kOriginalCompleteFirstResourceOffset);
    assert(p[3].complete_object_field_offset==ctor::kOriginalCompleteFirstByteOffset);
    assert(p[4].complete_object_field_offset==ctor::kOriginalCompleteFirstResourceOffset);
    assert(p[2].panel_relative_field_offset==0xC9U);
    assert(p[5].panel_relative_field_offset==0xC9U);
    assert(p[6].panel_relative_field_offset==0xEAU);
    assert(m::original_late_resource_resolver(0U,0xC9U)==0xDDU);
    assert(m::original_late_resource_resolver(0U,0xEAU)==0xFEU);
    assert(m::original_late_resource_resolver(0x1000U,0xC9U)==0x10C9U);
    assert(m::original_late_resource_resolver(0x1000U,0xEAU)==0x10EAU);
    assert(m::original_late_resource_resolver(0xFFFFFFF0U,0xEAU)==0xDAU);
    // Original +0xA3 resolver behaves differently on zero.
    assert(ctor::original_first_panel_byte_pointer(0U)==0U);
}
}
int main() {
    test_original_names_defaults_and_native_kind_bytes();
    test_native_constructor_order_and_one_time_behavior();
    test_original_guard_is_set_before_native_registration_callback();
    test_per_property_guards_are_independent();
    test_real_object_offsets_and_resolver_fallthrough();
}
