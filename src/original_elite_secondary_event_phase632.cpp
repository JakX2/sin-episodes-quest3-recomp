#include "sin_compat/original_elite_secondary_event_phase632.hpp"
namespace sin_compat::original_elite_secondary_event_phase632 {
namespace {
struct Guard {
  bool& busy;
  explicit Guard(bool& value)noexcept:busy(value){busy=true;}
  ~Guard()noexcept{busy=false;}
};
}
Result Reference::dispatch(std::uint32_t event_id)noexcept {
  Result result{};
  if(busy_) {
    result.code=Code::Reentrant;
    return result;
  }
  Guard guard(busy_);
  Stage stage=Stage::None;
  try {
    if(event_id!=kEvent43 && event_id!=kEvent47) {
      stage=Stage::InheritedDispatch;
      if(!host_.inherited_event_2235CCE0(event_id))
        return {Code::HostRejected,stage};
      result.code=Code::Inherited;
      result.inherited_called=true;
      return result;
    }
    if(event_id==kEvent43) {
      // Original: compare BYTE [Elite+0xFD9] to zero. If nonzero,
      // delegate the SAME event pointer to inherited 0x2235CCE0.
      std::uint8_t predicate=0u;
      stage=Stage::ReadElitePredicateFD9;
      if(!host_.read_elite_predicate_fd9(predicate))
        return {Code::HostRejected,stage};
      if(predicate!=0u) {
        stage=Stage::InheritedDispatch;
        if(!host_.inherited_event_2235CCE0(event_id))
          return {Code::HostRejected,stage};
        result.code=Code::Inherited;
        result.inherited_called=true;
        return result;
      }
      std::uint32_t handle=kInvalidHandle;
      stage=Stage::ReadEliteHandleFDC;
      if(!host_.read_elite_serial_handle_fdc(handle))
        return {Code::HostRejected,stage};
      std::uint64_t target=0u;
      if(handle!=kInvalidHandle) {
        stage=Stage::OpenSerialTable;
        if(!host_.open_original_serial_table())
          return {Code::HostRejected,stage};
        stage=Stage::ResolveSerial;
        const int lookup=host_.resolve_original_serial(handle,target);
        if(lookup!=0 && lookup!=1)
          return {Code::HostRejected,stage};
        if(lookup==0)target=0u;
      }
      // Retail unconditionally calls gate at 0x22355772, including when
      // the original serial handle was invalid/missing. Gate's first
      // instruction dereferences [ECX+0x594], so a null ECX is unsafe.
      stage=Stage::Gate43;
      if(target==0u) {
        result.code=Code::OriginalNullTargetHazard;
        result.stage=stage;
        return result;
      }
      bool allowed=false;
      result.gate_called=true;
      if(!host_.original_gate43(target,allowed)) {
        result.code=Code::HostRejected;
        result.stage=stage;
        return result;
      }
      if(!allowed) {
        result.code=Code::Skipped;
        return result;
      }
      stage=Stage::ActionZero;
      if(!host_.action_zero_2203D2D0()) {
        result.code=Code::HostRejected;
        result.stage=stage;
        return result;
      }
      result.action_zero_called=true;
      result.code=Code::ActionZero;
      return result;
    }

    // Native event 0x47 does NOT blindly delegate to the Grunt parent.
    // The predicate is a direct original call at 0x223556C5, followed
    // by a fresh object lookup. A false predicate returns immediately.
    bool enabled=false;
    stage=Stage::Predicate47;
    if(!host_.predicate_220326D0(enabled)) {
      result.code=Code::HostRejected;
      result.stage=stage;
      return result;
    }
    if(!enabled) {
      result.code=Code::Skipped;
      return result;
    }
    std::uint64_t first=0u;
    stage=Stage::FirstObjectLookup47;
    ++result.object_lookups;
    if(!host_.fetch_222AA640(first)) {
      result.code=Code::HostRejected;
      result.stage=stage;
      return result;
    }
    if(first!=0u) {
      // Distinct second original call to 0x222AA640, and a dynamic
      // vtable slot +0x404 on its returned object. Do not cache first.
      std::uint64_t second=0u;
      stage=Stage::SecondObjectLookup47;
      ++result.object_lookups;
      if(!host_.fetch_222AA640(second)) {
        result.code=Code::HostRejected;
        result.stage=stage;
        return result;
      }
      stage=Stage::Virtual404OnSecond47;
      if(second==0u) {
        result.code=Code::OriginalNullTargetHazard;
        result.stage=stage;
        return result;
      }
      std::uint32_t value=0u;
      if(!host_.object_virtual_404(second,value)) {
        result.code=Code::HostRejected;
        result.stage=stage;
        return result;
      }
      // Native passes &local_value (the virtual slot +0x404 result)
      // to the +0x4EC component obtained from a THIRD, fresh lookup.
      std::uint64_t third=0u;
      stage=Stage::ThirdObjectLookup47;
      ++result.object_lookups;
      if(!host_.fetch_222AA640(third)) {
        result.code=Code::HostRejected;
        result.stage=stage;
        return result;
      }
      stage=Stage::Component4EC47;
      if(third==0u) {
        result.code=Code::OriginalNullTargetHazard;
        result.stage=stage;
        return result;
      }
      if(!host_.component_4EC_call_2203F8E0(third,value)) {
        result.code=Code::HostRejected;
        result.stage=stage;
        return result;
      }
      for(unsigned n=3u;n<=5u;++n) {
        stage=(n==3u)?Stage::NumberedCallback3:
               (n==4u)?Stage::NumberedCallback4:Stage::NumberedCallback5;
        if(!host_.call_220398A0(n)) {
          result.code=Code::HostRejected;
          result.stage=stage;
          return result;
        }
        ++result.numbered_callbacks;
      }
    }
    // Both event47 predicate-true routes end in original +0(0).
    stage=Stage::ActionZero;
    if(!host_.action_zero_2203D2D0()) {
      result.code=Code::HostRejected;
      result.stage=stage;
      return result;
    }
    result.action_zero_called=true;
    result.code=Code::ActionZero;
    return result;
  } catch(...) {
    result.code=Code::HostRejected;
    result.stage=stage;
    return result;
  }
}
} // namespace sin_compat::original_elite_secondary_event_phase632
