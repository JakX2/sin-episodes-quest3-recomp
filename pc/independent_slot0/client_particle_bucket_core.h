#pragma once
// Independent particle startup boundary, not an engine-facing object layout.
// Phase 87 original audit identifies allocation size, constructor argument
// and label; the allocator and constructor internals are not assumed here.
#include <cstddef>
#include <string_view>

namespace sin_independent {
inline constexpr std::size_t ParticleBucketObjectBytes=48;
inline constexpr std::size_t ParticleBucketConstructorArgument=96;
inline constexpr std::size_t OriginalParticleBucketMemberOffset=0x84;
inline constexpr std::string_view ParticleBucketLabel="CParticleMgr::m_pParticleBucket";

// All callbacks operate on caller-owned storage. Allocation and construction
// are deliberately separate; no original address or engine object is called.
// A successful construction returns the allocated object itself. Null means
// no object lifetime began; the allocation is still owned by the caller.
// Destruction and deallocation are separate operations.
struct ParticleBucketBackend {
    virtual ~ParticleBucketBackend()=default;
    virtual void ResetManager() noexcept=0;
    virtual void* Allocate(std::size_t bytes) noexcept=0;
    virtual bool Construct(void* storage,std::size_t argument,
                           std::string_view label) noexcept=0;
    virtual void Destroy(void* storage) noexcept=0;
    virtual void Deallocate(void* storage) noexcept=0;
};
enum class ParticleBucketState { Idle, Starting, Ready, Failed, Stopping };
enum class ParticleBucketResult { Started, AlreadyAttempted, AllocationFailed, ConstructionFailed };

class ParticleBucketOwner {
public:
    explicit ParticleBucketOwner(ParticleBucketBackend& backend) noexcept:backend_(backend){}
    ~ParticleBucketOwner(){ Shutdown(); }
    ParticleBucketOwner(const ParticleBucketOwner&)=delete;
    ParticleBucketOwner& operator=(const ParticleBucketOwner&)=delete;
    ParticleBucketState State() const noexcept{return state_;}
    void* Bucket() const noexcept{return bucket_;}
    ParticleBucketResult Start() noexcept {
        if(state_!=ParticleBucketState::Idle)return ParticleBucketResult::AlreadyAttempted;
        state_=ParticleBucketState::Starting;
        backend_.ResetManager();
        void* storage=backend_.Allocate(ParticleBucketObjectBytes);
        if(!storage){
            state_=ParticleBucketState::Failed;
            return ParticleBucketResult::AllocationFailed;
        }
        if(!backend_.Construct(storage,ParticleBucketConstructorArgument,ParticleBucketLabel)){
            backend_.Deallocate(storage);
            state_=ParticleBucketState::Failed;
            return ParticleBucketResult::ConstructionFailed;
        }
        bucket_=storage;
        state_=ParticleBucketState::Ready;
        return ParticleBucketResult::Started;
    }
    bool Shutdown() noexcept {
        if(state_==ParticleBucketState::Starting || state_==ParticleBucketState::Stopping)
            return false;
        if(state_==ParticleBucketState::Ready){
            state_=ParticleBucketState::Stopping;
            void* storage=bucket_;
            bucket_=nullptr;
            backend_.Destroy(storage);
            backend_.Deallocate(storage);
        } else if(state_!=ParticleBucketState::Failed) {
            return false;
        }
        state_=ParticleBucketState::Idle;
        return true;
    }
private:
    ParticleBucketBackend& backend_; // must outlive this owner
    void* bucket_=nullptr;
    ParticleBucketState state_=ParticleBucketState::Idle;
};
} // namespace sin_independent
