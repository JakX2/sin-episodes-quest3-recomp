#include "sin_compat/original_grunt_event_switch_phase581.hpp"
#include <cassert>
#include <string>
#include <vector>
using namespace sin_compat::grunt_event_switch_phase581;

struct Voice final : prev::VoiceSink {
  int queue_status = 0;
  int calls=0;
  std::string token;
  std::vector<std::string>* history = nullptr;
  int Queue(std::string_view name,int arg,int mode) override {
    ++calls;token=std::string(name);
    assert(arg==2&&mode==2);
    if(history)history->push_back("voice");
    return queue_status;
  }
  float RandomFloat(float,float) override {assert(false&&"alert should not use randomized cooldown");return 0;}
};
struct Hooks final : EngineHooks {
  int draw = 1;
  int draws = 0;
  int state = 0;
  int followup = 0;
  int notified = 0;
  int fallbacks = 0;
  int fallback_id = 0;
  std::vector<std::string> history;
  int RandomInt(int lo,int hi) override {
    assert(lo==1&&hi==4);++draws;history.push_back("random");return draw;
  }
  void OnEvent8CAfterOptionalAlert() override {++followup;history.push_back("after");}
  void OnEvent8CGlobalState() override {++state;history.push_back("global");}
  void OnEvent91Notify(int argument) override {notified=argument;history.push_back("notify");}
  void GenericBaseEvent(int id) override {++fallbacks;fallback_id=id;history.push_back("base");}
};

void check_table() {
  const struct {int code; Branch branch;} known[] = {
    {0x1b,Branch::Event1B},{0x4a,Branch::Event4A},
    {0x8c,Branch::Event8C},{0x8d,Branch::Event8D},
    {0x8f,Branch::Event8F},{0x90,Branch::Event90},
    {0x91,Branch::Event91},{0x92,Branch::Event92},
    {0x94,Branch::Event94}
  };
  int count=0;
  for(int id=0x1b;id<=0x94;++id){
    Branch b=IdentifyOriginalGruntBranch(id);
    if(b==Branch::GenericFallback)continue;
    ++count;
    bool found=false;
    for(const auto& entry:known)if(entry.code==id && entry.branch==b)found=true;
    assert(found);
  }
  assert(count==9);
  assert(IdentifyOriginalGruntBranch(0x1a)==Branch::GenericFallback);
  assert(IdentifyOriginalGruntBranch(0x95)==Branch::GenericFallback);
  assert(IdentifyOriginalGruntBranch(0x52)==Branch::GenericFallback);
  assert(IdentifyOriginalGruntBranch(0x53)==Branch::GenericFallback);
  assert(IdentifyOriginalGruntBranch(0x54)==Branch::GenericFallback);
}
void check_event_8c() {
  Hooks hooks;
  Voice voice;voice.history=&hooks.history;
  float deadline=0.0f;
  int32_t copied=-1;
  hooks.draw=1;
  auto x=Dispatch(0x8c,hooks,voice,1.0f,deadline,9,copied);
  assert(x==Outcome::Event8CCompleted);
  assert(voice.calls==1&&voice.token=="GRUNT_GO_ALERT");
  assert(hooks.draws==1&&hooks.followup==1&&hooks.state==1);
  assert(hooks.history==std::vector<std::string>({"random","voice","after","global"}));
  assert(deadline==11.0f&&copied==-1);
  hooks.history.clear();
  hooks.draw=4;
  x=Dispatch(0x8c,hooks,voice,2.0f,deadline,9,copied);
  assert(x==Outcome::Event8CCompleted);
  assert(voice.calls==1 && hooks.history==std::vector<std::string>({"random","after","global"}));
  // Original event 0x8C random=1 still calls virtual+0x590; the *separate*
  // Grunt go-alert cooldown can independently veto the sentence.
  hooks.history.clear();hooks.draw=1;
  x=Dispatch(0x8c,hooks,voice,3.0f,deadline,9,copied);
  assert(x==Outcome::Event8CCompleted && voice.calls==1);
  assert(hooks.history==std::vector<std::string>({"random","after","global"}));
  hooks.draw=0;
  hooks.history.clear();
  assert(Dispatch(0x8c,hooks,voice,4.0f,deadline,9,copied)==Outcome::UnsupportedHostRandom);
  assert(hooks.history==std::vector<std::string>({"random"}));
}
void check_other_branches() {
  Hooks hooks;
  Voice voice;
  float deadline=0;
  int32_t copied=-111;
  assert(Dispatch(0x91,hooks,voice,1,deadline,777,copied)==Outcome::Event91Completed);
  assert(copied==777&&hooks.notified==0x1e&&hooks.history==std::vector<std::string>({"notify"}));
  hooks.history.clear();
  assert(Dispatch(0x94,hooks,voice,1,deadline,3,copied)==Outcome::Event94NoOp);
  assert(hooks.history.empty()&&copied==777);
  for(int id:{0x1b,0x4a,0x8d,0x8f,0x90,0x92}){
    assert(Dispatch(id,hooks,voice,1,deadline,3,copied)==Outcome::NeedsOpaqueNativeBranch);
    assert(hooks.history.empty());
  }
  assert(Dispatch(0x54,hooks,voice,1,deadline,3,copied)==Outcome::DelegatedToBaseEventHandler);
  assert(hooks.fallbacks==1&&hooks.fallback_id==0x54);
  assert(Dispatch(0x95,hooks,voice,1,deadline,3,copied)==Outcome::DelegatedToBaseEventHandler);
  assert(hooks.fallback_id==0x95&&hooks.fallbacks==2);
  // The base event handler may then itself dispatch the 0x54 float payload
  // to a separate Grunt virtual +0x5A8 (Phase580); avoid double dispatch.
}
int main(){check_table();check_event_8c();check_other_branches();}