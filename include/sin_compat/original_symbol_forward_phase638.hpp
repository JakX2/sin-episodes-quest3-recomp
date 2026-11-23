#pragma once
// Phase638: authenticated complete 2006 SiN native symbol-forwarding
// 0x22282370 (77 B) and NPC condition +0x34 wrapper (31 B).
// Original tree lookup and slot allocator remain host-owned effects.
#include <cstdint>
#include "sin_compat/original_elite_range_registration_phase637.hpp"
namespace sin_compat::original_symbol_forward_phase638 {
inline constexpr std::uint32_t kOriginalForwardVA=0x22282370u;
inline constexpr std::uint32_t kOriginalForwardBytes=77u;
inline constexpr std::uint32_t kOriginalTreeLookupVA=0x2228f7a0u;
inline constexpr std::uint32_t kOriginalSlotAllocationVA=0x22282270u;
inline constexpr std::uint32_t kOriginalConditionWrapperVA=0x2205a760u;
inline constexpr std::uint32_t kOriginalConditionWrapperBytes=31u;
inline constexpr std::uint32_t kOriginalConditionCategoryVA=0x223e05c8u;
inline constexpr std::uint32_t kOriginalConditionNodeOffset=0x34u;
inline constexpr std::uint32_t kSlotStride=16u;
inline constexpr std::uint32_t kSlotMappedIdOffset=12u;
enum class Code:std::uint8_t {
  Written,HostRejected,OriginalNullContextHazard,OriginalNullTableHazard,
  OriginalNullSlotArrayHazard,OriginalAddressOverflow,Reentrant
};
enum class Stage:std::uint8_t {
  None,ReadContextTable0,ResolveTree1C,TreeLookup2228F7A0,
  AllocateSlot22282270,ReadSlots4,WriteSlotMappedId,
  ResolveConditionNode
};
struct Result {
  Code code=Code::HostRejected;
  Stage stage=Stage::None;
  bool tree_lookup_completed=false;
  bool allocation_completed=false;
  bool mapped_index_written=false;
  std::uint16_t tree_token=0xffffu;
  std::uint16_t slot_handle=0xffffu;
  std::uint32_t table_va=0u;
  std::uint32_t slot_address=0u;
};
class IHost {
public:
  virtual ~IHost()=default;
  virtual bool read_u32(std::uint32_t guest_va,std::uint32_t& output)=0;
  virtual bool write_u32(std::uint32_t guest_va,std::uint32_t value)=0;
  // Native 0x2228F7A0 uses ECX=table+1C, pointer to original arg1
  // stack word, and original symbol string pointer. Returns pointer to
  // a 16-bit tree token. Host supplies token and may mutate scratch.
  virtual bool lookup_original_tree_2228f7a0(
      std::uint32_t tree_va,std::uint32_t& symbol_argument_scratch,
      std::uint32_t original_symbol_argument,
      std::uint16_t& tree_token)=0;
  // Native 0x22282270: ECX=table, first arg=16-bit token pointer,
  // second arg=zero; returns slot handle in AX. Effects NOT optional.
  virtual bool allocate_original_slot_22282270(
      std::uint32_t table_va,std::uint16_t& token,
      std::uint16_t& slot_handle)=0;
};
class Forwarder final {
public:
  explicit Forwarder(IHost& host)noexcept:host_(host){}
  // Original __thiscall ECX=context; stack symbol,index; RET 8.
  // Native has no return status; Result describes safe host execution.
  Result write(std::uint32_t context_va,std::uint32_t symbol_argument,
      std::int32_t mapped_id)noexcept;
private:
  IHost& host_;
  bool busy_=false;
};
// Original +0x34 wrapper label is "condition", not "animation event".
class ConditionWrapper final {
public:
  explicit ConditionWrapper(
     sin_compat::original_elite_range_registration_phase637::RegisterRange& r)
     noexcept:registrar_(r){}
  sin_compat::original_elite_range_registration_phase637::Result add_condition(
      std::uint32_t owner_va,std::uint32_t symbol,
      std::int32_t local_id,std::uint32_t display_name_va)noexcept;
private:
  sin_compat::original_elite_range_registration_phase637::RegisterRange& registrar_;
};
} // namespace sin_compat::original_symbol_forward_phase638
