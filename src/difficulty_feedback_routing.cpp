#include "sin_compat/difficulty_feedback_routing.hpp"
namespace sin_compat {
bool DifficultyFeedbackRouting::configure(
    int global_level, const std::array<int,kFeedbackCategoryCount>& levels) noexcept {
    valid_=false;
    if (global_level<0 || global_level>=kFeedbackLevelCount) return false;
    for (const int level:levels) {
        if (level<0 || level>=kFeedbackLevelCount) return false;
    }
    global_=global_level;
    levels_=levels;
    valid_=true;
    return true;
}
bool DifficultyFeedbackRouting::select(
    std::size_t index, FeedbackCategorySelection& out) const noexcept {
    if (!valid_ || index>=kFeedbackCategoryCount) return false;
    const int local=levels_[index];
    int effective=local;
    // Branch-for-branch decision table from VA 0x222B9E90..0x222B9ECC:
    // 4 and 5 bypass this entire region, preserving local level.
    if (index<4) {
        if (global_<2) {
            if (local>=2) effective=2;
        } else if (global_>2) {
            if (local>2 && global_>=local) effective=2;
        }
        // global == 2: no override, regardless of category level.
    }
    out={static_cast<FeedbackCategory>(index),local,effective,index>=4};
    return true;
}
bool DifficultyFeedbackRouting::dispatch_all(IFeedbackCategoryRoutingSink& sink) const {
    if(!valid_) return false;
    for (std::size_t i=0;i<kFeedbackCategoryCount;++i) {
        FeedbackCategorySelection selection;
        if(!select(i,selection)) return false;
        sink.dispatch(selection);
    }
    return true;
}
} // namespace sin_compat
