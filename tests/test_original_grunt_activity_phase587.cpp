#include "sin_compat/original_grunt_activity_phase587.hpp"
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
using namespace sin_compat::grunt_activity_phase587;

struct FakeEngine : Engine {
  bool internal=false,weapon=false,registry=true;
  std::uint32_t caps=0;
  int delegated_output=99, next_718=88, next_714=77, weapon_present=900, weapon_absent=901;
  int delegate_calls=0, identity_calls=0, inherited_calls=0, weapon_calls=0, route_714_calls=0;
  std::vector<std::string> trace;
  bool internal_component_identity() override {++identity_calls;trace.push_back("identity");return internal;}
  int component_translate(int a) override {++delegate_calls;trace.push_back("delegate:"+std::to_string(a));return delegated_output;}
  std::uint32_t inherited_capability_bits() override {++inherited_calls;trace.push_back("capability");return caps;}
  int virtual_718_with_target(std::optional<int> target) override {trace.push_back(target?"target":"null_target");return next_718;}
  int original_22039130() override {trace.push_back("probe");return 55;}
  int virtual_714(int a) override {++route_714_calls;assert(a==55);trace.push_back("virtual714");return next_714;}
  bool registered_activity(int x) override {assert(x==next_714);trace.push_back("lookup");return registry;}
  bool has_grenade_launcher() override {++weapon_calls;trace.push_back("weapon");return weapon;}
  int runtime_activity_for_weapon(bool found) override {trace.push_back("runtime");return found?weapon_present:weapon_absent;}
};
struct FakeTarget : TargetTranslator {
  int calls=0,entity=-1,requested=-1;
  int virtual_47c(int ent,int req,bool* flag) override {++calls;entity=ent;requested=req;if(flag)*flag=true;return req+100;}
};
struct FakeMatcher : WeaponMatcher {
  int matched=0;
  bool original_class_match(int,const std::string& name) override {++matched;return name=="weapon_grenadelauncher";}
  int original_owner_tag(int entity) override {return entity==123?0:1;}
};
int main(){
  // The ACT names were verified against all 921 original activity registrations.
  assert(ACT_WALK==6&&ACT_RUN==10&&ACT_RANGE_ATTACK1==16&&ACT_RANGE_ATTACK2==17);
  assert(ACT_RANGE_ATTACK1_LOW==18&&ACT_RELOAD==66&&ACT_RELOAD_LOW==67);
  assert(ACT_ON_KNEES==196&&ACT_SHOOT_ON_KNEES==199&&ACT_RELOAD_ON_KNEES==200);
  for(int input:{ACT_RANGE_ATTACK1,ACT_RELOAD,ACT_IDLE}){
    GruntState s;FakeEngine e;
    int expected=input==ACT_RANGE_ATTACK1?ACT_RANGE_ATTACK1_LOW:input==ACT_RELOAD?ACT_RELOAD_LOW:ACT_CROUCHIDLE;
    assert(grunt_activity_338(s,input,e)==expected);
    assert(e.identity_calls==1&&e.inherited_calls==1);
    GruntState kneeling;kneeling.raw_ca0=true;FakeEngine f;
    expected=input==ACT_RANGE_ATTACK1?ACT_SHOOT_ON_KNEES:input==ACT_RELOAD?ACT_RELOAD_ON_KNEES:ACT_ON_KNEES;
    assert(grunt_activity_338(kneeling,input,f)==expected);
  }
  // Once movement marks raw +0xC64, subsequent low attack/reload variants
  // are no longer chosen by the +C64==0 test.
  {
    GruntState s;FakeEngine e;
    assert(grunt_activity_338(s,ACT_RUN,e)==ACT_RUN);assert(s.raw_c64);
    assert(grunt_activity_338(s,ACT_RELOAD,e)==ACT_RELOAD);
    assert(grunt_activity_338(s,ACT_RANGE_ATTACK1,e)==ACT_RANGE_ATTACK1);
    s.raw_850=3;assert(grunt_activity_338(s,ACT_IDLE,e)==ACT_IDLE_ANGRY);
    s.raw_850=2;assert(grunt_activity_338(s,ACT_IDLE,e)==ACT_IDLE_ANGRY);
    s.raw_850=1;assert(grunt_activity_338(s,ACT_IDLE,e)==ACT_IDLE);
  }
  // The component identity branch is AFTER Grunt-specific mapping, but
  // BEFORE any component or inherited delegate.
  {
    GruntState s;s.raw_c64=true;FakeEngine e;e.internal=true;
    assert(grunt_activity_338(s,ACT_WALK,e)==ACT_WALK_AIM);
    assert(grunt_activity_338(s,ACT_RUN,e)==ACT_RUN_AIM);
    assert(grunt_activity_338(s,ACT_IDLE,e)==ACT_IDLE_ANGRY);
    s.has_c18=true;e.delegated_output=123;
    assert(grunt_activity_338(s,ACT_RUN,e)==123);
    assert(e.trace.back()=="delegate:11");
    assert(e.delegate_calls==1);
  }
  // Class-specific weapon activity exits before any component identity or
  // fallback, as the original ret 4 at 0x2235A2D4 / 0x2235A2DE does.
  {
    GruntState s;FakeEngine e;
    assert(grunt_activity_338(s,ACT_RANGE_ATTACK2,e)==901);
    e.weapon=true;assert(grunt_activity_338(s,ACT_RANGE_ATTACK2,e)==900);
    assert(e.identity_calls==0&&e.inherited_calls==0&&e.delegate_calls==0);
    assert(e.weapon_calls==2);
  }
  // Original inherited activity behavior is conditional on bit 0x08000000.
  {
    GruntState s;s.raw_c64=true;FakeEngine e;
    e.caps=0x08000000u;
    assert(grunt_activity_338(s,ACT_RELOAD,e,12)==88);
    assert(e.trace.back()=="target");
    e.trace.clear();assert(grunt_activity_338(s,ACT_RELOAD,e)==88);
    assert(e.trace.back()=="null_target");
    s.raw_988_bit1=true;
    assert(grunt_activity_338(s,ACT_IDLE,e)==77);
    assert(e.route_714_calls==1);
    e.registry=false;assert(grunt_activity_338(s,ACT_IDLE,e)==ACT_IDLE);
    s.raw_988_bit1=false;assert(grunt_activity_338(s,ACT_IDLE,e)==ACT_IDLE);
    assert(grunt_activity_338(s,3,e)==ACT_IDLE); // original code falls back to 1
    e.caps=0;assert(grunt_activity_338(s,ACT_RELOAD,e)==ACT_RELOAD);
  }
  // Raw 12-bit index and upper 20-bit serial identity, unchanged request
  // on stale/missing target plus output flag reset.
  {
    SerialTable t;t.slots.resize(3);t.slots[1]={7,123,true};
    FakeTarget target;bool flag=true;
    assert(grunt_activity_33c(0x00007001u,t,10,&flag,target)==110);
    assert(flag&&target.calls==1&&target.entity==123&&target.requested==10);
    flag=true;assert(grunt_activity_33c(0x00008001u,t,10,&flag,target)==10);
    assert(!flag&&target.calls==1);
    flag=true;assert(grunt_activity_33c(0xFFFFFFFFu,t,3,&flag,target)==3&&!flag);
    assert(grunt_activity_33c(0x00007001u,t,1,nullptr,target)==101);
    assert(grunt_activity_33c(0x00007004u,t,1,nullptr,target)==1);
  }
  // +0x6CC: stop at 48 original handles, require serial resolution and
  // virtual+0x2E4 owner tag match; fallback classname comparison injected.
  {
    SerialTable t;t.slots.resize(52);t.slots[1]={3,123,true};t.slots[2]={4,456,true};
    std::vector<WeaponCandidate> candidates{{(3u<<12)|1u,"weapon_grenadelauncher",0}};
    FakeMatcher m;
    assert(find_weapon_222aa840(t,candidates,"weapon_grenadelauncher",0,m)==123);
    assert(!find_weapon_222aa840(t,candidates,"weapon_grenadelauncher",1,m).has_value());
    candidates.insert(candidates.begin(),{(6u<<12)|2u,"weapon_grenadelauncher",0}); // stale serial
    assert(find_weapon_222aa840(t,candidates,"weapon_grenadelauncher",0,m)==123);
    std::vector<WeaponCandidate> far(49,{0xFFFFFFFFu,"",0});
    far[48]={(3u<<12)|1u,"weapon_grenadelauncher",0};
    assert(!find_weapon_222aa840(t,far,"weapon_grenadelauncher",0,m).has_value());
    assert(m.matched==0);
  }
}