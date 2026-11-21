#include "sin_compat/original_grunt_full_ai_postinit.hpp"

namespace sin_compat::original_druglab_grunt::full_ai_postinit {
Result Initializer::initialize() noexcept {
    if (busy_) return {Code::Reentrant};
    struct Guard {
        bool& busy;
        explicit Guard(bool& b) noexcept : busy(b) {busy=true;}
        ~Guard() noexcept {busy=false;}
    } guard(busy_);
    Result out{};
    out.failed_stage=Stage::CoreSubsystems;
    out.core=core_.initialize();
    if (!out.core.ok()) return out;
    // Original virtual +0x3F8 checks core construction before dispatching
    // behavior vslot +0x76C; only then it queries sentence key +0x7AC.
    out.failed_stage=Stage::EmbeddedBehaviorsAndSentences;
    out.behavior=behavior_.initialize();
    if (!out.behavior.ok()) return out;
    out.failed_stage=Stage::None;
    out.code=Code::Success;
    return out;
}
} // namespace sin_compat::original_druglab_grunt::full_ai_postinit