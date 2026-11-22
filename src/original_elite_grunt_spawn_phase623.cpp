#include "sin_compat/original_elite_grunt_spawn_phase623.hpp"
#include <cmath>
#include <cstdint>

namespace sin_compat::original_elite_grunt_spawn_phase623 {
namespace {
bool convert_original_trunc(float f,std::int32_t& converted) noexcept {
  // The 2006 source temporarily sets x87 RC=truncate and restores it.
  // x87 invalid-operation/indefinite outcomes are not portable; refuse.
  const double d=static_cast<double>(f);
  if(!std::isfinite(d)||d<-2147483648.0||d>=2147483648.0)
    return false;
  converted=static_cast<std::int32_t>(std::trunc(d));
  return true;
}
DirtyPath dirty(const Snapshot& s) noexcept {
  if(s.original_flag_1f0)return DirtyPath::NativeField200;
  if(s.original_edict_16c_present)return DirtyPath::EdictStateBit0;
  return DirtyPath::NoMarker;
}
} // namespace
Result SpawnReference::apply(const Snapshot& current) noexcept {
  if(current.original_classname!=kOriginalClassname)
    return {Code::UnsupportedClass};
  if(busy_)return {Code::Reentrant};
  struct Guard final {
    bool& flag;
    explicit Guard(bool& b)noexcept:flag(b){flag=true;}
    ~Guard()noexcept{flag=false;}
  } guard(busy_);

  Result r{};
  r.flags_after=current.original_flags_b4;
  Stage stage=Stage::None;
  try {
    if(current.current_weapon_symbol!=kOriginalWeapon) {
      stage=Stage::WeaponSymbol;
      if(!host_.assign_original_weapon_symbol(kOriginalWeapon))goto refusal;
    }
    stage=Stage::Virtual68;
    if(!host_.native_virtual_68())goto refusal;
    stage=Stage::Virtual1CThen6C;
    if(!host_.native_virtual_1c_then_6c())goto refusal;
    stage=Stage::InheritedSpawn;
    if(!host_.inherited_npc_spawn_2235F2A0())goto refusal;

    // The source clears +0xFD9 before invoking virtual +0x798(0).
    stage=Stage::ClearFD9;
    if(!host_.clear_byte_fd9())goto refusal;
    stage=Stage::Virtual798;
    if(!host_.native_virtual_798_with_zero())goto refusal;

    // The original reads the same source ConVar twice. Respect order:
    // +0x9C with virtual +0x1B4, then +0x98 with virtual +0x1B0.
    {
      float scalar=0;
      std::int32_t converted=0;
      stage=Stage::ReadScalar9C;
      if(!host_.read_original_convar_float_28(scalar))goto refusal;
      if(!convert_original_trunc(scalar,converted)) {
        r.code=Code::UnsupportedOriginalX87;
        r.failed_stage=stage;
        return r;
      }
      if(current.original_field_9c!=converted) {
        stage=Stage::UpdateField9C;
        if(!host_.virtual_1b4_then_write_field_9c(converted))goto refusal;
      }
    }
    {
      float scalar=0;
      std::int32_t converted=0;
      stage=Stage::ReadScalar98;
      if(!host_.read_original_convar_float_28(scalar))goto refusal;
      if(!convert_original_trunc(scalar,converted)) {
        r.code=Code::UnsupportedOriginalX87;
        r.failed_stage=stage;
        return r;
      }
      if(current.original_field_98!=converted) {
        stage=Stage::UpdateField98;
        if(!host_.virtual_1b0_then_write_field_98(converted))goto refusal;
      }
    }
    if((r.flags_after&kPrimaryFlags)==0u) {
      stage=Stage::FlagsPrimary;
      if(!host_.commit_flags_b4(r.flags_after|kPrimaryFlags,
                               dirty(current)))goto refusal;
      r.flags_after|=kPrimaryFlags;
    }
    if((r.flags_after&kSecondaryFlags)==0u) {
      stage=Stage::FlagsSecondary;
      if(!host_.commit_flags_b4(r.flags_after|kSecondaryFlags,
                               dirty(current)))goto refusal;
      r.flags_after|=kSecondaryFlags;
    }
    {
      stage=Stage::ResolveActiveChaingun;
      std::uint32_t handle=0u;
      const int code=host_.resolve_chaingun_encoded_handle(handle);
      if(code!=0 && code!=1)goto refusal;
      r.final_handle_fdc=(code==1)?handle:kInvalidHandle;
      stage=Stage::WriteHandleFDC;
      if(!host_.write_field_fdc(r.final_handle_fdc))goto refusal;
      r.wrote_handle=true;
    }
  } catch(...) {
    r.code=Code::HostRejected;
    r.failed_stage=stage;
    return r;
  }
  r.code=Code::Applied;
  r.failed_stage=Stage::None;
  return r;
refusal:
  r.code=Code::HostRejected;
  r.failed_stage=stage;
  return r;
}
} // namespace sin_compat::original_elite_grunt_spawn_phase623
