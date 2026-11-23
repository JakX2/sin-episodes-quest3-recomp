#include "sin_compat/original_nested_clear_phase655.hpp"
#include <limits>

namespace sin_compat::original_nested_clear_phase655 {
namespace {
struct Guard {
  bool& busy;
  explicit Guard(bool& value) noexcept : busy(value) { busy=true; }
  ~Guard() noexcept { busy=false; }
};
bool addr(std::uint32_t base, std::uint32_t offset,
          std::uint32_t& out) noexcept {
  if (base==0u || base>std::numeric_limits<std::uint32_t>::max()-offset)
    return false;
  out=base+offset;
  return true;
}
std::int64_t signed32(std::uint32_t v) noexcept {
  return v <= 0x7fffffffu ? static_cast<std::int64_t>(v) :
                            static_cast<std::int64_t>(v)-0x100000000ll;
}
}

Result ClearSlots::run(std::uint32_t map) noexcept {
  Result result{};result.map_va=map;
  if(busy_) { result.code=Code::Reentrant;return result; }
  if(!map) { result.code=Code::NullOwnerHazard;return result; }
  Guard guard(busy_);
  try {
    std::uint32_t next_va=0,root_va=0,count_va=0,head_va=0;
    if(!addr(map,0x16u,next_va) || !addr(map,0x10u,root_va) ||
       !addr(map,0x12u,count_va) || !addr(map,0x14u,head_va)){
      result.code=Code::AddressOverflow;return result;
    }
    std::uint16_t next=0;
    result.stage=Stage::ReadNextUnused16;
    if(!host_.read_u16(next_va,next))return result;
    // Original starts at zero-extended WORD next_unused minus one.
    // For zero nodes, it does not initialize the sentinel or touch any slots.
    if(next!=0u){
      std::uint8_t flag=0;
      result.stage=Stage::ReadSentinelFlag;
      if(!host_.read_u8(kSentinelFlagVA,flag))return result;
      if(!(flag&1u)){
        result.stage=Stage::InitializeSentinel;
        std::uint32_t flags=0;
        if(!host_.read_u32(kSentinelFlagVA,flags) ||
           !host_.write_u32(kSentinelFlagVA,flags|1u))return result;
        ++result.writes_committed;
        for(std::uint32_t off=0u;off<6u;off+=2u){
          if(!host_.write_u16(kSentinelVA+off,kInvalid))return result;
          ++result.writes_committed;
        }
        if(!host_.write_u16(kSentinelVA+6u,1u))return result;
        ++result.writes_committed;result.sentinel_initialized=true;
      }
      // Original writes WORD[node+2] (next in free list) before WORD[node+0]
      // (self index). Node+4/+6/+8 and next-unused remain untouched.
      std::uint16_t successor=kInvalid;
      for(std::uint32_t i=next;i!=0u;){
        --i;
        std::uint32_t slot_owner=0,array=0,node=0,link=0;
        result.stage=Stage::ReadSlotArray4;
        if(!addr(map,4u,slot_owner)||!host_.read_u32(slot_owner,array))return result;
        if(!array) {result.code=Code::NullArrayHazard;return result;}
        if(!addr(array,i*12u,node)||!addr(node,2u,link)){
          result.code=Code::AddressOverflow;return result;
        }
        result.stage=Stage::WriteFreeRight2;
        if(!host_.write_u16(link,successor))return result;
        ++result.writes_committed;
        result.stage=Stage::WriteFreeLeft0;
        if(!host_.write_u16(node,static_cast<std::uint16_t>(i)))return result;
        ++result.writes_committed;
        successor=static_cast<std::uint16_t>(i);
        ++result.slots_recycled;
      }
    }
    result.stage=Stage::WriteRoot10;
    if(!host_.write_u16(root_va,kInvalid))return result;
    ++result.writes_committed;
    result.stage=Stage::WriteCount12;
    if(!host_.write_u16(count_va,0u))return result;
    ++result.writes_committed;
    result.free_head=(next==0u?kInvalid:0u);
    result.stage=Stage::WriteFreeHead14;
    if(!host_.write_u16(head_va,result.free_head))return result;
    ++result.writes_committed;result.code=Code::Completed;
  } catch(...) {result.code=Code::HostRejected;}
  return result;
}

Result ReleaseBlocks::run(std::uint32_t map) noexcept {
  Result result{};result.map_va=map;
  if(busy_) {result.code=Code::Reentrant;return result;}
  if(!map) {result.code=Code::NullOwnerHazard;return result;}
  Guard guard(busy_);
  try{
    result.stage=Stage::ClearSlotsEC50;result.underlying_clear_called=true;
    const auto cleared=clear_.run(map);
    result.writes_committed+=cleared.writes_committed;
    result.slots_recycled=cleared.slots_recycled;
    result.free_head=cleared.free_head;
    result.sentinel_initialized=cleared.sentinel_initialized;
    if(cleared.code!=Code::Completed){
      result.code=cleared.code;result.stage=cleared.stage;return result;
    }
    std::uint32_t count_va=0,array_va=0;
    if(!addr(map,0x28u,count_va)||!addr(map,0x1cu,array_va)){
      result.code=Code::AddressOverflow;return result;
    }
    std::uint32_t count=0;
    result.stage=Stage::ReadBlockCount28;
    if(!host_.read_u32(count_va,count))return result;
    if(signed32(count)>0){
      if(count>65536u){result.code=Code::BlockLoopGuard;return result;}
      std::uint32_t blocks=0;
      result.stage=Stage::ReadBlockArray1C;
      if(!host_.read_u32(array_va,blocks))return result;
      if(!blocks){result.code=Code::NullArrayHazard;return result;}
      for(std::uint32_t i=0u;i<count;++i){
        std::uint32_t ptr_va=0,block=0;
        if(!addr(blocks,i*4u,ptr_va)){
          result.code=Code::AddressOverflow;return result;
        }
        result.stage=Stage::ReadBlockPointer;
        if(!host_.read_u32(ptr_va,block))return result;
        // The retail call site passes each pointer directly to the
        // allocator wrapper; the original has no skip for null entries.
        result.stage=Stage::FreeBlockE3E0;
        ++result.block_free_calls;
        if(!host_.free_original_2226e3e0(block))return result;
      }
    }
    result.stage=Stage::ClearBlockCount28;
    if(!host_.write_u32(count_va,0u))return result;
    ++result.writes_committed;result.code=Code::Completed;
  }catch(...){result.code=Code::HostRejected;}
  return result;
}

Result FindNameView::run(std::uint32_t map,std::uint32_t output,
                         std::uint32_t name,std::uint32_t scratch_ref) noexcept {
  Result result{};result.map_va=map;result.return_eax=output;
  if(busy_){result.code=Code::Reentrant;return result;}
  if(!output){result.code=Code::NullOwnerHazard;return result;}
  Guard guard(busy_);
  try{
    if(name==0u){
      result.null_name=true;result.handle=kInvalid;
      result.stage=Stage::WriteAbsentHandle;
      if(!host_.write_u16(output,kInvalid))return result;
      ++result.writes_committed;result.code=Code::Completed;return result;
    }
    if(map==0u||scratch_ref==0u){
      result.code=Code::NullOwnerHazard;return result;
    }
    std::uint32_t fallback=0,upper=0;
    if(!addr(map,0x30u,fallback)||!addr(scratch_ref,2u,upper)){
      result.code=Code::AddressOverflow;return result;
    }
    result.stage=Stage::WriteFallback30;
    if(!host_.write_u32(fallback,name))return result;
    ++result.writes_committed;
    result.stage=Stage::WriteScratchInvalid;
    if(!host_.write_u16(scratch_ref,kInvalid))return result;
    ++result.writes_committed;
    if(!host_.write_u16(upper,kInvalid))return result;
    ++result.writes_committed;
    result.stage=Stage::FindExistingEB40;result.lookup_called=true;
    if(!host_.find_original_2228eb40(map,scratch_ref,map,result.handle))
      return result;
    result.stage=Stage::WriteFoundHandle;
    if(!host_.write_u16(output,result.handle))return result;
    ++result.writes_committed;result.code=Code::Completed;
  }catch(...){result.code=Code::HostRejected;}
  return result;
}
} // namespace sin_compat::original_nested_clear_phase655
