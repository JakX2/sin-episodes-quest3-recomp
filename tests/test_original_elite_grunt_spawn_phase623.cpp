// Phase623: authentic original EliteGrunt::Spawn order on a synthetic host.
// No original code, real Source engine, or native retail NPC is loaded.
#include "sin_compat/original_elite_grunt_spawn_phase623.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace spawn=sin_compat::original_elite_grunt_spawn_phase623;
using spawn::Stage;

struct Fake final:spawn::IHost {
  std::vector<std::string> calls;
  std::vector<spawn::DirtyPath> dirty;
  std::string weapon;
  std::uint32_t flags=0,fdc=0;
  std::int32_t field_9c=0,field_98=0;
  std::uint32_t weapon_handle=0x13200005u;
  int lookup_result=1,scalar_calls=0;
  float scalars[2]={9.75f,-2.9f};
  Stage refused=Stage::None,throw_at=Stage::None;
  spawn::SpawnReference* reenter=nullptr;
  spawn::Code reentry_result=spawn::Code::Applied;

  bool mark(Stage stage,const char* name) {
    calls.emplace_back(name);
    if(stage==throw_at)throw std::runtime_error("synthetic host error");
    return stage!=refused;
  }
  bool assign_original_weapon_symbol(std::string_view name)override {
    if(!mark(Stage::WeaponSymbol,"weapon"))return false;
    assert(name==spawn::kOriginalWeapon);
    weapon=std::string(name);return true;
  }
  bool native_virtual_68()override {
    const bool success=mark(Stage::Virtual68,"virtual68");
    if(reenter)reentry_result=reenter->apply({
      spawn::kOriginalClassname,"other",0,0,0,false,false}).code;
    return success;
  }
  bool native_virtual_1c_then_6c()override {
    return mark(Stage::Virtual1CThen6C,"virtual1c+6c");
  }
  bool inherited_npc_spawn_2235F2A0()override {
    return mark(Stage::InheritedSpawn,"inherited");
  }
  bool clear_byte_fd9()override {
    return mark(Stage::ClearFD9,"clear-fd9");
  }
  bool native_virtual_798_with_zero()override {
    return mark(Stage::Virtual798,"virtual798(0)");
  }
  bool read_original_convar_float_28(float& out)override {
    const Stage stage=scalar_calls==0?Stage::ReadScalar9C:Stage::ReadScalar98;
    if(!mark(stage,scalar_calls==0?"read9c":"read98"))return false;
    if(scalar_calls>=2)return false;
    out=scalars[scalar_calls++];
    return true;
  }
  bool virtual_1b4_then_write_field_9c(std::int32_t value)override {
    if(!mark(Stage::UpdateField9C,"v1b4+field9c"))return false;
    field_9c=value;return true;
  }
  bool virtual_1b0_then_write_field_98(std::int32_t value)override {
    if(!mark(Stage::UpdateField98,"v1b0+field98"))return false;
    field_98=value;return true;
  }
  bool commit_flags_b4(std::uint32_t value,spawn::DirtyPath marker)override {
    const Stage stage=(value&spawn::kSecondaryFlags)!=0?
      Stage::FlagsSecondary:Stage::FlagsPrimary;
    if(!mark(stage,(stage==Stage::FlagsPrimary)?
                    "flags+100000":"flags+2000"))return false;
    flags=value;
    dirty.push_back(marker);return true;
  }
  int resolve_chaingun_encoded_handle(std::uint32_t& out)override {
    if(!mark(Stage::ResolveActiveChaingun,"lookup-rtti"))return -1;
    if(lookup_result==1)out=weapon_handle;
    return lookup_result;
  }
  bool write_field_fdc(std::uint32_t v)override {
    if(!mark(Stage::WriteHandleFDC,"field-fdc"))return false;
    fdc=v;return true;
  }
};
static spawn::Snapshot base() {
  return {spawn::kOriginalClassname,"wrong_weapon",4,4,0,
          false,true};
}
void test_full_source_order_independent_scalar_reads() {
  Fake host;
  spawn::SpawnReference runtime(host);
  const auto r=runtime.apply(base());
  assert(r.ok()&&r.wrote_handle);
  assert(r.failed_stage==Stage::None);
  assert(r.final_handle_fdc==0x13200005u);
  assert(host.weapon=="weapon_chaingun");
  assert(host.field_9c==9 && host.field_98==-2);
  assert(host.flags==(spawn::kPrimaryFlags|spawn::kSecondaryFlags));
  assert(r.flags_after==host.flags);
  assert(host.scalar_calls==2);
  assert(host.dirty==std::vector<spawn::DirtyPath>({
    spawn::DirtyPath::EdictStateBit0,
    spawn::DirtyPath::EdictStateBit0}));
  assert((host.calls==std::vector<std::string>{
    "weapon","virtual68","virtual1c+6c","inherited",
    "clear-fd9","virtual798(0)",
    "read9c","v1b4+field9c","read98","v1b0+field98",
    "flags+100000","flags+2000","lookup-rtti","field-fdc"
  }));
}
void test_cached_weapon_and_already_set_fields() {
  Fake host;spawn::SpawnReference runtime(host);
  host.scalars[0]=2.3f;host.scalars[1]=2.8f;
  auto current=base();
  current.current_weapon_symbol="weapon_chaingun";
  current.original_field_9c=2;
  current.original_field_98=2;
  current.original_flags_b4=spawn::kPrimaryFlags|spawn::kSecondaryFlags;
  const auto r=runtime.apply(current);
  assert(r.ok()&&r.flags_after==current.original_flags_b4);
  assert(host.weapon.empty()&&host.dirty.empty());
  assert((host.calls==std::vector<std::string>{
    "virtual68","virtual1c+6c","inherited",
    "clear-fd9","virtual798(0)",
    "read9c","read98","lookup-rtti","field-fdc"
  }));
}
void test_rtti_rejection_and_raw_zero_handle() {
  {
    Fake host;host.lookup_result=0;
    spawn::SpawnReference runtime(host);
    const auto result=runtime.apply(base());
    assert(result.ok()&&result.final_handle_fdc==spawn::kInvalidHandle);
    assert(host.fdc==spawn::kInvalidHandle);
  }
  {
    Fake host;host.weapon_handle=0u;
    spawn::SpawnReference runtime(host);
    const auto result=runtime.apply(base());
    // Zero is a source raw handle; do not silently substitute -1.
    assert(result.ok()&&result.final_handle_fdc==0u);
    assert(host.fdc==0u);
  }
}
void test_all_network_dirty_paths() {
  for(int i=0;i<3;++i) {
    Fake host;spawn::SpawnReference runtime(host);
    auto in=base();
    in.original_flag_1f0=(i==0);
    in.original_edict_16c_present=(i==1);
    auto result=runtime.apply(in);
    assert(result.ok() && host.dirty.size()==2);
    spawn::DirtyPath expected=i==0?spawn::DirtyPath::NativeField200:
      (i==1?spawn::DirtyPath::EdictStateBit0:spawn::DirtyPath::NoMarker);
    assert(host.dirty[0]==expected && host.dirty[1]==expected);
  }
}
void test_refusal_at_every_stage_and_partial_mutations() {
  constexpr Stage stages[]{
    Stage::WeaponSymbol,Stage::Virtual68,Stage::Virtual1CThen6C,
    Stage::InheritedSpawn,Stage::ClearFD9,Stage::Virtual798,
    Stage::ReadScalar9C,Stage::UpdateField9C,Stage::ReadScalar98,
    Stage::UpdateField98,Stage::FlagsPrimary,Stage::FlagsSecondary,
    Stage::ResolveActiveChaingun,Stage::WriteHandleFDC
  };
  for(auto stage:stages) {
    Fake host;host.refused=stage;
    spawn::SpawnReference runtime(host);
    auto r=runtime.apply(base());
    assert(r.code==spawn::Code::HostRejected);
    assert(r.failed_stage==stage);
    assert(!r.wrote_handle);
    assert(!host.calls.empty());
    assert(host.fdc==0u);
    if(stage==Stage::FlagsSecondary) {
      assert(r.flags_after==spawn::kPrimaryFlags);
      assert(host.flags==spawn::kPrimaryFlags);
    }
  }
  {
    Fake host;host.throw_at=Stage::Virtual798;
    spawn::SpawnReference runtime(host);
    auto result=runtime.apply(base());
    assert(result.code==spawn::Code::HostRejected &&
           result.failed_stage==Stage::Virtual798);
  }
  {
    Fake host;host.lookup_result=-1;
    spawn::SpawnReference runtime(host);
    const auto result=runtime.apply(base());
    assert(result.code==spawn::Code::HostRejected);
    assert(result.failed_stage==Stage::ResolveActiveChaingun);
  }
}
void test_original_x87_limit_refusal() {
  for(float invalid:{
     std::numeric_limits<float>::quiet_NaN(),
     std::numeric_limits<float>::infinity(),
     -std::numeric_limits<float>::infinity(),1e35f,-1e35f,
     2147483648.f}) {
    for(int slot=0;slot<2;++slot) {
      Fake host;host.scalars[slot]=invalid;
      spawn::SpawnReference runtime(host);
      const auto result=runtime.apply(base());
      assert(result.code==spawn::Code::UnsupportedOriginalX87);
      assert(result.failed_stage==(slot==0?Stage::ReadScalar9C:
                                          Stage::ReadScalar98));
      assert(host.flags==0u && host.fdc==0u);
    }
  }
  {
    Fake host;
    host.scalars[0]=-2147483648.f;
    host.scalars[1]=2147483520.f;
    spawn::SpawnReference runtime(host);
    assert(runtime.apply(base()).ok());
    assert(host.field_9c==std::numeric_limits<std::int32_t>::min());
    assert(host.field_98==2147483520);
  }
}
void test_classname_and_reentry_fail_closed() {
  Fake host;spawn::SpawnReference runtime(host);
  auto s=base();s.original_classname="npc_druglab_grunt";
  assert(runtime.apply(s).code==spawn::Code::UnsupportedClass);
  s.original_classname="npc_druglab_grunt_elite_extra";
  assert(runtime.apply(s).code==spawn::Code::UnsupportedClass);
  assert(host.calls.empty());
  host.reenter=&runtime;
  assert(runtime.apply(base()).ok());
  assert(host.reentry_result==spawn::Code::Reentrant);
}
int main() {
  test_full_source_order_independent_scalar_reads();
  test_cached_weapon_and_already_set_fields();
  test_rtti_rejection_and_raw_zero_handle();
  test_all_network_dirty_paths();
  test_refusal_at_every_stage_and_partial_mutations();
  test_original_x87_limit_refusal();
  test_classname_and_reentry_fail_closed();
  std::puts("PASS Phase623 Elite Grunt Spawn original weapon/schedule/FD9/flags/RTTI chain; synthetic host only");
}
