#include "sin_compat/original_grunt_combat_phase559.hpp"
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

using namespace sin_compat::original_grunt_combat_phase559;

struct TestHost final : IPrefixHost {
  std::vector<int> seen_conditions;
  std::vector<int> seen_external_gates;
  const Record* candidate = nullptr;
  bool cond_2b=false,cond_0c=false,cond_45=false,cond_16=false;
  bool external_result=false;
  bool linked_live=false;
  std::uint32_t linked_handle=0xffffffffu;
  int resolved_count=0;
  bool has_condition(int v) override {
    seen_conditions.push_back(v);
    switch(v){case 0x2b:return cond_2b;case 0x0c:return cond_0c;
      case 0x45:return cond_45;case 0x16:return cond_16;default:assert(false);return false;}
  }
  const Record* resolve_grunt_handle_0x98c() override {++resolved_count;return candidate;}
  bool external_gate_22054d40(int v) override {seen_external_gates.push_back(v);return external_result;}
  std::uint32_t grunt_handle_0xcb4() override {return linked_handle;}
  bool is_live_handle_0xcb4(std::uint32_t v) override {assert(v==linked_handle);return linked_live;}
};

static void test_group_path() {
  Record a{101,42,"npc_a",true,false,false};
  Record b{102,10,"npc_b",false,true,false};
  Record c{103,4,"npc_c",true,true,true}; // blocked by vtable +0x694
  Record d{104,9,"npc_d",false,false,false}; // no entity +0x204 / component
  HandleTable table;
  table.slots.resize(6);
  table.slots[1]={&a,4}; table.slots[2]={&b,6};
  table.slots[3]={&c,8}; table.slots[4]={&d,10};
  auto h=[](std::uint32_t index,std::uint32_t generation){return (generation<<12)|index;};
  assert(table.resolve(0xffffffffu)==nullptr);
  assert(table.resolve(h(2,7))==nullptr); // stale generation
  assert(table.resolve(h(0xffe,1))==nullptr); // extra host bounds gate
  assert(table.resolve(h(1,4))==&a);
  assert(eligible_220a_shared(&a)); assert(eligible_220a_shared(&b));
  assert(!eligible_220a_shared(&c));assert(!eligible_220a_shared(&d));
  assert(!eligible_220a_shared(nullptr));
  LinkedGroup none{};
  assert(group_count_220a9480(none,table,false)==0);
  assert(group_first_when_multiple_220a9210(none,table)==nullptr);
  LinkedGroup singleton{{h(1,4)}};
  assert(group_count_220a9480(singleton,table,true)==1);
  assert(group_first_when_multiple_220a9210(singleton,table)==nullptr);
  assert(!group_candidate_matches_220aa2d0(singleton,table,&a));
  LinkedGroup mixed{{h(3,8),h(1,4),h(4,10),h(2,6),h(2,7),0xffffffffu}};
  assert(group_count_220a9480(mixed,table,false)==6);
  assert(group_count_220a9480(mixed,table,true)==2);
  assert(group_first_when_multiple_220a9210(mixed,table)==&a);
  assert(group_candidate_matches_220aa2d0(mixed,table,&a));
  assert(!group_candidate_matches_220aa2d0(mixed,table,&b));
  assert(!group_candidate_matches_220aa2d0(mixed,table,&c));
  LinkedGroup reversed{{h(2,6),h(1,4),h(3,8)}};
  assert(group_first_when_multiple_220a9210(reversed,table)==&b);
  assert(!group_candidate_matches_220aa2d0(reversed,table,&a));
  assert(group_candidate_matches_220aa2d0(reversed,table,&b));
  LinkedGroup duplicate{{h(1,4),h(1,4)}};
  // Native group traversal counts two valid slots even if both resolve
  // to the same entity; deduplication is NOT implemented by this routine.
  assert(group_count_220a9480(duplicate,table,true)==2);
  assert(group_candidate_matches_220aa2d0(duplicate,table,&a));
  LinkedGroup only_bad{{0xffffffffu,h(3,8),h(4,10),h(2,7)}};
  assert(group_count_220a9480(only_bad,table,true)==0);
  assert(group_first_when_multiple_220a9210(only_bad,table)==nullptr);
}

static void test_prefix() {
  assert(is_special_object_class("func_physbox"));
  assert(is_special_object_class("prop_physics"));
  assert(is_special_object_class("func_breakable"));
  assert(is_special_object_class("func_door"));
  assert(!is_special_object_class("Func_Door"));
  assert(!is_special_object_class("npc_turret_floor"));
  Record obj{25,0,"func_door",false,false,false};
  TestHost host;
  auto out=evaluate_original_combat_prefix(host);
  assert(out.kind==PrefixKind::Continue);
  assert((host.seen_conditions==std::vector<int>{0x2b,0x0c,0x45,0x16}));
  assert(host.resolved_count==0);
  host=TestHost{};host.cond_2b=true;host.candidate=&obj;
  out=evaluate_original_combat_prefix(host);
  assert(out.kind==PrefixKind::ReturnNumeric&&out.raw_schedule==0x70);
  assert((host.seen_conditions==std::vector<int>{0x2b})); // 0x0c skipped
  assert(host.seen_external_gates.empty());
  obj.raw_dword_0x9c=10;host=TestHost{};
  host.cond_0c=true;host.candidate=&obj;host.external_result=true;
  out=evaluate_original_combat_prefix(host);
  assert(out.raw_schedule==0x70);
  assert((host.seen_external_gates==std::vector<int>{0x0e}));
  obj.classname="npc_custom";host.external_result=false;host=TestHost{};
  host.cond_2b=true;host.candidate=&obj;host.external_result=false;
  host.cond_45=true;
  out=evaluate_original_combat_prefix(host);
  assert(out.raw_schedule==0x66);
  assert((host.seen_conditions==std::vector<int>{0x2b,0x45}));
  assert((host.seen_external_gates==std::vector<int>{0x0e}));
  obj.raw_dword_0x9c=1000;obj.classname="func_door";
  host=TestHost{};host.cond_2b=true;host.candidate=&obj;host.cond_45=true;
  out=evaluate_original_combat_prefix(host);assert(out.raw_schedule==0x66);
  assert(host.seen_external_gates.empty()); // signed 1000 upper bound
  host=TestHost{};host.cond_16=true;host.linked_handle=0xffffffffu;
  out=evaluate_original_combat_prefix(host);assert(out.raw_schedule==0x27);
  host=TestHost{};host.cond_16=true;host.linked_handle=0x1001u;host.linked_live=true;
  out=evaluate_original_combat_prefix(host);assert(out.raw_schedule==0x72);
  host.linked_live=false;out=evaluate_original_combat_prefix(host);
  assert(out.raw_schedule==0x27);
  obj.raw_dword_0x9c=-1;obj.classname="func_physbox";
  host=TestHost{};host.cond_2b=true;host.candidate=&obj;
  out=evaluate_original_combat_prefix(host);assert(out.raw_schedule==0x70);
  assert(host.seen_external_gates.empty());
}
int main(){test_group_path();test_prefix();}