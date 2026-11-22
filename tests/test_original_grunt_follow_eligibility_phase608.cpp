// Phase608: original-free host regression for serial-checked Grunt Follow AI.
// No original DLL, entity memory or Source engine integration required.
#include "sin_compat/original_grunt_follow_eligibility_phase608.hpp"
#include <cassert>
#include <cstdint>
#include <utility>
#include <vector>

namespace follow = sin_compat::grunt_follow_eligibility_phase608;

struct RecordingHost final : follow::Host {
  bool gate = true;
  std::vector<std::uint32_t> handle_reads;
  std::size_t read_index = 0;
  struct Resolution {
    std::uint32_t expected_index;
    std::uint32_t expected_serial;
    bool valid;
    std::uint32_t flags;
  };
  std::vector<Resolution> resolutions;
  std::size_t resolution_index = 0;
  unsigned owner_gate_calls = 0;

  bool owner_gate() override {
    ++owner_gate_calls;
    return gate;
  }
  std::uint32_t read_encoded_handle() override {
    assert(read_index < handle_reads.size());
    return handle_reads.at(read_index++);
  }
  bool resolve_flags(std::uint32_t index, std::uint32_t serial,
                     std::uint32_t& result) override {
    assert(resolution_index < resolutions.size());
    const Resolution& expected = resolutions.at(resolution_index++);
    assert(index == expected.expected_index);
    assert(serial == expected.expected_serial);
    result = expected.flags;
    return expected.valid;
  }
};

static void test_owner_gate_refuses_before_any_handle_lookup() {
  RecordingHost h;
  h.gate = false;
  assert(!follow::eligible(h));
  assert(h.owner_gate_calls == 1);
  assert(h.read_index == 0 && h.resolution_index == 0);
}
static void test_invalid_handle_short_circuits() {
  RecordingHost h;
  h.handle_reads = {follow::kInvalidOriginalHandle};
  assert(!follow::eligible(h));
  assert(h.read_index == 1 && h.resolution_index == 0);
}
static void test_first_missing_or_serial_stale() {
  RecordingHost h;
  h.handle_reads = {0xabcde123u};
  h.resolutions = {{0x123u, 0xabcdeu, false, 0}};
  assert(!follow::eligible(h));
  assert(h.read_index == 1 && h.resolution_index == 1);
}
static void test_repeated_handle_lookup_and_bit15_clear() {
  RecordingHost h;
  h.handle_reads = {0xabcde123u, 0xabcde123u};
  h.resolutions = {{0x123u, 0xabcdeu, true, 0xffffffffu},
                   {0x123u, 0xabcdeu, true, 0x00007fffu}};
  assert(follow::eligible(h));
  assert(h.read_index == 2 && h.resolution_index == 2);
}
static void test_flag_bit15_disqualifies() {
  RecordingHost h;
  h.handle_reads = {0xabcde123u, 0xabcde123u};
  h.resolutions = {{0x123u, 0xabcdeu, true, 0},
                   {0x123u, 0xabcdeu, true, 0x8000u}};
  assert(!follow::eligible(h));
  assert(h.read_index == 2 && h.resolution_index == 2);
}
static void test_changed_handle_second_lookup_safe() {
  RecordingHost h;
  h.handle_reads = {0xabcde123u, 0x54321001u};
  h.resolutions = {{0x123u, 0xabcdeu, true, 0},
                   {0x001u, 0x54321u, false, 0}};
  assert(!follow::eligible(h));
  assert(h.read_index == 2 && h.resolution_index == 2);
  // Native 2006 can dereference [0xBC] after a failed second serial
  // resolution; our independent safe host does not reproduce a crash.
}
static void test_second_handle_becomes_invalid_safe() {
  RecordingHost h;
  h.handle_reads = {0xabcde123u, follow::kInvalidOriginalHandle};
  h.resolutions = {{0x123u, 0xabcdeu, true, 0}};
  assert(!follow::eligible(h));
  assert(h.read_index == 2 && h.resolution_index == 1);
}
static void test_second_resolve_uses_second_handle() {
  RecordingHost h;
  h.handle_reads = {0xabcde123u, 0x54321001u};
  h.resolutions = {{0x123u, 0xabcdeu, true, 0x8000u},
                   {0x001u, 0x54321u, true, 0}};
  assert(follow::eligible(h));
  assert(h.resolution_index == 2);
}
int main() {
  static_assert(follow::kIndexMask == 0xfffu);
  static_assert(follow::kDisqualifyingEntityFlag == 0x8000u);
  test_owner_gate_refuses_before_any_handle_lookup();
  test_invalid_handle_short_circuits();
  test_first_missing_or_serial_stale();
  test_repeated_handle_lookup_and_bit15_clear();
  test_flag_bit15_disqualifies();
  test_changed_handle_second_lookup_safe();
  test_second_handle_becomes_invalid_safe();
  test_second_resolve_uses_second_handle();
}
