#pragma once
// Phase663: opt-in adapter for original SiN trigger_dynamic_difficulty
// map entities when adaptive scoring has been intentionally replaced
// with selectable fixed Easy/Normal/Hard gameplay metrics.
//
// Base engine entity lifetime, Source event emission and queued deletion
// still come from a REAL IOriginalTriggerHost. Only the original manager
// completion callback is redirected to FixedDifficultyRuntime.
#include "fixed_difficulty_phase663.hpp"
#include "original_sin_trigger_entities.hpp"

namespace sin_compat::difficulty::fixed_phase663 {
class FixedDifficultyTriggerHost final : public map_entities::IOriginalTriggerHost {
public:
    FixedDifficultyTriggerHost(map_entities::IOriginalTriggerHost& real_host,
                               FixedDifficultyRuntime& fixed) noexcept
        :real_(real_host),fixed_(fixed){}
    bool is_live(sin_compat::EntityRef entity) const override;
    bool initialize_native_base_trigger(sin_compat::EntityRef entity) override;
    void report_original_warning(std::string_view text) override;
    bool is_player(sin_compat::EntityRef entity) const override;
    int current_difficulty_owner_index() const override;
    bool perform_special_trigger_base_action(sin_compat::EntityRef trigger) override;
    bool publish(const map_entities::TriggerEvent& event) override;
    bool mark_difficulty_manager() override;
    bool queue_remove(sin_compat::EntityRef entity) override;
private:
    map_entities::IOriginalTriggerHost& real_;
    FixedDifficultyRuntime& fixed_;
};
} // namespace sin_compat::difficulty::fixed_phase663
