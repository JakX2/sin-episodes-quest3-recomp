#pragma once
// Original ServerGameEnts001 seven-virtual-slot native PE32 dispatch.
// This is an independently authored, host-bound diagnostic object.
// It is NOT registered in CreateInterface or safe to hand to a retail engine.
// Host callbacks prove pointer ownership and bounded buffer lifetimes.
// No retail executable, CBaseEntity, edict_t, or CCheckTransmitInfo is cloned.
#include "server_gameents_abi.h"
#include "server_entity_conversion_core.h"
#include "server_entity_release_core.h"
#include "server_gameents_touch_contact_bridge.h"
#include "server_gameents_transmit_entry_core.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sin_independent {

// Forward declaration permits typed owner callbacks without changing the
// original ServerGameEnts001 x86 virtual ABI or slot numbering.
enum class GameEntsDispatchSlot : unsigned char;

struct GameEntsBoundedOwner {
    void* context=nullptr;
    // These callbacks grant and release a stable owner-side access lease
    // covering all original-layout pointers and nested owner callbacks.
    // The returned token is opaque and must be released exactly once with
    // the same slot. Failed acquire MUST NOT leave a lease held.
    bool (*begin_access)(void*,GameEntsDispatchSlot,std::uint64_t*) noexcept=nullptr;
    void (*end_access)(void*,GameEntsDispatchSlot,std::uint64_t) noexcept=nullptr;
    // Must authenticate an owner-live PE32 edict prefix of at least 0x10,
    // including the original unknown pointer at +0x0c, without dereference.
    bool (*valid_edict)(void*,const edict_t*) noexcept=nullptr;
    // Must authenticate live CBaseEntity bytes through original +0x16f
    // BEFORE the native +0x16c pointer read.
    bool (*valid_entity)(void*,const CBaseEntity*) noexcept=nullptr;
    // Authenticate live unknown-interface dispatch target and synthetic
    // TouchEntity storage before any virtual call or field dereference.
    bool (*valid_unknown)(void*,const ServerUnknownAbi*) noexcept=nullptr;
    // The unknown object must belong to this EXACT edict generation before
    // issuing its +0x14 virtual GetBaseEntity dispatch. A valid unknown
    // pointer from a second owned actor is not sufficient.
    bool (*valid_unknown_pair)(void*,const edict_t*,
                               const ServerUnknownAbi*) noexcept=nullptr;
    bool (*valid_touch)(void*,const TouchEntity*) noexcept=nullptr;
    // Must prove that this edict and entity refer to the SAME live owner
    // generation, not merely two individually approved heap addresses.
    // The host checks a trusted edict serial/handle registry under lease.
    bool (*valid_pair)(void*,const edict_t*,const CBaseEntity*) noexcept=nullptr;
    // Authenticate the entity against the exact table and slot generation.
    bool (*valid_candidate)(void*,std::uint32_t,std::size_t,
                            const CBaseEntity*) noexcept=nullptr;
    // Must prove stable storage for CCheckTransmitInfo and index array.
    // Count is the original signed input; callback reports index ELEMENTS.
    bool (*transmit_extent)(void*,const CCheckTransmitInfo*,
                            const unsigned short*,int,
                            std::size_t*,std::size_t*) noexcept=nullptr;
    GameEntsContactBridgeHooks touch{};
    EntityReleaseHooks release{};
    GameEntsTransmitEntryHooks transmit{};
    bool Complete() const noexcept {
        return begin_access&&end_access&&
            valid_edict&&valid_entity&&valid_unknown&&
            valid_unknown_pair&&valid_touch&&
            valid_pair&&valid_candidate&&transmit_extent&&
            touch.resolve&&touch.init_trace&&touch.publish_trace&&
            touch.contacts&&touch.entries&&touch.entry_count&&
            touch.touch.Complete()&&release.Complete()&&
            transmit.table&&transmit.recipient&&transmit.bits&&
            transmit.entity&&transmit.policy.Complete();
    }
};

enum class GameEntsDispatchResult : unsigned char {
    NotCalled, Completed, NoEntity, Unauthorized, Unbound, Reentrant,
    InvalidInput, HostFailure, OperationRefused, LeaseDenied
};
enum class GameEntsDispatchSlot : unsigned char {
    SetDebugEdictBase=1, MarkEntitiesAsTouching=2,
    FreeContainingEntity=3, BaseEntityToEdict=4,
    EdictToBaseEntity=5, CheckTransmit=6
};
struct GameEntsDispatchReport {
    GameEntsDispatchSlot slot=GameEntsDispatchSlot::SetDebugEdictBase;
    GameEntsDispatchResult status=GameEntsDispatchResult::NotCalled;
    unsigned invocations[7]{};
    unsigned lease_attempts=0,lease_acquisitions=0,
             lease_releases=0,lease_denials=0;
    GameEntsContactBridgeResult touch{};
    EntityReleaseStatus release=EntityReleaseStatus::NoAssociatedEntity;
    GameEntsTransmitEntryResult transmit{};
    TransmitResult transmit_status=TransmitResult::InvalidInput;
};

// Concrete, complete *diagnostic* vtable. Engine callbacks are immutable
// after construction. Each original void method records a diagnostics result
// without changing its original x86 __thiscall signature/return convention.
class ServerGameEntsBoundedDispatch final : public ServerGameEntsAbi {
public:
    explicit ServerGameEntsBoundedDispatch(
        const GameEntsBoundedOwner& owner) noexcept
        : owner_(owner),ready_(owner.Complete()) {}
    ~ServerGameEntsBoundedDispatch() noexcept override=default;

    ServerGameEntsBoundedDispatch(const ServerGameEntsBoundedDispatch&)=delete;
    ServerGameEntsBoundedDispatch& operator=(const ServerGameEntsBoundedDispatch&)=delete;

    bool Ready() const noexcept {return ready_;}
    const GameEntsDispatchReport& Report() const noexcept {return report_;}
    edict_t* DebugEdictBase() const noexcept {return conversion_.DebugEdictBase();}

    void __thiscall SetDebugEdictBase(edict_t* base) noexcept override {
        const auto gate=Begin(GameEntsDispatchSlot::SetDebugEdictBase);
        if(gate!=GameEntsDispatchResult::Completed)return;
        if(base&&!owner_.valid_edict(owner_.context,base))
            report_.status=GameEntsDispatchResult::Unauthorized;
        else {
            conversion_.SetDebugEdictBase(base);
            report_.status=GameEntsDispatchResult::Completed;
        }
        End();
    }

    void __thiscall MarkEntitiesAsTouching(
        edict_t* first,edict_t* second) noexcept override {
        const auto gate=Begin(GameEntsDispatchSlot::MarkEntitiesAsTouching);
        if(gate!=GameEntsDispatchResult::Completed)return;
        if((first&&(!owner_.valid_edict(owner_.context,first)||
                    !UnknownAuthorized(first)))||
           (second&&(!owner_.valid_edict(owner_.context,second)||
                     !UnknownAuthorized(second)))) {
            report_.status=GameEntsDispatchResult::Unauthorized;
        } else {
            auto bounded_touch=owner_.touch;
            bounded_touch.authorized_touch=owner_.valid_touch;
            bounded_touch.authorized_entity=owner_.valid_entity;
            bounded_touch.authorization_context=owner_.context;
            bounded_touch.authorized_pair=&VerifyContactPair;
            bounded_touch.pair_authorization_context=const_cast<GameEntsBoundedOwner*>(&owner_);
            const auto outcome=MarkEntitiesAsTouchingWithContactGraph(
                first,second,bounded_touch,report_.touch);
            switch(outcome) {
            case GameEntsTouchEntryStatus::Completed:
                report_.status=GameEntsDispatchResult::Completed;break;
            case GameEntsTouchEntryStatus::NoEntities:
                report_.status=GameEntsDispatchResult::NoEntity;break;
            case GameEntsTouchEntryStatus::InvalidInput:
            case GameEntsTouchEntryStatus::InvalidOrigin:
                report_.status=GameEntsDispatchResult::InvalidInput;break;
            case GameEntsTouchEntryStatus::HostFailure:
                report_.status=GameEntsDispatchResult::HostFailure;break;
            }
        }
        End();
    }

    void __thiscall FreeContainingEntity(edict_t* edict) noexcept override {
        const auto gate=Begin(GameEntsDispatchSlot::FreeContainingEntity);
        if(gate!=GameEntsDispatchResult::Completed)return;
        if(edict&&(!owner_.valid_edict(owner_.context,edict)||
                   !UnknownAuthorized(edict))) {
            report_.status=GameEntsDispatchResult::Unauthorized;
        } else {
            report_.release=TryReleaseContainingEntity(
                conversion_,edict,owner_.release,
                owner_.valid_entity,owner_.context,owner_.valid_pair);
            switch(report_.release) {
            case EntityReleaseStatus::DetachedAndHooksInvoked:
                report_.status=GameEntsDispatchResult::Completed;break;
            case EntityReleaseStatus::NoAssociatedEntity:
                report_.status=GameEntsDispatchResult::NoEntity;break;
            case EntityReleaseStatus::MissingCleanupHooks:
                report_.status=GameEntsDispatchResult::OperationRefused;break;
            case EntityReleaseStatus::UnauthorizedEntity:
                report_.status=GameEntsDispatchResult::Unauthorized;break;
            }
        }
        End();
    }

    edict_t* __thiscall BaseEntityToEdict(
        CBaseEntity* entity) noexcept override {
        const auto gate=Begin(GameEntsDispatchSlot::BaseEntityToEdict);
        if(gate!=GameEntsDispatchResult::Completed)return nullptr;
        edict_t* result=nullptr;
        if(entity&&!owner_.valid_entity(owner_.context,entity)) {
            report_.status=GameEntsDispatchResult::Unauthorized;
        } else {
            result=conversion_.BaseEntityToEdict(entity);
            // The original may return a stale edict pointer. Do not pass an
            // unauthenticated owner pointer out of the diagnostic boundary.
            if(result&&(!owner_.valid_edict(owner_.context,result)||
                        !owner_.valid_pair(owner_.context,result,entity))) {
                result=nullptr;
                report_.status=GameEntsDispatchResult::Unauthorized;
            } else report_.status=result
                ?GameEntsDispatchResult::Completed
                :GameEntsDispatchResult::NoEntity;
        }
        End();
        return result;
    }

    CBaseEntity* __thiscall EdictToBaseEntity(
        edict_t* edict) noexcept override {
        const auto gate=Begin(GameEntsDispatchSlot::EdictToBaseEntity);
        if(gate!=GameEntsDispatchResult::Completed)return nullptr;
        CBaseEntity* result=nullptr;
        if(edict&&(!owner_.valid_edict(owner_.context,edict)||
                   !UnknownAuthorized(edict))) {
            report_.status=GameEntsDispatchResult::Unauthorized;
        } else {
            result=conversion_.EdictToBaseEntity(edict);
            if(result&&(!owner_.valid_entity(owner_.context,result)||
                        !owner_.valid_pair(owner_.context,edict,result))) {
                result=nullptr;
                report_.status=GameEntsDispatchResult::Unauthorized;
            } else report_.status=result
                ?GameEntsDispatchResult::Completed
                :GameEntsDispatchResult::NoEntity;
        }
        End();
        return result;
    }

    void __thiscall CheckTransmit(
        CCheckTransmitInfo* info,const unsigned short* indices,
        int count) noexcept override {
        const auto gate=Begin(GameEntsDispatchSlot::CheckTransmit);
        if(gate!=GameEntsDispatchResult::Completed)return;
        std::size_t info_extent=0,index_capacity=0;
        if(!owner_.transmit_extent(owner_.context,info,indices,count,
                                   &info_extent,&index_capacity)) {
            report_.status=GameEntsDispatchResult::Unauthorized;
        } else {
            static_assert(sizeof(unsigned short)==sizeof(std::uint16_t),
                          "original transmit index ABI is 16-bit");
            auto bounded_transmit=owner_.transmit;
            bounded_transmit.authorized_entity=owner_.valid_entity;
            bounded_transmit.authorization_context=owner_.context;
            bounded_transmit.authorized_candidate=owner_.valid_candidate;
            bounded_transmit.candidate_authorization_context=owner_.context;
            report_.transmit_status=CheckTransmitEntry(
                reinterpret_cast<const unsigned char*>(info),info_extent,
                reinterpret_cast<const std::uint16_t*>(indices),
                index_capacity,count,bounded_transmit,report_.transmit);
            switch(report_.transmit_status) {
            case TransmitResult::Completed:
                report_.status=GameEntsDispatchResult::Completed;break;
            case TransmitResult::MissingRecipient:
                report_.status=GameEntsDispatchResult::NoEntity;break;
            case TransmitResult::HostFailure:
                report_.status=report_.transmit.unauthorized_entity
                    ?GameEntsDispatchResult::Unauthorized
                    :GameEntsDispatchResult::HostFailure;break;
            default:
                report_.status=GameEntsDispatchResult::InvalidInput;break;
            }
        }
        End();
    }

private:
    // Raw-edict touch helper requires a void* ABI, while the independent
    // owner contract carries the original typed edict pointer.
    static bool VerifyContactPair(void* p,const void* edict,
                                  const CBaseEntity* entity) noexcept {
        const auto* owner=static_cast<const GameEntsBoundedOwner*>(p);
        return owner->valid_pair(owner->context,
                                 static_cast<const edict_t*>(edict),entity);
    }

    // Release while active_ is still set, so a host release callback cannot
    // reenter the same object between the last operation and lease unlock.
    void End() noexcept {
        owner_.end_access(owner_.context,report_.slot,lease_token_);
        ++report_.lease_releases;
        lease_token_=0;
        active_=false;
    }

    // edict +0x0c is a separate virtual dispatch target, not covered by
    // attesting only the 0x10-byte edict prefix. Never call GetBaseEntity
    // through an unknown interface the owner has not explicitly attested.
    bool UnknownAuthorized(const edict_t* edict) const noexcept {
        if(!edict)return true;
        ServerUnknownAbi* unknown=nullptr;
        const auto* bytes=reinterpret_cast<const unsigned char*>(edict);
        std::memcpy(&unknown,bytes+kOriginalEdictUnknownOffset,sizeof(unknown));
        return !unknown||
            (owner_.valid_unknown(owner_.context,unknown)&&
             owner_.valid_unknown_pair(owner_.context,edict,unknown));
    }

    // The model's host services can call back into game interfaces. Do not
    // reenter a half-mutated synthetic contact/lifetime operation.
    GameEntsDispatchResult Begin(GameEntsDispatchSlot slot) noexcept {
        if(active_)return GameEntsDispatchResult::Reentrant;
        report_.slot=slot;
        const auto number=static_cast<unsigned>(slot);
        ++report_.invocations[number];
        if(!ready_) {
            report_.status=GameEntsDispatchResult::Unbound;
            return report_.status;
        }
        // Mark active BEFORE entering the owner's lease callback: acquiring
        // a lease can pump host callbacks that attempt nested vtable entry.
        active_=true;
        report_.status=GameEntsDispatchResult::NotCalled;
        ++report_.lease_attempts;
        std::uint64_t token=0;
        if(!owner_.begin_access(owner_.context,slot,&token)) {
            ++report_.lease_denials;
            report_.status=GameEntsDispatchResult::LeaseDenied;
            active_=false;
            return report_.status;
        }
        lease_token_=token;
        ++report_.lease_acquisitions;
        return GameEntsDispatchResult::Completed;
    }

    const GameEntsBoundedOwner owner_;
    const bool ready_;
    bool active_=false;
    std::uint64_t lease_token_=0;
    ServerGameEntsConversionCore conversion_{};
    GameEntsDispatchReport report_{};
};

} // namespace sin_independent
