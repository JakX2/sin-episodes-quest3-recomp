#pragma once
// Original-free C++17 *host* models for source-authenticated SiN 2006
// linked-group gate and filtered member iteration. This header intentionally
// does not impersonate the original MSVC x86 class layouts or Source ABI.
#include <cstdint>
#include <limits>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace sin_compat::original_grunt_group_phase560 {

struct Member {
  std::uint32_t native_handle = 0xFFFFFFFFu;
  bool serial_resolves = false; // host-verified handle generation and object
  bool state_0x204_nonzero = false;
  bool component_virtual_0x2c_allows = false;
  bool virtual_0x694_rejects = false;
  bool field_0x94_nonzero = false;
  bool spawnflag_0x800 = false;
  std::int32_t metric_scalar_0x0c = 0;
  bool is_owner = false;
};

// Native 0x220A9300/0x220A93C0: a filter flag controls whether the
// state+0x204, component+0x2C, and virtual+0x694 gates are evaluated.
// The original also resolves each entity handle using a native table. The
// host explicitly supplies the already serial-checked outcome above.
inline bool eligible(const Member& m) {
  return m.serial_resolves &&
         (m.state_0x204_nonzero || m.component_virtual_0x2c_allows) &&
         !m.virtual_0x694_rejects;
}

class Members {
 public:
  explicit Members(std::vector<Member> members) : entries_(std::move(members)) {}
  const std::vector<Member>& all() const { return entries_; }

  struct Cursor { std::size_t index = 0; };
  const Member* first(Cursor& cursor, bool filter = true) const {
    cursor.index = 0;
    return advance(cursor, filter);
  }
  const Member* next(Cursor& cursor, bool filter = true) const {
    if (cursor.index >= entries_.size()) return nullptr;
    ++cursor.index;
    return advance(cursor, filter);
  }

  // Native 0x22054E50, after the owner's conversion helper produces an
  // opaque scalar. This model preserves repeated entity lookups and count,
  // but not its native conversion ABI or double-call accessor effects.
  std::uint32_t count_matching(std::int32_t key) const {
    Cursor c;
    std::uint32_t count = 0;
    for (auto* m = first(c); m; m = next(c)) {
      if (m->metric_scalar_0x0c == key) ++count;
    }
    return count;
  }

 private:
  const Member* advance(Cursor& cursor, bool filter) const {
    while (cursor.index < entries_.size()) {
      const auto& m=entries_[cursor.index];
      if (!filter || eligible(m)) {
        return m.serial_resolves ? &m : nullptr;
      }
      ++cursor.index;
    }
    return nullptr;
  }
  std::vector<Member> entries_;
};

// Native 0x22054D80; the original checks the THIRD stack parameter's
// +0x3ac field only when owner +0x9d0 is nonzero. The host models pointer
// identity as a finite integer token rather than a retail raw pointer.
inline bool optional_identity_guard(bool has_identity, std::uint32_t owner_identity,
                                    bool candidate_present, std::uint32_t candidate_identity) {
  return !has_identity || (candidate_present && owner_identity == candidate_identity);
}

// Native 0x22054D40, called from the original Grunt decision tree with
// condition selectors 0x0c/0x0e. No-group path returns 1. With a group,
// the native +0x154 virtual prepares a selection value and 0x220AAEB0 checks
// its bitmap range/selection. Numeric arguments remain deliberately opaque.
struct IRestrictionHost {
  virtual ~IRestrictionHost() = default;
  virtual std::int32_t owner_virtual_0x154(std::int32_t condition_a,
                                           std::int32_t condition_b,
                                           std::int32_t& selected_index) = 0;
  virtual bool linked_group_0x220AAEB0(std::int32_t category,
                                        std::int32_t begin, std::int32_t end,
                                        std::int32_t& selected_index) = 0;
};

inline bool permits_condition(bool has_group, std::int32_t condition,
                              std::int32_t& chosen_index,
                              IRestrictionHost& host) {
  if (!has_group) return true;
  const auto selector=host.owner_virtual_0x154(condition,condition,chosen_index);
  return host.linked_group_0x220AAEB0(selector,condition,condition,chosen_index);
}

// Independent synthetic representation of the bounded *normal path* in
// original 0x220AAEB0. A live bitset is obtained by native 0x220AA920,
// whose timer/cache rebuilds are NOT simulated here. The model instead
// accepts the host's current bitmap for the requested category.
class Reservations {
 public:
  bool occupied(std::int32_t category,std::int32_t idx) const {
    auto it = per_category_.find(category);
    return it != per_category_.end() && it->second.count(idx) != 0;
  }
  void occupy(std::int32_t category,std::int32_t idx) {
    if (idx>=0) per_category_[category].insert(idx);
  }
  void release(std::int32_t category,std::int32_t idx) {
    per_category_[category].erase(idx);
  }
  // An original selected index already *within* the inclusive range is
  // admitted immediately. If not, the first unoccupied index is chosen.
  // The host refuses negative/over-large indices to avoid unchecked shifts.
  bool admit(std::int32_t category,std::int32_t begin,std::int32_t end,
             std::int32_t& selected) {
    if (begin<0 || end<begin || end==std::numeric_limits<std::int32_t>::max())
      return false; // host-only safety, not original malformed-input parity
    if (begin<=selected && selected<=end) return true;
    for (std::int64_t idx=begin;idx<=end;++idx) {
      const auto i=static_cast<std::int32_t>(idx);
      if (!occupied(category,i)) {
        if (selected!=-1) {
          // Original emits a diagnostic if this old bit is unexpectedly
          // absent, then clears it anyway.
          if (!occupied(category,selected)) ++missing_old_bit_notifications_;
          release(category,selected);
        }
        occupy(category,i);
        selected=i;
        return true;
      }
    }
    return false;
  }
  unsigned missing_old_bit_notifications() const {return missing_old_bit_notifications_;}
 private:
  std::unordered_map<std::int32_t,std::set<std::int32_t>> per_category_;
  unsigned missing_old_bit_notifications_ = 0;
};

struct IGroupLifecycle {
  virtual ~IGroupLifecycle()=default;
  virtual std::uint32_t find_group(std::uint32_t name)=0;
  virtual std::uint32_t create_group(std::uint32_t name)=0;
  virtual void attach_to_owner(std::uint32_t group)=0;
  virtual void detach_from_owner(std::uint32_t group)=0;
};

// Native 0x22054CE0 does a lookup; on a missing result calls a create-like
// helper then attaches. The named host operations are not native vtable ABI.
inline void acquire_group(std::uint32_t name,IGroupLifecycle& host) {
  auto group=host.find_group(name);
  if (!group) group=host.create_group(name);
  host.attach_to_owner(group);
}
inline void clear_group(std::uint32_t& group,IGroupLifecycle& host) {
  if (group) host.detach_from_owner(group);
  group=0;
}

struct IDispatchHost {
  virtual ~IDispatchHost()=default;
  virtual void activate_special_entity(const Member& member)=0;
  virtual bool dispatch_candidate(const Member& member)=0;
};

// Native 0x22054DB0 loops over first/next filtered members. It only
// dispatches to distinct, +0x94-enabled members, fires a special 0x800
// flag hook before dispatch, and returns at the first successful callback.
inline bool dispatch_linked(const Members* group,IDispatchHost& host) {
  if (!group) return false;
  Members::Cursor c;
  for (const Member* m=group->first(c,true);m;m=group->next(c,true)) {
    if (!m->field_0x94_nonzero || m->is_owner) continue;
    if (m->spawnflag_0x800) host.activate_special_entity(*m);
    if (host.dispatch_candidate(*m)) return true;
  }
  return false;
}

// Native 0x22054E50 returns 0/1 for the owner-only fallback when no group,
// but counts all matching eligible linked members once a group exists.
inline std::uint32_t count_metric_matches(const Members* group,
                                          bool owner_metric_object_exists,
                                          std::int32_t owner_metric,
                                          std::int32_t requested) {
  if (!group) return owner_metric_object_exists && owner_metric==requested ? 1u : 0u;
  return group->count_matching(requested);
}
} // namespace sin_compat::original_grunt_group_phase560