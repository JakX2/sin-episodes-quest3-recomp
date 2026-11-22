#include "sin_compat/original_druglab_grunt_factory.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

using namespace sin_compat::original_druglab_grunt;

struct Host final : IHost {
    std::vector<std::string> calls;
    std::uintptr_t next_allocation=0x100000u;
    std::uintptr_t forwarded=0;
    std::uintptr_t delegated=0;
    bool allocation_fails=false;
    bool base_ok=true,virtual70_ok=true,release_ok=true,delegate_ok=true;
    int fail_vptr=-1;
    int vptr_counter=0,release_calls=0;
    GruntFactoryReference* reenter=nullptr;
    Code nested_code=Code::Created;
    std::uintptr_t allocate_native_object(std::size_t bytes) override {
        assert(bytes==kOriginalAllocationBytes);
        calls.emplace_back("alloc-0xFD8");
        return allocation_fails?0:next_allocation;
    }
    bool invoke_verified_base_constructor(std::uintptr_t p) override {
        assert(p==next_allocation);
        calls.emplace_back("base-ctor");
        return base_ok;
    }
    bool establish_native_vtable(
        std::uintptr_t p,std::uint32_t offset,VptrRole role) override {
        assert(p==next_allocation);
        const int n=vptr_counter++;
        if(n==0){assert(offset==0&&role==VptrRole::Primary);
            calls.emplace_back("vptr-0");}
        if(n==1){assert(offset==0x790&&role==VptrRole::Secondary790);
            calls.emplace_back("vptr-790");}
        if(n==2){assert(offset==0xC14&&role==VptrRole::TertiaryC14);
            calls.emplace_back("vptr-c14");}
        return n!=fail_vptr;
    }
    bool invoke_native_virtual_70(
        std::uintptr_t p,std::uintptr_t argument) override {
        assert(p==next_allocation);
        calls.emplace_back("virtual70");
        forwarded=argument;
        if(reenter)nested_code=reenter->create(
            "npc_druglab_grunt_pistol",argument).code;
        return virtual70_ok;
    }
    bool reclaim_unpublished_object(std::uintptr_t p) override {
        assert(p==next_allocation);
        calls.emplace_back("rollback");
        ++release_calls;
        return release_ok;
    }
    bool invoke_delegate_virtual_18(std::uintptr_t p) override {
        delegated=p;
        calls.emplace_back("virtual18");
        return delegate_ok;
    }
};
void test_shared_four_alias_constructor_path() {
    constexpr const char* classes[]{
        "npc_druglab_grunt",
        "npc_druglab_grunt_pistol",
        "npc_druglab_grunt_scattergun",
        "npc_druglab_grunt_assault_rifle"};
    constexpr Variant variants[]{
        Variant::Base,Variant::Pistol,Variant::Scattergun,Variant::AssaultRifle};
    for(int i=0;i<4;++i){
        Host h;
        GruntFactoryReference factory(h);
        const auto r=factory.create(classes[i],0xCAFED00Du);
        assert(r.ok()&&r.variant==variants[i]);
        assert(r.allocation_base==0x100000u);
        assert(r.factory_return_view==0x100160u);
        assert(h.forwarded==0xCAFED00Du);
        assert(h.vptr_counter==3&&h.release_calls==0);
        assert((h.calls==std::vector<std::string>{
            "alloc-0xFD8","base-ctor","vptr-0",
            "vptr-790","vptr-c14","virtual70"
        }));
    }
}
void test_no_fabricated_grunt_alias() {
    Host h;
    GruntFactoryReference factory(h);
    assert(GruntFactoryReference::classify("npc_druglab_grunt_elite")==
           Variant::Unsupported);
    assert(factory.create("npc_sintek_grunt",0).code==
           Code::UnsupportedClass);
    assert(factory.create("npc_druglab_grunt_elite",0).code==
           Code::UnsupportedClass);
    assert(factory.create("npc_druglab_grunt_pistol_extra",0).code==
           Code::UnsupportedClass);
    assert(h.calls.empty());
}
void test_fail_closed_by_original_constructor_stage() {
    for(int step=0;step<5;++step){
        Host h;
        if(step==0)h.base_ok=false;
        if(step>=1&&step<=3)h.fail_vptr=step-1;
        if(step==4)h.virtual70_ok=false;
        GruntFactoryReference factory(h);
        const auto r=factory.create("npc_druglab_grunt_pistol",0);
        assert(r.code==Code::HostRejected);
        assert(r.factory_return_view==0);
        assert(h.release_calls==1&&h.calls.back()=="rollback");
    }
    {
        Host h;
        h.allocation_fails=true;
        GruntFactoryReference factory(h);
        assert(factory.create("npc_druglab_grunt_pistol",0).code==
               Code::AllocationFailed);
        assert(h.release_calls==0);
    }
    {
        Host h;
        h.base_ok=false;h.release_ok=false;
        GruntFactoryReference factory(h);
        assert(factory.create("npc_druglab_grunt_pistol",0).code==
               Code::RollbackFailed);
    }
}
void test_original_subobject_adjustment_safety() {
    Host h;
    h.next_allocation=std::numeric_limits<std::uintptr_t>::max()-0x100u;
    GruntFactoryReference factory(h);
    const auto result=factory.create(
        "npc_druglab_grunt_scattergun",0);
    assert(result.code==Code::PointerOverflow);
    assert(h.release_calls==1);
    assert((h.calls==std::vector<std::string>{"alloc-0xFD8","rollback"}));
}
void test_creator_is_nonreentrant_and_delegate_safely_dispatches() {
    Host h;
    GruntFactoryReference factory(h);
    h.reenter=&factory;
    const auto result=factory.create(
        "npc_druglab_grunt_assault_rifle",8);
    assert(result.ok());
    assert(h.nested_code==Code::Reentrant);
    assert(h.calls.size()==6);
    assert(factory.delegate_release(0));
    assert(h.calls.size()==6);
    assert(factory.delegate_release(0x100160u));
    assert(h.delegated==0x100160u);
    assert(h.calls.back()=="virtual18");
    h.delegate_ok=false;
    assert(!factory.delegate_release(0x100160u));
}
int main() {
    test_shared_four_alias_constructor_path();
    test_no_fabricated_grunt_alias();
    test_fail_closed_by_original_constructor_stage();
    test_original_subobject_adjustment_safety();
    test_creator_is_nonreentrant_and_delegate_safely_dispatches();
    std::puts("PASS original Druglab Grunt 4-alias factory: size 0xFD8, vptr+0/790/C14, base ctor, virtual70, returned+0x160, delegate virtual18 and safe rollback (synthetic host)");
}
