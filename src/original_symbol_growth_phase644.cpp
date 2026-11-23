#include "sin_compat/original_symbol_growth_phase644.hpp"
#include <limits>
namespace sin_compat::original_symbol_growth_phase644 {
namespace {
struct Guard {
  bool& busy;
  explicit Guard(bool& b)noexcept:busy(b){busy=true;}
  ~Guard()noexcept{busy=false;}
};
constexpr bool addr(std::uint32_t base,std::uint32_t offset,
                    std::uint32_t& result)noexcept {
  if(base==0u||base>std::numeric_limits<std::uint32_t>::max()-offset)
    return false;
  result=base+offset;return true;
}
// Signed x86 JGE/JL ordering without implementation-defined unsigned->signed
// conversion or signed-overflow UB. The native ADD and SHL wrap at 32 bits.
constexpr bool signed_less(std::uint32_t a,std::uint32_t b)noexcept {
  return (a^0x80000000u)<(b^0x80000000u);
}
}
Result GrowArray::grow(std::uint32_t owner,std::uint32_t increment)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result r{};r.owner_va=owner;r.requested_increment=increment;
  if(owner==0u){r.code=Code::NullOwnerHazard;return r;}
  std::uint32_t cap_addr=0,step_addr=0;
  if(!addr(owner,4u,cap_addr)||!addr(owner,8u,step_addr)){
    r.code=Code::AddressOverflow;return r;
  }
  try {
    std::uint32_t step=0u;
    r.stage=Stage::ReadGrowthStep8;
    if(!host_.read_u32(step_addr,step))return r;
    // Original 0x22280DD9 skips the entire body, including allocation.
    if(step==kDisabledGrowStep){
      r.skipped_disabled=true;r.code=Code::Completed;return r;
    }
    std::uint32_t cap=0u;
    r.stage=Stage::ReadCapacity4;
    if(!host_.read_u32(cap_addr,cap))return r;
    r.initial_capacity=cap;
    // Original EDX = original [owner+4] + requested DWORD, modulo 2^32.
    const std::uint32_t target=cap+increment;
    r.target_capacity_wrapped=target;
    r.stage=Stage::ComputeRequestedCapacity;
    // Original uses signed comparison of EAX/EDX (JGE/JL).
    if(signed_less(cap,target)){
      for(std::uint32_t iter=0;iter<65536u;++iter){
        r.stage=Stage::ReadCapacityForLoop;
        if(!host_.read_u32(cap_addr,cap))return r;
        std::uint32_t newcap=0u;
        if(cap==0u)newcap=2u;
        else if(step==0u)newcap=cap+cap;
        else newcap=cap+step;
        r.stage=Stage::WriteCapacity4;
        if(!host_.write_u32(cap_addr,newcap))return r;
        ++r.capacity_writes;
        ++r.loop_iterations;
        // Native 0x22280E08 compares new [owner+4] after the write,
        // which a guest memory callback may mutate.
        if(!host_.read_u32(cap_addr,cap))return r;
        if(!signed_less(cap,target))break;
        if(iter==65535u){r.code=Code::GrowthLoopGuard;return r;}
      }
    }
    r.final_capacity=cap;
    // Even when growth not needed (increment=0 or signed overflow),
    // the original still calls allocate/reallocate and stores its result.
    r.stage=Stage::ReadArrayPointer0;
    std::uint32_t ptr=0u;
    if(!host_.read_u32(owner,ptr))return r;
    r.old_pointer=ptr;
    r.stage=Stage::ReadCapacityForAllocation;
    if(!host_.read_u32(cap_addr,cap))return r;
    r.final_capacity=cap;
    const std::uint32_t byte_count=cap<<4u; // Original x86 SHL, wrap.
    r.requested_bytes_wrapped=byte_count;
    std::uint32_t updated=0u;
    if(ptr!=0u){
      r.stage=Stage::Reallocate2226e400;
      r.performed_reallocation=true;
      if(!host_.reallocate_original_2226e400(ptr,byte_count,updated))return r;
    }else{
      r.stage=Stage::Allocate2226e390;
      r.performed_initial_allocate=true;
      if(!host_.allocate_original_2226e390(byte_count,updated))return r;
    }
    r.new_pointer=updated;
    r.stage=Stage::StoreArrayPointer0;
    if(!host_.write_u32(owner,updated))return r;
    ++r.pointer_writes;
    r.code=Code::Completed;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
Result ReleaseArray::release(std::uint32_t owner)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result r{};r.owner_va=owner;
  if(owner==0u){r.code=Code::NullOwnerHazard;return r;}
  std::uint32_t step_addr=0u,cap_addr=0u;
  if(!addr(owner,8u,step_addr)||!addr(owner,4u,cap_addr)){
    r.code=Code::AddressOverflow;return r;
  }
  try{
    std::uint32_t step=0u;
    r.stage=Stage::ReadGrowthStep8;
    if(!host_.read_u32(step_addr,step))return r;
    if(step==kDisabledGrowStep){
      r.skipped_disabled=true;r.code=Code::Completed;return r;
    }
    r.stage=Stage::ReadArrayPointerForRelease0;
    std::uint32_t ptr=0u;
    if(!host_.read_u32(owner,ptr))return r;
    r.old_pointer=ptr;
    if(ptr!=0u){
      r.stage=Stage::Free2226e3e0;
      r.performed_free=true;
      if(!host_.free_original_2226e3e0(ptr))return r;
      r.stage=Stage::ClearArrayPointer0;
      if(!host_.write_u32(owner,0u))return r;
      ++r.pointer_writes;
    }
    // Even null array still has capacity reset. Original skips writing
    // null to [owner] when it was already null.
    r.stage=Stage::ClearCapacity4;
    if(!host_.write_u32(cap_addr,0u))return r;
    ++r.capacity_writes;
    r.code=Code::Completed;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
} // namespace sin_compat::original_symbol_growth_phase644
