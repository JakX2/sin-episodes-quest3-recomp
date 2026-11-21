#include "sin_compat/original_grunt_sentence_phase558.hpp"
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

using namespace sin_compat::original_grunt_sentence_phase558;

struct Host : ISentenceHost {
  std::vector<std::string> calls;
  int state=0;
  std::uint32_t flags=0;
  bool related=false;
  int related_metric=0;
  bool relation_accepts=false;
  bool owner_accepts=false;
  int queue_result=41;
  SentenceState* observed_state=nullptr;
  std::int32_t owner_state_0x850() override { calls.emplace_back("state"); return state; }
  std::uint32_t owner_flags_0xb4() override { calls.emplace_back("flags"); return flags; }
  bool has_related_0x9c4() override { calls.emplace_back("related"); return related; }
  std::int32_t related_metric_arg1() override { calls.emplace_back("metric1"); return related_metric; }
  bool related_accepts_owner() override { calls.emplace_back("relation"); return relation_accepts; }
  bool owner_accepts_argument(std::int32_t n) override {
    calls.emplace_back("allows:"+std::to_string(n)); return owner_accepts;
  }
  std::int32_t queue_sentence(std::string_view v) override {
    calls.emplace_back("queue:"+std::string(v));
    if (observed_state) assert(observed_state->queue_id == -1);
    return queue_result;
  }
  void owner_update_argument(std::int32_t arg, std::uint32_t flags_in) override {
    assert(flags_in == 0x40000000u);
    calls.emplace_back("update:"+std::to_string(arg));
  }
  void report_cull() override { calls.emplace_back("cull"); }
};

void equals(const std::vector<std::string>& a, std::initializer_list<const char*> wanted) {
  assert(a.size() == wanted.size());
  std::size_t i=0;
  for (const char* x : wanted) assert(a[i++] == x);
}

int main() {
  Host h;
  // Modes 0, unsigned >3, and reinterpretation of negative signed modes:
  assert(can_begin(0,h)); assert(can_begin(4,h)); assert(can_begin(UINT32_MAX,h));
  assert(h.calls.empty());

  // Mode1 short-circuits the owner flags if owner+0x850 == 3.
  h.state=3; h.flags=2;
  assert(can_begin(1,h)); equals(h.calls,{"state"}); h.calls.clear();
  h.state=0; assert(!can_begin(1,h)); equals(h.calls,{"state","flags"}); h.calls.clear();
  h.flags=0; assert(can_begin(1,h)); equals(h.calls,{"state","flags"}); h.calls.clear();

  // Mode2 uses the related pointer and strictly greater than one metric.
  assert(!can_begin(2,h)); equals(h.calls,{"state","flags","related"}); h.calls.clear();
  h.related=true; h.related_metric=1;
  assert(!can_begin(2,h)); equals(h.calls,{"state","flags","related","metric1"}); h.calls.clear();
  h.related_metric=2;
  assert(can_begin(2,h)); equals(h.calls,{"state","flags","related","metric1"}); h.calls.clear();

  // Mode3 requires a second related-object predicate; mode2 does not.
  assert(!can_begin(3,h)); equals(h.calls,{"state","flags","related","metric1","relation"}); h.calls.clear();
  h.relation_accepts=true;
  assert(can_begin(3,h)); equals(h.calls,{"state","flags","related","metric1","relation"}); h.calls.clear();
  h.flags=2;
  assert(!can_begin(3,h)); equals(h.calls,{"state","flags"}); h.calls.clear();

  // Default/-1 argument takes the direct queue branch. Native +0x0C reset
  // happens before queueing but not if the mode gate refused.
  SentenceState s{75}; h.observed_state=&s;
  assert(dispatch(s,h,"Grunt.Engage",-1,1) == -1); // gate denied
  assert(s.queue_id==75); equals(h.calls,{"state","flags"}); h.calls.clear();
  h.flags=0;
  assert(dispatch(s,h,"Grunt.Engage",-1,1)==41);
  assert(s.queue_id==-1);
  equals(h.calls,{"state","flags","queue:Grunt.Engage"}); h.calls.clear();

  // Nondefault argument rejection returns -1 and calls original diagnostic.
  s.queue_id=80; h.owner_accepts=false;
  assert(dispatch(s,h,"Grunt.Halt",7,0)==-1);
  assert(s.queue_id==-1);
  equals(h.calls,{"allows:7","cull"}); h.calls.clear();

  // Owner update still runs even when underlying queue failed.
  h.owner_accepts=true; h.queue_result=-1;
  assert(dispatch(s,h,"Grunt.Attack",3,0)==-1);
  equals(h.calls,{"allows:3","queue:Grunt.Attack","update:3"}); h.calls.clear();
  h.queue_result=112;
  assert(dispatch(s,h,"Grunt.Attack",3,0)==112);
  equals(h.calls,{"allows:3","queue:Grunt.Attack","update:3"}); h.calls.clear();

  // Mode3 success path checks exact branch order before postqueue dispatch.
  h.state=3; h.flags=2; h.related=true; h.related_metric=3; h.relation_accepts=true;
  assert(dispatch(s,h,"Grunt.Move",-1,3)==112);
  equals(h.calls,{"state","related","metric1","relation","queue:Grunt.Move"});
}