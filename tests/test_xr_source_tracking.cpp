// Synthetic XR/OpenVR/OpenXR-convention matrices. The compiled test requires
// no headset, OpenXR runtime, original SourceVR binary or commercial assets.
#include "sin_compat/xr_source_tracking.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <limits>

using namespace sin_compat::xr_tracking;

static bool near(double a, double b, double tolerance=1e-8) {
    return std::abs(a-b) <= tolerance;
}

static TrackingMatrix34 identity() {
    return {{{{1,0,0,0},{0,1,0,0},{0,0,1,0}}}};
}

static void test_identity_and_units() {
    const auto m=identity();
    assert(valid_tracking_matrix(m));
    const auto p=tracking_pose_to_source(m, 64.0);
    assert(p);
    assert(near(p->position.x,0) && near(p->position.y,0) &&
           near(p->position.z,0));
    assert(near(p->angles.pitch_degrees,0) &&
           near(p->angles.yaw_degrees,0) &&
           near(p->angles.roll_degrees,0));
    auto translated=m;
    translated.rows[0][3]=2.0;    // tracking: right
    translated.rows[1][3]=3.0;    // tracking: up
    translated.rows[2][3]=-4.0;   // tracking: forward
    const auto q=tracking_pose_to_source(translated, 10.0);
    assert(q);
    assert(near(q->position.x,40) && near(q->position.y,-20) &&
           near(q->position.z,30));
    const auto velocity=tracking_vector_to_source({2.0,3.0,-4.0},10.0);
    assert(velocity);
    assert(near(velocity->x,q->position.x));
    assert(near(velocity->y,q->position.y));
    assert(near(velocity->z,q->position.z));
}

static void test_orientation_convention() {
    constexpr double rad=0.017453292519943295769236907684886;
    constexpr double yaw=45.0;
    auto m=identity();
    const auto c=std::cos(yaw*rad),s=std::sin(yaw*rad);
    m.rows[0][0]=c; m.rows[0][2]=s;
    m.rows[2][0]=-s; m.rows[2][2]=c;
    const auto p=tracking_pose_to_source(m,100.0);
    assert(p);
    assert(near(p->angles.yaw_degrees,45,1e-6));
    assert(near(p->angles.pitch_degrees,0));
    assert(near(p->angles.roll_degrees,0));
    // Tracking tilt: positive Source pitch means looking down.
    auto tilt=identity();
    tilt.rows[1][1]=c; tilt.rows[1][2]=s;
    tilt.rows[2][1]=-s; tilt.rows[2][2]=c;
    const auto q=tracking_pose_to_source(tilt,100.0);
    assert(q && near(q->angles.pitch_degrees,45,1e-6));
    assert(near(q->angles.yaw_degrees,0));
    // Source roll is atan2(-M[1][0],M[1][1]).
    auto roll=identity();
    roll.rows[0][0]=c; roll.rows[0][1]=s;
    roll.rows[1][0]=-s; roll.rows[1][1]=c;
    const auto r=tracking_pose_to_source(roll,100.0);
    assert(r && near(r->angles.roll_degrees,45,1e-6));
}

static void test_bad_host_data_is_refused() {
    const auto id=identity();
    assert(!tracking_pose_to_source(id,0));
    assert(!tracking_pose_to_source(id,-4));
    assert(!tracking_pose_to_source(id,std::numeric_limits<double>::infinity()));
    assert(!tracking_vector_to_source({0,0,0},0));
    auto bad=id;
    bad.rows[0][3]=std::numeric_limits<double>::quiet_NaN();
    assert(!valid_tracking_matrix(bad));
    assert(!tracking_pose_to_source(bad,10));
    bad=id;
    bad.rows[0][1]=0.5; // shear is not a rigid tracked headset pose
    assert(!valid_tracking_matrix(bad));
    assert(!tracking_pose_to_source(bad,10));
    bad=id;
    bad.rows[2][2]=-1; // reflection is not a valid right-handed rotation
    assert(!valid_tracking_matrix(bad));
    assert(!tracking_pose_to_source(bad,10));
    bad=id;
    bad.rows[0][0]=0.0; // zero scale on one basis column
    assert(!tracking_pose_to_source(bad,10));
    bad=id;
    bad.rows[1][2]=std::numeric_limits<double>::infinity();
    assert(!tracking_pose_to_source(bad,10));
    assert(!tracking_vector_to_source(
        {1,0,std::numeric_limits<double>::quiet_NaN()},10));
    assert(!tracking_vector_to_source({2,0,0},1e308)); // overflow
    assert(!valid_tracking_matrix(id,0));
    assert(!valid_tracking_matrix(id,0.1));
}

static void test_realistic_float_roundoff() {
    auto m=identity();
    m.rows[0][0]=1.0000001;
    m.rows[1][1]=0.9999999;
    assert(valid_tracking_matrix(m));
    const auto p=tracking_pose_to_source(m,42.0);
    assert(p && near(p->angles.yaw_degrees,0));
}

int main() {
    test_identity_and_units();
    test_orientation_convention();
    test_bad_host_data_is_refused();
    test_realistic_float_roundoff();
}
