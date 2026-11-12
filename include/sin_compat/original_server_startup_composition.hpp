#pragma once
// Phase189: compose the separately reviewed Phase187 DLLInit gates with
// Phase188 scene/model-sounds cache loader decisions. This is a synthetic
// host adapter, NOT a decompilation of another original x86 function and NOT
// an implementation of the Source IFileSystem ABI or loadable game DLL.
#include <cstdint>

#include "sin_compat/original_server_post_emitter.hpp"
#include "sin_compat/original_server_cache_repositories.hpp"

namespace sin_compat::original_server_startup_composition {
namespace gates = original_server_post_emitter;
namespace caches = original_server_cache_repositories;

// Audit drift between two independently reconstructed original-address maps
// at compile time rather than quietly dispatching to a wrong singleton.
static_assert(gates::kRepositoryAObjectVA == caches::kScene.singleton_va);
static_assert(gates::kRepositoryAImplVA == caches::kScene.init_va);
static_assert(gates::kRepositoryBObjectVA == caches::kModelSounds.singleton_va);
static_assert(gates::kRepositoryBImplVA == caches::kModelSounds.init_va);

// The Host supplies the callbacks documented by BOTH input headers. This
// bridge exists only to inject Phase188's repository decisions into the
// Phase187 short-circuiting gate order, with distinct repository state.
// Neither native file record decoding nor engine vtables are inferred here.
template<class Host>
class StartupGateAdapter {
public:
    StartupGateAdapter(Host& host, caches::State& scene,
                       caches::State& model_sounds)
        : host_(host), scene_(scene), model_sounds_(model_sounds) {}

    void duplicate_system(gates::SystemNode& node) { host_.duplicate_system(node); }
    bool init_system(gates::SystemNode& node) { return host_.init_system(node); }

    bool init_repository_a(std::uint32_t, std::uint32_t) {
        return caches::initialize(caches::kScene, scene_, host_);
    }
    bool init_repository_b(std::uint32_t, std::uint32_t) {
        return caches::initialize(caches::kModelSounds, model_sounds_, host_);
    }

private:
    Host& host_;
    caches::State& scene_;
    caches::State& model_sounds_;
};

// Covers ONLY these three original mandatory gates in DLLInit. The remaining
// server DLL startup path, engine callbacks and Windows module loading are
// outside the composition, even if this function returns true.
template<class Host>
bool run_required_gates(gates::Registry& systems,
                        caches::State& scene,
                        caches::State& model_sounds,
                        Host& host) {
    StartupGateAdapter<Host> adapter(host, scene, model_sounds);
    return gates::run_three_required_gates(systems, adapter);
}
} // namespace sin_compat::original_server_startup_composition
