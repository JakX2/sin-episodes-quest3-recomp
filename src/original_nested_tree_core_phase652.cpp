#include "sin_compat/original_nested_tree_core_phase652.hpp"
#include <limits>
#include <initializer_list>
namespace sin_compat::original_nested_tree_core_phase652 {
namespace {
struct Guard{bool& flag; explicit Guard(bool& f)noexcept:flag(f){f=true;}
 ~Guard()noexcept{flag=false;}};
constexpr bool addr(std::uint32_t b,std::uint32_t delta,std::uint32_t& out)noexcept{
 if(b==0u||b>std::numeric_limits<std::uint32_t>::max()-delta)return false;
 out=b+delta;return true;
}
constexpr bool node(std::uint32_t b,std::uint16_t index,
                    std::uint32_t& out)noexcept {
 return addr(b,static_cast<std::uint32_t>(index)*kStride,out);
}
}
Result FindParent::find(std::uint32_t map,std::uint32_t key,
                       std::uint32_t parent_va,std::uint32_t branch_va,
                       std::uint32_t context)noexcept {
 if(busy_)return {Code::Reentrant,Stage::None};
 Guard g(busy_);
 Result r{};r.owner_va=map;
 if(!map||!key||!parent_va||!branch_va||!context){r.code=Code::NullPointerHazard;return r;}
 try{
  std::uint32_t root_va=0;
  if(!addr(map,0x10u,root_va)){r.code=Code::AddressOverflow;return r;}
  r.stage=Stage::ReadRoot10;
  std::uint16_t index=kInvalid;
  if(!host_.read_u16(root_va,index))return r;
  r.stage=Stage::WriteOutputParent;
  if(!host_.write_u16(parent_va,kInvalid))return r;
  ++r.writes_committed;
  r.stage=Stage::WriteOutputBranch;
  if(!host_.write_u8(branch_va,0u))return r;
  ++r.writes_committed;
  // Original empty tree returns without touching the lazy static sentinel.
  for(;index!=kInvalid;){
   if(++r.visited>65536u){r.code=Code::TraversalCycle;return r;}
   std::uint32_t slot_owner_va=0,base=0,node_va=0,cmp=0,value_va=0;
   if(!addr(map,4u,slot_owner_va)){r.code=Code::AddressOverflow;return r;}
   r.stage=Stage::ReadArray4;
   if(!host_.read_u32(slot_owner_va,base))return r;
   if(!node(base,index,node_va)||!addr(node_va,8u,value_va)){
    r.code=Code::AddressOverflow;return r;
   }
   r.stage=Stage::WriteOutputParent;
   if(!host_.write_u16(parent_va,index))return r;
   ++r.writes_committed;r.parent=index;
   if(!host_.read_u32(map,cmp))return r;
   r.stage=Stage::CompareCallback;
   std::uint8_t al=0;
   if(!host_.compare_original_indirect(cmp,key,value_va,context,al))return r;
   ++r.comparisons;
   r.branch=(al!=0u?1u:0u);
   r.stage=Stage::WriteOutputBranch;
   if(!host_.write_u8(branch_va,r.branch))return r;
   ++r.writes_committed;
   std::uint8_t flag=0;
   if(!host_.read_u8(kSentinelFlagVA,flag))return r;
   if((flag&1u)==0u){
    r.stage=Stage::InitializeSentinel;
    std::uint32_t full=0;
    if(!host_.read_u32(kSentinelFlagVA,full))return r;
    if(!host_.write_u32(kSentinelFlagVA,full|1u))return r;
    ++r.writes_committed;
    for(std::uint32_t off:{0u,2u,4u}){
     if(!host_.write_u16(kSentinelVA+off,kInvalid))return r;
     ++r.writes_committed;
    }
    if(!host_.write_u16(kSentinelVA+6u,1u))return r;
    ++r.writes_committed;r.sentinel_initialized=true;
   }
   // Native branch path: non-sentinel index reads left WORD offset 0,
   // right WORD offset +2, after writing the branch output flag.
   std::uint32_t child_va=0;
   if(!addr(node_va,r.branch?0u:2u,child_va)){
    r.code=Code::AddressOverflow;return r;
   }
   r.stage=r.branch?Stage::ReadLeft0:Stage::ReadRight2;
   if(!host_.read_u16(child_va,index))return r;
  }
  r.code=Code::Completed;
 }catch(...){r.code=Code::HostRejected;}
 return r;
}
Result AcquireSlot::acquire(std::uint32_t map)noexcept {
 if(busy_)return {Code::Reentrant,Stage::None};
 Guard g(busy_);
 Result r{};r.owner_va=map;
 if(!map){r.code=Code::NullPointerHazard;return r;}
 try{
  std::uint32_t head_va=0,next_va=0,capacity_va=0,slots_va=0,cache_va=0;
  if(!addr(map,0x14u,head_va)||!addr(map,0x16u,next_va)||
     !addr(map,8u,capacity_va)||!addr(map,4u,slots_va)||
     !addr(map,0x18u,cache_va)){
   r.code=Code::AddressOverflow;return r;
  }
  r.stage=Stage::ReadFreeHead14;
  std::uint16_t head=kInvalid;
  if(!host_.read_u16(head_va,head))return r;
  if(head==kInvalid){
   std::uint16_t next=0;
   r.stage=Stage::ReadNextIndex16;
   if(!host_.read_u16(next_va,next))return r;
   std::uint32_t capacity=0;
   r.stage=Stage::ReadCapacity8;
   if(!host_.read_u32(capacity_va,capacity))return r;
   if(capacity==next){
    r.stage=Stage::GrowSlots200;r.grow_called=true;
    if(!host_.grow_original_2228e200(slots_va,1u))return r;
   }
   r.stage=Stage::ReadNextIndex16;
   if(!host_.read_u16(next_va,r.handle))return r;
   std::uint32_t slots=0;
   r.stage=Stage::ReadArray4;
   if(!host_.read_u32(slots_va,slots))return r;
   r.stage=Stage::WriteArrayCache18;
   if(!host_.write_u32(cache_va,slots))return r;
   ++r.writes_committed;
   r.stage=Stage::WriteNextIndex16;
   if(!host_.write_u16(next_va,static_cast<std::uint16_t>(r.handle+1u)))return r;
   ++r.writes_committed;
  }else{
   r.used_free_list=true;r.handle=head;
   std::uint32_t freed_node=0;
   r.stage=Stage::ResolveFreeSlot100;
   if(!host_.resolve_original_2228e100(map,head,freed_node))return r;
   if(!addr(freed_node,2u,freed_node)){
    r.code=Code::AddressOverflow;return r;
   }
   std::uint16_t next_head=0;
   r.stage=Stage::ReadFreeNext2;
   if(!host_.read_u16(freed_node,next_head))return r;
   std::uint32_t slots=0;
   r.stage=Stage::ReadArray4;
   if(!host_.read_u32(slots_va,slots))return r;
   r.stage=Stage::WriteArrayCache18;
   if(!host_.write_u32(cache_va,slots))return r;
   ++r.writes_committed;
   r.stage=Stage::WriteFreeHead14;
   if(!host_.write_u16(head_va,next_head))return r;
   ++r.writes_committed;
  }
  r.code=Code::Completed;
 }catch(...){r.code=Code::HostRejected;}
 return r;
}
Result LinkNode::link(std::uint32_t map,std::uint16_t handle,
                      std::uint16_t parent,std::uint8_t branch)noexcept {
 if(busy_)return {Code::Reentrant,Stage::None};
 Guard g(busy_);
 Result r{};r.owner_va=map;r.handle=handle;r.parent=parent;r.branch=branch;
 if(!map){r.code=Code::NullPointerHazard;return r;}
 try{
  std::uint32_t slots_va=0,slots=0;
  if(!addr(map,4u,slots_va)){r.code=Code::AddressOverflow;return r;}
  r.stage=Stage::ReadNodeArray4;
  if(!host_.read_u32(slots_va,slots))return r;
  std::uint32_t current=0;
  if(!node(slots,handle,current)){r.code=Code::AddressOverflow;return r;}
  const auto w=[&](std::uint32_t base,std::uint32_t offset,
                    std::uint16_t value,Stage stage)->bool{
   std::uint32_t dst=0;r.stage=stage;
   if(!addr(base,offset,dst)){r.code=Code::AddressOverflow;return false;}
   if(!host_.write_u16(dst,value))return false;
   ++r.writes_committed;return true;
  };
  if(!w(current,4u,parent,Stage::WriteParent4))return r;
  if(!w(current,2u,kInvalid,Stage::WriteRight2))return r;
  if(!w(current,0u,kInvalid,Stage::WriteLeft0))return r;
  if(!w(current,6u,0u,Stage::WriteColor6))return r;
  if(parent==kInvalid){
   if(!w(map,0x10u,handle,Stage::WriteRoot10))return r;
  }else{
   std::uint32_t p=0;
   if(!node(slots,parent,p)){r.code=Code::AddressOverflow;return r;}
   if(!w(p,branch?0u:2u,handle,branch?Stage::WriteParentLeft0:Stage::WriteParentRight2))return r;
  }
  r.linked=true;r.stage=Stage::RebalanceEDF0;r.rebalance_called=true;
  if(!host_.rebalance_original_2228edf0(map,handle))return r;
  r.code=Code::Completed;
 }catch(...){r.code=Code::HostRejected;}
 return r;
}
} // namespace sin_compat::original_nested_tree_core_phase652
