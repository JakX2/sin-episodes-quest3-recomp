#pragma once
// Phase 1647: truly owner-allocated Windows PE32 pages with an exclusive
// physical lifetime pin, used ONLY for an original-free native fixture.
// NOT a pin on retail engine.dll memory, and never registered at factory.
#if !defined(_WIN32) || !defined(_M_IX86)
#error Phase 1647 Win32 owner pin pages require native Windows x86
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <mutex>
#include <thread>

namespace sin_independent {

// This test-only host owns all of the backing with VirtualAlloc/VirtualFree.
// Its recursive mutex is intentionally HELD, not merely sampled, across
// Begin -> independent game callbacks -> End. Host-controlled reallocation,
// release and writes therefore cannot run during a pin. Cross-thread owners
// fail immediately via try_lock. Same-thread callbacks can validate the
// pin or attempt mutation without deadlocking, and are explicitly rejected.
// This protects ONLY its own pages and cooperative host operations; it
// cannot pin or authenticate externally owned game/engine allocations.
class Win32OwnedGameEntsPinPages final {
public:
    static constexpr std::size_t kEdictStride=0x14;
    static constexpr std::size_t kEdictCapacity=6;
    static constexpr std::size_t kArrayBytes=kEdictStride*kEdictCapacity;
    static constexpr std::size_t kEntityBytes=0x180;
    static constexpr std::size_t kTouchBytes=0x90;

    struct Snapshot {
        unsigned char* array=nullptr;
        unsigned char* entity=nullptr;
        unsigned char* touch=nullptr;
        std::size_t array_bytes=0;
        std::size_t entity_bytes=0;
        std::size_t touch_bytes=0;
        std::uint64_t token=0;
    };

    Win32OwnedGameEntsPinPages() noexcept=default;
    Win32OwnedGameEntsPinPages(const Win32OwnedGameEntsPinPages&)=delete;
    Win32OwnedGameEntsPinPages& operator=(const Win32OwnedGameEntsPinPages&)=delete;
    ~Win32OwnedGameEntsPinPages() noexcept {
        // A host must outlive ALL outstanding pins, even on failure paths.
        if(!TryRelease())std::terminate();
    }

    bool Allocate() noexcept {
        std::lock_guard<std::recursive_mutex> guard(gate_);
        if(held_||array_||entity_||touch_)return false;
        array_=AllocPage(kArrayBytes);
        entity_=AllocPage(kEntityBytes);
        touch_=AllocPage(kTouchBytes);
        if(!array_||!entity_||!touch_) {
            ReleaseUnlocked();
            return false;
        }
        // Original edict initial serial is -1. All pages are independent,
        // zero-initialized, committed, non-moving Win32 virtual allocations.
        for(std::size_t i=0;i<kEdictCapacity;++i) {
            unsigned char* rec=array_+i*kEdictStride;
            rec[4]=rec[5]=rec[6]=rec[7]=0xff;
        }
        return true;
    }

    bool Begin(Snapshot* out) noexcept {
        if(!out||!gate_.try_lock())return false;
        if(held_||!array_||!entity_||!touch_||
           nonce_==(std::numeric_limits<std::uint64_t>::max)()) {
            gate_.unlock();
            return false;
        }
        held_=true;
        owner_=std::this_thread::get_id();
        ++nonce_;
        *out={array_,entity_,touch_,kArrayBytes,kEntityBytes,
              kTouchBytes,nonce_};
        // One mutex acquisition remains owned until End, including while
        // engine selector calls back into FreeContainingEntity and Commit.
        return true;
    }

    bool Validate(const Snapshot* snap) noexcept {
        if(!snap||!gate_.try_lock())return false;
        std::unique_lock<std::recursive_mutex> guard(gate_,std::adopt_lock);
        return held_&&owner_==std::this_thread::get_id()&&
               snap->token==nonce_&&
               snap->array==array_&&snap->entity==entity_&&
               snap->touch==touch_&&
               snap->array_bytes==kArrayBytes&&
               snap->entity_bytes==kEntityBytes&&
               snap->touch_bytes==kTouchBytes;
    }

    bool End(std::uint64_t token) noexcept {
        // Acquire the recursive level only when this thread already owns
        // the pin. A foreign-thread/end-token call must NEVER unlock it.
        if(!gate_.try_lock())return false;
        std::unique_lock<std::recursive_mutex> guard(gate_,std::adopt_lock);
        if(!held_||owner_!=std::this_thread::get_id()||
           nonce_!=token)return false;
        held_=false;
        owner_=std::thread::id{};
        gate_.unlock(); // release the original Begin level
        return true; // guard releases its temporary recursive level
    }

    bool TryRelease() noexcept {
        if(!gate_.try_lock()) {
            refused_operations_.fetch_add(1,std::memory_order_relaxed);
            return false;
        }
        std::unique_lock<std::recursive_mutex> guard(gate_,std::adopt_lock);
        if(held_) {
            refused_operations_.fetch_add(1,std::memory_order_relaxed);
            return false;
        }
        ReleaseUnlocked();
        return true;
    }

    // Only host-owned writes outside a pin use this path. During a pin
    // callbacks may mutate previously authorized edict fields directly,
    // but no concurrent host write can change their backing or bytes.
    bool TryWrite(const void* base,std::size_t offset,
                  std::uint32_t value) noexcept {
        if(!gate_.try_lock()) {
            refused_operations_.fetch_add(1,std::memory_order_relaxed);
            return false;
        }
        std::unique_lock<std::recursive_mutex> guard(gate_,std::adopt_lock);
        if(held_) {
            refused_operations_.fetch_add(1,std::memory_order_relaxed);
            return false;
        }
        unsigned char* destination=nullptr;
        std::size_t extent=0;
        if(base==array_&&array_) {
            destination=array_;extent=kArrayBytes;
        } else if(base==entity_&&entity_) {
            destination=entity_;extent=kEntityBytes;
        } else if(base==touch_&&touch_) {
            destination=touch_;extent=kTouchBytes;
        } else return false;
        if(offset>extent||extent-offset<4)return false;
        destination[offset]=static_cast<unsigned char>(value);
        destination[offset+1]=static_cast<unsigned char>(value>>8);
        destination[offset+2]=static_cast<unsigned char>(value>>16);
        destination[offset+3]=static_cast<unsigned char>(value>>24);
        return true;
    }

    unsigned RefusedOperations() const noexcept {
        return refused_operations_.load(std::memory_order_relaxed);
    }
    static bool IsCommittedPrivatePage(const void* address) noexcept {
        MEMORY_BASIC_INFORMATION mbi{};
        const auto n=VirtualQuery(address,&mbi,sizeof(mbi));
        return n==sizeof(mbi)&&mbi.State==MEM_COMMIT&&
               mbi.Type==MEM_PRIVATE&&
               (mbi.Protect&PAGE_READWRITE)==PAGE_READWRITE;
    }

private:
    static unsigned char* AllocPage(std::size_t bytes) noexcept {
        return static_cast<unsigned char*>(
            VirtualAlloc(nullptr,bytes,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    }
    void ReleaseUnlocked() noexcept {
        if(array_)VirtualFree(array_,0,MEM_RELEASE);
        if(entity_)VirtualFree(entity_,0,MEM_RELEASE);
        if(touch_)VirtualFree(touch_,0,MEM_RELEASE);
        array_=entity_=touch_=nullptr;
    }

    std::recursive_mutex gate_{};
    std::atomic<unsigned> refused_operations_{0};
    unsigned char* array_=nullptr;
    unsigned char* entity_=nullptr;
    unsigned char* touch_=nullptr;
    std::thread::id owner_{};
    std::uint64_t nonce_=0;
    bool held_=false;
};
} // namespace sin_independent
