#include "sin_compat/npc_maker_input_bridge.hpp"
#include "sin_compat/entity_io_runtime.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
using namespace sin_compat;
class Scene final : public INpcMakerHost, public IEntityOutputHost {
public:
    std::map<std::uint32_t, std::uint32_t> generations;
    std::map<std::string, EntityRef> names;
    std::vector<EntityRef> children;
    EntityRef trigger{}, maker{}, gate{}, player{};
    EntityIoRuntime* io = nullptr;
    NpcMakerRuntime* makers = nullptr;
    int opens = 0, all_dead = 0, quota_updates = 0, rearms = 0;
    EntityRef add(std::uint32_t id, const std::string& name) {
        generations[id] = 1;
        EntityRef result{id, 1}; names[name] = result; return result;
    }
    bool is_live(EntityRef e) const override {
        const auto it=generations.find(e.index);
        return it!=generations.end() && it->second==e.generation;
    }
    EntityRef create_child(EntityRef source) override {
        assert(source==maker);
        const auto child=add(static_cast<std::uint32_t>(100+children.size()),
                             "child_"+std::to_string(children.size()));
        children.push_back(child);
        return child;
    }
    std::vector<EntityRef> resolve_targets(const std::string& name,EntityRef a,EntityRef c) override {
        if(name=="!activator")return {a};
        if(name=="!caller")return {c};
        const auto it=names.find(name);
        return it==names.end()?std::vector<EntityRef>{}:std::vector<EntityRef>{it->second};
    }
    void accept_input(EntityRef target,const std::string& input,const std::string& param,
                      EntityRef activator, EntityRef caller) override {
        if(target==maker){
            const auto action=dispatch_npc_maker_input(*makers,maker,input,param);
            assert(action.recognized && action.result==NpcMakerResult::Applied);
            if(input=="AddMaxChildren") ++quota_updates;
            if(input=="Enable") ++rearms;
            for(const auto& e:action.events) send(e);
        }else if(target==gate && input=="Open"){
            assert(caller==maker && is_live(activator));
            ++opens;
        }else if(target==gate && input=="Final"){
            assert(caller==maker);
            ++all_dead;
        }else assert(false && "unrecognized synthetic input");
    }
    void send(const NpcMakerEvent& event){
        const char* output=event.type==NpcMakerEventType::Spawned?"OnSpawnNPC":
            event.type==NpcMakerEventType::AllSpawned?"OnAllSpawned":"OnAllSpawnedDead";
        const EntityRef activator=event.type==NpcMakerEventType::Spawned?event.child:maker;
        const auto queued=io->fire(maker,output,activator);
        // The synthetic map uses max-fires=1 for the quota-rearm script.
        if(event.type==NpcMakerEventType::AllSpawned &&
           makers->total_spawned(maker)>1) assert(queued==0);
        else assert(queued>=1);
    }
    void send_all(const std::vector<NpcMakerEvent>& events){for(const auto& e:events)send(e);}
};
int main(){
    Scene scene;
    scene.trigger=scene.add(1,"trigger");scene.maker=scene.add(2,"maker");
    scene.gate=scene.add(3,"gate");scene.player=scene.add(4,"player");
    NpcMakerRuntime makers(scene);EntityIoRuntime io(scene);
    scene.io=&io;scene.makers=&makers;
    assert(makers.register_maker(scene.maker,{1,1,2.0,true,false}));
    assert(io.connect(scene.trigger,"OnStartTouch",{"maker","Enable","",0,1}));
    assert(io.connect(scene.maker,"OnSpawnNPC",{"gate","Open","",0.1,-1}));
    // The SDK input names are exact. Timing and links are hypothetical.
    assert(io.connect(scene.maker,"OnAllSpawned",{"maker","AddMaxChildren","2",0.25,1}));
    assert(io.connect(scene.maker,"OnAllSpawned",{"maker","Enable","",0.5,1}));
    assert(io.connect(scene.maker,"OnAllSpawnedDead",{"gate","Final","",0,-1}));
    assert(io.fire(scene.trigger,"OnStartTouch",scene.player)==1);
    assert(io.advance_to(0).inputs_delivered==1);
    const auto first=makers.advance_to(0);
    assert(first.children_created==1 && first.events.size()==2);
    scene.send_all(first.events);
    assert(io.advance_to(0.1).inputs_delivered==1 && scene.opens==1);
    assert(io.advance_to(0.25).inputs_delivered==1 && scene.quota_updates==1);
    assert(makers.needs_reenable(scene.maker));
    assert(makers.advance_to(0.25).children_created==0);
    assert(io.advance_to(0.5).inputs_delivered==1 && scene.rearms==2);
    assert(!makers.needs_reenable(scene.maker));
    assert(makers.advance_to(0.5).children_created==0);
    scene.send_all(makers.notify_child_removed(scene.maker,scene.children[0]).events);
    const auto second=makers.advance_to(0.5);
    assert(second.children_created==1 && second.events.size()==1);
    scene.send_all(second.events);
    assert(io.advance_to(0.6).inputs_delivered==1 && scene.opens==2);
    scene.send_all(makers.notify_child_removed(scene.maker,scene.children[1]).events);
    const auto third=makers.advance_to(0.6);
    assert(third.children_created==1 && third.events.size()==2);
    scene.send_all(third.events);
    assert(io.advance_to(0.7).inputs_delivered==1 && scene.opens==3);
    scene.send_all(makers.notify_child_removed(scene.maker,scene.children[2]).events);
    assert(io.advance_to(0.7).inputs_delivered==1 && scene.all_dead==1);
    assert(makers.total_spawned(scene.maker)==3);
    assert(makers.live_children(scene.maker)==0);
    assert(io.pending_events()==0);
}
