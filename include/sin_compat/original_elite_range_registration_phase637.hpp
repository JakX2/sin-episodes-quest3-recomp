#pragma once
// Phase637. Entire 2006 retail SiN Episodes server.dll range registrar
// 0x22073760 (217 B), and original "task" wrapper 0x22088FA0 (31 B).
// Full retail hashes/rel32 instructions authenticated by the phase auditor.
// Guest addresses and original side effects are host-owned. No generic
// HL2 physics/props. This routine populates ONE mapped LOCALID range.
#include <cstdint>
#include "sin_compat/original_elite_mapping_init_phase636.hpp"
namespace sin_compat::original_elite_range_registration_phase637 {
inline constexpr std::uint32_t kOriginalRegisterVA=0x22073760u;
inline constexpr std::uint32_t kOriginalRegisterBytes=217u;
inline constexpr std::uint32_t kOriginalTaskWrapperVA=0x22088fa0u;
inline constexpr std::uint32_t kOriginalTaskWrapperBytes=31u;
inline constexpr std::uint32_t kOriginalMappingConverterVA=0x22073700u;
inline constexpr std::uint32_t kOriginalForwardSymbolVA=0x22282370u;
inline constexpr std::uint32_t kOriginalTaskLabelVA=0x223eb17cu;
// Independent original Elite startup callsite 0x22355B3A passes
// ECX=0x2259C2FC (owner+4) and literal category "schedule".
inline constexpr std::uint32_t kOriginalEliteScheduleCallVA=0x22355b3au;
inline constexpr std::uint32_t kOriginalScheduleLabelVA=0x223e5f78u;
inline constexpr std::uint32_t kOriginalScheduleNodeOffset=0x04u;
inline constexpr std::uint32_t kOriginalTaskNodeOffset=0x1cu;
inline constexpr std::uint32_t kOriginalAnimationNodeOffset=0x34u;
inline constexpr std::uint32_t kOriginalLowSentinel=9999u;
inline constexpr std::uint32_t kOriginalUninitializedHigh=0xffffffffu;
enum class Diagnostic:std::uint8_t {
  UninitializedTable,OrderedAfterSuccessor,BelowRangeStart
};
enum class Code:std::uint8_t {
  Registered,OriginalUninitializedTable,OriginalBadOrdering,
  OriginalBelowRangeStart,HostRejected,OriginalNullTargetHazard,
  OriginalNullNodeHazard,OriginalAddressOverflow,Reentrant
};
enum class Stage:std::uint8_t {
  None,ReadSourceRecord14,ReportUninitialized,
  ReadLow4,ReadSuccessor10,WriteInitialLow4,ReadSuccessorLow4,
  ReportOrdering,ReadCurrentLow4,ReportBelowRange,
  ReadUpper8,ReadMappedStart0,WriteHigh8,WriteMappedHighC,
  ConvertOriginalLocalId,ReadResourceContext0,
  ForwardOriginalSymbol,ReadResourceMax4,WriteResourceMax4,
  ResolveWrapperNode
};
struct Result {
  Code code=Code::HostRejected;
  Stage stage=Stage::None;
  unsigned writes_committed=0u;
  bool diagnostic_emitted=false;
  bool low_initialized=false;
  bool range_extended=false;
  bool mapping_called=false;
  bool mapping_found=false;
  bool forwarded=false;
  bool resource_count_written=false;
  std::int32_t mapped_id=-1;
  std::uint32_t node_va=0u;
  std::uint32_t record_va=0u;
};
class IHost:public sin_compat::original_elite_mapping_init_phase636::IGuestMemory {
public:
  ~IHost()override=default;
  // Original 0x223D214C imported diagnostic with original literal label
  // and display-name guest addresses. No unsupported log-format strings.
  virtual bool report_original_diagnostic(Diagnostic kind,
      std::uint32_t category_guest_va,
      std::uint32_t display_name_guest_va)=0;
  // Exact 2006 call 0x2207380E to the already recovered original
  // 58-byte range mapper 0x22073700 (Phase635). Return true for
  // both mapped IDs and native no-match -1; false on host refusal.
  virtual bool convert_original_id_22073700(
      std::uint32_t node_guest_va,std::int32_t local_id,
      std::int32_t& mapped_id)=0;
  // Exact original 0x22073822 call to 0x22282370: ECX=[source_record],
  // args are user-supplied native identifier #1 followed by mapped id.
  // The implementation and ABI of 0x22282370 remain engine-owned.
  virtual bool forward_original_symbol_22282370(
      std::uint32_t resource_context_guest_va,
      std::uint32_t symbol_argument,
      std::int32_t mapped_id)=0;
};
class RegisterRange final {
public:
  explicit RegisterRange(IHost& host)noexcept:host_(host){}
  // Original __thiscall node in ECX, arguments at entry [ESP+4,+8,
  // +0xC,+0x10] respectively: source_symbol, local_id, category,
  // display_name. All arguments are source DWORDs, including addresses.
  Result add(std::uint32_t node_va,std::uint32_t source_symbol,
      std::int32_t local_id,std::uint32_t category_guest_va,
      std::uint32_t display_name_guest_va)noexcept;
private:
  IHost& host_;
  bool busy_=false;
};
class TaskWrapper final {
public:
  explicit TaskWrapper(IHost& host)noexcept:registrar_(host){}
  // Original 0x22088FA0 adds +0x1C to mapping owner and passes literal
  // category "task" at 0x223EB17C; it does NOT use the animation +0x34.
  Result add_task(std::uint32_t mapping_owner_guest_va,
       std::uint32_t source_symbol,std::int32_t local_id,
       std::uint32_t display_name_guest_va)noexcept;
private:
  RegisterRange registrar_;
};
} // namespace sin_compat::original_elite_range_registration_phase637
