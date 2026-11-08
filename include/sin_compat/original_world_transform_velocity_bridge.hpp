#pragma once
// Bridge for Phase 107 IOriginalAbsoluteVelocityHost::refresh_world_transform:
// dispatches the Phase 108 reconstruction without assuming native pointers.
#include "sin_compat/original_world_transform.hpp"
namespace sin_compat::jessica {
class OriginalWorldTransformVelocityBridge final:
    public IOriginalAbsoluteVelocityHost {
public:
    OriginalWorldTransformVelocityBridge(IOriginalAbsoluteVelocityHost& velocity,
                                         IOriginalWorldTransformHost& world)
       : velocity_(velocity),world_(world){}
    std::optional<AbsoluteVelocitySnapshot> snapshot(AbsoluteEntityId id) override {
        return velocity_.snapshot(id);
    }
    bool invalidate_velocity_dependents(AbsoluteEntityId id,std::uint32_t mask) override {
        return velocity_.invalidate_velocity_dependents(id,mask);
    }
    bool clear_absolute_dirty(AbsoluteEntityId id) override {
        return velocity_.clear_absolute_dirty(id);
    }
    bool store_absolute_velocity(AbsoluteEntityId id,const TossVec3& v) override {
        return velocity_.store_absolute_velocity(id,v);
    }
    bool store_local_velocity(AbsoluteEntityId id,const TossVec3& v) override {
        return velocity_.store_local_velocity(id,v);
    }
    bool refresh_world_transform(AbsoluteEntityId id) override {
        return refresh_original_world_transform(world_,id).successful();
    }
private:
    IOriginalAbsoluteVelocityHost& velocity_;
    IOriginalWorldTransformHost& world_;
};
} // namespace sin_compat::jessica
