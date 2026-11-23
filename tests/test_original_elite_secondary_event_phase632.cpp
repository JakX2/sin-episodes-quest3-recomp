// Authentic 2006 Elite vtable +0x44C event43 / event47 reference model.
// Pure synthetic host: no real native Source objects or game execution.
#include "sin_compat/original_elite_secondary_event_phase632.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>
namespace source=sin_compat::original_elite_secondary_event_phase632;
namespace {
class Host final:public source::IHost {
public:
  std::vector<std::string> calls;
  std::uint8_t fd9=0u;
  std::uint32_t fdc=0xabc12345u;
  std::uint64_t serial_target=0x1fa30u;
  int serial_result=1;
  bool gate_allowed=false;
  bool predicate47=true;
  std::uint64_t object_values[3]{0x1001u,0x2002u,0x3003u};
  unsigned object_reads=0u;
  std::uint32_t virtual404_value=0xDEADBEEFu;
  std::uint32_t forwarded_value=0u;
  std::string fail;
  source::Reference* reenter=nullptr;
  source::Code nested=source::Code::Skipped;
  bool throw_call=false;
  bool ok(const char* stage) {
    calls.emplace_back(stage);
    if(throw_call&&fail==stage)throw std::runtime_error("synthetic callback exception");
    return fail!=stage;
  }
  bool read_elite_predicate_fd9(std::uint8_t& value)override {
    if(!ok("read-fd9"))return false;
    value=fd9;return true;
  }
  bool read_elite_serial_handle_fdc(std::uint32_t& value)override {
    if(!ok("read-fdc"))return false;
    value=fdc;return true;
  }
  bool open_original_serial_table()override {
    return ok("open-table");
  }
  int resolve_original_serial(std::uint32_t handle,
                              std::uint64_t& target)override {
    if(!ok("resolve-serial"))return -1;
    assert(handle==fdc);
    target=serial_target;
    return serial_result;
  }
  bool original_gate43(std::uint64_t target,bool& allowed)override {
    if(!ok("gate43"))return false;
    assert(target==serial_target&&target!=0u);
    if(reenter)nested=reenter->dispatch(source::kEvent43).code;
    allowed=gate_allowed;
    return true;
  }
  bool inherited_event_2235CCE0(std::uint32_t event)override {
    if(!ok("inherited"))return false;
    assert(event!=0u);
    return true;
  }
  bool predicate_220326D0(bool& allowed)override {
    if(!ok("predicate47"))return false;
    allowed=predicate47;return true;
  }
  bool fetch_222AA640(std::uint64_t& object)override {
    if(!ok("fetch-object"))return false;
    assert(object_reads<3u);
    object=object_values[object_reads++];
    return true;
  }
  bool object_virtual_404(std::uint64_t obj,std::uint32_t& out)override {
    if(!ok("virtual404"))return false;
    assert(obj==object_values[1]&&obj!=0u);
    out=virtual404_value;
    return true;
  }
  bool component_4EC_call_2203F8E0(
      std::uint64_t obj,std::uint32_t value)override {
    if(!ok("component4ec"))return false;
    assert(obj==object_values[2]&&obj!=0u);
    forwarded_value=value;
    return true;
  }
  bool call_220398A0(unsigned command)override {
    assert(command>=3u&&command<=5u);
    return ok(command==3u?"command3":
              command==4u?"command4":"command5");
  }
  bool action_zero_2203D2D0()override {return ok("action0");}
};
void expect(const Host& host,std::initializer_list<std::string> wanted) {
  assert(host.calls==std::vector<std::string>(wanted));
}
void test_unknown_and_fd9_delegate() {
  Host h;source::Reference ref(h);
  const auto generic=ref.dispatch(0x123456u);
  assert(generic.code==source::Code::Inherited&&
         generic.inherited_called&&!generic.gate_called);
  expect(h,{"inherited"});
  h.calls.clear();h.fd9=7u;
  auto r=ref.dispatch(source::kEvent43);
  assert(r.code==source::Code::Inherited&&
         r.inherited_called&&!r.gate_called);
  expect(h,{"read-fd9","inherited"});
}
void test_gate43_paths() {
  Host h;source::Reference ref(h);
  auto r=ref.dispatch(source::kEvent43);
  assert(r.code==source::Code::Skipped && r.gate_called);
  expect(h,{"read-fd9","read-fdc","open-table",
            "resolve-serial","gate43"});
  h.calls.clear();h.gate_allowed=true;
  r=ref.dispatch(source::kEvent43);
  assert(r.code==source::Code::ActionZero &&
         r.gate_called && r.action_zero_called &&
         !r.inherited_called);
  expect(h,{"read-fd9","read-fdc","open-table",
            "resolve-serial","gate43","action0"});
  for(int scenario=0;scenario<3;++scenario) {
    Host missing;source::Reference guard(missing);
    if(scenario==0)missing.fdc=source::kInvalidHandle;
    if(scenario==1)missing.serial_result=0;
    if(scenario==2)missing.serial_target=0u;
    const auto blocked=guard.dispatch(source::kEvent43);
    assert(blocked.code==source::Code::OriginalNullTargetHazard &&
           blocked.stage==source::Stage::Gate43 &&
           !blocked.gate_called && !blocked.action_zero_called);
    if(scenario==0)expect(missing,{"read-fd9","read-fdc"});
    else expect(missing,{"read-fd9","read-fdc","open-table",
                         "resolve-serial"});
  }
}
void test_event47_false_predicate_and_missing_first_object() {
  Host h;h.predicate47=false;
  source::Reference ref(h);
  auto r=ref.dispatch(source::kEvent47);
  assert(r.code==source::Code::Skipped &&
         !r.inherited_called&&!r.action_zero_called);
  expect(h,{"predicate47"});
  h.calls.clear();h.predicate47=true;h.object_values[0]=0u;
  r=ref.dispatch(source::kEvent47);
  assert(r.code==source::Code::ActionZero &&
         r.object_lookups==1u && r.numbered_callbacks==0u);
  expect(h,{"predicate47","fetch-object","action0"});
}
void test_event47_complete_three_fresh_lookups() {
  Host h;source::Reference ref(h);
  const auto r=ref.dispatch(source::kEvent47);
  assert(r.code==source::Code::ActionZero &&
         r.object_lookups==3u && r.numbered_callbacks==3u &&
         !r.inherited_called && r.action_zero_called);
  assert(h.object_reads==3u && h.forwarded_value==h.virtual404_value);
  expect(h,{"predicate47","fetch-object","fetch-object","virtual404",
            "fetch-object","component4ec","command3","command4",
            "command5","action0"});
}
void test_event47_second_and_third_null_hazard() {
  for(int missing_index:{1,2}) {
    Host h;source::Reference ref(h);
    h.object_values[missing_index]=0u;
    const auto r=ref.dispatch(source::kEvent47);
    assert(r.code==source::Code::OriginalNullTargetHazard &&
           !r.action_zero_called && !r.inherited_called);
    if(missing_index==1) {
      assert(r.stage==source::Stage::Virtual404OnSecond47);
      expect(h,{"predicate47","fetch-object","fetch-object"});
    } else {
      assert(r.stage==source::Stage::Component4EC47);
      expect(h,{"predicate47","fetch-object","fetch-object",
                "virtual404","fetch-object"});
    }
  }
}
void test_injected_rejections_with_stage_provenance() {
  const struct Case {
    const char* stage;
    std::uint32_t event_id;
    source::Stage expected;
    bool allow_gate;
  } cases[]{
    {"inherited",0x1u,source::Stage::InheritedDispatch,false},
    {"read-fd9",0x43u,source::Stage::ReadElitePredicateFD9,false},
    {"read-fdc",0x43u,source::Stage::ReadEliteHandleFDC,false},
    {"open-table",0x43u,source::Stage::OpenSerialTable,false},
    {"resolve-serial",0x43u,source::Stage::ResolveSerial,false},
    {"gate43",0x43u,source::Stage::Gate43,false},
    {"action0",0x43u,source::Stage::ActionZero,true},
    {"predicate47",0x47u,source::Stage::Predicate47,false},
    {"fetch-object",0x47u,source::Stage::FirstObjectLookup47,false},
    {"virtual404",0x47u,source::Stage::Virtual404OnSecond47,false},
    {"component4ec",0x47u,source::Stage::Component4EC47,false},
    {"command3",0x47u,source::Stage::NumberedCallback3,false},
    {"command4",0x47u,source::Stage::NumberedCallback4,false},
    {"command5",0x47u,source::Stage::NumberedCallback5,false}
  };
  for(const auto& tc:cases) {
    Host h;source::Reference ref(h);h.fail=tc.stage;
    h.gate_allowed=tc.allow_gate;
    const auto r=ref.dispatch(tc.event_id);
    assert(r.code==source::Code::HostRejected&&r.stage==tc.expected);
    Host ex;source::Reference thrown(ex);
    ex.fail=tc.stage;ex.throw_call=true;
    ex.gate_allowed=tc.allow_gate;
    const auto x=thrown.dispatch(tc.event_id);
    assert(x.code==source::Code::HostRejected&&x.stage==tc.expected);
  }
}
void test_reentrance_and_constants() {
  Host h;source::Reference ref(h);
  h.gate_allowed=true;h.reenter=&ref;
  const auto r=ref.dispatch(source::kEvent43);
  assert(r.code==source::Code::ActionZero);
  assert(h.nested==source::Code::Reentrant);
  assert(source::kVtableVA+source::kVtableOffset==0x2248E350u);
  assert(source::kOriginalVA==0x223556A0u);
  assert(source::kInheritedVA==0x2235CCE0u);
}
}
int main() {
  test_unknown_and_fd9_delegate();
  test_gate43_paths();
  test_event47_false_predicate_and_missing_first_object();
  test_event47_complete_three_fresh_lookups();
  test_event47_second_and_third_null_hazard();
  test_injected_rejections_with_stage_provenance();
  test_reentrance_and_constants();
  std::puts("PASS Phase632: authenticated original Elite event43/47 secondary handler, 3 lookups, numbered calls, null hazards");
}
