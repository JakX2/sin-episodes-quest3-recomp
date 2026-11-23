#include "sin_compat/original_elite_range_registration_phase637.hpp"
#include <limits>
namespace sin_compat::original_elite_range_registration_phase637 {
namespace {
struct Guard {
  bool& busy;
  explicit Guard(bool& value)noexcept:busy(value){busy=true;}
  ~Guard()noexcept{busy=false;}
};
bool at(std::uint32_t base,std::uint32_t offset,
        std::uint32_t& addr)noexcept {
  if(base==0u||base>std::numeric_limits<std::uint32_t>::max()-offset)
    return false;
  addr=base+offset;
  return true;
}
std::int64_t sx(std::uint32_t value)noexcept {
  return value<=0x7fffffffu?
    static_cast<std::int64_t>(value):
    static_cast<std::int64_t>(value)-0x100000000LL;
}
std::uint32_t wrap_range_end(
    std::uint32_t mapped_start,std::uint32_t source_low,
    std::int32_t new_high)noexcept {
  return mapped_start-source_low+static_cast<std::uint32_t>(new_high);
}
} // namespace
Result RegisterRange::add(
    std::uint32_t node_va,std::uint32_t source_symbol,
    std::int32_t local_id,std::uint32_t category_guest_va,
    std::uint32_t display_name_guest_va)noexcept {
  if(busy_)return {Code::Reentrant,Stage::None};
  Guard guard(busy_);
  Result result{};
  result.node_va=node_va;
  result.stage=Stage::ReadSourceRecord14;
  std::uint32_t address=0u;
  auto read=[&](Stage stage,std::uint32_t base,std::uint32_t off,
                std::uint32_t& value)->bool {
    result.stage=stage;
    if(!at(base,off,address)) {
      result.code=base==0u?Code::OriginalNullNodeHazard:
                                Code::OriginalAddressOverflow;
      return false;
    }
    return host_.read_u32(address,value);
  };
  auto write=[&](Stage stage,std::uint32_t base,std::uint32_t off,
                 std::uint32_t value)->bool {
    result.stage=stage;
    if(!at(base,off,address)) {
      result.code=base==0u?Code::OriginalNullNodeHazard:
                                Code::OriginalAddressOverflow;
      return false;
    }
    if(!host_.write_u32(address,value))return false;
    ++result.writes_committed;
    return true;
  };
  auto diagnostic=[&](Code error,Stage stage,Diagnostic kind,
                       std::uint32_t category)->void {
    result.code=error;
    result.stage=stage;
    if(!host_.report_original_diagnostic(
         kind,category,display_name_guest_va)) {
      result.code=Code::HostRejected;
    } else {
      result.diagnostic_emitted=true;
    }
  };
  try {
    // Native mov EDI,[ECX+14], TEST EDI,EDI. No other node access
    // happens on the uninitialized-table error branch.
    std::uint32_t source_record=0u;
    if(!read(Stage::ReadSourceRecord14,node_va,0x14u,
             source_record))return result;
    result.record_va=source_record;
    if(source_record==0u) {
      diagnostic(Code::OriginalUninitializedTable,
        Stage::ReportUninitialized,
        Diagnostic::UninitializedTable,0u);
      return result;
    }
    std::uint32_t source_low=0u;
    if(!read(Stage::ReadLow4,node_va,4u,source_low))
      return result;
    const std::uint32_t id_bits=static_cast<std::uint32_t>(local_id);
    if(source_low==kOriginalLowSentinel) {
      // Exact source order: read next +0x10, publish first low ID at
      // +4, then inspect successor low only if next is non-null.
      std::uint32_t successor=0u;
      if(!read(Stage::ReadSuccessor10,node_va,0x10u,
               successor))return result;
      if(!write(Stage::WriteInitialLow4,node_va,4u,id_bits))
        return result;
      result.low_initialized=true;
      if(successor!=0u) {
        std::uint32_t successor_low=0u;
        if(!read(Stage::ReadSuccessorLow4,successor,4u,
                 successor_low))return result;
        if(successor_low!=kOriginalLowSentinel &&
           static_cast<std::int64_t>(local_id)<=sx(successor_low)) {
          diagnostic(Code::OriginalBadOrdering,
              Stage::ReportOrdering,Diagnostic::OrderedAfterSuccessor,
              category_guest_va);
          return result; // the original +4 mutation is NOT rolled back
        }
      }
    }
    if(!read(Stage::ReadCurrentLow4,node_va,4u,source_low))
      return result;
    if(static_cast<std::int64_t>(local_id)<sx(source_low)) {
      diagnostic(Code::OriginalBelowRangeStart,
          Stage::ReportBelowRange,Diagnostic::BelowRangeStart,
          category_guest_va);
      return result;
    }
    std::uint32_t high=0u;
    if(!read(Stage::ReadUpper8,node_va,8u,high))return result;
    if(high==kOriginalUninitializedHigh) {
      // First valid entry: upper = source low, mapped_upper =
      // destination start. Its ID now becomes eligible for Phase635.
      if(!write(Stage::WriteHigh8,node_va,8u,source_low))
        return result;
      std::uint32_t mapped_start=0u;
      if(!read(Stage::ReadMappedStart0,node_va,0u,
               mapped_start))return result;
      if(!write(Stage::WriteMappedHighC,node_va,12u,mapped_start))
        return result;
      result.range_extended=true;
    } else if(static_cast<std::int64_t>(local_id)>sx(high)) {
      // Source computes mapped last ID BEFORE either +8/+C write.
      // Original MOV ESI,[ECX]; SUB ESI,[ECX+4]; ADD ESI,EAX.
      std::uint32_t mapped_start=0u;
      if(!read(Stage::ReadMappedStart0,node_va,0u,
               mapped_start))return result;
      const auto mapped_high=wrap_range_end(
        mapped_start,source_low,local_id);
      if(!write(Stage::WriteHigh8,node_va,8u,id_bits))
        return result;
      if(!write(Stage::WriteMappedHighC,node_va,12u,mapped_high))
        return result;
      result.range_extended=true;
    }
    // Original call at 0x2207380E: 0x22073700(this,local_id).
    result.stage=Stage::ConvertOriginalLocalId;
    result.mapping_called=true;
    std::int32_t mapped=-1;
    if(!host_.convert_original_id_22073700(node_va,local_id,mapped))
      return result;
    result.mapped_id=mapped;
    if(mapped==-1) {
      result.code=Code::Registered;
      result.stage=Stage::None;
      return result; // no callback, no source record max-count update
    }
    result.mapping_found=true;
    std::uint32_t resource_context=0u;
    if(!read(Stage::ReadResourceContext0,source_record,0u,
             resource_context))return result;
    if(resource_context==0u) {
      result.code=Code::OriginalNullTargetHazard;
      return result;
    }
    // Exact original guest ECX=[EDI]; two stack arguments are the
    // original symbol/name/id (first) and mapped index (second).
    result.stage=Stage::ForwardOriginalSymbol;
    if(!host_.forward_original_symbol_22282370(
          resource_context,source_symbol,mapped))
      return result;
    result.forwarded=true;
    // Native source rereads [EDI+4] AFTER forwarding to 0x22282370.
    std::uint32_t current_limit=0u;
    if(!read(Stage::ReadResourceMax4,source_record,4u,
             current_limit))return result;
    const std::uint32_t new_limit=
        static_cast<std::uint32_t>(mapped)+1u;
    if(sx(current_limit)<sx(new_limit)) {
      if(!write(Stage::WriteResourceMax4,source_record,4u,
                 new_limit))return result;
      result.resource_count_written=true;
    }
    result.code=Code::Registered;
    result.stage=Stage::None;
    return result;
  } catch(...) {
    result.code=Code::HostRejected;
    return result;
  }
}
Result TaskWrapper::add_task(
    std::uint32_t mapping_owner_guest_va,
    std::uint32_t source_symbol,std::int32_t local_id,
    std::uint32_t display_name_guest_va)noexcept {
  if(mapping_owner_guest_va==0u)
    return {Code::OriginalNullNodeHazard,Stage::ResolveWrapperNode};
  if(mapping_owner_guest_va>
      std::numeric_limits<std::uint32_t>::max()-kOriginalTaskNodeOffset)
    return {Code::OriginalAddressOverflow,Stage::ResolveWrapperNode};
  // 0x22088FA0: push name; push literal "task"; push local ID;
  // push symbol; add ECX,0x1C; call 0x22073760; ret 0x0C.
  return registrar_.add(mapping_owner_guest_va+kOriginalTaskNodeOffset,
        source_symbol,local_id,kOriginalTaskLabelVA,
        display_name_guest_va);
}
} // namespace sin_compat::original_elite_range_registration_phase637
