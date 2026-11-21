#include "sin_compat/original_grunt_sidestep_gate.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

using namespace sin_compat::original_grunt_sidestep;
namespace {
struct FakeHost final : IHost, IState3Fallback {
    std::vector<std::string> calls;
    bool cond4=false, cond3=false, cond4d=false;
    bool owner_accepts=true, manager_accepts=true, mobility_accepts=true;
    std::uint32_t fallback_value=0;
    bool throws=false;
    EligibilityGate* nested=nullptr;
    Exit nested_exit=Exit::Eligible;
    float last_factor=0.0f;
    bool has_original_condition(std::uint8_t id) override {
        calls.emplace_back("condition-"+std::to_string(id));
        if (throws) throw std::runtime_error("synthetic condition exception");
        if (nested && id==4) nested_exit=nested->evaluate({}).exit;
        if (id==4) return cond4;
        if (id==3) return cond3;
        assert(id==0x4D);
        return cond4d;
    }
    bool call_owner_virtual_304_with_original_context() override {
        calls.emplace_back("owner-304");
        return owner_accepts;
    }
    bool call_manager_virtual_300_with_owner() override {
        calls.emplace_back("manager-300");
        return manager_accepts;
    }
    bool original_rules_mobility_decision(float factor) override {
        calls.emplace_back("rules-1a0");
        last_factor=factor;
        return mobility_accepts;
    }
    std::uint32_t invoke_original_virtual_844() override {
        calls.emplace_back("fallback-844");
        return fallback_value;
    }
};
void assert_trace(const FakeHost& host,std::vector<std::string> expected) {
    assert(host.calls==expected);
}
void test_vtable_and_offsets() {
    static_assert(kGruntVtableVA==0x2249AAA4);
    static_assert(kOriginalGateVA==0x22359620);
    static_assert(kOriginalSelectorVA==0x2235E4E0);
    static_assert(kNativeGateVirtualOffset==0x848);
    static_assert(kNativeFallbackVirtualOffset==0x844);
    static_assert(kOriginalPositiveScheduleId==0x55);
    static_assert(kInPlayerSightsCondition==0x4D);
}
void test_all_early_rejections() {
    {
        FakeHost h;EligibilityGate gate(h);
        assert(gate.evaluate({true,true,true}).exit==Exit::Ca0Set);
        assert(h.calls.empty());
    }
    {
        FakeHost h;EligibilityGate gate(h);
        assert(gate.evaluate({false,true,true}).exit==Exit::Flag988Set);
        assert(h.calls.empty());
    }
    {
        FakeHost h;h.cond4=true;EligibilityGate gate(h);
        assert(gate.evaluate({}).exit==Exit::Condition4Present);
        assert_trace(h,{"condition-4"});
    }
    {
        FakeHost h;h.cond3=true;EligibilityGate gate(h);
        assert(gate.evaluate({}).exit==Exit::Condition3Present);
        assert_trace(h,{"condition-4","condition-3"});
    }
    {
        FakeHost h;EligibilityGate gate(h);
        assert(gate.evaluate({false,false,true}).exit==Exit::Field8cNonzero);
        assert_trace(h,{"condition-4","condition-3"});
    }
    {
        FakeHost h;h.owner_accepts=false;EligibilityGate gate(h);
        assert(gate.evaluate({}).exit==Exit::OwnerVirtual304Rejected);
        assert_trace(h,{"condition-4","condition-3","owner-304"});
    }
    {
        FakeHost h;h.manager_accepts=false;EligibilityGate gate(h);
        assert(gate.evaluate({}).exit==Exit::ManagerVirtual300Rejected);
        assert_trace(h,{"condition-4","condition-3","owner-304","manager-300"});
    }
    {
        FakeHost h;h.mobility_accepts=false;EligibilityGate gate(h);
        const Decision r=gate.evaluate({});
        assert(r.exit==Exit::MobilityRuleRejected && r.requested_mobility_factor==1.0f);
        assert_trace(h,{"condition-4","condition-3","owner-304","manager-300","condition-77","rules-1a0"});
    }
}
void test_two_authenticated_factors() {
    const std::vector<std::string> sequence={
        "condition-4","condition-3","owner-304","manager-300","condition-77","rules-1a0"};
    for(bool seen:{false,true}) {
        FakeHost h;h.cond4d=seen;EligibilityGate gate(h);
        const auto r=gate.evaluate({});
        assert(r.eligible());
        assert(r.requested_mobility_factor==(seen?4.0f:1.0f));
        assert(h.last_factor==(seen?4.0f:1.0f));
        assert_trace(h,sequence);
    }
}
void test_original_selector_state_three() {
    {
        FakeHost h;EligibilityGate gate(h);
        auto r=select_original_state_three(2,gate,{},h);
        assert(!r.handled&&h.calls.empty());
    }
    {
        FakeHost h;EligibilityGate gate(h);
        auto r=select_original_state_three(3,gate,{},h);
        assert(r.handled&&r.original_schedule_id==0x55&&r.gate.eligible());
        assert(h.calls.back()=="rules-1a0");
    }
    {
        FakeHost h;h.cond4=true;h.fallback_value=0x6E;
        EligibilityGate gate(h);
        const auto r=select_original_state_three(3,gate,{},h);
        assert(r.handled&&r.original_schedule_id==0x6E);
        assert_trace(h,{"condition-4","fallback-844"});
    }
    {
        FakeHost h;h.cond4=true;EligibilityGate gate(h);
        auto r=select_original_state_three(3,gate,{},h);
        assert(!r.handled&&r.original_schedule_id==0);
        assert_trace(h,{"condition-4","fallback-844"});
    }
}
void test_exception_and_reentrancy() {
    FakeHost h;EligibilityGate gate(h);
    h.nested=&gate;
    auto r=gate.evaluate({});
    assert(r.eligible()&&h.nested_exit==Exit::Reentrant);
    h.calls.clear();h.nested=nullptr;h.throws=true;
    r=gate.evaluate({});
    assert(r.exit==Exit::HostException);
    const std::size_t count=h.calls.size();
    const auto state3=select_original_state_three(3,gate,{},h);
    assert(!state3.handled&&state3.gate.exit==Exit::HostException);
    assert(h.calls.size()==count+1); // no virtual844 on host exception
    h.throws=false;h.calls.clear();
    r=gate.evaluate({});
    assert(r.eligible()); // busy guard released even when callbacks throw
}
}
int main() {
    test_vtable_and_offsets();
    test_all_early_rejections();
    test_two_authenticated_factors();
    test_original_selector_state_three();
    test_exception_and_reentrancy();
    std::puts("PASS Grunt original +0x848 sidestep gate, state3 schedule 0x55, original +0x844 fallback (synthetic host)");
}