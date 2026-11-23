#include "sin_compat/original_client_appsystem_phase659.hpp"
#include <limits>

namespace sin_compat::original_client_appsystem_phase659 {
namespace {
struct BusyGuard {
  bool& busy;
  explicit BusyGuard(bool& f)noexcept:busy(f){busy=true;}
  ~BusyGuard()noexcept{busy=false;}
};
bool add(std::uint32_t base,std::uint32_t displacement,
         std::uint32_t& result)noexcept{
  if(!base||base>std::numeric_limits<std::uint32_t>::max()-displacement)
    return false;
  result=base+displacement;return true;
}
}
Result AppSystemChain::register_entry(std::uint32_t entry,
       std::uint32_t destination,std::uint32_t resolver,
       std::uint32_t releaser)noexcept {
  Result r{};r.failed_entry=entry;
  if(busy_){r.code=Code::Reentrant;return r;}
  if(!entry||!resolver||!releaser){r.code=Code::InvalidArgument;return r;}
  BusyGuard guard(busy_);
  try{
    std::uint32_t ptr4=0,ptr8=0,ptr12=0;
    if(!add(entry,4u,ptr4)||!add(entry,8u,ptr8)||!add(entry,12u,ptr12)){
      r.code=Code::InvalidArgument;return r;
    }
    r.stage=Stage::WriteEntryTarget;
    if(!host_.write_u32(entry,destination))return r;
    ++r.writes_committed;
    r.stage=Stage::WriteEntryResolve;
    if(!host_.write_u32(ptr4,resolver))return r;
    ++r.writes_committed;
    r.stage=Stage::WriteEntryRelease;
    if(!host_.write_u32(ptr8,releaser))return r;
    ++r.writes_committed;
    std::uint32_t head=0u;
    r.stage=Stage::ReadHead;
    if(!host_.read_u32(kHeadVA,head))return r;
    r.stage=Stage::WriteEntryNext;
    if(!host_.write_u32(ptr12,head))return r;
    ++r.writes_committed;
    r.stage=Stage::WriteHead;
    if(!host_.write_u32(kHeadVA,entry))return r;
    ++r.writes_committed;
    r.code=Code::Completed;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
Result AppSystemChain::disconnect_internal()noexcept{
  Result r{};
  try {
    std::uint32_t entry=0u;
    r.stage=Stage::ReadHead;
    if(!host_.read_u32(kHeadVA,entry))return r;
    while(entry!=0u){
      if(++r.entries_visited>65536u){r.code=Code::CycleGuard;return r;}
      r.failed_entry=entry;
      std::uint32_t dst=0,release_addr=0,next_addr=0;
      r.stage=Stage::ReadEntryTarget;
      if(!host_.read_u32(entry,dst))return r;
      if(dst!=0u) {
        if(!add(entry,8u,release_addr)){r.code=Code::InvalidArgument;return r;}
        r.stage=Stage::ReadEntryRelease;
        std::uint32_t callback=0u,interface_object=0u;
        if(!host_.read_u32(release_addr,callback))return r;
        r.failed_callback=callback;
        r.stage=Stage::ReadTargetPointer;
        if(!host_.read_u32(dst,interface_object))return r;
        r.stage=Stage::CallRelease;
        ++r.release_calls;
        if(!host_.release_original_callback(callback,interface_object))return r;
        r.stage=Stage::ClearTarget;
        if(!host_.write_u32(dst,0u))return r;
        ++r.writes_committed;
      }
      r.stage=Stage::ReadEntryNext;
      if(!add(entry,12u,next_addr)){r.code=Code::InvalidArgument;return r;}
      if(!host_.read_u32(next_addr,entry))return r;
    }
    r.failed_entry=r.failed_callback=0u;r.code=Code::Completed;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
Result AppSystemChain::disconnect_all()noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  BusyGuard guard(busy_);
  return disconnect_internal();
}
Result AppSystemChain::connect_all()noexcept {
  Result r{};
  if(busy_){r.code=Code::Reentrant;return r;}
  BusyGuard guard(busy_);
  try {
    std::uint32_t entry=0u;
    r.stage=Stage::ReadHead;
    if(!host_.read_u32(kHeadVA,entry))return r;
    while(entry!=0u){
      if(++r.entries_visited>65536u){r.code=Code::CycleGuard;return r;}
      r.failed_entry=entry;
      std::uint32_t resolve_addr=0,next_addr=0;
      if(!add(entry,4u,resolve_addr)||!add(entry,12u,next_addr)){
        r.code=Code::InvalidArgument;return r;
      }
      std::uint32_t callback=0u;
      r.stage=Stage::ReadEntryResolve;
      if(!host_.read_u32(resolve_addr,callback))return r;
      r.failed_callback=callback;
      std::uint32_t result=0u;
      r.stage=Stage::CallResolver;
      ++r.resolver_calls;
      if(!host_.resolve_original_callback(callback,result))return r;
      if(result!=0u) {
        // Exact original: MOV ECX,[entry+0]; MOV [ECX],EAX.
        std::uint32_t dst=0u;
        r.stage=Stage::ReadEntryTarget;
        if(!host_.read_u32(entry,dst))return r;
        if(!dst){r.code=Code::NullDestinationHazard;return r;}
        r.stage=Stage::StoreResolvedPointer;
        if(!host_.write_u32(dst,result))return r;
        ++r.writes_committed;
      }else{
        std::uint32_t dst=0u;
        r.stage=Stage::ReadEntryTarget;
        if(!host_.read_u32(entry,dst))return r;
        if(dst!=0u){
          // Original failed required resolver invokes original full
          // disconnect-list function 0x241640F0, not a local rollback.
          r.cleanup_invoked=true;
          const auto cleaned=disconnect_internal();
          r.release_calls+=cleaned.release_calls;
          r.writes_committed+=cleaned.writes_committed;
          if(cleaned.code!=Code::Completed) {
            r.code=cleaned.code;r.stage=cleaned.stage;
            r.failed_entry=cleaned.failed_entry;
            r.failed_callback=cleaned.failed_callback;
            return r;
          }
          r.code=Code::MissingRequiredAppSystem;
          r.stage=Stage::DisconnectAfterFailure;
          return r;
        }
        // A resolver returning NULL with a null destination is optional.
      }
      r.stage=Stage::ReadEntryNext;
      if(!host_.read_u32(next_addr,entry))return r;
    }
    r.failed_entry=r.failed_callback=0u;r.code=Code::Completed;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
} // namespace sin_compat::original_client_appsystem_phase659
