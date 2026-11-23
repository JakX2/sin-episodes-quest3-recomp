#pragma once
// Authenticated original SiN 2006 server.dll tree rotations and sentinel slot accessor.
// Complete methods: 0x22281180(231 B), 0x22281270(234 B), 0x22280D40(81 B).
// This is a portable source model. Memory and callers remain host supplied.
#include <cstdint>
namespace sin_compat::original_symbol_rotations_phase641 {
inline constexpr std::uint32_t kLeftRotationVA=0x22281180u;
inline constexpr std::uint32_t kRightRotationVA=0x22281270u;
inline constexpr std::uint32_t kSentinelAccessorVA=0x22280d40u;
inline constexpr std::uint32_t kLeftRotationSize=231u;
inline constexpr std::uint32_t kRightRotationSize=234u;
inline constexpr std::uint32_t kSentinelAccessorSize=81u;
inline constexpr std::uint32_t kRebalanceVA=0x22281840u;
inline constexpr std::uint32_t kSentinelNode=0x22589138u;
inline constexpr std::uint32_t kSentinelInitFlags=0x22589140u;
inline constexpr std::uint16_t kNullIndex=0xffffu;
inline constexpr std::uint32_t kSlotStride=16u;
enum class Code : std::uint8_t {Completed,HostRejected,NullTableHazard,NullArrayHazard,AddressOverflow,Reentrant};
enum class Stage : std::uint8_t {None,ReadSentinelInitFlag,ReadSentinelInitDword,WriteSentinelInitFlag,InitSentinel0,InitSentinel2,InitSentinel4,InitSentinel6,ReadSlotArray,ResolveSlot,ResolveNodeWord,ReadNodeWord,ReadRootWord,WriteNodeWord,WriteRootWord};
struct AccessResult {
  Code code=Code::HostRejected;
  Stage stage=Stage::None;
  std::uint32_t slot_va=0;
  std::uint32_t writes_committed=0;
  bool initialized_sentinel=false;
};
struct RotateResult {
  Code code=Code::HostRejected;
  Stage stage=Stage::None;
  std::uint32_t table_va=0;
  std::uint16_t index=kNullIndex;
  std::uint16_t pivot=kNullIndex;
  std::uint16_t transferred=kNullIndex;
  std::uint32_t accessor_calls=0;
  std::uint32_t writes_committed=0;
  std::uint32_t sentinel_initializations=0;
  bool root_updated=false;
  bool parent_updated=false;
};
class IHost {
public:
  virtual ~IHost()=default;
  virtual bool read_u8(std::uint32_t guest_va,std::uint8_t& result)=0;
  virtual bool read_u16(std::uint32_t guest_va,std::uint16_t& result)=0;
  virtual bool read_u32(std::uint32_t guest_va,std::uint32_t& result)=0;
  virtual bool write_u16(std::uint32_t guest_va,std::uint16_t value)=0;
  virtual bool write_u32(std::uint32_t guest_va,std::uint32_t value)=0;
};
class SentinelSlotAccessor final {
public:
  explicit SentinelSlotAccessor(IHost& host)noexcept:host_(host){}
  // Original __thiscall(table, index32), RET 4. Only low WORD of index
  // participates in sentinel selection and 16-byte address arithmetic.
  AccessResult resolve(std::uint32_t table_va,std::uint32_t index32)noexcept;
private:
  IHost& host_;
  bool busy_=false;
};
class RotateNode final {
public:
  RotateNode(IHost& host,SentinelSlotAccessor& accessor)noexcept:
    host_(host),accessor_(accessor){}
  // Original __thiscall ECX=table, argument index32, RET 4.
  RotateResult left(std::uint32_t table_va,std::uint32_t index32)noexcept;
  RotateResult right(std::uint32_t table_va,std::uint32_t index32)noexcept;
private:
  RotateResult run(std::uint32_t table_va,std::uint32_t index32,bool right)noexcept;
  IHost& host_;
  SentinelSlotAccessor& accessor_;
  bool busy_=false;
};
} // namespace sin_compat::original_symbol_rotations_phase641
