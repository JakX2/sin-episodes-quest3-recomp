#include "sin_compat/fixed_difficulty_trigger_phase663.hpp"

namespace sin_compat::difficulty::fixed_phase663 {
bool FixedDifficultyTriggerHost::is_live(map_entities::EntityRef entity) const {
    return real_.is_live(entity);
}
bool FixedDifficultyTriggerHost::initialize_native_base_trigger(
        map_entities::EntityRef entity) {
    return real_.initialize_native_base_trigger(entity);
}
void FixedDifficultyTriggerHost::report_original_warning(
        std::string_view text) {
    real_.report_original_warning(text);
}
bool FixedDifficultyTriggerHost::is_player(map_entities::EntityRef entity) const {
    return real_.is_player(entity);
}
int FixedDifficultyTriggerHost::current_difficulty_owner_index() const {
    return real_.current_difficulty_owner_index();
}
bool FixedDifficultyTriggerHost::perform_special_trigger_base_action(
        map_entities::EntityRef trigger) {
    return real_.perform_special_trigger_base_action(trigger);
}
bool FixedDifficultyTriggerHost::publish(const map_entities::TriggerEvent& event) {
    if(event.event==map_entities::TriggerEventName::DynamicDifficulty) {
        // Preserve the original gameplay event and entity-removal chain.
        // The original trigger calls mark_difficulty_manager() *later*
        // only when EndOfGame has a nonzero low byte. Count the observed
        // dynamic event exactly once, regardless of event delivery success.
        fixed_.observe_dynamic_trigger({event.decision,event.value,0});
    }
    return real_.publish(event);
}
bool FixedDifficultyTriggerHost::mark_difficulty_manager() {
    // The adaptive manager itself is not invoked in this opt-in policy.
    // Redirect the original completion flag to the fixed-profile state.
    // A host that needs this flag elsewhere must consult fixed_.
    fixed_.mark_playthrough_complete();
    return true;
}
bool FixedDifficultyTriggerHost::queue_remove(map_entities::EntityRef entity) {
    return real_.queue_remove(entity);
}
} // namespace sin_compat::difficulty::fixed_phase663
