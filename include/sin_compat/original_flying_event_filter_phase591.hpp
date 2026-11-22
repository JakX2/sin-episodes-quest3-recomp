#pragma once
// Independently authored bounded model of the 2006 SiN flying attachment's
// original 0x2238B3C0 switch; native effects are explicit host callbacks.
#include <cstdint>
namespace sin_compat::flying_event_filter_phase591 {
constexpr uint32_t kFirstNativeEvent=0x09u;
constexpr uint32_t kLastNativeEvent=0x3Cu;
constexpr uint32_t kNativeTableEntries=52u;
enum class EventCase {
  Native09Or0A,Native28,Native2B,Native38Or3C,NoNativeCase
};
constexpr EventCase classify(uint32_t id) noexcept {
  switch(id){
   case 0x09:case 0x0A:return EventCase::Native09Or0A;
   case 0x28:return EventCase::Native28;
   case 0x2B:return EventCase::Native2B;
   case 0x38:case 0x3C:return EventCase::Native38Or3C;
   default:return EventCase::NoNativeCase;
  }
}
struct Result {
  // Returned AL from the original switch's observed terminal exit.
  bool native_boolean=false;
  // True if the native branch has additional still-opaque engine side effects.
  bool native_effects_unmodeled=false;
  // Host-only safety guard for special original branches that dereference
  // the original attachment's backreference+0xC50.
  bool needs_original_owner=false;
};
struct Host {
 virtual ~Host()=default;
 virtual void native_special_event(uint32_t event,EventCase native_case)=0;
};
inline Result dispatch(uint32_t event, bool owner_backreference_resolved, Host& host) {
 const EventCase branch=classify(event);
 if(branch==EventCase::NoNativeCase)return {false,false,false};
 if(!owner_backreference_resolved)return {false,true,true};
 host.native_special_event(event,branch);
 const bool consumed=branch!=EventCase::Native38Or3C;
 return {consumed,true,false};
}
inline bool original_event_0x54_cannot_be_consumed() noexcept {
 return classify(0x54u)==EventCase::NoNativeCase;
}
} // namespace sin_compat::flying_event_filter_phase591