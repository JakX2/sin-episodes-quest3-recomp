#include "sin_compat/difficulty_advisor_presets.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <array>
using namespace sin_compat::difficulty;
static bool near(float a, float b) { return std::fabs(a-b)<0.00005f; }
static std::uint32_t bits(float x) { std::uint32_t n=0; std::memcpy(&n,&x,sizeof n); return n; }
int main() {
    static_assert(challenge_advisor_count == 6);
    static_assert(challenge_preset_float_count == 5);
    assert(challenge_cvar_name == "dd_challenge");
    assert(challenge_cvar_default_text == "0.2f");
    assert(forgiveness_cvar_name == "dd_forgiveness");
    const std::array<std::uint32_t,30> expected{{
        0x42a00000,0x41a00000,0x41a00000,0x3e99999a,0x3eaaa64c,
        0x41a00000,0x42700000,0x41200000,0x3eaaa64c,0x3e2aa8eb,
        0x428c0000,0x41a00000,0x41a00000,0x3f000000,0x3f000000,
        0x41f00000,0x40a00000,0x41700000,0x3dcccccd,0x3f000000,
        0x41200000,0x428c0000,0x41f00000,0x3f000000,0x3f000000,
        0,0,0,0,0
    }};
    std::size_t pos = 0;
    for (const auto& p : original_advisor_presets)
        for (float f : {p.first,p.second,p.spread,p.first_fraction,p.second_fraction})
            assert(bits(f) == expected[pos++]);
    assert(pos==expected.size());

    // Row 0: first >= second. Row 1: first < second.
    const auto current=blend_all_advisor_presets(0.2f);
    assert(near(current[0].output_0,80.4f));
    assert(near(current[0].output_1,72.f));
    assert(near(current[0].output_2,52.f));
    assert(near(current[0].output_3,52.f * (1.f - 0.3333f)));
    assert(near(current[1].output_0,26.f * (1.f - 0.3333f)));
    assert(near(current[1].output_1,26.f));
    assert(near(current[1].output_2,36.f));
    assert(near(current[1].output_3,36.f + 0.16666f * 64.f));
    for (float f : {current[5].output_0,current[5].output_1,
                    current[5].output_2,current[5].output_3}) assert(bits(f)==0);

    const auto start=blend_all_advisor_presets(0.f);
    const auto end=blend_all_advisor_presets(1.f);
    assert(near(start[0].output_1,80.f));
    assert(near(end[0].output_1,40.f));
    assert(near(start[1].output_2,30.f));
    assert(near(end[1].output_2,60.f));
    const auto beyond=blend_all_advisor_presets(1.5f);
    assert(near(beyond[0].output_1,20.f));
    assert(near(initial_forgiveness_complement(0.5f),0.5f));
    assert(near(initial_forgiveness_complement(0.1f),0.9f));
}
