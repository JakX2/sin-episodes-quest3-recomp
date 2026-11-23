#include "sin_compat/original_elite_weapon_tick_phase631.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <stdexcept>
namespace tick=sin_compat::original_elite_weapon_tick_phase631;
namespace {
class Host final:public tick::IHost {
public:
  std::vector<std::string> calls;
  std::uint32_t handles[2]{0x41234003u,0x51234003u};
  std::uint64_t first_target=0x123456u,second_target=0x778899u;
  int serial_results[2]{1,1};
  unsigned handle_reads=0,serial_reads=0,update_calls=0,inherited=0;
  std::string failure;
  tick::Reference* reenter=nullptr;
  tick::Code nested=tick::Code::InheritedOnly;
  bool throw_host=false;
  bool read_original_elite_fdc(std::uint32_t& value)override {
    calls.emplace_back("read-fdc");
    if(failure=="read-fdc")return false;
    assert(handle_reads<2);
    value=handles[handle_reads++];
    return true;
  }
  bool open_original_serial_table()override {
    calls.emplace_back("open-table");
    return failure!="open-table";
  }
  int resolve_original_serial(std::uint32_t handle,
                              std::uint64_t& out)override {
    calls.emplace_back("resolve");
    assert(serial_reads<2);
    assert(handle==handles[serial_reads]);
    const unsigned step=serial_reads++;
    if(failure=="resolve"&&step==0)return -1;
    if(failure=="resolve-second"&&step==1)return -1;
    out=(step==0)?first_target:second_target;
    return serial_results[step];
  }
  bool invoke_original_weapon_update(std::uint64_t target)override {
    calls.emplace_back("update");
    assert(target==second_target); // never accidentally use first target
    ++update_calls;
    if(throw_host)throw std::runtime_error("synthetic tick host");
    if(reenter)nested=reenter->run().code;
    return failure!="update";
  }
  bool inherited_grunt_tick_2235DF10()override {
    calls.emplace_back("inherited");
    ++inherited;
    return failure!="inherited";
  }
};
void assert_calls(const Host& h,std::initializer_list<std::string> names) {
  assert(h.calls==std::vector<std::string>(names));
}
void test_full_double_resolve() {
  Host h;tick::Reference m(h);
  const tick::Result r=m.run();
  assert(r.code==tick::Code::WeaponUpdatedThenInherited);
  assert(r.first_handle==h.handles[0]&&r.second_handle==h.handles[1]);
  assert(r.first_target==h.first_target&&r.second_target==h.second_target);
  assert(r.update_attempted&&r.inherited_called);
  assert(h.handle_reads==2&&h.serial_reads==2);
  assert(h.update_calls==1&&h.inherited==1);
  assert_calls(h,{"read-fdc","open-table","resolve",
                  "read-fdc","resolve","update","inherited"});
}
void test_no_updater_with_early_absences() {
  for(int scenario=0;scenario<4;++scenario) {
    Host h;tick::Reference m(h);
    if(scenario==0)h.handles[0]=tick::kHandleInvalid;
    if(scenario==1)h.serial_results[0]=0;
    if(scenario==2)h.first_target=0;
    if(scenario==3)h.serial_results[0]=0;
    const auto r=m.run();
    assert(r.code==tick::Code::InheritedOnly);
    assert(!r.update_attempted && r.inherited_called);
    assert(h.update_calls==0&&h.inherited==1);
    if(scenario==0) {
      assert_calls(h,{"read-fdc","inherited"});
      assert(h.serial_reads==0&&h.handle_reads==1);
    } else {
      assert_calls(h,{"read-fdc","open-table","resolve","inherited"});
      assert(h.serial_reads==1&&h.handle_reads==1);
    }
  }
}
void test_double_lookup_null_hazard() {
  for(int scenario=0;scenario<3;++scenario) {
    Host h;tick::Reference m(h);
    if(scenario==0)h.handles[1]=tick::kHandleInvalid;
    if(scenario==1)h.serial_results[1]=0;
    if(scenario==2)h.second_target=0;
    const auto r=m.run();
    assert(r.code==tick::Code::OriginalNullTargetHazard);
    assert(r.failed_stage==tick::Stage::OriginalTargetUpdate);
    assert(!r.update_attempted&&!r.inherited_called);
    assert(h.update_calls==0&&h.inherited==0);
    if(scenario==0) {
      assert_calls(h,{"read-fdc","open-table","resolve","read-fdc"});
    } else {
      assert_calls(h,{"read-fdc","open-table","resolve",
                      "read-fdc","resolve"});
    }
  }
}
void test_callback_failures_and_reentrance() {
  for(const char* fail:{"read-fdc","open-table","resolve",
                        "resolve-second","update","inherited"}) {
    Host h;tick::Reference m(h);h.failure=fail;
    const auto r=m.run();
    assert(r.code==tick::Code::HostRejected);
    assert(!r.inherited_called);
    if(h.failure=="read-fdc")
      assert(r.failed_stage==tick::Stage::FirstHandleRead);
    if(h.failure=="update")
      assert(r.failed_stage==tick::Stage::OriginalTargetUpdate);
    if(h.failure=="inherited")
      assert(r.failed_stage==tick::Stage::InheritedGruntTick);
  }
  Host h;tick::Reference m(h);
  h.reenter=&m;
  assert(m.run().code==tick::Code::WeaponUpdatedThenInherited);
  assert(h.nested==tick::Code::Reentrant);
  Host throws;tick::Reference thrown(throws);
  throws.throw_host=true;
  const auto result=thrown.run();
  assert(result.code==tick::Code::HostRejected);
  assert(result.failed_stage==tick::Stage::OriginalTargetUpdate);
}
void test_constants() {
  assert(tick::kOriginalEliteTickVA==0x223555f0u);
  assert(tick::kOriginalEliteVtableVA+tick::kOriginalTickVtableOffset==
         0x2248e328u);
  assert(tick::kOriginalWeaponUpdateVA==0x22315390u);
  assert(tick::kOriginalInheritedTickVA==0x2235df10u);
}
}
int main() {
  test_constants();
  test_full_double_resolve();
  test_no_updater_with_early_absences();
  test_double_lookup_null_hazard();
  test_callback_failures_and_reentrance();
  std::puts("PASS Phase631 original Elite vtable+0x424 double serial-resolution updater, parent tailcall, stale-handle hazard");
  return 0;
}
