#include "sin_compat/original_jessica_toss.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace sin_compat::jessica {
namespace {
bool finite(const TossVec3& p) noexcept {
    return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);
}
}
TossResult solve_toss(const TossGeometry& geo,float extra_apex_height) noexcept {
    TossResult out{};
    if (!finite(geo.source)||!finite(geo.target)||
        !std::isfinite(geo.world_gravity)||geo.world_gravity<=0.0f||
        !std::isfinite(extra_apex_height)||extra_apex_height<0.0f)
        return out;
    // Original helper 0x2204C850 normalizes the XY delta and keeps dz.
    const double dx=double(geo.target.x)-geo.source.x;
    const double dy=double(geo.target.y)-geo.source.y;
    const double dz=double(geo.target.z)-geo.source.z;
    const double horiz=std::hypot(dx,dy);
    const double g=geo.world_gravity;
    const double peak=double(extra_apex_height)+std::max(dz,0.0);
    const double below_peak=peak-dz;
    if (!std::isfinite(horiz)||!std::isfinite(peak)||
        !std::isfinite(below_peak)||peak<0.0||below_peak<0.0) return out;
    // x87 caller: sqrt(2*g*peak) and sqrt(2*g*(peak-dz)).
    const double up=std::sqrt(2.0*g*peak);
    const double down=std::sqrt(2.0*g*below_peak);
    const double sum=up+down;
    if (!std::isfinite(sum)||sum<=0.0) {
        out.code=TossCode::Degenerate;
        return out;
    }
    const double t=sum/g;
    const double vx=dx*g/sum;
    const double vy=dy*g/sum;
    const double limit=std::numeric_limits<float>::max();
    if (!std::isfinite(vx)||!std::isfinite(vy)||!std::isfinite(t)||
        std::abs(vx)>limit||std::abs(vy)>limit||up>limit||t>limit||
        peak>limit) return out;
    out.code=TossCode::Calculated;
    out.velocity={float(vx),float(vy),float(up)};
    out.flight_seconds=float(t);
    out.apex_above_source=float(peak);
    return out;
}
} // namespace sin_compat::jessica
