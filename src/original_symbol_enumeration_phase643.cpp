#include "sin_compat/original_symbol_enumeration_phase643.hpp"
#include <limits>
namespace sin_compat::original_symbol_enumeration_phase643 {
namespace {
struct Guard { bool& busy; explicit Guard(bool& b)noexcept:busy(b){busy=true;}
  ~Guard()noexcept {busy=false;} };
constexpr bool address(std::uint32_t base,std::uint32_t offset,
                       std::uint32_t& out)noexcept {
  if(base==0u || base>std::numeric_limits<std::uint32_t>::max()-offset)
    return false;
  out=base+offset;return true;
}
constexpr std::uint32_t replace_word(std::uint32_t prior,
                                     std::uint16_t value)noexcept {
  return (prior&0xffff0000u)|value;
}
constexpr std::uint32_t replace_byte(std::uint32_t prior,
                                     std::uint8_t value)noexcept {
  return (prior&0xffffff00u)|value;
}
}
Result Enumerate::search(std::uint32_t table,std::uint32_t key,
                         std::uint32_t& parent_word,
                         std::uint32_t& side_word)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result r{};r.table_va=table;r.key_va=key;
  if(table==0u){r.code=Code::NullTableHazard;return r;}
  try {
    std::uint32_t root_va=0,array_field=0;
    if(!address(table,0x10u,root_va)||!address(table,4u,array_field)){
      r.code=Code::AddressOverflow;return r;
    }
    std::uint16_t current=kNullIndex;
    r.stage=Stage::ReadRoot10;
    if(!host_.read_u16(root_va,current))return r;
    // Source 0x22280FC9 and 0x22280FCC: even on an empty tree,
    // set WORD parent FFFF followed by BYTE side 0. Never clobber the
    // other bytes in the containing DWORD caller argument slots.
    r.stage=Stage::InitializeParentWord;
    parent_word=replace_word(parent_word,kNullIndex);
    r.caller_parent_initialized=true;
    r.stage=Stage::InitializeSideByte;
    side_word=replace_byte(side_word,0u);
    r.caller_side_initialized=true;
    if(current==kNullIndex){r.empty_tree=true;r.code=Code::Completed;return r;}
    // Original x86 has no cycle guard. This portability limit makes
    // corrupted or malicious test trees fail closed instead of hanging.
    for(std::uint32_t iteration=0;iteration<65536u;++iteration){
      r.last_parent=current;
      r.stage=Stage::WriteVisitedParentWord;
      parent_word=replace_word(parent_word,current);
      ++r.visited_nodes;
      std::uint32_t slots=0;
      r.stage=Stage::ReadArrayBeforeComparator;
      if(!host_.read_u32(array_field,slots))return r;
      if(slots==0u){r.code=Code::NullArrayHazard;return r;}
      std::uint32_t record_va=0;
      if(!address(slots,static_cast<std::uint32_t>(current)*kSlotStride+8u,
                  record_va)) {r.code=Code::AddressOverflow;return r;}
      std::uint32_t comparator_va=0;
      r.stage=Stage::ReadComparator0;
      if(!host_.read_u32(table,comparator_va))return r;
      if(comparator_va==0u){r.code=Code::NullComparatorHazard;return r;}
      std::uint8_t al=0u;
      r.stage=Stage::CompareOriginalCallback;
      if(!host_.compare_original_22280fb0(table,comparator_va,key,
                                           record_va,al))return r;
      ++r.comparator_calls;
      const std::uint8_t direction=(al!=0u)?1u:0u;
      r.last_side=direction;
      r.stage=Stage::WriteSideByte;
      side_word=replace_byte(side_word,direction);
      // Source 0x2228100B/0x2228105B: inline sentinel initialization
      // after every comparison, even if the next child is FFFF. The
      // first flag DWORD write precedes all four sentinel WORD writes.
      std::uint8_t marker=0u;
      r.stage=Stage::ReadSentinelInitFlag;
      if(!host_.read_u8(kSentinelFlags,marker))return r;
      if((marker&1u)==0u){
        std::uint32_t flags=0;
        r.stage=Stage::ReadSentinelFlagsDword;
        if(!host_.read_u32(kSentinelFlags,flags))return r;
        r.stage=Stage::WriteSentinelFlagsDword;
        if(!host_.write_u32(kSentinelFlags,flags|1u))return r;
        ++r.sentinel_writes_committed;
        constexpr Stage stages[]={Stage::WriteSentinelWord0,
          Stage::WriteSentinelWord2,Stage::WriteSentinelWord4,
          Stage::WriteSentinelWord6};
        for(std::uint32_t i=0;i<4u;++i){
          std::uint32_t dest=0;
          r.stage=stages[i];
          if(!address(kSentinelNode,i*2u,dest)){
            r.code=Code::AddressOverflow;return r;
          }
          if(!host_.write_u16(dest,static_cast<std::uint16_t>(
              i==3u?1u:kNullIndex)))return r;
          ++r.sentinel_writes_committed;
        }
        r.initialized_sentinel=true;
      }
      // The comparator may mutate/reallocate table+4. Source reloads
      // it AFTER callback and sentinel handling on both branch paths.
      r.stage=Stage::ReadArrayAfterComparator;
      if(!host_.read_u32(array_field,slots))return r;
      if(slots==0u){r.code=Code::NullArrayHazard;return r;}
      std::uint32_t next_va=0;
      r.stage=direction?Stage::ReadNextLeft0:Stage::ReadNextRight2;
      if(!address(slots,static_cast<std::uint32_t>(current)*kSlotStride+
                              (direction?0u:2u),next_va)){
        r.code=Code::AddressOverflow;return r;
      }
      if(!host_.read_u16(next_va,current))return r;
      if(current==kNullIndex){r.code=Code::Completed;return r;}
    }
    r.code=Code::CycleGuard;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
} // namespace sin_compat::original_symbol_enumeration_phase643
