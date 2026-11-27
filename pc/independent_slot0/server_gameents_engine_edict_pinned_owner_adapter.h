#pragma once
// Trusted host pin boundary for independently reconstructed 2006 engine.dll
// edict selection + reuse. Not installed into the retail CreateInterface.
// A pin MUST block physical buffer moves/frees and concurrent owner writes
// until end_pin returns. Callback authorization alone cannot pin memory.
#include "server_gameents_engine_edict_select_core.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace sin_independent {

struct EnginePinnedEdictSnapshot {
    void* array=nullptr;
    std::size_t capacity_bytes=0;
    std::size_t current_count=0;
    std::size_t protected_last=0;
    std::uint64_t token=0;
};

struct EnginePinnedEdictCommit {
    std::size_t index=0;
    std::size_t old_count=0;
    std::size_t new_count=0;
    std::uint64_t epoch=0;
    EngineEdictReuseEvent restamp{};
};

// The host, not this adapter, owns allocation backing and the count. Its
// acquire_pin must take an exclusive, stable physical pin before filling the
// snapshot. validate_pin must not dereference a revoked/unpinned pointer.
// end_pin is required exactly once after every successful acquire, including
// failure paths. Host cleanup is idempotent on a now-empty entity: the
// original selector and subsequent reuse transition both call slot +0x0C.
struct EnginePinnedEdictHost {
    void* context=nullptr;
    bool (*acquire_pin)(void*,EnginePinnedEdictSnapshot*) noexcept=nullptr;
    bool (*validate_pin)(void*,const EnginePinnedEdictSnapshot*) noexcept=nullptr;
    bool (*end_pin)(void*,std::uint64_t) noexcept=nullptr;
    bool (*free_containing_entity)(void*,std::uint64_t,std::size_t,void*) noexcept=nullptr;
    bool (*reserve_epoch)(void*,std::uint64_t,const EngineEdictReuseEvent&,
                          std::uint64_t*) noexcept=nullptr;
    // Atomic, host-controlled update of count + index + the reserved epoch.
    // A failed commit leaves the adapter quarantined, never reports a slot.
    bool (*commit)(void*,std::uint64_t,const EnginePinnedEdictCommit&) noexcept=nullptr;
    bool Complete() const noexcept {
        return acquire_pin&&validate_pin&&end_pin&&free_containing_entity&&
               reserve_epoch&&commit;
    }
};

enum class EnginePinnedEdictStatus : unsigned char {
    InvalidHost, Busy, Quarantined, PinUnavailable, BadSnapshot,
    SelectionFailed, ReuseFailed, CommitFailed, PinReleaseFailed, Activated
};
struct EnginePinnedEdictResult {
    EnginePinnedEdictStatus status=EnginePinnedEdictStatus::InvalidHost;
    EngineEdictSelectStatus selection=EngineEdictSelectStatus::InvalidInput;
    EngineEdictReuseStatus reuse=EngineEdictReuseStatus::InvalidInput;
    std::size_t index=0;
    std::size_t count=0;
    std::uint32_t serial=0;
    std::uint64_t owner_epoch=0;
};

// An adapter instance must outlive its concurrent callers. It never keeps
// or returns a naked edict pointer after the pin is released. Quarantine is
// deliberately permanent: a partially retired slot requires a new owner
// reconciliation/snapshot, not silent retries with an uncertain serial.
class EnginePinnedEdictAllocatorAdapter final {
public:
    explicit EnginePinnedEdictAllocatorAdapter(std::uint64_t prior_host_epoch=0)
        noexcept :latest_reserved_epoch_(prior_host_epoch) {}
    EnginePinnedEdictAllocatorAdapter(const EnginePinnedEdictAllocatorAdapter&)=delete;
    EnginePinnedEdictAllocatorAdapter& operator=(const EnginePinnedEdictAllocatorAdapter&)=delete;

    bool Quarantined() const noexcept {
        return quarantined_.load(std::memory_order_acquire);
    }
    std::uint64_t LatestReservedEpoch() const noexcept {
        return latest_reserved_epoch_.load(std::memory_order_acquire);
    }

    EnginePinnedEdictResult Allocate(const EnginePinnedEdictHost& host,
                                     std::int32_t ticks,float seconds_per_tick)
        noexcept {
        using S=EnginePinnedEdictStatus;
        if(!host.Complete())return {S::InvalidHost};
        if(busy_.test_and_set(std::memory_order_acquire))return {S::Busy};
        struct OperationFlag {
            std::atomic_flag& flag;
            ~OperationFlag(){flag.clear(std::memory_order_release);}
        } const operation{busy_};
        if(Quarantined())return {S::Quarantined};

        Active active{host};
        active.epoch_floor=LatestReservedEpoch();
        if(!host.acquire_pin(host.context,&active.view))
            return {S::PinUnavailable};
        struct PinGuard {
            Active& active;
            bool held=true;
            bool Release() noexcept {
                if(!held)return false;
                held=false;
                return active.host.end_pin(active.host.context,active.view.token);
            }
            ~PinGuard(){if(held)Release();}
        } pin{active};
        EnginePinnedEdictResult result{};
        if(!WellFormed(active.view)||!ArrayAuthorized(&active,active.view.array,
                                                         active.view.capacity_bytes)) {
            result.status=S::BadSnapshot;
        } else {
            const EngineEdictRetireHooks retire{
                &active,&RecordAuthorized,&ArrayAuthorized,&FreeEntity};
            const auto select=SelectEngineEdictForReuse(
                active.view.array,active.view.capacity_bytes,
                active.view.current_count,active.view.protected_last,
                ticks,seconds_per_tick,retire);
            result.selection=select.status;
            if(select.status!=EngineEdictSelectStatus::SelectedExisting&&
               select.status!=EngineEdictSelectStatus::SelectedNew) {
                result.status=S::SelectionFailed;
                // The original cleanup is nontransactional: a callback can
                // reject after the selector has cleared the edict flag.
                if(select.status==EngineEdictSelectStatus::CallbackRefused||
                   select.status==EngineEdictSelectStatus::PostCallbackUnauthorized)
                    quarantined_.store(true,std::memory_order_release);
            } else {
                active.selected_index=select.index;
                active.has_selection=true;
                // A successful original selector already mutated the edict.
                // Any failure thereafter requires explicit owner recovery.
                const EngineEdictReuseHooks reuse_hooks{
                    &active,&ArrayAuthorized,&RecordAuthorized,&FreeEntity,
                    &ReserveEpoch};
                const auto reuse=ReuseEngineEdictAt(
                    active.view.array,active.view.capacity_bytes,
                    static_cast<int>(select.index),active.view.protected_last,
                    ticks,seconds_per_tick,reuse_hooks);
                result.reuse=reuse.status;
                // Reservations are never recycled, including when a later
                // epilogue or commit fails. The next operation must exceed
                // the highest accepted reservation, not the last commit.
                if(active.has_reservation)
                    latest_reserved_epoch_.store(active.reserved_epoch,
                                                 std::memory_order_release);
                if(reuse.status!=EngineEdictReuseStatus::Reused||
                   !active.has_reservation||
                   reuse.owner_epoch!=active.reserved_epoch) {
                    result.status=S::ReuseFailed;
                    quarantined_.store(true,std::memory_order_release);
                } else {
                    const EnginePinnedEdictCommit commit{
                        select.index,active.view.current_count,select.new_count,
                        reuse.owner_epoch,active.restamp};
                    if(!ArrayAuthorized(&active,active.view.array,
                                        active.view.capacity_bytes)||
                       !host.commit(host.context,active.view.token,commit)) {
                        result.status=S::CommitFailed;
                        quarantined_.store(true,std::memory_order_release);
                    } else {
                        result.status=S::Activated;
                        result.index=select.index;
                        result.count=select.new_count;
                        result.serial=reuse.new_serial;
                        result.owner_epoch=reuse.owner_epoch;
                    }
                }
            }
        }
        // Even a committed activation is not visible as success until the
        // host acknowledges unpin. Failed unpin quarantines the adapter.
        if(!pin.Release()) {
            quarantined_.store(true,std::memory_order_release);
            result.status=S::PinReleaseFailed;
            result.index=0;
            result.count=0;
            result.serial=0;
            result.owner_epoch=0;
        }
        return result;
    }

private:
    struct Active {
        EnginePinnedEdictHost host{};
        EnginePinnedEdictSnapshot view{};
        std::uint64_t epoch_floor=0,reserved_epoch=0;
        std::size_t selected_index=0,cleanup_index=0;
        std::uint32_t cleanup_calls=0;
        bool has_selection=false,has_reservation=false;
        EngineEdictReuseEvent restamp{};
    };
    static bool WellFormed(const EnginePinnedEdictSnapshot& v) noexcept {
        constexpr auto kIntMax=static_cast<std::size_t>((std::numeric_limits<int>::max)());
        if(!v.array||!v.token||!v.capacity_bytes||
           v.capacity_bytes%kEngineOriginalEdictStride||
           v.capacity_bytes/kEngineOriginalEdictStride>kIntMax)return false;
        const auto capacity=v.capacity_bytes/kEngineOriginalEdictStride;
        if(v.current_count==0||v.current_count>capacity||
           v.protected_last>=v.current_count)return false;
        const auto base=reinterpret_cast<std::uintptr_t>(v.array);
        return base!=0&&v.capacity_bytes<=
            (std::numeric_limits<std::uintptr_t>::max)()-base;
    }
    static bool ArrayAuthorized(void* opaque,const void* ptr,
                                std::size_t bytes) noexcept {
        auto& a=*static_cast<Active*>(opaque);
        return ptr==a.view.array&&bytes==a.view.capacity_bytes&&
            a.host.validate_pin(a.host.context,&a.view);
    }
    static bool RecordAuthorized(void* opaque,const void* ptr,
                                 std::size_t bytes) noexcept {
        auto& a=*static_cast<Active*>(opaque);
        if(bytes!=kEngineOriginalEdictStride||
           !ArrayAuthorized(opaque,a.view.array,a.view.capacity_bytes))return false;
        auto* base=static_cast<unsigned char*>(a.view.array);
        const auto capacity=a.view.capacity_bytes/kEngineOriginalEdictStride;
        for(std::size_t i=0;i<capacity;++i)
            if(ptr==base+i*kEngineOriginalEdictStride)return true;
        return false;
    }
    static bool FreeEntity(void* opaque,void* edict) noexcept {
        auto& a=*static_cast<Active*>(opaque);
        if(!RecordAuthorized(opaque,edict,kEngineOriginalEdictStride))return false;
        auto* base=static_cast<unsigned char*>(a.view.array);
        const auto capacity=a.view.capacity_bytes/kEngineOriginalEdictStride;
        for(std::size_t i=0;i<capacity;++i) {
            if(edict!=base+i*kEngineOriginalEdictStride)continue;
            if(a.cleanup_calls&&i!=a.cleanup_index)return false;
            a.cleanup_index=i;
            ++a.cleanup_calls;
            return a.host.free_containing_entity(a.host.context,a.view.token,i,edict);
        }
        return false;
    }
    static bool ReserveEpoch(void* opaque,const EngineEdictReuseEvent& e,
                             std::uint64_t* out) noexcept {
        auto& a=*static_cast<Active*>(opaque);
        if(!out||a.has_reservation||!a.has_selection||
           e.index!=a.selected_index||
           !ArrayAuthorized(opaque,a.view.array,a.view.capacity_bytes))return false;
        std::uint64_t epoch=0;
        if(!a.host.reserve_epoch(a.host.context,a.view.token,e,&epoch)||
           epoch<=a.epoch_floor)return false;
        a.epoch_floor=epoch;
        a.reserved_epoch=epoch;
        a.has_reservation=true;
        a.restamp=e;
        *out=epoch;
        // The enclosing Allocate() stores this epoch in its adapter journal
        // immediately after reuse returns, even if later commit fails.
        return true;
    }
    std::atomic_flag busy_=ATOMIC_FLAG_INIT;
    std::atomic<bool> quarantined_{false};
    std::atomic<std::uint64_t> latest_reserved_epoch_{0};
};
} // namespace sin_independent
