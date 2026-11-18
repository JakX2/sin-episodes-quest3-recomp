#include "client_particle_pool_core.h"
#include <array>
#include <cstdlib>
using namespace sin_independent;
#define CHECK(x) do { if(!(x)) std::abort(); } while(false)

int main(){
    // 2048 is a test configuration only; not certified as retail capacity.
    constexpr std::size_t Capacity=2048;
    IndependentParticleBucketBackend backend(Capacity);
    ParticleBucketOwner owner(backend);
    auto& pool=backend.Pool();
    CHECK(!pool.Ready() && !pool.Acquire() && !pool.Release({}));
    CHECK(!pool.Initialize(0) && !pool.Initialize(OwnedParticlePool::MaxRecords+1));
    CHECK(owner.Start()==ParticleBucketResult::Started);
    CHECK(pool.Ready() && pool.Capacity()==Capacity && pool.Used()==0);
    CHECK(!pool.Initialize(1));
    std::array<OwnedParticlePool::Handle,Capacity> h{};
    auto* first=static_cast<ParticleStorageRecord*>(nullptr);
    for(std::size_t i=0;i<Capacity;++i){
        h[i]=pool.Acquire();CHECK(h[i] && h[i].index==i);
        auto* p=pool.Resolve(h[i]);CHECK(p && p->bytes[95]==std::byte{0});
        if(i==0)first=p;
        CHECK(p==first+i);
        CHECK(reinterpret_cast<std::uintptr_t>(p)%16==0);
        p->bytes[0]=std::byte{0x6b};p->bytes[95]=std::byte{0xcd};
    }
    CHECK(pool.Used()==Capacity && !pool.Acquire());
    OwnedParticlePool other;CHECK(other.Initialize(1));
    auto foreign=other.Acquire();
    CHECK(!pool.Resolve(foreign) && !pool.Release(foreign));
    CHECK(!other.Resolve(h[0]));
    auto forged=h[0];forged.index=Capacity;
    CHECK(!pool.Resolve(forged) && !pool.Release(forged));
    CHECK(pool.Release(h[17]) && !pool.Release(h[17]));
    CHECK(!pool.Resolve(h[17]));
    auto renewed=pool.Acquire();CHECK(renewed && renewed.index==17);
    CHECK(renewed.generation!=h[17].generation);
    CHECK(pool.Resolve(renewed)->bytes[0]==std::byte{0});
    CHECK(pool.Resolve(renewed)->bytes[95]==std::byte{0});
    CHECK(!pool.Release(h[17]) && pool.Used()==Capacity);
    CHECK(pool.Release(renewed));
    for(std::size_t i=0;i<Capacity;++i)if(i!=17)CHECK(pool.Release(h[i]));
    CHECK(pool.Used()==0);
    auto live=pool.Acquire();CHECK(live);
    CHECK(owner.Shutdown());
    CHECK(!pool.Ready() && pool.Used()==0 && !pool.Resolve(live));
    CHECK(owner.Start()==ParticleBucketResult::Started);
    auto afterRestart=pool.Acquire();
    CHECK(afterRestart && afterRestart.generation!=live.generation);
    CHECK(!pool.Resolve(live) && !pool.Release(live));
    CHECK(owner.Shutdown());
    IndependentParticleBucketBackend invalid(0);
    ParticleBucketOwner failed(invalid);
    CHECK(failed.Start()==ParticleBucketResult::ConstructionFailed);
    CHECK(!failed.Bucket() && !invalid.Pool().Ready());
    CHECK(failed.Shutdown());
    CHECK(failed.Start()==ParticleBucketResult::ConstructionFailed);
    CHECK(failed.Shutdown());
}
