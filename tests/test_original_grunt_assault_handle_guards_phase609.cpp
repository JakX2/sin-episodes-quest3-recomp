// Original-free Phase609 Assault handle prerequisite tests.
#include "sin_compat/original_grunt_assault_handle_guards_phase609.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace assault = sin_compat::grunt_assault_handle_guards_phase609;

struct Host final : assault::Host {
  struct Expected {
    std::uint32_t index;
    std::uint32_t serial;
    bool present;
  };
  std::vector<Expected> expected;
  std::size_t calls = 0;
  bool serial_checked_entity_present(std::uint32_t index,
                                     std::uint32_t serial) override {
    assert(calls < expected.size());
    const Expected& item = expected.at(calls++);
    assert(item.index == index && item.serial == serial);
    return item.present;
  }
};

static assault::State initialized_state() {
  return {0xDEADBEEFu, 0xA5u, 0x5Au};
}
static void expect_reset(const assault::State& state) {
  assert(state.raw_plus_0x1c == 0u);
  assert(state.raw_plus_0x24 == 0u);
  assert(state.raw_plus_0x25 == 0u);
}
static void test_both_valid_preserves_state_and_call_order() {
  Host h;
  h.expected = {{0x123u, 0xabcdeu, true}, {0x001u, 0x54321u, true}};
  auto state = initialized_state();
  assert(assault::both_targets_present_or_reset(h,0xabcde123u,0x54321001u,state));
  assert(h.calls == 2);
  assert(state.raw_plus_0x1c == 0xDEADBEEFu);
  assert(state.raw_plus_0x24 == 0xA5u && state.raw_plus_0x25 == 0x5Au);
}
static void test_first_invalid_short_circuits_second() {
  Host h;
  auto state = initialized_state();
  assert(!assault::both_targets_present_or_reset(
      h,assault::kInvalidHandle,0x54321001u,state));
  assert(h.calls == 0);
  expect_reset(state);
}
static void test_second_invalid_clears_state() {
  Host h;
  h.expected = {{0x123u, 0xabcdeu, true}};
  auto state = initialized_state();
  assert(!assault::both_targets_present_or_reset(
      h,0xabcde123u,assault::kInvalidHandle,state));
  assert(h.calls == 1);
  expect_reset(state);
}
static void test_stale_first_handle_short_circuits() {
  Host h;
  h.expected = {{0x123u, 0xabcdeu, false}};
  auto state = initialized_state();
  assert(!assault::both_targets_present_or_reset(
      h,0xabcde123u,0x54321001u,state));
  assert(h.calls == 1);
  expect_reset(state);
}
static void test_missing_second_entity() {
  Host h;
  h.expected = {{0x123u, 0xabcdeu, true}, {0x001u, 0x54321u, false}};
  auto state = initialized_state();
  assert(!assault::both_targets_present_or_reset(
      h,0xabcde123u,0x54321001u,state));
  assert(h.calls == 2);
  expect_reset(state);
}
static void test_one_valid_zero_serial_handle() {
  Host h;
  h.expected = {{0x000u, 0x0u, true}, {0xfffu, 0x0u, true}};
  auto state = initialized_state();
  assert(assault::both_targets_present_or_reset(h,0x00000000u,0x00000fffu,state));
  assert(h.calls == 2);
}
int main() {
  static_assert(assault::kOriginalIndexMask == 0xfffu);
  test_both_valid_preserves_state_and_call_order();
  test_first_invalid_short_circuits_second();
  test_second_invalid_clears_state();
  test_stale_first_handle_short_circuits();
  test_missing_second_entity();
  test_one_valid_zero_serial_handle();
}
