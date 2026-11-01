#include "sin_compat/entity_io_runtime.hpp"
#include "sin_compat/npc_maker_runtime.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
using namespace sin_compat;
struct Scene final : INpcMakerHost, IEntityOutputHost {
    std::map<std::uint32_t,std::uint32_t> generations;
    std::map<std::string,EntityRef> names;
    std::uint32_t next_child=30;
    NpcMakerRuntime* makers=nullptr;
    EntityIoRuntime* io=nullptr;
    EntityRef autoent{},maker{},gate{},player{};
    std::vector<EntityRef> spawned;
    int opened=0, exhausted=0, all_dead=0;
    EntityRef add(std::uint32_t id, const std::string& name) {
        generations[id]=1;
        EntityRef ref{id,1}; names[name]=ref;return ref;
    }
    bool is_live(EntityRef ref) const override {
        auto it=generations.find(ref.index);
        return it!=generations.end() && it->second==ref.generation;
    }
    EntityRef create_child(EntityRef source) override {
        assert(source==maker);
        const auto id=next_child++;
        EntityRef child=add(id,"child_"+std::to_string(id));
        spawned.push_back(child);
        return child;
    }
    std::vector<EntityRef> resolve_targets(const std::string& name,
                                           EntityRef activator,EntityRef caller) override {
        if(name=="!activator")return {activator};
        if(name=="!caller")return {caller};
        auto it=names.find(name);
        return it==names.end()?std::vector<EntityRef>{}:std::vector<EntityRef>{it->second};
    }
    void accept_input(EntityRef target,const std::string& input,const std::string&,
                      EntityRef activator,EntityRef caller) override {
        if(target==maker&&input=="Enable") {
            assert(activator==player);
            assert(makers->enable(maker)==NpcMakerResult::Applied);
        } else if(target==gate&&input=="Open") {
            assert(caller==maker && activator==spawned.back());
            ++opened;
        } else if(target==gate&&input=="Exhausted") {
            assert(caller==maker);
            ++exhausted;
        } else if(target==gate&&input=="AllDead") {
            assert(caller==maker);
            ++all_dead;
        } else assert(false&&"unexpected mock entity input");
    }
    void relay(const std::vector<NpcMakerEvent>& events) {
        for (const auto& event: events) {
            if(event.type==NpcMakerEventType::Spawned) {
                assert(io->fire(maker,"OnSpawnNPC",event.child)==1);
            } else if(event.type==NpcMakerEventType::AllSpawned) {
                assert(io->fire(maker,"OnAllSpawned",maker)==1);
            } else if(event.type==NpcMakerEventType::AllSpawnedDead) {
                assert(io->fire(maker,"OnAllSpawnedDead",maker)==1);
            }
        }
    }
};
int main() {
    Scene scene;
    scene.autoent=scene.add(1,"auto");
    scene.maker=scene.add(2,"maker");
    scene.gate=scene.add(3,"gate");
    scene.player=scene.add(4,"player");
    NpcMakerRuntime makers(scene);
    EntityIoRuntime io(scene);
    scene.makers=&makers;scene.io=&io;
    assert(makers.register_maker(scene.maker,{2,1,2.0,true,false}));
    assert(io.connect(scene.autoent,"OnStartTouch",{"maker","Enable","",0.0,1}));
    assert(io.connect(scene.maker,"OnSpawnNPC",{"gate","Open","",0.25,-1}));
    assert(io.connect(scene.maker,"OnAllSpawned",{"gate","Exhausted","",0.0,1}));
    assert(io.connect(scene.maker,"OnAllSpawnedDead",{"gate","AllDead","",0.0,1}));
    assert(io.fire(scene.autoent,"OnStartTouch",scene.player)==1);
    assert(io.advance_to(0).inputs_delivered==1);
    assert(makers.advance_to(0,0).budget_exhausted);
    const auto first=makers.advance_to(0);
    assert(first.children_created==1 && first.events.size()==1);
    scene.relay(first.events);
    assert(io.advance_to(0.24).inputs_delivered==0);
    assert(io.advance_to(0.25).inputs_delivered==1 && scene.opened==1);
    assert(makers.advance_to(2.0).children_created==0); // live child cap
    scene.relay(makers.notify_child_removed(scene.maker,scene.spawned[0]).events);
    scene.generations[scene.spawned[0].index]=2; // recycled slot
    assert(io.advance_to(2.0).inputs_delivered==0);
    const auto second=makers.advance_to(2.0);
    assert(second.children_created==1 && second.events.size()==2);
    scene.relay(second.events);
    assert(io.advance_to(2.0).inputs_delivered==1 && scene.exhausted==1);
    assert(io.advance_to(2.25).inputs_delivered==1 && scene.opened==2);
    assert(makers.advance_to(10.0).children_created==0);
    assert(makers.total_spawned(scene.maker)==2);
    assert(makers.notify_child_removed(scene.maker,scene.spawned[0]).events.empty());
    scene.relay(makers.notify_child_removed(scene.maker,scene.spawned[1]).events);
    assert(io.advance_to(10.0).inputs_delivered==1 && scene.all_dead==1);
    assert(makers.notify_child_removed(scene.maker,scene.spawned[1]).events.empty());
    assert(io.fire(scene.autoent,"OnStartTouch",scene.player)==0);
    makers.remove_maker(scene.maker);
    io.remove_source(scene.maker);
    assert(makers.maker_count()==0 && io.pending_events()==0);
}
