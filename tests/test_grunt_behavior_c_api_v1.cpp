// Phase613: external host callback integration against a genuinely linked ABI.
// No retail DLL is included. All entities are host-managed test tokens.
#include "sin_compat/grunt_behavior_c_api_v1.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

struct Mock {
  sin_grunt_behavior_session_v1* session=nullptr;
  int actor_live=1, other_rappel=0, other_follow=0, gate=1;
  int condition3b=0, owner_target=0, primary=1;
  int fail_condition=0, fail_serial=0;
  std::uint32_t link=0x22222009u;
  std::uint32_t set_condition_id=0, has_condition_id=0;
  int32_t reenter_status=99, destroy_reenter_status=99;
  int starts=0, ends=0, notifications=0, unlink_count=0, presence_queries=0;
  std::vector<std::string> order;
};
static int32_t live(void* u,sin_grunt_actor_v1 a) {
  assert(a.index==11 && a.generation==7);
  return static_cast<Mock*>(u)->actor_live;
}
static int32_t other(void* u,sin_grunt_actor_v1, uint32_t i) {
  auto& m=*static_cast<Mock*>(u);
  if(i==SIN_GRUNT_ASSAULT_INDEX_V1)assert(false && "Assault eligibility must be native model");
  return i==0?m.other_rappel:i==7?m.other_follow:0;
}
static int32_t gate(void* u,sin_grunt_actor_v1) {return static_cast<Mock*>(u)->gate;}
static int32_t has_cond(void* u,sin_grunt_actor_v1,uint32_t c) {
  auto& m=*static_cast<Mock*>(u);
  m.has_condition_id=c;
  assert(c==7001u);
  return m.condition3b;
}
static int32_t owner_target(void* u,sin_grunt_actor_v1) {
  return static_cast<Mock*>(u)->owner_target;
}
static int32_t present(void* u,uint32_t index,uint32_t serial) {
  auto& m=*static_cast<Mock*>(u);++m.presence_queries;
  if(m.fail_serial)return -1;
  return (index==0x123u&&serial==0xabcdeu)||
         (index==0x001u&&serial==0x54321u);
}
static int32_t primary(void* u,uint32_t encoded,uint8_t* byte) {
  assert(encoded==0xabcde123u);
  auto& m=*static_cast<Mock*>(u);
  *byte=static_cast<uint8_t>(m.primary);
  return 1;
}
static int32_t set_cond(void* u,sin_grunt_actor_v1,uint32_t c) {
  auto& m=*static_cast<Mock*>(u);
  m.set_condition_id=c;
  assert(c==7002u);
  return m.fail_condition?-1:1;
}
static int32_t resolve(void*,uint32_t encoded,uint64_t* token) {
  if(encoded==0x54321001u){*token=12u;return 1;}
  if(encoded==0x22222009u){*token=9u;return 1;}
  if(encoded==0x33333008u){*token=8u;return 1;}
  return 0;
}
static int32_t read_link(void* u,uint64_t target,uint32_t* encoded) {
  assert(target==12u);
  *encoded=static_cast<Mock*>(u)->link;
  return 1;
}
static int32_t write_link(void* u,uint64_t target,uint32_t encoded) {
  auto& m=*static_cast<Mock*>(u);
  assert(target==12u&&encoded==0xffffffffu);
  m.link=encoded;++m.unlink_count;
  return 1;
}
static int32_t lifecycle(void* u,sin_grunt_actor_v1,uint32_t i,uint32_t stage) {
  auto& m=*static_cast<Mock*>(u);
  m.order.push_back("life"+std::to_string(i)+":"+std::to_string(stage));
  if(stage==2)++m.ends;else ++m.starts;
  if(m.session && m.reenter_status==99) {
    sin_grunt_snapshot_v1 snap{};
    m.reenter_status=sin_grunt_behavior_snapshot_v1(m.session,&snap);
    m.destroy_reenter_status=sin_grunt_behavior_destroy_v1(m.session);
  }
  return 1;
}
static int32_t notify(void* u,sin_grunt_actor_v1,int32_t old,int32_t next) {
  auto& m=*static_cast<Mock*>(u);
  m.order.push_back("notify"+std::to_string(old)+":"+std::to_string(next));
  ++m.notifications;
  return 1;
}
static sin_grunt_behavior_host_v1 callbacks() {
  return {SIN_GRUNT_BEHAVIOR_ABI_V1,sizeof(sin_grunt_behavior_host_v1),
    live,other,gate,has_cond,owner_target,present,primary,set_cond,
    resolve,read_link,write_link,lifecycle,notify};
}
static sin_grunt_assault_state_v1 original_assault() {
  return {0xabcde123u,0x54321001u,1u,6u,7u,0u,0u,1.f,1.f};
}
static sin_grunt_behavior_session_v1* create(Mock& m,
                                             sin_grunt_assault_state_v1 s=original_assault()) {
  auto c=callbacks();
  auto* session=sin_grunt_behavior_create_v1(&c,&m,{11u,7u},9u,7001u,7002u,&s);
  assert(session);m.session=session;return session;
}
static void test_invalid_api() {
  auto c=callbacks(); Mock m;auto s=original_assault();
  assert(sin_grunt_behavior_abi_version_v1()==1u);
  assert(!sin_grunt_behavior_create_v1(&c,&m,{0u,7u},9u,7001,7002,&s));
  assert(!sin_grunt_behavior_create_v1(&c,&m,{11u,7u},0u,7001,7002,&s));
  assert(!sin_grunt_behavior_create_v1(&c,&m,{11u,7u},9u,7001,7001,&s));
  c.struct_size--;
  assert(!sin_grunt_behavior_create_v1(&c,&m,{11u,7u},9u,7001,7002,&s));
  assert(sin_grunt_behavior_destroy_v1(nullptr)==SIN_GRUNT_BEHAVIOR_INVALID);
}
static void test_assault_then_rappel_unlink() {
  Mock m;auto* s=create(m);
  int32_t current=-2;
  assert(sin_grunt_behavior_step_v1(s,10.f,&current)==SIN_GRUNT_BEHAVIOR_OK);
  assert(current==5&&m.notifications==1&&m.starts==1);
  assert(m.order.front()=="life5:1");
  for(const auto& event:m.order) assert(event!="life5:0");
  assert(m.reenter_status==SIN_GRUNT_BEHAVIOR_REENTRANT);
  assert(m.destroy_reenter_status==SIN_GRUNT_BEHAVIOR_REENTRANT);
  sin_grunt_snapshot_v1 snap{};
  assert(sin_grunt_behavior_snapshot_v1(s,&snap)==SIN_GRUNT_BEHAVIOR_OK);
  assert(snap.current_index==5);
  assert(sin_grunt_behavior_step_v1(s,10.f,&current)==SIN_GRUNT_BEHAVIOR_OK);
  assert(m.notifications==1&&m.starts==1);
  m.other_rappel=1;
  assert(sin_grunt_behavior_step_v1(s,10.f,&current)==SIN_GRUNT_BEHAVIOR_OK);
  assert(current==0&&m.notifications==2&&m.starts==3);
  assert(m.unlink_count==1&&m.link==0xffffffffu);
  assert(sin_grunt_behavior_snapshot_v1(s,&snap)==SIN_GRUNT_BEHAVIOR_OK);
  assert(snap.assault.byte_24==0&&snap.assault.byte_25==0);
  assert(m.order[m.order.size()-3]=="life0:0");
  assert(m.order[m.order.size()-2]=="life0:1");
  assert(m.order.back()=="notify5:0");
  m.session=nullptr;
  assert(sin_grunt_behavior_destroy_v1(s)==SIN_GRUNT_BEHAVIOR_OK);
}
static void test_other_owner_link_survives() {
  Mock m;m.link=0x33333008u;auto* s=create(m);
  int32_t current=-1;
  assert(sin_grunt_behavior_step_v1(s,10.f,&current)==0);
  m.other_rappel=1;
  assert(sin_grunt_behavior_step_v1(s,10.f,&current)==0);
  assert(current==0&&m.unlink_count==0&&m.link==0x33333008u);
  assert(sin_grunt_behavior_destroy_v1(s)==0);
}
static void test_no_eligible_after_assault() {
  Mock m;auto* s=create(m);int32_t current=-1;
  assert(sin_grunt_behavior_step_v1(s,10.f,&current)==0&&current==5);
  m.gate=0;
  assert(sin_grunt_behavior_step_v1(s,10.f,&current)==SIN_GRUNT_BEHAVIOR_NO_ELIGIBLE);
  assert(current==-1&&m.notifications==2&&m.unlink_count==1);
  assert(sin_grunt_behavior_destroy_v1(s)==0);
}
static void test_condition_mapping_and_exact_four_seconds() {
  Mock m;auto a=original_assault();a.time_28=0.f;auto* s=create(m,a);
  int32_t current=99;
  assert(sin_grunt_behavior_step_v1(s,4.f,&current)==SIN_GRUNT_BEHAVIOR_NO_ELIGIBLE);
  sin_grunt_snapshot_v1 snap{};
  assert(sin_grunt_behavior_snapshot_v1(s,&snap)==0);
  assert(snap.assault.byte_26==1);
  assert(m.has_condition_id==7001u);
  m.owner_target=1;
  assert(sin_grunt_behavior_step_v1(s,4.f,&current)==SIN_GRUNT_BEHAVIOR_OK);
  assert(current==5&&m.set_condition_id==7002u);
  assert(sin_grunt_behavior_destroy_v1(s)==0);
}
static void test_expired_and_host_callback_failure() {
  Mock m;auto* s=create(m);m.actor_live=0;
  int32_t current=123;
  assert(sin_grunt_behavior_step_v1(s,10.f,&current)==SIN_GRUNT_BEHAVIOR_ACTOR_EXPIRED);
  assert(current==-1);
  m.actor_live=1;m.fail_serial=1;
  assert(sin_grunt_behavior_step_v1(s,10.f,&current)==SIN_GRUNT_BEHAVIOR_HOST_FAILED);
  assert(current==-1);
  assert(sin_grunt_behavior_step_v1(s,std::numeric_limits<float>::quiet_NaN(),&current)==SIN_GRUNT_BEHAVIOR_INVALID);
  assert(sin_grunt_behavior_destroy_v1(s)==0);
}
int main(){
  test_invalid_api();
  test_assault_then_rappel_unlink();
  test_other_owner_link_survives();
  test_no_eligible_after_assault();
  test_condition_mapping_and_exact_four_seconds();
  test_expired_and_host_callback_failure();
}
