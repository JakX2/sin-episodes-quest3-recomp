// Phase614: attach behavior only to engine-confirmed native Grunts.
// All game NPCs are synthetic host identities, NOT allocated by registry.
#include "sin_compat/original_grunt_npc_session_registry_phase614.hpp"
#include <cassert>
#include <cstdint>
#include <string_view>
namespace reg=sin_compat::grunt_npc_session_registry_phase614;

struct Mock {
  bool live=true,allow_rappel=true;
  std::uint32_t liveness_calls=0,callback_count=0;
};
static int32_t live(void* u,sin_grunt_actor_v1 actor){
  auto& m=*static_cast<Mock*>(u);++m.liveness_calls;
  return m.live && actor.index>0 && actor.generation>0 ? 1 : 0;
}
static int32_t candidate(void* u,sin_grunt_actor_v1,uint32_t index){
  auto& m=*static_cast<Mock*>(u);
  return index==0 && m.allow_rappel ? 1 : 0;
}
static int32_t no_gate(void*,sin_grunt_actor_v1){return 0;}
static int32_t no_condition(void*,sin_grunt_actor_v1,uint32_t){return 0;}
static int32_t no_entity(void*,uint32_t,uint32_t){return 0;}
static int32_t no_byte(void*,uint32_t,uint8_t*){return 0;}
static int32_t set_condition(void*,sin_grunt_actor_v1,uint32_t){return 1;}
static int32_t resolve(void*,uint32_t,uint64_t*){return 0;}
static int32_t read_link(void*,uint64_t,uint32_t* out){*out=0xffffffffu;return 1;}
static int32_t write_link(void*,uint64_t,uint32_t){return 1;}
static int32_t lifecycle(void* u,sin_grunt_actor_v1,uint32_t,uint32_t){
  ++static_cast<Mock*>(u)->callback_count;return 1;
}
static int32_t notify(void* u,sin_grunt_actor_v1,int32_t,int32_t){
  ++static_cast<Mock*>(u)->callback_count;return 1;
}
static sin_grunt_behavior_host_v1 callbacks(){
  return {SIN_GRUNT_BEHAVIOR_ABI_V1,sizeof(sin_grunt_behavior_host_v1),
          live,candidate,no_gate,no_condition,no_gate,no_entity,no_byte,
          set_condition,resolve,read_link,write_link,lifecycle,notify};
}
struct Engine final:reg::NativeHost {
  bool confirmed=true;
  int confirms=0;
  reg::Registry* registry=nullptr;
  reg::Status reentry=reg::Status::NotFound;
  bool confirmed_existing_native_npc(std::string_view name,
       sin_grunt_actor_v1 actor,std::uint64_t owner) override {
    ++confirms;
    assert(reg::Registry::supports_original_class(name));
    if(registry){
      reg::Attachment attempt{};
      attempt.original_classname=name;
      attempt.actor=actor;attempt.owner_entity_token=owner;
      reentry=registry->attach_existing_native(attempt);
    }
    return confirmed;
  }
};
static reg::Attachment attachment(Mock& m,sin_grunt_actor_v1 actor,
                                 std::string_view name="npc_druglab_grunt_pistol"){
  reg::Attachment a;
  a.original_classname=name;
  a.actor=actor;
  a.owner_entity_token=actor.index+100u;
  a.registered_condition_3b=701u;
  a.registered_condition_18=702u;
  a.callbacks=callbacks();a.callback_userdata=&m;
  a.initial_assault={0xffffffffu,0xffffffffu,0u,0u,0u,0u,0u,0.f,0.f};
  return a;
}
static void test_exact_original_names(){
  assert(reg::Registry::supports_original_class("npc_druglab_grunt_pistol"));
  assert(reg::Registry::supports_original_class("npc_druglab_grunt_scattergun"));
  assert(reg::Registry::supports_original_class("npc_druglab_grunt_assault_rifle"));
  assert(!reg::Registry::supports_original_class("npc_mobgrunt"));
  assert(!reg::Registry::supports_original_class("prop_physics"));
}
static void test_host_denied_or_dead_never_attaches(){
  Engine engine;reg::Registry registry(engine);Mock m;
  auto a=attachment(m,{17u,5u});
  engine.confirmed=false;
  assert(registry.attach_existing_native(a)==reg::Status::HostRejected);
  assert(registry.count()==0);
  engine.confirmed=true;m.live=false;
  assert(registry.attach_existing_native(a)==reg::Status::HostRejected);
  assert(registry.count()==0);
  m.live=true;a.original_classname="npc_mobgrunt";
  assert(registry.attach_existing_native(a)==reg::Status::UnsupportedClass);
  assert(registry.count()==0);
  a.original_classname="npc_druglab_grunt_pistol";a.actor.generation=0;
  assert(registry.attach_existing_native(a)==reg::Status::InvalidIdentity);
}
static void test_attach_step_snapshot_detach_generation(){
  Engine engine;reg::Registry registry(engine);Mock m;
  auto a=attachment(m,{17u,5u});
  assert(registry.attach_existing_native(a)==reg::Status::Attached);
  assert(registry.count()==1&&engine.confirms==1);
  auto older=attachment(m,{17u,4u});
  assert(registry.attach_existing_native(older)==reg::Status::DuplicateIndex);
  auto newer=attachment(m,{17u,6u});
  assert(registry.attach_existing_native(newer)==reg::Status::DuplicateIndex);
  assert(registry.detach({17u,4u})==reg::Status::StaleGeneration);
  auto invalid=registry.step({17u,4u},1.f);
  assert(invalid.status==reg::Status::StaleGeneration);
  auto result=registry.step({17u,5u},1.f);
  assert(result.status==reg::Status::Advanced &&
         result.underlying_abi_status==SIN_GRUNT_BEHAVIOR_OK&&
         result.selected==0);
  assert(m.callback_count==3);
  sin_grunt_snapshot_v1 state{};
  assert(registry.snapshot({17u,5u},state)==reg::Status::Advanced);
  assert(state.current_index==0);
  assert(registry.snapshot({17u,4u},state)==reg::Status::StaleGeneration);
  m.live=false;
  assert(registry.step({17u,5u},1.f).underlying_abi_status==
         SIN_GRUNT_BEHAVIOR_ACTOR_EXPIRED);
  // Even after NPC removal, cleanly release old per-session state.
  assert(registry.detach({17u,5u})==reg::Status::Detached);
  assert(registry.count()==0);
  assert(registry.detach({17u,5u})==reg::Status::NotFound);
  m.live=true;
  assert(registry.attach_existing_native(newer)==reg::Status::Attached);
  assert(registry.detach({17u,5u})==reg::Status::StaleGeneration);
  assert(registry.detach({17u,6u})==reg::Status::Detached);
}
static void test_three_native_classes_and_reentrant_host(){
  Engine engine;reg::Registry registry(engine);Mock m;
  engine.registry=&registry;
  assert(registry.attach_existing_native(attachment(m,{1u,1u}))==reg::Status::Attached);
  assert(engine.reentry==reg::Status::Reentrant);
  engine.registry=nullptr;
  assert(registry.attach_existing_native(attachment(m,{2u,1u},
      "npc_druglab_grunt_scattergun"))==reg::Status::Attached);
  assert(registry.attach_existing_native(attachment(m,{3u,1u},
      "npc_druglab_grunt_assault_rifle"))==reg::Status::Attached);
  assert(registry.count()==3);
  assert(registry.detach({2u,1u})==reg::Status::Detached);
  assert(registry.detach({1u,1u})==reg::Status::Detached);
  assert(registry.detach({3u,1u})==reg::Status::Detached);
}
int main(){
  test_exact_original_names();
  test_host_denied_or_dead_never_attaches();
  test_attach_step_snapshot_detach_generation();
  test_three_native_classes_and_reentrant_host();
}
