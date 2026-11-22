#include "sin_compat/original_grunt_event_parent_phase592.hpp"
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
using namespace sin_compat::grunt_event_parent_phase592;

struct Harness final:Host {
 std::vector<std::string> calls;
 bool clear_marker=false;
 int shared=0,dedicated=0,special43=0,component=0,prefix=0;
 Event last{};
 void original_prefix_update(float v) override {
  assert(v==0.75f); ++prefix;calls.push_back("prefix");
 }
 void unresolved_dedicated(std::uint32_t event,std::uint8_t branch) override {
  assert(branch==original_case_index(event)&&branch!=24);++dedicated;calls.push_back("dedicated");
 }
 void optional_component_event(const Event& event,std::uint8_t& marker) override {
  ++component;last=event;assert(marker==1);calls.push_back("component");
  if(clear_marker)marker=0;
 }
 void original_special_43(const Event& event) override {
  assert(event.numeric_id==0x43u);++special43;calls.push_back("special43");
 }
 void original_shared_npc_event(const Event& event) override {
  ++shared;last=event;calls.push_back("base");
 }
};

int main() {
 static_assert(native_table_entries==156u);
 static_assert(first_native_id==3u&&last_native_id==0x9eu);
 static_assert(jetpack_event_0x54_uses_generic_case());
 static_assert(original_case_index(0x2b)==original_case_index(0x2c));
 static_assert(original_case_index(0x38)==original_case_index(0x3c));
 static_assert(original_case_index(0x43)==3);
 static_assert(original_case_index(0x54)==24);
 static_assert(original_case_index(0x9f)==24);
 std::uint32_t dedicated=0,defaults=0;
 for(std::uint32_t id=first_native_id;id<=last_native_id;++id){
  auto branch=original_case_index(id);
  assert(branch<=24);
  if(branch==24)++defaults;else ++dedicated;
 }
 assert(dedicated==native_specific_ids&&defaults==native_default_in_range);
 for(std::uint32_t id:{0u,1u,2u,0x9fu,0x123u,0xffffffffu})
  assert(original_case_index(id)==24);
 {
  Harness h;Event e{0x54u,1.75f};
  assert(grunt_parent_2235bfd0(e,false,h)==Result::DelegatedToSharedNPCEvent);
  assert((h.calls==std::vector<std::string>{"prefix","base"}));
  assert(h.last.numeric_id==0x54u&&h.last.native_float_parameter==1.75f);
 }
 {
  Harness h;Event e{0x54u,1.f};
  assert(grunt_parent_2235bfd0(e,true,h)==Result::ConsumedByOptionalComponent);
  assert((h.calls==std::vector<std::string>{"prefix","component"}));
  assert(h.shared==0&&h.component==1);
 }
 {
  Harness h;h.clear_marker=true;Event e{0x54u,1.f};
  assert(grunt_parent_2235bfd0(e,true,h)==Result::DelegatedToSharedNPCEvent);
  assert((h.calls==std::vector<std::string>{"prefix","component","base"}));
 }
 {
  Harness h;Event e{0x43u,1.f};
  assert(intermediary_2202d930(e,true,h)==Result::HandledSpecial43);
  assert((h.calls==std::vector<std::string>{"special43"}));
  // The full Grunt parent has a DEDICATED case for 0x43, so it does not
  // dispatch directly through the intermediary's special-0x43 path.
  assert(grunt_parent_2235bfd0(e,true,h)==Result::RequiresOriginalDedicatedHandler);
  assert((h.calls==std::vector<std::string>{"special43","prefix","dedicated"}));
 }
 {
  Harness h;Event e{0x8cu,0.f};
  assert(grunt_parent_2235bfd0(e,false,h)==Result::RequiresOriginalDedicatedHandler);
  assert((h.calls==std::vector<std::string>{"prefix","dedicated"}));
 }
 {
  Harness h;Event e{0x100u,0.f};
  assert(grunt_parent_2235bfd0(e,false,h)==Result::DelegatedToSharedNPCEvent);
  assert((h.calls==std::vector<std::string>{"prefix","base"}));
 }
}