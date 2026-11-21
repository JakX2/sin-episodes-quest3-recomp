#include "sin_compat/original_grunt_reactions_phase565.hpp"
#include <cassert>
#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <vector>
using namespace sin_compat::original_grunt_reactions_phase565;

struct HostProbe final : Host {
  std::vector<std::string> events;
  std::string name;
  int argument=99;
  unsigned mode=99;
  int return_value=-1;
  int assault_calls=0;
  int queue(std::string_view phrase,int arg,unsigned m) override {
    events.push_back("queue");name=std::string(phrase);argument=arg;mode=m;
    return return_value;
  }
  void assault_side_effect() override {events.push_back("assault");++assault_calls;}
  void reset(){events.clear();name.clear();argument=99;mode=99;}
};

int main() {
  HostProbe h;
  assert(grunt_flank=="GRUNT_FLANK" && grunt_die=="GRUNT_DIE");
  assert(grunt_mutantdeath=="GRUNT_MUTANTDEATH");
  assert(grunt_pain=="GRUNT_PAIN" && grunt_taunt=="GRUNT_TAUNT");

  // +0x5A8 event 0 delegates to the separately researched assault helper.
  assert(event_voice_2235ae30(0,12.f,h)==Outcome::Delegated);
  assert(h.assault_calls==1 && (h.events==std::vector<std::string>{"assault"}));
  h.reset();
  assert(event_voice_2235ae30(2,300.f,h)==Outcome::Suppressed);
  assert(event_voice_2235ae30(1,240.f,h)==Outcome::Suppressed);
  assert(event_voice_2235ae30(1,239.f,h)==Outcome::Suppressed);
  assert(h.events.empty());
  assert(event_voice_2235ae30(1,241.f,h)==Outcome::Requested);
  assert(h.name=="GRUNT_FLANK" && h.argument==0 && h.mode==2u);
  h.reset();
  assert(event_voice_2235ae30(1,std::numeric_limits<float>::quiet_NaN(),h)==Outcome::UnmodeledFloat);
  assert(h.events.empty());

  GruntVoiceState g;
  g.next_voice_time_0xc78=12.f;
  assert(pain_taunt_2235ae90(g,12.f,h)==Outcome::Suppressed);
  assert(g.next_voice_time_0xc78==12.f && h.events.empty());
  g.flags_0xbc=0x10000000u;
  assert(pain_taunt_2235ae90(g,13.f,h)==Outcome::Suppressed);
  g.flags_0xbc=0x08000000u;
  assert(pain_taunt_2235ae90(g,13.f,h)==Outcome::Suppressed);
  g.flags_0xbc=0;
  g.value_0x98=0;
  assert(pain_taunt_2235ae90(g,13.f,h)==Outcome::UnmodeledFloat);
  assert(g.next_voice_time_0xc78==12.f && h.events.empty());
  g.value_0x98=100;

  // Original +0x598 first chooses a default GRUNT_PAIN getter, but a
  // high signed ratio (>0.9) when bit31 is clear upgrades to GRUNT_TAUNT.
  g.value_0x9c=91;
  assert(pain_taunt_2235ae90(g,13.f,h)==Outcome::Requested);
  assert(h.name=="GRUNT_TAUNT" && h.argument==-1 && h.mode==0u);
  assert(g.flags_0x988 & 0x80000000u);
  assert(g.next_voice_time_0xc78==14.f);
  h.reset();
  assert(pain_taunt_2235ae90(g,13.5f,h)==Outcome::Suppressed);
  assert(h.events.empty());
  assert(pain_taunt_2235ae90(g,15.f,h)==Outcome::Requested);
  assert(h.name=="GRUNT_PAIN" && (g.flags_0x988&0x80000000u));

  // Low ratio <0.25 sets bit30 once and chooses native +0x830, still the
  // same literal GRUNT_PAIN; a ratio exactly 0.25 is not a low threshold.
  h.reset();g.flags_0x988=0;g.value_0x9c=25;
  assert(pain_taunt_2235ae90(g,17.f,h)==Outcome::Requested);
  assert(h.name=="GRUNT_PAIN" && g.flags_0x988==0);
  g.value_0x9c=24;h.reset();
  assert(pain_taunt_2235ae90(g,19.f,h)==Outcome::Requested);
  assert(h.name=="GRUNT_PAIN" && (g.flags_0x988&0x40000000u));
  assert(g.next_voice_time_0xc78==20.f);
  h.reset();
  assert(pain_taunt_2235ae90(g,21.f,h)==Outcome::Requested);
  assert(h.name=="GRUNT_PAIN" && g.flags_0x988==0x40000000u);

  // Original active voice cooldown is a 1-second stored float. A safe
  // host-only NaN case deliberately returns unresolved instead of
  // claiming the 2006 x87 invalid status and exceptions are C++-equal.
  h.reset();
  assert(pain_taunt_2235ae90(g,std::numeric_limits<float>::quiet_NaN(),h)==Outcome::UnmodeledFloat);
  assert(h.events.empty());

  // +0x58C requires all three original gates to admit a death phrase.
  g.flag_0xbf=0x10;g.flag_0xcb1=1;g.flag_0xc44=0;
  assert(death_voice_2235aff0(g,h)==Outcome::Suppressed);
  g.flag_0xbf=0;g.flag_0xcb1=0;
  assert(death_voice_2235aff0(g,h)==Outcome::Suppressed);
  g.flag_0xcb1=1;g.flag_0xc44=1;
  assert(death_voice_2235aff0(g,h)==Outcome::Suppressed);
  assert(h.events.empty());
  g.flag_0xc44=0;g.dword_0xc68=9;
  assert(death_voice_2235aff0(g,h)==Outcome::Requested);
  assert(h.name=="GRUNT_DIE" && h.argument==-1 && h.mode==0);
  h.reset();g.dword_0xc68=10;
  assert(death_voice_2235aff0(g,h)==Outcome::Requested);
  assert(h.name=="GRUNT_MUTANTDEATH" && h.argument==-1 && h.mode==0);
  return 0;
}