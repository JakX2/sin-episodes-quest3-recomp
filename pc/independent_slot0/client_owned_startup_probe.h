#pragma once
#include "client_object_initializer_core.h"
#include "client_particle_pool_core.h"

namespace sin_independent {
// Owned native lifecycle diagnostic. This does NOT populate the original
// deferred registry or claim the original particle allocator layout.
using OwnedStartupTrace=void (*)(const char*,const char*) noexcept;
inline void* ownedParticleBackend=nullptr;
inline void* __cdecl CreateOwnedParticleBackend() {
    return new(std::nothrow) IndependentParticleBucketBackend(2048);
}
inline void __cdecl DeleteOwnedParticleBackend(void* p) {
    delete static_cast<IndependentParticleBucketBackend*>(p);
}
inline bool RunOwnedClientStartup(OwnedStartupTrace trace) noexcept {
    if(!trace || ownedParticleBackend)return false;
    ClientInitializers<1> initializers;
    ClientInitializer record{&ownedParticleBackend,CreateOwnedParticleBackend,
        DeleteOwnedParticleBackend,nullptr};
    trace("owned-startup-entered","independent-only");
    if(!initializers.Add(record) ||
       initializers.Initialize()!=InitializerResult::Initialized)return false;
    trace("owned-startup-objects-ready","1");
    auto& backend=*static_cast<IndependentParticleBucketBackend*>(ownedParticleBackend);
    bool verified=false;
    {
        ParticleBucketOwner bucket(backend);
        if(bucket.Start()==ParticleBucketResult::Started) {
            auto& pool=backend.Pool();
            if(pool.Ready() && pool.Capacity()==2048 && pool.Used()==0) {
                trace("owned-startup-pool-ready","2048x96");
                auto first=pool.Acquire();
                auto second=pool.Acquire();
                auto* p=pool.Resolve(first);
                auto* q=pool.Resolve(second);
                if(p && q && p!=q && pool.Used()==2) {
                    p->bytes[0]=std::byte{0x2a};
                    q->bytes[95]=std::byte{0x57};
                    if(pool.Release(first) && !pool.Resolve(first)) {
                        auto reused=pool.Acquire();
                        auto* fresh=pool.Resolve(reused);
                        if(fresh==p && reused.generation!=first.generation &&
                           fresh->bytes[0]==std::byte{0} &&
                           q->bytes[95]==std::byte{0x57} &&
                           pool.Release(second) && pool.Release(reused) &&
                           pool.Used()==0) {
                            verified=true;
                            trace("owned-startup-pool-verified","allocate-reuse-release");
                        }
                    }
                }
            }
        }
        // Release the bucket even on a verification failure.
        bucket.Shutdown();
        if(backend.Pool().Ready() || bucket.Bucket())verified=false;
    }
    const bool objectsFreed=initializers.Free() && !ownedParticleBackend;
    if(!objectsFreed)return false;
    if(verified)trace("owned-startup-cleanup-verified","pool-objects");
    return verified;
}
} // namespace sin_independent
