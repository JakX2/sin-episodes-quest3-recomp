#pragma once
// Phase 477: original-free CUserMessages *delivery-shape* adapter.
// The SHA-pinned original SiN client dispatcher at 0x241D0000 copies
// a six-dword (24-byte) bf_read for every registered listener.
// This owned adapter reproduces only the proven per-listener CURSOR
// isolation using an independent, bounded value type. It does NOT
// read the game's opaque bf_read*, call original function pointers,
// subscribe to the engine, or make VClient011::Init succeed.
#include "client_stats_owned_message_runtime.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_independent::stats_owned_dispatch {
using Cursor=stats_bitbuf::Cursor;
using StatsResult=stats_owned_messages::Result;
using Observer=void (*)(void*,int,Cursor&) noexcept;

inline constexpr std::size_t kTrackedMessages=3;
inline constexpr std::size_t kMaxObserversPerMessage=4;

inline constexpr int TrackedIndex(int messageId) noexcept {
    return messageId==27?0:messageId==28?1:messageId==5?2:-1;
}

enum class Status {
    DeliveredOwned, NotInitialized, UnboundMessage, InvalidCursor,
    HeaderRefused, ValueRefused
};
inline const char* StatusName(Status s) noexcept {
    switch(s){
        case Status::DeliveredOwned:return "independent-owned-message-delivered";
        case Status::NotInitialized:return "independent-dispatch-not-initialized";
        case Status::UnboundMessage:return "unbound-owned-message";
        case Status::InvalidCursor:return "invalid-owned-reader";
        case Status::HeaderRefused:return "refused-owned-message-header";
        case Status::ValueRefused:return "refused-owned-value";
    }
    return "unknown";
}
struct Report final {
    Status status=Status::NotInitialized;
    StatsResult stats=StatsResult::NotInitialized;
    std::size_t observer_calls=0;
    bool delivered() const noexcept{return status==Status::DeliveredOwned;}
};

// One owned Stats callback (27, 28, 5) and up to four independently
// authored observers per message. Original callback chaining beyond the
// proven reader-copy property is NOT asserted: observers are invoked
// only after this independent stat update was accepted.
class Dispatcher final {
public:
    bool Initialize() noexcept {
        ready_=false;
        observers_={};
        if(!stats_.Initialize())return false;
        ready_=stats_.initialized() && stats_.hooks().Complete();
        return ready_;
    }
    void Disable() noexcept {
        // Reject all deliveries; a later Initialize recreates owned
        // stat values and discards every observer registration.
        ready_=false;
        observers_={};
    }
    bool ready() const noexcept {return ready_;}
    const stats_owned_messages::OwnedStatsRuntime& stats() const noexcept {
        return stats_;
    }
    bool AddObserver(int messageId,Observer observer,void* context) noexcept {
        const int index=TrackedIndex(messageId);
        if(!ready_ || index<0 || !observer)return false;
        auto& bucket=observers_[static_cast<std::size_t>(index)];
        if(bucket.count==bucket.entries.size())return false;
        bucket.entries[bucket.count++]={observer,context};
        return true;
    }
    std::size_t ObserverCount(int messageId) const noexcept {
        const int index=TrackedIndex(messageId);
        return index<0?0:observers_[static_cast<std::size_t>(index)].count;
    }
    Report Dispatch(int messageId,const Cursor& supplied) noexcept {
        Report report{};
        if(!ready_)return report;
        const int index=TrackedIndex(messageId);
        if(index<0){
            report.status=Status::UnboundMessage;
            return report;
        }
        if(!supplied.valid()){
            report.status=Status::InvalidCursor;
            return report;
        }
        // Always clone the input first. A bad frame must not advance
        // the caller's reader, publish a partial stat or notify observers.
        Cursor statsCursor=supplied;
        report.stats=stats_.Handle(messageId,statsCursor);
        switch(report.stats){
            case StatsResult::StaticUpdated:
            case StatsResult::StaticUnchanged:
            case StatsResult::DynamicCreated:
            case StatsResult::DynamicUpdated:
            case StatsResult::DynamicUnchanged:
            case StatsResult::ResetApplied:break;
            case StatsResult::ValueRefused:
                report.status=Status::ValueRefused;
                return report;
            default:
                report.status=Status::HeaderRefused;
                return report;
        }
        // Snapshot the observer entries. A callback may register another
        // observer, but it must not affect the current dispatch.
        const Bucket snapshot=observers_[static_cast<std::size_t>(index)];
        for(std::size_t i=0;i<snapshot.count;++i){
            Cursor listenerCursor=supplied;
            snapshot.entries[i].callback(
                snapshot.entries[i].context,messageId,listenerCursor);
            ++report.observer_calls;
        }
        report.status=Status::DeliveredOwned;
        return report;
    }
private:
    struct Entry final {Observer callback=nullptr;void* context=nullptr;};
    struct Bucket final {
        std::array<Entry,kMaxObserversPerMessage> entries{};
        std::size_t count=0;
    };
    stats_owned_messages::OwnedStatsRuntime stats_{};
    std::array<Bucket,kTrackedMessages> observers_{};
    bool ready_=false;
};
} // namespace sin_independent::stats_owned_dispatch
