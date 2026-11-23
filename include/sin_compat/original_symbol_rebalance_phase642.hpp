#pragma once
// Original SiN Episodes: Emergence 2006 server.dll 0x22281840.
// Authenticated whole method: 1482 bytes, SHA-256 in owner binary audit.
// The structurally decompiled red-black insertion fixup below delegates to
// the independently recovered Phase641 original tree rotations / sentinel.
// All memory and original-engine invocation remain host supplied.
#include "sin_compat/original_symbol_rotations_phase641.hpp"
#include <cstdint>
namespace sin_compat::original_symbol_rebalance_phase642 {
inline constexpr std::uint32_t kOriginalVA=0x22281840u;
inline constexpr std::uint32_t kOriginalLength=1482u;
inline constexpr std::uint32_t kOriginalLeftRotationVA=0x22281180u;
inline constexpr std::uint32_t kOriginalRightRotationVA=0x22281270u;
inline constexpr std::uint32_t kOriginalSentinelVA=0x22589138u;
inline constexpr std::uint16_t kNullIndex=0xffffu;
inline constexpr std::uint16_t kRed=0u;
inline constexpr std::uint16_t kBlack=1u;
enum class Code : std::uint8_t {
  Completed, HostRejected, NullTableHazard, NullArrayHazard,
  AddressOverflow, SentinelWriteHazard, NonTerminatingTree, Reentrant
};
enum class Stage : std::uint8_t {
  None, ReadRoot, ReadParent, ReadParentColor, ReadGrandparent,
  ReadGrandparentLeft, ReadUncle, ReadUncleColor,
  ReadParentChildForZigzag, ReadParentChildForMirrorZigzag,
  RecolorParentBlack, RecolorUncleBlack, RecolorGrandparentRed,
  RotateLeft, RotateRight, RefreshParentAfterRotation,
  RefreshGrandparentAfterRotation, FinalReadRoot, FinalBlackenRoot
};
struct Result {
  Code code=Code::HostRejected;
  Stage stage=Stage::None;
  std::uint32_t table_va=0u;
  std::uint32_t input_index32=0u;
  std::uint16_t final_index=kNullIndex;
  std::uint32_t loop_iterations=0u;
  std::uint32_t recolor_cases=0u;
  std::uint32_t left_rotations=0u;
  std::uint32_t right_rotations=0u;
  std::uint32_t direct_writes_committed=0u;
  std::uint32_t rotation_writes_committed=0u;
  std::uint32_t sentinel_writes_committed=0u;
  bool root_blackened=false;
  bool reached_root_initially=false;
};
class Rebalance final {
public:
  using IHost=original_symbol_rotations_phase641::IHost;
  using Accessor=original_symbol_rotations_phase641::SentinelSlotAccessor;
  using Rotate=original_symbol_rotations_phase641::RotateNode;
  Rebalance(IHost& host,Accessor& accessor,Rotate& rotate)noexcept:
    host_(host),accessor_(accessor),rotate_(rotate){}
  // Original __thiscall ECX=table, stack arg DWORD index, RET 4.
  // This is a source reference, not an executable x86 ABI stub.
  Result insert(std::uint32_t table_va,std::uint32_t index32)noexcept;
private:
  IHost& host_;
  Accessor& accessor_;
  Rotate& rotate_;
  bool busy_=false;
};
} // namespace sin_compat::original_symbol_rebalance_phase642
