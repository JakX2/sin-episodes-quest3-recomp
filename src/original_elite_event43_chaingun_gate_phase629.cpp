#include "sin_compat/original_elite_event43_chaingun_gate_phase629.hpp"
namespace sin_compat::original_elite_event43_chaingun_gate_phase629 {
namespace {
struct Guard {
  bool& busy;
  explicit Guard(bool& x)noexcept:busy(x){busy=true;}
  ~Guard()noexcept{busy=false;}
};
// The original uses x87 FLD/FSub/FComp then tests the C0 unordered-or-less
// bit. Thus the state advances only when elapsed >= original 1.0f.
// Double is sufficient for normal single-precision game clock values;
// exceptional x87 status/FP control modes are outside this portable model.
bool elapsed_one_second(float now,float since)noexcept {
  return static_cast<double>(now)-static_cast<double>(since)>=1.0;
}
} // namespace
Result Reference::gate(State& state,bool valid_native_target)noexcept {
  // The original x86 function directly reads [ECX+0x594] with NO null
  // protection. The exact class of the weapon-linked handle's resolved
  // referent is unproven. Fail closed on an absent resolved entity rather
  // than dereference null or fabricate a Source object.
  if(!valid_native_target)return {Code::RejectedNullTarget,Stage::None};
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Stage stage=Stage::None;
  try {
    switch(state.original_phase_594) {
      case 0u: {
        stage=Stage::GateVirtual3C4Command13;
        if(!host_.native_virtual_3c4(13u,0u))
          return {Code::HostRejected,stage};
        state.original_phase_594=1u;
        // Retail method reads the engine-global clock twice and stores
        // two independent floats. It does not reuse the first value.
        stage=Stage::GateTime590;
        if(!host_.read_original_engine_time(state.original_time_590))
          return {Code::HostRejected,stage};
        stage=Stage::GateTime58C;
        if(!host_.read_original_engine_time(state.original_time_58c))
          return {Code::HostRejected,stage};
        return {Code::Blocked,Stage::None};
      }
      case 2u: return {Code::Allowed,Stage::None};
      case 1u: // fallthrough
      case 3u: // fallthrough
      default: return {Code::Blocked,Stage::None};
    }
  } catch(...) {
    return {Code::HostRejected,stage};
  }
}
Result Reference::update(State& state,bool valid_native_target)noexcept {
  if(!valid_native_target)return {Code::RejectedNullTarget,Stage::None};
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Stage stage=Stage::None;
  try {
    switch(state.original_phase_594) {
      case 1u: {
        float now=0.f;
        stage=Stage::UpdateReadTime;
        if(!host_.read_original_engine_time(now))
          return {Code::HostRejected,stage};
        if(!elapsed_one_second(now,state.original_time_590))
          return {Code::Unchanged,Stage::None};
        state.original_phase_594=2u;
        // The native code performs another pair of independent clock
        // reads, NOT a reuse of the comparison's 'now' value.
        stage=Stage::UpdateTime590;
        if(!host_.read_original_engine_time(state.original_time_590))
          return {Code::HostRejected,stage};
        stage=Stage::UpdateTime58C;
        if(!host_.read_original_engine_time(state.original_time_58c))
          return {Code::HostRejected,stage};
        return {Code::Advanced,Stage::None};
      }
      case 2u: {
        float now=0.f;
        stage=Stage::UpdateReadTime;
        if(!host_.read_original_engine_time(now))
          return {Code::HostRejected,stage};
        if(!elapsed_one_second(now,state.original_time_58c))
          return {Code::Unchanged,Stage::None};
        state.original_phase_594=3u;
        return {Code::Advanced,Stage::None};
      }
      case 3u:
        stage=Stage::UpdateVirtual3C8Command13;
        if(!host_.native_virtual_3c8(13u))
          return {Code::HostRejected,stage};
        stage=Stage::UpdateVirtual3C4Command14;
        if(!host_.native_virtual_3c4(14u,0u))
          return {Code::HostRejected,stage};
        state.original_phase_594=0u;
        return {Code::Advanced,Stage::None};
      case 0u:
      default:
        return {Code::Unchanged,Stage::None};
    }
  } catch(...) {
    return {Code::HostRejected,stage};
  }
}
} // namespace sin_compat::original_elite_event43_chaingun_gate_phase629
