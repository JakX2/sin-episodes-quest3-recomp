// Phase533 integrated 2006 Grunt factory->real entity init->Spawn logic.
// All native-engine operations are synthetic. The C++ modules under test
// are ACTUAL independent project implementations, not bytecode copies.
#include "sin_compat/original_druglab_grunt_factory.hpp"
#include "sin_compat/original_druglab_grunt_entity_init.hpp"
#include "sin_compat/original_druglab_grunt_spawn.hpp"
#include "sin_compat/original_grunt_ai_subsystems.hpp"
#include "sin_compat/original_grunt_behavior_postinit.hpp"
#include "sin_compat/original_grunt_full_ai_postinit.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace factory=sin_compat::original_druglab_grunt;
namespace link=sin_compat::original_druglab_grunt::entity_init;
namespace spawn=sin_compat::original_druglab_grunt::spawn;
namespace ai=sin_compat::original_druglab_grunt::ai_init;
namespace behavior=sin_compat::original_druglab_grunt::behavior_postinit;
namespace full_ai=sin_compat::original_druglab_grunt::full_ai_postinit;

struct SimulatedNativeSource final :factory::IHost,link::IHost,spawn::IHost,ai::IHost,behavior::IHost {
    std::vector<std::string> steps{};
    std::string_view classname{};
    std::string weapon{};
    int mode=-1,value98=-1,value9c=-1;
    std::uintptr_t native_allocation=0x200000;
    bool live_edict=false,ctx_cleared=false;
    bool refuse_edict_attach=false;
    ai::Stage refuse_ai_stage=ai::Stage::None;
    std::size_t ai_created=0;
    std::size_t behavior_appended=0;
    std::size_t behavior_completed=0;
    bool sentence_owner_bound=false, sentence_parameters_loaded=false;
    behavior::Operation refuse_behavior_stage=behavior::Operation::None;
    bool postinit_complete=false;
    bool original_spawn_called=false;
    link::EntityInitializer entity_initializer{*this};
    ai::Initializer ai_initializer{*this};
    behavior::Registrar behavior_registrar{*this};
    full_ai::Initializer complete_ai_initializer{ai_initializer,behavior_registrar};

    std::uintptr_t allocate_native_object(std::size_t bytes) override {
        assert(bytes==0xFD8);steps.emplace_back("allocate");
        return native_allocation;
    }
    bool invoke_verified_base_constructor(std::uintptr_t p) override {
        assert(p==native_allocation);
        steps.emplace_back("base-constructor");return true;
    }
    bool establish_native_vtable(
        std::uintptr_t p,std::uint32_t offset,
        factory::VptrRole role) override {
        assert(p==native_allocation);
        assert((offset==0&&role==factory::VptrRole::Primary)||
               (offset==0x790&&role==factory::VptrRole::Secondary790)||
               (offset==0xc14&&role==factory::VptrRole::TertiaryC14));
        steps.emplace_back("install-vtable");return true;
    }
    bool invoke_native_virtual_70(
        std::uintptr_t p,std::uintptr_t arg) override {
        assert(p==native_allocation&&arg==0x1234);
        steps.emplace_back("virtual70");
        const auto result=entity_initializer.initialize({
            true,classname,0,true
        });
        return result.ok();
    }
    bool reclaim_unpublished_object(std::uintptr_t p) override {
        assert(p==native_allocation);
        steps.emplace_back("reclaim");
        return true;
    }
    bool invoke_delegate_virtual_18(std::uintptr_t) override{return true;}

    bool intern_native_classname(std::string_view n) override {
        assert(n==classname);steps.emplace_back("intern-classname");
        return true;
    }
    bool special_bit9_native_registration() override {
        steps.emplace_back("special-bit9");return true;
    }
    bool attach_native_networkable(bool has_edict_context) override {
        assert(has_edict_context);
        steps.emplace_back("attach-networkable-160");
        if(refuse_edict_attach)return false;
        live_edict=true;return true;
    }
    bool clear_original_factory_edict_context() override {
        steps.emplace_back("clear-global-context");
        ctx_cleared=true;return true;
    }
    bool has_live_native_edict() const override{return live_edict;}
    bool link_edict_networkable_backpointer() override {
        assert(live_edict);
        steps.emplace_back("backpointer-edict8");return true;
    }
    bool base_initialize_2214BF30_zero() override {
        steps.emplace_back("base-BF30");return true;
    }
    bool base_initialize_2214B280() override {
        steps.emplace_back("base-B280");return true;
    }
    bool invoke_real_grunt_postinit_slot3F8() override {
        assert(live_edict&&ctx_cleared);
        steps.emplace_back("npc-postinit3f8");
        const auto result=complete_ai_initializer.initialize();
        if(!result.ok())return false;
        assert(result.core.created_count==7);
        assert(result.behavior.registered_count==8);
        assert(sentence_owner_bound&&sentence_parameters_loaded);
        postinit_complete=true;return true;
    }
    bool accept_ai(ai::Stage stage,std::string name) {
        steps.emplace_back(name);
        return refuse_ai_stage!=stage;
    }
    bool construct_and_store(ai::ComponentSpec spec) override {
        assert(ai_created<ai::kOriginalComponents.size());
        assert(spec.kind==ai::kOriginalComponents[ai_created].kind);
        const auto stage=static_cast<ai::Stage>(
            static_cast<unsigned>(ai::Stage::Senses)+
            static_cast<unsigned>(ai_created));
        if(!accept_ai(stage,"ai-"+std::string(spec.rtti_name)))return false;
        ++ai_created;return true;
    }
    bool wire_embedded_owner_relay() override {
        return accept_ai(ai::Stage::OwnerRelay,"ai-owner-relay");
    }
    bool wire_motor_to_local_navigator() override {
        return accept_ai(ai::Stage::MotorToLocalNavigator,"ai-motor-localnav");
    }
    bool wire_local_navigator_to_navigator() override {
        return accept_ai(ai::Stage::LocalNavigatorToNavigator,"ai-localnav-nav");
    }
    bool wire_navigator_shared_context() override {
        return accept_ai(ai::Stage::NavigatorContext,"ai-nav-context");
    }
    bool wire_pathfinder_shared_context() override {
        return accept_ai(ai::Stage::PathfinderContext,"ai-path-context");
    }
    bool wire_tactical_services_shared_context() override {
        return accept_ai(ai::Stage::TacticalServicesContext,"ai-tact-context");
    }
    bool accept_behavior(behavior::Operation operation,std::string name) {
        steps.emplace_back(name);
        return refuse_behavior_stage!=operation;
    }
    bool append_to_collection(behavior::Spec spec,
                              std::uint32_t list_offset,
                              std::uint32_t count_field) override {
        assert(list_offset==behavior::kBehaviorArrayOffset);
        assert(count_field==behavior::kBehaviorArrayCountField);
        assert(behavior_appended<behavior::kOriginalBehaviors.size());
        assert(spec.kind==behavior::kOriginalBehaviors[behavior_appended].kind);
        if(!accept_behavior(behavior::Operation::AppendToCollection,
                            "behavior-append-"+std::string(spec.exact_rtti_name)))
            return false;
        ++behavior_appended;return true;
    }
    bool bind_to_npc(behavior::Spec spec) override {
        assert(behavior_appended>0);
        assert(spec.kind==behavior::kOriginalBehaviors[behavior_appended-1].kind);
        return accept_behavior(behavior::Operation::BindNPC,
                               "behavior-bind-"+std::string(spec.exact_rtti_name));
    }
    bool set_owner_subobject_reference(behavior::Spec spec,
                                      std::uint32_t npc_offset,
                                      std::uint32_t field_offset) override {
        assert(npc_offset==0xC14 && field_offset==0x0C);
        assert(behavior_appended>0);
        assert(spec.kind==behavior::kOriginalBehaviors[behavior_appended-1].kind);
        if(!accept_behavior(behavior::Operation::StoreOwnerSubobject,
                            "behavior-link-"+std::string(spec.exact_rtti_name)))
            return false;
        ++behavior_completed;return true;
    }
    bool bind_sentence_owner(std::uint32_t embedded_offset,
                             std::uint32_t owner_field) override {
        assert(behavior_completed==behavior::kOriginalBehaviors.size());
        assert(embedded_offset==0xCC0 && owner_field==0x04);
        if(!accept_behavior(behavior::Operation::BindSentenceOwner,
                            "sentence-bind-owner"))return false;
        sentence_owner_bound=true;return true;
    }
    bool initialize_sentence_parameters(std::uint32_t embedded_offset,
                                        std::string_view key) override {
        assert(sentence_owner_bound&&embedded_offset==0xCC0);
        assert(key=="NPC_Grunt.SentenceParameters");
        if(!accept_behavior(behavior::Operation::InitializeSentenceParameters,
                            "sentence-load-parameters"))return false;
        sentence_parameters_loaded=true;return true;
    }
    bool assign_original_weapon_symbol(std::string_view w) override {
        assert(postinit_complete);
        steps.emplace_back("weapon");
        weapon=std::string(w);
        return true;
    }
    bool commit_original_field_3B0(
        std::int32_t v,spawn::DirtyPath d) override {
        assert(postinit_complete);
        assert(d==spawn::DirtyPath::EdictStateBit0);
        mode=v;steps.emplace_back("field3b0");return true;
    }
    bool native_virtual_68() override {
        steps.emplace_back("spawn-v68");return true;
    }
    bool native_virtual_1C_then_6C() override {
        steps.emplace_back("spawn-v1c-v6c");return true;
    }
    bool inherited_npc_spawn_2235F2A0() override {
        original_spawn_called=true;
        steps.emplace_back("inherited-spawn");return true;
    }
    bool virtual_1B0_then_write_field_98(std::int32_t v) override {
        value98=v;steps.emplace_back("field98");return true;
    }
    bool virtual_1B4_then_write_field_9C(std::int32_t v) override {
        value9c=v;steps.emplace_back("field9c");return true;
    }
};
int main() {
    struct Case {std::string_view classname,expected_weapon;int mode;};
    const Case variants[]{
        {"npc_druglab_grunt_pistol","weapon_npc_pistol",1},
        {"npc_druglab_grunt_scattergun","weapon_scattergun",0},
        {"npc_druglab_grunt_assault_rifle","weapon_assault_rifle",2}
    };
    for(const auto& v:variants){
        SimulatedNativeSource h;
        h.classname=v.classname;
        factory::GruntFactoryReference allocator(h);
        spawn::SpawnReference spawner(h);
        const auto created=allocator.create(v.classname,0x1234);
        assert(created.ok()&&created.factory_return_view==0x200160);
        assert(h.postinit_complete&&h.ctx_cleared&&h.live_edict&&h.ai_created==7);
        assert(h.behavior_appended==8 && h.behavior_completed==8);
        assert(h.sentence_owner_bound && h.sentence_parameters_loaded);
        const auto spawned=spawner.apply({
            v.classname,"nonmatching_weapon",99,-1,-1,
            false,true,5.9f
        });
        assert(spawned.ok()&&h.original_spawn_called);
        assert(h.weapon==v.expected_weapon&&h.mode==v.mode);
        assert(h.value98==5&&h.value9c==5);
        std::vector<std::string> expected{
            "allocate","base-constructor","install-vtable",
            "install-vtable","install-vtable","virtual70",
            "intern-classname","attach-networkable-160",
            "clear-global-context","backpointer-edict8",
            "base-BF30","base-B280","npc-postinit3f8"
        };
        for(const auto& component:ai::kOriginalComponents)
            expected.emplace_back("ai-"+std::string(component.rtti_name));
        expected.insert(expected.end(),{
            "ai-owner-relay","ai-motor-localnav","ai-localnav-nav",
            "ai-nav-context","ai-path-context","ai-tact-context"
        });
        for(const auto& spec:behavior::kOriginalBehaviors) {
            expected.emplace_back("behavior-append-"+std::string(spec.exact_rtti_name));
            expected.emplace_back("behavior-bind-"+std::string(spec.exact_rtti_name));
            expected.emplace_back("behavior-link-"+std::string(spec.exact_rtti_name));
        }
        expected.insert(expected.end(),{
            "sentence-bind-owner","sentence-load-parameters",
            "weapon","field3b0","spawn-v68","spawn-v1c-v6c",
            "inherited-spawn","field98","field9c"
        });
        assert(h.steps==expected);
    }
    // Host cannot publish a fake NPC after failed real networkable bind.
    {
        SimulatedNativeSource h;
        h.classname="npc_druglab_grunt_pistol";
        h.refuse_edict_attach=true;
        factory::GruntFactoryReference allocator(h);
        const auto created=allocator.create(h.classname,0x1234);
        assert(created.code==factory::Code::HostRejected);
        assert(h.ctx_cleared&&!h.postinit_complete);
        assert(h.steps.back()=="reclaim");
        assert(!h.original_spawn_called&&h.weapon.empty());
    }
    // A failed original CAI_MoveProbe factory cannot publish a Grunt or spawn its weapon.
    {
        SimulatedNativeSource h;
        h.classname="npc_druglab_grunt_pistol";
        h.refuse_ai_stage=ai::Stage::MoveProbe;
        factory::GruntFactoryReference allocator(h);
        const auto created=allocator.create(h.classname,0x1234);
        assert(created.code==factory::Code::HostRejected);
        assert(h.ai_created==3&&!h.postinit_complete);
        assert(h.steps.back()=="reclaim");
        assert(!h.original_spawn_called&&h.weapon.empty());
    }
    // Grunt must not publish its entity or spawn a weapon if final sentence
    // configuration fails after all eight behaviors are registered.
    {
        SimulatedNativeSource h;
        h.classname="npc_druglab_grunt_assault_rifle";
        h.refuse_behavior_stage=behavior::Operation::InitializeSentenceParameters;
        factory::GruntFactoryReference allocator(h);
        const auto created=allocator.create(h.classname,0x1234);
        assert(created.code==factory::Code::HostRejected);
        assert(h.ai_created==7 && h.behavior_completed==8);
        assert(h.sentence_owner_bound && !h.sentence_parameters_loaded);
        assert(!h.postinit_complete && !h.original_spawn_called);
        assert(h.weapon.empty() && h.steps.back()=="reclaim");
    }
    std::puts("PASS original Grunt factory->edict->7 CAI systems->8 behaviors->sentence->3 weapon Spawn variants (synthetic host)");
}
