#include "sin_compat/original_grunt_virtuals_phase564.hpp"
#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace sin_compat::original_grunt_virtuals_phase564;

struct Probe final : Host {
  std::vector<std::string> events;
  std::uint32_t last_handle = 0;
  std::uintptr_t resolved = 0xCAFE;
  bool admission_result = true;
  bool bit_exists = true;
  int eligible = 0;
  int sentence_result = -1;
  bool callback_result = true;
  int note = -100;
  std::uint32_t mark = 10;
  std::string sent;
  int sent_arg = -111;
  unsigned sent_mode = 0;
  std::uintptr_t group = 0;
  std::uintptr_t key = 0;
  int first = 0, last = 0;
  std::uintptr_t callback_arg = 0;

  std::uintptr_t resolve_entity(std::uint32_t handle) override {
    events.push_back("resolve"); last_handle=handle;return resolved;
  }
  bool reserve_group(std::uintptr_t grp, std::uintptr_t k, std::int32_t a,
                     std::int32_t b, std::int32_t& selection) override {
    events.push_back("admit");group=grp;key=k;first=a;last=b;
    if(admission_result)selection=a;
    return admission_result;
  }
  void bitmap_lookup(std::uintptr_t grp,std::uintptr_t k) override {
    events.push_back("bitmap"); group=grp;key=k;
  }
  bool bitmap_has(std::int32_t index) override {
    events.push_back("testbit");note=index;return bit_exists;
  }
  void report_missing_bit() override {events.push_back("missing");}
  void bitmap_clear(std::int32_t index) override {
    events.push_back("clearbit");note=index;
  }
  std::int32_t eligible_group_members(std::uintptr_t grp) override {
    events.push_back("count");group=grp;return eligible;
  }
  int dispatch_sentence(std::string_view token,int arg,unsigned mode) override {
    events.push_back("speak");sent=std::string(token);sent_arg=arg;sent_mode=mode;
    return sentence_result;
  }
  void owner_mark(std::int32_t arg,std::uint32_t mask) override {
    events.push_back("mark");note=arg;mark=mask;
  }
  bool generic_member_callback(std::uintptr_t candidate,
                               std::uintptr_t position,
                               std::uintptr_t context) override {
    events.push_back("callback");callback_arg=candidate;
    return callback_result && position==2 && context==3;
  }
  void clear_events(){events.clear();}
};

int main() {
  Probe h;
  Grunt g;
  assert(!grunt_virtual_694());
  assert(last_of_squad_token=="GRUNT_LAST_OF_SQUAD");
  assert(man_down_token=="GRUNT_MAN_DOWN");

  // Actual Grunt +0x690 stores the linked group pointer and supports null.
  set_group_22044670(g,0xAABB);
  assert(g.group_0x9c4==0xAABBu);
  set_group_22044670(g,0);
  assert(g.group_0x9c4==0);

  // Original +0x154 resolves +0x8A0; sentinel bypasses the resolver.
  assert(selected_entity_22044840(g,h)==0);
  assert(h.events.empty());
  g.target_handle_0x8a0=0x56780014u;
  assert(selected_entity_22044840(g,h)==0xCAFE);
  assert(h.last_handle==g.target_handle_0x8a0);
  h.clear_events();

  // Original group admission without any group returns true without reads.
  g.selection_0x9cc=77;
  assert(admission_22054d40(g,0x0e,h));
  assert(g.selection_0x9cc==77 && h.events.empty());
  set_group_22044670(g,0xBEEFu);
  h.admission_result=false;
  assert(!admission_22054d40(g,0x0c,h));
  assert((h.events==std::vector<std::string>{"resolve","admit"}));
  assert(h.key==0xCAFEu && h.group==0xBEEFu);
  assert(h.first==0x0c && h.last==0x0c && g.selection_0x9cc==77);
  h.admission_result=true; h.clear_events();
  assert(admission_22054d40(g,0x0e,h));
  assert(g.selection_0x9cc==0x0e);
  assert(selected_in_range_22054c80(g,0x0e,0x0e));
  assert(selected_in_range_22054c80(g,0x0a,0x0e));
  assert(!selected_in_range_22054c80(g,0x0f,0x20));
  assert(!selected_in_range_22054c80(g,0x11,0x0a));

  // Bitmap release: 2 lookups even if the first sees an unmarked bit.
  h.bit_exists=false;h.clear_events();
  release_selection_22054ca0(g,h);
  assert(g.selection_0x9cc==-1);
  assert(h.key==0xCAFEu && h.note==0x0e);
  assert((h.events==std::vector<std::string>{
    "resolve","bitmap","testbit","missing","bitmap","clearbit"}));
  h.clear_events();
  release_selection_22054ca0(g,h);
  assert((h.events==std::vector<std::string>{"resolve"}));
  g.selection_0x9cc=15;h.bit_exists=true;h.clear_events();
  release_selection_22054ca0(g,h);
  assert((h.events==std::vector<std::string>{
    "resolve","bitmap","testbit","bitmap","clearbit"}));
  assert(g.selection_0x9cc==-1);
  set_group_22044670(g,0);
  g.selection_0x9cc=9;h.clear_events();
  release_selection_22054ca0(g,h);
  assert(g.selection_0x9cc==9 && h.events.empty());

  // Original +0x678 condition flag suppresses generic callback, while
  // clear bit forwards all 3 original context operands to a base routine.
  g.flag_0xb6=1;
  assert(!member_callback_22358590(g,h,1,2,3));
  assert(h.events.empty());
  g.flag_0xb6=0;
  assert(member_callback_22358590(g,h,1,2,3));
  assert(h.callback_arg==1);
  assert((h.events==std::vector<std::string>{"callback"}));

  // Original group leave only performs the speech callback if requested by
  // the *calling* group +0xAA0D0. Grunt +0x750 performs its own gates.
  set_group_22044670(g,0xD00Du);h.clear_events();
  g.flag_0xbf=8;h.eligible=9;
  on_member_removed_2235af70(g,0,h);
  assert(h.events.empty());
  g.flag_0xbf=0;
  on_member_removed_2235af70(g,1,h);
  assert(h.events.empty()); // positive original removed object +0x9C

  // Less than two: last-of-squad; argument -1, mode 1; mark even on error.
  for(int n : {-2,0,1}) {
    h.eligible=n;h.clear_events();h.sentence_result=-1;
    on_member_removed_2235af70(g,0,h);
    assert((h.events==std::vector<std::string>{"count","speak","mark"}));
    assert(h.sent=="GRUNT_LAST_OF_SQUAD" && h.sent_arg==-1 && h.sent_mode==1u);
    assert(h.note==0 && h.mark==0 && h.group==0xD00Du);
  }

  // Two or more: man-down; argument 0, mode 2; no manual owner mark.
  for(int n : {2,3,19}) {
    h.eligible=n;h.clear_events();h.sentence_result=17;
    on_member_removed_2235af70(g,-5,h);
    assert((h.events==std::vector<std::string>{"count","speak"}));
    assert(h.sent=="GRUNT_MAN_DOWN" && h.sent_arg==0 && h.sent_mode==2u);
  }
  return 0;
}