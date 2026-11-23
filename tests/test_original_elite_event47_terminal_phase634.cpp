// Phase634: original Elite event47 terminal methods and full 632/633/634
// source-backed path composition. Original game DLL is NOT loaded.
#include "sin_compat/original_elite_event47_terminal_phase634.hpp"
#include "sin_compat/original_elite_event47_helpers_phase633.hpp"
#include "sin_compat/original_elite_secondary_event_phase632.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
namespace t=sin_compat::original_elite_event47_terminal_phase634;
namespace h=sin_compat::original_elite_event47_helpers_phase633;
namespace e=sin_compat::original_elite_secondary_event_phase632;
namespace {
struct Flags:t::IHost {
  std::vector<std::string> calls;
  std::array<std::uint32_t,4> words{{0xffffffffu,0u,0u,0u}};
  std::uint32_t state7a4=0u;
  std::uint64_t context=0x50500u;
  std::int32_t translated_override=std::numeric_limits<std::int32_t>::min();
  bool override_map=false;
  bool maintenance_ok=true;
  std::string fail;
  bool throw_on_fail=false;
  unsigned maps=0,maintenance=0,state_writes=0,clear_writes=0,tests=0;
  t::ClearFlag* reenter_clear=nullptr;
  t::StateZero* reenter_state=nullptr;
  t::Code nested_clear=t::Code::Skipped,nested_state=t::Code::Skipped;
  bool ok(const char* name) {
    calls.emplace_back(name);
    if(throw_on_fail && fail==name)
      throw std::runtime_error("injected host exception");
    return fail!=name;
  }
  bool original_virtual_760_context(std::uint64_t& out)override {
    if(!ok("vtable760"))return false;
    out=context;return true;
  }
  bool original_id_map_22073700(std::uint64_t ctx,
      std::int32_t id,std::int32_t& out)override {
    if(!ok("map-id"))return false;
    assert(ctx==context&&ctx!=0u);
    ++maps;
    if(reenter_clear)nested_clear=reenter_clear->run(3).code;
    if(reenter_state)nested_state=reenter_state->run(0).code;
    if(override_map)out=translated_override;
    else if(id==3 || id==4 || id==5 || id==0x21)
      out=static_cast<std::int32_t>(t::kOriginalIndexBase)+id;
    else out=-1;
    return true;
  }
  bool clear_original_elite_flag_bit(
      std::uint32_t index,std::uint32_t mask)override {
    if(!ok("clear-bit"))return false;
    assert(index<words.size());
    words[index]&=~mask;++clear_writes;return true;
  }
  bool read_original_elite_flag_bit(
      std::uint32_t index,std::uint32_t mask,bool& set)override {
    if(!ok("test-bit"))return false;
    assert(index<words.size());
    set=(words[index]&mask)!=0u;++tests;return true;
  }
  bool original_maintenance_2204E420()override {
    if(!ok("maintenance"))return false;
    ++maintenance;return maintenance_ok;
  }
  bool write_original_elite_state_7a4(std::uint32_t value)override {
    if(!ok("state4"))return false;
    assert(value==4u);state7a4=value;++state_writes;return true;
  }
};
void equals(const Flags& f,std::initializer_list<std::string> wanted) {
  assert(f.calls==std::vector<std::string>(wanted));
}
void test_three_flag_clear_calls_are_distinct() {
  Flags f;t::ClearFlag ref(f);
  for(std::int32_t id:{3,4,5}) {
    const auto result=ref.run(id);
    assert(result.code==t::Code::Cleared&&result.map_called);
    assert(result.converted_id==
        static_cast<std::int32_t>(t::kOriginalIndexBase)+id);
    assert(result.word_index==0&&result.mask==(1u<<id));
    assert(result.bit_cleared);
  }
  assert(f.words[0]==(0xffffffffu&~(1u<<3)&~(1u<<4)&~(1u<<5)));
  assert(f.maps==3&&f.clear_writes==3);
  equals(f,{"vtable760","map-id","clear-bit",
            "vtable760","map-id","clear-bit",
            "vtable760","map-id","clear-bit"});
}
void test_direct_encoded_input_and_original_hazards() {
  Flags f;t::ClearFlag ref(f);
  auto direct=ref.run(1000000035);
  assert(direct.code==t::Code::Cleared&&!direct.map_called);
  assert(direct.word_index==1 && direct.mask==(1u<<3));
  assert(f.maps==0 && f.words[1]==0u);
  f.override_map=true;f.translated_override=-1;
  auto skip=ref.run(3);
  assert(skip.code==t::Code::Skipped&&skip.map_called);
  assert(f.clear_writes==1u);
  f.translated_override=999999999;
  skip=ref.run(3);
  assert(skip.code==t::Code::Skipped);
  f.translated_override=999999998;
  const auto unsafe=ref.run(3);
  assert(unsafe.code==t::Code::OriginalUnsafeFlagIndex);
  assert(unsafe.failed_stage==t::Stage::ComputeFlagIndex);
  assert(f.clear_writes==1u);
  f.override_map=false;f.context=0u;
  const auto null=ref.run(3);
  assert(null.code==t::Code::OriginalNullContextHazard);
  assert(null.failed_stage==t::Stage::ObtainVirtual760Context);
}
void test_maintenance_unconditional_and_flag_state() {
  Flags f;t::StateZero ref(f);
  f.words[1]=1u<<1;  // original ID 0x21 -> bit1 in word1
  auto blocked=ref.run(0u);
  assert(blocked.code==t::Code::StatePreserved);
  assert(blocked.maintenance_called&&blocked.bit_tested);
  assert(!blocked.state_written&&f.state_writes==0u);
  equals(f,{"maintenance","vtable760","map-id","test-bit"});
  f.calls.clear();f.words[1]=0u;
  const auto written=ref.run(0u);
  assert(written.code==t::Code::State4Written);
  assert(written.state_written && f.state7a4==4u);
  equals(f,{"maintenance","vtable760","map-id","test-bit","state4"});
  f.calls.clear();
  const auto nonzero=ref.run(9u);
  assert(nonzero.code==t::Code::State4Written);
  assert(nonzero.maintenance_called && !nonzero.map_called);
  equals(f,{"maintenance","state4"});
  f.calls.clear();f.override_map=true;f.translated_override=-1;
  const auto absent=ref.run(0u);
  assert(absent.code==t::Code::State4Written);
  assert(!absent.bit_tested);
  equals(f,{"maintenance","vtable760","map-id","state4"});
  f.calls.clear();f.translated_override=999999998;
  const auto negative=ref.run(0u);
  assert(negative.code==t::Code::OriginalUnsafeFlagIndex);
  assert(negative.maintenance_called&&!negative.state_written);
}
void test_terminal_failures_and_reentrancy() {
  struct Case {const char* name;t::Stage stage;bool clear;};
  for(const Case c:{
      Case{"vtable760",t::Stage::ObtainVirtual760Context,true},
      Case{"map-id",t::Stage::ConvertSourceId,true},
      Case{"clear-bit",t::Stage::ClearFlagWord,true},
      Case{"maintenance",t::Stage::MaintenanceCall,false},
      Case{"vtable760",t::Stage::ObtainVirtual760Context,false},
      Case{"map-id",t::Stage::ConvertSourceId,false},
      Case{"test-bit",t::Stage::TestFlagWord,false},
      Case{"state4",t::Stage::WriteState4,false}
    }) {
    for(int should_throw=0;should_throw<2;++should_throw) {
      Flags f;f.fail=c.name;f.throw_on_fail=should_throw!=0;
      t::ClearFlag clear(f);t::StateZero state(f);
      const t::Result result=c.clear?clear.run(3):state.run(0);
      assert(result.code==t::Code::HostRejected);
      assert(result.failed_stage==c.stage);
      if(!c.clear && std::string(c.name)!="maintenance")
        assert(result.maintenance_called);
    }
  }
  Flags f;t::ClearFlag clear(f);
  f.reenter_clear=&clear;
  assert(clear.run(3).code==t::Code::Cleared);
  assert(f.nested_clear==t::Code::Reentrant);
  f.reenter_clear=nullptr;
  t::StateZero state(f);f.reenter_state=&state;
  assert(state.run(0).code==t::Code::State4Written);
  assert(f.nested_state==t::Code::Reentrant);
}
// Full compiled original source chain: Phase632 secondary event0x47 ->
// Phase633 cooldown, three re-read serial targets, +4EC dirty setter ->
// Phase634 flag clear commands 3/4/5 and state-zero with bit 0x21.
struct Integrated final:Flags,e::IHost,h::IClockHost,h::ISerialHost,
                      h::IFieldHost {
  float clock=8.f,deadline=7.f;
  std::uint32_t serial_handles[3]{0x1001u,0x2002u,0x3003u};
  std::uint64_t targets[3]{0x111u,0x222u,0x333u};
  unsigned lookup_reads=0,open_count=0,slot_reads=0,numbered=0;
  std::uint32_t field4ec=17u,owner_dirty=0u;
  std::uint8_t special_flag=0u,dirty_byte=0u;
  std::uint64_t dirty_owner=0x666u;
  std::uint32_t virtual404_value=23u;
  std::uint8_t fd9=0u;
  std::uint32_t fdc=0x4001u;
  bool allow_gate43=false;
  h::Cooldown cooldown;
  h::SerialLookup serial;
  h::Field4ECSetter setter;
  t::ClearFlag clear;
  t::StateZero state;
  Integrated():cooldown(*this),serial(*this),setter(*this),
               clear(*this),state(*this){}
  bool read_original_engine_time(float& out)override {
    calls.emplace_back("clock");out=clock;return true;
  }
  bool read_original_elite_deadline_9dc(float& out)override {
    calls.emplace_back("deadline");out=deadline;return true;
  }
  bool read_original_elite_serial_78c(std::uint32_t& out)override {
    calls.emplace_back("read78c");
    if(lookup_reads>=3u)return false;
    out=serial_handles[lookup_reads++];
    return true;
  }
  bool open_original_serial_table()override {
    calls.emplace_back("open-serial");++open_count;return true;
  }
  int read_original_serial_slot(
      std::uint32_t index,std::uint32_t generation,
      std::uint64_t& out)override {
    calls.emplace_back("serial-slot");
    assert(slot_reads<3);
    const unsigned i=slot_reads++;
    assert(index==(serial_handles[i]&0xfffu));
    assert(generation==(serial_handles[i]>>12u));
    out=targets[i];return 1;
  }
  bool read_native_dword_4ec(std::uint32_t& out)override {
    calls.emplace_back("read4ec");out=field4ec;return true;
  }
  bool read_native_flag_1f0(std::uint8_t& out)override {
    calls.emplace_back("flag1f0");out=special_flag;return true;
  }
  bool write_native_dirty_byte_200(std::uint8_t value)override {
    calls.emplace_back("write200");dirty_byte=value;return true;
  }
  bool read_native_dirty_owner_16c(std::uint64_t& out)override {
    calls.emplace_back("owner16c");out=dirty_owner;return true;
  }
  bool or_native_dirty_owner_bit0(std::uint64_t id)override {
    calls.emplace_back("or-owner");assert(id==dirty_owner);
    owner_dirty|=1u;return true;
  }
  bool write_native_dword_4ec(std::uint32_t value)override {
    calls.emplace_back("write4ec");field4ec=value;return true;
  }
  bool read_elite_predicate_fd9(std::uint8_t& out)override {
    out=fd9;return true;
  }
  bool read_elite_serial_handle_fdc(std::uint32_t& out)override {
    out=fdc;return true;
  }
  int resolve_original_serial(
      std::uint32_t handle,std::uint64_t& out)override {
    assert(handle==fdc);out=0x777u;return 1;
  }
  bool original_gate43(std::uint64_t target,bool& allowed)override {
    assert(target==0x777u);allowed=allow_gate43;return true;
  }
  bool inherited_event_2235CCE0(std::uint32_t)override{return true;}
  bool predicate_220326D0(bool& out)override {
    const auto r=cooldown.check();
    out=r.code==h::Code::Allowed;
    return r.code==h::Code::Allowed||r.code==h::Code::Blocked;
  }
  bool fetch_222AA640(std::uint64_t& out)override {
    const auto r=serial.resolve();out=r.target;
    return r.code==h::Code::TargetFound||r.code==h::Code::TargetAbsent;
  }
  bool object_virtual_404(std::uint64_t id,std::uint32_t& out)override {
    assert(id==targets[1]);
    calls.emplace_back("virtual404");out=virtual404_value;return true;
  }
  bool component_4EC_call_2203F8E0(
      std::uint64_t id,std::uint32_t n)override {
    assert(id==targets[2]);
    const auto r=setter.set(n);
    return r.code==h::Code::Applied||r.code==h::Code::Unchanged;
  }
  bool call_220398A0(unsigned n)override {
    assert(n==numbered+3u);++numbered;
    return clear.run(static_cast<std::int32_t>(n)).code==t::Code::Cleared;
  }
  bool action_zero_2203D2D0()override {
    const auto r=state.run(0u);
    return r.code==t::Code::State4Written||
           r.code==t::Code::StatePreserved;
  }
};
void test_full_original_event47_chain() {
  Integrated h;
  e::Reference event(h);
  // If bit 0x21 is absent, original state4 is written at the end.
  const auto r=event.dispatch(e::kEvent47);
  assert(r.code==e::Code::ActionZero);
  assert(r.object_lookups==3u&&r.numbered_callbacks==3u);
  assert(h.lookup_reads==3u&&h.open_count==3u&&h.slot_reads==3u);
  assert(h.field4ec==h.virtual404_value&&h.owner_dirty==1u);
  assert(h.numbered==3u&&h.maps==4u&&h.clear_writes==3u);
  assert(h.words[0]==(0xffffffffu&~(1u<<3)&~(1u<<4)&~(1u<<5)));
  assert(h.words[1]==0u&&h.state7a4==4u&&h.state_writes==1u);
  assert(h.maintenance==1u&&h.tests==1u);
  // For 0x43, the source gate controls the same state0 invocation.
  Integrated second;e::Reference event43(second);
  second.words[1]=1u<<1; // original 0x21 flag set: preserve state
  second.allow_gate43=true;
  const auto gate=event43.dispatch(e::kEvent43);
  assert(gate.code==e::Code::ActionZero);
  assert(second.maintenance==1u&&second.state_writes==0u);
  second.allow_gate43=false;second.maintenance=0;
  const auto denied=event43.dispatch(e::kEvent43);
  assert(denied.code==e::Code::Skipped&&second.maintenance==0u);
}
} // namespace
int main() {
  test_three_flag_clear_calls_are_distinct();
  test_direct_encoded_input_and_original_hazards();
  test_maintenance_unconditional_and_flag_state();
  test_terminal_failures_and_reentrancy();
  test_full_original_event47_chain();
  std::puts("PASS Phase634 complete original Elite flag-clear/state4, mapped bit IDs, 632+633+634 integrated event0x47 and gate0x43 paths");
}
