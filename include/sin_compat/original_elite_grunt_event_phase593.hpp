#pragma once
// Independently authored bounded C++17 model of original 2006 SiN EliteGrunt
// +0x444 event override. Host callbacks stand for still-native engine calls.
#include <cmath>
#include <cstdint>
#include <limits>
namespace sin_compat::elite_grunt_event_phase593 {
struct Event { std::uint32_t id=0; float native_parameter=0.f; };
struct State {
  std::uint32_t serial_handle_fdc=0xffffffffu;
  std::uint8_t predicate_fd9=0;
  float previous_voice_time_99c=0.f;
};
enum class Status {
  ForwardedToInheritedGrunt,
  EliteEvent43Blocked,
  EliteEvent47Handled,
  EliteEvent03ParamNotOne,
  EliteEvent03Reset,
  EliteEvent03VoiceAction,
  UnsupportedOriginalX87,
};
struct Host {
  virtual ~Host()=default;
  virtual bool elite47_precondition_zero()=0;
  virtual std::uint32_t elite47_resolve()=0;
  virtual void elite47_apply(std::uint32_t selected)=0;
  virtual void original_timed_action(float time,unsigned mode)=0;
  // The actual 12-bit slot / high serial resolver at owner+0xFDC is native.
  virtual std::uint32_t resolved_entity_or_zero(std::uint32_t serial_handle)=0;
  // Original 0x22315320 also runs when the handle is invalid / null.
  virtual bool original_gate43(std::uint32_t resolved_entity)=0;
  virtual void inherited_grunt_event(const Event& event)=0;
  virtual bool original_event03_target()=0;
  virtual bool original_event03_target_virtual_120()=0;
  virtual bool original_event03_target_virtual_300_owner()=0;
  virtual float original_engine_now()=0;
  virtual void native_event03_voice_queue(int argument,unsigned mode)=0;
  virtual void native_event03_numeric_request(unsigned numeric)=0;
  virtual void native_reset(unsigned value)=0;
};
constexpr std::uint32_t kEvent03=0x03u,kEvent43=0x43u,kEvent47=0x47u;
constexpr float kNative47Time=1.0f,kNative03Time=2.0f;
constexpr double kNative03Cooldown=3.0;
inline Status dispatch(State& state,const Event& event,Host& host){
 if(event.id==kEvent47){
  if(host.elite47_precondition_zero()) {
   const auto selected=host.elite47_resolve();
   host.elite47_apply(selected);
  }
  host.original_timed_action(kNative47Time,0);
  return Status::EliteEvent47Handled;
 }
 if(event.id==kEvent43){
  const auto entity=state.serial_handle_fdc==0xffffffffu?0u:
    host.resolved_entity_or_zero(state.serial_handle_fdc);
  const bool permit=host.original_gate43(entity);
  state.predicate_fd9=permit?1:0;
  if(!permit)return Status::EliteEvent43Blocked;
  host.inherited_grunt_event(event);
  return Status::ForwardedToInheritedGrunt;
 }
 if(event.id==kEvent03){
  // Native temporary x87 truncation mode, restored before checks. This host
  // models finite/in-range ordinary inputs, not x87 exceptional outcomes.
  if(!std::isfinite(event.native_parameter) ||
     static_cast<double>(event.native_parameter)>=2147483648.0 ||
     static_cast<double>(event.native_parameter)<-2147483648.0)
    return Status::UnsupportedOriginalX87;
  const auto converted=static_cast<std::int32_t>(std::trunc(event.native_parameter));
  if(converted!=1)return Status::EliteEvent03ParamNotOne;
  if(host.original_event03_target() &&
     host.original_event03_target_virtual_120() &&
     !host.original_event03_target_virtual_300_owner()){
   const float now=host.original_engine_now();
   const float elapsed=now-state.previous_voice_time_99c;
   if(!std::isfinite(elapsed))return Status::UnsupportedOriginalX87;
   if(elapsed>static_cast<float>(kNative03Cooldown)){
    state.previous_voice_time_99c=now;
    host.native_event03_voice_queue(2,2u);
    host.original_timed_action(kNative03Time,0);
    host.native_event03_numeric_request(1u);
    return Status::EliteEvent03VoiceAction;
   }
  }
  host.native_reset(0);
  return Status::EliteEvent03Reset;
 }
 host.inherited_grunt_event(event);
 return Status::ForwardedToInheritedGrunt;
}
} // namespace sin_compat::elite_grunt_event_phase593