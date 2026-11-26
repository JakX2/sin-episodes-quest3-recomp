#pragma once
namespace sin_port::weapon_range_math {
// Original ordered FCOMP branches in 222aaae0 and 2230b2d0.
// A NaN in a weapon-configured threshold is unordered, not less/greater.
// The enclosing runtime validates caller-supplied dot and distance.
inline int condition(float dot,float distance,float minimum,float maximum) noexcept{
    return distance<minimum?0x24:distance>maximum?0x25:dot<.5f?0x26:0x14;
}
}
