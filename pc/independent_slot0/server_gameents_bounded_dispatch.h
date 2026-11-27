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

namespace sin_independent {

struct GameEntsBoundedOwner {
    void* context=nullptr;
    // Must authenticate an owner-live PE32 edict prefix of at least 0x10,
    // including the original unknown pointer at +0x0c, without dereference.
    bool (*valid_edict)(void*,const edict_t*) noexcept=nullptr;
    // Must authenticate live CBaseEntity bytes through original +0x16f
    // BEFORE the native +0x16c pointer read.
    bool (*valid_entity)(void*,const CBaseEntity*) noexcept=nullptr;
    // Must prove stable storage for CCheckTransmitInfo and index array.
    // Count is the original signed input; callback reports index ELEMENTS.
    bool (*transmit_extent)(void*,const CCheckTransmitInfo*,
                            const unsigned short*,int,
                            std::size_t*,std::size_t*) noexcept=nullptr;
    GameEntsContactBridgeHooks touch{};
    EntityReleaseHooks release{};
    GameEntsTransmitEntryHooks transmit{};
    bool Complete() const noexcept {
        return valid_edict&&valid_entity&&transmit_extent&&
            touch.resolve&&touch.init_trace&&touch.publish_trace&&
            touch.contacts&&touch.entries&&touch.entry_count&&
            touch.touch.Complete()&&release.Complete()&&
            transmit.table&&transmit.recipient&&transmit.bits&&
            transmit.entity&&transmit.policy.Complete();
    }
};

enum class GameEntsDispatchResult : unsigned char {
    NotCalled, Completed, NoEntity, Unauthorized, Unbound, Reentrant,
    InvalidInput, HostFailure, OperationRefused
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
        active_=false;
    }

    void __thiscall MarkEntitiesAsTouching(
        edict_t* first,edict_t* second) noexcept override {
        const auto gate=Begin(GameEntsDispatchSlot::MarkEntitiesAsTouching);
        if(gate!=GameEntsDispatchResult::Completed)return;
        if((first&&!owner_.valid_edict(owner_.context,first))||
           (second&&!owner_.valid_edict(owner_.context,second))) {
            report_.status=GameEntsDispatchResult::Unauthorized;
        } else {
            const auto outcome=MarkEntitiesAsTouchingWithContactGraph(
                first,second,owner_.touch,report_.touch);
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
        active_=false;
    }

    void __thiscall FreeContainingEntity(edict_t* edict) noexcept override {
        const auto gate=Begin(GameEntsDispatchSlot::FreeContainingEntity);
        if(gate!=GameEntsDispatchResult::Completed)return;
        if(edict&&!owner_.valid_edict(owner_.context,edict)) {
            report_.status=GameEntsDispatchResult::Unauthorized;
        } else {
            report_.release=TryReleaseContainingEntity(
                conversion_,edict,owner_.release);
            switch(report_.release) {
            case EntityReleaseStatus::DetachedAndHooksInvoked:
                report_.status=GameEntsDispatchResult::Completed;break;
            case EntityReleaseStatus::NoAssociatedEntity:
                report_.status=GameEntsDispatchResult::NoEntity;break;
            case EntityReleaseStatus::MissingCleanupHooks:
                report_.status=GameEntsDispatchResult::OperationRefused;break;
            }
        }
        active_=false;
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
            if(result&&!owner_.valid_edict(owner_.context,result)) {
                result=nullptr;
                report_.status=GameEntsDispatchResult::Unauthorized;
            } else report_.status=result
                ?GameEntsDispatchResult::Completed
                :GameEntsDispatchResult::NoEntity;
        }
        active_=false;
        return result;
    }

    CBaseEntity* __thiscall EdictToBaseEntity(
        edict_t* edict) noexcept override {
        const auto gate=Begin(GameEntsDispatchSlot::EdictToBaseEntity);
        if(gate!=GameEntsDispatchResult::Completed)return nullptr;
        CBaseEntity* result=nullptr;
        if(edict&&!owner_.valid_edict(owner_.context,edict)) {
            report_.status=GameEntsDispatchResult::Unauthorized;
        } else {
            result=conversion_.EdictToBaseEntity(edict);
            if(result&&!owner_.valid_entity(owner_.context,result)) {
                result=nullptr;
                report_.status=GameEntsDispatchResult::Unauthorized;
            } else report_.status=result
                ?GameEntsDispatchResult::Completed
                :GameEntsDispatchResult::NoEntity;
        }
        active_=false;
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
            report_.transmit_status=CheckTransmitEntry(
                reinterpret_cast<const unsigned char*>(info),info_extent,
                reinterpret_cast<const std::uint16_t*>(indices),
                index_capacity,count,owner_.transmit,report_.transmit);
            switch(report_.transmit_status) {
            case TransmitResult::Completed:
                report_.status=GameEntsDispatchResult::Completed;break;
            case TransmitResult::MissingRecipient:
                report_.status=GameEntsDispatchResult::NoEntity;break;
            case TransmitResult::HostFailure:
                report_.status=GameEntsDispatchResult::HostFailure;break;
            default:
                report_.status=GameEntsDispatchResult::InvalidInput;break;
            }
        }
        active_=false;
    }

private:
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
        active_=true;
        report_.status=GameEntsDispatchResult::NotCalled;
        return GameEntsDispatchResult::Completed;
    }

    const GameEntsBoundedOwner owner_;
    const bool ready_;
    bool active_=false;
    ServerGameEntsConversionCore conversion_{};
    GameEntsDispatchReport report_{};
};

} // namespace sin_independent
