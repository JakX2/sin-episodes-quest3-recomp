#pragma once
// Independent C++17 partial reference of original SiN 2006 Jetpack Grunt
// virtual +0x444 animation-event switch. No commercial binary/source content.
#include <cstdint>
#include <string_view>

namespace sin_compat::jetpack_events_phase590 {
constexpr uint32_t kNativeFirstEvent=0x19;
constexpr uint32_t kNativeLastEvent=0xA6;
constexpr uint32_t kNativeOriginalTableEntries=142;
constexpr uint32_t kNativeDefaultInRangeEntries=133;

// Based on all 142 SHA-authenticated original byte-index table entries.
enum class DispatchClass {
  JetpackNavigation19, JetpackAttachment38Or3C,
  JetpackState9F,JetpackStateA0,JetpackDebugA3,
  JetpackTimerA4,JetpackResetA5,JetpackTaskA6,
  OriginalFallback,
};
constexpr DispatchClass classify(uint32_t id) noexcept {
  switch(id) {
    case 0x19:return DispatchClass::JetpackNavigation19;
    case 0x38:case 0x3C:return DispatchClass::JetpackAttachment38Or3C;
    case 0x9F:return DispatchClass::JetpackState9F;
    case 0xA0:return DispatchClass::JetpackStateA0;
    case 0xA3:return DispatchClass::JetpackDebugA3;
    case 0xA4:return DispatchClass::JetpackTimerA4;
    case 0xA5:return DispatchClass::JetpackResetA5;
    case 0xA6:return DispatchClass::JetpackTaskA6;
    default:return DispatchClass::OriginalFallback;
  }
}
struct State {uint32_t raw_fec=0xFFFFFFFFu;};
enum class Status {ModeledNativeBranch, DelegatedToBase, ConsumedByOriginalFilter, RequiresOriginalJetpackBranch};
struct Host {
  virtual ~Host()=default;
  virtual void native_reset(State& state,uint32_t requested)=0;
  virtual void native_timed_action(float native_time,uint32_t mode)=0;
  virtual void native_jetpack_a5_handler()=0;
  virtual void original_debug_event(uint32_t id)=0;
  virtual bool native_event_filter(uint32_t id)=0;
  virtual void native_parent_grunt_event(uint32_t id)=0;
};
inline Status dispatch(State& state,uint32_t event_id,Host& host,bool original_debug_enabled=false) {
  switch(classify(event_id)) {
  case DispatchClass::JetpackState9F:
    state.raw_fec=0; // original write occurs before 0x2203D2D0(this,0)
    host.native_reset(state,0);
    return Status::ModeledNativeBranch;
  case DispatchClass::JetpackStateA0:
    state.raw_fec=2;
    host.native_reset(state,0);
    return Status::ModeledNativeBranch;
  case DispatchClass::JetpackDebugA3:
    if(original_debug_enabled)host.original_debug_event(event_id);
    return Status::ModeledNativeBranch;
  case DispatchClass::JetpackTimerA4:
    host.native_timed_action(0.05f,0);
    return Status::ModeledNativeBranch;
  case DispatchClass::JetpackResetA5:
    host.native_jetpack_a5_handler();
    host.native_reset(state,0);
    return Status::ModeledNativeBranch;
  case DispatchClass::OriginalFallback:
    // Original default branch resolves owner+0x1024, calls 0x2238B3C0
    // and only when it reports false invokes parent 0x2235BFD0.
    if(host.native_event_filter(event_id))return Status::ConsumedByOriginalFilter;
    host.native_parent_grunt_event(event_id);
    return Status::DelegatedToBase;
  default:
    // Original 0x19, 0x38/0x3C and 0xA6 cases have complex native
    // navigation/attachment/task effects; no fabricated host semantics.
    return Status::RequiresOriginalJetpackBranch;
  }
}
} // namespace sin_compat::jetpack_events_phase590