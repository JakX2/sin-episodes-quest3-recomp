#include "sin_compat/original_memory_wrappers_phase645.hpp"
#include <limits>
namespace sin_compat::original_memory_wrappers_phase645 {
namespace {
struct Guard { bool& flag; explicit Guard(bool& f)noexcept:flag(f){f=true;}
  ~Guard()noexcept{flag=false;} };
constexpr bool add(std::uint32_t base,std::uint32_t offset,
                   std::uint32_t& value)noexcept {
  if(base==0u || base>std::numeric_limits<std::uint32_t>::max()-offset)
    return false;
  value=base+offset;return true;
}
}
bool AllocatorWrappers::resolve_method(std::uint32_t slot,Result& r,
    std::uint32_t& instance,std::uint32_t& method)noexcept {
  std::uint32_t indirect=0,vtable=0,at=0;
  r.stage=Stage::ReadManagerGlobal;
  if(!host_.read_u32(kMemoryManagerGlobal,indirect))return false;
  if(indirect==0u){r.code=Code::NullManagerHazard;return false;}
  r.stage=Stage::ReadManagerInstance;
  if(!host_.read_u32(indirect,instance))return false;
  if(instance==0u){r.code=Code::NullManagerHazard;return false;}
  r.stage=Stage::ReadManagerVtable;
  if(!host_.read_u32(instance,vtable))return false;
  if(!add(vtable,slot,at)){r.code=Code::AddressOverflow;return false;}
  r.stage=Stage::ReadVtableMethod;
  if(!host_.read_u32(at,method))return false;
  if(method==0u){r.code=Code::NullManagerHazard;return false;}
  r.dispatch_slot=slot;r.dispatch_target=method;
  return true;
}
Result AllocatorWrappers::discover_tag()noexcept {
  if(discovery_busy_)return {Code::Reentrant,Stage::None};
  Guard guard(discovery_busy_);
  Result r{};r.tag_discovery_calls=1u;
  try {
    std::uint32_t instance=0u,method=0u;
    if(!resolve_method(kSlotTagProbe,r,instance,method))return r;
    r.stage=Stage::ProbeManagerTag;
    const std::array<std::uint32_t,4> args{};
    std::uint32_t enabled=0u;
    if(!host_.call_memory_manager(instance,method,kSlotTagProbe,args,0u,enabled))return r;
    if((enabled&0xffu)==0u){r.code=Code::Completed;return r;}
    std::uint32_t heap=0u,buffer=0u,base=0u,ret=0u;
    r.stage=Stage::GetProcessHeap;
    if(!host_.win_get_process_heap(heap))return r;
    r.stage=Stage::HeapAlloc260;
    if(!host_.win_heap_alloc(heap,0u,kModuleNameBytes,buffer))return r;
    // Original does not test HeapAlloc's returned pointer; it still passes
    // NULL through to GetModuleFileNameA and returns NULL to its caller.
    r.stage=Stage::VirtualQueryModuleBase;
    if(!host_.win_virtual_query(kTagProbeAddress,kVirtualQueryBytes,base))return r;
    r.stage=Stage::GetModuleFileNameA260;
    if(!host_.win_get_module_file_name_a(base,buffer,kModuleNameBytes,ret))return r;
    // Return value of GetModuleFileNameA and VirtualQuery length ignored.
    r.result_pointer=buffer;r.tag_pointer=buffer;r.tag_enabled=buffer!=0u;
    r.code=Code::Completed;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
bool AllocatorWrappers::load_cached_tag(bool is_realloc,Result& r)noexcept {
  const std::uint32_t flag=is_realloc?kReallocateFlag:kAllocateFlag;
  const std::uint32_t cache=is_realloc?kReallocateTag:kAllocateTag;
  r.stage=Stage::ReadCacheFlagByte;
  std::uint8_t marker=0u;
  if(!host_.read_u8(flag,marker))return false;
  if((marker&1u)!=0u){
    r.cache_was_initially_set=true;
    r.stage=Stage::ReadCachedTag;
    if(!host_.read_u32(cache,r.tag_pointer))return false;
    r.tag_enabled=r.tag_pointer!=0u;
    return true;
  }
  std::uint32_t old_flag=0u;
  r.stage=Stage::ReadCacheFlagDword;
  if(!host_.read_u32(flag,old_flag))return false;
  r.stage=Stage::WriteCacheFlagDword;
  if(!host_.write_u32(flag,old_flag|1u))return false;
  ++r.writes_committed;
  // The native stores the per-wrapper init bit BEFORE calling the resolver.
  const auto discovered=discover_tag();
  r.tag_discovery_calls+=discovered.tag_discovery_calls;
  if(discovered.code!=Code::Completed){r.code=discovered.code;r.stage=discovered.stage;return false;}
  r.tag_pointer=discovered.result_pointer;
  r.tag_enabled=r.tag_pointer!=0u;
  r.stage=Stage::WriteCachedTag;
  if(!host_.write_u32(cache,r.tag_pointer))return false;
  ++r.writes_committed;
  return true;
}
Result AllocatorWrappers::allocate(std::uint32_t bytes)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result r{};
  try {
    if(!load_cached_tag(false,r))return r;
    const auto slot=r.tag_enabled?kSlotTaggedAllocate:kSlotUntaggedAllocate;
    std::uint32_t instance=0,method=0;
    if(!resolve_method(slot,r,instance,method))return r;
    std::array<std::uint32_t,4> args{};
    args[0]=bytes;
    const std::uint32_t count=r.tag_enabled?3u:1u;
    if(r.tag_enabled){args[1]=r.tag_pointer;args[2]=0u;}
    r.stage=r.tag_enabled?Stage::DispatchTaggedAllocate:Stage::DispatchUntaggedAllocate;
    if(!host_.call_memory_manager(instance,method,slot,args,count,r.result_pointer))return r;
    r.code=Code::Completed;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
Result AllocatorWrappers::reallocate(std::uint32_t old_ptr,std::uint32_t bytes)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result r{};
  try {
    if(!load_cached_tag(true,r))return r;
    const auto slot=r.tag_enabled?kSlotTaggedReallocate:kSlotUntaggedReallocate;
    std::uint32_t instance=0,method=0;
    if(!resolve_method(slot,r,instance,method))return r;
    std::array<std::uint32_t,4> args{};
    args[0]=old_ptr;args[1]=bytes;
    const std::uint32_t count=r.tag_enabled?4u:2u;
    if(r.tag_enabled){args[2]=r.tag_pointer;args[3]=0u;}
    r.stage=r.tag_enabled?Stage::DispatchTaggedReallocate:Stage::DispatchUntaggedReallocate;
    if(!host_.call_memory_manager(instance,method,slot,args,count,r.result_pointer))return r;
    r.code=Code::Completed;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
Result AllocatorWrappers::free(std::uint32_t old_ptr)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result r{};
  try {
    std::uint32_t instance=0,method=0;
    if(!resolve_method(kSlotFree,r,instance,method))return r;
    std::array<std::uint32_t,4> args{};args[0]=old_ptr;
    r.stage=Stage::DispatchFree;
    if(!host_.call_memory_manager(instance,method,kSlotFree,args,1u,r.result_pointer))return r;
    r.code=Code::Completed;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
} // namespace sin_compat::original_memory_wrappers_phase645
