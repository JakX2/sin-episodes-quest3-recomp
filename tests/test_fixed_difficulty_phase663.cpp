#include "sin_compat/fixed_difficulty_phase663.hpp"
#include "sin_compat/difficulty_state_archive_contract.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>

using namespace sin_compat::difficulty;
namespace f=sin_compat::difficulty::fixed_phase663;
namespace {
bool eq(float a,float b) {return std::fabs(a-b)<0.00001f;}
bool same(const RuntimeMetrics& a,const RuntimeMetrics& b) {
 for(std::size_t i=0;i<original_metric_count;++i)
  if(a[i]!=b[i])return false;
 return true;
}
void profile_invariants(){
 static_assert(original_metric_count==14,"original 14 GamePlayVarValue names");
 assert(f::kEngineSkillConVar=="skill");
 assert(f::kOriginalAdaptiveGateConVar=="dd_enabled");
 assert(f::valid_skill(1)&&f::valid_skill(2)&&f::valid_skill(3));
 for(const int bad:{-1,0,4,999,std::numeric_limits<int>::max()})
  assert(!f::valid_skill(bad));
 assert(f::level_for_skill(1)==f::Level::Easy);
 assert(f::level_for_skill(3)==f::Level::Hard);
 assert(f::level_for_skill(999)==f::Level::Normal);
 const auto easy=f::make_profile(f::Level::Easy);
 const auto normal=f::make_profile(f::Level::Normal);
 const auto hard=f::make_profile(f::Level::Hard);
 assert(easy.label=="Easy"&&normal.label=="Normal"&&hard.label=="Hard");
 const auto original=original_initial_metric_values();
 assert(same(normal.metrics,original));
 // Phase677: metric12 multiplies NPC spread, not hit probability.
 // Larger spread on Easy and smaller spread on Hard.
 const std::array<float,7> em{{0.75f,0.5f,0.f,0.1f,0.1f,1.f,0.3f}};
 const std::array<float,7> nm{{1.075f,0.4f,0.25f,0.35000002f,0.f,0.75f,0.5f}};
 const std::array<float,7> hm{{1.4f,0.3f,0.5f,0.6f,-0.1f,0.5f,0.7f}};
 constexpr std::array<std::size_t,7> indexes{{0u,4u,5u,8u,11u,12u,13u}};
 for(std::size_t i=0u;i<indexes.size();++i){
  assert(eq(easy.metrics[indexes[i]],em[i]));
  assert(eq(normal.metrics[indexes[i]],nm[i]));
  assert(eq(hard.metrics[indexes[i]],hm[i]));
 }
 // No fabricated probability interpretation for spawn/different-NPC,
 // helmet-shot-off or unknown original metric fields 3..6.
 constexpr std::array<std::size_t,7> unchanged{{1u,2u,3u,6u,7u,9u,10u}};
 for(const auto id:unchanged){
  assert(easy.metrics[id]==original[id]);
  assert(normal.metrics[id]==original[id]);
  assert(hard.metrics[id]==original[id]);
 }
 // Original endpoints are lighter damage and wider NPC spread at t=0.
 // Fixed profiles remain deliberately independent of adaptive updates.
 const auto old_low=blended_metrics(0.f);
 const auto old_high=blended_metrics(1.f);
 assert(old_low[0]<old_high[0]&&old_low[12]>old_high[12]);
 assert(easy.metrics[12]>hard.metrics[12]);
 assert(easy.metrics[0]<hard.metrics[0]);
}
void stable_skill_changes_and_lookup(){
 f::FixedDifficultyRuntime mode;
 assert(mode.skill()==2&&mode.name()=="Normal");
 assert(!mode.adaptive_updates_enabled());
 assert(mode.revision()==0u);
 assert(same(mode.metrics(),original_initial_metric_values()));
 float value=55.f;
 assert(mode.lookup("DAMAGE_TAKEN_SCALE",value)&&eq(value,1.075f));
 assert(mode.lookup("PERCENT_NPC_ACCURATE",value)&&eq(value,0.75f));
 assert(!mode.lookup("damage_taken_scale",value)&&eq(value,0.75f));
 assert(!mode.lookup("unknown",value)&&eq(value,0.75f));
 assert(std::isnan(mode.metric_at(14)));
 assert(std::isnan(mode.metric_at(static_cast<std::size_t>(-1))));
 const auto original=mode.metrics();
 for(const int invalid:{-1,0,4,99}){
  assert(mode.set_skill(invalid)==f::Selection::InvalidSkill);
  assert(same(mode.metrics(),original)&&mode.revision()==0u);
 }
 assert(mode.set_skill(2)==f::Selection::Unchanged);
 assert(mode.set_skill(1)==f::Selection::Applied);
 assert(mode.skill()==1&&mode.name()=="Easy"&&mode.revision()==1u);
 assert(mode.lookup("DAMAGE_TAKEN_SCALE",value)&&eq(value,0.75f));
 assert(mode.set_skill(1)==f::Selection::Unchanged);
 assert(mode.revision()==1u);
 assert(mode.set_skill(3)==f::Selection::Applied);
 assert(mode.skill()==3&&mode.name()=="Hard"&&mode.revision()==2u);
 assert(mode.lookup("DAMAGE_TAKEN_SCALE",value)&&eq(value,1.4f));
}
void no_adaptation_on_trigger_or_performance(){
 f::FixedDifficultyRuntime mode;
 assert(mode.set_skill(3)==f::Selection::Applied);
 const auto fixed=mode.metrics();
 mode.observe_performance_sample(0.f);
 mode.observe_performance_sample(999999.f);
 mode.observe_performance_sample(-999.f);
 assert(same(mode.metrics(),fixed));
 assert(mode.skill()==3&&mode.revision()==1u);
 mode.observe_dynamic_trigger({1,27.f,256});
 assert(mode.trigger_count()==1u&&!mode.playthrough_completed());
 mode.observe_dynamic_trigger({-1000,99999.f,0});
 assert(mode.trigger_count()==2u&&!mode.playthrough_completed());
 mode.observe_dynamic_trigger({999,0.f,257});
 assert(mode.trigger_count()==3u&&mode.playthrough_completed());
 assert(mode.skill()==3&&mode.revision()==1u&&same(mode.metrics(),fixed));
 // Triggers remain a separate map-compatible event/removal code path.
 assert(mode.set_skill(2)==f::Selection::Applied);
 assert(mode.playthrough_completed()&&mode.trigger_count()==3u);
}
void original_damage_rules_remain_compatible(){
 f::FixedDifficultyRuntime mode;
 assert(mode.set_skill(1)==f::Selection::Applied);
 OriginalDamageFields impact{100.f,0u};
 apply_damage_taken_metric(impact,mode.metrics());
 assert(eq(impact.damage,75.f));
 impact={100.f,0x80024021u};
 apply_damage_taken_metric(impact,mode.metrics());
 assert(eq(impact.damage,100.f));
 assert(eq(scaled_damage_done(23.f,mode.metrics()),23.f));
 assert(eq(ammo_quantity_multiplier(mode.metrics()),1.f));
 assert(eq(npc_random_wait_before_shoot(0.25f,mode.metrics()),0.35f));
 assert(mode.set_skill(3)==f::Selection::Applied);
 impact={100.f,0u};
 apply_damage_taken_metric(impact,mode.metrics());
 assert(eq(impact.damage,140.f));
 assert(eq(npc_random_wait_before_shoot(0.25f,mode.metrics()),0.15f));
}
struct MetricSink final:f::IOriginalMetricSink {
 RuntimeMetrics slots{};
 std::vector<std::size_t> order;
 std::size_t fail_at=original_metric_count;
 bool throw_on_failure=false;
 bool write_original_metric(std::size_t index,float value)override{
  order.push_back(index);
  if(index==fail_at) {
   if(throw_on_failure)throw 42;
   return false;
  }
  slots[index]=value;
  return true;
 }
};
void original_metric_host_publication(){
 f::FixedDifficultyRuntime mode;
 MetricSink original;
 const auto a=mode.publish_to_host(original);
 assert(a.complete&&a.attempted==14u&&a.committed==14u);
 assert(same(original.slots,original_initial_metric_values()));
 assert(original.order.size()==14u);
 for(std::size_t i=0u;i<14u;++i)assert(original.order[i]==i);
 assert(mode.set_skill(3)==f::Selection::Applied);
 original.order.clear();original.fail_at=7u;
 const auto failure=mode.publish_to_host(original);
 assert(!failure.complete&&failure.attempted==8u&&failure.committed==7u);
 assert(original.order.size()==8u&&original.order.back()==7u);
 const auto hard=f::make_profile(f::Level::Hard).metrics;
 for(std::size_t i=0u;i<7u;++i)assert(original.slots[i]==hard[i]);
 // Field 8 is deliberately still the normal profile: host publication
 // is not atomically rolled back and the host MUST abort normal Init.
 assert(original.slots[8]!=hard[8]);
 original.order.clear();original.throw_on_failure=true;
 const auto thrown=mode.publish_to_host(original);
 assert(!thrown.complete&&thrown.committed==7u&&thrown.attempted==8u);
 original.order.clear();original.fail_at=original_metric_count;
 const auto success=mode.publish_to_host(original);
 assert(success.complete&&success.committed==14u);
 assert(same(original.slots,hard));
 // Save restore example: after importing unknown adaptive values,
 // reapply the selected skill and republish it in the exact same order.
 assert(mode.reapply_after_archive(1)==f::Selection::Applied);
 assert(mode.publish_to_host(original).complete);
 assert(same(original.slots,f::make_profile(f::Level::Easy).metrics));
}

void archive_policy_reapplies_current_skill(){
 f::FixedDifficultyRuntime mode;
 const auto easy=f::make_profile(f::Level::Easy).metrics;
 const auto hard=f::make_profile(f::Level::Hard).metrics;
 // The original archive contains 14 mutable floats and up to ten pairs.
 // Adaptive archive values must NOT supersede fixed skill after load.
 ManagerStateArchiveFields archive{};
 const std::vector<float> fake_adaptive_metrics(14u,999.f);
 assert(apply_metric_scalars(archive,14,fake_adaptive_metrics));
 assert(archive.metrics[0]==999.f);
 assert(mode.reapply_after_archive(1)==f::Selection::Applied);
 assert(mode.revision()==1u&&same(mode.metrics(),easy));
 assert(mode.reapply_after_archive(1)==f::Selection::Unchanged);
 assert(mode.revision()==2u&&same(mode.metrics(),easy));
 assert(mode.reapply_after_archive(0)==f::Selection::InvalidSkill);
 assert(mode.revision()==2u&&same(mode.metrics(),easy));
 assert(mode.reapply_after_archive(3)==f::Selection::Applied);
 assert(mode.revision()==3u&&same(mode.metrics(),hard));
 // Placeholder or corrupted archived metrics have no access to the
 // read-only profile returned by gameplay consumers.
 assert(archive.metrics[0]==999.f&&mode.metric_at(0u)==1.4f);
}
}
int main(){
 profile_invariants();stable_skill_changes_and_lookup();
 no_adaptation_on_trigger_or_performance();
 original_damage_rules_remain_compatible();
 archive_policy_reapplies_current_skill();
 original_metric_host_publication();
}
