// Phase630: actual independent Phase626 v1 C registry + Phase629 source gate
// through a NEW C ABI DLL. All NPCs and engine side effects are synthetic;
// this does not claim actual original game/Quest execution.
#include "sin_compat/elite_native_source_gate_phase630.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
#include <stdexcept>
namespace {
struct Mock {
  sin_elite_native_registry_v1* registry=nullptr;
  sin_elite_actor_v1 actor{12u,5u};
  std::uint64_t owner=0xADDC012u;
  std::uint64_t target=0xABC005u;
  bool alive=true;
  bool resolved=true;
  int fail_at=0;
  bool reenter=false;
  int reenter_result=0;
  bool source_reenter=false;
  int source_reenter_code=0;
  const sin_elite_source_gate_callbacks_v1* source_callbacks=nullptr;
  std::uint32_t weapon_handle=0x13500005u;
  std::uint32_t fdc=SIN_ELITE_HANDLE_INVALID_V1;
  std::uint32_t phase=0u;
  float t590=0.f,t58c=0.f;
  std::vector<float> times;
  unsigned next_time=0;
  std::vector<std::string> calls;
  std::uint8_t fd9=0xffu,cb2=0xffu;
  int forward_count=0;
  void record(const char* s){calls.emplace_back(s);}
  void set_times(std::initializer_list<float> values){
    times.assign(values.begin(),values.end());
    next_time=0;
    calls.clear();
  }
};
int32_t confirm(void* u,const char* cls,sin_elite_actor_v1 actor,
                std::uint64_t owner) {
  auto& m=*static_cast<Mock*>(u);
  assert(std::string(cls)=="npc_druglab_grunt_elite");
  return m.alive && actor.index==m.actor.index &&
    actor.generation==m.actor.generation && owner==m.owner;
}
int32_t spawn_weapon(void*,const char* symbol) {
  assert(std::string(symbol)=="weapon_chaingun");return 1;
}
int32_t spawn_yes(void*){return 1;}
int32_t spawn_zero_fd9(void* u){static_cast<Mock*>(u)->fd9=0;return 1;}
int32_t spawn_zero_cb2(void* u){static_cast<Mock*>(u)->cb2=0;return 1;}
int32_t spawn_float(void*,float* v){*v=2.f;return 1;}
int32_t spawn_write_int(void*,std::int32_t){return 1;}
int32_t spawn_flags(void*,std::uint32_t,std::uint32_t){return 1;}
int32_t spawn_handle(void* u,std::uint32_t* out){
  *out=static_cast<Mock*>(u)->weapon_handle;return 1;
}
int32_t spawn_write_handle(void* u,std::uint32_t h) {
  static_cast<Mock*>(u)->fdc=h;return 1;
}
const sin_elite_native_host_v1 kHost{
  SIN_ELITE_NATIVE_ABI_V1,sizeof(sin_elite_native_host_v1),confirm
};
const sin_elite_native_spawn_callbacks_v1 kSpawn{
  SIN_ELITE_NATIVE_ABI_V1,sizeof(sin_elite_native_spawn_callbacks_v1),
  spawn_weapon,spawn_yes,spawn_yes,spawn_yes,
  spawn_zero_fd9,spawn_zero_cb2,spawn_float,
  spawn_write_int,spawn_write_int,spawn_flags,
  spawn_handle,spawn_write_handle
};
int32_t read_handle(void* u,std::uint32_t* out) {
  auto& m=*static_cast<Mock*>(u);m.record("fdc");
  if(m.fail_at==1)return -1;
  *out=m.fdc;return 1;
}
int32_t resolve(void* u,std::uint32_t handle,std::uint64_t* out) {
  auto& m=*static_cast<Mock*>(u);m.record("resolve");
  assert(handle==m.fdc);
  if(m.fail_at==2)return -1;
  if(!m.resolved)return 0;
  *out=m.target;return 1;
}
int32_t read_state(void* u,std::uint64_t t,std::uint32_t* phase,
                   float* a,float* b) {
  auto& m=*static_cast<Mock*>(u);m.record("state");
  assert(t==m.target);
  if(m.reenter) {
    sin_elite_native_state_v1 snap{};
    snap.version=1u;snap.struct_size=sizeof(snap);
    m.reenter_result=sin_elite_native_snapshot_v1(
       m.registry,m.actor,&snap);
  }
  if(m.source_reenter) {
    m.source_reenter=false;
    sin_elite_source_gate_result_v1 nested{};
    nested.version=SIN_ELITE_SOURCE_GATE_ABI_V1;
    nested.struct_size=sizeof(nested);
    assert(m.source_callbacks!=nullptr);
    m.source_reenter_code=sin_elite_source_gate_update_v1(
      m.registry,m.actor,m.source_callbacks,&m,&nested);
    m.source_reenter=true;
  }
  if(m.fail_at==3)return -1;
  *phase=m.phase;*a=m.t590;*b=m.t58c;return 1;
}
int32_t write_phase(void* u,std::uint64_t t,std::uint32_t phase) {
  auto& m=*static_cast<Mock*>(u);m.record("phase");
  assert(t==m.target);
  if(m.fail_at==4)return 0;
  m.phase=phase;return 1;
}
int32_t write_t590(void* u,std::uint64_t t,float v) {
  auto& m=*static_cast<Mock*>(u);m.record("time590");
  assert(t==m.target);
  if(m.fail_at==5)return 0;
  m.t590=v;return 1;
}
int32_t write_t58c(void* u,std::uint64_t t,float v) {
  auto& m=*static_cast<Mock*>(u);m.record("time58c");
  assert(t==m.target);
  if(m.fail_at==6)return 0;
  m.t58c=v;return 1;
}
int32_t virtual3c4(void* u,std::uint64_t t,std::uint32_t cmd,
                   std::uint32_t zero) {
  auto& m=*static_cast<Mock*>(u);assert(t==m.target&&zero==0u);
  m.record(cmd==13u?"v3c4_13":"v3c4_14");
  if(m.fail_at==7)return 0;
  return 1;
}
int32_t virtual3c8(void* u,std::uint64_t t,std::uint32_t cmd) {
  auto& m=*static_cast<Mock*>(u);assert(t==m.target&&cmd==13u);
  m.record("v3c8_13");return m.fail_at!=8;
}
int32_t global_time(void* u,float* v) {
  auto& m=*static_cast<Mock*>(u);m.record("clock");
  if(m.fail_at==9)return 0;
  assert(m.next_time<m.times.size());
  *v=m.times[m.next_time++];return 1;
}
int32_t publish(void* u,std::uint8_t p) {
  auto& m=*static_cast<Mock*>(u);m.record("fd9");
  if(m.fail_at==10)return 0;
  m.fd9=p;return 1;
}
int32_t forward(void* u,float v) {
  auto& m=*static_cast<Mock*>(u);m.record("inherited");
  assert(v==0.75f);++m.forward_count;
  return m.fail_at!=11;
}
const sin_elite_source_gate_callbacks_v1 kCallbacks{
  SIN_ELITE_SOURCE_GATE_ABI_V1,
  sizeof(sin_elite_source_gate_callbacks_v1),
  read_handle,resolve,read_state,
  write_phase,write_t590,write_t58c,
  virtual3c4,virtual3c8,global_time,
  publish,forward
};
sin_elite_source_gate_result_v1 result() {
  sin_elite_source_gate_result_v1 r{};
  r.version=1u;r.struct_size=sizeof(r);return r;
}
void attach_only(Mock& m) {
  m.registry=sin_elite_native_create_v1(&kHost,&m);
  assert(m.registry);
  const sin_elite_native_attachment_v1 a{
    SIN_ELITE_NATIVE_ABI_V1,sizeof(a),
    "npc_druglab_grunt_elite",m.actor,m.owner
  };
  assert(sin_elite_native_attach_v1(m.registry,&a)==SIN_ELITE_NATIVE_OK);
}
void spawn(Mock& m) {
  const sin_elite_native_spawn_snapshot_v1 snapshot{
    SIN_ELITE_NATIVE_ABI_V1,sizeof(snapshot),
    "weapon_pistol",0,0,0,0,1
  };
  sin_elite_native_spawn_result_v1 out{};
  out.version=SIN_ELITE_NATIVE_ABI_V1;out.struct_size=sizeof(out);
  assert(sin_elite_native_spawn_v1(
      m.registry,m.actor,&snapshot,&kSpawn,&m,&out)==SIN_ELITE_NATIVE_OK);
  assert(out.handle_fdc==m.weapon_handle);
  assert(m.fdc==m.weapon_handle && m.fd9==0u && m.cb2==0u);
}
void close(Mock& m) {
  assert(sin_elite_native_detach_v1(m.registry,m.actor)==SIN_ELITE_NATIVE_OK);
  assert(sin_elite_native_destroy_v1(m.registry)==SIN_ELITE_NATIVE_OK);
  m.registry=nullptr;
}
void expect(const Mock& m,std::initializer_list<std::string> expected) {
  assert(m.calls==std::vector<std::string>(expected));
}
void test_source_order_complete_cycle() {
  Mock m;attach_only(m);spawn(m);auto out=result();
  assert(sin_elite_source_gate_abi_version_v1()==1u);
  // Phase626 enforces original Spawn before game events or ticks.
  m.set_times({0.5f,0.75f});
  assert(sin_elite_source_gate_event43_v1(m.registry,m.actor,0.75f,
     &kCallbacks,&m,&out)==SIN_ELITE_NATIVE_EVENT_BLOCKED);
  assert(out.source_status==SIN_ELITE_SOURCE_GATE_BLOCKED_V1 &&
         out.phase_before==0u && out.phase_after==1u &&
         out.target_valid==1);
  expect(m,{"fdc","resolve","state","v3c4_13","phase",
            "clock","time590","clock","time58c","fd9"});
  assert(m.phase==1u&&m.t590==0.5f&&m.t58c==0.75f&&m.fd9==0u);

  m.set_times({1.49f});
  assert(sin_elite_source_gate_update_v1(m.registry,m.actor,&kCallbacks,
     &m,&out)==SIN_ELITE_SOURCE_UPDATE_UNCHANGED_V1);
  assert(m.phase==1u);
  expect(m,{"fdc","resolve","state","clock"});

  m.set_times({1.5f,1.75f,2.0f});
  assert(sin_elite_source_gate_update_v1(m.registry,m.actor,&kCallbacks,
     &m,&out)==SIN_ELITE_SOURCE_UPDATE_ADVANCED_V1);
  assert(out.phase_before==1u&&out.phase_after==2u);
  expect(m,{"fdc","resolve","state","clock","phase",
            "clock","time590","clock","time58c"});
  assert(m.phase==2u&&m.t590==1.75f&&m.t58c==2.0f);

  m.calls.clear();
  assert(sin_elite_source_gate_event43_v1(m.registry,m.actor,0.75f,
      &kCallbacks,&m,&out)==SIN_ELITE_NATIVE_EVENT_FORWARDED);
  expect(m,{"fdc","resolve","state","fd9","inherited"});
  assert(out.source_status==SIN_ELITE_SOURCE_GATE_ALLOWED_V1);
  assert(m.fd9==1&&m.forward_count==1);

  m.set_times({2.999f});
  assert(sin_elite_source_gate_update_v1(m.registry,m.actor,&kCallbacks,
    &m,&out)==SIN_ELITE_SOURCE_UPDATE_UNCHANGED_V1);
  m.set_times({3.f});
  assert(sin_elite_source_gate_update_v1(m.registry,m.actor,&kCallbacks,
    &m,&out)==SIN_ELITE_SOURCE_UPDATE_ADVANCED_V1);
  assert(m.phase==3u);
  expect(m,{"fdc","resolve","state","clock","phase"});
  m.calls.clear();
  assert(sin_elite_source_gate_event43_v1(m.registry,m.actor,0.75f,
    &kCallbacks,&m,&out)==SIN_ELITE_NATIVE_EVENT_BLOCKED);
  assert(m.fd9==0u);
  m.calls.clear();
  assert(sin_elite_source_gate_update_v1(m.registry,m.actor,&kCallbacks,
    &m,&out)==SIN_ELITE_SOURCE_UPDATE_ADVANCED_V1);
  expect(m,{"fdc","resolve","state","v3c8_13","v3c4_14","phase"});
  assert(m.phase==0u);
  close(m);
}
void test_spawn_gate_stale_identity_and_null_handle() {
  Mock m;attach_only(m);auto out=result();
  m.set_times({1.f,1.f});
  assert(sin_elite_source_gate_event43_v1(m.registry,m.actor,0.75f,
    &kCallbacks,&m,&out)==SIN_ELITE_NATIVE_SPAWN_REQUIRED);
  assert(sin_elite_source_gate_update_v1(m.registry,m.actor,&kCallbacks,
    &m,&out)==SIN_ELITE_NATIVE_SPAWN_REQUIRED);
  assert(m.calls.empty());
  spawn(m);
  auto wrong=m.actor;wrong.generation--;
  assert(sin_elite_source_gate_event43_v1(m.registry,wrong,0.75f,
    &kCallbacks,&m,&out)==SIN_ELITE_NATIVE_STALE_GENERATION);
  assert(sin_elite_source_gate_update_v1(m.registry,wrong,&kCallbacks,
    &m,&out)==SIN_ELITE_NATIVE_STALE_GENERATION);
  assert(m.calls.empty());
  m.alive=false;
  assert(sin_elite_source_gate_event43_v1(m.registry,m.actor,0.75f,
    &kCallbacks,&m,&out)==SIN_ELITE_NATIVE_HOST_REJECTED);
  assert(sin_elite_source_gate_update_v1(m.registry,m.actor,&kCallbacks,
    &m,&out)==SIN_ELITE_NATIVE_HOST_REJECTED);
  m.alive=true;
  m.fdc=SIN_ELITE_HANDLE_INVALID_V1;
  m.calls.clear();
  assert(sin_elite_source_gate_event43_v1(m.registry,m.actor,0.75f,
    &kCallbacks,&m,&out)==SIN_ELITE_NATIVE_HOST_REJECTED);
  assert(out.source_status==SIN_ELITE_SOURCE_TARGET_MISSING_V1);
  expect(m,{"fdc"});
  m.calls.clear();
  assert(sin_elite_source_gate_update_v1(m.registry,m.actor,&kCallbacks,
    &m,&out)==SIN_ELITE_NATIVE_HOST_REJECTED);
  assert(out.source_status==SIN_ELITE_SOURCE_TARGET_MISSING_V1);
  expect(m,{"fdc"});
  m.fdc=m.weapon_handle;m.resolved=false;
  m.calls.clear();
  assert(sin_elite_source_gate_event43_v1(m.registry,m.actor,0.75f,
    &kCallbacks,&m,&out)==SIN_ELITE_NATIVE_HOST_REJECTED);
  assert(out.source_status==SIN_ELITE_SOURCE_TARGET_MISSING_V1);
  expect(m,{"fdc","resolve"});
  close(m);
}
void test_failures_and_native_partial_writes() {
  Mock m;attach_only(m);spawn(m);auto out=result();
  for(int fail=1;fail<=11;++fail) {
    m.phase=0u;m.t590=0.f;m.t58c=0.f;
    m.fdc=m.weapon_handle;m.resolved=true;
    m.fail_at=fail;
    m.set_times({0.5f,0.75f});
    const auto result_code=sin_elite_source_gate_event43_v1(
      m.registry,m.actor,0.75f,&kCallbacks,&m,&out);
    if(fail==8) { // v3c8 is update-only; event43 succeeds
      assert(result_code==SIN_ELITE_NATIVE_EVENT_BLOCKED);
    } else if(fail==11) { // inherited path is gated out for state zero
      assert(result_code==SIN_ELITE_NATIVE_EVENT_BLOCKED);
    } else {
      assert(result_code==SIN_ELITE_NATIVE_HOST_REJECTED);
    }
    if(fail==5 || fail==6 || fail==9) {
      // Phase may have been committed despite a later failing callback.
      assert(m.phase==1u);
    }
  }
  m.fail_at=0;m.reenter=true;
  m.phase=2u;m.calls.clear();
  assert(sin_elite_source_gate_event43_v1(m.registry,m.actor,0.75f,
    &kCallbacks,&m,&out)==SIN_ELITE_NATIVE_EVENT_FORWARDED);
  assert(m.reenter_result==SIN_ELITE_NATIVE_REENTRANT);
  m.reenter=false;
  m.source_callbacks=&kCallbacks;
  m.source_reenter=true;
  m.phase=2u;m.calls.clear();
  assert(sin_elite_source_gate_event43_v1(m.registry,m.actor,0.75f,
    &kCallbacks,&m,&out)==SIN_ELITE_NATIVE_EVENT_FORWARDED);
  assert(m.source_reenter_code==SIN_ELITE_NATIVE_REENTRANT);
  m.source_reenter=false;
  m.fail_at=10;m.phase=2u;m.calls.clear();
  assert(sin_elite_source_gate_event43_v1(m.registry,m.actor,0.75f,
    &kCallbacks,&m,&out)==SIN_ELITE_NATIVE_HOST_REJECTED);
  assert(out.source_status==SIN_ELITE_SOURCE_GATE_ALLOWED_V1);
  expect(m,{"fdc","resolve","state","fd9"});
  m.fail_at=11;m.calls.clear();
  assert(sin_elite_source_gate_event43_v1(m.registry,m.actor,0.75f,
    &kCallbacks,&m,&out)==SIN_ELITE_NATIVE_HOST_REJECTED);
  expect(m,{"fdc","resolve","state","fd9","inherited"});
  close(m);
}
void test_invalid_abi() {
  Mock m;attach_only(m);spawn(m);auto out=result();
  auto wrong=kCallbacks;wrong.version=99u;
  assert(sin_elite_source_gate_update_v1(m.registry,m.actor,&wrong,
    &m,&out)==SIN_ELITE_NATIVE_INVALID_ARGUMENT);
  wrong=kCallbacks;wrong.write_time_58c=nullptr;
  assert(sin_elite_source_gate_event43_v1(m.registry,m.actor,0.75f,
    &wrong,&m,&out)==SIN_ELITE_NATIVE_INVALID_ARGUMENT);
  out.struct_size--;
  assert(sin_elite_source_gate_update_v1(m.registry,m.actor,&kCallbacks,
    &m,&out)==SIN_ELITE_NATIVE_INVALID_ARGUMENT);
  close(m);
}
} // namespace
int main() {
  test_source_order_complete_cycle();
  test_spawn_gate_stale_identity_and_null_handle();
  test_failures_and_native_partial_writes();
  test_invalid_abi();
  std::puts("PASS Phase630: exact native Elite Spawn -> weapon-linked gate -> event43 -> tick C callbacks");
  return 0;
}
