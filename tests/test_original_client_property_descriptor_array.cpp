#include "sin_compat/original_client_property_descriptor_array.hpp"
#include "sin_compat/original_client_timer_property_registrars.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <vector>

namespace array=sin_compat::original_client_property_descriptor_array;
namespace props=sin_compat::original_client_timer_properties;
namespace {
array::NativeDescriptor24 six(std::uint32_t seed) {
    return {{{seed,seed+1u,seed+2u,seed+3u,seed+4u,seed+5u}}};
}
void test_original_first_allocation_double_growth_and_cached_pointer() {
    array::NativeDescriptorArray state{};
    std::vector<std::array<std::int32_t,2>> observed;
    const auto track=[&](std::int32_t before,std::int32_t after) {
        observed.push_back({{before,after}});
    };
    auto result=array::insert_original_descriptor(state,0,six(10),track);
    assert(result && result->returned_index==0);
    assert(result->allocated_or_reallocated && !result->shifted_existing_records);
    assert(state.capacity==2 && state.cache_refreshed && state.records.size()==1u);
    state.cache_refreshed=false;
    result=array::insert_original_descriptor(state,1,six(20),track);
    assert(result && !result->allocated_or_reallocated);
    assert(state.capacity==2 && state.records.size()==2u);
    assert(state.cache_refreshed);
    result=array::insert_original_descriptor(state,2,six(30),track);
    assert(result && result->allocated_or_reallocated);
    assert(state.capacity==4);
    result=array::insert_original_descriptor(state,3,six(40),track);
    assert(result && !result->allocated_or_reallocated && state.capacity==4);
    result=array::insert_original_descriptor(state,4,six(50),track);
    assert(result && result->allocated_or_reallocated && state.capacity==8);
    assert((observed==std::vector<std::array<std::int32_t,2>>{{
        {{0,2}},{{2,4}},{{4,8}}}}));
    for(std::size_t i=0;i<5;++i)
        assert(state.records[i].raw_words==six(static_cast<std::uint32_t>(
            10+10*i)).raw_words);
}
void test_original_middle_insert_moves_entire_24_byte_records() {
    array::NativeDescriptorArray state{};
    assert(array::append_original_descriptor(state,six(100)));
    assert(array::append_original_descriptor(state,six(300)));
    assert(state.capacity==2);
    const auto inserted=array::insert_original_descriptor(state,1,six(200));
    assert(inserted && inserted->returned_index==1);
    assert(inserted->allocated_or_reallocated && inserted->shifted_existing_records);
    assert(state.records.size()==3u && state.capacity==4);
    assert(state.records[0].raw_words==six(100).raw_words);
    assert(state.records[1].raw_words==six(200).raw_words);
    assert(state.records[2].raw_words==six(300).raw_words);
    auto prepended=array::insert_original_descriptor(state,0,six(50));
    assert(prepended && prepended->shifted_existing_records);
    assert(!prepended->allocated_or_reallocated);
    assert(state.records[0].raw_words==six(50).raw_words);
    assert(state.records[3].raw_words==six(300).raw_words);
    // Original six DWORD copy preserves opaque padding word (at index 4).
    state.records[1].raw_words[4]=0xdeadbeefU;
    assert(state.records[1].raw_words[4]==0xdeadbeefU);
}
void test_original_explicit_grow_quantum_and_disabled_growth() {
    assert(array::plan_original_capacity_growth(0,1,3)==2);
    assert(array::plan_original_capacity_growth(2,1,3)==5);
    assert(array::plan_original_capacity_growth(5,5,3)==11);
    assert(array::plan_original_capacity_growth(5,5,0)==10);
    assert(array::plan_original_capacity_growth(5,1,-1)==5);
    assert(array::plan_original_capacity_growth(5,0,-1)==5);
    array::NativeDescriptorArray state{};
    state.original_grow_amount=3;
    for(int i=0;i<6;++i)assert(array::append_original_descriptor(
        state,six(static_cast<std::uint32_t>(i))));
    assert(state.capacity==8);
    array::NativeDescriptorArray fixed{};
    fixed.capacity=2;
    fixed.original_grow_amount=-1;
    assert(array::append_original_descriptor(fixed,six(1)));
    assert(array::append_original_descriptor(fixed,six(2)));
    fixed.cache_refreshed=false;
    assert(!array::append_original_descriptor(fixed,six(3)));
    assert(fixed.records.size()==2 && fixed.capacity==2);
    assert(!fixed.cache_refreshed);
}
void test_native_invalid_indices_and_math_rejected_without_ub() {
    array::NativeDescriptorArray state{};
    assert(!array::insert_original_descriptor(state,-1,six(1)));
    assert(!array::insert_original_descriptor(state,1,six(1)));
    assert(!state.cache_refreshed && state.records.empty());
    assert(!array::plan_original_capacity_growth(
        std::numeric_limits<std::int32_t>::max()-1,2,0));
    assert(!array::plan_original_capacity_growth(4,1,-2));
    assert(!array::plan_original_capacity_growth(-1,1,0));
    assert(array::plan_original_capacity_growth(2,0,0)==2);
    state.original_grow_amount=-2;
    assert(!array::append_original_descriptor(state,six(1)));
    assert(state.records.empty());
}
void test_seven_real_original_timer_property_registration_events() {
    props::RegistryState guards{};
    array::NativeDescriptorArray native{};
    std::vector<std::uint32_t> labels{};
    const auto register_one=[&](const props::Property& p) {
        const std::uint32_t idx=static_cast<std::uint32_t>(labels.size());
        // Original engine descriptor consists of SIX raw dwords.
        // The test uses synthetic distinguishable data; no original
        // module image pointers are supplied as portable host pointers.
        auto item=six(0x1000u+idx*0x10u);
        item.raw_words[4]=static_cast<std::uint32_t>(
            p.native_kind_byte);
        const auto result=array::append_original_descriptor(native,item);
        assert(result && result->returned_index==static_cast<std::int32_t>(idx));
        labels.push_back(p.registration_function_va);
    };
    assert(props::register_original_class_properties(
        guards,props::TimerClass::Breath,register_one)==3);
    assert(props::register_original_class_properties(
        guards,props::TimerClass::Poison,register_one)==4);
    assert(native.records.size()==7u && native.capacity==8);
    assert((labels==std::vector<std::uint32_t>{
        0x24230DE0u,0x24230E50u,0x24230EC0u,0x24236450u,
        0x242364C0u,0x24236530u,0x242365A0u}));
    for(std::size_t i=0;i<7;++i)
        assert(native.records[i].raw_words[4]==((i==0||i==3)?0u:1u));
    assert(props::register_original_class_properties(
        guards,props::TimerClass::Poison,register_one)==0);
    assert(native.records.size()==7u);
}
}
int main() {
    test_original_first_allocation_double_growth_and_cached_pointer();
    test_original_middle_insert_moves_entire_24_byte_records();
    test_original_explicit_grow_quantum_and_disabled_growth();
    test_native_invalid_indices_and_math_rejected_without_ub();
    test_seven_real_original_timer_property_registration_events();
}
