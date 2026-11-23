#include "sin_compat/original_nested_insertion_phase651.hpp"
#include <limits>
namespace sin_compat::original_nested_insertion_phase651 {
namespace {
struct Guard{bool& flag;explicit Guard(bool& f)noexcept:flag(f){f=true;}
 ~Guard()noexcept{flag=false;}};
constexpr bool at(std::uint32_t base,std::uint32_t offset,
                  std::uint32_t& out)noexcept {
 if(base==0u || base>std::numeric_limits<std::uint32_t>::max()-offset)return false;
 out=base+offset;return true;
}
constexpr std::int64_t signed32(std::uint32_t v)noexcept {
 return v<=0x7fffffffu?static_cast<std::int64_t>(v):
       static_cast<std::int64_t>(v)-0x100000000ll;
}
}
Result ArrayGrowth::grow(std::uint32_t owner,std::uint32_t increment)noexcept {
 if(busy_)return {Code::Reentrant,Stage::None};
 Guard guard(busy_);Result r{};r.owner_va=owner;
 if(owner==0u){r.code=Code::NullOwnerHazard;return r;}
 try {
  std::uint32_t step_va=0,cap_va=0;
  if(!at(owner,8u,step_va)||!at(owner,4u,cap_va)){
   r.code=Code::AddressOverflow;return r;
  }
  std::uint32_t step=0,cap=0;
  r.stage=Stage::ReadGrowthStep8;
  if(!host_.read_u32(step_va,step))return r;
  if(step==kDisabledStep){r.disabled_growth=true;r.code=Code::Completed;return r;}
  r.stage=Stage::ReadCapacity4;
  if(!host_.read_u32(cap_va,cap))return r;
  r.old_capacity=cap;
  const std::uint32_t goal=cap+increment; // exact modulo-2^32 native ADD
  while(signed32(cap)<signed32(goal)){
   if(++r.growth_rounds>65536u){r.code=Code::GrowthLoopGuard;return r;}
   cap=cap==0u?kDefaultCapacity:step==0u?cap*2u:cap+step;
   r.stage=Stage::WriteCapacity4;
   if(!host_.write_u32(cap_va,cap))return r;
   ++r.writes_committed;
  }
  r.new_capacity=cap;
  r.alloc_bytes=cap*4u; // native SHL twice, wrapping DWORD
  std::uint32_t pointer=0u;
  r.stage=Stage::ReadArray0;
  if(!host_.read_u32(owner,pointer))return r;
  std::uint32_t output=0u;
  if(pointer==0u){
   r.stage=Stage::AllocateOriginal390;r.allocation_called=true;
   if(!host_.allocate_original_2226e390(r.alloc_bytes,output))return r;
  }else{
   r.stage=Stage::ReallocateOriginal400;r.reallocation_called=true;
   if(!host_.reallocate_original_2226e400(pointer,r.alloc_bytes,output))return r;
  }
  r.stage=Stage::WriteArray0;
  if(!host_.write_u32(owner,output))return r;
  ++r.writes_committed;r.allocated_pointer=output;
  r.code=Code::Completed;
 }catch(...){r.code=Code::HostRejected;}
 return r;
}
Result BlockArrayInsert::insert(std::uint32_t owner,std::uint32_t index,
                                std::uint32_t source_word)noexcept {
 if(busy_)return {Code::Reentrant,Stage::None};
 Guard guard(busy_);Result r{};r.owner_va=owner;r.returned_index=index;
 if(owner==0u||source_word==0u){r.code=Code::NullOwnerHazard;return r;}
 try {
  std::uint32_t count_va=0,capacity_va=0,cache_va=0;
  if(!at(owner,0xcu,count_va)||!at(owner,4u,capacity_va)||
     !at(owner,0x10u,cache_va)){
   r.code=Code::AddressOverflow;return r;
  }
  std::uint32_t count=0,capacity=0;
  r.stage=Stage::ReadAppendCountC;
  if(!host_.read_u32(count_va,count))return r;
  r.stage=Stage::ReadAppendCapacity4;
  if(!host_.read_u32(capacity_va,capacity))return r;
  const std::uint32_t after=count+1u;
  if(signed32(after)>signed32(capacity)){
   r.growth_called=true;r.stage=Stage::GrowArray280;
   const auto grew=growth_.grow(owner,after-capacity);
   r.writes_committed+=grew.writes_committed;
   r.growth_rounds=grew.growth_rounds;
   r.allocation_called=grew.allocation_called;
   r.reallocation_called=grew.reallocation_called;
   if(grew.code!=Code::Completed){r.code=grew.code;r.stage=grew.stage;return r;}
  }
  // Native ADD [owner+0xC],1 is committed before any array-pointer read.
  r.stage=Stage::IncrementCountC;
  if(!host_.write_u32(count_va,after))return r;
  ++r.writes_committed;
  std::uint32_t array=0;
  r.stage=Stage::ReadAppendArray0;
  if(!host_.read_u32(owner,array))return r;
  r.stage=Stage::WriteAppendCache10;
  if(!host_.write_u32(cache_va,array))return r;
  ++r.writes_committed;
  if(array==0u){r.code=Code::NullArrayHazard;return r;}
  std::uint32_t target=0;
  if(index>std::numeric_limits<std::uint32_t>::max()/4u||
     !at(array,index*4u,target)){
   r.code=Code::AddressOverflow;return r;
  }
  const std::uint32_t count_after_index=after-index-1u;
  if(signed32(count_after_index)>0){
   // Original 0x223A5D10 called with (dst=target+4,src=target,
   // bytes=(count_after-index-1)*4), allowing overlapping ranges.
   std::uint32_t dst=0;
   if(!at(target,4u,dst)){r.code=Code::AddressOverflow;return r;}
   r.move_called=true;r.moved_bytes=count_after_index*4u;
   r.stage=Stage::MoveOriginal5D10;
   if(!host_.move_original_223a5d10(dst,target,r.moved_bytes))return r;
  }
  std::uint32_t value=0u;
  r.stage=Stage::ReadSourceWord;
  if(!host_.read_u32(source_word,value))return r;
  r.stage=Stage::WriteInsertedPointer;
  if(!host_.write_u32(target,value))return r;
  ++r.writes_committed;r.code=Code::Completed;r.inserted_record=true;
 }catch(...){r.code=Code::HostRejected;}
 return r;
}
Result TreeInsert::insert(std::uint32_t map,std::uint32_t key_ref,
                          std::uint32_t context)noexcept {
 if(busy_)return {Code::Reentrant,Stage::None};
 Guard guard(busy_);Result r{};r.owner_va=map;
 if(map==0u||key_ref==0u||context==0u){r.code=Code::NullOwnerHazard;return r;}
 try {
  r.stage=Stage::FindParent470;
  if(!host_.find_parent_original_2228e470(map,key_ref,context,
                                          r.parent,r.branch))return r;
  r.found_parent=true;
  r.stage=Stage::AcquireSlot620;
  if(!host_.acquire_slot_original_2228e620(map,r.handle))return r;
  r.acquired_node=true;
  r.stage=Stage::LinkNode460;
  if(!host_.link_original_2228f460(map,r.handle,r.parent,r.branch))return r;
  r.linked_node=true;
  std::uint32_t count_va=0u,slots_va=0u;
  if(!at(map,0x12u,count_va)||!at(map,4u,slots_va)){
   r.code=Code::AddressOverflow;return r;
  }
  r.stage=Stage::ReadTreeCount12;
  std::uint16_t count=0u;
  if(!host_.read_u16(count_va,count))return r;
  r.stage=Stage::WriteTreeCount12;
  if(!host_.write_u16(count_va,static_cast<std::uint16_t>(count+1u)))return r;
  ++r.writes_committed;
  std::uint32_t slots=0u;
  r.stage=Stage::ReadTreeArray4;
  if(!host_.read_u32(slots_va,slots))return r;
  if(slots==0u){r.code=Code::NullArrayHazard;return r;}
  std::uint32_t record=0u;
  if(!at(slots,static_cast<std::uint32_t>(r.handle)*kTreeStride+8u,record)){
   r.code=Code::AddressOverflow;return r;
  }
  std::uint32_t value=0u;
  r.stage=Stage::ReadReferenceWord;
  if(!host_.read_u32(key_ref,value))return r;
  r.stage=Stage::WriteTreeReference8;
  if(!host_.write_u32(record,value))return r;
  ++r.writes_committed;r.inserted_record=true;r.code=Code::Completed;
 }catch(...){r.code=Code::HostRejected;}
 return r;
}
} // namespace sin_compat::original_nested_insertion_phase651
