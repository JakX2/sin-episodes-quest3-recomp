#include "sin_compat/original_elite_event47_terminal_phase634.hpp"
namespace sin_compat::original_elite_event47_terminal_phase634 {
namespace {
struct Guard {
  bool& busy;
  explicit Guard(bool& value)noexcept:busy(value){busy=true;}
  ~Guard()noexcept{busy=false;}
};
struct FlagIndex {
  bool skip=false;
  bool unsafe=false;
  std::int32_t word=-1;
  std::uint32_t mask=0u;
};
FlagIndex original_x86_bit_index(std::int32_t translated)noexcept {
  if(translated==-1)return {true,false,-1,0u};
  // Original LEA/ADD is modulo 2^32. Recover its signed x86 DWORD
  // value without C++ signed overflow or implementation-defined casts.
  const std::uint32_t delta_u=
      static_cast<std::uint32_t>(translated)-kOriginalIndexBase;
  const std::int64_t delta=delta_u<=0x7fffffffu?
      static_cast<std::int64_t>(delta_u):
      static_cast<std::int64_t>(delta_u)-0x100000000LL;
  if(delta==-1LL)return {true,false,-1,0u};
  // Retail SAR 5 can form *negative indices* into Elite's flag array.
  // That is a real native out-of-bounds hazard absent a class-length
  // proof. Never pass a guessed negative index to a portable host.
  if(delta<0)return {false,true,-1,0u};
  return {false,false,static_cast<std::int32_t>(delta/32),
          1u<<(static_cast<std::uint32_t>(delta)&31u)};
}
} // namespace
Result ClearFlag::run(std::int32_t original_id)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result out{};
  Stage stage=Stage::None;
  try {
    std::int32_t translated=original_id;
    // Original signed CMP/JL threshold 1,000,000,000: 3/4/5 and -1
    // require a *fresh* virtual+0x760 and mapping call per invocation.
    if(original_id<static_cast<std::int32_t>(kOriginalIndexBase) ||
       original_id==-1) {
      stage=Stage::ObtainVirtual760Context;
      std::uint64_t context=0u;
      if(!host_.original_virtual_760_context(context)) {
        out.failed_stage=stage;return out;
      }
      if(context==0u) {
        out.code=Code::OriginalNullContextHazard;
        out.failed_stage=stage;
        return out;
      }
      stage=Stage::ConvertSourceId;
      out.map_called=true;
      if(!host_.original_id_map_22073700(
           context,original_id,translated)) {
        out.failed_stage=stage;return out;
      }
    }
    out.converted_id=translated;
    stage=Stage::ComputeFlagIndex;
    const FlagIndex index=original_x86_bit_index(translated);
    if(index.skip) {
      out.code=Code::Skipped;return out;
    }
    if(index.unsafe) {
      out.code=Code::OriginalUnsafeFlagIndex;
      out.failed_stage=stage;return out;
    }
    out.word_index=index.word;
    out.mask=index.mask;
    stage=Stage::ClearFlagWord;
    if(!host_.clear_original_elite_flag_bit(
         static_cast<std::uint32_t>(index.word),index.mask)) {
      out.failed_stage=stage;return out;
    }
    out.code=Code::Cleared;
    out.bit_cleared=true;
    return out;
  } catch(...) {
    out.failed_stage=stage;out.code=Code::HostRejected;return out;
  }
}
Result StateZero::run(std::uint8_t state_argument)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result out{};
  Stage stage=Stage::MaintenanceCall;
  try {
    // The first original action is always the 0x2204E420 call,
    // even for nonzero input (which skips every following ID lookup).
    if(!host_.original_maintenance_2204E420()) {
      out.failed_stage=stage;return out;
    }
    out.maintenance_called=true;
    stage=Stage::CheckStateArgument;
    if(state_argument==0u) {
      stage=Stage::ObtainVirtual760Context;
      std::uint64_t context=0u;
      if(!host_.original_virtual_760_context(context)) {
        out.failed_stage=stage;return out;
      }
      if(context==0u) {
        out.code=Code::OriginalNullContextHazard;
        out.failed_stage=stage;return out;
      }
      stage=Stage::ConvertSourceId;
      out.map_called=true;
      std::int32_t translated=-1;
      if(!host_.original_id_map_22073700(
          context,static_cast<std::int32_t>(kOriginalTestEventId),
          translated)) {
        out.failed_stage=stage;return out;
      }
      out.converted_id=translated;
      stage=Stage::ComputeFlagIndex;
      const FlagIndex index=original_x86_bit_index(translated);
      if(!index.skip) {
        if(index.unsafe) {
          out.code=Code::OriginalUnsafeFlagIndex;
          out.failed_stage=stage;return out;
        }
        out.word_index=index.word;out.mask=index.mask;
        stage=Stage::TestFlagWord;
        bool set=false;
        if(!host_.read_original_elite_flag_bit(
            static_cast<std::uint32_t>(index.word),index.mask,set)) {
          out.failed_stage=stage;return out;
        }
        out.bit_tested=true;
        if(set) {
          out.code=Code::StatePreserved;
          return out;
        }
      }
    }
    stage=Stage::WriteState4;
    if(!host_.write_original_elite_state_7a4(kOriginalStateValue)) {
      out.failed_stage=stage;return out;
    }
    out.state_written=true;
    out.code=Code::State4Written;
    return out;
  } catch(...) {
    out.failed_stage=stage;out.code=Code::HostRejected;return out;
  }
}
} // namespace sin_compat::original_elite_event47_terminal_phase634
