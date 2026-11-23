#include "sin_compat/original_elite_event47_helpers_phase633.hpp"
#include "sin_compat/original_elite_secondary_event_phase632.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
namespace h=sin_compat::original_elite_event47_helpers_phase633;
namespace e=sin_compat::original_elite_secondary_event_phase632;
namespace {
struct Clock final:h::IClockHost {
  float now=10.0f, deadline=10.0f;
  unsigned count=0;
  bool refuse=false,throw_on_deadline=false;
  h::Cooldown* reenter=nullptr;
  h::Code nested=h::Code::Allowed;
  bool read_original_engine_time(float& out)override {
    ++count;out=now;return !refuse;
  }
  bool read_original_elite_deadline_9dc(float& out)override {
    ++count;
    if(throw_on_deadline)throw std::runtime_error("clock");
    if(reenter)nested=reenter->check().code;
    out=deadline;
    return !refuse;
  }
};
struct Serial final:h::ISerialHost {
  std::vector<std::uint32_t> handles{0x00001004u};
  std::vector<std::uint64_t> targets{0xABC00004u};
  unsigned reads=0,opens=0,slots=0;
  bool missing=false,wrong_serial=false,refuse=false;
  std::uint32_t last_handle=0;
  h::SerialLookup* reenter=nullptr;
  h::Code nested=h::Code::TargetFound;
  bool read_original_elite_serial_78c(std::uint32_t& out)override {
    ++reads;
    if(refuse)return false;
    const unsigned i=(reads-1u)%static_cast<unsigned>(handles.size());
    last_handle=handles[i];out=handles[i];
    return true;
  }
  bool open_original_serial_table()override {
    ++opens;return !refuse;
  }
  int read_original_serial_slot(std::uint32_t index,
      std::uint32_t serial,std::uint64_t& out)override {
    ++slots;
    if(reenter)nested=reenter->resolve().code;
    if(refuse)return -1;
    assert(index==(last_handle&0xfffu));
    assert(serial==(last_handle>>12u));
    if(wrong_serial)return 0;
    const unsigned i=(reads-1u)%static_cast<unsigned>(targets.size());
    out=missing?0u:targets[i];
    return 1;
  }
};
struct Field final:h::IFieldHost {
  std::vector<std::string> calls;
  std::uint32_t stored=17u;
  std::uint8_t special=0u, dirty_byte=0u;
  std::uint64_t owner=0x23u;
  std::uint32_t owner_bits=0u;
  std::string refuse;
  h::Field4ECSetter* reenter=nullptr;
  h::Code nested=h::Code::Applied;
  bool check(const char* name) {
    calls.emplace_back(name);
    if(reenter && std::string(name)=="flag")
      nested=reenter->set(55u).code;
    return refuse!=name;
  }
  bool read_native_dword_4ec(std::uint32_t& out)override {
    if(!check("read4ec"))return false;
    out=stored;return true;
  }
  bool read_native_flag_1f0(std::uint8_t& out)override {
    if(!check("flag"))return false;
    out=special;return true;
  }
  bool write_native_dirty_byte_200(std::uint8_t value)override {
    if(!check("dirty200"))return false;
    assert(value==1u);dirty_byte=value;return true;
  }
  bool read_native_dirty_owner_16c(std::uint64_t& out)override {
    if(!check("owner16c"))return false;
    out=owner;return true;
  }
  bool or_native_dirty_owner_bit0(std::uint64_t target)override {
    if(!check("dirty0"))return false;
    assert(target==owner);owner_bits|=1u;return true;
  }
  bool write_native_dword_4ec(std::uint32_t value)override {
    if(!check("write4ec"))return false;
    stored=value;return true;
  }
};
void expect(const Field& f,std::initializer_list<std::string> trace){
  assert(f.calls==std::vector<std::string>(trace));
}
void test_clock_original_f32_conditions() {
  Clock clock;h::Cooldown reference(clock);
  assert(reference.check().code==h::Code::Allowed);
  assert(clock.count==2u);
  clock.now=9.999f;clock.count=0;
  assert(reference.check().code==h::Code::Blocked);
  assert(clock.count==2u);
  clock.now=100.f;clock.deadline=std::numeric_limits<float>::infinity();
  assert(reference.check().code==h::Code::Blocked);
  clock.now=std::numeric_limits<float>::quiet_NaN();
  assert(reference.check().code==h::Code::Blocked);
  clock.now=10.f;clock.deadline=std::numeric_limits<float>::quiet_NaN();
  assert(reference.check().code==h::Code::Blocked);
  clock.now=std::numeric_limits<float>::infinity();
  clock.deadline=std::numeric_limits<float>::infinity();
  assert(reference.check().code==h::Code::Allowed);
  clock.refuse=true;clock.count=0;
  const auto rejected=reference.check();
  assert(rejected.code==h::Code::HostRejected &&
         rejected.stage==h::Stage::ClockRead&&clock.count==1);
  clock.refuse=false;clock.throw_on_deadline=true;
  const auto thrown=reference.check();
  assert(thrown.code==h::Code::HostRejected&&
         thrown.stage==h::Stage::EliteDeadline9DC);
  clock.throw_on_deadline=false;clock.reenter=&reference;
  assert(reference.check().code==h::Code::Allowed);
  assert(clock.nested==h::Code::Reentrant);
}
void test_encoded_target_lookup_live_three_times() {
  Serial host;
  host.handles={0x00001004u,0x00002005u,0x00003006u};
  host.targets={0x1001u,0x2002u,0x3003u};
  h::SerialLookup lookup(host);
  for(unsigned i=0;i<3;++i) {
    const auto r=lookup.resolve();
    assert(r.code==h::Code::TargetFound);
    assert(r.target==host.targets[i]);
    assert(r.slot_index==(4u+i)&&r.serial==(1u+i));
  }
  assert(host.reads==3&&host.opens==3&&host.slots==3);
  host.handles={h::kInvalidSerialHandle};
  host.reads=host.opens=host.slots=0;
  const auto missing=lookup.resolve();
  assert(missing.code==h::Code::TargetAbsent);
  assert(host.reads==1&&host.opens==0&&host.slots==0);
  host.handles={0x10000fffu};host.wrong_serial=true;
  assert(lookup.resolve().code==h::Code::TargetAbsent);
  host.wrong_serial=false;host.missing=true;
  assert(lookup.resolve().code==h::Code::TargetAbsent);
  host.missing=false;host.reenter=&lookup;
  assert(lookup.resolve().code==h::Code::TargetFound);
  assert(host.nested==h::Code::Reentrant);
  host.reenter=nullptr;host.refuse=true;
  const auto denied=lookup.resolve();
  assert(denied.code==h::Code::HostRejected&&
         denied.stage==h::Stage::EliteEncodedHandle78C);
}
void test_field_change_dirty_branches() {
  Field target;h::Field4ECSetter setter(target);
  auto same=setter.set(17u);
  assert(same.code==h::Code::Unchanged&&!same.changed);
  expect(target,{"read4ec"});
  target.calls.clear();
  auto result=setter.set(25u);
  assert(result.code==h::Code::Applied&&result.changed);
  assert(result.edict_dirty_committed&&!result.dirty_byte_committed);
  assert(result.field_committed&&target.stored==25u&&target.owner_bits==1u);
  expect(target,{"read4ec","flag","owner16c","dirty0","write4ec"});
  target.calls.clear();target.owner=0u;
  result=setter.set(26u);
  assert(result.code==h::Code::Applied&&!result.edict_dirty_committed);
  expect(target,{"read4ec","flag","owner16c","write4ec"});
  target.calls.clear();target.special=1u;
  result=setter.set(27u);
  assert(result.code==h::Code::Applied&&result.dirty_byte_committed);
  assert(target.dirty_byte==1u);
  expect(target,{"read4ec","flag","dirty200","write4ec"});
}
void test_source_order_partial_failures() {
  for(const char* stage:{"read4ec","flag","owner16c",
                         "dirty0","write4ec"}) {
    Field f;h::Field4ECSetter setter(f);
    f.refuse=stage;
    const auto r=setter.set(55u);
    assert(r.code==h::Code::HostRejected);
    if(f.refuse=="write4ec") {
      assert(r.edict_dirty_committed);
      assert(f.owner_bits==1u&&f.stored==17u);
      assert(!r.field_committed);
    } else {
      assert(!r.field_committed&&f.stored==17u);
    }
  }
  Field f;f.special=1u;f.refuse="write4ec";
  h::Field4ECSetter setter(f);
  const auto partial=setter.set(99u);
  assert(partial.code==h::Code::HostRejected);
  assert(partial.dirty_byte_committed&&f.dirty_byte==1u);
  assert(f.stored==17u);
  f.refuse.clear();f.reenter=&setter;
  assert(setter.set(99u).code==h::Code::Applied);
  assert(f.nested==h::Code::Reentrant);
}
// Integration: real Phase632 event dispatch calls all three complete
// original helpers, with independent object reads and the source-backed
// target+0x4EC field mutation.
struct EventHost final:e::IHost,h::IClockHost,h::ISerialHost,h::IFieldHost {
  Clock clock;Serial serial;Field field;
  std::vector<std::string> events;
  unsigned commands=0;
  h::Cooldown clock_ref;
  h::SerialLookup serial_ref;
  h::Field4ECSetter setter_ref;
  EventHost():clock_ref(*this),serial_ref(*this),setter_ref(*this) {
    serial.handles={0x1005u,0x2006u,0x3007u};
    serial.targets={0x111u,0x222u,0x333u};
    clock.now=8.f;clock.deadline=7.f;
  }
  bool read_original_engine_time(float& out)override {
    return clock.read_original_engine_time(out);
  }
  bool read_original_elite_deadline_9dc(float& out)override {
    return clock.read_original_elite_deadline_9dc(out);
  }
  bool read_original_elite_serial_78c(std::uint32_t& out)override {
    return serial.read_original_elite_serial_78c(out);
  }
  bool open_original_serial_table()override {
    return serial.open_original_serial_table();
  }
  int read_original_serial_slot(std::uint32_t index,
    std::uint32_t serial_id,std::uint64_t& out)override {
    return serial.read_original_serial_slot(index,serial_id,out);
  }
  bool read_native_dword_4ec(std::uint32_t& out)override {
    return field.read_native_dword_4ec(out);
  }
  bool read_native_flag_1f0(std::uint8_t& out)override {
    return field.read_native_flag_1f0(out);
  }
  bool write_native_dirty_byte_200(std::uint8_t n)override {
    return field.write_native_dirty_byte_200(n);
  }
  bool read_native_dirty_owner_16c(std::uint64_t& out)override {
    return field.read_native_dirty_owner_16c(out);
  }
  bool or_native_dirty_owner_bit0(std::uint64_t t)override {
    return field.or_native_dirty_owner_bit0(t);
  }
  bool write_native_dword_4ec(std::uint32_t n)override {
    return field.write_native_dword_4ec(n);
  }
  bool read_elite_predicate_fd9(std::uint8_t& out)override {
    out=0;return true;
  }
  bool read_elite_serial_handle_fdc(std::uint32_t& out)override {
    out=h::kInvalidSerialHandle;return true;
  }
  int resolve_original_serial(std::uint32_t,std::uint64_t&)override{
    return 0;
  }
  bool original_gate43(std::uint64_t,bool&)override{return false;}
  bool inherited_event_2235CCE0(std::uint32_t)override{return true;}
  bool predicate_220326D0(bool& enabled)override {
    events.emplace_back("cooldown");
    auto r=clock_ref.check();
    enabled=r.code==h::Code::Allowed;
    return r.code==h::Code::Allowed||r.code==h::Code::Blocked;
  }
  bool fetch_222AA640(std::uint64_t& target)override {
    events.emplace_back("serial-lookup");
    const auto r=serial_ref.resolve();
    target=r.target;
    return r.code==h::Code::TargetFound||
           r.code==h::Code::TargetAbsent;
  }
  bool object_virtual_404(std::uint64_t target,
                          std::uint32_t& out)override{
    events.emplace_back("virtual404");
    if(target!=0x222u)return false;
    out=0xF00Du;return true;
  }
  bool component_4EC_call_2203F8E0(std::uint64_t target,
                                    std::uint32_t value)override {
    events.emplace_back("field4ec");
    if(target!=0x333u)return false;
    const auto r=setter_ref.set(value);
    return r.code==h::Code::Applied||r.code==h::Code::Unchanged;
  }
  bool call_220398A0(unsigned n)override {
    assert(n==commands+3u);++commands;
    events.emplace_back("clear-flag");return true;
  }
  bool action_zero_2203D2D0()override {
    events.emplace_back("state-set");return true;
  }
};
void test_real_helper_composition_with_event47() {
  EventHost host;
  e::Reference original_event(host);
  const auto result=original_event.dispatch(e::kEvent47);
  assert(result.code==e::Code::ActionZero);
  assert(result.object_lookups==3u && result.numbered_callbacks==3u);
  assert(host.serial.reads==3u&&host.serial.opens==3u&&host.serial.slots==3u);
  assert(host.clock.count==2u);
  assert(host.field.stored==0xF00Du && host.field.owner_bits==1u);
  assert(host.commands==3u);
  assert(host.events==std::vector<std::string>({
    "cooldown","serial-lookup","serial-lookup","virtual404",
    "serial-lookup","field4ec","clear-flag","clear-flag",
    "clear-flag","state-set"
  }));
  EventHost too_early;
  too_early.clock.now=5.f;
  e::Reference ref(too_early);
  const auto early=ref.dispatch(e::kEvent47);
  assert(early.code==e::Code::Skipped);
  assert(too_early.serial.reads==0u && too_early.commands==0u);
  assert(too_early.events==std::vector<std::string>({"cooldown"}));
}
} // namespace
int main() {
  test_clock_original_f32_conditions();
  test_encoded_target_lookup_live_three_times();
  test_field_change_dirty_branches();
  test_source_order_partial_failures();
  test_real_helper_composition_with_event47();
  std::puts("PASS Phase633 original Elite event47 clock+9DC, serial+78C and dirty +4EC helper integration");
}
