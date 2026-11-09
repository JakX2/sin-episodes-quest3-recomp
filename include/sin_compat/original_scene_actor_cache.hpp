#pragma once

// Portable reconstruction of the original CSceneEntity lazy actor-handle
// cache. The engine host owns native entity handles, VCD resources and lookup.

#include "sin_compat/original_scene_io.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace sin_compat::scene_io {

using SceneActorToken = std::uint64_t;

enum class SceneActorCacheResult : std::uint8_t {
    CacheHit,
    Resolved,
    MissingSceneEntity,
    MissingSceneResource,
    InvalidActorIndex,
    MissingChoreographyActor,
    ActorNotFound,
    HostRejected
};

struct SceneActorCacheLookup {
    SceneActorCacheResult result = SceneActorCacheResult::HostRejected;
    std::optional<SceneActorToken> actor;
};

// These callbacks are the portable equivalents of the original call
// boundaries. The host must use its own generation-safe entity handles and
// native scene/choreography objects. In particular, it must not reuse the
// original x86 offsets or the original scene virtual byte offset as an ARM64
// vtable slot.
class IOriginalSceneActorCacheHost {
public:
    virtual ~IOriginalSceneActorCacheHost() = default;

    virtual bool is_live_scene(SceneEntityToken scene) const noexcept = 0;

    // nullopt means the scene entity has no loaded choreography resource.
    virtual std::optional<std::size_t> choreography_actor_count(
        SceneEntityToken scene) const noexcept = 0;

    virtual std::size_t cached_actor_count(
        SceneEntityToken scene) const noexcept = 0;

    // Called only when the original cached vector count is zero.
    virtual bool initialize_actor_cache(
        SceneEntityToken scene,
        std::size_t actor_count) noexcept = 0;

    // nullopt represents an empty/invalid handle slot.
    virtual std::optional<SceneActorToken> cached_actor_at(
        SceneEntityToken scene,
        std::size_t actor_index) const noexcept = 0;

    // Resolves the cached handle and applies the original entity-state
    // eligibility check. Its exact semantic name is intentionally not guessed.
    virtual bool cached_actor_is_usable(
        SceneActorToken actor) const noexcept = 0;

    // nullopt means the choreography resource has no actor at this index.
    // The returned view must remain valid until find_named_flex_actor returns.
    virtual std::optional<std::string_view> choreography_actor_name(
        SceneEntityToken scene,
        std::size_t actor_index) const noexcept = 0;

    // Corresponds to the original scene's name-based actor lookup virtual.
    // The host owns aliases, targetname lookup, wildcard handling and the
    // requirement that the result support choreography/flex animation.
    virtual std::optional<SceneActorToken> find_named_flex_actor(
        SceneEntityToken scene,
        std::string_view actor_name) noexcept = 0;

    virtual bool store_cached_actor(
        SceneEntityToken scene,
        std::size_t actor_index,
        SceneActorToken actor) noexcept = 0;
};

// Reconstructs the original lazy actor lookup order:
//   1. Initialize the cache only when its current count is zero.
//   2. Bounds-check against the cache count.
//   3. Reuse a cached actor only when the host says it is still usable.
//   4. Otherwise fetch the VCD actor name, resolve it through the scene host,
//      and cache a successful result.
// This is an engine-neutral behavior component, not a CSceneEntity ABI shim.
SceneActorCacheLookup resolve_original_scene_actor(
    SceneEntityToken scene,
    std::size_t actor_index,
    IOriginalSceneActorCacheHost& host) noexcept;

} // namespace sin_compat::scene_io
