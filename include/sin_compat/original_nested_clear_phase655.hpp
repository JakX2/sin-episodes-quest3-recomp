#pragma once
// Behavioral reconstruction from owner 2006 SiN Episodes server.dll.
// Original 0x2228EC50 (132B) free-slot reset, 0x2228F400 (49B) nested
// name-block cleanup, 0x2228F3C0 (61B) existing-name view resolution.
// No original executable bytes are embedded here.
#include <cstdint>
namespace sin_compat::original_nested_clear_phase655 {
inline constexpr std::uint32_t kClearSlotsVA = 0x2228ec50u;
inline constexpr std::uint32_t kClearSlotsBytes = 132u;
inline constexpr std::uint32_t kReleaseBlocksVA = 0x2228f400u;
inline constexpr std::uint32_t kReleaseBlocksBytes = 49u;
inline constexpr std::uint32_t kNameViewVA = 0x2228f3c0u;
inline constexpr std::uint32_t kNameViewBytes = 61u;
inline constexpr std::uint32_t kSentinelVA = 0x2259012cu;
inline constexpr std::uint32_t kSentinelFlagVA = 0x22590134u;
inline constexpr std::uint16_t kInvalid = 0xffffu;

enum class Code : std::uint8_t {
  Completed, HostRejected, NullOwnerHazard, NullArrayHazard,
  AddressOverflow, BlockLoopGuard, Reentrant
};
enum class Stage : std::uint8_t {
  None, ReadNextUnused16, ReadSentinelFlag, InitializeSentinel,
  ReadSlotArray4, WriteFreeRight2, WriteFreeLeft0,
  WriteRoot10, WriteCount12, WriteFreeHead14,
  ClearSlotsEC50, ReadBlockCount28, ReadBlockArray1C,
  ReadBlockPointer, FreeBlockE3E0, ClearBlockCount28,
  WriteAbsentHandle, WriteFallback30, WriteScratchInvalid,
  FindExistingEB40, WriteFoundHandle
};
struct Result {
  Code code = Code::HostRejected;
  Stage stage = Stage::None;
  std::uint32_t map_va = 0;
  std::uint32_t writes_committed = 0;
  std::uint32_t slots_recycled = 0;
  std::uint32_t block_free_calls = 0;
  std::uint32_t return_eax = 0;
  std::uint16_t free_head = kInvalid, handle = kInvalid;
  bool sentinel_initialized = false;
  bool underlying_clear_called = false;
  bool lookup_called = false;
  bool null_name = false;
};
class IHost {
public:
  virtual ~IHost() = default;
  virtual bool read_u8(std::uint32_t va, std::uint8_t& out) = 0;
  virtual bool read_u16(std::uint32_t va, std::uint16_t& out) = 0;
  virtual bool read_u32(std::uint32_t va, std::uint32_t& out) = 0;
  virtual bool write_u16(std::uint32_t va, std::uint16_t value) = 0;
  virtual bool write_u32(std::uint32_t va, std::uint32_t value) = 0;
  // Original 0x2226E3E0 allocator free dispatch, not a synthetic free().
  virtual bool free_original_2226e3e0(std::uint32_t block_va) = 0;
  // Original 0x2228EB40 __thiscall two-direction nested map lookup.
  virtual bool find_original_2228eb40(std::uint32_t map_va,
      std::uint32_t key_ref_va, std::uint32_t context_va,
      std::uint16_t& handle) = 0;
};
class ClearSlots final {
public:
  explicit ClearSlots(IHost& host) noexcept : host_(host) {}
  // Native __thiscall ECX=map, no stack arguments.
  Result run(std::uint32_t map) noexcept;
private:
  IHost& host_; bool busy_ = false;
};
class ReleaseBlocks final {
public:
  ReleaseBlocks(IHost& host, ClearSlots& clear) noexcept
    : host_(host),clear_(clear) {}
  // Native __thiscall ECX=map, no stack arguments.
  Result run(std::uint32_t map) noexcept;
private:
  IHost& host_; ClearSlots& clear_; bool busy_ = false;
};
class FindNameView final {
public:
  explicit FindNameView(IHost& host) noexcept : host_(host) {}
  // Native __thiscall ECX=map, args WORD* output, char* name, RET 8.
  // scratch_ref is a writable host stand-in for original mutated stack arg2.
  Result run(std::uint32_t map,std::uint32_t output,
             std::uint32_t name,std::uint32_t scratch_ref) noexcept;
private:
  IHost& host_;bool busy_ = false;
};
} // namespace sin_compat::original_nested_clear_phase655
