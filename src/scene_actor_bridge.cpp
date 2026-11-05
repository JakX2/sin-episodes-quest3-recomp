#include "sin_compat/scene_actor_bridge.hpp"

namespace sin_compat {
namespace {
constexpr SceneActorSpec kJC{"npc_jc", "npc_citizen",
    "models/characters/JC/jc_torso.mdl", 8, false};
constexpr SceneActorSpec kJessica{"npc_jessica", "npc_alyx",
    "models/characters/Jessica/Jessica.mdl", 80, true};
constexpr SceneActorSpec kGeneric{"npc_generic_actor", "npc_citizen",
    nullptr, -1, false};
} // namespace

const SceneActorSpec& scene_actor_spec(SceneActorKind kind) noexcept {
    switch (kind) {
        case SceneActorKind::JC: return kJC;
        case SceneActorKind::Jessica: return kJessica;
        case SceneActorKind::Generic: return kGeneric;
    }
    return kGeneric;
}

SceneActorBridge::SceneActorBridge(ISceneActorHost &host, SceneActorKind kind)
    : host_(host), kind_(kind) {}

SceneActorResult SceneActorBridge::spawn(const SceneActorSpawnOptions &options) {
    if (actor_.valid()) return SceneActorResult::InvalidState;
    const auto& spec = scene_actor_spec(kind_);
    const char *model = options.map_model_override.empty()
        ? spec.fallback_model : options.map_model_override.c_str();
    if (!model || !*model) return SceneActorResult::InvalidArgument;
    auto candidate = host_.create_from_existing_base(spec.candidate_hl2_base,
                                                       spec.original_classname);
    if (!candidate.valid()) return SceneActorResult::HostFailed;
    if (!host_.set_model(candidate, model) ||
        (spec.spawn_health >= 0 && !host_.set_health(candidate, spec.spawn_health)) ||
        (!options.targetname.empty() &&
         !host_.set_targetname(candidate, options.targetname.c_str())) ||
        !host_.choreography_capable(candidate)) {
        host_.remove_actor(candidate);
        return SceneActorResult::HostFailed;
    }
    actor_ = candidate;
    return SceneActorResult::Ok;
}

SceneActorResult SceneActorBridge::begin_scene(std::string_view path,
                                               std::string_view actor_token) {
    if (!actor_.valid()) return SceneActorResult::InvalidState;
    if (path.empty() || actor_token.empty() || path.find('\0')!=std::string_view::npos ||
        actor_token.find('\0')!=std::string_view::npos)
        return SceneActorResult::InvalidArgument;
    // Do not coerce '!target1', 'JC_Hospital', or 'Roddick' to classnames.
    const std::string full_path(path), token(actor_token);
    return host_.play_scene(actor_, full_path.c_str(), token.c_str())
        ? SceneActorResult::Ok : SceneActorResult::HostFailed;
}

SceneActorResult SceneActorBridge::input(std::string_view name,
                                         const SceneActorInput &value) {
    if (!actor_.valid()) return SceneActorResult::InvalidState;
    if (kind_ != SceneActorKind::Jessica) return SceneActorResult::UnsupportedInput;
    if (name == "ThrowItemAtPlayer") {
        if (value.type != SceneActorInput::Type::String)
            return SceneActorResult::WrongInputType;
        if (value.string_value.empty()) return SceneActorResult::InvalidArgument;
        throw_classname_ = value.string_value;
        pending_throw_ = true;
        return SceneActorResult::Ok;
    }
    if (name == "SetPrimaryAmmoCountToThrow" ||
        name == "SetSecondaryAmmoCountToThrow") {
        if (value.type != SceneActorInput::Type::Integer)
            return SceneActorResult::WrongInputType;
        (name == "SetPrimaryAmmoCountToThrow" ? primary_ammo_ : secondary_ammo_)
            = value.integer_value;
        return SceneActorResult::Ok;
    }
    if (name == "HolsterWeapon") {
        if (value.type != SceneActorInput::Type::Void)
            return SceneActorResult::WrongInputType;
        return host_.holster_player_weapon() ? SceneActorResult::Ok
                                             : SceneActorResult::HostFailed;
    }
    return SceneActorResult::UnsupportedInput;
}

SceneActorResult SceneActorBridge::animation_event(std::string_view symbol) {
    if (!actor_.valid()) return SceneActorResult::InvalidState;
    if (kind_ != SceneActorKind::Jessica || symbol != "AE_JESSICA_THROW_ITEM")
        return SceneActorResult::UnsupportedInput;
    if (!pending_throw_) return SceneActorResult::NoPendingThrow;
    if (!host_.throw_item_at_player(actor_, throw_classname_.c_str(),
                                    primary_ammo_, secondary_ammo_))
        return SceneActorResult::HostFailed;
    // Do not double-spawn if the host repeats a single animation event.
    pending_throw_ = false;
    primary_ammo_ = -1;
    secondary_ammo_ = -1;
    throw_classname_.clear();
    return SceneActorResult::Ok;
}

} // namespace sin_compat