#include "sin_compat/original_jessica_throw.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>
using namespace sin_compat::jessica;
struct Host final : IHost, IOriginalTossProvider {
    std::vector<const char*> calls;
    bool condition=false,invalid_gravity=false,velocity_set=false,legacy_called=false;
    bool use_physics=false;
    TossVec3 velocity{},starting_velocity{11,22,33},physics_added{};
    void set_condition(int c) override {assert(c==0x47);condition=true;calls.push_back("set");}
    bool has_condition(int c) const override {assert(c==0x47);return condition;}
    void clear_condition(int c) override {assert(c==0x47);condition=false;calls.push_back("clear");}
    int select_companion_schedule() override {return -1;}
    void on_companion_animation_event(int) override {assert(false);}
    std::optional<SpawnedEntity> spawn_at_attachment(std::string_view n,std::string_view a) override {
        assert(n=="weapon_assault_rifle"&&a=="anim_attachment_RH");
        calls.push_back("spawn");return 17;
    }
    bool is_combat_weapon(SpawnedEntity e) override {assert(e==17);return true;}
    void set_primary_ammo(SpawnedEntity e,std::int32_t n) override {
        assert(e==17&&n==12);calls.push_back("ammo");
    }
    void set_secondary_ammo(SpawnedEntity,std::int32_t) override {assert(false);}
    bool launch_toward_player(SpawnedEntity) override {legacy_called=true;return false;}
    bool holster_local_player_weapon() override {return true;}
    std::optional<TossGeometry> observe_toss(SpawnedEntity e,std::string_view a) override {
        assert(e==17&&a=="anim_attachment_RH");
        calls.push_back("observe");
        return TossGeometry{{0,0,32},{120,0,48},invalid_gravity?0.0f:600.0f};
    }
    std::optional<std::int32_t> movement_type(ImpulseEntity e) override {
        assert(e==17);calls.push_back("type");return use_physics ? 6 : 2;
    }
    bool add_physics_velocity(ImpulseEntity e,const TossVec3& v) override {
        assert(e==17);calls.push_back("physics");physics_added=v;return true;
    }
    std::optional<bool> absolute_velocity_dirty(ImpulseEntity e) override {
        assert(e==17);calls.push_back("dirty");return false;
    }
    bool refresh_absolute_velocity(ImpulseEntity) override {assert(false);return false;}
    std::optional<TossVec3> read_absolute_velocity(ImpulseEntity e) override {
        assert(e==17);calls.push_back("read");return starting_velocity;
    }
    bool set_absolute_velocity(ImpulseEntity e,const TossVec3& v) override {
        assert(e==17);velocity_set=true;velocity=v;calls.push_back("write");return true;
    }
};
int main() {
    Host h;ThrowRuntime rt(h,&h);rt.bind_animation_event(201);
    rt.input_throw_item_at_player(InputValue::string_value("weapon_assault_rifle"));
    rt.input_set_primary_ammo_count_to_throw(InputValue::integer_value(12));
    assert(rt.select_schedule()==0x59);
    assert(rt.on_animation_event(201)==AnimationResult::Launched);
    assert(h.calls==std::vector<const char*>(
        {"set","clear","spawn","ammo","observe","type","dirty","read","write"}));
    assert(h.velocity_set&&!h.legacy_called&&!h.condition&&h.velocity.z>h.starting_velocity.z);
    assert(h.velocity.x>h.starting_velocity.x && h.velocity.y==h.starting_velocity.y);
    h.calls.clear();h.velocity_set=false;h.invalid_gravity=true;
    assert(rt.on_animation_event(201)==AnimationResult::LaunchRejected);
    assert(h.calls==std::vector<const char*>({"clear","spawn","observe"}));
    assert(!h.velocity_set&&!h.legacy_called);
    h.invalid_gravity=false;h.use_physics=true;h.calls.clear();
    assert(rt.on_animation_event(201)==AnimationResult::Launched);
    assert(h.calls==std::vector<const char*>(
        {"clear","spawn","observe","type","physics"}));
    assert(h.physics_added.x>0 && h.physics_added.z>0);
    assert(!h.legacy_called && !h.velocity_set);
    std::cout<<"Original Jessica toss physics-host impulse integration PASS\n";
}
