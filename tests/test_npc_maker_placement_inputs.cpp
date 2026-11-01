#include "sin_compat/npc_maker_placement_input_bridge.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <map>
#include <vector>
using namespace sin_compat;
struct World final : INpcSpawnPlacementWorld {
    std::map<std::uint32_t,std::uint32_t> gen;
    std::vector<NpcSpawnDestination> places;
    std::vector<NpcSpawnPlacement> clones;
    int notifications=0;
    unsigned next=10;
    EntityRef add(unsigned id){gen[id]=1;return {id,1};}
    bool is_live(EntityRef e) const override{auto i=gen.find(e.index);return i!=gen.end() && i->second==e.generation;}
    bool template_exists(const std::string& s) const override{return s=="npc_grunt";}
    bool is_maker_visible(EntityRef) const override{return false;}
    bool player_origin(SpawnVector3& p) const override{p={0,0,0};return true;}
    std::vector<NpcSpawnDestination> destinations(const std::string& group) const override{
        std::vector<NpcSpawnDestination> out;
        for(auto d:places)if(d.group==group)out.push_back(d);
        return out;
    }
    bool radius_point(EntityRef,double radius,SpawnVector3& p) override{
        assert(radius==64);p={64,0,0};return true;
    }
    bool line_point(EntityRef,SpawnVector3& p) override{p={-16,0,0};return true;}
    EntityRef create_template_child(EntityRef,const std::string&,const NpcSpawnPlacement& p) override{
        clones.push_back(p);return add(next++);
    }
    void destination_spawned(EntityRef,EntityRef) override{++notifications;}
};
int main(){
    World w;
    auto maker=w.add(1),dest=w.add(2);
    w.places={{dest,"group_b",{100,0,0},false,true,1,"renamed"}};
    NpcMakerPlacementHostAdapter adapter(w);
    assert(adapter.bind(maker,{"npc_grunt","group_a",64,SpawnVisibility::Any,SpawnDistance::Any,false,false}));
    NpcMakerRuntime runtime(adapter);
    assert(runtime.register_maker(maker,{3,-1,2,true,false}));
    const auto input=[&](const std::string& name,const std::string& value=""){
        return dispatch_npc_maker_placement_input(runtime,adapter,maker,name,value);
    };
    assert(!input("Nonexistent").recognized);
    assert(input("SpawnNPCInRadius").result==NpcMakerResult::Disabled);
    assert(input("Enable").result==NpcMakerResult::Applied);
    assert(input("ChangeDestinationGroup","group_b").result==NpcMakerResult::Applied);
    assert(input("Spawn").result==NpcMakerResult::Applied);
    assert(adapter.last_attempt().placement.destination==dest);
    assert(w.notifications==1);
    assert(input("SpawnNPCInLine").result==NpcMakerResult::Applied);
    assert(w.clones.back().origin.x==-16 && w.notifications==1);
    assert(input("SpawnNPCInRadius").result==NpcMakerResult::Applied);
    assert(w.clones.back().origin.x==64 && w.notifications==1);
    assert(runtime.total_spawned(maker)==3);
    assert(input("SpawnNPCInRadius").result==NpcMakerResult::Exhausted);
    assert(w.clones.size()==3);
    assert(input("ChangeDestinationGroup","").result==NpcMakerResult::Applied);
    assert(input("AddMaxChildren","2").result==NpcMakerResult::Applied);
    assert(input("Spawn").result==NpcMakerResult::NeedsReenable);
    assert(input("Enable").result==NpcMakerResult::Applied);
    assert(input("Spawn").result==NpcMakerResult::Applied);
    assert(w.clones.back().mode==SpawnPlacementMode::Radius);
    assert(w.notifications==1);
    w.gen[1]=2;
    assert(input("ChangeDestinationGroup","something").result==NpcMakerResult::InvalidMaker);
}
