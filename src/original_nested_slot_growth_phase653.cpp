#include "sin_compat/original_nested_slot_growth_phase653.hpp"
#include <limits>
namespace sin_compat::original_nested_slot_growth_phase653 {
namespace {
struct Guard{bool& busy;explicit Guard(bool& b)noexcept:busy(b){b=true;}
 ~Guard()noexcept{busy=false;}};
constexpr bool addr(std::uint32_t base,std::uint32_t delta,
                    std::uint32_t& value)noexcept{
 if(base==0u||base>std::numeric_limits<std::uint32_t>::max()-delta)return false;
 value=base+delta;return true;
}
constexpr std::int64_t as_signed32(std::uint32_t v)noexcept{
 return v<=0x7fffffffu?static_cast<std::int64_t>(v):
  static_cast<std::int64_t>(v)-0x100000000ll;
}
}
Result ResolveSlot::resolve(std::uint32_t map,std::uint16_t slot)noexcept{
 if(busy_)return {Code::Reentrant,Stage::None};
 Guard guard(busy_);
 Result r{};r.owner_va=map;r.index=slot;
 if(map==0u){r.code=Code::NullOwnerHazard;return r;}
 try{
  std::uint8_t flag=0;
  r.stage=Stage::ReadSentinelFlag;
  if(!host_.read_u8(kSentinelFlagVA,flag))return r;
  if((flag&1u)==0u){
   std::uint32_t full=0;
   r.stage=Stage::ReadSentinelFlagDWORD;
   if(!host_.read_u32(kSentinelFlagVA,full))return r;
   r.stage=Stage::SetSentinelFlag;
   if(!host_.write_u32(kSentinelFlagVA,full|1u))return r;
   ++r.writes_committed;
   for(std::uint32_t delta=0;delta<6u;delta+=2u){
    r.stage=(delta==0u?Stage::WriteSentinelLeft:
             delta==2u?Stage::WriteSentinelRight:Stage::WriteSentinelParent);
    if(!host_.write_u16(kSentinelVA+delta,kInvalid))return r;
    ++r.writes_committed;
   }
   r.stage=Stage::WriteSentinelBlack;
   if(!host_.write_u16(kSentinelVA+6u,1u))return r;
   ++r.writes_committed;r.sentinel_initialized=true;
  }
  if(slot==kInvalid){
   r.result_pointer=kSentinelVA;r.sentinel_returned=true;
   r.code=Code::Completed;return r;
  }
  std::uint32_t ptr_va=0;
  if(!addr(map,4u,ptr_va)){r.code=Code::AddressOverflow;return r;}
  std::uint32_t base=0;
  r.stage=Stage::ReadArray4;
  if(!host_.read_u32(ptr_va,base))return r;
  if(base==0u){r.code=Code::NullArrayHazard;return r;}
  if(!addr(base,static_cast<std::uint32_t>(slot)*kStride,r.result_pointer)){
   r.code=Code::AddressOverflow;return r;
  }
  r.code=Code::Completed;
 }catch(...){r.code=Code::HostRejected;}
 return r;
}
Result GrowSlots::grow(std::uint32_t owner,std::uint32_t increment)noexcept{
 if(busy_)return {Code::Reentrant,Stage::None};
 Guard guard(busy_);
 Result r{};r.owner_va=owner;
 if(owner==0u){r.code=Code::NullOwnerHazard;return r;}
 try{
  std::uint32_t step_va=0,capacity_va=0;
  if(!addr(owner,8u,step_va)||!addr(owner,4u,capacity_va)){
   r.code=Code::AddressOverflow;return r;
  }
  std::uint32_t step=0;
  r.stage=Stage::ReadStep8;
  if(!host_.read_u32(step_va,step))return r;
  if(step==kDisabled){r.growth_disabled=true;r.code=Code::Completed;return r;}
  std::uint32_t cap=0;
  r.stage=Stage::ReadCapacity4;
  if(!host_.read_u32(capacity_va,cap))return r;
  r.old_capacity=cap;
  const std::uint32_t required=cap+increment; // exact original wrapped ADD
  while(as_signed32(cap)<as_signed32(required)){
   if(++r.growth_rounds>65536u){r.code=Code::GrowthLoopGuard;return r;}
   cap=cap==0u?kInitialCapacity:step==0u?cap*2u:cap+step;
   r.stage=Stage::WriteCapacity4;
   if(!host_.write_u32(capacity_va,cap))return r;
   ++r.writes_committed;
  }
  r.new_capacity=cap;
  r.alloc_bytes=cap*kStride; // native LEA plus two ADD, wrapping DWORD
  std::uint32_t prior=0,allocation=0;
  r.stage=Stage::ReadArray4;
  if(!host_.read_u32(owner,prior))return r;
  if(prior!=0u){
   r.stage=Stage::Reallocate400;r.reallocation_called=true;
   if(!host_.reallocate_original_2226e400(prior,r.alloc_bytes,allocation))return r;
  }else{
   r.stage=Stage::Allocate390;r.allocation_called=true;
   if(!host_.allocate_original_2226e390(r.alloc_bytes,allocation))return r;
  }
  r.stage=Stage::WriteArray0;
  if(!host_.write_u32(owner,allocation))return r;
  ++r.writes_committed;
  r.result_pointer=allocation;r.code=Code::Completed;
 }catch(...){r.code=Code::HostRejected;}
 return r;
}
} // namespace sin_compat::original_nested_slot_growth_phase653
