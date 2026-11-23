#include "sin_compat/original_nested_map_phase647.hpp"
#include <limits>
namespace sin_compat::original_nested_map_phase647 {
namespace {
struct Guard {bool& flag;explicit Guard(bool& f)noexcept:flag(f){f=true;}
  ~Guard()noexcept{flag=false;}};
constexpr bool address(std::uint32_t base,std::uint32_t offset,
                       std::uint32_t& out)noexcept {
  if(base==0u||base>std::numeric_limits<std::uint32_t>::max()-offset)
    return false;
  out=base+offset;return true;
}
}
Result ArrayConstructor::construct(std::uint32_t object,
    std::uint32_t step,std::uint32_t capacity,std::uint32_t callback)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result r{};r.object_va=object;r.initial_capacity=capacity;
  if(object==0u){r.code=Code::NullObjectHazard;return r;}
  try {
    const auto w32=[&](std::uint32_t offset,std::uint32_t value,
                       Stage stage)->bool {
      r.stage=stage;std::uint32_t va=0;
      if(!address(object,offset,va)){r.code=Code::AddressOverflow;return false;}
      if(!host_.write_u32(va,value))return false;
      ++r.writes_committed;return true;
    };
    const auto w16=[&](std::uint32_t offset,std::uint16_t value,
                       Stage stage)->bool {
      r.stage=stage;std::uint32_t va=0;
      if(!address(object,offset,va)){r.code=Code::AddressOverflow;return false;}
      if(!host_.write_u16(va,value))return false;
      ++r.writes_committed;return true;
    };
    if(!w32(0u,callback,Stage::WriteCallback0))return r;
    if(!w32(4u,0u,Stage::WriteArray4Null))return r;
    if(!w32(8u,capacity,Stage::WriteCapacity8))return r;
    if(!w32(0xcu,step,Stage::WriteStepC))return r;
    if(capacity!=0u){
      // Native LEA/ADD/ADD computes 12*capacity modulo 2**32.
      r.allocation_bytes=capacity*12u;
      r.stage=Stage::AllocateOriginal2226e390;
      r.allocation_called=true;
      if(!host_.allocate_original_2226e390(r.allocation_bytes,
                                            r.allocated_pointer))return r;
      if(!w32(4u,r.allocated_pointer,Stage::WriteArray4Allocated))return r;
    }
    if(!w16(0x10u,kNullIndex,Stage::WriteRoot10))return r;
    if(!w16(0x14u,kNullIndex,Stage::WriteFreeHead14))return r;
    std::uint32_t pointer_field=0,cache=0;
    r.stage=Stage::ReadArray4ForCache;
    if(!address(object,4u,pointer_field)){r.code=Code::AddressOverflow;return r;}
    if(!host_.read_u32(pointer_field,cache))return r;
    if(!w32(0x18u,cache,Stage::WriteCache18))return r;
    if(!w16(0x12u,0u,Stage::WriteCount12))return r;
    if(!w16(0x16u,0u,Stage::WriteNext16))return r;
    r.code=Code::Completed;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
Result MapConstructor::construct(std::uint32_t object,
    std::uint32_t step,std::uint32_t capacity,std::uint32_t selector)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result r{};r.object_va=object;r.initial_capacity=capacity;
  if(object==0u){r.code=Code::NullObjectHazard;return r;}
  try {
    r.stage=Stage::NestedArrayConstructor;
    const Result inner=array_.construct(object,step,capacity,0u);
    r.writes_committed=inner.writes_committed;
    r.allocation_called=inner.allocation_called;
    r.allocation_bytes=inner.allocation_bytes;
    r.allocated_pointer=inner.allocated_pointer;
    if(inner.code!=Code::Completed){r.code=inner.code;r.stage=inner.stage;return r;}
    r.nested_array_completed=true;
    const auto w32=[&](std::uint32_t offset,std::uint32_t value,
                       Stage stage)->bool {
      r.stage=stage;std::uint32_t va=0;
      if(!address(object,offset,va)){r.code=Code::AddressOverflow;return false;}
      if(!host_.write_u32(va,value))return false;
      ++r.writes_committed;return true;
    };
    if(!w32(0x1cu,0u,Stage::WriteMap1C))return r;
    if(!w32(0x20u,0u,Stage::WriteMap20))return r;
    if(!w32(0x24u,8u,Stage::WriteMap24))return r;
    if(!w32(0x28u,0u,Stage::WriteMap28))return r;
    if(!w32(0x2cu,0u,Stage::WriteMap2C))return r;
    r.stage=Stage::ReadCallback0ForSelector;
    std::uint32_t prior=0u;
    if(!host_.read_u32(object,prior))return r;
    if(prior==0u){
      const auto method=(selector&0xffu)!=0u?
          kStringComparatorVA:kBytewiseLessComparatorVA;
      r.selected_comparator=method;
      if(!w32(0u,method,Stage::WriteDefaultComparator0))return r;
    }else r.selected_comparator=prior;
    r.code=Code::Completed;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
Result ArrayRelease::release(std::uint32_t object)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result r{};r.object_va=object;
  if(object==0u){r.code=Code::NullObjectHazard;return r;}
  try {
    std::uint32_t step_va=0,arr_va=0,cap_va=0;
    if(!address(object,0xcu,step_va)||!address(object,4u,arr_va)||
       !address(object,8u,cap_va)){
      r.code=Code::AddressOverflow;return r;
    }
    std::uint32_t step=0;
    r.stage=Stage::ReadStepCForRelease;
    if(!host_.read_u32(step_va,step))return r;
    if(step==kDisabledStep){r.skipped_disabled=true;r.code=Code::Completed;return r;}
    std::uint32_t pointer=0;
    r.stage=Stage::ReadArray4ForRelease;
    if(!host_.read_u32(arr_va,pointer))return r;
    r.allocated_pointer=pointer;
    if(pointer!=0u){
      r.stage=Stage::FreeOriginal2226e3e0;
      r.release_called=true;
      if(!host_.free_original_2226e3e0(pointer))return r;
      r.stage=Stage::ClearArray4;
      if(!host_.write_u32(arr_va,0u))return r;
      ++r.writes_committed;
    }
    r.stage=Stage::ClearCapacity8;
    if(!host_.write_u32(cap_va,0u))return r;
    ++r.writes_committed;
    r.code=Code::Completed;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
} // namespace sin_compat::original_nested_map_phase647
