// Phase622: original-free faithful elite *factory sequence* regressions.
// This synthetic IHost is NOT a Source CBaseNPC or engine ABI implementation.
#include "sin_compat/original_elite_grunt_factory_phase622.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace elite=sin_compat::original_elite_grunt_factory_phase622;

struct FakeEngine final:elite::IHost {
  std::vector<std::string> events;
  std::uintptr_t object=0x1100000u,argument=0;
  elite::Epochs epochs{3,4};
  elite::Stage refusal=elite::Stage::None;
  elite::Stage exception=elite::Stage::None;
  elite::FactoryReference* reenter=nullptr;
  elite::Code nested=elite::Code::Created;
  unsigned reclaimed=0,committed_epoch=0;
  bool reclaim_ok=true,delegate_ok=true;

  bool stage(elite::Stage at,const char* event) {
    events.emplace_back(event);
    if(at==exception)throw std::runtime_error("fake engine failure");
    return at!=refusal;
  }
  std::uintptr_t allocate_native_npc(std::size_t bytes)override {
    assert(bytes==0xFE0u);
    return stage(elite::Stage::Allocate,"allocate")?object:0;
  }
  bool base_npc_constructor_22360D80(std::uintptr_t p)override {
    assert(p==object);
    return stage(elite::Stage::BaseConstructor,"base-ctor");
  }
  bool write_host_native_vptr(
      std::uintptr_t p,std::uint32_t offset,elite::VptrRole role)override {
    assert(p==object);
    if(offset==0) {
      assert(role==elite::VptrRole::Primary);
      return stage(elite::Stage::PrimaryVptr,"vptr-0");
    } else if(offset==0x790) {
      assert(role==elite::VptrRole::Secondary790);
      return stage(elite::Stage::SecondaryVptr,"vptr-790");
    } else {
      assert(offset==0xC14&&role==elite::VptrRole::TertiaryC14);
      return stage(elite::Stage::TertiaryVptr,"vptr-c14");
    }
  }
  bool static_initialize_22360BC0()override {
    return stage(elite::Stage::StaticInitialization,"static-init");
  }
  bool read_epoch_words(elite::Epochs& out)override {
    if(!stage(elite::Stage::ReadEpochs,"read-epochs"))return false;
    out=epochs;return true;
  }
  bool refresh_static_epoch_22355A20()override {
    return stage(elite::Stage::EpochRefresh,"refresh-epoch");
  }
  bool commit_static_epoch(std::uint32_t current)override {
    if(!stage(elite::Stage::PublishEpoch,"publish-epoch"))return false;
    committed_epoch=current;
    epochs.cached=current;
    return true;
  }
  bool write_extra_sentinel(
      std::uintptr_t p,std::uint32_t offset,std::uint32_t value)override {
    assert(p==object&&offset==0xFDCu&&value==0xffffffffu);
    return stage(elite::Stage::ExtraSentinel,"sentinel-fdc");
  }
  bool native_virtual_70(std::uintptr_t p,std::uintptr_t arg)override {
    assert(p==object);argument=arg;
    const bool result=stage(elite::Stage::NativeVirtual70,"virtual70");
    if(reenter)nested=reenter->create("npc_druglab_grunt_elite",arg).code;
    return result;
  }
  bool reclaim_unpublished_npc(std::uintptr_t p)override {
    assert(p==object);
    ++reclaimed;events.emplace_back("rollback");
    return reclaim_ok;
  }
  bool delegate_original_virtual_18(std::uintptr_t v)override {
    assert(v==object+0x160u);
    events.emplace_back("delegate-v18");return delegate_ok;
  }
};
static void test_elite_is_not_mobgrunt_variant() {
  FakeEngine h;
  elite::FactoryReference r(h);
  assert(elite::FactoryReference::supports("npc_druglab_grunt_elite"));
  for(std::string_view n:{"npc_druglab_grunt",
         "npc_druglab_grunt_pistol","npc_druglab_grunt_scattergun",
         "npc_druglab_grunt_assault_rifle",
         "npc_druglab_grunt_elite_extra","npc_sintek_grunt",""}) {
    assert(!elite::FactoryReference::supports(n));
    assert(r.create(n,0).code==elite::Code::UnsupportedClass);
  }
  assert(h.events.empty());
}
static void test_source_order_and_epoch_difference() {
  FakeEngine h;
  elite::FactoryReference r(h);
  const auto created=r.create("npc_druglab_grunt_elite",0xA5u);
  assert(created.ok());
  assert(created.original_object==h.object);
  assert(created.original_networkable_view==h.object+0x160u);
  assert(h.argument==0xA5u&&h.committed_epoch==4u&&h.reclaimed==0);
  assert((h.events==std::vector<std::string>{
    "allocate","base-ctor","vptr-0","vptr-790","vptr-c14",
    "static-init","read-epochs","refresh-epoch","publish-epoch",
    "sentinel-fdc","virtual70"
  }));
  assert(r.delegate_release(0u));
  assert(r.delegate_release(h.object+0x160u));
  assert(h.events.back()=="delegate-v18");
}
static void test_equal_epoch_skips_refresh_only() {
  FakeEngine h;h.epochs={9,9};
  elite::FactoryReference r(h);
  assert(r.create("npc_druglab_grunt_elite",1).ok());
  assert(h.committed_epoch==0);
  assert((h.events==std::vector<std::string>{
    "allocate","base-ctor","vptr-0","vptr-790","vptr-c14",
    "static-init","read-epochs","sentinel-fdc","virtual70"
  }));
}
static void test_fail_closed_at_each_native_stage() {
  constexpr elite::Stage stages[]{
    elite::Stage::BaseConstructor,elite::Stage::PrimaryVptr,
    elite::Stage::SecondaryVptr,elite::Stage::TertiaryVptr,
    elite::Stage::StaticInitialization,elite::Stage::ReadEpochs,
    elite::Stage::EpochRefresh,elite::Stage::PublishEpoch,
    elite::Stage::ExtraSentinel,elite::Stage::NativeVirtual70
  };
  for(auto target:stages) {
    FakeEngine h;h.refusal=target;
    elite::FactoryReference r(h);
    const auto outcome=r.create("npc_druglab_grunt_elite",0);
    assert(outcome.code==elite::Code::HostRejected);
    assert(outcome.failed_stage==target);
    assert(!outcome.original_object&&!outcome.original_networkable_view);
    assert(h.reclaimed==1u&&h.events.back()=="rollback");
  }
  {
    FakeEngine h;h.refusal=elite::Stage::Allocate;
    elite::FactoryReference r(h);
    assert(r.create("npc_druglab_grunt_elite",0).code==
           elite::Code::AllocationFailed);
    assert(h.reclaimed==0u);
  }
  {
    FakeEngine h;h.exception=elite::Stage::ReadEpochs;
    elite::FactoryReference r(h);
    assert(r.create("npc_druglab_grunt_elite",0).code==
           elite::Code::HostRejected);
    assert(h.reclaimed==1u);
  }
  {
    FakeEngine h;h.refusal=elite::Stage::NativeVirtual70;
    h.reclaim_ok=false;
    elite::FactoryReference r(h);
    assert(r.create("npc_druglab_grunt_elite",0).code==
           elite::Code::RollbackFailed);
  }
}
static void test_address_overflow_and_reentrancy() {
  {
    FakeEngine h;
    h.object=std::numeric_limits<std::uintptr_t>::max()-0x100u;
    elite::FactoryReference r(h);
    assert(r.create("npc_druglab_grunt_elite",0).code==
           elite::Code::PointerOverflow);
    assert(h.reclaimed==1u);
    assert((h.events==std::vector<std::string>{"allocate","rollback"}));
  }
  {
    FakeEngine h;elite::FactoryReference r(h);
    h.reenter=&r;
    assert(r.create("npc_druglab_grunt_elite",0).ok());
    assert(h.nested==elite::Code::Reentrant);
    h.delegate_ok=false;
    assert(!r.delegate_release(h.object+0x160u));
  }
}
int main() {
  test_elite_is_not_mobgrunt_variant();
  test_source_order_and_epoch_difference();
  test_equal_epoch_skips_refresh_only();
  test_fail_closed_at_each_native_stage();
  test_address_overflow_and_reentrancy();
  std::puts("PASS Phase622 original Elite Grunt distinct factory: native 0xFE0, subobject +0x160, vptr offsets, conditional static epoch and 0xFDC sentinel");
}
