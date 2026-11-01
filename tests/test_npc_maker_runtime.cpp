#include "sin_compat/npc_maker_runtime.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <vector>
using namespace sin_compat;
struct Host : INpcMakerHost {
    std::map<std::uint32_t, std::uint32_t> entities;
    std::uint32_t next = 100;
    bool failure = false;
    EntityRef add(std::uint32_t index, std::uint32_t generation=1) {
        entities[index] = generation;
        return {index, generation};
    }
    bool is_live(EntityRef e) const override {
        auto it=entities.find(e.index);
        return it != entities.end() && it->second==e.generation;
    }
    EntityRef create_child(EntityRef) override {
        if (failure) return {};
        return add(next++);
    }
};
int main() {
    Host host;
    const auto maker=host.add(1);
    NpcMakerRuntime runtime(host);
    assert(runtime.register_maker(maker,{3,1,1.0,true,false}));
    assert(!runtime.register_maker(maker,{}));
    assert(runtime.advance_to(2.0).children_created == 0); // disabled
    assert(runtime.enable(maker)==NpcMakerResult::Applied);
    assert(runtime.enable(maker)==NpcMakerResult::NoChange);
    const auto first=runtime.advance_to(2.0);
    assert(first.children_created==1 && first.events.size()==1);
    assert(first.events[0].type==NpcMakerEventType::Spawned);
    EntityRef first_child=first.events[0].child;
    assert(runtime.live_children(maker)==1 && runtime.total_spawned(maker)==1);
    assert(runtime.advance_to(3.0).children_created==0); // live cap
    assert(runtime.spawn_now(maker).result==NpcMakerResult::LiveLimitReached);
    const auto died=runtime.notify_child_removed(maker,first_child);
    assert(died.result==NpcMakerResult::Applied && died.events.empty());
    assert(runtime.notify_child_removed(maker,first_child).result==NpcMakerResult::NoChange);
    assert(runtime.advance_to(3.0).children_created==1);
    auto next=runtime.spawn_now(maker); // at cap
    assert(next.result==NpcMakerResult::LiveLimitReached);
    assert(runtime.disable(maker)==NpcMakerResult::Applied);
    assert(runtime.disable(maker)==NpcMakerResult::NoChange);
    assert(runtime.spawn_now(maker).result==NpcMakerResult::Disabled);
    assert(runtime.advance_to(10.0).children_created==0);
    assert(runtime.toggle(maker)==NpcMakerResult::Applied);
    const auto prev=host.next-1;
    assert(runtime.notify_child_removed(maker,EntityRef{prev,1}).result==NpcMakerResult::Applied);
    const auto third=runtime.advance_to(10.0);
    assert(third.children_created==1 && third.events.size()==2);
    assert(third.events[1].type==NpcMakerEventType::AllSpawned);
    assert(runtime.total_spawned(maker)==3);
    assert(runtime.spawn_now(maker).result==NpcMakerResult::Exhausted);
    assert(runtime.notify_child_removed(maker,third.events[0].child).events.size()==1);
    assert(runtime.notify_child_removed(maker,third.events[0].child).events.empty());
    assert(runtime.advance_to(50.0).children_created==0);
    assert(runtime.advance_to(49.0).invalid_time);
    assert(runtime.advance_to(NAN).invalid_time);

    // Death-driven special case: -1 means replace after the last live child.
    const auto death_maker=host.add(2);
    assert(runtime.register_maker(death_maker,{5,-1,-1,true,false}));
    assert(runtime.enable(death_maker)==NpcMakerResult::Applied);
    const auto death_first=runtime.advance_to(50.0);
    assert(death_first.children_created==1);
    assert(runtime.advance_to(200.0).children_created==0);
    assert(runtime.spawn_now(death_maker).result==NpcMakerResult::Applied); // explicit spawns allowed
    assert(runtime.live_children(death_maker)==2);
    assert(runtime.notify_child_removed(death_maker,death_first.events.front().child).result==NpcMakerResult::Applied);
    assert(runtime.advance_to(200.0).children_created==0); // not last dead
    const EntityRef other{host.next-1,1};
    assert(runtime.notify_child_removed(death_maker,other).result==NpcMakerResult::Applied);
    assert(runtime.advance_to(200.0).children_created==1);

    // Finite schedules catch up with a bounded per-frame budget.
    const auto catchup=host.add(3);
    assert(runtime.register_maker(catchup,{10,-1,1.0,false,false}));
    auto zero=runtime.advance_to(200.0,0);
    assert(zero.budget_exhausted && zero.children_created==0);
    auto one=runtime.advance_to(206.0,3);
    assert(one.budget_exhausted && one.children_created==3);
    auto rest=runtime.advance_to(206.0,15);
    assert(!rest.budget_exhausted && rest.children_created==4);
    assert(runtime.total_spawned(catchup)==7);
    assert(runtime.notify_child_removed(catchup,EntityRef{999,1}).result==NpcMakerResult::NoChange);

    const auto bad=host.add(4);
    assert(!runtime.register_maker(bad,{-1,1,1,false,false}));
    assert(!runtime.register_maker(bad,{1,0,1,false,false}));
    assert(!runtime.register_maker(bad,{1,-2,1,false,false}));
    assert(!runtime.register_maker(bad,{1,1,0,false,false}));
    assert(!runtime.register_maker(bad,{1,1,-2,false,false}));
    assert(!runtime.register_maker(bad,{1,1,INFINITY,false,false}));
    assert(!runtime.register_maker(bad,{1,1,NAN,false,false}));
    assert(!runtime.register_maker(EntityRef{},{}));
    assert(runtime.register_maker(bad,{1,1,5,true,false}));
    host.entities[4]=2; // stale maker generation
    assert(runtime.enable(bad)==NpcMakerResult::InvalidMaker);
    assert(runtime.spawn_now(bad).result==NpcMakerResult::InvalidMaker);
    runtime.remove_maker(bad);
    assert(runtime.register_maker(EntityRef{4,2},{1,1,5,false,false}));
    host.failure=true;
    const auto fail=runtime.advance_to(206.0);
    assert(fail.attempts>=1 && fail.children_created==0);
    host.failure=false;
    const auto recovered=runtime.advance_to(211.0);
    assert(recovered.children_created>=1);
    bool recovered_bad_maker=false;
    for (const auto& event : recovered.events)
        if (event.type==NpcMakerEventType::Spawned && event.maker==EntityRef{4,2})
            recovered_bad_maker=true;
    assert(recovered_bad_maker);

    const auto infinite=host.add(5);
    assert(runtime.register_maker(infinite,{0,-1,2.0,false,true}));
    assert(runtime.advance_to(211.0).children_created==1);
    assert(runtime.total_spawned(infinite)==1);
    assert(runtime.spawn_now(infinite).result==NpcMakerResult::Applied);
    assert(runtime.total_spawned(infinite)==2);
    auto maxcfg = NpcMakerConfig{0,1,2,false,false};
    const auto exhausted=host.add(6);
    assert(runtime.register_maker(exhausted,maxcfg));
    assert(runtime.advance_to(211.0).children_created==0);
    assert(runtime.spawn_now(exhausted).result==NpcMakerResult::Exhausted);

    bool threw=false;
    try { NpcMakerRuntime impossible(host,-1); }
    catch(const std::invalid_argument&) { threw=true; }
    assert(threw);
}
