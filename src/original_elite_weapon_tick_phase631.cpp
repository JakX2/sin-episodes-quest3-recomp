#include "sin_compat/original_elite_weapon_tick_phase631.hpp"
namespace sin_compat::original_elite_weapon_tick_phase631 {
namespace {
struct Guard final {
  bool& busy;
  explicit Guard(bool& flag)noexcept:busy(flag){busy=true;}
  ~Guard()noexcept{busy=false;}
};
}
Result Reference::run()noexcept {
  Result result{};
  if(busy_) {
    result.code=Code::Reentrant;
    return result;
  }
  Guard guard(busy_);
  Stage stage=Stage::None;
  try {
    // Original source: 0x223555F3 reads +0xFDC. An invalid first
    // handle skips the updater but tail-jumps to inherited Grunt.
    stage=Stage::FirstHandleRead;
    if(!host_.read_original_elite_fdc(result.first_handle))
      return {Code::HostRejected,stage};
    if(result.first_handle!=kHandleInvalid) {
      // 0x223555FE obtains global table 0x224F5620 exactly once.
      // Both resolutions use that one captured table base.
      stage=Stage::OpenSerialTable;
      if(!host_.open_original_serial_table())
        return {Code::HostRejected,stage};
      stage=Stage::FirstSerialResolve;
      const int first=host_.resolve_original_serial(
          result.first_handle,result.first_target);
      if(first!=0 && first!=1)
        return {Code::HostRejected,stage};
      if(first==1 && result.first_target!=0u) {
        // The original deliberately rereads +0xFDC and validates the
        // slot/serial AGAIN before calling 0x22315390.
        stage=Stage::SecondHandleRead;
        if(!host_.read_original_elite_fdc(result.second_handle)) {
          result.code=Code::HostRejected;
          result.failed_stage=stage;
          return result;
        }
        if(result.second_handle==kHandleInvalid) {
          // Retail enters 0x22315390 with ECX=0 via 0x22355654.
          result.code=Code::OriginalNullTargetHazard;
          result.failed_stage=Stage::OriginalTargetUpdate;
          return result;
        }
        stage=Stage::SecondSerialResolve;
        const int second=host_.resolve_original_serial(
            result.second_handle,result.second_target);
        if(second!=0 && second!=1) {
          result.code=Code::HostRejected;
          result.failed_stage=stage;
          return result;
        }
        if(second==0 || result.second_target==0u) {
          result.code=Code::OriginalNullTargetHazard;
          result.failed_stage=Stage::OriginalTargetUpdate;
          return result;
        }
        stage=Stage::OriginalTargetUpdate;
        result.update_attempted=true;
        if(!host_.invoke_original_weapon_update(result.second_target)) {
          result.code=Code::HostRejected;
          result.failed_stage=stage;
          return result;
        }
      }
    }
    // Tail-call to inherited NPC callback at 0x2235DF10 is present
    // after both safe paths; it is *not* an invented game Think name.
    stage=Stage::InheritedGruntTick;
    if(!host_.inherited_grunt_tick_2235DF10()) {
      result.code=Code::HostRejected;
      result.failed_stage=stage;
      return result;
    }
    result.inherited_called=true;
    result.code=result.update_attempted?
        Code::WeaponUpdatedThenInherited:Code::InheritedOnly;
    return result;
  } catch(...) {
    result.code=Code::HostRejected;
    result.failed_stage=stage;
    return result;
  }
}
} // namespace sin_compat::original_elite_weapon_tick_phase631
