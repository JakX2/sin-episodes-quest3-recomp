#include "sin_compat/original_jessica_target_center.hpp"
#include <cmath>
#include <limits>

namespace sin_compat::jessica {
namespace {
bool finite(const TossVec3& p) noexcept {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}
bool fits_float(double v) noexcept {
    const auto limit=static_cast<double>(std::numeric_limits<float>::max());
    return std::isfinite(v) && v<=limit && v>=-limit;
}
bool valid_matrix(const TargetTransform3x4& m) noexcept {
    for(float x:m.row_major) if (!std::isfinite(x)) return false;
    return true;
}
bool zero_angles(const TossVec3& a) noexcept {
    return a.x==0.0f && a.y==0.0f && a.z==0.0f;
}
}
TargetCenterResult compute_player_world_collision_center(
    const PlayerTargetObservation& o) noexcept {
    TargetCenterResult result;
    if (!finite(o.obb_mins)||!finite(o.obb_maxs)||
        !finite(o.absolute_origin)||!finite(o.absolute_angles)) return result;

    // Verified helper 0x2214B020: mins + (maxs - mins)*0.5f,
    // with the literal 0.5f read from original VA 0x223D9778.
    const double cx=static_cast<double>(o.obb_mins.x)+
        (static_cast<double>(o.obb_maxs.x)-o.obb_mins.x)*0.5;
    const double cy=static_cast<double>(o.obb_mins.y)+
        (static_cast<double>(o.obb_maxs.y)-o.obb_mins.y)*0.5;
    const double cz=static_cast<double>(o.obb_mins.z)+
        (static_cast<double>(o.obb_maxs.z)-o.obb_mins.z)*0.5;
    if (!fits_float(cx)||!fits_float(cy)||!fits_float(cz)) return result;

    // 0x2214B0D0: collision flags bit 0x40 OR solid type 0/2 OR
    // zero world angles -> add entity origin. Otherwise, world transform.
    // These are the original numeric tests, not invented Source enum names.
    const bool translate=(o.solid_flags&0x40u)!=0u ||
        o.solid_type==0 || o.solid_type==2 || zero_angles(o.absolute_angles);
    double wx,wy,wz;
    if (translate) {
        wx=cx+o.absolute_origin.x;
        wy=cy+o.absolute_origin.y;
        wz=cz+o.absolute_origin.z;
        result.branch=TargetCenterBranch::TranslateOnly;
    } else {
        if (!o.entity_to_world) {
            result.code=TargetCenterCode::MissingWorldTransform;
            return result;
        }
        if(!valid_matrix(*o.entity_to_world)) return result;
        const auto& m=o.entity_to_world->row_major;
        wx=static_cast<double>(m[0])*cx+static_cast<double>(m[1])*cy+
           static_cast<double>(m[2])*cz+m[3];
        wy=static_cast<double>(m[4])*cx+static_cast<double>(m[5])*cy+
           static_cast<double>(m[6])*cz+m[7];
        wz=static_cast<double>(m[8])*cx+static_cast<double>(m[9])*cy+
           static_cast<double>(m[10])*cz+m[11];
        result.branch=TargetCenterBranch::EntityMatrix;
    }
    if (!fits_float(wx)||!fits_float(wy)||!fits_float(wz)) return result;
    result.world_center={static_cast<float>(wx),static_cast<float>(wy),
                         static_cast<float>(wz)};
    result.code=TargetCenterCode::Computed;
    return result;
}

std::optional<TossGeometry> build_original_jessica_toss_geometry(
    const TossVec3& right_hand_world,
    const PlayerTargetObservation& player,
    float live_sv_gravity) noexcept {
    if (!finite(right_hand_world)||!std::isfinite(live_sv_gravity)||
        live_sv_gravity<=0.0f) return std::nullopt;
    const auto center=compute_player_world_collision_center(player);
    if (!center.valid()) return std::nullopt;
    return TossGeometry{right_hand_world,center.world_center,live_sv_gravity};
}
} // namespace sin_compat::jessica
