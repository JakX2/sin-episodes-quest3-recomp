#pragma once
// Connect original additive impulse (0x2214BA20) to recovered absolute-
// velocity set/refresh logic. No native 2006/Android engine ABI is assumed.
#include "sin_compat/original_absolute_velocity.hpp"
#include "sin_compat/original_jessica_impulse.hpp"
namespace sin_compat::jessica {
class OriginalAbsoluteImpulseAdapter final:public IOriginalImpulseHost {
public:
    OriginalAbsoluteImpulseAdapter(IOriginalImpulseHost& movement,
                                   IOriginalAbsoluteVelocityHost& velocity)
        : movement_(movement),velocity_(velocity){}
    std::optional<std::int32_t> movement_type(ImpulseEntity id) override {
        return movement_.movement_type(id);
    }
    bool add_physics_velocity(ImpulseEntity id,const TossVec3& v) override {
        return movement_.add_physics_velocity(id,v);
    }
    std::optional<bool> absolute_velocity_dirty(ImpulseEntity id) override {
        const auto s=velocity_.snapshot(id);
        if(!s)return std::nullopt;
        return s->absolute_dirty;
    }
    bool refresh_absolute_velocity(ImpulseEntity id) override {
        return refresh_original_absolute_velocity(velocity_,id).successful();
    }
    std::optional<TossVec3> read_absolute_velocity(ImpulseEntity id) override {
        const auto s=velocity_.snapshot(id);
        if(!s || s->absolute_dirty)return std::nullopt;
        return s->absolute;
    }
    bool set_absolute_velocity(ImpulseEntity id,const TossVec3& v) override {
        return set_original_absolute_velocity(velocity_,id,v).successful();
    }
private:
    IOriginalImpulseHost& movement_;
    IOriginalAbsoluteVelocityHost& velocity_;
};
} // namespace sin_compat::jessica
