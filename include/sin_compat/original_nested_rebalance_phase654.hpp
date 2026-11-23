#pragma once
// Behavioral reconstruction of SiN Episodes (2006) server.dll insertion fixup.
// Native __thiscall ECX=map, stack DWORD node slot, RET 4. Guest WORD indices.
#include <cstdint>

namespace sin_compat::original_nested_rebalance_phase654 {
inline constexpr std::uint32_t kRotateLeftVA = 0x2228e710u;
inline constexpr std::uint32_t kRotateRightVA = 0x2228e800u;
inline constexpr std::uint32_t kFixupVA = 0x2228edf0u;
inline constexpr std::uint32_t kSentinelVA = 0x2259012cu;
inline constexpr std::uint32_t kSentinelFlagVA = 0x22590134u;
inline constexpr std::uint16_t kNil = 0xffffu;
inline constexpr std::uint16_t kRed = 0u;
inline constexpr std::uint16_t kBlack = 1u;

enum class Code : std::uint8_t {
  Completed, HostRejected, InvalidMap, InvalidNode,
  InvalidRotation, AddressOverflow, LoopGuard, Reentrant
};
enum class Stage : std::uint8_t {
  None, ReadRoot, ReadNodeArray, ReadNode, ReadSentinelFlag,
  InitSentinel, WriteNode, WriteRoot, RotateLeft, RotateRight
};
struct Result {
  Code code = Code::HostRejected;
  Stage stage = Stage::None;
  std::uint32_t writes_committed = 0;
  std::uint32_t iterations = 0;
  std::uint32_t left_rotations = 0, right_rotations = 0;
  bool sentinel_initialized = false, root_blacked = false;
};
class IHost {
public:
  virtual ~IHost() = default;
  virtual bool read_u8(std::uint32_t address, std::uint8_t& value) = 0;
  virtual bool read_u16(std::uint32_t address, std::uint16_t& value) = 0;
  virtual bool read_u32(std::uint32_t address, std::uint32_t& value) = 0;
  virtual bool write_u16(std::uint32_t address, std::uint16_t value) = 0;
  virtual bool write_u32(std::uint32_t address, std::uint32_t value) = 0;
};
class Rebalance final {
public:
  explicit Rebalance(IHost& host) noexcept : host_(host) {}
  Result fixup(std::uint32_t map, std::uint16_t inserted) noexcept;
  Result rotate_left(std::uint32_t map, std::uint16_t pivot) noexcept;
  Result rotate_right(std::uint32_t map, std::uint16_t pivot) noexcept;
private:
  IHost& host_;
  bool busy_ = false;
  bool init_sentinel(Result& result);
  bool field(std::uint32_t map, std::uint16_t node,
             std::uint32_t offset, std::uint32_t& address, Result& result);
  bool read(std::uint32_t map, std::uint16_t node,
            std::uint32_t offset, std::uint16_t& value, Result& result);
  bool write(std::uint32_t map, std::uint16_t node,
             std::uint32_t offset, std::uint16_t value, Result& result);
  bool read_root(std::uint32_t map, std::uint16_t& root, Result& result);
  bool write_root(std::uint32_t map, std::uint16_t root, Result& result);
  bool rotate(std::uint32_t map, std::uint16_t pivot, bool left, Result& result);
};
}  // namespace sin_compat::original_nested_rebalance_phase654
