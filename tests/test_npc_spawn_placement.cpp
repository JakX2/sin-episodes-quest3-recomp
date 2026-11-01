#include "sin_compat/npc_spawn_placement.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
using namespace sin_compat;
struct World : INpcSpawnPlacementWorld {
    std::map<std::uint32_t,std::uint32_t> generations;
    std::vector<NpcSpawnDestination> available;
    std::uint32_t next=100;
    std::vector<NpcSpawnPlacement> created;
    std::vector<std::string> clone_templates;
    std::vector<EntityRef> notified;
    bool visible=false, has_template=true, radius_ok=true, line_ok=true, can_create=true, player_ok=true;
    SpawnVector3 player{};
    EntityRef add(unsigned id,unsigned gen=1){generations[id]=gen;return {id,gen};}
    bool is_live(EntityRef e) const override {
        auto i=generations.find(e.index);return i!=generations.end() && i->second==e.generation;
    }
    bool template_exists(const std::string& name) const override {
        return has_template && name=="npc_template_A";
    }
    bool is_maker_visible(EntityRef) const override {return visible;}
    bool player_origin(SpawnVector3& out) const override {if(!player_ok)return false;out=player;return true;}
    std::vector<NpcSpawnDestination> destinations(const std::string& group) const override {
        std::vector<NpcSpawnDestination> result;
        for (auto& dest:available) if(dest.group==group) result.push_back(dest);
        return result;
    }
    bool radius_point(EntityRef,double radius,SpawnVector3& out) override {
        assert(radius>=0); out={10,20,30};return radius_ok;
    }
    bool line_point(EntityRef,SpawnVector3& out) override {out={-7,3,2};return line_ok;}
    EntityRef create_template_child(EntityRef,const std::string& name,const NpcSpawnPlacement& where) override {
        if(!can_create)return {};
        created.push_back(where);clone_templates.push_back(name);
        return add(next++);
    }
    void destination_spawned(EntityRef dest,EntityRef child) override {
        assert(is_live(dest) && is_live(child)); notified.push_back(dest);
    }
};
int main(){
    World w;
    auto maker=w.add(1);
    const auto near=w.add(2),far=w.add(3),hidden=w.add(4),stale=w.add(5);
    w.available={{near,"group_a",{2,0,0},true,true,2.0,"near_npc"},
                 {far,"group_a",{20,0,0},true,true,1.0,"far_npc"},
                 {hidden,"group_a",{5,0,0},false,true,0.0,"hidden_npc"},
                 {stale,"group_a",{1,0,0},false,true,0.0,"stale"}};
    w.generations[5]=2;
    NpcSpawnPlacementPlanner planner(w);
    NpcSpawnPlacementConfig c{"npc_template_A","group_a",256,SpawnVisibility::Hidden,
                              SpawnDistance::Nearest,false,false};
    auto a=planner.spawn(maker,c);
    assert(a && a.placement.destination==hidden && a.placement.rename_npc=="hidden_npc");
    assert(w.notified.size()==1);
    assert(w.clone_templates.back()=="npc_template_A");
    c.criterion_visibility=SpawnVisibility::Visible;
    a=planner.spawn(maker,c);
    assert(a && a.placement.destination==near);
    assert(planner.advance_to(0.5));
    a=planner.spawn(maker,c);
    assert(a && a.placement.destination==far);
    assert(planner.advance_to(1.0));
    assert(planner.spawn(maker,c).result==SpawnPlacementResult::NoDestination);
    assert(planner.advance_to(1.5));
    a=planner.spawn(maker,c);
    assert(a && a.placement.destination==far);
    assert(planner.advance_to(2.0));
    a=planner.spawn(maker,c);
    assert(a && a.placement.destination==near);
    c.criterion_distance=SpawnDistance::Farthest;
    assert(planner.advance_to(3.0));
    a=planner.spawn(maker,c);
    assert(a && a.placement.destination==far);
    c.criterion_distance=SpawnDistance::Any;
    assert(planner.advance_to(4.0));
    a=planner.spawn(maker,c);
    assert(a && a.placement.destination==near);
    w.visible=true;
    c.dont_spawn_while_visible=true;
    assert(planner.spawn(maker,c).result==SpawnPlacementResult::MakerVisible);
    w.visible=false;
    c.force_radius=true;
    a=planner.spawn(maker,c);
    assert(a && a.placement.mode==SpawnPlacementMode::Radius && a.placement.destination==EntityRef{});
    assert(a.placement.origin.x==10);
    a=planner.spawn(maker,c,SpawnPlacementMode::Line);
    assert(a && a.placement.mode==SpawnPlacementMode::Line && a.placement.origin.x==-7);
    w.line_ok=false;
    assert(planner.spawn(maker,c,SpawnPlacementMode::Line).result==SpawnPlacementResult::NoSpawnPoint);
    w.line_ok=true;
    w.radius_ok=false;
    assert(planner.spawn(maker,c).result==SpawnPlacementResult::NoSpawnPoint);
    w.radius_ok=true;
    w.has_template=false;
    assert(planner.spawn(maker,c).result==SpawnPlacementResult::MissingTemplate);
    w.has_template=true;
    c.template_name="";
    assert(planner.spawn(maker,c).result==SpawnPlacementResult::InvalidConfig);
    c.template_name="npc_template_A";
    c.radius=-1;
    assert(planner.spawn(maker,c).result==SpawnPlacementResult::InvalidConfig);
    c.radius=256;
    assert(planner.spawn(EntityRef{1,2},c).result==SpawnPlacementResult::InvalidMaker);
    assert(!planner.advance_to(-1) && !planner.advance_to(NAN) && !planner.advance_to(INFINITY));
    assert(planner.now()==4.0);
    assert(planner.advance_to(5));
    c.force_radius=false;
    c.criterion_visibility=SpawnVisibility::Visible;
    c.criterion_distance=SpawnDistance::Nearest;
    assert(planner.advance_to(6));
    const auto notifications=w.notified.size();
    w.can_create=false;
    assert(planner.spawn(maker,c).result==SpawnPlacementResult::FactoryRejected);
    assert(w.notified.size()==notifications);
    w.can_create=true;
    assert(planner.spawn(maker,c));
    c.criterion_distance=SpawnDistance::Farthest;
    w.player_ok=false;
    assert(planner.spawn(maker,c).result==SpawnPlacementResult::NoDestination);
    w.player_ok=true;
    c.destination_group="wrong";
    assert(planner.spawn(maker,c).result==SpawnPlacementResult::NoDestination);
    bool threw=false;
    try{NpcSpawnPlacementPlanner fail(w,-1);}catch(const std::invalid_argument&){threw=true;}
    assert(threw);
    return 0;
}
