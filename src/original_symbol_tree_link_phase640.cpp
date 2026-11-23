#include "sin_compat/original_symbol_tree_link_phase640.hpp"
#include <limits>
namespace sin_compat::original_symbol_tree_link_phase640 {
namespace {
struct Guard {
  bool& value;
  explicit Guard(bool& v) noexcept:value(v){value=true;}
  ~Guard() noexcept {value=false;}
};
constexpr bool at(std::uint32_t base,std::uint32_t off,
                  std::uint32_t& result) noexcept {
  if(base==0u || base>std::numeric_limits<std::uint32_t>::max()-off)
    return false;
  result=base+off;return true;
}
}
Result LinkNode::link(std::uint32_t table_va,std::uint32_t child_index32,
                      std::uint32_t parent_index32,std::uint32_t side32) noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result result{};result.table_va=table_va;
  result.child_index=static_cast<std::uint16_t>(child_index32);
  result.parent_index=static_cast<std::uint16_t>(parent_index32);
  if(table_va==0u){result.code=Code::NullTableHazard;return result;}
  try {
    std::uint32_t array_field=0;
    if(!at(table_va,4u,array_field)){
      result.code=Code::AddressOverflow;return result;
    }
    result.stage=Stage::ReadSlotArray4;
    std::uint32_t array=0;
    if(!host_.read_u32(array_field,array))return result;
    if(array==0u){result.code=Code::NullArrayHazard;return result;}
    if(!at(array,kSlotStride*result.child_index,result.slot_va)){
      result.code=Code::AddressOverflow;return result;
    }
    // Original 0x22281F48, +4 parent write comes BEFORE the reset writes.
    auto store=[&](Stage stage,std::uint32_t base,std::uint32_t offset,
                   std::uint16_t value)->bool {
      result.stage=stage;
      std::uint32_t destination=0;
      if(!at(base,offset,destination)){
        result.code=Code::AddressOverflow;return false;
      }
      if(!host_.write_u16(destination,value))return false;
      ++result.writes_committed;return true;
    };
    if(!store(Stage::InitializeParent4,result.slot_va,4u,result.parent_index))return result;
    if(!store(Stage::InitializeChild2,result.slot_va,2u,kInvalidNodeIndex))return result;
    if(!store(Stage::InitializeChild0,result.slot_va,0u,kInvalidNodeIndex))return result;
    if(!store(Stage::InitializeColor6,result.slot_va,6u,0u))return result;
    if(result.parent_index==kInvalidNodeIndex) {
      // Native 0x22281F8D: set the root index before rebalance.
      if(!store(Stage::LinkRoot10,table_va,0x10u,result.child_index))return result;
      result.linked_as_root=true;
    } else {
      // A fresh table+4 load follows the four node initialization writes.
      // It can differ from the initial array pointer after host callbacks.
      result.stage=Stage::RefreshSlotArray4;
      std::uint32_t parent_array=0;
      if(!host_.read_u32(array_field,parent_array))return result;
      if(parent_array==0u){result.code=Code::NullArrayHazard;return result;}
      if(!at(parent_array,kSlotStride*result.parent_index,result.parent_slot_va)){
        result.code=Code::AddressOverflow;return result;
      }
      // Original uses BYTE [esp+0x10], not an entire bool/DWORD compare.
      if((side32&0xffu)!=0u) {
        if(!store(Stage::LinkParentLeft0,result.parent_slot_va,0u,result.child_index))
          return result;
        result.linked_as_left=true;
      } else {
        if(!store(Stage::LinkParentRight2,result.parent_slot_va,2u,result.child_index))
          return result;
        result.linked_as_right=true;
      }
    }
    result.stage=Stage::RebalanceOriginal81840;
    result.rebalance_called=true;
    // Native passes original 32-bit ESI, not the masked 16-bit slot index.
    if(!host_.rebalance_original_22281840(table_va,child_index32))return result;
    result.code=Code::Linked;
  } catch(...) {result.code=Code::HostRejected;}
  return result;
}
AddressResult DirectSlotAccessor::resolve(std::uint32_t table_va,
                                          std::uint32_t index32) noexcept {
  AddressResult r{};
  if(table_va==0u){r.code=Code::NullTableHazard;return r;}
  try {
    std::uint32_t field=0;
    r.stage=Stage::ReadAccessorArray4;
    if(!at(table_va,4u,field)) {r.code=Code::AddressOverflow;return r;}
    std::uint32_t array=0;
    if(!host_.read_u32(field,array))return r;
    if(array==0u){r.code=Code::NullArrayHazard;return r;}
    r.stage=Stage::ResolveSlotAddress;
    if(!at(array,kSlotStride*static_cast<std::uint16_t>(index32),r.slot_va)) {
      r.code=Code::AddressOverflow;return r;
    }
    r.code=Code::Linked;
  } catch(...) {r.code=Code::HostRejected;}
  return r;
}
} // namespace sin_compat::original_symbol_tree_link_phase640
