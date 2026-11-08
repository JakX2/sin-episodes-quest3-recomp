#pragma once
// Original 2006 Jessica ballistic helper, independently reconstructed in
// C++17. Evidence: genuine server.dll 0x2204C850, called at 0x2236251A.
#include <cstdint>

namespace sin_compat::jessica {
struct TossVec3 { float x=0, y=0, z=0; };
struct TossGeometry {
    TossVec3 source{}, target{};
    float world_gravity=0; // host MUST supply live sv_gravity; default is 600
};
enum class TossCode : std::uint8_t { Calculated, InvalidInput, Degenerate };
struct TossResult {
    TossCode code=TossCode::InvalidInput;
    TossVec3 velocity{};
    float flight_seconds=0;
    float apex_above_source=0;
    bool valid() const noexcept { return code==TossCode::Calculated; }
};
// Original fifth argument is extra apex height, NOT throw speed.
// Finite/positive gravity guards are our safe host policy, not original x87.
TossResult solve_toss(const TossGeometry& geo,
                      float extra_apex_height=15.0f) noexcept;
}
