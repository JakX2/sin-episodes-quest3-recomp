#pragma once

// Independently authored, original-code-free reference for observable decisions.
// Native owner offsets, original class/vtable ABI, game engine sound processing,
// and x87 exceptional conversions are NOT implemented here.
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>

namespace sin_compat::grunt_events_phase580 {

struct VoiceSink {
  virtual ~VoiceSink() = default;
  // Returns native-style sentence queue status: negative = not queued.
  virtual int Queue(std::string_view token, int argument, int mode) = 0;
  // Corresponds to original engine random provider requested with 5f and 15f.
  virtual float RandomFloat(float lower, float upper) = 0;
};

struct ExistingEventSink {
  virtual ~ExistingEventSink() = default;
  virtual void PainOrTauntEvent() = 0;   // original Grunt vtable +0x598
  virtual void DeathSentenceEvent() = 0; // original Grunt vtable +0x58C
  virtual void AssaultSentenceEvent() = 0; // original Grunt event parameter 0
  virtual void FlankPathLengthEvent() = 0; // original Grunt event parameter 1
};

enum class DispatchResult {
  Handled,
  DelegatedPreviouslyAuthenticatedHandler,
  Ignored,
  NeedsNativeX87Conversion,
};

// Native 0x223589C0, vtable+0x594, original animation event 0x50.
inline int Announce(VoiceSink& sink) {
  return sink.Queue("GRUNT_ANNOUNCE", 0, 2);
}

// Native 0x22358950, vtable+0x5A4, called from original 0x2203056C.
inline int RefindEnemy(VoiceSink& sink) {
  return sink.Queue("GRUNT_REFIND_ENEMY", 2, 2);
}

// Native 0x22358970, vtable+0x590, original animation event 0x51.
// Native x87 comparison accepts ordered strictly later; this safe host model
// restricts to finite floats to avoid asserting parity for NaN/Inf payloads.
inline bool GoAlert(VoiceSink& sink, float now, float& next_deadline) {
  if (!std::isfinite(now) || !std::isfinite(next_deadline)) return false;
  if (!(now > next_deadline)) return false;
  (void)sink.Queue("GRUNT_GO_ALERT", 2, 2);
  next_deadline = now + 10.0f; // updated even if queue fails
  return true;
}

// Original native code uses `test ah,0x41; jnp skip`. An x87 status AH
// containing 00 or 0x41 has EVEN parity after the AND, so it *enters* the
// speech path (the second case is the x87 unordered encoding). These helpers
// accept raw provided status flags, without pretending to emulate a 2006 FPU.
inline bool LostAttemptAllowedByX87(uint8_t status_ah) {
  const uint8_t masked = static_cast<uint8_t>(status_ah & 0x41u);
  return masked == 0x00u || masked == 0x41u;
}
inline bool ElapsedStrictlyGreaterThanTenByX87(uint8_t status_ah) {
  return (status_ah & 0x41u) == 0;
}

enum class LostOutcome {
  CooldownBlocked,
  SentenceRejected,
  QueuedShort,
  QueuedLong,
  UnsupportedHostRandom,
};

struct LostInput {
  // x87 AH after comparing original engine time with owner +0xC88.
  uint8_t deadline_compare_ah = 0;
  // x87 AH after comparing `engine time - last seen` with 10.0f.
  uint8_t elapsed_compare_ah = 0;
  bool resolved_target = false;
  // The original reads engine time again *after* a successful queue/random.
  float now_after_queue = 0;
};

// Native 0x223588B0, vtable+0x5A0. This code does not infer whether an
// engine queue success actually made the audio audible.
inline LostOutcome LostEnemy(VoiceSink& sink, const LostInput& in,
                             float& next_deadline) {
  if (!LostAttemptAllowedByX87(in.deadline_compare_ah))
    return LostOutcome::CooldownBlocked;
  const bool long_line = !in.resolved_target ||
      ElapsedStrictlyGreaterThanTenByX87(in.elapsed_compare_ah);
  const int queue_status = sink.Queue(long_line ? "GRUNT_LOST_LONG" : "GRUNT_LOST_SHORT", 0, 2);
  if (queue_status < 0) return LostOutcome::SentenceRejected;
  const float delay = sink.RandomFloat(5.0f, 15.0f);
  if (!std::isfinite(delay) || delay < 5.0f || delay > 15.0f ||
      !std::isfinite(in.now_after_queue)) return LostOutcome::UnsupportedHostRandom;
  next_deadline = in.now_after_queue + delay;
  return long_line ? LostOutcome::QueuedLong : LostOutcome::QueuedShort;
}

// Native 0x2205259D converts a float stored at event+4 via temporary x87
// control word set to truncation and restores the original control word.
// Host behavior is defined only for finite float values representable as i32.
inline bool TruncateFloatArgument(float raw, int32_t& result) {
  if (!std::isfinite(raw)) return false;
  const double t = std::trunc(static_cast<double>(raw));
  if (t < static_cast<double>(std::numeric_limits<int32_t>::min()) ||
      t > static_cast<double>(std::numeric_limits<int32_t>::max())) return false;
  result = static_cast<int32_t>(t);
  return true;
}

// Original full switch source is large and not counted as a new method. This
// covers ONLY original event ids 0x50..0x54 and the known Grunt vtable calls.
inline DispatchResult DispatchOriginalEvent(int event_id, float raw_parameter,
                                            float now, float& alert_deadline,
                                            VoiceSink& voice,
                                            ExistingEventSink& existing) {
  switch (event_id) {
    case 0x50:
      (void)Announce(voice);
      return DispatchResult::Handled;
    case 0x51:
      (void)GoAlert(voice, now, alert_deadline);
      return DispatchResult::Handled;
    case 0x52:
      existing.PainOrTauntEvent();
      return DispatchResult::DelegatedPreviouslyAuthenticatedHandler;
    case 0x53:
      existing.DeathSentenceEvent();
      return DispatchResult::DelegatedPreviouslyAuthenticatedHandler;
    case 0x54: {
      int32_t inner = 0;
      if (!TruncateFloatArgument(raw_parameter, inner))
        return DispatchResult::NeedsNativeX87Conversion;
      if (inner == 0) existing.AssaultSentenceEvent();
      else if (inner == 1) existing.FlankPathLengthEvent();
      else return DispatchResult::Ignored;
      return DispatchResult::DelegatedPreviouslyAuthenticatedHandler;
    }
    default: return DispatchResult::Ignored;
  }
}

} // namespace sin_compat::grunt_events_phase580