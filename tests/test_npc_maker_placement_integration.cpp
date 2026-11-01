#include "sin_compat/npc_spawn_placement.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <map>
#include <string>
#include <vector>
using namespace sin_compat;
struct World: INpcSpawnPlacementWorld {
    std::map<std::uint32_t,std::uint32_t> gen;
    std::vector<NpcSpawnDestination> entries;
    std::vector<NpcSpawnPlacement> placements;
    int notifications=0;
    unsigned next=100;
    EntityRef add(unsigned id){gen[id]=1;return {id,1};}
    bool is_live(EntityRef e) const override {auto it=gen.find(e.index);return it!=gen.end()&&it->second==e.generation;}
    bool template_exists(const std::string& n) const override {return n=="npc_grunt_template";}
    bool is_maker_visible(EntityRef) const override{return false;}
    bool player_origin(SpawnVector3& p) const override{p={0,0,0};return true;}
    std::vector<NpcSpawnDestination> destinations(const std::string& group) const override{
        std::vector<NpcSpawnDestination> out;for(const auto& e:entries)if(e.group==group)out.push_back(e);return out;
    }
    bool radius_point(EntityRef,double,SpawnVector3& out) override{out={10,10,0};return true;}
    bool line_point(EntityRef,SpawnVector3& out) override{out={-10,0,0};return true;}
    EntityRef create_template_child(EntityRef,const std::string&,const NpcSpawnPlacement& p) override{
        placements.push_back(p);return add(next++);
    }
    void destination_spawned(EntityRef,EntityRef) override{++notifications;}
};
int main(){
    World world;
    auto maker=world.add(1),d1=world.add(2),d2=world.add(3);
    world.entries={{d1,"grp",{2,0,0},false,true,2.0,"first_name"},
                   {d2,"grp",{5,0,0},false,true,2.0,"second_name"}};
    NpcMakerPlacementHostAdapter adapter(world);
    assert(adapter.bind(maker,{"npc_grunt_template","grp",256,SpawnVisibility::Hidden,SpawnDistance::Nearest,false,false}));
    NpcMakerRuntime maker_runtime(adapter);
    assert(maker_runtime.register_maker(maker,{3,2,1.0,true,false}));
    assert(adapter.set_time(0));
    assert(maker_runtime.enable(maker)==NpcMakerResult::Applied);
    auto first=maker_runtime.advance_to(0);
    assert(first.children_created==1 && adapter.last_attempt() && world.notifications==1);
    assert(adapter.last_attempt().placement.destination==d1);
    assert(adapter.set_time(1));
    auto second=maker_runtime.advance_to(1);
    assert(second.children_created==1 && adapter.last_attempt().placement.destination==d2);
    assert(world.notifications==2 && maker_runtime.live_children(maker)==2);
    auto forced=adapter.spawn_with_mode(maker_runtime,maker,SpawnPlacementMode::Line);
    assert(forced.result==NpcMakerResult::LiveLimitReached);
    assert(world.placements.size()==2);
    assert(maker_runtime.notify_child_removed(maker,first.events[0].child).result==NpcMakerResult::Applied);
    assert(adapter.set_time(1));
    forced=adapter.spawn_with_mode(maker_runtime,maker,SpawnPlacementMode::Line);
    assert(forced.result==NpcMakerResult::Applied && forced.events.size()==2);
    assert(adapter.last_attempt().placement.mode==SpawnPlacementMode::Line);
    assert(world.placements.size()==3 && world.notifications==2);
    assert(maker_runtime.total_spawned(maker)==3);
    assert(maker_runtime.spawn_now(maker).result==NpcMakerResult::Exhausted);
    assert(maker_runtime.needs_reenable(maker));
    assert(maker_runtime.add_max_children(maker,1)==NpcMakerResult::Applied);
    assert(adapter.set_time(3));
    assert(maker_runtime.advance_to(3).children_created==0);
    assert(maker_runtime.enable(maker)==NpcMakerResult::Applied);
    assert(maker_runtime.notify_child_removed(maker,second.events[0].child).result==NpcMakerResult::Applied);
    auto fourth=maker_runtime.advance_to(3);
    assert(fourth.children_created==1 && world.notifications==3);
    assert(adapter.last_attempt().placement.destination==d1);
    assert(maker_runtime.total_spawned(maker)==4);
    return 0;
}
