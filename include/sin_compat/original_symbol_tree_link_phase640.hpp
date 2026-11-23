#pragma once
// Phase640 original 2006 SiN symbol table tree insertion and direct slot accessor.
// Verified complete native methods: 0x22281F30 (106B), 0x22280D30 (14B).
// Tree rebalance 0x22281840 is not reconstructed, and is a mandatory host action.
#include <cstdint>
namespace sin_compat::original_symbol_tree_link_phase640 {
inline constexpr std::uint32_t kOriginalLinkVA = 0x22281f30u;
inline constexpr std::uint32_t kOriginalLinkBytes = 106u;
inline constexpr std::uint32_t kOriginalSlotAccessorVA = 0x22280d30u;
inline constexpr std::uint32_t kOriginalSlotAccessorBytes = 14u;
inline constexpr std::uint32_t kOriginalRebalanceVA = 0x22281840u;
inline constexpr std::uint32_t kOriginalRegistrarCallVA = 0x222822a6u;
inline constexpr std::uint32_t kInvalidNodeIndex = 0xffffu;
inline constexpr std::uint32_t kSlotStride = 16u;

enum class Code : std::uint8_t {
  Linked, AddressOverflow, NullTableHazard, NullArrayHazard,
  HostRejected, Reentrant
};
enum class Stage : std::uint8_t {
  None, ReadSlotArray4, InitializeParent4, InitializeChild2,
  InitializeChild0, InitializeColor6, RefreshSlotArray4,
  LinkParentLeft0, LinkParentRight2, LinkRoot10,
  RebalanceOriginal81840, ReadAccessorArray4,
  ResolveSlotAddress
};
struct Result {
  Code code = Code::HostRejected;
  Stage stage = Stage::None;
  std::uint32_t table_va = 0;
  std::uint32_t slot_va = 0;
  std::uint32_t parent_slot_va = 0;
  std::uint32_t writes_committed = 0;
  std::uint16_t child_index = kInvalidNodeIndex;
  std::uint16_t parent_index = kInvalidNodeIndex;
  bool linked_as_root = false;
  bool linked_as_left = false;
  bool linked_as_right = false;
  bool rebalance_called = false;
};
struct AddressResult {
  Code code = Code::HostRejected;
  Stage stage = Stage::None;
  std::uint32_t slot_va = 0;
};
class IHost {
public:
  virtual ~IHost()=default;
  virtual bool read_u32(std::uint32_t guest_va, std::uint32_t& out)=0;
  virtual bool write_u16(std::uint32_t guest_va, std::uint16_t value)=0;
  // Original 0x22281840: ECX=table; one stack argument is the entire
  // original DWORD index (not truncated). Full rebalancing is unrecovered.
  virtual bool rebalance_original_22281840(
    std::uint32_t table_guest_va,std::uint32_t full_child_index)=0;
};
class LinkNode final {
public:
  explicit LinkNode(IHost& host) noexcept : host_(host) {}
  // Original __thiscall ECX=table; args index32, parent32, side32.
  // Index and parent in node fields are truncated to 16 bits;
  // the low BYTE of side selects left(nonzero) vs right(zero).
  Result link(std::uint32_t table_va, std::uint32_t child_index32,
              std::uint32_t parent_index32, std::uint32_t side32) noexcept;
private:
  IHost& host_;
  bool busy_=false;
};
class DirectSlotAccessor final {
public:
  explicit DirectSlotAccessor(IHost& host) noexcept : host_(host) {}
  // Original 0x22280D30 returns [table+4] + 16 * uint16(index).
  // Unlike 0x22280D40, there is no 0xFFFF special sentinel branch.
  AddressResult resolve(std::uint32_t table_va,std::uint32_t index32) noexcept;
private:
  IHost& host_;
};
} // namespace sin_compat::original_symbol_tree_link_phase640
