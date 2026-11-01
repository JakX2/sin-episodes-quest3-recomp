#pragma once
// Independent, ABI-free routing model of the six-category collector in the
// original 2006 SiN Episodes server.dll, VA 0x222B9E80.
// No proprietary game files or performance weighting are embedded.
#include <array>
#include <cstddef>
namespace sin_compat {
enum class FeedbackCategory : unsigned {
    Health=0, Damage=1, Accuracy=2, Headshot=3, Death=4, Time=5
};
constexpr std::size_t kFeedbackCategoryCount = 6;
constexpr int kFeedbackLevelCount = 5; // Very Easy...Very Hard, 0..4

struct FeedbackCategorySelection {
    FeedbackCategory category = FeedbackCategory::Health;
    int raw_level = 0;            // pointed-to category object field +0x20
    int effective_level = 0;      // original collector's EDI at dispatch
    bool bypasses_global = false; // categories 4 and 5 bypass comparisons
};
class IFeedbackCategoryRoutingSink {
public:
    virtual ~IFeedbackCategoryRoutingSink() = default;
    virtual void dispatch(const FeedbackCategorySelection& selection) = 0;
};
class DifficultyFeedbackRouting final {
public:
    // Host-provided values, not the original engine object/SaveRestore ABI.
    bool configure(int global_level,
                   const std::array<int,kFeedbackCategoryCount>& category_levels) noexcept;
    bool configured() const noexcept { return valid_; }
    int global_level() const noexcept { return global_; }
    // Returns false for out-of-range slot or unconfigured state.
    bool select(std::size_t index, FeedbackCategorySelection& out) const noexcept;
    bool dispatch_all(IFeedbackCategoryRoutingSink& sink) const;
private:
    int global_ = 0;
    std::array<int,kFeedbackCategoryCount> levels_{{0,0,0,0,0,0}};
    bool valid_ = false;
};
} // namespace sin_compat
