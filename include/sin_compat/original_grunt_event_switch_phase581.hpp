#pragma once

// Independently-authored host reference for original Grunt-specific event
// routing. Entire original method SHA is authenticated separately; its opaque
// engine-dependent event bodies are NOT claimed to be reconstructed here.
#include "sin_compat/original_grunt_events_phase580.hpp"
#include <cstdint>

namespace sin_compat::grunt_event_switch_phase581 {
namespace prev = sin_compat::grunt_events_phase580;

enum class Branch : uint8_t {
  Event1B, Event4A, Event8C, Event8D, Event8F, Event90,
  Event91, Event92, Event94, GenericFallback
};

inline Branch IdentifyOriginalGruntBranch(int event_id) {
  switch (event_id) {
    case 0x1B:return Branch::Event1B;
    case 0x4A:return Branch::Event4A;
    case 0x8C:return Branch::Event8C;
    case 0x8D:return Branch::Event8D;
    case 0x8F:return Branch::Event8F;
    case 0x90:return Branch::Event90;
    case 0x91:return Branch::Event91;
    case 0x92:return Branch::Event92;
    case 0x94:return Branch::Event94;
    default:return Branch::GenericFallback;
  }
}

struct EngineHooks {
  virtual ~EngineHooks() = default;
  // Original engine random integer interface is called with (1,4).
  virtual int RandomInt(int minimum, int maximum) = 0;
  virtual void OnEvent8CAfterOptionalAlert() = 0; // 0x22350D30
  virtual void OnEvent8CGlobalState() = 0;         // 0x22049C20(global)
  virtual void OnEvent91Notify(int argument) = 0; // original 0x22049C20(0x1E)
  virtual void GenericBaseEvent(int event_id) = 0; // original 0x22050150
};

enum class Outcome {
  Event8CCompleted,
  Event91Completed,
  Event94NoOp,
  DelegatedToBaseEventHandler,
  NeedsOpaqueNativeBranch,
  UnsupportedHostRandom,
};

// The known source-order event handlers. The Grunt event 0x8C's random=1
// invokes a distinct Grunt virtual +0x590. That method independently checks
// its own strict 10-second alert deadline (authenticated in Phase580).
inline Outcome Dispatch(int event_id, EngineHooks& hooks,prev::VoiceSink& voice,
                        float now, float& alert_deadline,
                        int32_t raw_owner_0x368,int32_t& raw_owner_0xC30) {
  switch (IdentifyOriginalGruntBranch(event_id)) {
    case Branch::Event8C: {
      int draw=hooks.RandomInt(1,4);
      if (draw < 1 || draw > 4) return Outcome::UnsupportedHostRandom;
      if (draw==1) (void)prev::GoAlert(voice,now,alert_deadline);
      hooks.OnEvent8CAfterOptionalAlert();
      hooks.OnEvent8CGlobalState();
      return Outcome::Event8CCompleted;
    }
    case Branch::Event91:
      hooks.OnEvent91Notify(0x1E);
      raw_owner_0xC30=raw_owner_0x368;
      return Outcome::Event91Completed;
    case Branch::Event94:
      return Outcome::Event94NoOp;
    case Branch::GenericFallback:
      hooks.GenericBaseEvent(event_id);
      return Outcome::DelegatedToBaseEventHandler;
    default:
      // Six further verified case destinations are NOT equivalent to
      // generic fallback. Preserve that distinction until their callbacks
      // and owner mutations have complete source-level models.
      return Outcome::NeedsOpaqueNativeBranch;
  }
}
} // namespace sin_compat::grunt_event_switch_phase581