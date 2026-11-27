#pragma once
// Original-guided ServerGameEnts001::CheckTransmit entry adapter.
// PE32 addresses are opaque 32-bit owner tokens, never host pointer casts.
// The owner supplies stable table storage, recipient state and writable
// bitset extents; existing CheckTransmitAccess owns the shared policy loop.
#include "server_transmit_policy_core.h"
#include <cstddef>
#include <cstdint>

namespace sin_independent {
constexpr std::size_t kGameEntsTransmitRecordBytes=20;
constexpr std::size_t kGameEntsTransmitEntityOffset=0x0c;
constexpr std::size_t kGameEntsTransmitSelectedOffset=0x2008;
constexpr std::size_t kGameEntsTransmitAlwaysOffset=0x200c;

struct GameEntsTransmitTable {
    const unsigned char* records=nullptr;
    std::size_t bytes=0;
    std::uint32_t base_token=0; // original returned table pointer, also edict fallback
};
struct GameEntsTransmitEntryHooks {
    void* context=nullptr;
    bool (*table)(void*,int,GameEntsTransmitTable*) noexcept=nullptr;
    // Original edict+0xc -> virtual+0x14, then recipient +828/+92c snapshot.
    bool (*recipient)(void*,std::uint32_t,TransmitRecipient*) noexcept=nullptr;
    // Resolve bounded bitsets from the context's original PE32 pointer tokens.
    bool (*bits)(void*,std::uint32_t,TransmitBits*) noexcept=nullptr;
    bool (*entity)(void*,std::uint32_t,CBaseEntity**) noexcept=nullptr;
    TransmitHooks policy{};
    // Bound diagnostic users attest callback-returned entity pointers BEFORE
    // policy callbacks; left optional for existing lower-level fixtures.
    bool (*authorized_entity)(void*,const CBaseEntity*) noexcept=nullptr;
    void* authorization_context=nullptr;
    // Optional on the isolated core; mandatory for bounded original vtable.
    bool (*authorized_candidate)(void*,std::uint32_t,std::size_t,
                                 const CBaseEntity*) noexcept=nullptr;
    void* candidate_authorization_context=nullptr;
};
struct GameEntsTransmitEntryResult {
    unsigned table_calls=0,recipient_calls=0,bitset_calls=0,entity_calls=0;
    bool fallback=false;
    bool unauthorized_entity=false;
    TransmitContext policy_context{};
};

inline std::uint32_t GameEntsReadU32(const unsigned char* p) noexcept {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1])<<8) |
           (std::uint32_t(p[2])<<16) | (std::uint32_t(p[3])<<24);
}

inline TransmitResult CheckTransmitEntry(
    const unsigned char* info,std::size_t info_bytes,
    const std::uint16_t* indices,std::size_t index_capacity,int count,
    const GameEntsTransmitEntryHooks& hooks,
    GameEntsTransmitEntryResult& out) noexcept {
    out={};
    if(!info||info_bytes<4||!hooks.table||!hooks.recipient)
        return TransmitResult::InvalidInput;
    // Original 221e94de obtains candidate storage first. When info+0
    // is null, 221e94f8 repeats the query ONLY to obtain the recipient
    // edict. It never replaces the saved first candidate-table pointer.
    GameEntsTransmitTable primary{};
    ++out.table_calls;
    if(!hooks.table(hooks.context,0,&primary))return TransmitResult::HostFailure;
    auto recipient_token=GameEntsReadU32(info);
    if(!recipient_token) {
        out.fallback=true;
        GameEntsTransmitTable fallback{};
        ++out.table_calls;
        if(!hooks.table(hooks.context,0,&fallback))return TransmitResult::HostFailure;
        recipient_token=fallback.base_token;
    }
    if(!recipient_token)return TransmitResult::MissingRecipient;
    auto& ctx=out.policy_context;
    ++out.recipient_calls;
    if(!hooks.recipient(hooks.context,recipient_token,&ctx.recipient))
        return TransmitResult::HostFailure;
    if(!ctx.recipient.valid)return TransmitResult::MissingRecipient;
    // The original resolves and snapshots the recipient before its signed
    // count<=0 exit. No candidate records or bitsets are needed on that path.
    if(count<=0)return TransmitResult::Completed;
    const auto work=static_cast<std::size_t>(count);
    if(work>index_capacity||!indices||!primary.records||
       primary.bytes%kGameEntsTransmitRecordBytes!=0||
       info_bytes<kGameEntsTransmitAlwaysOffset+4||
       !hooks.bits||!hooks.entity||!hooks.policy.Complete())
        return TransmitResult::InvalidInput;
    // Independent safety: obtain bounded stable bitsets once, before writes.
    // Retail reads the corresponding raw pointers during the candidate loop.
    const auto selected_token=GameEntsReadU32(info+kGameEntsTransmitSelectedOffset);
    if(!selected_token)return TransmitResult::InvalidInput;
    ++out.bitset_calls;
    if(!hooks.bits(hooks.context,selected_token,&ctx.selected))
        return TransmitResult::HostFailure;
    if(ctx.recipient.hltv) {
        const auto always_token=GameEntsReadU32(info+kGameEntsTransmitAlwaysOffset);
        if(!always_token)return TransmitResult::InvalidInput;
        ++out.bitset_calls;
        if(!hooks.bits(hooks.context,always_token,&ctx.always))
            return TransmitResult::HostFailure;
    }
    const auto records=primary.records;
    const auto table_token=primary.base_token;
    return CheckTransmitAccess(primary.bytes/kGameEntsTransmitRecordBytes,
        indices,work,ctx,hooks.policy,
        [records](std::size_t i) noexcept {
            return GameEntsReadU32(records+i*kGameEntsTransmitRecordBytes);
        },
        [&hooks,&out,records,table_token](std::size_t i,CBaseEntity*& entity) noexcept {
            const auto token=GameEntsReadU32(records+i*kGameEntsTransmitRecordBytes+
                                          kGameEntsTransmitEntityOffset);
            if(!token){entity=nullptr;return true;}
            ++out.entity_calls;
            if(!hooks.entity(hooks.context,token,&entity))return false;
            if(entity&&hooks.authorized_entity&&
               !hooks.authorized_entity(hooks.authorization_context,entity)) {
                out.unauthorized_entity=true;
                return false;
            }
            if(entity&&hooks.authorized_candidate&&
               !hooks.authorized_candidate(
                   hooks.candidate_authorization_context,table_token,i,entity)) {
                out.unauthorized_entity=true;
                return false;
            }
            return true;
        });
}
} // namespace sin_independent
