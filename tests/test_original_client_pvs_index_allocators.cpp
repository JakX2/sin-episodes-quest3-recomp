#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include "sin_compat/original_client_pvs_index_allocators.hpp"
namespace n=sin_compat::original_client_pvs_index_allocators;

template<class A> struct GrowOne final:A::Growth {
    std::size_t calls{};
    void request_one(std::vector<typename A::Record>& records) override {
        ++calls; records.resize(records.size()+1u);
    }
};
template<class A,std::size_t Prev,std::size_t Next>
static void verify_layout(bool detached) {
    A a(2u);
    a.records[0u][0]=0xA5u;
    a.records[1u][0]=0xB6u;
    assert(a.allocate(detached)==0u);
    assert(a.next_unused==1u && a.cached_base_refreshes==1u);
    assert(a.read_word(0u,Prev)==(detached?n::kAbsent:0u));
    assert(a.read_word(0u,Next)==(detached?n::kAbsent:0u));
    assert(a.records[0u][0]==0xA5u);
    assert(a.allocate(!detached)==1u);
    assert(a.records[1u][0]==0xB6u);
    assert(a.cached_base_refreshes==2u);
    a.seed_free(1u,0u);
    a.seed_free(0u,n::kAbsent); // head=0 -> 0xFFFF
    assert(a.allocate(detached)==0u);
    assert(a.free_head==n::kAbsent && a.next_unused==2u);
    assert(a.records[0u][0]==0xA5u);
    GrowOne<A> growth;
    assert(a.allocate(detached,&growth)==2u);
    assert(growth.calls==1u && a.records.size()==3u);
    assert(a.next_unused==3u && a.cached_base_refreshes==4u);
    bool rejected=false;
    try{(void)a.allocate(detached);}catch(const std::out_of_range&){rejected=true;}
    assert(rejected && a.cached_base_refreshes==4u);
}
int main() {
    static_assert(n::kAuxAllocatorVA==0x24106570u);
    static_assert(n::kPvsAllocatorVA==0x24106610u);
    static_assert(n::kNativeReviewedBytes==308u);
    static_assert(n::kNativeFreeHeadWord==0x10u);
    static_assert(n::kNativeNextUnusedWord==0x14u);
    static_assert(n::kNativeCapacityDword==0x04u);
    static_assert(n::kNativeCachedBasePointerDword==0x18u);
    static_assert(sizeof(n::AuxEight::Record)==8u);
    static_assert(sizeof(n::PvsSixteen::Record)==16u);
    verify_layout<n::AuxEight,4u,6u>(false);
    verify_layout<n::AuxEight,4u,6u>(true);
    verify_layout<n::PvsSixteen,12u,14u>(false);
    verify_layout<n::PvsSixteen,12u,14u>(true);
    n::PvsSixteen reused(3u);
    reused.records[2u][1]=0xC7u;
    reused.seed_free(2u,1u);
    reused.write_word(1u,14u,n::kAbsent);
    assert(reused.allocate(false)==2u);
    assert(reused.free_head==1u && reused.records[2u][1]==0xC7u);
    assert(reused.allocate(true)==1u);
    assert(reused.free_head==n::kAbsent);
    n::AuxEight edge(0u);
    GrowOne<n::AuxEight> grow;
    assert(edge.allocate(true,&grow)==0u && grow.calls==1u);
    n::PvsSixteen sentinel(0u);
    sentinel.next_unused=n::kAbsent;
    bool rejected=false;
    try{(void)sentinel.allocate(false);}catch(const std::out_of_range&){rejected=true;}
    assert(rejected);
    return 0;
}
