#include "sin_compat/original_assault_schedule.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <set>
#include <type_traits>

int main() {
    using namespace sin_compat;

    static_assert(!std::is_convertible_v<LegacyAssaultTaskId, LegacyAssaultScheduleId>,
                  "Original Source task and schedule ID registries must not be mixed");
    static_assert(!std::is_convertible_v<std::uint32_t, LegacyAssaultScheduleId>,
                  "No implicit raw x86 ID -> semantic schedule conversion");
    static_assert(original_assault_schedules.size() == 9);
    static_assert(decode_original_assault_schedule(LegacyAssaultScheduleId{0x186A0}).value() ==
                  AssaultSchedule::MoveToRallyPoint);
    static_assert(decode_original_assault_schedule(LegacyAssaultScheduleId{0x186A3}).value() ==
                  AssaultSchedule::MoveToAssaultPoint);
    static_assert(decode_original_assault_schedule(LegacyAssaultScheduleId{0x186A4}).value() ==
                  AssaultSchedule::HoldRallyPoint);

    std::set<std::uint32_t> distinct;
    for (const auto& row : original_assault_schedules) {
        assert(distinct.insert(row.original_x86_id.value).second);
        assert(decode_original_assault_schedule(row.original_x86_id).value() ==
               row.semantic);
        const auto* name = original_assault_schedule_name(row.semantic);
        assert(name != nullptr);
        assert(std::strcmp(name, row.original_symbol_name) == 0);
        assert(std::strncmp(name, "SCHED_", 6) == 0);
    }
    assert(!decode_original_assault_schedule(LegacyAssaultScheduleId{0x186AF}));
    assert(!decode_original_assault_schedule(LegacyAssaultScheduleId{0}));
    return 0;
}
