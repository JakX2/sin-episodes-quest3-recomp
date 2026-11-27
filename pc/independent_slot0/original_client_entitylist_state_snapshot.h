// Phase 1657: original-client CClientEntityList startup state snapshot.
//
// Diagnostic read ONLY, after owner-authorized and full-SHA-gated original
// DllMain. This is separate from the nine-method bounded behavior fixture.
// No original entity-list method, engine callback or pointer is invoked.
#pragma once
#include "original_client_entitylist_readonly_snapshot.h"
#include <cstdint>

namespace sin_independent {
namespace original_client_entitylist_state_snapshot {

namespace abi=original_client_entitylist_snapshot;
constexpr std::uintptr_t kNetworkedCountRva=abi::kParentObjectRva+0x10018u;
constexpr std::uintptr_t kMaxEntitiesRva=abi::kParentObjectRva+0x1001Cu;
constexpr std::uintptr_t kFlaggedCountRva=abi::kParentObjectRva+0x10020u;
constexpr std::uintptr_t kHighestIndexRva=abi::kParentObjectRva+0x10024u;
static_assert(kNetworkedCountRva==abi::kSecondaryObjectRva+4u,
              "original entity-list live count offset drift");
static_assert(kMaxEntitiesRva==abi::kSecondaryObjectRva+8u,
              "original entity-list max offset drift");
static_assert(kFlaggedCountRva==abi::kSecondaryObjectRva+12u,
              "original entity-list flagged count offset drift");
static_assert(kHighestIndexRva==abi::kSecondaryObjectRva+16u,
              "original entity-list highest slot offset drift");

enum class Status {
    Valid,
    MissingReader,
    InvalidBase,
    LiveCountUnreadable,
    MaxEntitiesUnreadable,
    FlaggedCountUnreadable,
    HighestIndexUnreadable,
    CountsOutOfRange,
    HighestIndexContradiction,
};
struct Result {
    Status status=Status::MissingReader;
    std::uint32_t networked_count=0u;
    std::uint32_t max_entities=0u;
    std::uint32_t secondary_flagged_count=0u;
    std::int32_t highest_live_index=-1;
    unsigned successful_reads=0u;
};
inline const char* StatusName(Status status) noexcept {
    switch(status) {
    case Status::Valid:return "bounded_state_read";
    case Status::MissingReader:return "missing_reader";
    case Status::InvalidBase:return "invalid_module_base";
    case Status::LiveCountUnreadable:return "live_count_unreadable";
    case Status::MaxEntitiesUnreadable:return "max_entities_unreadable";
    case Status::FlaggedCountUnreadable:return "flagged_count_unreadable";
    case Status::HighestIndexUnreadable:return "highest_index_unreadable";
    case Status::CountsOutOfRange:return "counts_out_of_range";
    case Status::HighestIndexContradiction:return "highest_index_contradiction";
    }
    return "unknown_status";
}
inline Result Observe(std::uintptr_t base,abi::Reader reader) noexcept {
    Result out{};
    if(!reader.read)return out;
    // Do not allow 32-bit original module addresses to wrap.
    if(!base || base>0xFFFFFFFFu-kHighestIndexRva-4u) {
        out.status=Status::InvalidBase;
        return out;
    }
    std::uint32_t highest_bits=0u;
    if(!reader.read(reader.context,base+kNetworkedCountRva,
                    &out.networked_count)) {
        out.status=Status::LiveCountUnreadable;return out;
    }
    ++out.successful_reads;
    if(!reader.read(reader.context,base+kMaxEntitiesRva,
                    &out.max_entities)) {
        out.status=Status::MaxEntitiesUnreadable;return out;
    }
    ++out.successful_reads;
    if(!reader.read(reader.context,base+kFlaggedCountRva,
                    &out.secondary_flagged_count)) {
        out.status=Status::FlaggedCountUnreadable;return out;
    }
    ++out.successful_reads;
    if(!reader.read(reader.context,base+kHighestIndexRva,&highest_bits)) {
        out.status=Status::HighestIndexUnreadable;return out;
    }
    ++out.successful_reads;
    out.highest_live_index=static_cast<std::int32_t>(highest_bits);
    if(out.networked_count>2048u || out.max_entities>4096u ||
       out.secondary_flagged_count>4096u) {
        out.status=Status::CountsOutOfRange;return out;
    }
    // Original maximum reported occupied index is 0x7FF and empty -1.
    if(out.highest_live_index<-1 || out.highest_live_index>=2048 ||
       (out.networked_count==0 && out.highest_live_index!=-1) ||
       (out.networked_count!=0 && out.highest_live_index<0) ||
       (out.highest_live_index>=0 &&
        out.networked_count>static_cast<std::uint32_t>(
                                      out.highest_live_index+1))) {
        out.status=Status::HighestIndexContradiction;return out;
    }
    out.status=Status::Valid;
    return out;
}
} // namespace original_client_entitylist_state_snapshot
} // namespace sin_independent
