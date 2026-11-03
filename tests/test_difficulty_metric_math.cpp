#include "sin_compat/difficulty_metric_math.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
using namespace sin_compat::difficulty;
static bool near(float a,float b) {return std::fabs(a-b)<0.000001f;}
int main() {
    static_assert(original_metric_count==14 && original_modifier_count==10);
    // The original +0 for NPC helmet probability is one float ULP above 0.35f.
    std::uint32_t helmet_bits=0;
    std::memcpy(&helmet_bits,&original_metrics[8].fields[0],sizeof helmet_bits);
    assert(helmet_bits==0x3eb33334u);
    assert(metric_index("DAMAGE_TAKEN_SCALE")==0);
    assert(metric_index("PERCENT_NPC_MOBILE")==13);
    assert(metric_index("damage_taken_scale")==-1); // case-sensitive original
    assert(metric_index("NONEXISTENT")==-1);
    for(std::size_t i=0;i<original_metrics.size();++i) {
        assert(metric_index(original_metrics[i].name)==static_cast<int>(i));
        assert(near(blended_metric(original_metrics[i],0.0f), original_metrics[i].fields[1]));
        assert(near(blended_metric(original_metrics[i],1.0f), original_metrics[i].fields[2]));
        assert(near(blended_metric(original_metrics[i],0.5f), original_metrics[i].fields[0]));
    }
    const auto midpoint=blended_metrics(0.5f);
    assert(near(midpoint[0],1.075f));
    assert(near(midpoint[4],0.4f));
    assert(near(midpoint[9],0.6f));
    const auto extrapolate=blended_metrics(1.5f); // no clamp in original routine
    assert(near(extrapolate[0],1.725f));
    assert(modifier_side(-100)==0 && modifier_side(0)==0 &&
           modifier_side(1)==0 && modifier_side(2)==1 && modifier_side(3)==1);
    assert(near(set_modifier_finite(0.0f),0.05f));
    assert(near(set_modifier_finite(0.05f),0.05f));
    assert(near(set_modifier_finite(0.5f),0.5f));
    assert(near(set_modifier_finite(2.0f),2.0f));
    assert(near(set_modifier_finite(7.0f),2.0f));
    assert(near(update_modifier_finite(1.0f,0.3f),1.3f));
    assert(near(update_modifier_finite(1.0f,-0.96f),0.05f));
    assert(near(update_modifier_finite(1.9f,0.2f),2.0f));
    // NaNs / negative time stepping and exact 80-bit x87 rounding are not
    // claimed by the finite-input model, and need target runtime validation.
}
