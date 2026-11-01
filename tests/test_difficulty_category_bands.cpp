#include "sin_compat/difficulty_category_bands.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cmath>
#include <limits>
using namespace sin_compat;
int main() {
    DifficultyCategoryBands table;
    float number=-1;
    assert(!table.get(0,0,number));
    assert(table.set(0,0,1)==DifficultyBandStatus::NotConfigured);
    assert(table.reset_from_saved()==DifficultyBandStatus::NotConfigured);
    std::array<DifficultyBandPair,6> initial{};
    initial[0]={0.5f,0.25f,1.5f,1.75f};
    initial[5]={0.5f,0.5f,0.8f,0.8f};
    assert(table.configure(initial));
    assert(table.category(0) && table.category(0)->low_saved==0.25f);
    assert(table.category(5) && !table.category(6));
    assert(table.get(0,0,number)&&number==0.5f);
    assert(table.get(0,1,number)&&number==0.5f);
    assert(table.get(0,2,number)&&number==1.5f);
    assert(table.get(0,3,number)&&number==1.5f);
    assert(table.get(0,-1,number)&&number==0.5f);
    assert(table.get(0,9,number)&&number==1.5f);
    assert(table.set(0,0,-30)==DifficultyBandStatus::Applied);
    assert(table.get(0,0,number)&&number==DifficultyCategoryBands::kMinimum);
    assert(table.set(0,3,30)==DifficultyBandStatus::Applied);
    assert(table.get(0,3,number)&&number==2.0f);
    assert(table.add(0,0,0.1f)==DifficultyBandStatus::Applied);
    assert(table.get(0,0,number)&&number>0.14f&&number<0.151f);
    assert(table.add(0,1,-1e30f)==DifficultyBandStatus::Applied);
    assert(table.get(0,1,number)&&number==DifficultyCategoryBands::kMinimum);
    assert(table.add(0,2,1e30f)==DifficultyBandStatus::Applied);
    assert(table.get(0,3,number)&&number==DifficultyCategoryBands::kMaximum);
    assert(table.add(5,2,-1e30f)==DifficultyBandStatus::Applied);
    assert(table.get(5,2,number)&&number==DifficultyCategoryBands::kMinimum);
    assert(table.add(5,2,1e30f)==DifficultyBandStatus::Applied);
    assert(table.get(5,2,number)&&number==DifficultyCategoryBands::kMaximum);
    assert(table.set(6,0,1)==DifficultyBandStatus::InvalidCategory);
    assert(table.add(6,0,1)==DifficultyBandStatus::InvalidCategory);
    assert(table.set(0,0,NAN)==DifficultyBandStatus::InvalidFactor);
    assert(table.add(0,1,INFINITY)==DifficultyBandStatus::InvalidDelta);
    assert(table.get(0,0,number)&&number==DifficultyCategoryBands::kMinimum);
    assert(table.reset_from_saved()==DifficultyBandStatus::Applied);
    assert(table.get(0,0,number)&&number==0.25f);
    assert(table.get(0,3,number)&&number==1.75f);
    assert(table.get(5,3,number)&&number==0.8f);
    // Reset copies directly, even if saved baselines are outside set/add clamps.
    initial[2]={0.1f,-1.0f,1.0f,3.0f};
    assert(table.configure(initial));
    assert(table.reset_from_saved()==DifficultyBandStatus::Applied);
    assert(table.get(2,0,number)&&number==-1.0f);
    assert(table.get(2,2,number)&&number==3.0f);
    assert(table.add(2,2,-0.5f)==DifficultyBandStatus::Applied);
    assert(table.get(2,2,number)&&number==2.0f);
    initial[4].high_saved=std::numeric_limits<float>::quiet_NaN();
    assert(!table.configure(initial));
    assert(!table.category(0));
    return 0;
}
