#pragma once
#include "client_particle_bucket_core.h"
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <utility>

namespace sin_independent {
// Independent storage policy, NOT the original CParticleMgr/CMemoryPool ABI.
// The record stride comes from the known bucket constructor argument. No
// original field offsets or original pool capacity are assumed here.
struct alignas(16) ParticleStorageRecord {
    std::array<std::byte,ParticleBucketConstructorArgument> bytes{};
};
static_assert(sizeof(ParticleStorageRecord)==96);

class OwnedParticlePool {
public:
    struct Handle {
        const OwnedParticlePool* owner=nullptr;
        std::size_t index=0;
        std::uint64_t generation=0;
        explicit operator bool() const noexcept{return owner!=nullptr && generation!=0;}
    };
    static constexpr std::size_t MaxRecords=65536; // independent bounded policy
    OwnedParticlePool()=default;
    OwnedParticlePool(const OwnedParticlePool&)=delete;
    OwnedParticlePool& operator=(const OwnedParticlePool&)=delete;
    bool Initialize(std::size_t capacity) noexcept {
        if(records_ || !capacity || capacity>MaxRecords)return false;
        std::unique_ptr<ParticleStorageRecord[]> records(
            new(std::nothrow) ParticleStorageRecord[capacity]);
        if(!records)return false;
        std::unique_ptr<Slot[]> slots(new(std::nothrow) Slot[capacity]);
        if(!slots)return false;
        for(std::size_t i=0;i<capacity;++i)slots[i].next=i+1;
        records_=std::move(records);slots_=std::move(slots);
        capacity_=capacity;free_=0;used_=0;
        return true;
    }
    void Shutdown() noexcept {
        records_.reset();slots_.reset();capacity_=used_=free_=0;
        // Never reset the generation counter: an old handle must not
        // become valid if the same pool object is initialized again.
    }
    bool Ready() const noexcept{return records_!=nullptr;}
    std::size_t Capacity() const noexcept{return capacity_;}
    std::size_t Used() const noexcept{return used_;}
    Handle Acquire() noexcept {
        if(!Ready() || free_==capacity_ ||
           serial_==std::numeric_limits<std::uint64_t>::max())return {};
        auto index=free_;auto& slot=slots_[index];
        free_=slot.next;slot.generation=++serial_;++used_;
        records_[index].bytes.fill(std::byte{0});
        return {this,index,slot.generation};
    }
    ParticleStorageRecord* Resolve(Handle h) noexcept {
        return Valid(h)?&records_[h.index]:nullptr;
    }
    const ParticleStorageRecord* Resolve(Handle h) const noexcept {
        return Valid(h)?&records_[h.index]:nullptr;
    }
    bool Release(Handle h) noexcept {
        if(!Valid(h))return false;
        auto& slot=slots_[h.index];slot.generation=0;
        slot.next=free_;free_=h.index;--used_;
        return true;
    }
    // Handles are valid only while their originating pool object lives;
    // raw resolved pointers must not be used after Release or Shutdown.
private:
    struct Slot {std::size_t next=0;std::uint64_t generation=0;};
    bool Valid(Handle h) const noexcept {
        return Ready() && h.owner==this && h.index<capacity_ &&
            h.generation!=0 && slots_[h.index].generation==h.generation;
    }
    std::unique_ptr<ParticleStorageRecord[]> records_;
    std::unique_ptr<Slot[]> slots_;
    std::size_t capacity_=0,used_=0,free_=0;
    std::uint64_t serial_=0;
};

// Concrete backing storage for the independently implemented bucket
// lifecycle. The 48-byte allocation is opaque, not a fabricated original
// object layout. The actual records and bookkeeping are owned separately.
// The caller must keep the backend alive until ParticleBucketOwner ends.
class IndependentParticleBucketBackend final : public ParticleBucketBackend {
public:
    explicit IndependentParticleBucketBackend(std::size_t capacity) noexcept:capacity_(capacity){}
    ~IndependentParticleBucketBackend() override {
        // Fallback for direct use; ordinary owner calls Destroy/Deallocate.
        pool_.Shutdown();::operator delete(storage_);
    }
    IndependentParticleBucketBackend(const IndependentParticleBucketBackend&)=delete;
    IndependentParticleBucketBackend& operator=(const IndependentParticleBucketBackend&)=delete;
    OwnedParticlePool& Pool() noexcept{return pool_;}
    void ResetManager() noexcept override {
        // An active bucket must be torn down by its owner, not reset
        // behind its back. No original manager fields are modified.
    }
    void* Allocate(std::size_t bytes) noexcept override {
        if(storage_ || bytes!=ParticleBucketObjectBytes)return nullptr;
        storage_=::operator new(bytes,std::nothrow);
        return storage_;
    }
    bool Construct(void* p,std::size_t argument,std::string_view label) noexcept override {
        if(!p || p!=storage_ || pool_.Ready() ||
           argument!=ParticleBucketConstructorArgument || label!=ParticleBucketLabel)return false;
        return pool_.Initialize(capacity_);
    }
    void Destroy(void* p) noexcept override {
        if(p && p==storage_)pool_.Shutdown();
    }
    void Deallocate(void* p) noexcept override {
        if(!p || p!=storage_ || pool_.Ready())return;
        ::operator delete(storage_);storage_=nullptr;
    }
private:
    std::size_t capacity_;
    void* storage_=nullptr;
    OwnedParticlePool pool_;
};
} // namespace sin_independent
