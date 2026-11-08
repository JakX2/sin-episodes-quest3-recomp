#include "sin_compat/original_absolute_impulse_adapter.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
using namespace sin_compat::jessica;
struct Movement final:IOriginalImpulseHost {
    bool physical=false;int physics_calls=0;
    std::optional<std::int32_t> movement_type(ImpulseEntity) override {
        return physical?6:2;
    }
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
struct Graph final:IOriginalAbsoluteVelocityHost {
    std::map<AbsoluteEntityId,AbsoluteVelocitySnapshot> nodes;
    std::optional<AbsoluteVelocitySnapshot> snapshot(AbsoluteEntityId id) override {
        auto i=nodes.find(id);
        return i==nodes.end()?std::nullopt:
            std::optional<AbsoluteVelocitySnapshot>(i->second);
    }
    bool invalidate_velocity_dependents(AbsoluteEntityId,std::uint32_t m) override {
        assert(m==4);return true;
    }
    bool clear_absolute_dirty(AbsoluteEntityId id) override {
        nodes[id].absolute_dirty=false;return true;
    }
    bool store_absolute_velocity(AbsoluteEntityId id,const TossVec3& v) override {
        nodes[id].absolute=v;return true;
    }
    bool store_local_velocity(AbsoluteEntityId id,const TossVec3& v) override {
        nodes[id].local=v;return true;
    }
    bool refresh_world_transform(AbsoluteEntityId id) override {
        nodes[id].transform_dirty=false;return true;
    }
};
void eq(float a,float b){assert(std::abs(a-b)<0.001f);}
int main(){
    Movement movement;Graph graph;
    AbsoluteVelocitySnapshot parent{};
    parent.absolute={10,20,0};parent.local=parent.absolute;
    parent.entity_to_world.row_major={0,-1,0,100,1,0,0,200,0,0,1,300};
    graph.nodes[7]=parent;
    AbsoluteVelocitySnapshot child{};
    child.parent=7;child.absolute={10,20,0};child.local={0,0,0};
    graph.nodes[8]=child;
    OriginalAbsoluteImpulseAdapter bridge(movement,graph);
    assert(apply_original_velocity_impulse(bridge,8,{10,0,5})==
           ImpulseCode::AppliedNonPhysics);
    eq(graph.nodes[8].absolute.x,20);eq(graph.nodes[8].absolute.y,20);
    eq(graph.nodes[8].absolute.z,5);
    eq(graph.nodes[8].local.x,0);eq(graph.nodes[8].local.y,-10);
    eq(graph.nodes[8].local.z,5);
    graph.nodes[7].absolute_dirty=true;graph.nodes[7].local={7,9,0};
    graph.nodes[8].absolute_dirty=true;graph.nodes[8].local={0,-10,5};
    assert(apply_original_velocity_impulse(bridge,8,{10,0,0})==
           ImpulseCode::AppliedNonPhysics);
    eq(graph.nodes[7].absolute.x,7);
    eq(graph.nodes[8].absolute.x,27);eq(graph.nodes[8].absolute.y,9);
    movement.physical=true;
    assert(apply_original_velocity_impulse(bridge,8,{4,5,6})==
           ImpulseCode::AppliedPhysics);
    assert(movement.physics_calls==1);
    assert(apply_original_velocity_impulse(bridge,8,{0,0,0})==
           ImpulseCode::IgnoredZero);
    assert(movement.physics_calls==1);
    std::cout<<"SiN additive impulse and absolute velocity bridge PASS\n";
}
