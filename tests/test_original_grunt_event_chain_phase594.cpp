// Phase 594: compose previously SHA-authenticated gameplay event boundaries.
// This is an original-free host regression, not a new retail DLL decompilation.
#include "sin_compat/original_jetpack_events_phase590.hpp"
#include "sin_compat/original_flying_event_filter_phase591.hpp"
#include "sin_compat/original_grunt_event_parent_phase592.hpp"
#include "sin_compat/original_elite_grunt_event_phase593.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace jp = sin_compat::jetpack_events_phase590;
namespace fly = sin_compat::flying_event_filter_phase591;
namespace grunt = sin_compat::grunt_event_parent_phase592;
namespace elite = sin_compat::elite_grunt_event_phase593;

enum class Variant { Grunt, EliteGrunt, JetpackGrunt };

// The four adapters deliberately preserve the native layer boundaries.
// The shared-NPC event 0x54 inner parameter and route-length checks are
// source-bound prior research, NOT a reimplementation of all 0x22050150.
struct Chain final : jp::Host, fly::Host, grunt::Host, elite::Host {
  std::vector<std::string> calls;
  jp::State jetpack_state{};
  elite::State elite_state{};
  grunt::Event event{};
  grunt::Result parent_result = grunt::Result::RequiresOriginalDedicatedHandler;
  bool component_present = false;
  bool component_clears_marker = false;
  bool owner_backreference_resolved = true;
  bool elite_gate43 = false;
  bool flank_requested = false;
  float route_length = 241.0f;

  void parent(std::uint32_t id) {
    calls.emplace_back("parent");
    assert(id == event.numeric_id);
    parent_result = grunt::grunt_parent_2235bfd0(event, component_present, *this);
  }

  // Original Jetpack +0x444 virtual, Phase 590.
  void native_reset(jp::State&, std::uint32_t value) override {
    assert(value == 0);
    calls.emplace_back("jetpack-reset");
  }
  void native_timed_action(float time, std::uint32_t mode) override {
    assert(time == 0.05f && mode == 0);
    calls.emplace_back("jetpack-timer");
  }
  void native_jetpack_a5_handler() override { calls.emplace_back("jetpack-a5"); }
  void original_debug_event(std::uint32_t) override { calls.emplace_back("jetpack-debug"); }
  bool native_event_filter(std::uint32_t id) override {
    calls.emplace_back("filter");
    const fly::Result r = fly::dispatch(id, owner_backreference_resolved, *this);
    assert(!r.needs_original_owner);
    return r.native_boolean;
  }
  void native_parent_grunt_event(std::uint32_t id) override { parent(id); }

  // Attached flying entity's original filter, Phase 591.
  void native_special_event(std::uint32_t, fly::EventCase) override {
    calls.emplace_back("flying-special");
  }

  // Grunt inherited +0x444 and +0xC18 optional component, Phase 592.
  void original_prefix_update(float value) override {
    assert(value == 0.75f);
    calls.emplace_back("prefix");
  }
  void unresolved_dedicated(std::uint32_t id, std::uint8_t selected) override {
    assert(selected != 24 && selected == grunt::original_case_index(id));
    calls.emplace_back("dedicated-unresolved");
  }
  void optional_component_event(const grunt::Event& received,
                                std::uint8_t& marker) override {
    assert(received.numeric_id == event.numeric_id && marker == 1);
    calls.emplace_back("component");
    if (component_clears_marker) marker = 0;
  }
  void original_special_43(const grunt::Event& received) override {
    assert(received.numeric_id == 0x43);
    calls.emplace_back("intermediary-43");
  }
  void original_shared_npc_event(const grunt::Event& received) override {
    calls.emplace_back("shared");
    if (received.numeric_id != 0x54) return;
    const double param = static_cast<double>(received.native_float_parameter);
    // Native x87 exceptional / out-of-range conversions remain unmodeled.
    assert(std::isfinite(param) && param >= -2147483648.0 &&
           param < 2147483648.0);
    const auto inner = static_cast<std::int32_t>(std::trunc(param));
    // For inner 0, the retail game has a different assault callback.
    // Only model the previously proven flank gate for inner 1.
    if (inner == 1 && route_length > 240.0f) {
      calls.emplace_back("flank-request");
      flank_requested = true;
    }
  }

  // Elite +0x444 original event override, Phase 593.
  bool elite47_precondition_zero() override {
    calls.emplace_back("elite47-precondition");
    return false;
  }
  std::uint32_t elite47_resolve() override {
    assert(false && "elite47 resolver must not be reached in these tests");
    return 0;
  }
  void elite47_apply(std::uint32_t) override {
    assert(false && "elite47 apply must not be reached in these tests");
  }
  void original_timed_action(float time, unsigned mode) override {
    assert(time == 1.0f && mode == 0);
    calls.emplace_back("elite-timer");
  }
  std::uint32_t resolved_entity_or_zero(std::uint32_t) override {
    assert(false && "invalid sentinel should skip handle lookup");
    return 0;
  }
  bool original_gate43(std::uint32_t entity) override {
    assert(entity == 0);
    calls.emplace_back("elite43-predicate");
    return elite_gate43;
  }
  void inherited_grunt_event(const elite::Event& received) override {
    calls.emplace_back("elite-parent");
    assert(received.id == event.numeric_id &&
           received.native_parameter == event.native_float_parameter);
    parent(received.id);
  }
  bool original_event03_target() override {
    assert(false && "event 0x03 target not exercised here");
    return false;
  }
  bool original_event03_target_virtual_120() override {
    assert(false && "event 0x03 virtual not exercised here");
    return false;
  }
  bool original_event03_target_virtual_300_owner() override {
    assert(false && "event 0x03 virtual not exercised here");
    return false;
  }
  float original_engine_now() override {
    assert(false && "event 0x03 clock not exercised here");
    return 0.f;
  }
  void native_event03_voice_queue(int, unsigned) override {
    assert(false && "event 0x03 voice not exercised here");
  }
  void native_event03_numeric_request(unsigned) override {
    assert(false && "event 0x03 request not exercised here");
  }
  void native_reset(unsigned) override {
    assert(false && "event 0x03 reset not exercised here");
  }

  void run(Variant kind, std::uint32_t id, float parameter = 1.0f) {
    event = {id, parameter};
    switch (kind) {
      case Variant::Grunt: parent(id); break;
      case Variant::EliteGrunt: {
        const elite::Status result =
            elite::dispatch(elite_state, {id, parameter}, *this);
        if (id == 0x54) assert(result == elite::Status::ForwardedToInheritedGrunt);
        break;
      }
      case Variant::JetpackGrunt: {
        const jp::Status result = jp::dispatch(jetpack_state, id, *this);
        if (id == 0x54) assert(result == jp::Status::DelegatedToBase);
        break;
      }
    }
  }
};

static std::vector<std::string> expected_prefix(Variant v) {
  switch (v) {
    case Variant::Grunt: return {"parent", "prefix"};
    case Variant::EliteGrunt: return {"elite-parent", "parent", "prefix"};
    case Variant::JetpackGrunt: return {"filter", "parent", "prefix"};
  }
  assert(false && "invalid variant");
  return {};
}
static void expect_calls(const Chain& c, std::vector<std::string> expected) {
  assert(c.calls == expected);
}
static void test_event54_three_subclasses_and_component() {
  for (Variant v : {Variant::Grunt, Variant::EliteGrunt,
                    Variant::JetpackGrunt}) {
    // Null component: shared NPC always receives the event.
    Chain none;
    none.owner_backreference_resolved = false; // 0x54 filter is default
    none.run(v, 0x54);
    auto a = expected_prefix(v);
    a.insert(a.end(), {"shared", "flank-request"});
    expect_calls(none, a);
    assert(none.parent_result == grunt::Result::DelegatedToSharedNPCEvent);
    assert(none.flank_requested);

    // Present component retaining byte +0x08: 0x54 never reaches shared NPC.
    Chain consumed;
    consumed.component_present = true;
    consumed.run(v, 0x54);
    a = expected_prefix(v);
    a.push_back("component");
    expect_calls(consumed, a);
    assert(consumed.parent_result == grunt::Result::ConsumedByOptionalComponent);
    assert(!consumed.flank_requested);

    // Present component clearing byte +0x08: base dispatch is restored.
    Chain cleared;
    cleared.component_present = true;
    cleared.component_clears_marker = true;
    cleared.run(v, 0x54);
    a = expected_prefix(v);
    a.insert(a.end(), {"component", "shared", "flank-request"});
    expect_calls(cleared, a);
    assert(cleared.flank_requested);
  }
}
static void test_inner_parameter_and_strict_flank_threshold() {
  for (Variant v : {Variant::Grunt, Variant::EliteGrunt,
                    Variant::JetpackGrunt}) {
    for (float parameter : {0.f, -1.f, 2.f, 0.99f}) {
      Chain c;
      c.run(v, 0x54, parameter);
      auto a = expected_prefix(v);
      a.push_back("shared");
      expect_calls(c, a);
      assert(!c.flank_requested);
    }
    for (float length : {239.0f, 240.0f}) {
      Chain c;
      c.route_length = length;
      c.run(v, 0x54, 1.99f); // x87 truncate makes inner integer 1
      auto a = expected_prefix(v);
      a.push_back("shared");
      expect_calls(c, a);
      assert(!c.flank_requested);
    }
    Chain beyond;
    beyond.route_length = 240.01f;
    beyond.run(v, 0x54, 1.99f);
    assert(beyond.flank_requested);
  }
}
static void test_class_specific_nonflank_boundaries() {
  {
    Chain c;
    c.run(Variant::EliteGrunt, 0x47);
    expect_calls(c, {"elite47-precondition", "elite-timer"});
  }
  {
    Chain c;
    c.run(Variant::EliteGrunt, 0x43);
    expect_calls(c, {"elite43-predicate"}); // false predicate stops inheritance
    c.calls.clear();
    c.elite_gate43 = true;
    c.run(Variant::EliteGrunt, 0x43);
    expect_calls(c, {"elite43-predicate", "elite-parent", "parent",
                     "prefix", "dedicated-unresolved"});
    // Full Grunt 0x43 is dedicated, not its intermediary 0x43 shortcut.
  }
  {
    Chain c;
    c.run(Variant::JetpackGrunt, 0x09);
    expect_calls(c, {"filter", "flying-special"}); // flying predicate consumes
  }
  {
    Chain c;
    c.run(Variant::JetpackGrunt, 0x9f);
    expect_calls(c, {"jetpack-reset"});
    assert(c.jetpack_state.raw_fec == 0u);
  }
  {
    Chain c;
    c.run(Variant::JetpackGrunt, 0xa4);
    expect_calls(c, {"jetpack-timer"});
  }
  {
    Chain c;
    c.run(Variant::JetpackGrunt, 0x38);
    assert(c.calls.empty()); // native specialized attachment branch unresolved
  }
}
int main() {
  static_assert(jp::classify(0x54) == jp::DispatchClass::OriginalFallback);
  static_assert(fly::classify(0x54) == fly::EventCase::NoNativeCase);
  static_assert(grunt::original_case_index(0x54) == 24u);
  static_assert(grunt::original_case_index(0x43) == 3u);
  test_event54_three_subclasses_and_component();
  test_inner_parameter_and_strict_flank_threshold();
  test_class_specific_nonflank_boundaries();
}
