#pragma once
// Original SiN Episodes: Emergence 2006 server.dll 0x22280FB0 (255 bytes).
// Source-level model of symbol-tree parent/side lookup. No retail DLL bytes.
// Original __thiscall: ECX=table, args: key*, WORD* parent, BYTE* side,
// unused fourth DWORD; RET 0x10. Comparator is table+0 cdecl key, slot+8.
#include <cstdint>
namespace sin_compat::original_symbol_enumeration_phase643 {
inline constexpr std::uint32_t kOriginalVA=0x22280fb0u;
inline constexpr std::uint32_t kOriginalBytes=255u;
inline constexpr std::uint32_t kOriginalCallerVA=0x2228228bu;
inline constexpr std::uint32_t kOriginalComparatorSlotOffset=0u;
inline constexpr std::uint32_t kSlotStride=16u;
inline constexpr std::uint32_t kSentinelNode=0x22589138u;
inline constexpr std::uint32_t kSentinelFlags=0x22589140u;
inline constexpr std::uint16_t kNullIndex=0xffffu;
enum class Code:std::uint8_t {Completed,HostRejected,NullTableHazard,NullArrayHazard,
  NullComparatorHazard,AddressOverflow,CycleGuard,Reentrant};
enum class Stage:std::uint8_t {None,ReadRoot10,InitializeParentWord,
  InitializeSideByte,WriteVisitedParentWord,ReadArrayBeforeComparator,
  ReadComparator0,CompareOriginalCallback,WriteSideByte,
  ReadSentinelInitFlag,ReadSentinelFlagsDword,WriteSentinelFlagsDword,
  WriteSentinelWord0,WriteSentinelWord2,WriteSentinelWord4,WriteSentinelWord6,
  ReadArrayAfterComparator,ReadNextLeft0,ReadNextRight2};
struct Result {
  Code code=Code::HostRejected;
  Stage stage=Stage::None;
  std::uint32_t table_va=0;
  std::uint32_t key_va=0;
  std::uint16_t last_parent=kNullIndex;
  std::uint8_t last_side=0;
  std::uint32_t visited_nodes=0;
  std::uint32_t comparator_calls=0;
  std::uint32_t sentinel_writes_committed=0;
  bool initialized_sentinel=false;
  bool empty_tree=false;
  bool caller_parent_initialized=false;
  bool caller_side_initialized=false;
};
class IHost {
public:
  virtual ~IHost()=default;
  virtual bool read_u8(std::uint32_t va,std::uint8_t& out)=0;
  virtual bool read_u16(std::uint32_t va,std::uint16_t& out)=0;
  virtual bool read_u32(std::uint32_t va,std::uint32_t& out)=0;
  virtual bool write_u16(std::uint32_t va,std::uint16_t value)=0;
  virtual bool write_u32(std::uint32_t va,std::uint32_t value)=0;
  // Original calls function pointer [table+0] as cdecl(key*,slot+8),
  // tests AL != 0. Callback may relocate/mutate the table. No invented
  // success value: the host must supply its actual returned AL byte.
  virtual bool compare_original_22280fb0(std::uint32_t table_va,
      std::uint32_t callback_va,std::uint32_t key_va,
      std::uint32_t slot_record_va,std::uint8_t& returned_al)=0;
};
class Enumerate final {
public:
  explicit Enumerate(IHost& host)noexcept:host_(host){}
  // The two DWORDs model the original mutable *caller stack words*.
  // Native stores only WORD parent and BYTE side, leaving upper bits intact.
  Result search(std::uint32_t table_va,std::uint32_t key_va,
      std::uint32_t& mutable_parent_arg_word,
      std::uint32_t& mutable_side_arg_word)noexcept;
private:
  IHost& host_;
  bool busy_=false;
};
} // namespace sin_compat::original_symbol_enumeration_phase643
