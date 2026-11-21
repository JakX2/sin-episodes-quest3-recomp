#include "sin_compat/original_grunt_behavior_postinit.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace sin_compat::original_druglab_grunt::behavior_postinit;
namespace {
struct Call {
    Operation operation;
    std::uint8_t index;
    bool operator==(const Call& rhs) const noexcept {
        return operation==rhs.operation && index==rhs.index;
    }
};
struct Host final : IHost {
    Operation reject=Operation::None;
    int reject_index=-1;
    Operation throw_on=Operation::None;
    int throw_index=-1;
    Registrar* reenter=nullptr;
    Code nested_code=Code::Success;
    std::vector<Call> calls;
    std::size_t completed=0;
    std::size_t index_for(Spec spec) {
        for(std::size_t i=0;i<kOriginalBehaviors.size();++i)
            if(kOriginalBehaviors[i].kind==spec.kind) {
                assert(spec.owner_offset==kOriginalBehaviors[i].owner_offset);
                assert(spec.exact_rtti_name==kOriginalBehaviors[i].exact_rtti_name);
                return i;
            }
        assert(false);return 0;
    }
    bool hit(Operation op,std::size_t ix) {
        calls.push_back({op,static_cast<std::uint8_t>(ix)});
        if(throw_on==op && throw_index==static_cast<int>(ix))
            throw std::runtime_error("synthetic host error");
        if(reenter && op==Operation::BindNPC && ix==2)
            nested_code=reenter->initialize().code;
        return !(reject==op && reject_index==static_cast<int>(ix));
    }
    bool append_to_collection(Spec b,std::uint32_t a,std::uint32_t n) override {
        assert(a==0xC1C && n==0xC28);
        return hit(Operation::AppendToCollection,index_for(b));
    }
    bool bind_to_npc(Spec b) override {
        return hit(Operation::BindNPC,index_for(b));
    }
    bool set_owner_subobject_reference(Spec b,std::uint32_t off,std::uint32_t field) override {
        assert(off==0xC14 && field==0x0C);
        const auto ix=index_for(b);
        const bool ok=hit(Operation::StoreOwnerSubobject,ix);
        if(ok)++completed;
        return ok;
    }
    bool bind_sentence_owner(std::uint32_t off,std::uint32_t field) override {
        assert(off==0xCC0 && field==0x04);
        return hit(Operation::BindSentenceOwner,8);
    }
    bool initialize_sentence_parameters(std::uint32_t off,std::string_view key) override {
        assert(off==0xCC0 && key=="NPC_Grunt.SentenceParameters");
        return hit(Operation::InitializeSentenceParameters,8);
    }
};
std::vector<Call> expected_calls() {
    std::vector<Call> out;
    for(std::uint8_t i=0;i<8;++i) {
        out.push_back({Operation::AppendToCollection,i});
        out.push_back({Operation::BindNPC,i});
        out.push_back({Operation::StoreOwnerSubobject,i});
    }
    out.push_back({Operation::BindSentenceOwner,8});
    out.push_back({Operation::InitializeSentenceParameters,8});
    return out;
}
void test_exact_metadata_and_order() {
    assert(kOriginalBehaviors.size()==8);
    assert(kGruntPrimaryVtable==0x2249AAA4);
    assert(kBehaviorRegistrationVA==0x2235F6E0);
    assert(kBehaviorInitVslot==0x76C);
    assert(kSentenceKeyVslot==0x7AC);
    assert(kSentenceKeyMethodVA==0x22353F80);
    assert(kSentenceInitVA==0x22358FA0);
    assert(kOriginalBehaviors[5].exact_rtti_name=="CAI_AssaultBehavior");
    assert(kOriginalBehaviors[3].exact_rtti_name=="CAI_BeserkBehavior");
    assert(kOriginalBehaviors[4].owner_offset==0xF88);
    assert(kOriginalBehaviors[6].exact_rtti_name=="CGruntStandoffBehavior");
    assert(kOriginalBehaviors[6].original_vtable_va==0x22495344);
    assert(kStandoffBaseConstructorVtable==0x223FA1B4);
    assert(kSentenceObjectClass=="CAI_Sentence<CNPC_GruntBase>");
    Host host;Registrar x(host);
    const auto r=x.initialize();
    assert(r.ok() && r.registered_count==8 && r.behavior_index==8);
    assert(host.completed==8 && host.calls==expected_calls());
}
void test_every_rejection_path() {
    const auto calls=expected_calls();
    for(std::size_t target=0;target<calls.size();++target) {
        Host host;
        host.reject=calls[target].operation;
        host.reject_index=calls[target].index;
        Registrar x(host);
        const auto r=x.initialize();
        assert(r.code==Code::HostRefused);
        assert(r.failed_operation==calls[target].operation);
        assert(r.behavior_index==calls[target].index);
        assert(r.registered_count==target/3);
        assert(host.calls==std::vector<Call>(calls.begin(),calls.begin()+target+1));
    }
}
void test_reentrant_exception_retry() {
    Host host;Registrar x(host);host.reenter=&x;
    auto result=x.initialize();
    assert(result.ok() && host.nested_code==Code::Reentrant);
    Host throwing;Registrar y(throwing);
    throwing.throw_on=Operation::BindNPC;
    throwing.throw_index=3;
    result=y.initialize();
    assert(result.code==Code::HostRefused);
    assert(result.failed_operation==Operation::BindNPC);
    assert(result.behavior_index==3);
    assert(result.registered_count==3);
    throwing.throw_on=Operation::None;
    throwing.calls.clear();throwing.completed=0;
    result=y.initialize();
    assert(result.ok() && throwing.calls==expected_calls());
}
}
int main() {
    test_exact_metadata_and_order();
    test_every_rejection_path();
    test_reentrant_exception_retry();
    std::puts("PASS Grunt eight RTTI behavior insertions, owner binding, sentence config, 26 failure points");
}