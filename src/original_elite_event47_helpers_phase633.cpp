#include "sin_compat/original_elite_event47_helpers_phase633.hpp"
namespace sin_compat::original_elite_event47_helpers_phase633 {
namespace {
struct Guard final {
  bool& busy;
  explicit Guard(bool& value)noexcept:busy(value){busy=true;}
  ~Guard()noexcept{busy=false;}
};
// Native x87: FLD current clock, FCOMP deadline, FNSTSW AX, TEST AH,1.
// The original returns true when the x87 C0 less-than/unordered bit is
// clear. Both inputs are original IEEE f32; promotion to f64 precisely
// represents their finite values. FP control-word quirks not emulated.
bool clock_reached(float now,float deadline)noexcept {
  return static_cast<double>(now)>=static_cast<double>(deadline);
}
} // namespace
Result Cooldown::check()noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Stage stage=Stage::ClockRead;
  try {
    float now=0.f,deadline=0.f;
    if(!host_.read_original_engine_time(now))
      return {Code::HostRejected,stage};
    stage=Stage::EliteDeadline9DC;
    if(!host_.read_original_elite_deadline_9dc(deadline))
      return {Code::HostRejected,stage};
    return {clock_reached(now,deadline)?Code::Allowed:Code::Blocked,
            Stage::None};
  } catch(...){return {Code::HostRejected,stage};}
}
Result SerialLookup::resolve()noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result out{};
  Stage stage=Stage::EliteEncodedHandle78C;
  try {
    if(!host_.read_original_elite_serial_78c(out.encoded_handle)) {
      out.code=Code::HostRejected;out.stage=stage;return out;
    }
    if(out.encoded_handle==kInvalidSerialHandle) {
      out.code=Code::TargetAbsent;return out;
    }
    // This is the original 12-bit serial-table layout. No table access
    // occurs for a missing sentinel handle.
    out.slot_index=out.encoded_handle&0xfffu;
    out.serial=out.encoded_handle>>12u;
    stage=Stage::OpenSerialTable;
    if(!host_.open_original_serial_table()) {
      out.code=Code::HostRejected;out.stage=stage;return out;
    }
    stage=Stage::SerialSlotCheck;
    const int found=host_.read_original_serial_slot(
        out.slot_index,out.serial,out.target);
    if(found!=0 && found!=1) {
      out.code=Code::HostRejected;out.stage=stage;return out;
    }
    if(found==0)out.target=0u;
    out.code=(out.target!=0u)?Code::TargetFound:Code::TargetAbsent;
    return out;
  } catch(...){
    out.code=Code::HostRejected;out.stage=stage;return out;
  }
}
Result Field4ECSetter::set(std::uint32_t new_value)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result out{};
  Stage stage=Stage::Field4ECRead;
  try {
    std::uint32_t previous=0u;
    if(!host_.read_native_dword_4ec(previous)) {
      out.stage=stage;return out;
    }
    if(previous==new_value) {
      out.code=Code::Unchanged;return out; // original returns BEFORE dirty
    }
    out.changed=true;
    std::uint8_t original_flag=0u;
    stage=Stage::OriginalChangeFlag1F0;
    if(!host_.read_native_flag_1f0(original_flag)) {
      out.stage=stage;return out;
    }
    if(original_flag!=0u) {
      stage=Stage::WriteDirtyByte200;
      if(!host_.write_native_dirty_byte_200(1u)) {
        out.stage=stage;return out;
      }
      out.dirty_byte_committed=true;
    } else {
      stage=Stage::DirtyOwner16CRead;
      std::uint64_t owner=0u;
      if(!host_.read_native_dirty_owner_16c(owner)) {
        out.stage=stage;return out;
      }
      if(owner!=0u) {
        stage=Stage::MarkDirtyBit0;
        if(!host_.or_native_dirty_owner_bit0(owner)) {
          out.stage=stage;return out;
        }
        out.edict_dirty_committed=true;
      }
    }
    stage=Stage::WriteField4EC;
    if(!host_.write_native_dword_4ec(new_value)) {
      out.stage=stage;return out;
    }
    out.field_committed=true;
    out.code=Code::Applied;
    return out;
  } catch(...){out.stage=stage;out.code=Code::HostRejected;return out;}
}
} // namespace sin_compat::original_elite_event47_helpers_phase633
