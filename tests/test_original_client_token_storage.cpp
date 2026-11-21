#include "sin_compat/original_client_token_storage.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <vector>
using namespace sin_compat::original_client_string_tokens::storage;
int main() {
    static_assert(sizeof(NativeArrayHeader)==12);
    static_assert(sizeof(PageHeader)==8);
    static_assert(kNodeStride==12&&kPagePointerStride==4);
    auto loc=node_location(kMissingToken);
    assert(loc.global_missing_sentinel&&loc.array_byte_offset==0);
    loc=node_location(0);
    assert(!loc.global_missing_sentinel&&loc.array_byte_offset==0);
    loc=node_location(3);
    assert(!loc.global_missing_sentinel&&loc.array_byte_offset==36);
    loc=node_location(0xFFFE);
    assert(!loc.global_missing_sentinel&&loc.array_byte_offset==0xFFFEu*12);

    auto plan=node_reserve(0,0,1,false);
    assert(plan.status==PlanStatus::Valid && plan.next_capacity==3 &&
           plan.allocation_bytes==36 && !plan.realloc_existing);
    plan=node_reserve(3,0,1,true);
    assert(plan.status==PlanStatus::Valid && plan.next_capacity==6 &&
           plan.allocation_bytes==72 && plan.realloc_existing);
    plan=node_reserve(3,2,4,true);
    assert(plan.status==PlanStatus::Valid && plan.next_capacity==7 &&
           plan.allocation_bytes==84);
    plan=node_reserve(3,2,5,true);
    assert(plan.status==PlanStatus::Valid && plan.next_capacity==9);
    plan=node_reserve(3,-1,1,true);
    assert(plan.status==PlanStatus::FixedStorage);
    plan=node_reserve(3,-2,1,true);
    assert(plan.status==PlanStatus::InvalidInput);
    plan=node_reserve(std::numeric_limits<std::int32_t>::max(),0,1,true);
    assert(plan.status==PlanStatus::Overflow);
    plan=node_reserve(1000000000,0,1,true);
    assert(plan.status==PlanStatus::Overflow); // doubled capacity * 12 cannot fit native32

    plan=page_pointer_reserve(0,0,1,false);
    assert(plan.status==PlanStatus::Valid && plan.next_capacity==8 &&
           plan.allocation_bytes==32);
    plan=page_pointer_reserve(8,0,1,true);
    assert(plan.status==PlanStatus::Valid && plan.next_capacity==16 &&
           plan.allocation_bytes==64);
    plan=page_pointer_reserve(8,3,7,true);
    assert(plan.status==PlanStatus::Valid && plan.next_capacity==17);
    plan=page_pointer_reserve(10,0,0,true);
    assert(plan.status==PlanStatus::Valid && !plan.growth_needed &&
           plan.next_capacity==10);

    std::vector<PageHeader> pages{{64,64},{100,90},{2048,512}};
    assert(first_page_with_space(pages,1).value()==1);
    assert(first_page_with_space(pages,11).value()==2);
    assert(first_page_with_space(pages,1536).value()==2);
    assert(!first_page_with_space(pages,1537).has_value());
    assert(!first_page_with_space({},1).has_value());
    assert(first_page_with_space(pages,0).value()==0);
    pages[0]={32,64}; // guard impossible page used count
    assert(first_page_with_space(pages,1).value()==1);
    std::puts("PASS 2006 client token: 16bit-node offset, x12/x4 growth, fixed mode, page-space scan, host overflow guards");
}