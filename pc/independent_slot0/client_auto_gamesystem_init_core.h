#pragma once
// Independently modeled original 2006 SiN 0x24156840/0x24156860/
// 0x24156980 AutoGameSystem startup. Host-only object ownership.
// Original runtime array is CUtlVector and original callbacks are raw
// Win32 vtable slot zero. Never feed unknown retail pointers here.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstddef>
#include <unordered_set>
#include <vector>
namespace sin_independent::auto_gamesystem {
enum class DirectSource {CallbackReturn, LazySingletonReturn, SavedGlobalValue, NullableGlobalPlusFour, FixedAddress};
struct DirectRegistration {std::uint32_t callsite_va;DirectSource source;std::uint32_t source_va;};
// Twelve original 2006 client Init callsites, not reconstructed objects.
inline constexpr std::array<DirectRegistration,12> kOriginalDirectRegistrations{{
    {0x240FE3FCu,DirectSource::CallbackReturn,0x2413D240u},
    {0x240FE407u,DirectSource::CallbackReturn,0x241BCAA0u},
    {0x240FE420u,DirectSource::NullableGlobalPlusFour,0x242FA79Cu},
    {0x240FE42Bu,DirectSource::CallbackReturn,0x24123CB0u},
    {0x240FE436u,DirectSource::CallbackReturn,0x241ED2F0u},
    {0x240FE441u,DirectSource::LazySingletonReturn,0x24105660u},
    {0x240FE44Du,DirectSource::SavedGlobalValue,0x242FA890u},
    {0x240FE457u,DirectSource::FixedAddress,0x24378DE8u},
    {0x240FE461u,DirectSource::FixedAddress,0x24378D18u},
    {0x240FE46Bu,DirectSource::FixedAddress,0x242FA6B8u},
    {0x240FE476u,DirectSource::CallbackReturn,0x240F23A0u},
    {0x240FE481u,DirectSource::CallbackReturn,0x24181FC0u}
}};
struct StaticGetter {std::uint32_t getter_va;std::uint32_t native_object_va;};
// Six original six-byte return-address getters, SHA checked privately.
inline constexpr std::array<StaticGetter,6> kOriginalDirectStaticGetters{{
    {0x2413D240u,0x2436212Cu},
    {0x241BCAA0u,0x242FFC1Cu},
    {0x24123CB0u,0x243589A8u},
    {0x241ED2F0u,0x2431DD84u},
    {0x240F23A0u,0x2433E4F8u},
    {0x24181FC0u,0x242FC9DCu}
}};
struct System {
    // Original 0x24156950 dispatches the first virtual method, returning bool.
    virtual bool Init() noexcept=0;
    System* next=nullptr;   // native pending constructor node at +4 on x86
};
enum class Outcome { Initialized, InitRefused, InvalidEntry, CyclicPending };
struct Result {
    Outcome outcome=Outcome::Initialized;
    std::size_t initialized_count=0;
    std::size_t duplicate_skipped=0;
    bool succeeded() const noexcept {return outcome==Outcome::Initialized;}
};
class Registry {
public:
    // Native 0x24156980 queues an uninitialized system at the head.
    // Once the initialization bit is set it registers directly.
    void Construct(System* entry) {
        if(initialization_started_) {
            registered_.push_back(entry);
        } else if(entry) {
            entry->next=pending_;
            pending_=entry;
        }
    }
    // Native 0x24156840 calls a CUtlVector insertion helper directly
    // without checking for duplicates. Preserve its direct registration.
    void DirectRegister(System* entry) {registered_.push_back(entry);}
    Result Initialize() {
        Result result{};
        // Original 0x24156860 walks the pending linked list, appending
        // only systems missing from the direct vector and warning on
        // duplicates. Native list order is LIFO (constructor head).
        std::unordered_set<System*> visited;
        for(System* item=pending_;item;item=item->next) {
            if(!visited.insert(item).second) {
                result.outcome=Outcome::CyclicPending;
                return result; // safety difference; native has no guard
            }
            if(std::find(registered_.begin(),registered_.end(),item)!=
               registered_.end()) {
                ++result.duplicate_skipped;
            } else {
                registered_.push_back(item);
            }
        }
        pending_=nullptr;
        // Native sets the byte at 0x24366C84 before calling Init,
        // enabling later registrations to append directly.
        initialization_started_=true;
        // Original compares i to live vector count every iteration.
        // A system registered by an Init callback can run in this pass.
        for(std::size_t i=0;i<registered_.size();++i) {
            System* current=registered_[i];
            if(!current) {result.outcome=Outcome::InvalidEntry;return result;}
            if(!current->Init()) {
                result.outcome=Outcome::InitRefused;
                return result;
            }
            ++result.initialized_count;
        }
        return result;
    }
    std::size_t registered_size() const noexcept{return registered_.size();}
    bool initialization_started() const noexcept{return initialization_started_;}
    bool has_pending() const noexcept{return pending_!=nullptr;}
    const std::vector<System*>& registered() const noexcept{return registered_;}
private:
    std::vector<System*> registered_;
    System* pending_=nullptr;
    bool initialization_started_=false;
};
} // namespace sin_independent::auto_gamesystem
