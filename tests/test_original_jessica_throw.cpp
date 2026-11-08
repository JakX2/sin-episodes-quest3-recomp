#include "sin_compat/original_jessica_throw.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace sin_compat::jessica;
struct FakeHost final : IHost {
    std::vector<std::string> actions;
    bool condition = false;
    bool combat_weapon = true;
    bool can_spawn = true;
    bool launch_success = true;
    bool holster_success = false;
    int base_schedule = 300;
    std::string spawn_name;
    std::string spawn_attachment;
    std::int32_t primary = -999;
    std::int32_t secondary = -999;
    void set_condition(int id) override { assert(id==0x47); condition=true; actions.push_back("set"); }
    bool has_condition(int id) const override { assert(id==0x47); return condition; }
    void clear_condition(int id) override { assert(id==0x47); condition=false; actions.push_back("clear"); }
    int select_companion_schedule() override { actions.push_back("base_schedule"); return base_schedule; }
    void on_companion_animation_event(int id) override { actions.push_back("base_event:"+std::to_string(id)); }
    std::optional<SpawnedEntity> spawn_at_attachment(std::string_view name,std::string_view attachment) override {
        actions.push_back("spawn");spawn_name=std::string(name);spawn_attachment=std::string(attachment);
        return can_spawn ? std::optional<SpawnedEntity>(45) : std::nullopt;
    }
    bool is_combat_weapon(SpawnedEntity e) override { assert(e==45);actions.push_back("weapon_check"); return combat_weapon; }
    void set_primary_ammo(SpawnedEntity e,std::int32_t v) override { assert(e==45); primary=v;actions.push_back("primary"); }
    void set_secondary_ammo(SpawnedEntity e,std::int32_t v) override { assert(e==45); secondary=v;actions.push_back("secondary"); }
    bool launch_toward_player(SpawnedEntity e) override { assert(e==45);actions.push_back("launch");return launch_success; }
    bool holster_local_player_weapon() override { actions.push_back("holster");return holster_success; }
};
int main() {
    FakeHost host;
    ThrowRuntime npc(host);
    // Original spawn defaults, including no fixed animation event ID.
    assert(npc.pending_primary_ammo()==-1 && npc.pending_secondary_ammo()==-1);
    assert(npc.select_schedule()==300);
    assert(host.actions==std::vector<std::string>{"base_schedule"});
    host.actions.clear();
    assert(npc.on_animation_event(42)==AnimationResult::NoRegisteredEvent);
    assert(host.actions==std::vector<std::string>{"base_event:42"});
    npc.bind_animation_event(12345);
    host.actions.clear();
    assert(npc.on_animation_event(0x59)==AnimationResult::ForwardedToCompanion);
    assert(host.actions==std::vector<std::string>{"base_event:89"});
    host.actions.clear();

    npc.input_throw_item_at_player(InputValue::string_value("weapon_assault_rifle"));
    npc.input_set_primary_ammo_count_to_throw(InputValue::integer_value(35));
    npc.input_set_secondary_ammo_count_to_throw(InputValue::integer_value(4));
    assert(host.condition);
    assert(npc.select_schedule()==0x59);
    assert(host.actions==std::vector<std::string>{"set"});
    host.actions.clear();
    assert(npc.on_animation_event(12345)==AnimationResult::Launched);
    assert(host.actions==std::vector<std::string>({"clear","spawn","weapon_check","primary","secondary","launch"}));
    assert(!host.condition);
    assert(host.spawn_name=="weapon_assault_rifle" && host.spawn_attachment=="anim_attachment_RH");
    assert(host.primary==35 && host.secondary==4);
    assert(npc.pending_primary_ammo()==-1 && npc.pending_secondary_ammo()==-1);
    assert(npc.select_schedule()==300);

    // Wrong source variant types reset to empty classname or ZERO ammo.
    host.actions.clear();
    npc.input_throw_item_at_player(InputValue::integer_value(7));
    npc.input_set_primary_ammo_count_to_throw(InputValue::string_value("no"));
    npc.input_set_secondary_ammo_count_to_throw(InputValue::string_value("no"));
    assert(host.condition && npc.pending_classname().empty());
    assert(npc.pending_primary_ammo()==0 && npc.pending_secondary_ammo()==0);
    host.can_spawn=false;
    assert(npc.on_animation_event(12345)==AnimationResult::SpawnRejected);
    assert(!host.condition && npc.pending_primary_ammo()==0 && npc.pending_secondary_ammo()==0);
    assert(host.actions==std::vector<std::string>({"set","clear","spawn"}));

    // Nonweapon entities can still be thrown; pending ammo isn't consumed.
    host.can_spawn=true;host.combat_weapon=false;host.actions.clear();
    assert(npc.on_animation_event(12345)==AnimationResult::Launched);
    assert(host.actions==std::vector<std::string>({"clear","spawn","weapon_check","launch"}));
    assert(npc.pending_primary_ammo()==0 && npc.pending_secondary_ammo()==0);

    // Negative count -1 is 'do not override' and stays negative on an event.
    host.combat_weapon=true;
    npc.input_set_primary_ammo_count_to_throw(InputValue::integer_value(-20));
    npc.input_set_secondary_ammo_count_to_throw(InputValue::integer_value(6));
    host.actions.clear();host.launch_success=false;
    assert(npc.on_animation_event(12345)==AnimationResult::LaunchRejected);
    assert(host.actions==std::vector<std::string>({"clear","spawn","weapon_check","secondary","launch"}));
    assert(npc.pending_primary_ammo()==-20 && npc.pending_secondary_ammo()==-1);
    assert(host.secondary==6);

    // Nothing in the compiled handler checks the condition again at event
    // time; repeated matching animation events may invoke the host again.
    host.actions.clear();
    assert(npc.on_animation_event(12345)==AnimationResult::LaunchRejected);
    assert(host.actions==std::vector<std::string>({"clear","spawn","weapon_check","launch"}));

    assert(!npc.input_holster_weapon());
    host.holster_success=true;
    assert(npc.input_holster_weapon());
    npc.unbind_animation_event();
    host.actions.clear();
    assert(npc.on_animation_event(12345)==AnimationResult::NoRegisteredEvent);
    assert(host.actions==std::vector<std::string>{"base_event:12345"});
    std::cout<<"Original Jessica throw reference tests PASS\n";
}
