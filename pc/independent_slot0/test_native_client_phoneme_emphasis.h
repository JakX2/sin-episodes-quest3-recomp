#pragma once
#include "native_client_phoneme_emphasis.h"
#include <cassert>
#include <cmath>
namespace sin_port::native_weapon_tables::test {
inline void check_client_phoneme_emphasis(const ClientPhonemeEmphasisApi& api){
    auto close_weight=[](float a,float b){return std::fabs(a-b)<.00001f;};
    float e=-1;
    assert(api.curve(nullptr,0,0,1,&e)&&e==.5f);
    ClientEmphasisSample rows[]={{.25f,0},{.75f,1}};
    assert(api.curve(rows,2,0,1,&e)&&close_weight(e,.5f));
    assert(api.curve(rows,2,.25f,1,&e)&&close_weight(e,0));
    assert(api.curve(rows,2,.5f,1,&e)&&close_weight(e,.5f));
    assert(api.curve(rows,2,.75f,1,&e)&&close_weight(e,1));
    assert(api.curve(rows,2,1,1,&e)&&close_weight(e,.5f));
    assert(api.curve(rows,2,-1,1,&e)&&close_weight(e,.5f));
    ClientEmphasisSample long_rows[]={{0,0},{2,1}};
    assert(api.curve(long_rows,2,1,3,&e)&&close_weight(e,1)); // original caps interval divisor at 1
    ClientPhonemeBankWeights b{};
    assert(api.blend(.2f,true,true,&b)&&close_weight(b.weak,.5f)&&close_weight(b.normal,.4f)&&b.strong==0);
    assert(api.blend(.2f,false,true,&b)&&b.weak==0&&close_weight(b.normal,.8f)&&b.strong==0);
    assert(api.blend(.4f,true,true,&b)&&b.weak==0&&close_weight(b.normal,.8f)&&b.strong==0);
    assert(api.blend(.5f,true,true,&b)&&b.weak==0&&b.normal==1&&b.strong==0);
    assert(api.blend(.6f,true,true,&b)&&b.weak==0&&close_weight(b.normal,1.2f)&&b.strong==0);
    assert(api.blend(.8f,true,true,&b)&&b.weak==0&&close_weight(b.normal,.6f)&&close_weight(b.strong,.5f));
    assert(api.blend(.8f,true,false,&b)&&b.weak==0&&close_weight(b.normal,1.2f)&&b.strong==0);
    assert(api.blend(.2f,true,true,&b));
    ClientPhonemeContribution weak[]={{7,2},{7,-1}},normal[]={{7,1},{8,.5f}};
    ClientPhonemeBank banks[]={{weak,2},{normal,2},{nullptr,0}};
    float weights[128]{};weights[7]=.25f;weights[9]=.75f;
    assert(api.accumulate(.5f,&b,banks,weights));
    assert(close_weight(weights[7],.7f)&&close_weight(weights[8],.1f)&&weights[9]==.75f);
    normal[1].global=128;
    assert(!api.accumulate(.5f,&b,banks,weights)&&close_weight(weights[7],.7f));
    rows[1].time=.1f;e=42;
    assert(!api.curve(rows,2,0,1,&e)&&e==42);
}
}
