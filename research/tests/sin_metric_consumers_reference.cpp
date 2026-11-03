#include "sin_compat/difficulty_metric_consumers.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <array>
using namespace sin_compat::difficulty;
static bool close(float a,float b) {return std::fabs(a-b)<0.000002f;}
static std::uint32_t bits(float v) {std::uint32_t u=0;std::memcpy(&u,&v,4);return u;}
int main(){
    static_assert(original_damage_scale_bypass_mask==0x80024021u);
    auto init=original_initial_metric_values();
    for(std::size_t i=0;i<original_metric_count;++i)
        assert(bits(init[i])==bits(original_metrics[i].fields[0]));
    auto easy=metrics_for_blend_input(0.f);
    auto hard=metrics_for_blend_input(1.f);
    auto mid=metrics_for_blend_input(.5f);
    assert(close(easy[0],.75f) && close(hard[0],1.4f));
    assert(close(easy[11],.1f) && close(hard[11],-.1f));
    assert(close(mid[0],1.075f) && close(mid[11],0.f));
    OriginalDamageFields taken{100.0f,0};
    apply_damage_taken_metric(taken,easy);
    assert(close(taken.damage,75.f));
    taken={100.0f,0}; apply_damage_taken_metric(taken,hard);
    assert(close(taken.damage,140.f));
    for(auto flag : std::array<std::uint32_t,5>{0x00000001u,0x00000020u,0x00004000u,0x00020000u,0x80000000u}){
        taken={100.f,flag};apply_damage_taken_metric(taken,hard);
        assert(bits(taken.damage)==bits(100.f));
    }
    taken={100.f,original_damage_scale_bypass_mask};
    apply_damage_taken_metric(taken,hard);
    assert(bits(taken.damage)==bits(100.f));
    taken={100.f,0x00000002u};apply_damage_taken_metric(taken,hard);
    assert(close(taken.damage,140.f));
    // Post-archive mutable values verify correct metric index selection.
    auto altered=mid;
    altered[0]=.6f;altered[1]=1.25f;altered[2]=.7f;altered[11]=-.125f;
    assert(close(scaled_damage_done(80.f,altered),100.f));
    assert(close(ammo_quantity_multiplier(altered),.7f));
    assert(close(npc_random_wait_before_shoot(.5f,altered),.375f));
    taken={50.f,0};apply_damage_taken_metric(taken,altered);
    assert(close(taken.damage,30.f));
    assert(close(npc_random_wait_before_shoot(.5f,easy),.6f));
    assert(close(npc_random_wait_before_shoot(.5f,hard),.4f));
    auto extended=metrics_for_blend_input(1.5f);
    assert(close(extended[0],1.725f));
}
