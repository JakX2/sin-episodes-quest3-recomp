#pragma once
// Original-guided ServerGameEnts001::CheckTransmit policy over independently
// supplied entity metadata and bounded bitsets; not a retail ABI overlay.
#include "server_entity_handle_core.h"
#include <cstdint>
#include <cstddef>

namespace sin_independent {
constexpr std::uint32_t kTransmitFullCheck=0;
constexpr std::uint32_t kTransmitAlways=0x08;
constexpr std::uint32_t kTransmitDontSend=0x10;
constexpr std::uint32_t kTransmitPvsCheck=0x20;
constexpr std::uint32_t kTransmitMask=0x38;

struct TransmitEntry {
    std::uint32_t flags=0;
    CBaseEntity* entity=nullptr;
};
struct TransmitRecipient {
    bool valid=false;
    bool hltv=false;
    int skyboxArea=0;
};
struct TransmitBits {
    std::uint32_t* words=nullptr;
    std::size_t wordCount=0;
    bool Covers(std::size_t index) const noexcept {
        return words && index/32<wordCount;
    }
    bool Test(std::size_t index) const noexcept {
        return Covers(index) && (words[index/32] & (std::uint32_t{1}<<(index%32)));
    }
    void Set(std::size_t index) const noexcept {
        if(Covers(index)) words[index/32]|=std::uint32_t{1}<<(index%32);
    }
};
struct TransmitContext {
    TransmitRecipient recipient{};
    TransmitBits selected{}, always{};
};
struct TransmitHooks {
    std::uint32_t (*shouldTransmit)(CBaseEntity*,TransmitContext&,void*) noexcept=nullptr;
    int (*area)(CBaseEntity*,void*) noexcept=nullptr;
    bool (*inPvs)(CBaseEntity*,TransmitContext&,void*) noexcept=nullptr;
    // This adapter owns actual SetTransmit side effects (including parent/
    // dependent entity propagation). It must update the selected bits
    // as appropriate, so repeated indices follow the original fast path.
    void (*setTransmit)(CBaseEntity*,std::size_t,TransmitContext&,bool,void*) noexcept=nullptr;
    void* context=nullptr;
    bool Complete() const noexcept {
        return shouldTransmit && area && inPvs && setTransmit;
    }
};
enum class TransmitResult { Completed, MissingRecipient, InvalidInput, HostFailure };

// Independent policy. Caller resolves the recipient and supplies stable
// typed entities; callback implementations own networking/PVS state.
// Preflight validates all indices before any writes or calls, unlike the
// trusted original engine data path. Missing entities are skipped rather
// than invoking a virtual function through an invalid pointer.
// Shared policy loop for both typed records and the bounded original PE32
// record adapter. Entity resolution is deferred until the direct-bit and
// already-selected branches have been excluded.
template<class ReadFlags,class ResolveEntity>
inline TransmitResult CheckTransmitAccess(
    std::size_t entryCount,const std::uint16_t* indices,std::size_t indexCount,
    TransmitContext& ctx,const TransmitHooks& hooks,
    ReadFlags readFlags,ResolveEntity resolveEntity) noexcept {
    if(!ctx.recipient.valid) return TransmitResult::MissingRecipient;
    if(indexCount==0) return TransmitResult::Completed;
    if(!indices || !hooks.Complete())
        return TransmitResult::InvalidInput;
    for(std::size_t i=0;i<indexCount;++i) {
        const auto index=indices[i];
        if(index>=entryCount || !ctx.selected.Covers(index) ||
           (ctx.recipient.hltv && !ctx.always.Covers(index)))
            return TransmitResult::InvalidInput;
    }
    // Original CheckTransmit snapshots these from the recipient before
    // making entity callbacks, which may update its state for later frames.
    const auto recipient=ctx.recipient;
    for(std::size_t i=0;i<indexCount;++i) {
        const std::size_t index=indices[i];
        auto mode=readFlags(index)&kTransmitMask;
        if((mode&kTransmitDontSend) || ctx.selected.Test(index)) continue;
        if(mode&kTransmitAlways) {
            // Original fast path sets bits directly; it does NOT invoke
            // CBaseEntity::SetTransmit or require a base entity.
            ctx.selected.Set(index);
            if(recipient.hltv) ctx.always.Set(index);
            continue;
        }
        CBaseEntity* entity=nullptr;
        if(!resolveEntity(index,entity)) return TransmitResult::HostFailure;
        if(!entity) continue;
        if(mode==kTransmitFullCheck) {
            mode=hooks.shouldTransmit(entity,ctx,hooks.context);
            if(mode==kTransmitAlways) {
                hooks.setTransmit(entity,index,ctx,true,hooks.context);
                continue;
            }
        }
        if(mode!=kTransmitPvsCheck) continue;
        const bool skybox=hooks.area(entity,hooks.context)==recipient.skyboxArea;
        if(skybox) {
            hooks.setTransmit(entity,index,ctx,true,hooks.context);
        } else if(recipient.hltv || hooks.inPvs(entity,ctx,hooks.context)) {
            hooks.setTransmit(entity,index,ctx,false,hooks.context);
        }
    }
    return TransmitResult::Completed;
}
// Existing typed entry ABI remains available, sharing the same decision loop.
inline TransmitResult CheckTransmit(
    const TransmitEntry* entries,std::size_t entryCount,
    const std::uint16_t* indices,std::size_t indexCount,
    TransmitContext& ctx,const TransmitHooks& hooks) noexcept {
    if(!ctx.recipient.valid) return TransmitResult::MissingRecipient;
    if(indexCount && !entries) return TransmitResult::InvalidInput;
    return CheckTransmitAccess(entryCount,indices,indexCount,ctx,hooks,
        [entries](std::size_t i) noexcept {return entries[i].flags;},
        [entries](std::size_t i,CBaseEntity*& entity) noexcept {
            entity=entries[i].entity;return true;
        });
}
} // namespace sin_independent
