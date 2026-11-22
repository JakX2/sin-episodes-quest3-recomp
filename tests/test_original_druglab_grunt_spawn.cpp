#include "sin_compat/original_druglab_grunt_spawn.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using sin_compat::original_druglab_grunt::Variant;
using namespace sin_compat::original_druglab_grunt::spawn;

struct FakeNativeGrunt final:IHost {
    std::vector<std::string> events;
    std::string new_weapon;
    std::int32_t mode=999,field98=999,field9c=999;
    DirtyPath dirty=DirtyPath::NoNetworkMarker;
    Stage refuse_stage=Stage::None;
    Stage throw_stage=Stage::None;
    SpawnReference* reentry=nullptr;
    Code reentrant_result=Code::Applied;
    Snapshot nested{};
    bool stage(Stage stage,std::string_view name) {
        events.emplace_back(name);
        if(throw_stage==stage)throw std::runtime_error("synthetic host failure");
        return refuse_stage!=stage;
    }
    bool assign_original_weapon_symbol(std::string_view n) override {
        if(!stage(Stage::WeaponSymbol,"weapon"))return false;
        new_weapon=std::string(n);
        return true;
    }
    bool commit_original_field_3B0(
        std::int32_t new_mode,DirtyPath how) override {
        if(!stage(Stage::Field3B0,"mode"))return false;
        mode=new_mode;dirty=how;
        return true;
    }
    bool native_virtual_68() override {
        const bool accepted=stage(Stage::Virtual68,"virtual68");
        if(reentry)reentrant_result=reentry->apply(nested).code;
        return accepted;
    }
    bool native_virtual_1C_then_6C() override {
        return stage(Stage::Virtual1CThen6C,"virtual1c-virtual6c");
    }
    bool inherited_npc_spawn_2235F2A0() override {
        return stage(Stage::InheritedSpawn,"inherited-spawn");
    }
    bool virtual_1B0_then_write_field_98(std::int32_t value) override {
        if(!stage(Stage::Field98,"virtual1b0-field98"))return false;
        field98=value;return true;
    }
    bool virtual_1B4_then_write_field_9C(std::int32_t value) override {
        if(!stage(Stage::Field9C,"virtual1b4-field9c"))return false;
        field9c=value;return true;
    }
};
void test_original_three_classnames_and_weapons() {
    struct Case {
        std::string_view classname,weapon;
        std::int32_t desired_mode;
        Variant variant;
    };
    constexpr Case cases[]{
        {"npc_druglab_grunt_pistol","weapon_npc_pistol",1,Variant::Pistol},
        {"npc_druglab_grunt_scattergun","weapon_scattergun",0,Variant::Scattergun},
        {"npc_druglab_grunt_assault_rifle","weapon_assault_rifle",2,Variant::AssaultRifle}
    };
    for(const auto& c:cases){
        FakeNativeGrunt host;
        SpawnReference runtime(host);
        Snapshot in{};
        in.classname=c.classname;
        in.weapon_name="weapon_wrong";
        in.original_field_3B0=9;
        in.original_field_98=0;
        in.original_field_9C=0;
        in.original_edict_16C_present=true;
        in.variant_float_28=3.9f;
        const auto r=runtime.apply(in);
        assert(r.ok()&&r.plan.variant==c.variant);
        assert(r.plan.desired_weapon==c.weapon &&
               host.new_weapon==c.weapon);
        assert(r.plan.desired_field_3B0==c.desired_mode &&
               host.mode==c.desired_mode);
        assert(r.plan.truncated_global_integer==3 &&
               host.field98==3&&host.field9c==3);
        assert(host.dirty==DirtyPath::EdictStateBit0);
        assert((host.events==std::vector<std::string>{
            "weapon","mode","virtual68","virtual1c-virtual6c",
            "inherited-spawn","virtual1b0-field98","virtual1b4-field9c"
        }));
    }
}
void test_retail_default_grunt_has_only_inherited_spawn() {
    FakeNativeGrunt host;
    SpawnReference runtime(host);
    Snapshot in{};
    in.classname="npc_druglab_grunt";
    in.weapon_name="weapon_existing";
    in.original_field_3B0=77;
    in.original_field_98=19;
    in.original_field_9C=23;
    in.original_flag_1F0=true;
    in.original_edict_16C_present=true;
    // Unused for the base classname: the original Spawn does not touch
    // per-variant ConVar +0x28 or numeric fields on this branch.
    in.variant_float_28=std::numeric_limits<float>::quiet_NaN();
    const auto r=runtime.apply(in);
    assert(r.ok() && r.plan.variant==Variant::Base);
    assert(r.plan.desired_weapon.empty());
    assert(r.plan.desired_field_3B0==77);
    assert(!r.plan.overwrite_weapon && !r.plan.update_field_3B0 &&
           !r.plan.update_field_98 && !r.plan.update_field_9C);
    assert(host.new_weapon.empty());
    assert(host.mode==999 && host.field98==999 && host.field9c==999);
    assert((host.events==std::vector<std::string>{
        "virtual68","virtual1c-virtual6c","inherited-spawn"
    }));
    // Case-sensitive, literal-alias safe: do not admit a lookalike classname.
    assert(runtime.apply({"npc_druglab_grunt_extra","",0,0,0,
                          false,false,0.f}).code==Code::UnsupportedClass);
}
void test_original_existing_weapon_and_matching_fields() {
    FakeNativeGrunt host;
    SpawnReference runtime(host);
    Snapshot in{};
    in.classname="npc_druglab_grunt_scattergun";
    in.weapon_name="weapon_scattergun";
    in.original_field_3B0=0;
    in.original_field_98=8;
    in.original_field_9C=8;
    in.variant_float_28=8.5f;
    const auto r=runtime.apply(in);
    assert(r.ok());
    assert(!r.plan.overwrite_weapon && !r.plan.update_field_3B0 &&
           !r.plan.update_field_98 && !r.plan.update_field_9C);
    assert((host.events==std::vector<std::string>{
        "virtual68","virtual1c-virtual6c","inherited-spawn"
    }));
}
void test_original_dirty_flag_conditions() {
    for(int case_id=0;case_id<4;++case_id){
        FakeNativeGrunt host;
        SpawnReference runtime(host);
        Snapshot in{};
        in.classname="npc_druglab_grunt_assault_rifle";
        in.weapon_name="weapon_assault_rifle";
        in.original_field_3B0=1;
        in.original_flag_1F0=case_id==0||case_id==3;
        in.original_edict_16C_present=case_id==1||case_id==3;
        in.original_field_98=-2;
        in.original_field_9C=-2;
        in.variant_float_28=-2.8f;
        const auto r=runtime.apply(in);
        assert(r.ok());
        assert(r.plan.truncated_global_integer==-2);
        assert(!r.plan.overwrite_weapon &&
               r.plan.update_field_3B0 &&
               !r.plan.update_field_98 && !r.plan.update_field_9C);
        const auto expected=(case_id==0||case_id==3)?
            DirtyPath::NativeField200:(case_id==1)?
            DirtyPath::EdictStateBit0:DirtyPath::NoNetworkMarker;
        assert(host.dirty==expected);
        assert((host.events==std::vector<std::string>{
            "mode","virtual68","virtual1c-virtual6c","inherited-spawn"
        }));
    }
}
void test_strict_source_int32_conversion_boundaries() {
    const Snapshot origin{
        "npc_druglab_grunt_pistol","weapon_npc_pistol",1,0,0,false,false,0
    };
    for(float value:{
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),1e30f,
        -1e30f,2147483648.0f
    }){
        FakeNativeGrunt host;
        SpawnReference runtime(host);
        Snapshot in=origin;in.variant_float_28=value;
        Plan out{};
        assert(plan_spawn(in,out)==Code::NonFiniteOrOutOfRangeGlobal);
        assert(runtime.apply(in).code==Code::NonFiniteOrOutOfRangeGlobal);
        assert(host.events.empty());
    }
    {
        FakeNativeGrunt host;
        SpawnReference runtime(host);
        Snapshot in=origin;in.variant_float_28=-2147483648.0f;
        assert(runtime.apply(in).ok());
        assert(host.field98==std::numeric_limits<std::int32_t>::min());
        assert(host.field9c==std::numeric_limits<std::int32_t>::min());
    }
    {
        FakeNativeGrunt host;
        SpawnReference runtime(host);
        Snapshot in=origin;in.variant_float_28=2147483520.0f;
        assert(runtime.apply(in).ok());
        assert(host.field98==2147483520&&host.field9c==2147483520);
    }
}
void test_no_fabricated_npc_unsupported() {
    FakeNativeGrunt host;
    SpawnReference runtime(host);
    for(std::string_view classname:{
        "npc_druglab_grunt_elite","npc_druglab_jetpack",
        "npc_sintek_grunt","npc_securitybot","NPC_DRUGLAB_GRUNT_PISTOL",
        "npc_druglab_grunt_pistol_extra",""
    }) {
        const auto r=runtime.apply({classname,"",0,0,0,false,false,1.0f});
        assert(r.code==Code::UnsupportedClass);
    }
    assert(host.events.empty());
}
void test_original_failure_stage_does_not_claim_success_or_rollback() {
    struct Case {
        Stage bad_stage;
        bool force_weapon,force_mode,force98,force9c;
    };
    constexpr Case cases[]{
        {Stage::WeaponSymbol,true,true,true,true},
        {Stage::Field3B0,true,true,true,true},
        {Stage::Virtual68,true,true,true,true},
        {Stage::Virtual1CThen6C,true,true,true,true},
        {Stage::InheritedSpawn,true,true,true,true},
        {Stage::Field98,true,true,true,true},
        {Stage::Field9C,true,true,true,true}
    };
    for(const auto& c:cases){
        assert(c.force_weapon&&c.force_mode&&c.force98&&c.force9c);
        FakeNativeGrunt host;
        host.refuse_stage=c.bad_stage;
        SpawnReference runtime(host);
        const auto r=runtime.apply({
            "npc_druglab_grunt_pistol","weapon_wrong",5,0,0,
            false,true,4.0f
        });
        assert(r.code==Code::HostRejected&&r.failed_stage==c.bad_stage);
        assert(!host.events.empty());
        if(c.bad_stage==Stage::WeaponSymbol)assert(host.events.size()==1);
        if(c.bad_stage==Stage::Field98)
            assert(host.field98==999&&host.field9c==999);
        if(c.bad_stage==Stage::Field9C)
            assert(host.field98==4&&host.field9c==999);
    }
    FakeNativeGrunt host;
    host.throw_stage=Stage::InheritedSpawn;
    SpawnReference runtime(host);
    const auto r=runtime.apply({
        "npc_druglab_grunt_pistol","weapon_wrong",5,0,0,
        false,true,4.0f
    });
    assert(r.code==Code::HostRejected&&r.failed_stage==Stage::InheritedSpawn);
}
void test_reentrant_spawn_is_refused_before_host_mutations() {
    FakeNativeGrunt host;
    SpawnReference runtime(host);
    host.reentry=&runtime;
    host.nested={"npc_druglab_grunt_pistol","wrong",0,0,0,
                 false,false,1.0f};
    const auto out=runtime.apply({
        "npc_druglab_grunt_pistol","weapon_npc_pistol",
        1,1,1,false,false,1.0f
    });
    assert(out.ok());
    assert(host.reentrant_result==Code::Reentrant);
    assert((host.events==std::vector<std::string>{
        "virtual68","virtual1c-virtual6c","inherited-spawn"
    }));
}
int main() {
    test_original_three_classnames_and_weapons();
    test_retail_default_grunt_has_only_inherited_spawn();
    test_original_existing_weapon_and_matching_fields();
    test_original_dirty_flag_conditions();
    test_strict_source_int32_conversion_boundaries();
    test_no_fabricated_npc_unsupported();
    test_original_failure_stage_does_not_claim_success_or_rollback();
    test_reentrant_spawn_is_refused_before_host_mutations();
    std::puts("PASS original SiN Druglab Grunt Spawn: exact class weapons, field3b0 modes, Source dirty paths, x87 truncation and inherited NPC spawn ordering (mock native host)");
}
