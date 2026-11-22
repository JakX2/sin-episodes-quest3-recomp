/* Phase626 synthetic native C11 host test: no engine-owned NPC object
 * and no retail binaries. Full Spawn -> original +0xFDC -> event 0x43. */
#include "sin_compat/elite_native_host_c_api_v1.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef struct Mock {
  sin_elite_native_registry_v1* reg;
  sin_elite_actor_v1 actor;
  uint64_t owner;
  int live,wrong_class,host_failure;
  int confirmations,callback_count,reentrant;
  int fail_stage,scalar_reads,lookup_result;
  float scalars[2];
  uint32_t flags,fdc,weapon_handle;
  uint8_t fd9,cb2;
  int32_t field9c,field98;
  int gate,resolve_result,forward_calls,publish_calls,lookup_calls;
  uint64_t gate_entity,entity_token;
  float forwarded_parameter;
  char calls[96];
  size_t used;
} Mock;

static void mark(Mock*m,char c) {
  assert(m->used+1<sizeof(m->calls));
  m->calls[m->used++]=c;
  m->calls[m->used]=0;
}
static int32_t host_confirm(void* u,const char* name,
                            sin_elite_actor_v1 a,uint64_t token) {
  Mock*m=(Mock*)u;
  ++m->confirmations;
  assert(strcmp(name,"npc_druglab_grunt_elite")==0);
  if(m->reentrant&&m->reg) {
    sin_elite_native_state_v1 out={0};
    out.version=SIN_ELITE_NATIVE_ABI_V1;
    out.struct_size=sizeof(out);
    assert(sin_elite_native_snapshot_v1(m->reg,a,&out)==
           SIN_ELITE_NATIVE_REENTRANT);
    assert(sin_elite_native_destroy_v1(m->reg)==SIN_ELITE_NATIVE_REENTRANT);
  }
  if(m->host_failure)return -1;
  return m->live && !m->wrong_class &&
    token==m->owner && a.index==m->actor.index &&
    a.generation==m->actor.generation ? 1:0;
}
static int32_t weapon(void* u,const char* name) {
  Mock*m=(Mock*)u;mark(m,'W');
  assert(strcmp(name,"weapon_chaingun")==0);return m->fail_stage!=1;
}
static int32_t virtual68(void* u){Mock*m=(Mock*)u;mark(m,'A');return m->fail_stage!=2;}
static int32_t virtual1c6c(void* u){Mock*m=(Mock*)u;mark(m,'B');return m->fail_stage!=3;}
static int32_t inherited(void* u){Mock*m=(Mock*)u;mark(m,'I');return m->fail_stage!=4;}
static int32_t fd9(void* u){
  Mock*m=(Mock*)u;mark(m,'D');if(m->fail_stage==5)return 0;
  m->fd9=0;return 1;
}
static int32_t cb2(void* u){
  Mock*m=(Mock*)u;mark(m,'E');if(m->fail_stage==6)return 0;
  m->cb2=0;return 1;
}
static int32_t scalar(void* u,float*out){
  Mock*m=(Mock*)u;mark(m,'R');if(m->fail_stage==7)return 0;
  assert(m->scalar_reads<2);
  *out=m->scalars[m->scalar_reads++];
  return 1;
}
static int32_t field9c(void* u,int32_t value){
  Mock*m=(Mock*)u;mark(m,'C');if(m->fail_stage==8)return 0;
  m->field9c=value;return 1;
}
static int32_t field98(void* u,int32_t value){
  Mock*m=(Mock*)u;mark(m,'F');if(m->fail_stage==9)return 0;
  m->field98=value;return 1;
}
static int32_t flags(void* u,uint32_t value,uint32_t marker) {
  Mock*m=(Mock*)u;mark(m,'G');
  assert(marker==SIN_ELITE_DIRTY_EDICT_BIT0_V1);
  if(m->fail_stage==10)return 0;
  m->flags=value;return 1;
}
static int32_t lookup(void* u,uint32_t*out) {
  Mock*m=(Mock*)u;mark(m,'L');
  ++m->lookup_calls;
  if(m->fail_stage==11)return -1;
  if(m->lookup_result==1)*out=m->weapon_handle;
  return m->lookup_result;
}
static int32_t writefdc(void* u,uint32_t handle) {
  Mock*m=(Mock*)u;mark(m,'H');if(m->fail_stage==12)return 0;
  m->fdc=handle;return 1;
}
static int32_t read_handle(void* u,uint32_t*handle) {
  Mock*m=(Mock*)u;mark(m,'q');
  if(m->fail_stage==15)return -1;
  *handle=m->fdc;return 1;
}
static int32_t resolve(void* u,uint32_t h,uint64_t*token) {
  Mock*m=(Mock*)u;mark(m,'r');++m->callback_count;
  assert(h==m->fdc);
  if(m->resolve_result==1)*token=m->entity_token;
  return m->resolve_result;
}
static int32_t gate(void* u,uint64_t token) {
  Mock*m=(Mock*)u;mark(m,'g');m->gate_entity=token;
  return m->gate;
}
static int32_t publish(void* u,uint8_t p) {
  Mock*m=(Mock*)u;mark(m,'p');++m->publish_calls;
  if(m->fail_stage==13)return 0;
  m->fd9=p;return 1;
}
static int32_t forward(void* u,float parameter) {
  Mock*m=(Mock*)u;mark(m,'f');++m->forward_calls;
  m->forwarded_parameter=parameter;
  if(m->fail_stage==14)return 0;
  return 1;
}
static const sin_elite_native_host_v1 host={
  SIN_ELITE_NATIVE_ABI_V1,sizeof(sin_elite_native_host_v1),host_confirm
};
static const sin_elite_native_spawn_callbacks_v1 spawns={
  SIN_ELITE_NATIVE_ABI_V1,sizeof(sin_elite_native_spawn_callbacks_v1),
  weapon,virtual68,virtual1c6c,inherited,fd9,cb2,scalar,field9c,field98,
  flags,lookup,writefdc
};
static const sin_elite_native_event43_callbacks_v1 events={
  SIN_ELITE_NATIVE_ABI_V1,sizeof(sin_elite_native_event43_callbacks_v1),
  read_handle,resolve,gate,publish,forward
};
static sin_elite_native_spawn_snapshot_v1 spawn_state(void) {
  sin_elite_native_spawn_snapshot_v1 s={
    SIN_ELITE_NATIVE_ABI_V1,sizeof(sin_elite_native_spawn_snapshot_v1),
    "wrong_weapon",0,0,0,0,1
  };
  return s;
}
static sin_elite_native_spawn_result_v1 spawn_result(void) {
  sin_elite_native_spawn_result_v1 s={
    SIN_ELITE_NATIVE_ABI_V1,sizeof(sin_elite_native_spawn_result_v1),0,0,0,0
  };
  return s;
}
static sin_elite_native_state_v1 native_state(void) {
  sin_elite_native_state_v1 s={
    SIN_ELITE_NATIVE_ABI_V1,sizeof(sin_elite_native_state_v1),0,0,0,0,{0,0,0}
  };
  return s;
}
static sin_elite_native_attachment_v1 attachment(Mock*m) {
  sin_elite_native_attachment_v1 a={
    SIN_ELITE_NATIVE_ABI_V1,sizeof(sin_elite_native_attachment_v1),
    "npc_druglab_grunt_elite",m->actor,m->owner
  };
  return a;
}
static void init(Mock*m) {
  memset(m,0,sizeof(*m));
  m->actor.index=51;m->actor.generation=7;m->owner=0x21345u;
  m->live=1;m->lookup_result=1;m->weapon_handle=0x12500007u;
  m->scalars[0]=8.75f;m->scalars[1]=-2.8f;
  m->fd9=0xffu;m->cb2=0xffu;
  m->entity_token=0x41410007u;m->resolve_result=1;
  m->gate=1;
}
static void test_full_spawn_and_event43(void) {
  Mock m;init(&m);
  m.reg=sin_elite_native_create_v1(&host,&m);
  assert(m.reg);
  sin_elite_native_attachment_v1 a=attachment(&m);
  sin_elite_native_spawn_snapshot_v1 s=spawn_state();
  sin_elite_native_spawn_result_v1 out=spawn_result();
  sin_elite_native_state_v1 state=native_state();
  assert(sin_elite_native_abi_version_v1()==1);
  assert(sin_elite_native_attach_v1(m.reg,&a)==SIN_ELITE_NATIVE_OK);
  assert(sin_elite_native_event43_v1(m.reg,m.actor,1.f,&events,&m)==
         SIN_ELITE_NATIVE_SPAWN_REQUIRED);
  assert(m.callback_count==0);
  m.reentrant=1;
  assert(sin_elite_native_spawn_v1(m.reg,m.actor,&s,&spawns,&m,&out)==
         SIN_ELITE_NATIVE_OK);
  m.reentrant=0;
  assert(strcmp(m.calls,"WABIDERCRFGGLH")==0);
  assert(out.failed_stage==0&&out.wrote_handle==1);
  assert(out.flags_after==(0x00100000u|0x00002000u));
  assert(out.handle_fdc==m.weapon_handle);
  assert(m.fd9==0&&m.cb2==0&&m.field9c==8&&m.field98==-2);
  assert(sin_elite_native_snapshot_v1(m.reg,m.actor,&state)==SIN_ELITE_NATIVE_OK);
  assert(state.spawn_attempted==1&&state.spawn_completed==1);
  assert(state.serial_handle_fdc==m.weapon_handle&&state.predicate_fd9==0);
  assert(sin_elite_native_spawn_v1(m.reg,m.actor,&s,&spawns,&m,&out)==
         SIN_ELITE_NATIVE_ALREADY_SPAWNED);
  m.used=0;m.calls[0]=0;
  assert(sin_elite_native_event43_v1(m.reg,m.actor,1.5f,&events,&m)==
         SIN_ELITE_NATIVE_EVENT_FORWARDED);
  assert(strcmp(m.calls,"qrgpf")==0);
  assert(m.gate_entity==m.entity_token&&m.fd9==1);
  assert(m.forward_calls==1&&m.forwarded_parameter==1.5f);
  assert(sin_elite_native_snapshot_v1(m.reg,m.actor,&state)==SIN_ELITE_NATIVE_OK);
  assert(state.predicate_fd9==1);
  m.gate=0;
  m.used=0;m.calls[0]=0;
  assert(sin_elite_native_event43_v1(m.reg,m.actor,2.f,&events,&m)==
         SIN_ELITE_NATIVE_EVENT_BLOCKED);
  assert(strcmp(m.calls,"qrgp")==0&&m.fd9==0&&m.forward_calls==1);
  assert(sin_elite_native_detach_v1(m.reg,m.actor)==SIN_ELITE_NATIVE_OK);
  assert(sin_elite_native_destroy_v1(m.reg)==SIN_ELITE_NATIVE_OK);
}
static void test_identity_revalidation_reuse_and_death(void) {
  Mock m;init(&m);
  m.reg=sin_elite_native_create_v1(&host,&m);
  assert(m.reg);
  sin_elite_native_attachment_v1 a=attachment(&m);
  sin_elite_native_spawn_snapshot_v1 s=spawn_state();
  sin_elite_native_spawn_result_v1 out=spawn_result();
  sin_elite_native_state_v1 state=native_state();
  assert(sin_elite_native_attach_v1(m.reg,&a)==SIN_ELITE_NATIVE_OK);
  m.live=0;
  assert(sin_elite_native_spawn_v1(m.reg,m.actor,&s,&spawns,&m,&out)==
         SIN_ELITE_NATIVE_HOST_REJECTED);
  assert(m.used==0&&m.scalar_reads==0);
  assert(sin_elite_native_snapshot_v1(m.reg,m.actor,&state)==
         SIN_ELITE_NATIVE_HOST_REJECTED);
  m.live=1;m.wrong_class=1;
  assert(sin_elite_native_spawn_v1(m.reg,m.actor,&s,&spawns,&m,&out)==
         SIN_ELITE_NATIVE_HOST_REJECTED);
  m.wrong_class=0;m.host_failure=1;
  assert(sin_elite_native_spawn_v1(m.reg,m.actor,&s,&spawns,&m,&out)==
         SIN_ELITE_NATIVE_HOST_REJECTED);
  m.host_failure=0;
  assert(sin_elite_native_spawn_v1(m.reg,m.actor,&s,&spawns,&m,&out)==
         SIN_ELITE_NATIVE_OK);
  const sin_elite_actor_v1 stale={51u,6u};
  assert(sin_elite_native_snapshot_v1(m.reg,stale,&state)==
         SIN_ELITE_NATIVE_STALE_GENERATION);
  assert(sin_elite_native_event43_v1(m.reg,stale,0.f,&events,&m)==
         SIN_ELITE_NATIVE_STALE_GENERATION);
  assert(sin_elite_native_detach_v1(m.reg,stale)==
         SIN_ELITE_NATIVE_STALE_GENERATION);
  m.live=0;
  // Dead owners may still release only their exact original generation.
  assert(sin_elite_native_detach_v1(m.reg,m.actor)==SIN_ELITE_NATIVE_OK);
  m.actor.generation++;
  a=attachment(&m);
  m.live=1;
  assert(sin_elite_native_attach_v1(m.reg,&a)==SIN_ELITE_NATIVE_OK);
  assert(sin_elite_native_detach_v1(m.reg,stale)==
         SIN_ELITE_NATIVE_STALE_GENERATION);
  assert(sin_elite_native_detach_v1(m.reg,m.actor)==SIN_ELITE_NATIVE_OK);
  assert(sin_elite_native_destroy_v1(m.reg)==SIN_ELITE_NATIVE_OK);
}
static void test_rejected_spawn_one_shot_and_failure_order(void) {
  for(int fail=1;fail<=12;++fail) {
    Mock m;init(&m);m.fail_stage=fail;
    m.reg=sin_elite_native_create_v1(&host,&m);assert(m.reg);
    sin_elite_native_attachment_v1 a=attachment(&m);
    sin_elite_native_spawn_snapshot_v1 s=spawn_state();
    sin_elite_native_spawn_result_v1 out=spawn_result();
    assert(sin_elite_native_attach_v1(m.reg,&a)==SIN_ELITE_NATIVE_OK);
    assert(sin_elite_native_spawn_v1(m.reg,m.actor,&s,&spawns,&m,&out)==
           SIN_ELITE_NATIVE_HOST_REJECTED);
    assert(out.failed_stage>0);
    m.fail_stage=0;
    assert(sin_elite_native_spawn_v1(m.reg,m.actor,&s,&spawns,&m,&out)==
           SIN_ELITE_NATIVE_ALREADY_SPAWNED);
    assert(sin_elite_native_event43_v1(m.reg,m.actor,0.f,&events,&m)==
           SIN_ELITE_NATIVE_SPAWN_REQUIRED);
    assert(sin_elite_native_detach_v1(m.reg,m.actor)==SIN_ELITE_NATIVE_OK);
    assert(sin_elite_native_destroy_v1(m.reg)==SIN_ELITE_NATIVE_OK);
  }
}
static void test_no_valid_chaingun_and_partial_event(void) {
  Mock m;init(&m);m.lookup_result=0;
  m.reg=sin_elite_native_create_v1(&host,&m);assert(m.reg);
  sin_elite_native_attachment_v1 a=attachment(&m);
  sin_elite_native_spawn_snapshot_v1 s=spawn_state();
  sin_elite_native_spawn_result_v1 out=spawn_result();
  sin_elite_native_state_v1 state=native_state();
  assert(sin_elite_native_attach_v1(m.reg,&a)==SIN_ELITE_NATIVE_OK);
  assert(sin_elite_native_spawn_v1(m.reg,m.actor,&s,&spawns,&m,&out)==
         SIN_ELITE_NATIVE_OK);
  assert(out.handle_fdc==SIN_ELITE_HANDLE_INVALID_V1);
  m.used=0;m.calls[0]=0;
  assert(sin_elite_native_event43_v1(m.reg,m.actor,1.f,&events,&m)==
         SIN_ELITE_NATIVE_EVENT_FORWARDED);
  assert(strcmp(m.calls,"qgpf")==0 && m.gate_entity==0);
  /* The original NPC field +0xFDC may change after Spawn, e.g. when
   * equipment ownership changes. Re-read the authoritative live field. */
  m.fdc=0x1250000bu;
  m.used=0;m.calls[0]=0;
  assert(sin_elite_native_event43_v1(m.reg,m.actor,1.f,&events,&m)==
         SIN_ELITE_NATIVE_EVENT_FORWARDED);
  assert(strcmp(m.calls,"qrgpf")==0);
  assert(sin_elite_native_snapshot_v1(m.reg,m.actor,&state)==SIN_ELITE_NATIVE_OK);
  assert(state.serial_handle_fdc==0x1250000bu);
  m.fail_stage=15;
  m.used=0;m.calls[0]=0;
  assert(sin_elite_native_event43_v1(m.reg,m.actor,1.f,&events,&m)==
         SIN_ELITE_NATIVE_HOST_REJECTED);
  assert(strcmp(m.calls,"q")==0);
  m.fail_stage=13;
  assert(sin_elite_native_event43_v1(m.reg,m.actor,1.f,&events,&m)==
         SIN_ELITE_NATIVE_HOST_REJECTED);
  assert(sin_elite_native_snapshot_v1(m.reg,m.actor,&state)==SIN_ELITE_NATIVE_OK);
  assert(state.predicate_fd9==1);
  m.fail_stage=14;
  assert(sin_elite_native_event43_v1(m.reg,m.actor,1.f,&events,&m)==
         SIN_ELITE_NATIVE_HOST_REJECTED);
  assert(sin_elite_native_snapshot_v1(m.reg,m.actor,&state)==SIN_ELITE_NATIVE_OK);
  assert(state.predicate_fd9==1); // publish committed before forward failure
  assert(sin_elite_native_detach_v1(m.reg,m.actor)==SIN_ELITE_NATIVE_OK);
  assert(sin_elite_native_destroy_v1(m.reg)==SIN_ELITE_NATIVE_OK);
}
static void test_invalid_inputs_and_x87(void) {
  Mock m;init(&m);
  assert(sin_elite_native_create_v1(0,&m)==0);
  sin_elite_native_host_v1 bad=host;bad.version=2;
  assert(sin_elite_native_create_v1(&bad,&m)==0);
  bad=host;bad.struct_size--;
  assert(sin_elite_native_create_v1(&bad,&m)==0);
  m.reg=sin_elite_native_create_v1(&host,&m);assert(m.reg);
  sin_elite_native_attachment_v1 a=attachment(&m);
  a.original_classname="npc_druglab_grunt_pistol";
  assert(sin_elite_native_attach_v1(m.reg,&a)==SIN_ELITE_NATIVE_UNSUPPORTED_CLASS);
  a.original_classname="npc_druglab_grunt_elite";
  a.actor.generation=0;
  assert(sin_elite_native_attach_v1(m.reg,&a)==SIN_ELITE_NATIVE_INVALID_IDENTITY);
  a.actor=m.actor;
  assert(sin_elite_native_attach_v1(m.reg,&a)==SIN_ELITE_NATIVE_OK);
  assert(sin_elite_native_attach_v1(m.reg,&a)==SIN_ELITE_NATIVE_DUPLICATE_INDEX);
  sin_elite_native_spawn_snapshot_v1 s=spawn_state();
  sin_elite_native_spawn_result_v1 out=spawn_result();
  s.version=22;
  assert(sin_elite_native_spawn_v1(m.reg,m.actor,&s,&spawns,&m,&out)==
         SIN_ELITE_NATIVE_INVALID_ARGUMENT);
  s=spawn_state();
  sin_elite_native_spawn_callbacks_v1 missing=spawns;
  missing.write_field_fdc=0;
  assert(sin_elite_native_spawn_v1(m.reg,m.actor,&s,&missing,&m,&out)==
         SIN_ELITE_NATIVE_INVALID_ARGUMENT);
  m.scalars[0]=NAN;
  assert(sin_elite_native_spawn_v1(m.reg,m.actor,&s,&spawns,&m,&out)==
         SIN_ELITE_NATIVE_UNSUPPORTED_X87);
  assert(sin_elite_native_detach_v1(m.reg,m.actor)==SIN_ELITE_NATIVE_OK);
  assert(sin_elite_native_destroy_v1(m.reg)==SIN_ELITE_NATIVE_OK);
}
int main(void) {
  test_full_spawn_and_event43();
  test_identity_revalidation_reuse_and_death();
  test_rejected_spawn_one_shot_and_failure_order();
  test_no_valid_chaingun_and_partial_event();
  test_invalid_inputs_and_x87();
  puts("PASS Phase626 native Elite C ABI: authenticated one-shot Spawn to original event0x43 state, identity revocation and safe detach");
  return 0;
}
