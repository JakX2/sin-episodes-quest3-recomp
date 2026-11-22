// Phase611: original-free Assassin [sic: Assault] behavior shutdown tests.
// The callbacks simulate game-owned entity serial validation, not a retail
// executable or a new Source engine entity representation.
#include "sin_compat/original_grunt_assault_deactivate_phase611.hpp"
#include <cassert>
#include <cstdint>
#include <vector>
namespace a = sin_compat::grunt_assault_deactivate_phase611;

struct Mock final : a::Host {
  std::vector<std::uint32_t> expected_resolves;
  std::vector<bool> resolve_success;
  std::vector<std::uint64_t> result_tokens;
  std::vector<std::uint32_t> link_reads;
  std::vector<std::uint64_t> expected_link_targets;
  std::size_t resolves=0,links=0,writes=0;
  std::uint64_t written_target=0;
  std::uint32_t written_value=0;
  a::State* changing_state=nullptr;
  std::uint32_t new_handle=0;
  bool resolve_entity(std::uint32_t handle,
                      std::uint64_t& token) override {
    assert(resolves<expected_resolves.size());
    assert(handle==expected_resolves[resolves]);
    if(changing_state && resolves==0) changing_state->handle_plus_0x18=new_handle;
    token=result_tokens[resolves];
    return resolve_success[resolves++];
  }
  std::uint32_t read_link_handle_plus_0x3bc(std::uint64_t target) override {
    assert(links<link_reads.size());
    assert(target==expected_link_targets[links]);
    return link_reads[links++];
  }
  void write_link_handle_plus_0x3bc(std::uint64_t target,
                                    std::uint32_t value) override {
    ++writes;written_target=target;written_value=value;
  }
};
static a::State seeded(){return {0xabcde123u,6u,7u};}
static void test_invalid_or_stale_target_safely_clears_flags() {
  {Mock h;auto s=seeded();s.handle_plus_0x18=a::kInvalidHandle;
   a::deactivate(h,s,9u);
   assert(s.byte_plus_0x24==0u&&s.byte_plus_0x25==0u);
   assert(h.resolves==0u);}
  {Mock h;auto s=seeded();
   h.expected_resolves={0xabcde123u};h.resolve_success={false};
   h.result_tokens={0u};a::deactivate(h,s,9u);
   assert(h.resolves==1u&&h.writes==0u);}
  {Mock h;auto s=seeded();
   h.expected_resolves={0xabcde123u,0xabcde123u};
   h.resolve_success={true,false};h.result_tokens={11u,0u};
   a::deactivate(h,s,9u);
   assert(h.resolves==2u&&h.writes==0u);}
}
static void test_invalid_link_cleared() {
  Mock h;auto s=seeded();
  h.expected_resolves={0xabcde123u,0xabcde123u};
  h.resolve_success={true,true};h.result_tokens={11u,11u};
  h.link_reads={a::kInvalidHandle};h.expected_link_targets={11u};
  a::deactivate(h,s,9u);
  assert(h.writes==1u&&h.written_target==11u);
  assert(h.written_value==a::kInvalidHandle);
}
static void test_stale_link_cleared() {
  Mock h;auto s=seeded();
  h.expected_resolves={0xabcde123u,0xabcde123u,0x11111111u};
  h.resolve_success={true,true,false};h.result_tokens={11u,11u,0u};
  h.link_reads={0x11111111u};h.expected_link_targets={11u};
  a::deactivate(h,s,9u);assert(h.writes==1u);
}
static void test_link_ownership_match_only() {
  for(bool match:{false,true}) {
    Mock h;auto s=seeded();
    h.expected_resolves={0xabcde123u,0xabcde123u,0x11111111u,0x11111111u};
    h.resolve_success={true,true,true,true};
    h.result_tokens={11u,11u,match?9u:8u,match?9u:8u};
    h.link_reads={0x11111111u,0x11111111u};
    h.expected_link_targets={11u,11u};
    a::deactivate(h,s,9u);
    assert(h.writes==(match?1u:0u));
  }
}
static void test_changed_link_and_second_target() {
  {Mock h;auto s=seeded();
   h.expected_resolves={0xabcde123u,0x54321001u,0x11111111u,0x22222222u};
   h.resolve_success={true,true,true,true};h.result_tokens={11u,12u,9u,8u};
   h.changing_state=&s;h.new_handle=0x54321001u;
   h.link_reads={0x11111111u,0x22222222u};
   h.expected_link_targets={12u,12u};
   a::deactivate(h,s,9u);
   assert(h.writes==0u&&h.resolves==4u);}
  {Mock h;auto s=seeded();
   h.expected_resolves={0xabcde123u};h.resolve_success={true};
   h.result_tokens={11u};h.changing_state=&s;
   h.new_handle=a::kInvalidHandle;
   a::deactivate(h,s,9u);
   assert(h.resolves==1u&&h.writes==0u);}
}
int main() {
  test_invalid_or_stale_target_safely_clears_flags();
  test_invalid_link_cleared();
  test_stale_link_cleared();
  test_link_ownership_match_only();
  test_changed_link_and_second_target();
}
