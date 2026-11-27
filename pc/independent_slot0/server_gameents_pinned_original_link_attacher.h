#pragma once
// Original 2006 PE32 CServerNetworkProperty reciprocal binding, reconstructed
// using trusted host pins and independently owned synthetic state.
// The original engine/server DLLs are NEVER called or loaded here.
// Not a replacement retail ServerGameEnts001 factory.
#include "server_gameents_owner_lifetime_registry.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace sin_independent {

// SHA-authenticated original server.dll path (Phase 1643):
// property embedded in CBaseEntity at +0x160, property+8 == outer entity,
// property+0x0c == edict, and edict+0x0c == IServerUnknown == entity.
// All three stored pointers are *little-endian PE32*, even in x64 fixtures.
constexpr std::size_t kOriginalNetworkPropertyOffset=0x160;
constexpr std::size_t kOriginalNetworkPropertyOuterOffset=0x168;
constexpr std::size_t kOriginalNetworkPropertyEdictOffset=0x16c;
constexpr std::size_t kOriginalEdictUnknownLinkOffset=0x0c;
constexpr std::size_t kOriginalPE32EdictBytes=0x14;
constexpr std::size_t kOriginalEntityLinkMinimumBytes=0x170;

// The host provides an exclusive, physically stable simultaneous pin for
// edict, entity, and touch backing. Entity/edict PE32 addresses are native
// addresses on x86; on non-x86 they are *fixture address tokens only*.
struct GameEntsOriginalLinkSnapshot {
    const void* edict=nullptr;
    std::size_t edict_bytes=0;
    const void* entity=nullptr;
    std::size_t entity_bytes=0;
    const void* touch=nullptr;
    std::size_t touch_bytes=0;
    std::uint32_t edict_pe32=0;
    std::uint32_t entity_pe32=0;
    std::uint64_t epoch=0;
    std::uint64_t pin_token=0;
};

struct GameEntsOriginalLinkHost {
    void* context=nullptr;
    bool (*acquire_pin)(void*,GameEntsOriginalLinkSnapshot*) noexcept=nullptr;
    // Must authenticate backing WITHOUT following untrusted raw pointers.
    // The host, not this adapter, guarantees pinned allocations' lifetime.
    bool (*validate_pin)(void*,const GameEntsOriginalLinkSnapshot*) noexcept=nullptr;
    bool (*end_pin)(void*,std::uint64_t) noexcept=nullptr;
    bool Complete() const noexcept {
        return acquire_pin&&validate_pin&&end_pin;
    }
};

enum class GameEntsOriginalLinkStatus : unsigned char {
    InvalidHost, Busy, Quarantined, PinUnavailable, BadSnapshot,
    PinRevoked, MismatchedLinks, RegistryRefused, PinReleaseFailed, Activated
};

// This object must outlive all concurrent calls. On a partial successful
// activation followed by a failed unpin it is permanently quarantined;
// no rollback of an entity that the original server may already own.
template<std::size_t Slots,std::size_t Candidates>
class GameEntsPinnedOriginalLinkAttacher final {
public:
    using Registry=GameEntsOwnerLifetimeRegistry<Slots,Candidates>;
    GameEntsPinnedOriginalLinkAttacher() noexcept=default;
    GameEntsPinnedOriginalLinkAttacher(
        const GameEntsPinnedOriginalLinkAttacher&)=delete;
    GameEntsPinnedOriginalLinkAttacher& operator=(
        const GameEntsPinnedOriginalLinkAttacher&)=delete;

    bool Quarantined() const noexcept {
        return quarantined_.load(std::memory_order_acquire);
    }

    GameEntsOriginalLinkStatus Attach(
        Registry& registry,std::size_t slot,
        const GameEntsOwnerIdentity& identity,
        const GameEntsOriginalLinkHost& host) noexcept {
        using S=GameEntsOriginalLinkStatus;
        if(!host.Complete())return S::InvalidHost;
        if(busy_.test_and_set(std::memory_order_acquire))return S::Busy;
        struct BusyGuard {
            std::atomic_flag& flag;
            ~BusyGuard(){flag.clear(std::memory_order_release);}
        } const busy_guard{busy_};
        if(Quarantined())return S::Quarantined;

        GameEntsOriginalLinkSnapshot view{};
        if(!host.acquire_pin(host.context,&view))return S::PinUnavailable;
        bool pin_held=true;
        auto release=[&]() noexcept {
            if(!pin_held)return false;
            pin_held=false;
            return host.end_pin(host.context,view.pin_token);
        };
        // Not a C++ exception unwind contract: every return below flows
        // through a single release, and each successful acquire has one end.
        S result=S::BadSnapshot;
        if(!WellFormed(view,identity)) {
            result=S::BadSnapshot;
        } else if(!host.validate_pin(host.context,&view)) {
            result=S::PinRevoked;
        } else {
            const auto* edict=static_cast<const unsigned char*>(view.edict);
            const auto* entity=static_cast<const unsigned char*>(view.entity);
            const bool authentic_links=
                ReadPE32(entity+kOriginalNetworkPropertyOuterOffset)==
                    view.entity_pe32&&
                ReadPE32(entity+kOriginalNetworkPropertyEdictOffset)==
                    view.edict_pe32&&
                ReadPE32(edict+kOriginalEdictUnknownLinkOffset)==
                    view.entity_pe32;
            // Host must preserve physical extent while reading, not only
            // pass individual pointer checks before and after each read.
            if(!host.validate_pin(host.context,&view)) {
                result=S::PinRevoked;
            } else if(!authentic_links) {
                result=S::MismatchedLinks;
            } else if(!registry.ActivateReservedEntity(slot,identity)) {
                result=S::RegistryRefused;
            } else {
                result=S::Activated;
            }
        }
        if(!release()) {
            quarantined_.store(true,std::memory_order_release);
            return S::PinReleaseFailed;
        }
        return result;
    }

private:
    static bool ValidRange(const void* address,std::size_t bytes) noexcept {
        if(!address||!bytes)return false;
        const auto start=reinterpret_cast<std::uintptr_t>(address);
        return start&&bytes<=
            (std::numeric_limits<std::uintptr_t>::max)()-start;
    }
    static bool WellFormed(const GameEntsOriginalLinkSnapshot& v,
                           const GameEntsOwnerIdentity& identity) noexcept {
        if(!v.pin_token||!v.epoch||!v.edict_pe32||!v.entity_pe32||
           v.edict_pe32==v.entity_pe32||
           v.edict_pe32>(std::numeric_limits<std::uint32_t>::max)()-
               kOriginalPE32EdictBytes||
           v.entity_pe32>(std::numeric_limits<std::uint32_t>::max)()-
               kOriginalEntityLinkMinimumBytes||
           v.edict_bytes!=kOriginalPE32EdictBytes||
           v.entity_bytes<kOriginalEntityLinkMinimumBytes||
           !v.touch_bytes||!ValidRange(v.edict,v.edict_bytes)||
           !ValidRange(v.entity,v.entity_bytes)||
           !ValidRange(v.touch,v.touch_bytes)||
           identity.edict!=v.edict||
           identity.edict_bytes!=v.edict_bytes||
           identity.entity!=v.entity||
           identity.entity_bytes!=v.entity_bytes||
           identity.touch!=v.touch||
           identity.touch_bytes!=v.touch_bytes||
           identity.unknown!=identity.entity||
           identity.unknown_bytes<sizeof(void*)||
           identity.unknown_bytes>identity.entity_bytes||
           identity.generation!=v.epoch)return false;
        // A 64-bit independent reference fixture has distinct synthetic
        // PE32 tokens, but a real Win32 runtime must use native addresses.
        if(sizeof(void*)==4&&
           (v.edict_pe32!=reinterpret_cast<std::uintptr_t>(v.edict)||
            v.entity_pe32!=reinterpret_cast<std::uintptr_t>(v.entity)))
            return false;
        return true;
    }
    static std::uint32_t ReadPE32(const unsigned char* p) noexcept {
        return std::uint32_t(p[0])|
               (std::uint32_t(p[1])<<8)|
               (std::uint32_t(p[2])<<16)|
               (std::uint32_t(p[3])<<24);
    }

    std::atomic_flag busy_=ATOMIC_FLAG_INIT;
    std::atomic<bool> quarantined_{false};
};
} // namespace sin_independent
