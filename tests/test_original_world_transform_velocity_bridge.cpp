#include "sin_compat/original_world_transform_velocity_bridge.hpp"
#include "sin_compat/original_absolute_impulse_adapter.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
using namespace sin_compat::jessica;
void near(float a,float b){assert(std::abs(a-b)<0.002f);}
struct GameHost final : IOriginalAbsoluteVelocityHost {
    std::map<WorldEntityId,WorldTransformSnapshot> world;
    std::map<WorldEntityId,AbsoluteVelocitySnapshot> velocity;
    int refreshes=0;
    std::optional<AbsoluteVelocitySnapshot> snapshot(AbsoluteEntityId id) override {
        auto it=velocity.find(id);
        if(it==velocity.end()) return std::nullopt;
        return it->second;
    }
    std::optional<WorldTransformSnapshot> world_snapshot(WorldEntityId id) {
        auto it=world.find(id);
        if(it==world.end())return std::nullopt;
        return it->second;
    }
    // C++ cannot overload snapshot solely by return type. This host's world
    // adapter keeps the two read contracts independent.
    bool invalidate_velocity_dependents(AbsoluteEntityId,std::uint32_t mask) override {
        assert(mask==4);return true;
    }
    bool clear_absolute_dirty(AbsoluteEntityId id) override {
        velocity.at(id).absolute_dirty=false;return true;
    }
    bool store_absolute_velocity(AbsoluteEntityId id,const TossVec3& v) override {
        velocity.at(id).absolute=v;return true;
    }
    bool store_local_velocity(AbsoluteEntityId id,const TossVec3& v) override {
        velocity.at(id).local=v;return true;
    }
    bool refresh_world_transform(AbsoluteEntityId) override {
        assert(false && "must be dispatched through wrapper");return false;
    }
    std::optional<WorldMatrix3x4> attachment_parent_matrix(
        WorldEntityId,WorldEntityId,std::uint8_t) {return {};}
    bool commit_world_transform(WorldEntityId id,
                                const WorldTransformSnapshot& s) {
        world.at(id)=s;velocity.at(id).entity_to_world=s.world_matrix;
        velocity.at(id).transform_dirty=false;
        ++refreshes;return true;
    }
    bool on_world_transform_rebuilt(WorldEntityId) {return true;}
};
// Different snapshot types intentionally use separate provider interfaces.
struct WorldProvider final:IOriginalWorldTransformHost {
    GameHost& game;
    explicit WorldProvider(GameHost& g):game(g){}
    std::optional<WorldTransformSnapshot> snapshot(WorldEntityId id) override {
        return game.world_snapshot(id);
    }
    std::optional<WorldMatrix3x4> attachment_parent_matrix(
        WorldEntityId c,WorldEntityId p,std::uint8_t a) override {
        return game.attachment_parent_matrix(c,p,a);
    }
    bool commit_world_transform(WorldEntityId id,const WorldTransformSnapshot& s) {
        return game.commit_world_transform(id,s);
    }
    bool on_world_transform_rebuilt(WorldEntityId id) override {
        return game.on_world_transform_rebuilt(id);
    }
};
struct Movement final:IOriginalImpulseHost {
    int physics_calls=0;
    std::optional<std::int32_t> movement_type(ImpulseEntity) override {return 2;}
    bool add_physics_velocity(ImpulseEntity,const TossVec3&) override {
        ++physics_calls;return true;
    }
    std::optional<bool> absolute_velocity_dirty(ImpulseEntity) override {
        assert(false);return {};
    }
    bool refresh_absolute_velocity(ImpulseEntity) override {
        assert(false);return false;
    }
    std::optional<TossVec3> read_absolute_velocity(ImpulseEntity) override {
        assert(false);return {};
    }
    bool set_absolute_velocity(ImpulseEntity,const TossVec3&) override {
        assert(false);return false;
    }
};
int main(){
    GameHost game;
    WorldTransformSnapshot parent{};
    parent.dirty=true;parent.local_origin={10,20,30};parent.local_angles={0,90,0};
    game.world[1]=parent;
    auto parent_vel=AbsoluteVelocitySnapshot{};
    parent_vel.absolute={4,5,0};parent_vel.local={4,5,0};
    parent_vel.transform_dirty=true;
    game.velocity[1]=parent_vel;
    auto child_vel=AbsoluteVelocitySnapshot{};
    child_vel.parent=1;child_vel.local={0,-10,0};
    child_vel.absolute_dirty=true;
    game.velocity[2]=child_vel;
    WorldProvider world_provider(game);
    OriginalWorldTransformVelocityBridge bridge(game,world_provider);
    auto r=refresh_original_absolute_velocity(bridge,2);
    assert(r.code==AbsoluteVelocityCode::Refreshed);
    near(r.value.x,14);near(r.value.y,5);near(r.value.z,0);
    assert(game.refreshes==1);
    Movement movement;
    OriginalAbsoluteImpulseAdapter impulse(movement,bridge);
    assert(apply_original_velocity_impulse(impulse,2,{10,0,0})==
           ImpulseCode::AppliedNonPhysics);
    near(game.velocity[2].absolute.x,24);
    near(game.velocity[2].local.x,0);
    near(game.velocity[2].local.y,-20);
    assert(movement.physics_calls==0);
    assert(game.refreshes==1);
    std::cout<<"SiN world/absolute velocity/impulse integration PASS\n";
}
