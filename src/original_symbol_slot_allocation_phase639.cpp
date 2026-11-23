#include "sin_compat/original_symbol_slot_allocation_phase639.hpp"
#include <limits>
namespace sin_compat::original_symbol_slot_allocation_phase639 {
namespace {
struct Guard {
  bool& busy;
  explicit Guard(bool& value)noexcept:busy(value){busy=true;}
  ~Guard()noexcept{busy=false;}
};
constexpr bool addr(std::uint32_t base,std::uint32_t offset,
                    std::uint32_t& result)noexcept {
  if(base==0u || base>std::numeric_limits<std::uint32_t>::max()-offset)
    return false;
  result=base+offset;
  return true;
}
}
Result AcquireSlot::acquire(std::uint32_t table)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result r{};r.table_va=table;
  if(table==0u){r.code=Code::NullTableHazard;return r;}
  auto read16=[&](Stage stage,std::uint32_t base,std::uint32_t offset,
                  std::uint16_t& output)->bool {
    r.stage=stage;std::uint32_t address=0;
    if(!addr(base,offset,address)){r.code=Code::AddressOverflow;return false;}
    return host_.read_u16(address,output);
  };
  auto read32=[&](Stage stage,std::uint32_t base,std::uint32_t offset,
                  std::uint32_t& output)->bool {
    r.stage=stage;std::uint32_t address=0;
    if(!addr(base,offset,address)){r.code=Code::AddressOverflow;return false;}
    return host_.read_u32(address,output);
  };
  auto write16=[&](Stage stage,std::uint32_t base,std::uint32_t offset,
                   std::uint16_t value)->bool {
    r.stage=stage;std::uint32_t address=0;
    if(!addr(base,offset,address)){r.code=Code::AddressOverflow;return false;}
    if(!host_.write_u16(address,value))return false;
    ++r.writes_committed;return true;
  };
  auto write32=[&](Stage stage,std::uint32_t base,std::uint32_t offset,
                   std::uint32_t value)->bool {
    r.stage=stage;std::uint32_t address=0;
    if(!addr(base,offset,address)){r.code=Code::AddressOverflow;return false;}
    if(!host_.write_u32(address,value))return false;
    ++r.writes_committed;return true;
  };
  try {
    std::uint16_t free_head=0xffffu;
    if(!read16(Stage::ReadFreeHead14,table,0x14u,free_head))return r;
    if(free_head==kInvalidIndex) {
      std::uint16_t next=0u;
      if(!read16(Stage::ReadNextIndex16,table,0x16u,next))return r;
      std::uint32_t capacity=0;
      if(!read32(Stage::ReadCapacity8,table,8u,capacity))return r;
      if(capacity==next) {
        r.stage=Stage::GrowSlotArray80DD0;
        std::uint32_t array_owner=0u;
        if(!addr(table,4u,array_owner)){
          r.code=Code::AddressOverflow;return r;
        }
        if(!host_.grow_slot_array_22280dd0(array_owner,1u))return r;
        r.grew_slots=true;
      }
      // Native re-reads next-index after the possible capacity-growth call.
      if(!read16(Stage::ReadNextIndex16,table,0x16u,next))return r;
      r.handle=next;
      if(!write16(Stage::WriteNextIndex16,table,0x16u,
                  static_cast<std::uint16_t>(next+1u)))return r;
    } else {
      // Native free-list +2 is read through the original 0x22280D40.
      r.used_free_list=true;
      r.handle=free_head;
      std::uint16_t next=0xffffu;
      r.stage=Stage::QueryFreeNode80D40;
      if(!host_.get_free_slot_next_22280d40(table,free_head,next))return r;
      if(!write16(Stage::WriteFreeHead14,table,0x14u,next))return r;
    }
    // Original computes [table+4] + 16*handle + 8, optionally writes FFFF.
    // It checks only the calculated pointer for null, NOT actual capacity.
    std::uint32_t slots=0;
    if(!read32(Stage::ReadSlots4,table,4u,slots))return r;
    if(slots==0u){r.code=Code::NullSlotsHazard;return r;}
    if(!addr(slots,static_cast<std::uint32_t>(r.handle)*kSlotStride+8u,
             r.slot_va)){
      r.code=Code::AddressOverflow;return r;
    }
    if(!write16(Stage::WriteSlotFreeWord8,r.slot_va,0u,0xffffu))return r;
    // Exact original follows with a fresh read of [table+4] and the
    // source-owned cache write to table+0x18, AFTER the slot reset.
    std::uint32_t refreshed=0;
    if(!read32(Stage::ReadSlotsAgain4,table,4u,refreshed))return r;
    if(!write32(Stage::WriteSlotCache18,table,0x18u,refreshed))return r;
    r.code=Code::Completed;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
Result RegisterSlot::register_record(std::uint32_t table,
      std::uint32_t record,std::uint32_t second_arg)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result r{};r.table_va=table;
  if(table==0u){r.code=Code::NullTableHazard;return r;}
  if(record==0u){r.code=Code::NullRecordHazard;return r;}
  std::uint32_t stack_first=record,stack_second=second_arg;
  try {
    r.stage=Stage::Enumerate80FB0;
    if(!host_.enumerate_22280fb0(table,record,stack_first,stack_second))
      return r;
    r.enumerated=true;
    r.rewritten_stack_entry_word=stack_first;
    r.rewritten_stack_second_word=stack_second;
    r.stage=Stage::Acquire810E0;
    const auto acquired=acquire_.acquire(table);
    r.handle=acquired.handle;
    r.writes_committed+=acquired.writes_committed;
    r.used_free_list=acquired.used_free_list;
    r.grew_slots=acquired.grew_slots;
    if(acquired.code!=Code::Completed){
      r.code=acquired.code;
      r.stage=acquired.stage;
      return r;
    }
    r.acquired=true;
    // The enumerator may rewrite caller stack args S+4/S+8; the link
    // helper receives these *rewritten* words, not the initial pointer.
    r.stage=Stage::Link81F30;
    if(!host_.link_slot_22281f30(table,r.handle,stack_first,stack_second))
      return r;
    r.linked=true;
    std::uint32_t slots_address=0;
    r.stage=Stage::ReadSlotsAfterLink4;
    if(!addr(table,4u,slots_address)){
      r.code=Code::AddressOverflow;return r;
    }
    std::uint32_t slots=0;
    if(!host_.read_u32(slots_address,slots))return r;
    std::uint32_t count_address=0;
    r.stage=Stage::ReadActiveCount12;
    if(!addr(table,0x12u,count_address)){
      r.code=Code::AddressOverflow;return r;
    }
    std::uint16_t count=0;
    if(!host_.read_u16(count_address,count))return r;
    r.stage=Stage::WriteActiveCount12;
    if(!host_.write_u16(count_address,
           static_cast<std::uint16_t>(count+1u)))return r;
    ++r.writes_committed;
    r.active_count_incremented=true;
    // Native LEA checks ONLY computed [array+16*handle+8] for zero.
    if(slots==0u){r.code=Code::NullSlotsHazard;return r;}
    if(!addr(slots,static_cast<std::uint32_t>(r.handle)*kSlotStride+8u,
             r.slot_va)){
      r.code=Code::AddressOverflow;return r;
    }
    std::uint16_t first=0;
    r.stage=Stage::ReadRecordWord0;
    if(!host_.read_u16(record,first))return r;
    r.stage=Stage::WriteSlotRecordWord8;
    if(!host_.write_u16(r.slot_va,first))return r;
    ++r.writes_committed;
    std::uint32_t src4=0,rec4=0,dest4=0;
    r.stage=Stage::ReadRecordDword4;
    if(!addr(record,4u,rec4) || !addr(r.slot_va,4u,dest4)){
      r.code=Code::AddressOverflow;return r;
    }
    if(!host_.read_u32(rec4,src4))return r;
    r.stage=Stage::WriteSlotRecordDwordC;
    if(!host_.write_u32(dest4,src4))return r;
    ++r.writes_committed;
    r.copied_record=true;r.code=Code::Completed;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
} // namespace sin_compat::original_symbol_slot_allocation_phase639
