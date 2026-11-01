#include "sin_compat/difficulty_category_bands.hpp"
#include <algorithm>
#include <cmath>
namespace sin_compat {
namespace {
bool valid(float v) { return std::isfinite(v); }
float& selected(DifficultyBandPair& row, int selector) {
    return selector < 2 ? row.low_current : row.high_current;
}
const float& selected(const DifficultyBandPair& row, int selector) {
    return selector < 2 ? row.low_current : row.high_current;
}
}
bool DifficultyCategoryBands::configure(
    const std::array<DifficultyBandPair,kCategories>& values) noexcept {
    configured_=false;
    for (const auto& row:values) {
        if (!valid(row.low_current) || !valid(row.low_saved) ||
            !valid(row.high_current) || !valid(row.high_saved)) return false;
    }
    factors_=values;
    configured_=true;
    return true;
}
const DifficultyBandPair* DifficultyCategoryBands::category(std::size_t index) const noexcept {
    return configured_ && index<kCategories ? &factors_[index] : nullptr;
}
bool DifficultyCategoryBands::get(std::size_t index,int selector,float& result) const noexcept {
    const auto* row=category(index);
    if (!row) return false;
    result=selected(*row,selector);
    return true;
}
DifficultyBandStatus DifficultyCategoryBands::set(
    std::size_t index,int selector,float value) noexcept {
    if (!configured_) return DifficultyBandStatus::NotConfigured;
    if (index>=kCategories) return DifficultyBandStatus::InvalidCategory;
    if (!valid(value)) return DifficultyBandStatus::InvalidFactor;
    selected(factors_[index],selector)=std::clamp(value,kMinimum,kMaximum);
    return DifficultyBandStatus::Applied;
}
DifficultyBandStatus DifficultyCategoryBands::add(
    std::size_t index,int selector,float delta) noexcept {
    if (!configured_) return DifficultyBandStatus::NotConfigured;
    if (index>=kCategories) return DifficultyBandStatus::InvalidCategory;
    if (!valid(delta)) return DifficultyBandStatus::InvalidDelta;
    const double candidate=static_cast<double>(selected(factors_[index],selector))+
                            static_cast<double>(delta);
    selected(factors_[index],selector)=static_cast<float>(
        std::clamp(candidate,static_cast<double>(kMinimum),static_cast<double>(kMaximum)));
    return DifficultyBandStatus::Applied;
}
DifficultyBandStatus DifficultyCategoryBands::reset_from_saved() noexcept {
    if (!configured_) return DifficultyBandStatus::NotConfigured;
    for (auto& row:factors_) {
        row.low_current=row.low_saved;
        row.high_current=row.high_saved;
    }
    return DifficultyBandStatus::Applied;
}
} // namespace sin_compat
