#pragma once
// Phase535: observed original CNPC_MobGrunt virtual +0x3F8 call sequence.
// Host-only orchestration: cannot substitute for original Source engine AI.
#include "sin_compat/original_grunt_ai_subsystems.hpp"
#include "sin_compat/original_grunt_behavior_postinit.hpp"

namespace sin_compat::original_druglab_grunt::full_ai_postinit {
enum class Stage : unsigned char { None, CoreSubsystems, EmbeddedBehaviorsAndSentences };
enum class Code : unsigned char { Success, HostRefused, Reentrant };
struct Result {
    Code code=Code::HostRefused;
    Stage failed_stage=Stage::None;
    ai_init::Result core{};
    behavior_postinit::Result behavior{};
    bool ok() const noexcept {return code==Code::Success;}
};
class Initializer final {
public:
    Initializer(ai_init::Initializer& core,
                behavior_postinit::Registrar& behavior) noexcept
        : core_(core), behavior_(behavior) {}
    Result initialize() noexcept;
private:
    ai_init::Initializer& core_;
    behavior_postinit::Registrar& behavior_;
    bool busy_=false;
};
} // namespace sin_compat::original_druglab_grunt::full_ai_postinit