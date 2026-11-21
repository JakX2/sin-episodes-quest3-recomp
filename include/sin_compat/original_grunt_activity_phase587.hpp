#pragma once
// Original-free Grunt activity reference. Every numerical activity below is
// proven by the original 2006 shared ACT registration table. Engine methods
// remain injected contracts, not fabricated implementations.
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sin_compat::grunt_activity_phase587 {

enum Activity : int {
  ACT_IDLE=1, ACT_WALK=6, ACT_WALK_AIM=7, ACT_RUN=10, ACT_RUN_AIM=11,
  ACT_RANGE_ATTACK1=16, ACT_RANGE_ATTACK2=17, ACT_RANGE_ATTACK1_LOW=18,
  ACT_CROUCHIDLE=45, ACT_RELOAD=66, ACT_RELOAD_LOW=67, ACT_IDLE_ANGRY=72,
  ACT_ON_KNEES=196, ACT_SHOOT_ON_KNEES=199, ACT_RELOAD_ON_KNEES=200,
};
struct GruntState {
  // Raw verified offsets in the original Grunt, not inferred semantic fields.
  bool raw_ca0 = false;
  bool raw_c64 = false;
  int raw_850 = 0;
  bool has_c18 = false;
  bool raw_988_bit1 = false;
};
struct Engine {
  virtual ~Engine() = default;
  // Grunt virtual+0x338: raw @DBC->virtual+0x4A8 returns address +DB8.
  virtual bool internal_component_identity() = 0;
  // If +C18 is non-null, its virtual+0x7C handles the mapped code.
  virtual int component_translate(int activity) = 0;
  // Original inherited fallback at 0x2203CFC0.
  virtual std::uint32_t inherited_capability_bits() = 0;
  virtual int virtual_718_with_target(std::optional<int> resolved_target) = 0;
  virtual int original_22039130() = 0;
  virtual int virtual_714(int probe) = 0;
  virtual bool registered_activity(int code) = 0;
  // Original +0x11 special case uses weapon_grenadelauncher and a runtime
  // global ID. We do not guess the live global values.
  virtual bool has_grenade_launcher() = 0;
  virtual int runtime_activity_for_weapon(bool found) = 0;
};

// Exact grounded branch order of original Grunt vtable +0x338. Safe host
// substitutes avoid native undefined dereferences and external ABI effects.
inline int grunt_activity_338(GruntState& state, int code,
                             Engine& e, std::optional<int> target_a18=std::nullopt) {
  if(code==ACT_RANGE_ATTACK1) {
    if(state.raw_ca0)code=ACT_SHOOT_ON_KNEES;
    else if(!state.raw_c64)code=ACT_RANGE_ATTACK1_LOW;
  } else if(code==ACT_RELOAD) {
    if(state.raw_ca0)code=ACT_RELOAD_ON_KNEES;
    else if(!state.raw_c64)code=ACT_RELOAD_LOW;
  } else if(code==ACT_RANGE_ATTACK2) {
    // Native returns directly; no +C18 or inherited fallback calls.
    return e.runtime_activity_for_weapon(e.has_grenade_launcher());
  } else if(code==ACT_RUN || code==ACT_WALK) {
    state.raw_c64=true; // original byte +0xC64, even on later rejection
  } else if(code==ACT_IDLE) {
    if(state.raw_ca0)code=ACT_ON_KNEES;
    else if(!state.raw_c64)code=ACT_CROUCHIDLE;
    else if(state.raw_850==2 || state.raw_850==3)code=ACT_IDLE_ANGRY;
  }

  if(e.internal_component_identity()) {
    if(code==ACT_IDLE)code=ACT_IDLE_ANGRY;
    else if(code==ACT_WALK)code=ACT_WALK_AIM;
    else if(code==ACT_RUN)code=ACT_RUN_AIM;
  }
  if(state.has_c18)return e.component_translate(code);

  // Original inherited activity transformer 0x2203CFC0.
  if((e.inherited_capability_bits() & 0x08000000u)==0)return code;
  if(code==ACT_RELOAD)return e.virtual_718_with_target(target_a18);
  if(code==3 || (code==ACT_IDLE && state.raw_988_bit1)) {
    int chosen=e.virtual_714(e.original_22039130());
    return e.registered_activity(chosen) ? chosen : ACT_IDLE;
  }
  return code;
}

struct SerialSlot { std::uint32_t serial=0; int entity=0; bool live=false; };
// Host representation of original global serial-checked 16-byte slots.
struct SerialTable {
  std::vector<SerialSlot> slots;
  std::optional<int> resolve(std::uint32_t raw) const {
    if(raw==0xFFFFFFFFu)return std::nullopt;
    const std::uint32_t slot=raw & 0xFFFu;
    if(slot>=slots.size())return std::nullopt;
    const auto& s=slots[slot];
    if(!s.live || s.serial != (raw >> 12))return std::nullopt;
    return s.entity;
  }
};

// The Grunt's original vtable +0x33C resolves raw handle +0x78C, and then
// invokes the target entity's virtual+0x47C(request, flag). Invalid/stale
// handles return the request unchanged and clear a non-null output flag.
struct TargetTranslator {
  virtual ~TargetTranslator()=default;
  virtual int virtual_47c(int entity,int request,bool* flag)=0;
};
inline int grunt_activity_33c(std::uint32_t raw_handle,const SerialTable& table,
                               int request,bool* output_flag,TargetTranslator& target) {
  auto entity=table.resolve(raw_handle);
  if(!entity) { if(output_flag)*output_flag=false; return request; }
  return target.virtual_47c(*entity,request,output_flag);
}

// Original 0x222AA840 scans up to 0x30 serial handles at owner +0x6CC,
// compares native classname pointer (entity+0xC) or 0x221387F0, then
// checks virtual+0x2E4 against requested owner/context. A returned entity
// ID is intentionally a host analogue of an engine pointer.
struct WeaponCandidate { std::uint32_t handle=0xFFFFFFFFu; std::string classname; int owner_tag=0; };
struct WeaponMatcher {
  virtual ~WeaponMatcher()=default;
  virtual bool original_class_match(int entity,const std::string& requested)=0;
  virtual int original_owner_tag(int entity)=0;
};
inline std::optional<int> find_weapon_222aa840(const SerialTable& table,
    const std::vector<WeaponCandidate>& candidates,const std::string& classname,
    int requested_owner_tag,WeaponMatcher& matcher) {
  const std::size_t n=candidates.size()<48?candidates.size():48;
  for(std::size_t i=0;i<n;++i) {
    const auto entity=table.resolve(candidates[i].handle);
    if(!entity)continue;
    if(candidates[i].classname!=classname && !matcher.original_class_match(*entity,classname))continue;
    if(matcher.original_owner_tag(*entity)==requested_owner_tag)return entity;
  }
  return std::nullopt;
}
}