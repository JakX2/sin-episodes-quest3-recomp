#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "sin_compat/original_cross_dll_tier0_allocator.hpp"
#include "sin_compat/original_server_cache_array_growth.hpp"
#include "sin_compat/original_server_cache_slot_allocation.hpp"
namespace x=sin_compat::original_cross_dll_tier0_allocator;
namespace grow=sin_compat::original_server_cache_array_growth;
namespace slot=sin_compat::original_server_cache_slot_allocation;
struct Host {
    bool debug=true;
    std::uint32_t owner=0xa500u,next=0x10000u;
    int owner_queries=0,metadata_queries=0;
    std::map<std::uint32_t,std::uint32_t> live{};
    std::vector<std::string> log{};
    std::uint32_t new_block(std::uint32_t sz) {
        next+=0x1000u;live[next]=sz;return next;
    }
    std::uint32_t resize(std::uint32_t ptr,std::uint32_t sz) {
        assert(live.erase(ptr)==1u);return new_block(sz);
    }
    std::uintptr_t resolve_filename_owner() {
        ++owner_queries;return debug?owner:0u;
    }
    bool metadata_enabled() {++metadata_queries;return debug;}
    std::uint32_t make_metadata() {return owner;}
    std::uintptr_t allocate_plain(std::uint32_t n) {
        log.emplace_back("reg-basic");return new_block(n);
    }
    std::uintptr_t allocate_with_owner(std::uint32_t n,std::uint32_t o,std::uint32_t z) {
        assert(o==owner && z==0u);
        log.emplace_back("reg-tagged");return new_block(n);
    }
    std::uintptr_t pair_plain(std::uint32_t p,std::uint32_t n) {
        log.emplace_back("pair-basic");return resize(p,n);
    }
    std::uintptr_t pair_with_owner(std::uint32_t p,std::uint32_t n,
                                   std::uint32_t o,std::uint32_t z) {
        assert(o==owner && z==0u);
        log.emplace_back("pair-tagged");return resize(p,n);
    }
    std::uint32_t allocate_basic(std::uint32_t n) {
        log.emplace_back("stack-basic");return new_block(n);
    }
    std::uint32_t allocate_tagged(std::uint32_t n,std::uint32_t o,std::uint32_t z) {
        assert(o==owner && z==0u);
        log.emplace_back("stack-tagged");return new_block(n);
    }
    std::uint32_t reallocate_basic(std::uint32_t p,std::uint32_t n) {
        log.emplace_back("resize-basic");return resize(p,n);
    }
    std::uint32_t reallocate_tagged(std::uint32_t p,std::uint32_t n,
                                    std::uint32_t o,std::uint32_t z) {
        assert(o==owner && z==0u);
        log.emplace_back("resize-tagged");return resize(p,n);
    }
    void deallocate_direct(std::uint32_t ptr) {
        log.emplace_back("free");assert(live.erase(ptr)==1u);
    }
};
struct Heap {
    x::ModuleState& state;Host& h;
    std::uint32_t allocate(std::uint32_t n) {
        return x::allocate_from_stack(state,h,n);
    }
    std::uint32_t resize(std::uint32_t p,std::uint32_t n) {
        return x::dispatch_stack_reallocation(state,h,p,n);
    }
};
struct PoolHost {
    Heap& heap;
    void grow(slot::Pool& pool,std::uint32_t additional) {
        grow::NativeVectorHeader32 v{
            pool.native.slots_04,pool.native.capacity_08,pool.native.unknown_0c};
        assert(grow::grow(v,additional,heap).action!=grow::Allocation::skipped_disabled);
        pool.native.slots_04=v.backing_00;
        pool.native.capacity_08=v.capacity_04;
        pool.slots.resize(v.capacity_04);
    }
    std::uint16_t next_free(const slot::Pool& pool,std::uint16_t i) {
        assert(i<pool.slots.size());return pool.slots[i].right_02;
    }
};
static void verify_twin(const x::Module cfg) {
    assert(cfg.primary_flag_va==cfg.primary_context_va+4u);
    assert(cfg.secondary_flag_va==cfg.secondary_context_va+4u);
    Host h;x::ModuleState state{};
    auto p=static_cast<std::uint32_t>(x::allocate_from_esi(state,h,48u));
    assert(state.original.alloc_flags_04==1u && state.original.alloc_context_00==h.owner);
    assert(h.owner_queries==1 && h.metadata_queries==0 && h.log.back()=="reg-tagged");
    auto q=x::allocate_from_stack(state,h,32u);
    assert(h.owner_queries==1 && h.metadata_queries==0 && h.log.back()=="stack-tagged");
    p=static_cast<std::uint32_t>(x::dispatch_from_edi_esi(state,h,p,96u));
    assert(state.original.realloc_flags_0c==1u && state.original.realloc_context_08==h.owner);
    assert(h.owner_queries==2 && h.log.back()=="pair-tagged");
    q=x::dispatch_stack_reallocation(state,h,q,64u);
    assert(h.owner_queries==2 && h.metadata_queries==0 && h.log.back()=="resize-tagged");
    x::deallocate_from_stack(h,p);x::deallocate_from_stack(h,q);
    assert(h.live.empty());
}
static void verify_zero_context_and_inverse_order() {
    Host h;h.debug=false;x::ModuleState state{};
    state.original.alloc_flags_04=0xa0u;
    state.original.realloc_flags_0c=0x20u;
    auto p=static_cast<std::uint32_t>(x::allocate_from_esi(state,h,24u));
    assert(state.original.alloc_flags_04==0xa1u && state.original.alloc_context_00==0u);
    h.debug=true;
    auto q=x::allocate_from_stack(state,h,16u);
    assert(h.owner_queries==1 && h.metadata_queries==0 && h.log.back()=="stack-basic");
    p=static_cast<std::uint32_t>(x::dispatch_from_edi_esi(state,h,p,48u));
    assert(state.original.realloc_flags_0c==0x21u && h.owner_queries==2);
    q=x::dispatch_stack_reallocation(state,h,q,32u);
    assert(h.metadata_queries==0 && h.log.back()=="resize-tagged");
    x::deallocate_from_stack(h,p);x::deallocate_from_stack(h,q);
    assert(h.live.empty());
    Host other;x::ModuleState second{};
    p=x::allocate_from_stack(second,other,9u);
    assert(other.metadata_queries==1 && other.owner_queries==0);
    q=static_cast<std::uint32_t>(x::allocate_from_esi(second,other,10u));
    assert(other.metadata_queries==1 && other.owner_queries==0);
    p=x::dispatch_stack_reallocation(second,other,p,15u);
    assert(other.metadata_queries==2);
    q=static_cast<std::uint32_t>(x::dispatch_from_edi_esi(second,other,q,20u));
    assert(other.metadata_queries==2 && other.owner_queries==0);
    x::deallocate_from_stack(other,p);x::deallocate_from_stack(other,q);
    assert(other.live.empty());
}
static void verify_phase194_and_201() {
    for(bool tagged:{false,true}) {
        Host h;h.debug=tagged;
        x::ModuleState state{};
        Heap heap{state,h};PoolHost ph{heap};
        slot::Pool pool;
        assert(slot::claim(pool,ph)==0u && pool.native.capacity_08==2u);
        assert(slot::claim(pool,ph)==1u);
        assert(slot::claim(pool,ph)==2u && pool.native.capacity_08==4u);
        assert(h.live.size()==1u && h.live.begin()->second==96u);
        assert(state.original.alloc_flags_04==1u);
        assert(state.original.realloc_flags_0c==1u);
        assert(state.original.alloc_context_00==(tagged?h.owner:0u));
        x::deallocate_from_stack(h,pool.native.slots_04);
        assert(h.live.empty());
    }
}

static void verify_native_pre_callback_flag_order() {
    // Original x86 ORs bit0 into the module-global flag BEFORE it calls
    // the 90-byte filename-context constructor. Observe the same global
    // directly from a host callback, not a deferred temporary copy.
    struct ObservingHost : Host {
        std::uint32_t* active_flag=nullptr;
        int observations=0;
        std::uintptr_t resolve_filename_owner() {
            assert(active_flag && (*active_flag&1u)!=0u);
            ++observations;
            return 0u;
        }
    };
    ObservingHost h;
    x::ModuleState state{};
    h.active_flag=&state.original.alloc_flags_04;
    auto first=static_cast<std::uint32_t>(x::allocate_from_esi(state,h,11u));
    assert(h.observations==1 && state.original.alloc_flags_04==1u);
    h.active_flag=&state.original.realloc_flags_0c;
    first=static_cast<std::uint32_t>(
        x::dispatch_from_edi_esi(state,h,first,22u));
    assert(h.observations==2 && state.original.realloc_flags_0c==1u);
    // Stack wrappers must see the same once-only FAILED context.
    auto second=x::allocate_from_stack(state,h,7u);
    second=x::dispatch_stack_reallocation(state,h,second,13u);
    assert(h.observations==2 && h.metadata_queries==0);
    x::deallocate_from_stack(h,first);
    x::deallocate_from_stack(h,second);
    assert(h.live.empty());
}

int main() {
    static_assert(sizeof(x::ModuleState)==16u);
    static_assert(x::kServer.original_register_alloc_va==0x2226e2f0u);
    static_assert(x::kServer.original_register_realloc_va==0x2226e340u);
    static_assert(x::kClient.original_free_va==0x2416f940u);
    static_assert(x::kClient.original_stack_realloc_va==0x2416f960u);
    verify_twin(x::kServer);verify_twin(x::kClient);
    verify_zero_context_and_inverse_order();
    verify_phase194_and_201();
    verify_native_pre_callback_flag_order();
}
