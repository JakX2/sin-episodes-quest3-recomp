#include "sin_compat/original_ar_grenade_runtime.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

using namespace sin_compat::ar_grenade;

struct RecordingHost final : IHost {
    std::vector<std::string> order{};
    AiSoundEvent danger{};
    AiSoundEvent combat{};
    BlastDamageEvent blast{};
    ScreenShakeEvent shake{};
    Decal selected=Decal::None;
    int flight_count=0;
    int damage_count=0;
    int remove_count=0;
    Runtime* reentrant=nullptr;
    StepResult recursive_result=StepResult::Applied;

    void on_flight_ai_sound(const AiSoundEvent& e) override {
        order.emplace_back("danger");
        danger=e;
        ++flight_count;
    }
    void on_original_trace_effect(Decal d) override {
        order.emplace_back("trace");
        selected=d;
    }
    void on_screen_shake_feedback(const ScreenShakeEvent& s) override {
        order.emplace_back("shake");
        shake=s;
    }
    void on_radius_damage(const BlastDamageEvent& d) override {
        order.emplace_back("damage");
        blast=d;
        ++damage_count;
        if (reentrant) {
            recursive_result=reentrant->detonate(true,Decal::None,*this);
        }
    }
    void on_explosion_ai_sound(const AiSoundEvent& s) override {
        order.emplace_back("combat");
        combat=s;
    }
    void on_remove_projectile() override {
        order.emplace_back("remove");
        ++remove_count;
    }
};

int main() {
    static_assert(original_initial_damage==100.0f);
    static_assert(original_initial_blast_radius==250.0f);
    static_assert(original_initial_danger_radius==100);
    static_assert(original_danger_radius_increment==15);
    static_assert(original_danger_radius_comparison==300);
    static_assert(original_ai_sound_danger==0x8u);
    static_assert(original_ai_sound_explosion==0x02000001u);
    static_assert(original_damage_type_blast==0x40u);

    Runtime grenade{};
    RecordingHost host{};
    assert(!grenade.armed() && !grenade.spawned());
    assert(grenade.think(0.0f,true,true,host)==StepResult::Disarmed);
    assert(grenade.detonate(true,Decal::Scorch,host)==StepResult::Disarmed);
    assert(host.order.empty());

    assert(grenade.spawn({10,20,30},{4,-8,10},2.0f));
    assert(grenade.armed() && grenade.spawned());
    assert(grenade.next_think_seconds()>2.099f);
    assert(grenade.next_think_seconds()<2.101f);
    assert(grenade.damage()==100 && grenade.blast_radius()==250);
    assert(grenade.think(2.09f,true,true,host)==StepResult::NotDue);
    assert(host.flight_count==0 && grenade.danger_radius()==100);
    assert(grenade.think(2.1f,true,true,host)==StepResult::Applied);
    assert(host.flight_count==1 && grenade.danger_radius()==115);
    assert(host.danger.origin.x==12 && host.danger.origin.y==16 &&
           host.danger.origin.z==35);
    assert(host.danger.type==original_ai_sound_danger);
    assert(host.danger.duration_seconds==0.2f);
    assert(host.danger.channel==1 && host.danger.radius==115);

    // Original stimulus emission and radius growth use separate branch
    // decisions. A quiet think still reschedules the projectile.
    assert(grenade.think(2.16f,false,false,host)==StepResult::Applied);
    assert(host.flight_count==1 && grenade.danger_radius()==115);
    assert(grenade.update_kinematics({0,0,0},{10,20,30}));
    assert(grenade.think(2.22f,true,false,host)==StepResult::Applied);
    assert(host.flight_count==2 && grenade.danger_radius()==115);
    assert(host.danger.origin.x==5 && host.danger.origin.y==10 &&
           host.danger.origin.z==15);

    const auto old_time=grenade.next_think_seconds();
    const auto old_danger=grenade.danger_radius();
    const float nan=std::numeric_limits<float>::quiet_NaN();
    const float inf=std::numeric_limits<float>::infinity();
    assert(grenade.think(nan,true,true,host)==StepResult::InvalidInput);
    assert(grenade.think(inf,true,true,host)==StepResult::InvalidInput);
    assert(grenade.next_think_seconds()==old_time);
    assert(grenade.danger_radius()==old_danger);
    assert(!grenade.update_kinematics({nan,0,0},{1,1,1}));
    assert(!grenade.update_kinematics({0,0,0},{inf,0,0}));
    assert(!grenade.set_runtime_damage(nan,250));
    assert(!grenade.set_runtime_damage(100,-1));
    assert(!grenade.set_runtime_damage(100,std::numeric_limits<float>::max()));
    assert(grenade.damage()==100 && grenade.blast_radius()==250);

    assert(grenade.detonate(false,Decal::Scorch,host)==StepResult::TriggerRejected);
    assert(grenade.armed() && host.damage_count==0);
    assert(grenade.detonate(true,static_cast<Decal>(99),host)==
           StepResult::InvalidInput);
    assert(grenade.armed() && host.damage_count==0);
    assert(grenade.detonate(true,Decal::Scorch,host)==StepResult::Applied);
    assert(!grenade.armed());
    assert(host.selected==Decal::Scorch);
    assert(host.blast.origin.x==0 && host.blast.origin.y==0 &&
           host.blast.origin.z==0);
    assert(host.blast.damage==100 && host.blast.radius==250);
    assert(host.blast.damage_type==0x40u);
    assert(host.shake.requested_amplitude==25);
    assert(host.shake.engine_clamped_amplitude==16);
    assert(host.shake.frequency==150 && host.shake.duration_seconds==1);
    assert(host.shake.radius==750 && !host.shake.move_tracked_hmd);
    assert(host.combat.type==0x02000001u && host.combat.radius==375);
    assert(host.combat.duration_seconds==1.0f && host.combat.channel==0);
    assert(host.remove_count==1 && host.damage_count==1);
    assert(host.order==std::vector<std::string>({
        "danger","danger","trace","shake","damage","combat","remove"
    }));
    assert(grenade.detonate(true,Decal::SmallScorch,host)==StepResult::Disarmed);
    assert(grenade.think(10,true,true,host)==StepResult::Disarmed);
    assert(!grenade.update_kinematics({0,0,0},{0,0,0}));
    assert(host.remove_count==1 && host.damage_count==1);

    // 300 is a COMPARISON, not a hard upper clamp. 100 + 14*15 = 310.
    RecordingHost radius_host{};
    assert(grenade.spawn({0,0,0},{0,0,0},0.0f));
    for(int i=0;i<14;++i) {
        const float t=grenade.next_think_seconds();
        assert(grenade.think(t,false,true,radius_host)==StepResult::Applied);
    }
    assert(grenade.danger_radius()==310);
    assert(grenade.think(grenade.next_think_seconds(),true,true,radius_host)==
           StepResult::Applied);
    assert(grenade.danger_radius()==310);
    assert(radius_host.danger.radius==310);

    // An independent original damage/skill calculation can set runtime
    // values. Never confuse 250 blast damage radius with danger AI radius.
    assert(grenade.set_runtime_damage(20,50));
    assert(grenade.detonate(true,Decal::SmallScorch,radius_host)==StepResult::Applied);
    assert(radius_host.blast.damage==20 && radius_host.blast.radius==50);
    assert(radius_host.combat.radius==128);
    assert(radius_host.selected==Decal::SmallScorch);

    Runtime reenter{};
    RecordingHost reentrant_host{};
    assert(reenter.spawn({0,0,0},{0,0,0},1.0f));
    reentrant_host.reentrant=&reenter;
    assert(reenter.detonate(true,Decal::None,reentrant_host)==StepResult::Applied);
    assert(reentrant_host.recursive_result==StepResult::Disarmed);
    assert(reentrant_host.damage_count==1 && reentrant_host.remove_count==1);

    Runtime invalid{};
    assert(!invalid.spawn({inf,0,0},{0,0,0},1));
    assert(!invalid.spawn({0,0,0},{nan,0,0},1));
    assert(!invalid.spawn({0,0,0},{0,0,0},inf));
    assert(!invalid.spawn({0,0,0},{0,0,0},std::numeric_limits<float>::max()));
    assert(!invalid.spawned());
    return 0;
}
