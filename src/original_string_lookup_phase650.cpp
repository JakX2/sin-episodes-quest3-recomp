#include "sin_compat/original_string_lookup_phase650.hpp"
#include <limits>
namespace sin_compat::original_string_lookup_phase650 {
namespace {
struct BusyGuard{bool& flag;explicit BusyGuard(bool& f)noexcept:flag(f){f=true;}
 ~BusyGuard()noexcept{flag=false;}};
constexpr bool at(std::uint32_t base,std::uint32_t offset,
    std::uint32_t& out)noexcept{
 if(base==0u||base>std::numeric_limits<std::uint32_t>::max()-offset)return false;
 out=base+offset;return true;
}
constexpr std::int64_t signed_word(std::uint32_t value)noexcept {
 return value<=0x7fffffffu?static_cast<std::int64_t>(value):
      static_cast<std::int64_t>(value)-0x100000000ll;
}
}
bool TreeSearch::initialize_sentinel(Result& result)noexcept{
 result.stage=Stage::InitializeSentinel;
 std::uint8_t flag=0u;
 if(!host_.read_u8(kSentinelStateVA,flag))return false;
 if((flag&1u)!=0u)return true;
 std::uint32_t state=0u;
 if(!host_.read_u32(kSentinelStateVA,state))return false;
 if(!host_.write_u32(kSentinelStateVA,state|1u))return false;
 ++result.writes_committed;
 for(std::uint32_t i=0;i<3u;++i){
  if(!host_.write_u16(kSentinelWordsVA+2u*i,kInvalid))return false;
  ++result.writes_committed;
 }
 if(!host_.write_u16(kSentinelWordsVA+6u,1u))return false;
 ++result.writes_committed;
 result.sentinel_initialized=true;
 return true;
}
Result TreeSearch::find(std::uint32_t map,std::uint32_t key_ref,
    std::uint32_t context)noexcept {
 if(busy_)return {Code::Reentrant,Stage::None};
 BusyGuard guard(busy_);
 Result r{};r.map_va=map;r.search_key_va=key_ref;r.block_index=0xffffffffu;
 if(map==0u||key_ref==0u||context==0u){r.code=Code::NullPointerHazard;return r;}
 try{
  std::uint32_t addr=0u;
  if(!at(map,0x10u,addr)){r.code=Code::AddressOverflow;return r;}
  r.stage=Stage::ReadRoot10;
  std::uint16_t node=kInvalid;
  if(!host_.read_u16(addr,node))return r;
  if(node==kInvalid){r.code=Code::Completed;return r;}
  for(std::uint32_t pass=0;pass<65536u;++pass){
   std::uint32_t array=0u;
   if(!at(map,4u,addr)){r.code=Code::AddressOverflow;return r;}
   r.stage=Stage::ReadSlotArray4;
   if(!host_.read_u32(addr,array))return r;
   if(array==0u){r.code=Code::NullPointerHazard;return r;}
   const std::uint32_t offset=static_cast<std::uint32_t>(node)*kSlotStride;
   std::uint32_t slot=0u,string_ref=0u;
   if(!at(array,offset,slot)||!at(slot,8u,string_ref)){
     r.code=Code::AddressOverflow;return r;
   }
   std::uint32_t comparator=0u;
   if(!at(map,0u,addr)){r.code=Code::AddressOverflow;return r;}
   r.stage=Stage::CompareKeyNode;
   if(!host_.read_u32(addr,comparator))return r;
   if(comparator==0u){r.code=Code::NullPointerHazard;return r;}
   bool less=false;
   if(!host_.compare_original_nested(comparator,key_ref,string_ref,context,less))return r;
   ++r.comparisons;
   bool is_left=less;
   if(!less){
    r.stage=Stage::CompareNodeKey;
    if(!host_.read_u32(addr,comparator))return r;
    if(comparator==0u){r.code=Code::NullPointerHazard;return r;}
    if(!host_.compare_original_nested(comparator,string_ref,key_ref,context,less))return r;
    ++r.comparisons;
    if(!less){r.handle=node;r.found_existing=true;r.code=Code::Completed;return r;}
   }
   if(!initialize_sentinel(r))return r;
   std::uint32_t child_va=0u;
   if(!at(slot,is_left?0u:2u,child_va)){
      r.code=Code::AddressOverflow;return r;
   }
   r.stage=Stage::ReadChild;
   if(!host_.read_u16(child_va,node))return r;
   if(node==kInvalid){r.code=Code::Completed;return r;}
  }
  r.code=Code::CycleGuard;
 }catch(...){r.code=Code::HostRejected;}
 return r;
}
Result BlockFit::find(std::uint32_t map,std::uint32_t required)noexcept{
 if(busy_)return {Code::Reentrant,Stage::None};
 BusyGuard guard(busy_);
 Result r{};r.map_va=map;r.bytes_required=required;r.block_index=0xffffffffu;
 if(map==0u){r.code=Code::NullPointerHazard;return r;}
 try{
  std::uint32_t count_va=0u;
  if(!at(map,0x28u,count_va)){r.code=Code::AddressOverflow;return r;}
  r.stage=Stage::ReadBlockCount28;
  std::uint32_t count=0u;
  if(!host_.read_u32(count_va,count))return r;
  if(signed_word(count)<=0u){r.code=Code::Completed;return r;}
  if(count>65536u){r.code=Code::CycleGuard;return r;}
  std::uint32_t pointers_va=0u;
  if(!at(map,0x1cu,pointers_va)){r.code=Code::AddressOverflow;return r;}
  r.stage=Stage::ReadBlockPointers1C;
  std::uint32_t blocks=0u;
  if(!host_.read_u32(pointers_va,blocks))return r;
  if(blocks==0u){r.code=Code::NullPointerHazard;return r;}
  for(std::uint32_t i=0;i<count;++i){
   std::uint32_t p=0u,base=0u,cap=0u,used=0u,at_capacity=0u;
   if(!at(blocks,i*4u,p)){r.code=Code::AddressOverflow;return r;}
   r.stage=Stage::ReadBlockPointer;
   if(!host_.read_u32(p,base))return r;
   if(base==0u){r.code=Code::NullPointerHazard;return r;}
   r.stage=Stage::ReadBlockCapacity;
   if(!host_.read_u32(base,cap))return r;
   if(!at(base,4u,at_capacity)){r.code=Code::AddressOverflow;return r;}
   r.stage=Stage::ReadBlockUsed;
   if(!host_.read_u32(at_capacity,used))return r;
   // Retail CMP / JGE is signed, with 32-bit wrapping subtraction.
   if(signed_word(cap-used)>=signed_word(required)){
    r.block_index=i;r.block_capacity=cap;r.block_used_before=used;
    r.code=Code::Completed;return r;
   }
  }
  r.code=Code::Completed;
 }catch(...){r.code=Code::HostRejected;}
 return r;
}
Result LookupOrInsert::run(std::uint32_t map,std::uint32_t output_word,
       std::uint32_t name,std::uint32_t scratch_ref)noexcept{
 if(busy_)return {Code::Reentrant,Stage::None};
 BusyGuard guard(busy_);
 Result r{};r.map_va=map;r.output_va=output_word;
 r.search_key_va=scratch_ref;r.return_eax=output_word;
 if(map==0u||output_word==0u){r.code=Code::NullPointerHazard;return r;}
 try{
  const auto w16=[&](std::uint32_t addr,std::uint16_t val,Stage stage)->bool{
   r.stage=stage;
   if(!host_.write_u16(addr,val))return false;
   ++r.writes_committed;return true;
  };
  const auto w32=[&](std::uint32_t addr,std::uint32_t val,Stage stage)->bool{
   r.stage=stage;
   if(!host_.write_u32(addr,val))return false;
   ++r.writes_committed;return true;
  };
  if(name==0u){
   r.original_null_name=true;
   if(!w16(output_word,kInvalid,Stage::WriteOutputHandle))return r;
   r.code=Code::Completed;return r;
  }
  if(scratch_ref==0u){r.code=Code::NullPointerHazard;return r;}
  std::uint32_t fallback_va=0u, second_va=0u;
  if(!at(map,0x30u,fallback_va)||!at(scratch_ref,2u,second_va)){
   r.code=Code::AddressOverflow;return r;
  }
  if(!w32(fallback_va,name,Stage::WriteFallback30))return r;
  if(!w16(scratch_ref,kInvalid,Stage::WriteSentinelRef))return r;
  if(!w16(second_va,kInvalid,Stage::WriteSentinelRef))return r;
  r.stage=Stage::SearchExisting;
  const auto hit=search_.find(map,scratch_ref,map);
  r.comparisons=hit.comparisons;
  r.writes_committed+=hit.writes_committed;
  r.sentinel_initialized=hit.sentinel_initialized;
  if(hit.code!=Code::Completed){r.code=hit.code;r.stage=hit.stage;return r;}
  if(hit.found_existing){
   r.handle=hit.handle;r.found_existing=true;
   if(!w16(output_word,r.handle,Stage::WriteOutputHandle))return r;
   r.code=Code::Completed;return r;
  }
  // Native strlen()+1 reads and copies the NUL; portable cap rejects
  // unbounded guest input instead of following arbitrary memory forever.
  r.stage=Stage::ReadName;
  bool terminated=false;
  for(std::uint32_t i=0;i<65536u;++i){
   std::uint32_t va=0u;
   if(!at(name,i,va)){r.code=Code::AddressOverflow;return r;}
   std::uint8_t ch=0u;
   if(!host_.read_u8(va,ch))return r;
   if(ch==0u){r.bytes_required=i+1u;terminated=true;break;}
  }
  if(!terminated){r.code=Code::CycleGuard;return r;}
  const auto fit=fit_.find(map,r.bytes_required);
  if(fit.code!=Code::Completed){r.code=fit.code;r.stage=fit.stage;return r;}
  std::uint32_t block_index=fit.block_index;
  if(block_index==0xffffffffu){
   const std::uint32_t capacity=r.bytes_required>kDefaultBlockCapacity?
       r.bytes_required:kDefaultBlockCapacity;
   r.block_capacity=capacity;
   const std::uint32_t native_bytes=capacity+kBlockAllocationExtra;
   r.stage=Stage::AllocateBlock;
   std::uint32_t new_block=0u;
   if(!host_.allocate_original_2226e390(native_bytes,new_block))return r;
   if(new_block==0u){r.code=Code::NullPointerHazard;return r;}
   r.allocated_block=true;
   if(!w32(new_block,capacity,Stage::InitializeBlock))return r;
   std::uint32_t used_va=0u;
   if(!at(new_block,4u,used_va)){r.code=Code::AddressOverflow;return r;}
   if(!w32(used_va,0u,Stage::InitializeBlock))return r;
   std::uint32_t owner=0u;
   if(!at(map,0x1cu,owner)){r.code=Code::AddressOverflow;return r;}
   r.stage=Stage::AppendBlockPointer;
   if(!host_.append_original_2228ed80(owner,new_block,block_index))return r;
  }
  r.block_index=block_index;
  std::uint32_t field=0u,blocks=0u,pointer=0u,block=0u,used_va=0u;
  if(!at(map,0x1cu,field)){r.code=Code::AddressOverflow;return r;}
  r.stage=Stage::ReadBlockPointers1C;
  if(!host_.read_u32(field,blocks))return r;
  if(blocks==0u){r.code=Code::NullPointerHazard;return r;}
  if(block_index>0xffffu){r.code=Code::AddressOverflow;return r;}
  if(!at(blocks,block_index*4u,pointer)){r.code=Code::AddressOverflow;return r;}
  r.stage=Stage::ReadBlockPointer;
  if(!host_.read_u32(pointer,block))return r;
  if(block==0u){r.code=Code::NullPointerHazard;return r;}
  if(!at(block,4u,used_va)){r.code=Code::AddressOverflow;return r;}
  r.stage=Stage::ReadBlockUsed;
  if(!host_.read_u32(used_va,r.block_used_before))return r;
  for(std::uint32_t i=0;i<r.bytes_required;++i){
   std::uint32_t from=0u,to=0u;
   if(!at(name,i,from)||!at(block,kBlockHeaderBytes+r.block_used_before+i,to)){
    r.code=Code::AddressOverflow;return r;
   }
   std::uint8_t ch=0u;
   r.stage=Stage::CopyString;
   if(!host_.read_u8(from,ch)||!host_.write_u8(to,ch))return r;
   ++r.bytes_copied;++r.writes_committed;
  }
  if(!w32(used_va,r.block_used_before+r.bytes_required,Stage::AdvanceBlockUsed))return r;
  if(!w16(scratch_ref,static_cast<std::uint16_t>(block_index),Stage::WriteInsertionRef))return r;
  if(!w16(second_va,static_cast<std::uint16_t>(r.block_used_before),Stage::WriteInsertionRef))return r;
  r.stage=Stage::RegisterRef;
  if(!host_.register_original_2228f710(map,scratch_ref,r.handle))return r;
  r.inserted=true;
  if(!w16(output_word,r.handle,Stage::WriteOutputHandle))return r;
  r.code=Code::Completed;
 }catch(...){r.code=Code::HostRejected;}
 return r;
}
} // namespace sin_compat::original_string_lookup_phase650
