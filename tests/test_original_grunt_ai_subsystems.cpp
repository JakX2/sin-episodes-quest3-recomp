#include "sin_compat/original_grunt_ai_subsystems.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <vector>

using namespace sin_compat::original_druglab_grunt::ai_init;
namespace {
constexpr Stage kStages[]{Stage::Senses,Stage::BlendedMotor,
    Stage::LocalNavigator,Stage::MoveProbe,Stage::Navigator,
    Stage::Pathfinder,Stage::TacticalServices,Stage::OwnerRelay,
    Stage::MotorToLocalNavigator,Stage::LocalNavigatorToNavigator,
    Stage::NavigatorContext,Stage::PathfinderContext,
    Stage::TacticalServicesContext};
struct Host final:IHost {
    Stage refuse=Stage::None, throws_at=Stage::None;
    std::vector<Stage> calls;
    std::vector<ComponentSpec> created;
    Initializer* reenter=nullptr;
    Code nested_code=Code::Success;
    bool accept(Stage s) {
        calls.push_back(s);
        if(throws_at==s)throw std::runtime_error("fake native callback");
        if(reenter && s==Stage::BlendedMotor)
            nested_code=reenter->initialize().code;
        return refuse!=s;
    }
    bool construct_and_store(ComponentSpec spec) override {
        const auto index=created.size();
        assert(index<kOriginalComponents.size());
        assert(spec.kind==kOriginalComponents[index].kind);
        assert(spec.owner_field_offset==kOriginalComponents[index].owner_field_offset);
        if(!accept(kStages[index]))return false;
        created.push_back(spec);
        return true;
    }
    bool wire_embedded_owner_relay() override {
        assert(created.size()==7);return accept(Stage::OwnerRelay);
    }
    bool wire_motor_to_local_navigator() override {
        return accept(Stage::MotorToLocalNavigator);
    }
    bool wire_local_navigator_to_navigator() override {
        return accept(Stage::LocalNavigatorToNavigator);
    }
    bool wire_navigator_shared_context() override {
        return accept(Stage::NavigatorContext);
    }
    bool wire_pathfinder_shared_context() override {
        return accept(Stage::PathfinderContext);
    }
    bool wire_tactical_services_shared_context() override {
        return accept(Stage::TacticalServicesContext);
    }
};
void test_verified_specs() {
    assert(kOriginalInitializerVA==0x22031D50);
    assert(kOriginalComponents.size()==7);
    assert(kOriginalComponents[0].rtti_name=="CAI_Senses");
    assert(kOriginalComponents[1].rtti_name=="CAI_BlendedMotor");
    assert(kOriginalComponents[2].rtti_name=="CAI_LocalNavigator");
    assert(kOriginalComponents[3].rtti_name=="CAI_MoveProbe");
    assert(kOriginalComponents[4].rtti_name=="CAI_Navigator");
    assert(kOriginalComponents[5].rtti_name=="CAI_Pathfinder");
    assert(kOriginalComponents[6].rtti_name=="CAI_TacticalServices");
    assert(kOriginalComponents[0].owner_field_offset==0x898);
    assert(kOriginalComponents[6].owner_field_offset==0x9D8);
    assert(kSharedContextGlobalVA==0x225490C8);
}
void test_successful_native_sequence() {
    Host host;
    Initializer x(host);
    const auto r=x.initialize();
    assert(r.ok()&&r.created_count==7&&r.wiring_completed);
    assert(r.failed_stage==Stage::None);
    assert(host.calls==std::vector<Stage>(std::begin(kStages),std::end(kStages)));
}
void test_failure_at_every_stage() {
    for(std::size_t i=0;i<std::size(kStages);++i) {
        Host host;
        host.refuse=kStages[i];
        Initializer x(host);
        const auto r=x.initialize();
        assert(r.code==Code::HostRefused&&!r.wiring_completed);
        assert(r.failed_stage==kStages[i]);
        assert(r.created_count==(i<7?i:7));
        assert(host.calls.size()==i+1);
        for(std::size_t n=0;n<host.calls.size();++n)
            assert(host.calls[n]==kStages[n]);
    }
}
void test_exceptions_reentrancy_and_reuse() {
    Host host;
    Initializer x(host);
    host.reenter=&x;
    auto r=x.initialize();
    assert(r.ok()&&host.nested_code==Code::Reentrant);
    Host other;
    other.throws_at=Stage::LocalNavigatorToNavigator;
    Initializer y(other);
    r=y.initialize();
    assert(r.code==Code::HostRefused);
    assert(r.failed_stage==Stage::LocalNavigatorToNavigator);
    assert(r.created_count==7);
    other.throws_at=Stage::None;
    other.calls.clear();other.created.clear();
    r=y.initialize();
    assert(r.ok());
}
}
int main() {
    test_verified_specs();
    test_successful_native_sequence();
    test_failure_at_every_stage();
    test_exceptions_reentrancy_and_reuse();
    std::puts("PASS Grunt native AI seven RTTI class factories, ordered wiring and 13 failure stages (synthetic host)");
}