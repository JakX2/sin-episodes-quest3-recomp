#pragma once
// Original-guided ServerGameEnts001::CheckTransmit entry (0x221e94d0).
// The engine retains the global table, edict/networkable recipient lookup,
// two bitsets and the actual PVS, ShouldTransmit and SetTransmit services.
// This wrapper composes those owners with the existing transmit policy; it
// is not a retail CCheckTransmitInfo or CBaseEntity memory overlay.
#include "server_transmit_policy_core.h"
#include <cstddef>
#include <cstdint>

namespace sin_independent {
struct GameEntsTransmitEntryHooks {
    void* context=nullptr;
    // Original manager at 0x22572198, virtual +0x4c(arg 0). The result
    // represents indexed 20-byte entry records decoded by the engine.
    bool (*table)(void*,const TransmitEntry**,std::size_t*) noexcept=nullptr;
    // Resolve recipient from CCheckTransmitInfo or original fallback;
    // source owns +0x828 area and +0x92c special-recipient reads.
    bool (*recipient)(void*,const void*,TransmitRecipient*) noexcept=nullptr;
    // Source-owned CCheckTransmitInfo +0x2008/+0x200c selected/always
    // bitsets. Bounded native view, no guessed CCheckTransmitInfo layout.
    bool (*bitsets)(void*,const void*,TransmitBits*,TransmitBits*) noexcept=nullptr;
    TransmitHooks policy{};
};
enum class GameEntsTransmitEntryStatus {
    Completed, NoRecipient, InvalidInput, HostFailure
};
struct GameEntsTransmitEntryResult {
    unsigned table_calls=0,recipient_calls=0,bitset_calls=0,
             candidates=0,policy_calls=0,empty=0;
    TransmitRecipient snapshot{};
    TransmitResult policy_status=TransmitResult::InvalidInput;
};

inline GameEntsTransmitEntryStatus CheckTransmitEntry(
    const void* recipient_info,const std::uint16_t* indices,int count,
    const GameEntsTransmitEntryHooks& hooks,
    GameEntsTransmitEntryResult& result) noexcept {
    result={};
    if(!hooks.table||!hooks.recipient)
        return GameEntsTransmitEntryStatus::InvalidInput;

    const TransmitEntry* entries=nullptr;
    std::size_t entry_count=0;
    ++result.table_calls;
    if(!hooks.table(hooks.context,&entries,&entry_count))
        return GameEntsTransmitEntryStatus::HostFailure;

    TransmitRecipient recipient{};
    ++result.recipient_calls;
    if(!hooks.recipient(hooks.context,recipient_info,&recipient))
        return GameEntsTransmitEntryStatus::HostFailure;
    result.snapshot=recipient;
    if(!recipient.valid)
        return GameEntsTransmitEntryStatus::NoRecipient;
    // The original checks the signed count before iterating candidate
    // indices. For <=0 there is no bitset access or per-entity callback.
    if(count<=0) {
        result.empty=1;
        result.policy_status=TransmitResult::Completed;
        return GameEntsTransmitEntryStatus::Completed;
    }
    if(!indices||!hooks.bitsets||!hooks.policy.Complete())
        return GameEntsTransmitEntryStatus::InvalidInput;
    TransmitBits selected{},always{};
    ++result.bitset_calls;
    if(!hooks.bitsets(hooks.context,recipient_info,&selected,&always))
        return GameEntsTransmitEntryStatus::HostFailure;
    TransmitContext ctx{recipient,selected,always};
    result.candidates=static_cast<unsigned>(count);
    ++result.policy_calls;
    result.policy_status=CheckTransmit(entries,entry_count,indices,
        static_cast<std::size_t>(count),ctx,hooks.policy);
    return result.policy_status==TransmitResult::Completed
        ? GameEntsTransmitEntryStatus::Completed
        : result.policy_status==TransmitResult::MissingRecipient
        ? GameEntsTransmitEntryStatus::NoRecipient
        : GameEntsTransmitEntryStatus::InvalidInput;
}
} // namespace sin_independent
