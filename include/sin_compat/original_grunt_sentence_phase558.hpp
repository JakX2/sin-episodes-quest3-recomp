#pragma once
// Independent, original-free host contract for authenticated SiN 2006
// sentence gating/dispatch. Not original MSVC x86 ABI or live audio runtime.
#include <cstdint>
#include <string_view>

namespace sin_compat::original_grunt_sentence_phase558 {

struct ISentenceHost {
  virtual ~ISentenceHost() = default;
  // Original sentence object +0x04 points to owning NPC. These values are
  // supplied by a host, never loaded from 2006 x86 object memory here.
  virtual std::int32_t owner_state_0x850() = 0;
  virtual std::uint32_t owner_flags_0xb4() = 0;
  virtual bool has_related_0x9c4() = 0;
  virtual std::int32_t related_metric_arg1() = 0;
  virtual bool related_accepts_owner() = 0;
  virtual bool owner_accepts_argument(std::int32_t argument) = 0;
  virtual std::int32_t queue_sentence(std::string_view text) = 0;
  virtual void owner_update_argument(std::int32_t argument, std::uint32_t flags) = 0;
  virtual void report_cull() = 0;
};

struct SentenceState {
  std::int32_t queue_id = -1; // native +0x0C
};

// Original 0x220A3410 has four jump-table entries for mode 0..3, and
// permits any unsigned mode >3. Preserve original unsigned indexing:
// negative signed modes become unsigned >3 and also pass.
inline bool can_begin(std::uint32_t mode, ISentenceHost& host) {
  if (mode == 0 || mode > 3) return true;
  if (host.owner_state_0x850() != 3 &&
      (host.owner_flags_0xb4() & 0x02u) != 0u) {
    return false;
  }
  if (mode == 1) return true;
  if (!host.has_related_0x9c4()) return false;
  if (host.related_metric_arg1() <= 1) return false;
  if (mode == 2) return true;
  return host.related_accepts_owner();
}

// Original 0x220A35C0: gate -> reset +0x0C -> bypass or owner arg gate
// -> attempt to queue -> conditional owner postflag (+0x588), even on a
// queue failure. Host queue_sentence wraps separate native 0x220A34F0;
// buffering/string formatting and log behavior are *not* modeled here.
inline std::int32_t dispatch(SentenceState& state,
                             ISentenceHost& host,
                             std::string_view text,
                             std::int32_t argument,
                             std::uint32_t mode) {
  if (!can_begin(mode, host)) return -1;
  state.queue_id = -1;
  if (argument == -1) return host.queue_sentence(text);
  if (host.owner_accepts_argument(argument)) {
    const auto id = host.queue_sentence(text);
    host.owner_update_argument(argument, 0x40000000u);
    return id;
  }
  host.report_cull();
  return -1;
}

}  // namespace sin_compat::original_grunt_sentence_phase558