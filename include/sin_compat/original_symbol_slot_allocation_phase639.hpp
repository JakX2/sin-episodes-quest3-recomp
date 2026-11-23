#pragma once
// Source-authenticated 2006 SiN server.dll symbol-slot machinery.
// Complete native methods: 0x22282270 (102 B) and 0x222810E0 (98 B).
// Enumeration, link/fixup, slot lookup and allocation growth remain typed
// host dependencies. No engine headers or original DLL bytes are shipped.
#include <cstdint>
namespace sin_compat::original_symbol_slot_allocation_phase639 {
inline constexpr std::uint32_t kRegistrarVA=0x22282270u;
inline constexpr std::uint32_t kRegistrarBytes=102u;
inline constexpr std::uint32_t kAcquireVA=0x222810e0u;
inline constexpr std::uint32_t kAcquireBytes=98u;
inline constexpr std::uint32_t kEnumerateVA=0x22280fb0u;
inline constexpr std::uint32_t kLinkVA=0x22281f30u;
inline constexpr std::uint32_t kGetNodeVA=0x22280d40u;
inline constexpr std::uint32_t kGrowVA=0x22280dd0u;
inline constexpr std::uint32_t kOriginalFirstCallerVA=0x222823a0u;
inline constexpr std::uint32_t kInvalidIndex=0xffffu;
inline constexpr std::uint32_t kSlotStride=16u;

enum class Code:std::uint8_t {
  Completed,HostRejected,NullTableHazard,NullRecordHazard,
  NullSlotsHazard,AddressOverflow,Reentrant
};
enum class Stage:std::uint8_t {
  None,ReadFreeHead14,ReadNextIndex16,ReadCapacity8,
  GrowSlotArray80DD0,QueryFreeNode80D40,WriteFreeHead14,
  WriteNextIndex16,ReadSlots4,WriteSlotFreeWord8,
  ReadSlotsAgain4,WriteSlotCache18,
  Enumerate80FB0,Acquire810E0,Link81F30,
  ReadSlotsAfterLink4,ReadActiveCount12,WriteActiveCount12,
  ReadRecordWord0,WriteSlotRecordWord8,
  ReadRecordDword4,WriteSlotRecordDwordC
};
struct Result {
  Code code=Code::HostRejected;
  Stage stage=Stage::None;
  std::uint32_t table_va=0u;
  std::uint32_t slot_va=0u;
  std::uint16_t handle=0xffffu;
  std::uint32_t writes_committed=0u;
  bool used_free_list=false;
  bool grew_slots=false;
  bool enumerated=false;
  bool acquired=false;
  bool linked=false;
  bool active_count_incremented=false;
  bool copied_record=false;
  std::uint32_t rewritten_stack_entry_word=0u;
  std::uint32_t rewritten_stack_second_word=0u;
};
class IHost {
public:
  virtual ~IHost()=default;
  virtual bool read_u16(std::uint32_t guest_va,std::uint16_t& result)=0;
  virtual bool read_u32(std::uint32_t guest_va,std::uint32_t& result)=0;
  virtual bool write_u16(std::uint32_t guest_va,std::uint16_t value)=0;
  virtual bool write_u32(std::uint32_t guest_va,std::uint32_t value)=0;
  // Full original 255-byte 0x22280FB0 helper is NOT reconstructed.
  // Its caller supplies an original record pointer, pointers to *writable*
  // argument words, and zero. It can rewrite those argument words.
  virtual bool enumerate_22280fb0(std::uint32_t table_va,
      std::uint32_t original_record_va,
      std::uint32_t& mutable_first_stack_arg,
      std::uint32_t& mutable_second_stack_arg)=0;
  // Original 0x22281F30 (106 B) links an acquired slot and may mutate
  // storage; exact effects remain host-owned, not silently skipped.
  virtual bool link_slot_22281f30(std::uint32_t table_va,
      std::uint16_t new_handle,
      std::uint32_t rewritten_first_arg,
      std::uint32_t rewritten_second_arg)=0;
  // Original 0x22280D40 resolves a free-list slot, whose +2 WORD is next.
  virtual bool get_free_slot_next_22280d40(std::uint32_t table_va,
      std::uint16_t free_head,std::uint16_t& next)=0;
  // Original 0x22280DD0 uses ECX=table+4 and argument +1; may reallocate.
  virtual bool grow_slot_array_22280dd0(std::uint32_t table_plus4_va,
      std::uint32_t increment)=0;
};
class AcquireSlot final {
public:
  explicit AcquireSlot(IHost& host)noexcept:host_(host){}
  Result acquire(std::uint32_t table_va)noexcept;
private:
  IHost& host_;
  bool busy_=false;
};
class RegisterSlot final {
public:
  RegisterSlot(IHost& host,AcquireSlot& acquire)noexcept:
      host_(host),acquire_(acquire){}
  // Native __thiscall ECX=table, stack: first pointer to original 8-byte
  // record, second u32 flag. Native RET 8 and returns index in AX.
  Result register_record(std::uint32_t table_va,
      std::uint32_t record_va,std::uint32_t second_arg)noexcept;
private:
  IHost& host_;
  AcquireSlot& acquire_;
  bool busy_=false;
};
} // namespace sin_compat::original_symbol_slot_allocation_phase639
