#include "sin_compat/original_symbol_forward_phase638.hpp"
#include <limits>
namespace sin_compat::original_symbol_forward_phase638 {
namespace {
struct BusyGuard {
  bool& flag;
  explicit BusyGuard(bool& f)noexcept:flag(f){flag=true;}
  ~BusyGuard()noexcept{flag=false;}
};
constexpr bool at(std::uint32_t base,std::uint32_t offset,
                  std::uint32_t& out)noexcept {
  if(base==0u || base>std::numeric_limits<std::uint32_t>::max()-offset)
    return false;
  out=base+offset;
  return true;
}
}
Result Forwarder::write(std::uint32_t context_va,
                        std::uint32_t symbol_argument,
                        std::int32_t mapped_id)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  BusyGuard guard(busy_);
  Result result{};
  result.stage=Stage::ReadContextTable0;
  if(context_va==0u) {
    result.code=Code::OriginalNullContextHazard;
    return result;
  }
  try {
    // Native 0x22282378 ESI=[ECX]. No original null check.
    if(!host_.read_u32(context_va,result.table_va))return result;
    if(result.table_va==0u) {
      result.code=Code::OriginalNullTableHazard;
      return result;
    }
    std::uint32_t tree_va=0u;
    result.stage=Stage::ResolveTree1C;
    if(!at(result.table_va,0x1cu,tree_va)) {
      result.code=Code::OriginalAddressOverflow;
      return result;
    }
    // Native lookup gets pointer to original first-argument stack word
    // and its initial symbol pointer. Returned WORD is the tree token.
    std::uint32_t scratch=symbol_argument;
    result.stage=Stage::TreeLookup2228F7A0;
    if(!host_.lookup_original_tree_2228f7a0(
            tree_va,scratch,symbol_argument,result.tree_token))return result;
    result.tree_lookup_completed=true;
    // The native allocator may mutate token and table, and is mandatory.
    std::uint16_t token=result.tree_token;
    result.stage=Stage::AllocateSlot22282270;
    if(!host_.allocate_original_slot_22282270(
            result.table_va,token,result.slot_handle))return result;
    result.allocation_completed=true;
    // Native re-reads [table+4] AFTER slot allocation, not before it.
    std::uint32_t slots_field=0u;
    result.stage=Stage::ReadSlots4;
    if(!at(result.table_va,4u,slots_field)) {
      result.code=Code::OriginalAddressOverflow;
      return result;
    }
    std::uint32_t slots=0u;
    if(!host_.read_u32(slots_field,slots))return result;
    if(slots==0u) {
      result.code=Code::OriginalNullSlotArrayHazard;
      return result;
    }
    // MOVZX ECX,AX; SHL ECX,4; MOV [EDX+ECX+0C],ESI.
    // Original does NOT perform portable capacity/overflow checks.
    const auto offset=static_cast<std::uint32_t>(result.slot_handle)*
        kSlotStride+kSlotMappedIdOffset;
    result.stage=Stage::WriteSlotMappedId;
    if(!at(slots,offset,result.slot_address)) {
      result.code=Code::OriginalAddressOverflow;
      return result;
    }
    if(!host_.write_u32(result.slot_address,
                        static_cast<std::uint32_t>(mapped_id)))return result;
    result.mapped_index_written=true;
    result.code=Code::Written;
  } catch(...) {
    // Host rejection doesn't roll back earlier tree/allocator mutations.
    result.code=Code::HostRejected;
  }
  return result;
}
sin_compat::original_elite_range_registration_phase637::Result
ConditionWrapper::add_condition(std::uint32_t owner_va,
    std::uint32_t symbol,std::int32_t local_id,
    std::uint32_t display_name_va)noexcept {
  namespace r=sin_compat::original_elite_range_registration_phase637;
  std::uint32_t node=0u;
  if(!at(owner_va,kOriginalConditionNodeOffset,node)) {
    return {owner_va==0u?r::Code::OriginalNullNodeHazard:
                          r::Code::OriginalAddressOverflow,
            r::Stage::ResolveWrapperNode};
  }
  return registrar_.add(node,symbol,local_id,
                        kOriginalConditionCategoryVA,display_name_va);
}
}
