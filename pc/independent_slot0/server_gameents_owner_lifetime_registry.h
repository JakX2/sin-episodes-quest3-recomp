#pragma once
// Independently authored, fixed-capacity owner lifetime registry for the
// unregistered 2006 PE32 ServerGameEnts001 diagnostic interface.
// Metadata-only: does not read or execute original game memory.
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <thread>

namespace sin_independent {
struct GameEntsOwnerIdentity {
    const void* edict=nullptr;
    std::size_t edict_bytes=0;
    const void* entity=nullptr;
    std::size_t entity_bytes=0;
    const void* unknown=nullptr;
    std::size_t unknown_bytes=0;
    const void* touch=nullptr;
    std::size_t touch_bytes=0;
    std::uint64_t generation=0;
};

// A trusted host enrolls genuine live allocations and strictly increasing
// owner generations. A successful logical lease does not pin memory by
// itself; the engine allocator must independently preserve physical bytes.
template<std::size_t Slots,std::size_t Candidates>
class GameEntsOwnerLifetimeRegistry final {
    static_assert(Slots>0 && Candidates>0,"nonempty registry required");
public:
    GameEntsOwnerLifetimeRegistry() noexcept=default;
    GameEntsOwnerLifetimeRegistry(const GameEntsOwnerLifetimeRegistry&)=delete;
    GameEntsOwnerLifetimeRegistry& operator=(const GameEntsOwnerLifetimeRegistry&)=delete;

    bool Enroll(std::size_t slot,const GameEntsOwnerIdentity& value) noexcept {
        std::lock_guard<std::mutex> guard(mu_);
        if(leased_||slot>=Slots||!WellFormed(value))return false;
        const auto& prior=slots_[slot];
        if(prior.edict_owned||prior.entity_live||
           value.generation<=latest_generation_)return false;
        for(std::size_t i=0;i<Slots;++i) {
            if(i==slot)continue;
            const auto& old=slots_[i];
            // Retiring the entity frees its entity/unknown/touch ranges,
            // but its edict prefix remains owner-owned until ForgetEdict.
            if(old.edict_owned&&
               OverlapAny(value,old.value.edict,old.value.edict_bytes))
                return false;
            if(old.entity_live&&
               (OverlapAny(value,old.value.entity,old.value.entity_bytes)||
                OverlapAny(value,old.value.unknown,old.value.unknown_bytes)||
                OverlapAny(value,old.value.touch,old.value.touch_bytes)))
                return false;
        }
        slots_[slot]={value,true,true,true};
        latest_generation_=value.generation;
        return true;
    }

    // Only the original FreeContainingEntity slot can retire during lease.
    // Keep its edict prefix owned through cleanup; ForgetEdict is separate.
    bool RetireEntity(std::size_t slot) noexcept {
        std::lock_guard<std::mutex> guard(mu_);
        if(slot>=Slots||!slots_[slot].entity_live)return false;
        if(leased_&&(lease_slot_!=3||lease_thread_!=std::this_thread::get_id()))
            return false;
        slots_[slot].entity_live=false;
        return true;
    }
    bool ForgetEdict(std::size_t slot) noexcept {
        std::lock_guard<std::mutex> guard(mu_);
        if(leased_||slot>=Slots||!slots_[slot].edict_owned||
           slots_[slot].entity_live)return false;
        slots_[slot].edict_owned=false;
        return true;
    }

    // Explicit owner registration of table + candidate index + generation.
    bool BindCandidate(std::uint32_t table,std::size_t index,
                       std::size_t slot) noexcept {
        std::lock_guard<std::mutex> guard(mu_);
        if(leased_||!table||slot>=Slots||!slots_[slot].entity_live)return false;
        const auto generation=slots_[slot].value.generation;
        for(auto& row:candidates_) {
            if(!row.used||row.table!=table||row.index!=index)continue;
            const auto& previous=slots_[row.slot];
            if(row.slot==slot&&row.generation==generation)return true;
            if(previous.entity_live&&
               row.generation==previous.value.generation)return false;
            row={table,index,slot,generation,true};
            return true;
        }
        for(auto& row:candidates_)if(!row.used) {
            row={table,index,slot,generation,true};
            return true;
        }
        return false;
    }

    bool Begin(unsigned original_slot,std::uint64_t* out_token) noexcept {
        if(!out_token)return false;
        *out_token=0;
        std::lock_guard<std::mutex> guard(mu_);
        if(leased_||original_slot<1||original_slot>6||
           nonce_==(std::numeric_limits<std::uint64_t>::max)())return false;
        const auto token=++nonce_;
        leased_=true;
        lease_slot_=original_slot;
        lease_thread_=std::this_thread::get_id();
        lease_token_=token;
        *out_token=token;
        return true;
    }
    // Wrong slot/token/thread must not unlock an unrelated owner lease.
    bool End(unsigned original_slot,std::uint64_t token) noexcept {
        std::lock_guard<std::mutex> guard(mu_);
        if(!leased_||original_slot!=lease_slot_||token!=lease_token_||
           lease_thread_!=std::this_thread::get_id())return false;
        leased_=false;
        lease_slot_=0;
        lease_token_=0;
        lease_thread_=std::thread::id{};
        return true;
    }

    bool ValidEdict(const void* ptr) const noexcept {
        std::lock_guard<std::mutex> guard(mu_);
        if(!OwnedLease()||!ptr)return false;
        for(const auto& item:slots_)
            if(item.edict_owned&&item.value.edict==ptr)return true;
        return false;
    }
    bool ValidEntity(const void* ptr) const noexcept {
        std::lock_guard<std::mutex> guard(mu_);
        if(!OwnedLease()||!ptr)return false;
        for(const auto& item:slots_)
            if(item.entity_live&&item.value.entity==ptr)return true;
        return false;
    }
    bool ValidUnknown(const void* ptr) const noexcept {
        std::lock_guard<std::mutex> guard(mu_);
        if(!OwnedLease()||!ptr)return false;
        for(const auto& item:slots_)
            if(item.entity_live&&item.value.unknown==ptr)return true;
        return false;
    }
    bool ValidTouch(const void* ptr,std::size_t required_bytes) const noexcept {
        std::lock_guard<std::mutex> guard(mu_);
        if(!OwnedLease()||!ptr||!required_bytes)return false;
        for(const auto& item:slots_)
            if(item.entity_live&&item.value.touch==ptr&&
               item.value.touch_bytes>=required_bytes)return true;
        return false;
    }
    bool ValidPair(const void* edict,const void* entity) const noexcept {
        std::lock_guard<std::mutex> guard(mu_);
        if(!OwnedLease()||!edict||!entity)return false;
        for(const auto& item:slots_)
            if(item.edict_owned&&item.entity_live&&
               item.value.edict==edict&&item.value.entity==entity)return true;
        return false;
    }
    bool ValidCandidate(std::uint32_t table,std::size_t index,
                        const void* entity) const noexcept {
        std::lock_guard<std::mutex> guard(mu_);
        if(!OwnedLease()||!table||!entity)return false;
        for(const auto& row:candidates_) {
            if(!row.used||row.table!=table||row.index!=index)continue;
            const auto& item=slots_[row.slot];
            return item.entity_live&&item.edict_owned&&
                row.generation==item.value.generation&&
                item.value.entity==entity;
        }
        return false;
    }
    bool HasLease() const noexcept {
        std::lock_guard<std::mutex> guard(mu_);
        return leased_;
    }
private:
    struct Item {
        GameEntsOwnerIdentity value{};
        bool seen=false,edict_owned=false,entity_live=false;
    };
    struct Candidate {
        std::uint32_t table=0;
        std::size_t index=0,slot=0;
        std::uint64_t generation=0;
        bool used=false;
    };
    // All intervals are half-open [base,base+bytes); adjacent original
    // 0x14-byte edict records are allowed. Never form an overflowed end.
    static bool RangeValid(const void* ptr,std::size_t bytes) noexcept {
        if(!ptr||!bytes||bytes>
           (std::numeric_limits<std::uintptr_t>::max)())return false;
        const auto start=reinterpret_cast<std::uintptr_t>(ptr);
        return start!=0&&start<=
            (std::numeric_limits<std::uintptr_t>::max)()-bytes;
    }
    static bool Overlap(const void* a,std::size_t asz,
                        const void* b,std::size_t bsz) noexcept {
        const auto aa=reinterpret_cast<std::uintptr_t>(a);
        const auto bb=reinterpret_cast<std::uintptr_t>(b);
        return aa<bb+bsz&&bb<aa+asz;
    }
    static bool OverlapAny(const GameEntsOwnerIdentity& v,
                           const void* base,std::size_t bytes) noexcept {
        return Overlap(v.edict,v.edict_bytes,base,bytes)||
               Overlap(v.entity,v.entity_bytes,base,bytes)||
               Overlap(v.unknown,v.unknown_bytes,base,bytes)||
               Overlap(v.touch,v.touch_bytes,base,bytes);
    }
    static bool WellFormed(const GameEntsOwnerIdentity& v) noexcept {
        if(!v.generation||v.edict_bytes<0x14||v.entity_bytes<0x170||
           v.unknown_bytes<sizeof(void*)||!v.touch_bytes||
           !RangeValid(v.edict,v.edict_bytes)||
           !RangeValid(v.entity,v.entity_bytes)||
           !RangeValid(v.unknown,v.unknown_bytes)||
           !RangeValid(v.touch,v.touch_bytes))return false;
        return !Overlap(v.edict,v.edict_bytes,v.entity,v.entity_bytes)&&
            !Overlap(v.edict,v.edict_bytes,v.unknown,v.unknown_bytes)&&
            !Overlap(v.edict,v.edict_bytes,v.touch,v.touch_bytes)&&
            !Overlap(v.entity,v.entity_bytes,v.unknown,v.unknown_bytes)&&
            !Overlap(v.entity,v.entity_bytes,v.touch,v.touch_bytes)&&
            !Overlap(v.unknown,v.unknown_bytes,v.touch,v.touch_bytes);
    }
    bool OwnedLease() const noexcept {
        return leased_&&lease_thread_==std::this_thread::get_id();
    }

    mutable std::mutex mu_{};
    std::array<Item,Slots> slots_{};
    std::array<Candidate,Candidates> candidates_{};
    std::uint64_t nonce_=0,lease_token_=0,latest_generation_=0;
    std::thread::id lease_thread_{};
    unsigned lease_slot_=0;
    bool leased_=false;
};
} // namespace sin_independent
