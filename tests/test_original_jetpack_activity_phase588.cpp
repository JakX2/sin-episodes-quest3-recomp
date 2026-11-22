#include "sin_compat/original_jetpack_activity_phase588.hpp"
#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>
using namespace sin_compat::jetpack_activity_phase588;
namespace gr = sin_compat::grunt_activity_phase587;

struct EngineHost final : Engine {
  int identity_queries=0,translated_calls=0,weapon_calls=0,capability_queries=0;
  bool internal_component_identity() override {++identity_queries;return false;}
  int component_translate(int activity) override {++translated_calls;return activity;}
  std::uint32_t inherited_capability_bits() override {++capability_queries;return 0;}
  int virtual_718_with_target(std::optional<int>) override {return 66;}
  int original_22039130() override {return 0;}
  int virtual_714(int a) override {return a;}
  bool registered_activity(int) override {return false;}
  bool has_grenade_launcher() override {++weapon_calls;return false;}
  int runtime_activity_for_weapon(bool) override {return 777;}
};

int main() {
  assert(ACT_JETPACK_IDLE==208&&ACT_JETPACK_MOVE==213&&ACT_JETPACK_RELOAD==218);
  GruntState g;EngineHost h;
  for(int input:{ACT_IDLE,ACT_WALK_AIM,ACT_RANGE_ATTACK1}) {
   auto z=jetpack_activity_338(g,input,Vec3{3,4,0},h);
   assert(z.status==ResultStatus::Selected&&!z.delegated&&z.activity==ACT_JETPACK_IDLE);
   auto eq=jetpack_activity_338(g,input,Vec3{30,40,0},h);
   assert(eq.activity==ACT_JETPACK_IDLE); // original strict >50 branch
   auto gt=jetpack_activity_338(g,input,Vec3{30,40,1},h);
   assert(gt.activity==ACT_JETPACK_MOVE&&!gt.delegated);
  }
  assert(h.identity_queries==0&&h.capability_queries==0);
  auto r=jetpack_activity_338(g,ACT_RELOAD,Vec3{0,0,0},h);
  assert(r.activity==ACT_JETPACK_RELOAD&&!r.delegated);
  assert(h.identity_queries==0);
  // 62 other original in-range ACT IDs all fall back to the older Grunt
  // converter after a mandatory vector magnitude read.
  auto base=jetpack_activity_338(g,gr::ACT_WALK,Vec3{1,0,0},h);
  assert(base.status==ResultStatus::Selected&&base.delegated);
  assert(base.activity==gr::ACT_WALK&&g.raw_c64);
  assert(h.identity_queries==1&&h.capability_queries==1);
  auto base2=jetpack_activity_338(g,gr::ACT_RANGE_ATTACK2,Vec3{1,0,0},h);
  assert(base2.activity==777&&base2.delegated&&h.weapon_calls==1);
  // Native method unconditionally loads attached entity floats before
  // dispatch. Host avoids dereferencing stale +0x1024 generation IDs.
  g.raw_c64=false;auto noattach=jetpack_activity_338(g,ACT_RELOAD,std::nullopt,h);
  assert(noattach.status==ResultStatus::MissingNativeAttachment&&!noattach.delegated);
  assert(!g.raw_c64&&h.weapon_calls==1);
  auto notfinite=jetpack_activity_338(g,ACT_IDLE,
     Vec3{std::numeric_limits<float>::quiet_NaN(),0,0},h);
  assert(notfinite.status==ResultStatus::ExceptionalNativeFloatNotModeled);
  auto other=jetpack_activity_338(g,170,Vec3{0,0,0},h);
  assert(other.activity==170&&other.delegated);
}