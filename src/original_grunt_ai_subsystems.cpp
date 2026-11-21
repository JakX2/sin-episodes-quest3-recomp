#include "sin_compat/original_grunt_ai_subsystems.hpp"

namespace sin_compat::original_druglab_grunt::ai_init {
Result Initializer::initialize() noexcept {
    if(busy_) return {Code::Reentrant};
    struct BusyGuard {
        bool& busy;
        explicit BusyGuard(bool& b) noexcept:busy(b){busy=true;}
        ~BusyGuard() noexcept{busy=false;}
    } guard(busy_);
    Result result{};
    constexpr Stage stages[]{
        Stage::Senses, Stage::BlendedMotor, Stage::LocalNavigator,
        Stage::MoveProbe, Stage::Navigator, Stage::Pathfinder,
        Stage::TacticalServices
    };
    try {
        for(std::size_t i=0;i<kOriginalComponents.size();++i) {
            result.failed_stage=stages[i];
            if(!host_.construct_and_store(kOriginalComponents[i]))return result;
            ++result.created_count;
        }
        // Original 0x22031D50 has NO explicit cleanup/rollback on a
        // failed factory; previously stored native fields remain live.
        // This independent reference likewise does not claim rollback.
        result.failed_stage=Stage::OwnerRelay;
        if(!host_.wire_embedded_owner_relay())return result;
        result.failed_stage=Stage::MotorToLocalNavigator;
        if(!host_.wire_motor_to_local_navigator())return result;
        result.failed_stage=Stage::LocalNavigatorToNavigator;
        if(!host_.wire_local_navigator_to_navigator())return result;
        result.failed_stage=Stage::NavigatorContext;
        if(!host_.wire_navigator_shared_context())return result;
        result.failed_stage=Stage::PathfinderContext;
        if(!host_.wire_pathfinder_shared_context())return result;
        result.failed_stage=Stage::TacticalServicesContext;
        if(!host_.wire_tactical_services_shared_context())return result;
    } catch(...) {return result;}
    result.code=Code::Success;
    result.failed_stage=Stage::None;
    result.wiring_completed=true;
    return result;
}
} // namespace sin_compat::original_druglab_grunt::ai_init