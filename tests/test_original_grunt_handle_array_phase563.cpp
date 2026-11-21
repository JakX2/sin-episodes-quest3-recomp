#include "sin_compat/original_grunt_handle_array_phase563.hpp"
#include <cassert>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace sin_compat::original_grunt_handle_array_phase563;

struct Host : IHandleHost {
  std::map<std::uint32_t, std::uintptr_t> entities;
  std::vector<std::pair<std::uintptr_t, std::uint32_t>> writes;
  std::uintptr_t resolve(std::uint32_t handle) const override {
    if (handle == invalid_handle) return 0;
    auto it = entities.find(handle);
    return it == entities.end() ? 0 : it->second;
  }
  void or_member_flag(std::uintptr_t entity, std::uint32_t mask) override {
    writes.emplace_back(entity, mask);
  }
};

template<class E, class F> bool throws(F&& fn) {
  try { fn(); } catch(const E&) { return true; }
  return false;
}

int main() {
  Host h;
  h.entities = {{1, 0x100}, {2, 0x200}, {3, 0x300}, {4, 0x200}, {5, 0x500}};
  FixedGroupHandles group;
  assert(append(group,1) == 0);
  assert(append(group,2) == 1);
  assert(append(group,4) == 2);
  assert(append(group,3) == 3);
  assert(group.count == 4);
  assert(find_by_resolved_identity(group,4,h) == 1); // pointer aliases slot #1
  assert(find_by_resolved_identity(group,1,h) == 0);
  assert(find_by_resolved_identity(group,5,h) == -1);
  assert(!points_to_other(h, 4, 0x200));
  assert(points_to_other(h, 4, 0x100));
  assert(!points_to_other(h, invalid_handle, 0));
  assert(points_to_other(h, invalid_handle, 0x100));

  // Original iteration does not deduplicate entity identities.
  broadcast_or_flags(group, 0x04, h);
  assert((h.writes == std::vector<std::pair<std::uintptr_t,std::uint32_t>>{
    {0x100,4}, {0x200,4}, {0x200,4}, {0x300,4}}));
  h.writes.clear();

  erase_ordered(group,1);
  assert(group.count == 3);
  assert(group.slots[0] == 1 && group.slots[1] == 4 && group.slots[2] == 3);
  assert(group.slots[3] == 3); // spare slot remains untouched, as in retail
  assert(find_by_resolved_identity(group,2,h) == 1);
  assert(h.writes.empty()); // the group-remove helper does NOT notify anyone
  broadcast_or_flags(group, 8, h);
  assert((h.writes == std::vector<std::pair<std::uintptr_t,std::uint32_t>>{
    {0x100,8},{0x200,8},{0x300,8}}));

  // A stale or missing candidate resolves to null. The native search can
  // match another stale member because it compares resolved POINTERS.
  FixedGroupHandles stale;
  append(stale, 1);
  append(stale, 0x1234);
  append(stale, invalid_handle);
  assert(find_by_resolved_identity(stale, 0x4567, h) == 1);
  assert(find_by_resolved_identity(stale, invalid_handle, h) == 1);
  h.writes.clear();
  broadcast_or_flags(stale, 16,h);
  assert((h.writes == std::vector<std::pair<std::uintptr_t,std::uint32_t>>{{0x100,16}}));

  // Native append itself does NOT check capacity; the caller has an exact
  // 16-member branch. Our reference adds a host-only guard.
  FixedGroupHandles full;
  for (std::uint32_t i=0; i<capacity; ++i) assert(append(full,i)==i);
  assert(throws<std::length_error>([&]{append(full,123); }));
  assert(full.count==16);
  erase_ordered(full,15); // retail group-join last-member forced eviction
  assert(full.count==15);
  assert(append(full,123)==15);
  assert(full.count==16 && full.slots[15]==123);
  assert(throws<std::out_of_range>([&]{erase_ordered(full,16); }));

  // Duplicate raw handle insertion is also not prevented by append.
  FixedGroupHandles duplicates;
  append(duplicates,1);
  append(duplicates,1);
  assert(duplicates.count==2);
  assert(find_by_resolved_identity(duplicates,1,h)==0);
  assert(throws<std::out_of_range>([&]{FixedGroupHandles empty;erase_ordered(empty,0); }));
}