#include "sin_compat/original_symbol_rotations_phase641.hpp"
#include <limits>
namespace sin_compat::original_symbol_rotations_phase641 {
namespace {
struct Guard {
  bool& flag;
  explicit Guard(bool& value)noexcept:flag(value){flag=true;}
  ~Guard()noexcept{flag=false;}
};
constexpr bool addr(std::uint32_t base,std::uint32_t offset,
                    std::uint32_t& value)noexcept {
  if(base==0u || base>std::numeric_limits<std::uint32_t>::max()-offset)
    return false;
  value=base+offset;return true;
}
}
AccessResult SentinelSlotAccessor::resolve(std::uint32_t table,
                                           std::uint32_t index)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  AccessResult r{};
  if(table==0u){r.code=Code::NullTableHazard;return r;}
  try {
    std::uint8_t marker=0;
    r.stage=Stage::ReadSentinelInitFlag;
    if(!host_.read_u8(kSentinelInitFlags,marker))return r;
    if((marker&1u)==0u) {
      std::uint32_t dword=0;
      r.stage=Stage::ReadSentinelInitDword;
      if(!host_.read_u32(kSentinelInitFlags,dword))return r;
      r.stage=Stage::WriteSentinelInitFlag;
      if(!host_.write_u32(kSentinelInitFlags,dword|1u))return r;
      ++r.writes_committed;
      constexpr Stage stages[]={Stage::InitSentinel0,Stage::InitSentinel2,
                                Stage::InitSentinel4,Stage::InitSentinel6};
      for(std::uint32_t i=0;i<4u;++i){
        r.stage=stages[i];
        if(!host_.write_u16(kSentinelNode+i*2u,
                            static_cast<std::uint16_t>(i==3u?1u:kNullIndex)))return r;
        ++r.writes_committed;
      }
      r.initialized_sentinel=true;
    }
    if(static_cast<std::uint16_t>(index)==kNullIndex){
      r.slot_va=kSentinelNode;
    } else {
      r.stage=Stage::ReadSlotArray;
      std::uint32_t field=0,array=0;
      if(!addr(table,4u,field)){r.code=Code::AddressOverflow;return r;}
      if(!host_.read_u32(field,array))return r;
      if(array==0u){r.code=Code::NullArrayHazard;return r;}
      r.stage=Stage::ResolveSlot;
      if(!addr(array,kSlotStride*static_cast<std::uint16_t>(index),r.slot_va)){
        r.code=Code::AddressOverflow;return r;
      }
    }
    r.code=Code::Completed;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
RotateResult RotateNode::left(std::uint32_t table,std::uint32_t index)noexcept {
  return run(table,index,false);
}
RotateResult RotateNode::right(std::uint32_t table,std::uint32_t index)noexcept {
  return run(table,index,true);
}
RotateResult RotateNode::run(std::uint32_t table,std::uint32_t index32,
                             bool right_rotation)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  RotateResult r{};r.table_va=table;r.index=static_cast<std::uint16_t>(index32);
  if(table==0u){r.code=Code::NullTableHazard;return r;}
  try {
    auto failaddress=[&](){r.code=Code::AddressOverflow;return false;};
    auto read=[&](std::uint32_t address,std::uint16_t& output)->bool {
      r.stage=Stage::ReadNodeWord;
      return host_.read_u16(address,output);
    };
    auto write=[&](std::uint32_t address,std::uint16_t value,bool root=false)->bool {
      r.stage=root?Stage::WriteRootWord:Stage::WriteNodeWord;
      if(!host_.write_u16(address,value))return false;
      ++r.writes_committed;
      return true;
    };
    auto direct=[&](std::uint16_t index,std::uint32_t offset,
                    std::uint32_t& address)->bool {
      r.stage=Stage::ReadSlotArray;
      std::uint32_t field=0,storage=0;
      if(!addr(table,4u,field))return failaddress();
      if(!host_.read_u32(field,storage))return false;
      if(storage==0u){r.code=Code::NullArrayHazard;return false;}
      r.stage=Stage::ResolveSlot;
      return addr(storage,kSlotStride*index+offset,address)||failaddress();
    };
    auto from_helper=[&](std::uint32_t idx,std::uint32_t offset,
                         std::uint32_t& address)->bool {
      r.stage=Stage::ResolveNodeWord;
      const auto found=accessor_.resolve(table,idx);
      ++r.accessor_calls;
      r.writes_committed+=found.writes_committed;
      if(found.initialized_sentinel)++r.sentinel_initializations;
      if(found.code!=Code::Completed){r.code=found.code;return false;}
      return addr(found.slot_va,offset,address)||failaddress();
    };
    auto node_read=[&](std::uint32_t idx,std::uint32_t offset,
                       std::uint16_t& value)->bool {
      std::uint32_t address=0;
      return from_helper(idx,offset,address)&&read(address,value);
    };
    auto direct_write=[&](std::uint16_t idx,std::uint32_t off,
                          std::uint16_t value)->bool {
      std::uint32_t address=0;
      return direct(idx,off,address)&&write(address,value);
    };
    const std::uint32_t ahead=right_rotation?0u:2u;
    const std::uint32_t opposite=right_rotation?2u:0u;
    // Exact native order: 0x22280D40(index), 0x22280D40(pivot), then
    // fresh direct [table+4] read before the first x-child write.
    if(!node_read(index32,ahead,r.pivot))return r;
    if(!node_read(r.pivot,opposite,r.transferred))return r;
    if(!direct_write(r.index,ahead,r.transferred))return r;
    // Native invokes the accessor AGAIN to inspect the pivot child; it
    // does not reuse the first accessor's result or copied value.
    std::uint16_t transfer_now=kNullIndex;
    if(!node_read(r.pivot,opposite,transfer_now))return r;
    if(transfer_now!=kNullIndex){
      if(!node_read(r.pivot,opposite,transfer_now))return r;
      if(!direct_write(transfer_now,4u,r.index))return r;
    }
    // If the pivot is non-sentinel, copy x's CURRENT parent into pivot+4.
    if(r.pivot!=kNullIndex){
      std::uint16_t oldparent=kNullIndex;
      if(!node_read(index32,4u,oldparent))return r;
      if(!direct_write(r.pivot,4u,oldparent))return r;
    }
    r.stage=Stage::ReadRootWord;
    std::uint32_t root_field=0;
    if(!addr(table,0x10u,root_field)) {failaddress();return r;}
    std::uint16_t root=kNullIndex;
    if(!host_.read_u16(root_field,root))return r;
    if(root==r.index){
      if(!write(root_field,r.pivot,true))return r;
      r.root_updated=true;
    }else{
      std::uint16_t parent_first=kNullIndex;
      if(!node_read(index32,4u,parent_first))return r;
      std::uint16_t child_on_comparison_side=kNullIndex;
      if(!node_read(parent_first,right_rotation?2u:0u,
                    child_on_comparison_side))return r;
      // Original re-fetches x.parent before writing the parent field.
      std::uint16_t parent_refreshed=kNullIndex;
      if(!node_read(index32,4u,parent_refreshed))return r;
      const std::uint32_t destination=(child_on_comparison_side==r.index)?
          (right_rotation?2u:0u):(right_rotation?0u:2u);
      if(!direct_write(parent_refreshed,destination,r.pivot))return r;
      r.parent_updated=true;
    }
    if(!direct_write(r.pivot,opposite,r.index))return r;
    if(r.index!=kNullIndex){
      if(!direct_write(r.index,4u,r.pivot))return r;
    }
    r.code=Code::Completed;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
} // namespace sin_compat::original_symbol_rotations_phase641
