#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace sin_compat {

enum class SceneActorKind { JC, Jessica, Generic };

enum class SceneActorResult {
    Ok,
    InvalidState,
    InvalidArgument,
    HostFailed,
    UnsupportedInput,
    WrongInputType,
    NoPendingThrow
};

struct SceneActorRef {
    std::uint32_t index = 0;
    std::uint32_t generation = 0;
    bool valid() const noexcept { return index != 0 && generation != 0; }
};

struct SceneActorSpec {
    const char *original_classname;
    const char *candidate_hl2_base;
    const char *fallback_model;
    int spawn_health; // -1 means preserve host default
    bool supports_jessica_inputs;
};

// These are ORIGINAL entity defaults and explicitly UNVERIFIED fallback bases.
// A real Source host must validate bone/gesture compatibility for each model.
const SceneActorSpec& scene_actor_spec(SceneActorKind kind) noexcept;

// All operations must be implemented by the actual engine host. No opaque x86
// vtable pointer, model pointer, or SourceVR ABI is assumed here.
class ISceneActorHost {
public:
    virtual ~ISceneActorHost() = default;
    virtual SceneActorRef create_from_existing_base(const char *base_class,
                                                    const char *original_class) = 0;
    virtual bool set_model(SceneActorRef actor, const char *model) = 0;
    virtual bool set_health(SceneActorRef actor, int hp) = 0;
    virtual bool set_targetname(SceneActorRef actor, const char *targetname) = 0;
    virtual bool choreography_capable(SceneActorRef actor) = 0;
    // Bind the VCD's exact 'actor' token (may be '!Target1' or 'JC_Hospital')
    // to an engine actor and start playback through the real scene manager.
    virtual bool play_scene(SceneActorRef actor, const char *scene_file,
                            const char *scene_actor_token) = 0;
    virtual bool holster_player_weapon() = 0;
    // Host implements the original animation attachment lookup, compatible
    // weapon creation, optional ammo assignment, and ballistic motion.
    virtual bool throw_item_at_player(SceneActorRef actor, const char *classname,
                                      int primary_ammo, int secondary_ammo) = 0;
    virtual void remove_actor(SceneActorRef actor) = 0;
};

struct SceneActorSpawnOptions {
    std::string targetname;
    std::string map_model_override;
};

struct SceneActorInput {
    enum class Type { Void, String, Integer } type = Type::Void;
    std::string string_value;
    int integer_value = 0;
};

// Per-NPC state. The real host must bind map classname creation to spawn(),
// and forward Source datamap inputs / choreography events. No global mutable
// runtime and no claimed drop-in replacement for the 2006 game server.
class SceneActorBridge {
public:
    SceneActorBridge(ISceneActorHost &host, SceneActorKind kind);

    SceneActorResult spawn(const SceneActorSpawnOptions &options);
    SceneActorResult begin_scene(std::string_view vcd_path,
                                 std::string_view vcd_actor_token);
    SceneActorResult input(std::string_view name, const SceneActorInput &value);
    SceneActorResult animation_event(std::string_view registered_symbol);
    SceneActorRef actor() const noexcept { return actor_; }
    bool pending_throw() const noexcept { return pending_throw_; }

private:
    ISceneActorHost &host_;
    SceneActorKind kind_;
    SceneActorRef actor_{};
    std::string throw_classname_;
    int primary_ammo_ = -1;
    int secondary_ammo_ = -1;
    bool pending_throw_ = false;
};

} // namespace sin_compat