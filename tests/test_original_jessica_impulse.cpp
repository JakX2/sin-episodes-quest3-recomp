#include "sin_compat/original_jessica_impulse.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
using namespace sin_compat::jessica;
struct Host final : IOriginalImpulseHost {
    std::vector<std::string> log;
    std::optional<std::int32_t> type=2;
    std::optional<bool> dirty=false;
    std::optional<TossVec3> current=TossVec3{10,-20,3};
    TossVec3 written{},physics{};
    bool allow_refresh=true, allow_write=true, allow_physics=true, fail_throw=false;
    std::optional<std::int32_t> movement_type(ImpulseEntity e) override {
        assert(e==123);
        log.push_back("type");
        if (fail_throw) throw std::runtime_error("native failure");
        return type;
    }
    bool add_physics_velocity(ImpulseEntity e,const TossVec3& v) override {
        assert(e==123);log.push_back("physics");physics=v;return allow_physics;
    }
    std::optional<bool> absolute_velocity_dirty(ImpulseEntity e) override {
        assert(e==123);log.push_back("dirty");return dirty;
    }
    bool refresh_absolute_velocity(ImpulseEntity e) override {
        assert(e==123);log.push_back("refresh");return allow_refresh;
    }
    std::optional<TossVec3> read_absolute_velocity(ImpulseEntity e) override {
        assert(e==123);log.push_back("read");return current;
    }
    bool set_absolute_velocity(ImpulseEntity e,const TossVec3& v) override {
        assert(e==123);log.push_back("write");written=v;return allow_write;
    }
};
int main() {
    Host h;
    auto call=[&](TossVec3 v){return apply_original_velocity_impulse(h,123,v);};
    assert(call({0,0,0})==ImpulseCode::IgnoredZero);
    assert(call({-0.0f,0,-0.0f})==ImpulseCode::IgnoredZero);
    assert(h.log.empty());
    assert(call({1,2,3})==ImpulseCode::AppliedNonPhysics);
    assert((h.log==std::vector<std::string>{"type","dirty","read","write"}));
    assert(h.written.x==11 && h.written.y==-18 && h.written.z==6);
    h.log.clear();h.dirty=true;
    assert(call({3,-4,5})==ImpulseCode::AppliedNonPhysics);
    assert((h.log==std::vector<std::string>{"type","dirty","refresh","read","write"}));
    assert(h.written.x==13 && h.written.y==-24 && h.written.z==8);
    h.log.clear();h.allow_refresh=false;
    assert(call({1,2,3})==ImpulseCode::HostRejected);
    assert((h.log==std::vector<std::string>{"type","dirty","refresh"}));
    h.log.clear();h.allow_refresh=true;h.type=6;
    assert(call({1,2,3})==ImpulseCode::AppliedPhysics);
    assert((h.log==std::vector<std::string>{"type","physics"}));
    assert(h.physics.x==1 && h.physics.y==2 && h.physics.z==3);
    h.log.clear();h.allow_physics=false;
    assert(call({1,2,3})==ImpulseCode::HostRejected);
    h.log.clear();h.type.reset();
    assert(call({1,2,3})==ImpulseCode::HostRejected);
    assert((h.log==std::vector<std::string>{"type"}));
    h.log.clear();h.type=2;h.dirty.reset();
    assert(call({1,2,3})==ImpulseCode::HostRejected);
    assert((h.log==std::vector<std::string>{"type","dirty"}));
    h.dirty=false;h.log.clear();
    h.current=TossVec3{std::numeric_limits<float>::max(),1,1};
    assert(call({std::numeric_limits<float>::max(),1,1})==ImpulseCode::InvalidImpulse);
    assert((h.log==std::vector<std::string>{"type","dirty","read"}));
    h.log.clear();
    assert(call({std::numeric_limits<float>::quiet_NaN(),1,1})==ImpulseCode::InvalidImpulse);
    assert(h.log.empty());
    h.current.reset();h.log.clear();
    assert(call({1,1,1})==ImpulseCode::HostRejected);
    assert((h.log==std::vector<std::string>{"type","dirty","read"}));
    h.current=TossVec3{0,0,0};h.fail_throw=true;h.log.clear();
    assert(call({1,1,1})==ImpulseCode::HostRejected);
    assert((h.log==std::vector<std::string>{"type"}));
    std::cout<<"Original velocity impulse source-branch tests PASS\n";
}
