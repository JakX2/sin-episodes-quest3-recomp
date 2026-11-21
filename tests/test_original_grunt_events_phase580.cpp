#include "sin_compat/original_grunt_events_phase580.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

using namespace sin_compat::grunt_events_phase580;

struct Voice final : VoiceSink {
  struct Request { std::string token; int argument; int mode; };
  std::vector<Request> requests;
  int next_status = 0;
  float random_value = 7.0f;
  int random_calls = 0;
  float last_min = 0, last_max = 0;
  int Queue(std::string_view token, int argument, int mode) override {
    requests.push_back({std::string(token), argument, mode});
    return next_status;
  }
  float RandomFloat(float low, float high) override {
    ++random_calls; last_min=low; last_max=high; return random_value;
  }
};
struct Existing final : ExistingEventSink {
  int pain=0,death=0,assault=0,flank=0;
  void PainOrTauntEvent() override {++pain;}
  void DeathSentenceEvent() override {++death;}
  void AssaultSentenceEvent() override {++assault;}
  void FlankPathLengthEvent() override {++flank;}
};

void check_announce_and_refind() {
  Voice voice;
  assert(Announce(voice)==0);
  assert(voice.requests.size()==1);
  assert(voice.requests[0].token=="GRUNT_ANNOUNCE");
  assert(voice.requests[0].argument==0 && voice.requests[0].mode==2);
  assert(RefindEnemy(voice)==0);
  assert(voice.requests[1].token=="GRUNT_REFIND_ENEMY");
  assert(voice.requests[1].argument==2 && voice.requests[1].mode==2);
}
void check_alert_deadline() {
  Voice voice;
  float deadline=10.0f;
  assert(!GoAlert(voice,10.0f,deadline)); // strict >, equal skips
  assert(!GoAlert(voice,9.999f,deadline));
  assert(voice.requests.empty());
  voice.next_status=-9;
  assert(GoAlert(voice,10.001f,deadline)); // even failure advances deadline
  assert(voice.requests.size()==1 && voice.requests[0].token=="GRUNT_GO_ALERT");
  assert(voice.requests[0].argument==2 && voice.requests[0].mode==2);
  assert(deadline>20.0f);
  assert(!GoAlert(voice,deadline,deadline));
  assert(GoAlert(voice,deadline+1.0f,deadline));
  assert(voice.requests.size()==2);
  float nan=std::numeric_limits<float>::quiet_NaN();
  assert(!GoAlert(voice,nan,deadline));
  assert(!GoAlert(voice,deadline,nan));
}
void check_lost_status_and_side_effects() {
  Voice voice;
  float deadline=3.0f;
  LostInput data;
  data.deadline_compare_ah=0x40;
  data.now_after_queue=50.0f;
  assert(LostEnemy(voice,data,deadline)==LostOutcome::CooldownBlocked);
  data.deadline_compare_ah=0x01;
  assert(LostEnemy(voice,data,deadline)==LostOutcome::CooldownBlocked);
  assert(voice.requests.empty() && voice.random_calls==0 && deadline==3.0f);
  data.deadline_compare_ah=0;
  data.resolved_target=true;
  data.elapsed_compare_ah=0x40; // <=10 or equal -> short
  voice.next_status=-1;
  assert(LostEnemy(voice,data,deadline)==LostOutcome::SentenceRejected);
  assert(voice.requests[0].token=="GRUNT_LOST_SHORT");
  assert(voice.requests[0].argument==0 && voice.requests[0].mode==2);
  assert(voice.random_calls==0 && deadline==3.0f);
  voice.next_status=0;
  assert(LostEnemy(voice,data,deadline)==LostOutcome::QueuedShort);
  assert(voice.random_calls==1 && voice.last_min==5.0f && voice.last_max==15.0f);
  assert(deadline==57.0f);
  data.elapsed_compare_ah=0; // time since target observation >10 -> long
  voice.random_value=15.0f;
  assert(LostEnemy(voice,data,deadline)==LostOutcome::QueuedLong);
  assert(voice.requests.back().token=="GRUNT_LOST_LONG" && deadline==65.0f);
  data.resolved_target=false;
  data.elapsed_compare_ah=0x40;
  assert(LostEnemy(voice,data,deadline)==LostOutcome::QueuedLong);
  // Authentic x87 jnp path: unordered original deadline AH 0x45 is admitted.
  data.resolved_target=true;
  data.deadline_compare_ah=0x45;
  data.elapsed_compare_ah=0x45;
  assert(LostEnemy(voice,data,deadline)==LostOutcome::QueuedShort);
  assert(voice.requests.back().token=="GRUNT_LOST_SHORT");
  voice.random_value=4.0f;
  assert(LostEnemy(voice,data,deadline)==LostOutcome::UnsupportedHostRandom);
}
void check_float_integer_event_routing() {
  Voice voice;
  Existing prior;
  float deadline=0;
  assert(DispatchOriginalEvent(0x50,123.0f,0,deadline,voice,prior)==DispatchResult::Handled);
  assert(voice.requests.back().token=="GRUNT_ANNOUNCE");
  assert(DispatchOriginalEvent(0x51,0.0f,1,deadline,voice,prior)==DispatchResult::Handled);
  assert(voice.requests.back().token=="GRUNT_GO_ALERT");
  assert(deadline==11.0f);
  assert(DispatchOriginalEvent(0x52,0.0f,1,deadline,voice,prior)==DispatchResult::DelegatedPreviouslyAuthenticatedHandler);
  assert(DispatchOriginalEvent(0x53,0.0f,1,deadline,voice,prior)==DispatchResult::DelegatedPreviouslyAuthenticatedHandler);
  assert(prior.pain==1 && prior.death==1);
  assert(DispatchOriginalEvent(0x54,-0.9f,1,deadline,voice,prior)==DispatchResult::DelegatedPreviouslyAuthenticatedHandler);
  assert(prior.assault==1);
  assert(DispatchOriginalEvent(0x54,1.99f,1,deadline,voice,prior)==DispatchResult::DelegatedPreviouslyAuthenticatedHandler);
  assert(prior.flank==1);
  assert(DispatchOriginalEvent(0x54,-1.99f,1,deadline,voice,prior)==DispatchResult::Ignored);
  assert(DispatchOriginalEvent(0x54,2.0f,1,deadline,voice,prior)==DispatchResult::Ignored);
  assert(DispatchOriginalEvent(0x4f,1.0f,1,deadline,voice,prior)==DispatchResult::Ignored);
  assert(prior.assault==1 && prior.flank==1 && prior.pain==1 && prior.death==1);
  auto nan=std::numeric_limits<float>::quiet_NaN();
  assert(DispatchOriginalEvent(0x54,nan,1,deadline,voice,prior)==DispatchResult::NeedsNativeX87Conversion);
  assert(DispatchOriginalEvent(0x54,std::numeric_limits<float>::infinity(),1,deadline,voice,prior)==DispatchResult::NeedsNativeX87Conversion);
  assert(DispatchOriginalEvent(0x54,3.4e38f,1,deadline,voice,prior)==DispatchResult::NeedsNativeX87Conversion);
  int32_t converted=0;
  assert(TruncateFloatArgument(-1.9f,converted) && converted==-1);
  assert(TruncateFloatArgument(1.9f,converted) && converted==1);
}
int main() {
  check_announce_and_refind();
  check_alert_deadline();
  check_lost_status_and_side_effects();
  check_float_integer_event_routing();
}