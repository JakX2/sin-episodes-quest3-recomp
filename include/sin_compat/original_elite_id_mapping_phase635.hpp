#pragma once
// Gameplay Phase635. The *entire* 58-byte original 2006 SiN server.dll
// ID range-converter function is VA 0x22073700; complete SHA256:
// 5927384b29bc8c8ce0525ca759560659f83a87d2095bc051d0a1763c97df95c9
//
// Used directly from recovered original Elite flag-clear 0x220398A0
// and conditional state setter 0x2203D2D0 (Phase634). This model only
// reconstructs the exact range traversal / arithmetic, NOT actual runtime
// Source mapping-table contents, vtable+0x760 ownership, or engine ABI.
#include <cstdint>

namespace sin_compat::original_elite_id_mapping_phase635 {
inline constexpr std::uint32_t kOriginalVA=0x22073700u;
inline constexpr std::uint32_t kCompleteOriginalByteCount=58u;
// Exact Elite primary vtable +0x760 = a separately original-authenticated
// six-byte thunk at 0x22355D40: mov EAX,0x2259C2F8; ret.
// That object's embedded first range node is +0x34. On the retail DLL
// FILE these addresses lie beyond the raw .data bytes, inside its
// uninitialized virtual tail. The populated list must come from runtime.
inline constexpr std::uint32_t kOriginalEliteVtableBaseVA=0x2248df04u;
inline constexpr std::uint32_t kOriginalEliteVirtual760Offset=0x760u;
inline constexpr std::uint32_t kOriginalVirtual760ThunkVA=0x22355d40u;
inline constexpr std::uint32_t kOriginalVirtual760ThunkBytes=6u;
inline constexpr std::uint32_t kOriginalMappingOwnerVA=0x2259c2f8u;
inline constexpr std::uint32_t kOriginalMappingFirstNodeVA=0x2259c32cu;
inline constexpr std::uint32_t kOriginalSentinelStart=9999u; // 0x270F
inline constexpr std::uint32_t kVirtualContextMemberOffset=0x34u;
inline constexpr std::uint32_t kOriginalRangeDestStartOffset=0u;
inline constexpr std::uint32_t kOriginalRangeSourceStartOffset=4u;
inline constexpr std::uint32_t kOriginalRangeSourceEndOffset=8u;
inline constexpr std::uint32_t kOriginalRangeNextOffset=0x10u;
// The retail implementation does not defend against a cyclic linked list.
// This host reference refuses cycles and abnormally long chains without
// guessing an original engine allocation or dereferencing a raw pointer.
inline constexpr unsigned kMaxSafeOriginalNodes=512u;

struct RangeNode {
  std::int32_t mapped_start=0; // original native [ECX]
  std::int32_t source_start=0; // original native [ECX+0x04]
  std::int32_t source_end=0;   // original native [ECX+0x08]
  std::uint64_t next=0u;      // original native [ECX+0x10]
};
enum class Code:std::uint8_t {
  Mapped,NotFound,HostRejected,OriginalNullHeadHazard,
  OriginalMappingCycle,OriginalMappingLimit,Reentrant
};
enum class Stage:std::uint8_t {
  None,Head,ReadNode,CompareSourceStart,CompareSourceEnd,
  NextNode,ComputeDestination
};
struct Result {
  Code code=Code::HostRejected;
  Stage stage=Stage::None;
  std::int32_t output=-1; // original native EAX; -1 is not-found sentinel
  unsigned visited=0;
  std::uint64_t matched_node=0u;
  std::uint64_t last_node=0u;
  bool input_minus_one_shortcut=false;
};
class IHost {
public:
  virtual ~IHost()=default;
  // Original ECX points to an embedded node at virtual760_result+0x34.
  // The exact fields read are mapped_start [+0], source_start [+4],
  // inclusive source_end [+8], next [+0x10]. Native code reads them
  // in control-flow order; this model snapshots one immutable node
  // through a host callback and rejects host failure or exceptions.
  virtual bool read_original_mapping_node(
      std::uint64_t node_address,RangeNode& out)=0;
};
class Reference final {
public:
  explicit Reference(IHost& host)noexcept:host_(host){}
  Result translate(std::uint64_t head_context_node,
                   std::int32_t input_id)noexcept;
  Reference(const Reference&)=delete;
  Reference& operator=(const Reference&)=delete;
private:
  IHost& host_;
  bool busy_=false;
};
} // namespace sin_compat::original_elite_id_mapping_phase635
