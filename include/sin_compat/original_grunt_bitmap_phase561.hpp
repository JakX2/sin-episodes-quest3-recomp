#pragma once
// Independent, original-free host model for the authenticated SiN 2006
// linked-group bitmap cache timing and bit-operator helpers.
// Not the 2006 x86 ABI, not a live engine or entity-memory representation.
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <vector>

namespace sin_compat::original_grunt_bitmap_phase561 {

inline constexpr float kNativeRefreshPeriodSeconds=30.0f;

// Original 0x220AA920 performs x87 fcomp of engine time with owner +0x74,
// checks AH & 0x41, and enters refresh only for an *ordered strict greater*
// comparison. C++ isgreater conveys this finite/NaN decision but is not a
// full emulator of native x87 exception flags and extended precision.
inline bool refresh_due(float engine_time,float deadline) {
  return std::isgreater(engine_time,deadline);
}
inline float next_deadline(float engine_time) {
  return engine_time + kNativeRefreshPeriodSeconds;
}

// The actual original native bit operators use idx>>5 and idx&31, with
// pointer+4 as the first DWORD of a bitmap record. This vector is a
// deliberately safe, variable-width host analogue; it cannot prove how
// much adjacent native memory belongs to each original record.
struct Bitmap {
  std::vector<std::uint32_t> words;
  explicit Bitmap(std::size_t count=1):words(count,0u){}
  bool can_access(int id) const {
    if (id<0) return false;
    return static_cast<std::size_t>(id/32)<words.size();
  }
  bool test(int id) const {
    if (!can_access(id))return false;
    const auto pos=static_cast<unsigned>(id);
    return (words[pos/32] & (std::uint32_t{1}<<(pos&31)))!=0;
  }
  bool set(int id) {
    if (!can_access(id))return false;
    const auto pos=static_cast<unsigned>(id);
    words[pos/32] |= std::uint32_t{1}<<(pos&31);
    return true;
  }
  bool clear(int id) {
    if (!can_access(id))return false;
    const auto pos=static_cast<unsigned>(id);
    words[pos/32] &= ~(std::uint32_t{1}<<(pos&31));
    return true;
  }
};

struct IRefreshHost {
  virtual ~IRefreshHost()=default;
  virtual void refresh_eligible_members()=0;
};

// The original +0x60/+0x6C record pool and +0x5C last-entry cache are
// controlled by 0x220AA920. This reference exposes only the independently
// proven timer gate and per-key bit operations. The host owns the actual
// entity serials/liveness and may rebuild the cache on a refresh callback.
class TimedBitmapHost {
 public:
  explicit TimedBitmapHost(IRefreshHost& host,float initial_deadline=0.0f,
                           std::size_t bits_per_record=32)
      : host_(host), deadline_(initial_deadline),
        word_count_(bits_per_record/32+(bits_per_record%32!=0?1:0)) {}

  Bitmap& access(std::uint32_t owner_id,float now) {
    if (refresh_due(now,deadline_)) {
      if (!data_.empty()) host_.refresh_eligible_members();
      // Native source writes engine time + exact 30.0f into +0x74.
      deadline_=next_deadline(now);
      ++refresh_gate_executions_;
    }
    auto it=data_.find(owner_id);
    if (it==data_.end())it=data_.emplace(owner_id,Bitmap(word_count_)).first;
    last_owner_id_=owner_id;
    return it->second;
  }

  bool set(std::uint32_t owner,int id,float now) {
    return access(owner,now).set(id);
  }
  bool clear(std::uint32_t owner,int id,float now) {
    return access(owner,now).clear(id);
  }
  bool test(std::uint32_t owner,int id,float now) {
    return access(owner,now).test(id);
  }

  // This is an independently designed host invalidation hook. Native
  // 0x220AA920 has more complex per-entity handle cleanup/compaction and
  // allocation, and **does not** simply clear all records on each tick.
  void remove_owner(std::uint32_t owner) {data_.erase(owner);}
  float deadline() const{return deadline_;}
  std::uint32_t last_owner_id() const{return last_owner_id_;}
  unsigned refresh_gate_executions() const{return refresh_gate_executions_;}
  std::size_t record_count() const{return data_.size();}
 private:
  IRefreshHost& host_;
  float deadline_;
  std::size_t word_count_;
  std::unordered_map<std::uint32_t,Bitmap> data_;
  std::uint32_t last_owner_id_=0;
  unsigned refresh_gate_executions_=0;
};

} // namespace sin_compat::original_grunt_bitmap_phase561