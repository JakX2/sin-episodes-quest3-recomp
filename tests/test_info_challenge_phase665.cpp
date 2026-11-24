#include "sin_compat/info_challenge_phase665.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
namespace n=sin_compat::difficulty::info_challenge_phase665;
namespace f=sin_compat::difficulty::fixed_phase663;
namespace {
struct Values final:n::IChallengeValues {
 float spawn=0.2f,initial=17.f,current=40.f;
 int spawn_reads=0,initial_reads=0,current_reads=0;
 bool fail=false;
 bool read_spawn_value(float& v)override{
  ++spawn_reads;v=spawn;return !fail;
 }
 bool read_initial_value(float& v)override{
  ++initial_reads;v=initial;return !fail;
 }
 bool read_current_value(float& v)override{
  ++current_reads;v=current;return !fail;
 }
};
struct Event{
 std::uint32_t offset,activator,caller;
 std::string name;
 float value;
};
struct Out final:n::IChallengeOutput{
 std::vector<Event> events;
 n::InfoChallenge* entity=nullptr;
 n::Code nested=n::Code::Completed;
 bool fail=false,throwing=false,reenter=false;
 bool fire_float(std::uint32_t off,std::string_view name,float v,
                 std::uint32_t actor,std::uint32_t caller)override{
  events.push_back({off,actor,caller,std::string(name),v});
  if(reenter&&entity){
   reenter=false;
   nested=entity->accept(n::Input::Current).code;
  }
  if(throwing)throw 1;
  return !fail;
 }
};
void original_distinct_value_sources_and_routing(){
 Values values;Out output;n::InfoChallenge entity(42u,values,output);
 assert(entity.accept("GetChallengeDifference").code==n::Code::NoSnapshot);
 assert(entity.capture_snapshot()==n::Code::Completed);
 assert(entity.snapshot().valid&&entity.snapshot().value==0.2f);
 assert(values.spawn_reads==1&&values.initial_reads==0);
 assert(entity.accept("GetInitialChallenge").value==17.f);
 assert(entity.accept("GetCurrentChallenge").value==40.f);
 const auto diff=entity.accept("GetChallengeDifference");
 assert(diff.code==n::Code::Completed);
 assert(diff.value==3980.f); // no invented normalization of retail values
 assert(values.current_reads==2&&values.initial_reads==1);
 assert(output.events.size()==3u);
 const auto& a=output.events[0];const auto& b=output.events[1];
 const auto& c=output.events[2];
 assert(a.name=="OnInitialChallenge"&&a.offset==0x394u&&a.value==17.f);
 assert(b.name=="OnCurrentChallenge"&&b.offset==0x3acu&&b.value==40.f);
 assert(c.name=="OnChallengeDifference"&&c.offset==0x3c4u&&c.value==3980.f);
 for(const auto& e:output.events)assert(e.activator==42u&&e.caller==42u);
 assert(entity.accept("getcurrentchallenge").code==n::Code::UnsupportedInput);
 assert(entity.accept(static_cast<n::Input>(255)).code==n::Code::UnsupportedInput);
}
void fixed_profile_no_adaptive_drift(){
 f::FixedDifficultyRuntime policy;
 assert(policy.set_skill(3)==f::Selection::Applied);
 n::FixedChallengeValues values(policy);
 Out output;n::InfoChallenge entity(13u,values,output);
 assert(entity.capture_snapshot()==n::Code::Completed);
 for(int i=0;i<25;++i){
  policy.observe_performance_sample(9000.f);
  policy.observe_dynamic_trigger({i,200.f,0});
 }
 assert(entity.accept("GetInitialChallenge").value==1.f);
 assert(entity.accept("GetCurrentChallenge").value==1.f);
 assert(entity.accept("GetChallengeDifference").value==0.f);
 assert(policy.set_skill(1)==f::Selection::Applied);
 assert(entity.accept("GetInitialChallenge").value==1.f);
 assert(entity.accept("GetCurrentChallenge").value==0.f);
 assert(entity.accept("GetChallengeDifference").value==-100.f);
 const auto saved=entity.snapshot();
 Out restore_output;n::InfoChallenge restored(14u,values,restore_output);
 assert(restored.restore_snapshot(saved));
 assert(restored.accept("GetChallengeDifference").value==-100.f);
 assert(restored.capture_snapshot()==n::Code::Completed);
 assert(restored.accept("GetChallengeDifference").value==0.f);
}
void host_faults_and_snapshot_validation(){
 Values values;Out output;n::InfoChallenge entity(33u,values,output);
 values.fail=true;
 assert(entity.capture_snapshot()==n::Code::ProviderFailure);
 assert(entity.accept(n::Input::Initial).code==n::Code::ProviderFailure);
 assert(output.events.empty());
 values.fail=false;
 values.spawn=std::numeric_limits<float>::infinity();
 assert(entity.capture_snapshot()==n::Code::InvalidValue);
 assert(!entity.snapshot().valid);
 assert(!entity.restore_snapshot({std::numeric_limits<float>::quiet_NaN(),true}));
 values.spawn=0.7f;
 assert(entity.capture_snapshot()==n::Code::Completed);
 values.current=std::numeric_limits<float>::quiet_NaN();
 assert(entity.accept(n::Input::Current).code==n::Code::InvalidValue);
 assert(output.events.empty());
 values.current=1.f;output.fail=true;
 assert(entity.accept(n::Input::Current).code==n::Code::OutputFailure);
 output.fail=false;output.throwing=true;
 assert(entity.accept(n::Input::Current).code==n::Code::HostException);
 output.throwing=false;output.entity=&entity;output.reenter=true;
 assert(entity.accept(n::Input::Current).code==n::Code::Completed);
 assert(output.nested==n::Code::Reentrant);
}
}
int main(){
 original_distinct_value_sources_and_routing();
 fixed_profile_no_adaptive_drift();
 host_faults_and_snapshot_validation();
}
