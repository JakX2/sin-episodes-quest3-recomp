// Phase663: use actual compiled original SiN CTriggerDynamicDifficulty and
// CTriggerSecret adapters with the opt-in fixed challenge-manager bridge.
// Engine data/events and CBaseTrigger itself are represented by a typed host.
#include "sin_compat/fixed_difficulty_trigger_phase663.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <string>
#include <vector>
using namespace sin_compat::map_entities;
namespace f=sin_compat::difficulty::fixed_phase663;
namespace {
struct Captured {
 TriggerEventName kind;
 std::int32_t difficulty=0;
 float estimated_time=0;
 int descriptor=0;
 std::string name{};
};
struct Engine final:IOriginalTriggerHost {
 EntityRef trigger{90,1},secret{91,1},player{5,2},grunt{7,2};
 bool event_ok=true,remove_ok=true;
 int dynamic_signals=0,events=0,removals=0,base_calls=0;
 std::vector<Captured> captured;
 bool is_live(EntityRef e) const override {
  return e==trigger||e==secret||e==player||e==grunt;
 }
 bool is_player(EntityRef e) const override{return e==player;}
 bool initialize_native_base_trigger(EntityRef)override{
  ++base_calls;return true;
 }
 void report_original_warning(std::string_view)override{}
 int current_difficulty_owner_index() const override{return 5;}
 bool perform_special_trigger_base_action(EntityRef)override{return true;}
 bool publish(const TriggerEvent& e)override{
  ++events;
  captured.push_back({e.event,e.decision,e.value,e.descriptor,std::string(e.name)});
  return event_ok;
 }
 bool mark_difficulty_manager()override{
  // Fixed-mode test must NEVER forward to original adaptive manager.
  ++dynamic_signals;return false;
 }
 bool queue_remove(EntityRef)override{
  ++removals;return remove_ok;
 }
};
void dynamic_trigger_is_preserved_without_adaptation(){
 Engine game;
 f::FixedDifficultyRuntime skill;
 assert(skill.set_skill(3)==f::Selection::Applied);
 const auto before=skill.metrics();
 f::FixedDifficultyTriggerHost host(game,skill);
 OriginalTriggerEntityFactory entities(host);
 assert(entities.spawn({"trigger_dynamic_difficulty",game.trigger}).ok());
 TriggerConfig conf{};
 conf.targetname="challenge_gate_beta";
 conf.estimated_time=88.5f;
 conf.estimated_difficulty=900;
 conf.end_of_game=257;
 assert(entities.configure(game.trigger,conf)==TriggerCode::Handled);
 assert(entities.touch(game.trigger,game.grunt).code==TriggerCode::NonPlayer);
 assert(skill.trigger_count()==0u&&game.events==0);
 const auto finished=entities.touch(game.trigger,game.player);
 assert(finished.handled()&&finished.event_published);
 assert(finished.manager_signaled&&finished.removal_queued);
 assert(game.events==1&&game.removals==1&&game.dynamic_signals==0);
 assert(game.captured[0].kind==TriggerEventName::DynamicDifficulty);
 assert(game.captured[0].name=="challenge_gate_beta");
 assert(game.captured[0].difficulty==900&&game.captured[0].estimated_time==88.5f);
 assert(skill.trigger_count()==1u&&skill.playthrough_completed());
 assert(skill.skill()==3&&skill.revision()==1u&&skill.metrics()==before);
 assert(entities.touch(game.trigger,game.player).code==TriggerCode::AlreadyConsumed);
 assert(skill.trigger_count()==1u);
}
void secret_trigger_remains_independent(){
 Engine game;
 f::FixedDifficultyRuntime skill;
 f::FixedDifficultyTriggerHost host(game,skill);
 OriginalTriggerEntityFactory entities(host);
 assert(entities.spawn({"trigger_secret",game.secret}).ok());
 assert(entities.touch(game.secret,game.player).handled());
 assert(game.events==1&&game.removals==1);
 assert(game.captured[0].kind==TriggerEventName::Game);
 assert(game.captured[0].descriptor==12);
 assert(game.dynamic_signals==0);
 assert(skill.trigger_count()==0u&&!skill.playthrough_completed());
 assert(skill.skill()==2);
}
void original_low_byte_event_failure_and_deferred_delete(){
 Engine game;
 f::FixedDifficultyRuntime skill;
 f::FixedDifficultyTriggerHost host(game,skill);
 OriginalTriggerEntityFactory entities(host);
 assert(entities.spawn({"trigger_dynamic_difficulty",game.trigger}).ok());
 TriggerConfig cfg{};
 cfg.end_of_game=256; // original CMP BYTE low byte = 0
 cfg.estimated_difficulty=777;
 cfg.estimated_time=999.f;
 assert(entities.configure(game.trigger,cfg)==TriggerCode::Handled);
 game.event_ok=false;
 const auto fail=entities.touch(game.trigger,game.player);
 assert(fail.code==TriggerCode::EventUnavailable);
 assert(!fail.event_published&&fail.removal_queued);
 assert(game.events==1&&game.removals==1);
 assert(game.dynamic_signals==0);
 assert(skill.trigger_count()==1u&&!skill.playthrough_completed());
 assert(skill.skill()==2);
 // Even when the original event publisher fails, the trigger is
 // consumed and queued for removal, not retried to re-roll difficulty.
 assert(entities.touch(game.trigger,game.player).code==TriggerCode::AlreadyConsumed);
 assert(skill.trigger_count()==1u);
}
void original_manager_signal_replaced_not_duplicated(){
 Engine game;
 f::FixedDifficultyRuntime skill;
 assert(skill.set_skill(1)==f::Selection::Applied);
 f::FixedDifficultyTriggerHost host(game,skill);
 OriginalTriggerEntityFactory entities(host);
 assert(entities.spawn({"trigger_dynamic_difficulty",game.trigger}).ok());
 TriggerConfig cfg{};
 cfg.end_of_game=1;
 assert(entities.configure(game.trigger,cfg)==TriggerCode::Handled);
 game.event_ok=false;
 game.remove_ok=false;
 const auto report=entities.touch(game.trigger,game.player);
 assert(report.code==TriggerCode::RemovalPending);
 assert(!report.event_published&&report.manager_signaled);
 assert(!report.removal_queued&&game.removals==1);
 assert(skill.trigger_count()==1u&&skill.playthrough_completed());
 assert(game.dynamic_signals==0&&skill.skill()==1);
 game.remove_ok=true;
 assert(entities.retry_pending_remove(game.trigger)==TriggerCode::Handled);
 assert(game.removals==2&&game.events==1&&skill.trigger_count()==1u);
}
}
int main(){
 dynamic_trigger_is_preserved_without_adaptation();
 secret_trigger_remains_independent();
 original_low_byte_event_failure_and_deferred_delete();
 original_manager_signal_replaced_not_duplicated();
}
