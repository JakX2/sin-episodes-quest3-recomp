#include "sin_compat/scenario_entity_world.hpp"
#include "sin_compat/hammer_output_binding.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <string>
#include <utility>

namespace sin_compat {
namespace {

std::string lower_ascii(std::string text) {
    for (char& c : text)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return text;
}

bool wildcard_match(const std::string& pattern, const std::string& text) {
    // No regex allocation or unbounded recursion on potentially hostile maps.
    std::size_t p = 0, t = 0, star = std::string::npos, restart = 0;
    while (t < text.size()) {
        if (p < pattern.size() && (pattern[p] == text[t] || pattern[p] == '?')) {
            ++p; ++t;
        } else if (p < pattern.size() && pattern[p] == '*') {
            star = p++;
            restart = t;
        } else if (star != std::string::npos) {
            p = star + 1;
            t = ++restart;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '*') ++p;
    return p == pattern.size();
}

bool classify(const std::string& name, ScenarioEntityKind& kind) {
    if (name == "logic_auto") kind = ScenarioEntityKind::LogicAuto;
    else if (name == "logic_relay") kind = ScenarioEntityKind::LogicRelay;
    else if (name == "trigger_once") kind = ScenarioEntityKind::TriggerOnce;
    else if (name == "func_door") kind = ScenarioEntityKind::FuncDoor;
    else return false;
    return true;
}

} // namespace

ScenarioEntityWorld::ScenarioEntityWorld(IScenarioDoorMotionHost& movement)
    : movement_(movement), io_(*this) {}

const ScenarioEntityWorld::Slot* ScenarioEntityWorld::lookup(EntityRef entity) const {
    if (entity.index >= slots_.size()) return nullptr;
    const Slot& slot = slots_[entity.index];
    return slot.alive && slot.generation == entity.generation ? &slot : nullptr;
}

ScenarioEntityWorld::Slot* ScenarioEntityWorld::lookup(EntityRef entity) {
    return const_cast<Slot*>(
        static_cast<const ScenarioEntityWorld&>(*this).lookup(entity));
}

bool ScenarioEntityWorld::is_live(EntityRef entity) const {
    return lookup(entity) != nullptr;
}

ScenarioSpawnResult ScenarioEntityWorld::spawn(const ScenarioEntitySpec& spec) {
    ScenarioEntityKind kind;
    if (!classify(lower_ascii(spec.classname), kind))
        return {{}, "unsupported entity class; no factory is registered"};

    std::string targetname = spec.targetname;
    bool disabled = false;
    std::vector<std::pair<std::string, EntityOutputConnection>> links;
    for (const auto& kv : spec.keyvalues) {
        const auto key = lower_ascii(kv.first);
        if (key == "classname") {
            if (lower_ascii(kv.second) != lower_ascii(spec.classname))
                return {{}, "conflicting classname keyvalue"};
        } else if (key == "targetname") {
            if (!targetname.empty() && lower_ascii(targetname) != lower_ascii(kv.second))
                return {{}, "conflicting targetname"};
            targetname = kv.second;
        } else if (key == "startdisabled") {
            if (kv.second == "1") disabled = true;
            else if (kv.second == "0") disabled = false;
            else return {{}, "StartDisabled must be 0 or 1"};
        } else if (key.size() > 2 && key.compare(0, 2, "on") == 0) {
            const auto parsed = parse_hammer_output_value(kv.second);
            if (!parsed) return {{}, "malformed output connection: " + kv.first};
            links.emplace_back(kv.first, parsed.connection);
        }
    }
    // No arbitrary outside entity types or pointer-controlled factory names.
    // Even in the standalone simulation, all links must validate before any
    // slot is committed, so a malformed map entry cannot partially spawn.
    if (slots_.size() >= std::numeric_limits<std::uint32_t>::max() &&
        free_slots_.empty()) {
        return {{}, "entity index limit exceeded"};
    }

    std::uint32_t index;
    if (!free_slots_.empty()) {
        index = free_slots_.back();
        free_slots_.pop_back();
    } else {
        index = static_cast<std::uint32_t>(slots_.size());
        slots_.emplace_back();
    }
    Slot& slot = slots_[index];
    slot.alive = true;
    slot.state = {};
    slot.state.kind = kind;
    slot.state.targetname = std::move(targetname);
    slot.state.disabled = disabled;
    const EntityRef handle{index, slot.generation};
    for (auto& pair : links) {
        if (!io_.connect(handle, pair.first, std::move(pair.second))) {
            io_.remove_source(handle);
            slot.alive = false;
            free_slots_.push_back(index);
            return {{}, "output connection rejected by scheduler"};
        }
    }
    return {handle, {}};
}

bool ScenarioEntityWorld::remove(EntityRef entity) {
    auto* slot = lookup(entity);
    if (!slot) return false;
    io_.remove_source(entity);
    slot->alive = false;
    slot->state = {};
    if (slot->generation != std::numeric_limits<std::uint32_t>::max()) {
        ++slot->generation;
        free_slots_.push_back(entity.index);
    }
    return true;
}

bool ScenarioEntityWorld::snapshot(EntityRef entity, ScenarioEntitySnapshot& out) const {
    const auto* slot = lookup(entity);
    if (!slot) return false;
    out = slot->state;
    return true;
}

bool ScenarioEntityWorld::emit(EntityRef source, const char* output,
                               EntityRef activator) {
    if (!is_live(source)) return false;
    io_.fire(source, output, activator);
    return true;
}

bool ScenarioEntityWorld::activate_map(EntityRef activator) {
    if (map_activated_) return false;
    map_activated_ = true;
    for (std::uint32_t index = 0; index < slots_.size(); ++index) {
        const auto& slot = slots_[index];
        if (slot.alive && slot.state.kind == ScenarioEntityKind::LogicAuto)
            emit(EntityRef{index, slot.generation}, "OnMapSpawn", activator);
    }
    return true;
}

bool ScenarioEntityWorld::touch_once(EntityRef trigger, EntityRef activator) {
    auto* slot = lookup(trigger);
    if (!slot || slot->state.kind != ScenarioEntityKind::TriggerOnce ||
        slot->state.disabled || !is_live(activator)) return false;
    // Keep source alive long enough for delayed outputs. A real host may remove
    // the collision trigger separately after it becomes inactive.
    slot->state.disabled = true;
    emit(trigger, "OnStartTouch", activator);
    return true;
}

bool ScenarioEntityWorld::command_door(EntityRef door, bool opening,
                                        EntityRef activator) {
    auto* slot = lookup(door);
    if (!slot || slot->state.kind != ScenarioEntityKind::FuncDoor) return false;
    if (slot->state.locked || slot->state.disabled) return false;
    const auto phase = slot->state.door_phase;
    if (opening && (phase == ScenarioDoorPhase::Open ||
                    phase == ScenarioDoorPhase::Opening)) return false;
    if (!opening && (phase == ScenarioDoorPhase::Closed ||
                     phase == ScenarioDoorPhase::Closing)) return false;
    slot->state.door_phase = opening ? ScenarioDoorPhase::Opening
                                     : ScenarioDoorPhase::Closing;
    emit(door, opening ? "OnOpen" : "OnClose", activator);
    // Host-owned movement can complete synchronously or on a later frame.
    movement_.request_door_motion(door, opening, activator);
    return true;
}

bool ScenarioEntityWorld::door_motion_completed(EntityRef door, bool opened,
                                                 EntityRef activator) {
    auto* slot = lookup(door);
    if (!slot || slot->state.kind != ScenarioEntityKind::FuncDoor) return false;
    if (slot->state.door_phase != (opened ? ScenarioDoorPhase::Opening
                                        : ScenarioDoorPhase::Closing)) return false;
    slot->state.door_phase = opened ? ScenarioDoorPhase::Open
                                    : ScenarioDoorPhase::Closed;
    emit(door, opened ? "OnFullyOpen" : "OnFullyClosed", activator);
    return true;
}

bool ScenarioEntityWorld::input(EntityRef target, const std::string& input_name,
                                const std::string&, EntityRef activator,
                                EntityRef) {
    auto* slot = lookup(target);
    if (!slot) return false;
    const auto input_lower = lower_ascii(input_name);
    switch (slot->state.kind) {
        case ScenarioEntityKind::LogicAuto:
            return false; // OnMapSpawn occurs only through map activation.
        case ScenarioEntityKind::LogicRelay:
            if (input_lower == "trigger") {
                if (slot->state.disabled) return false;
                emit(target, "OnTrigger", activator);
                return true;
            }
            if (input_lower == "cancelpending") {
                io_.cancel_pending(target);
                return true;
            }
            if (input_lower == "enable") { slot->state.disabled = false; return true; }
            if (input_lower == "disable") { slot->state.disabled = true; return true; }
            if (input_lower == "toggle") { slot->state.disabled = !slot->state.disabled; return true; }
            return false;
        case ScenarioEntityKind::TriggerOnce:
            if (input_lower == "enable") { slot->state.disabled = false; return true; }
            if (input_lower == "disable") { slot->state.disabled = true; return true; }
            return false;
        case ScenarioEntityKind::FuncDoor:
            if (input_lower == "lock") { slot->state.locked = true; return true; }
            if (input_lower == "unlock") { slot->state.locked = false; return true; }
            if (input_lower == "open") return command_door(target, true, activator);
            if (input_lower == "close") return command_door(target, false, activator);
            if (input_lower == "toggle" || input_lower == "use") {
                return command_door(target,
                    slot->state.door_phase == ScenarioDoorPhase::Closed ||
                    slot->state.door_phase == ScenarioDoorPhase::Closing, activator);
            }
            return false;
    }
    return false;
}

std::vector<EntityRef> ScenarioEntityWorld::resolve_targets(
        const std::string& target, EntityRef activator, EntityRef caller) {
    const auto name = lower_ascii(target);
    if (name == "!activator") return is_live(activator) ? std::vector<EntityRef>{activator}
                                                        : std::vector<EntityRef>{};
    if (name == "!caller" || name == "!self")
        return is_live(caller) ? std::vector<EntityRef>{caller} : std::vector<EntityRef>{};
    // !player, !picker and other special Source engine selectors are deliberately
    // not guessed here; they require native host identity information.
    if (!name.empty() && name[0] == '!') return {};
    std::vector<EntityRef> result;
    for (std::uint32_t i = 0; i < slots_.size(); ++i) {
        const auto& slot = slots_[i];
        if (slot.alive && !slot.state.targetname.empty() &&
            wildcard_match(name, lower_ascii(slot.state.targetname)))
            result.push_back(EntityRef{i, slot.generation});
    }
    return result;
}

void ScenarioEntityWorld::accept_input(EntityRef target,
                                       const std::string& input_name,
                                       const std::string& parameter,
                                       EntityRef activator, EntityRef caller) {
    input(target, input_name, parameter, activator, caller);
}

EntityIoDrain ScenarioEntityWorld::advance_to(double seconds,
                                              std::size_t event_budget) {
    return io_.advance_to(seconds, event_budget);
}

} // namespace sin_compat
