#include "sin_compat/npc_maker_input_bridge.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
using namespace sin_compat;
struct Host : INpcMakerHost {
    std::map<std::uint32_t,std::uint32_t> gen;
    std::uint32_t next=10;
    EntityRef add(std::uint32_t id){gen[id]=1;return {id,1};}
    bool is_live(EntityRef e) const override {
        const auto it=gen.find(e.index);
        return it!=gen.end()&&it->second==e.generation;
    }
    EntityRef create_child(EntityRef) override {return add(next++);}
};
int main(){
    Host host;
    auto maker=host.add(1);
    NpcMakerRuntime runtime(host);
    assert(runtime.register_maker(maker,{1,1,2,true,false}));
    const auto send=[&](const std::string& key,const std::string& param=""){
        return dispatch_npc_maker_input(runtime,maker,key,param);
    };
    assert(!send("Unsupported").recognized);
    assert(send("SetMaxChildren","garbage").result==NpcMakerResult::InvalidSetting);
    assert(send("SetMaxChildren","1.5").result==NpcMakerResult::InvalidSetting);
    assert(send("SetMaxChildren","2147483648").result==NpcMakerResult::InvalidSetting);
    assert(send("SetMaxLiveChildren","0").result==NpcMakerResult::InvalidSetting);
    assert(send("SetSpawnFrequency","NaN").result==NpcMakerResult::InvalidSetting);
    assert(send("SetSpawnFrequency","1.0 nope").result==NpcMakerResult::InvalidSetting);
    assert(send("SetSpawnFrequency","0").result==NpcMakerResult::InvalidSetting);
    assert(send("SetSpawnFrequency","-2").result==NpcMakerResult::InvalidSetting);
    assert(send("AddMaxChildren","-2").result==NpcMakerResult::InvalidSetting);
    assert(send("Enable").result==NpcMakerResult::Applied);
    auto first=runtime.advance_to(0);
    assert(first.children_created==1);
    assert(first.events.size()==2 && first.events[1].type==NpcMakerEventType::AllSpawned);
    assert(runtime.needs_reenable(maker));
    assert(send("SetMaxChildren","2").result==NpcMakerResult::Applied);
    assert(runtime.advance_to(20).children_created==0); // quota increase does not restart
    assert(send("Spawn").result==NpcMakerResult::NeedsReenable);
    assert(send("Enable").result==NpcMakerResult::Applied); // rearm even when already enabled
    assert(!runtime.needs_reenable(maker));
    assert(runtime.advance_to(20).children_created==0); // live child cap remains
    assert(send("SetMaxLiveChildren"," 2 ").result==NpcMakerResult::Applied);
    auto second=runtime.advance_to(20);
    assert(second.children_created==1 && second.events.size()==2);
    assert(runtime.total_spawned(maker)==2 && runtime.needs_reenable(maker));
    assert(send("AddMaxChildren"," +2 ").result==NpcMakerResult::InvalidSetting); // strict from_chars
    assert(send("AddMaxChildren","2").result==NpcMakerResult::Applied);
    assert(runtime.advance_to(25).children_created==0);
    assert(send("Disable").result==NpcMakerResult::Applied);
    assert(send("Enable").result==NpcMakerResult::Applied);
    assert(send("SetSpawnFrequency"," 0.5 ").result==NpcMakerResult::Applied);
    assert(runtime.advance_to(25.49).children_created==0); // live cap reached
    assert(send("SetMaxLiveChildren","-1").result==NpcMakerResult::Applied);
    assert(runtime.advance_to(25.49).children_created==1); // resumed at available slot
    assert(runtime.advance_to(25.99).children_created==1);
    assert(runtime.total_spawned(maker)==4);
    assert(runtime.needs_reenable(maker));
    assert(send("AddMaxChildren","1").result==NpcMakerResult::Applied);
    assert(send("SetSpawnFrequency","-1").result==NpcMakerResult::Applied);
    assert(runtime.advance_to(30).children_created==0); // still latched
    assert(send("Enable").result==NpcMakerResult::Applied);
    assert(runtime.advance_to(30).children_created==0); // death driven while children remain
    for(std::uint32_t id=10;id<14;++id)
        assert(runtime.notify_child_removed(maker,{id,1}).result==NpcMakerResult::Applied);
    assert(runtime.advance_to(30).children_created==1);
    assert(runtime.total_spawned(maker)==5);
    const auto next=host.add(2);
    assert(runtime.register_maker(next,{5,-1,1,false,false}));
    assert(runtime.advance_to(30).children_created==1);
    assert(runtime.set_max_children(next,1)==NpcMakerResult::Applied);
    assert(runtime.needs_reenable(next));
    assert(runtime.add_max_children(next,1)==NpcMakerResult::Applied);
    assert(runtime.advance_to(200).children_created==0);
    assert(runtime.enable(next)==NpcMakerResult::Applied);
    assert(runtime.advance_to(200).children_created==1);
    assert(runtime.add_max_children(next,std::numeric_limits<int>::max())==NpcMakerResult::InvalidSetting);
    assert(runtime.set_max_live_children(next,-2)==NpcMakerResult::InvalidSetting);
    assert(runtime.set_spawn_frequency(next,INFINITY)==NpcMakerResult::InvalidSetting);
    host.gen[2]=2;
    assert(runtime.set_max_children(next,5)==NpcMakerResult::InvalidMaker);
    assert(runtime.set_spawn_frequency(next,2)==NpcMakerResult::InvalidMaker);
    return 0;
}
