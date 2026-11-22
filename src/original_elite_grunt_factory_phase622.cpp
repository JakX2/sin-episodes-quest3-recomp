#include "sin_compat/original_elite_grunt_factory_phase622.hpp"
#include <limits>

namespace sin_compat::original_elite_grunt_factory_phase622 {
bool FactoryReference::supports(std::string_view name)noexcept {
  return name==kOriginalClassname;
}
Result FactoryReference::create(
    std::string_view name,std::uintptr_t arg)noexcept {
  if(!supports(name))return {Code::UnsupportedClass,Stage::None};
  Guard guard(busy_);
  if(!guard)return {Code::Reentrant,Stage::None};
  std::uintptr_t p=0;
  Stage stage=Stage::Allocate;
  try {
    p=host_.allocate_native_npc(kOriginalAllocationBytes);
    if(!p)return {Code::AllocationFailed,stage};
    if(p>std::numeric_limits<std::uintptr_t>::max()-
           kFactoryReturnedSubobjectOffset) {
      stage=Stage::Allocate;
      const bool reclaimed=host_.reclaim_unpublished_npc(p);
      return {reclaimed?Code::PointerOverflow:Code::RollbackFailed,stage};
    }
    stage=Stage::BaseConstructor;
    if(!host_.base_npc_constructor_22360D80(p))goto failure;
    stage=Stage::PrimaryVptr;
    if(!host_.write_host_native_vptr(p,kPrimaryVptrOffset,
                                    VptrRole::Primary))goto failure;
    stage=Stage::SecondaryVptr;
    if(!host_.write_host_native_vptr(p,kSecondVptrOffset,
                                    VptrRole::Secondary790))goto failure;
    stage=Stage::TertiaryVptr;
    if(!host_.write_host_native_vptr(p,kThirdVptrOffset,
                                    VptrRole::TertiaryC14))goto failure;
    stage=Stage::StaticInitialization;
    if(!host_.static_initialize_22360BC0())goto failure;
    {
      Epochs epochs{};
      stage=Stage::ReadEpochs;
      if(!host_.read_epoch_words(epochs))goto failure;
      if(epochs.cached!=epochs.current) {
        stage=Stage::EpochRefresh;
        if(!host_.refresh_static_epoch_22355A20())goto failure;
        stage=Stage::PublishEpoch;
        if(!host_.commit_static_epoch(epochs.current))goto failure;
      }
    }
    stage=Stage::ExtraSentinel;
    if(!host_.write_extra_sentinel(
          p,kExtraSentinelFieldOffset,kExtraSentinel))goto failure;
    stage=Stage::NativeVirtual70;
    if(!host_.native_virtual_70(p,arg))goto failure;
    return {Code::Created,Stage::None,p,p+kFactoryReturnedSubobjectOffset};
  } catch(...) {}
failure:
  if(p) {
    try {
      if(!host_.reclaim_unpublished_npc(p))
        return {Code::RollbackFailed,stage};
    } catch(...){return {Code::RollbackFailed,stage};}
  }
  return {Code::HostRejected,stage};
}
bool FactoryReference::delegate_release(
    std::uintptr_t networkable_view)noexcept {
  if(!networkable_view)return true;
  Guard guard(busy_);
  if(!guard)return false;
  try{return host_.delegate_original_virtual_18(networkable_view);}
  catch(...){return false;}
}
} // namespace sin_compat::original_elite_grunt_factory_phase622
