#include "sin_compat/difficulty_metric_consumers.hpp"
#include <cmath>
#include <cstdint>
#include <cstdlib>
using namespace sin_compat::difficulty;
static void require(bool x) { if (!x) std::abort(); }
static bool near(float a,float b) { return std::fabs(a-b) < 0.00001f; }
int main() {
  auto easy=metrics_for_blend_input(0.0f);
  auto hard=metrics_for_blend_input(1.0f);
  OriginalDamageFields info{100.0f,0};
  apply_damage_taken_metric(info,easy);
  require(near(info.damage,75.0f));
  info={100.0f,0x80024021u};
  apply_damage_taken_metric(info,hard);
  require(near(info.damage,100.0f));
  info={100.0f,2u};
  apply_damage_taken_metric(info,hard);
  require(near(info.damage,140.0f));
  auto m=hard;
  m[1]=1.25f;m[2]=0.75f;m[11]=-0.1f;
  require(near(scaled_damage_done(80.0f,m),100.0f));
  require(near(ammo_quantity_multiplier(m),0.75f));
  require(near(npc_random_wait_before_shoot(0.5f,m),0.4f));
}
