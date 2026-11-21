#pragma once
// Original SiN 2006 Grunt voice selection, adapted for independent host
// behavior tests. NOT an engine-loadable implementation, x86 vtable or
// complete parity for native x87 exceptions, nan or invalid ratios.
#include <cmath>
#include <cstdint>
#include <string_view>

namespace sin_compat::original_grunt_reactions_phase565 {
inline constexpr std::string_view grunt_flank = "GRUNT_FLANK";
inline constexpr std::string_view grunt_die = "GRUNT_DIE";
inline constexpr std::string_view grunt_mutantdeath = "GRUNT_MUTANTDEATH";
inline constexpr std::string_view grunt_pain = "GRUNT_PAIN";
inline constexpr std::string_view grunt_taunt = "GRUNT_TAUNT";
inline constexpr double high_ratio_threshold = 0.9; // original .rdata double
inline constexpr double low_ratio_threshold = 0.25; // original .rdata double
inline constexpr float flank_measure_threshold = 240.0f;
inline constexpr float reaction_cooldown = 1.0f;

struct GruntVoiceState {
  std::uint32_t flags_0xbc = 0;
  std::uint32_t flags_0x988 = 0;
  std::int32_t value_0x98 = 100;   // divisors: semantic identity unresolved
  std::int32_t value_0x9c = 100;   // signed numerator: name not assumed
  std::uint8_t flag_0xbf = 0;
  std::uint8_t flag_0xcb1 = 0;
  std::uint8_t flag_0xc44 = 0;
  std::int32_t dword_0xc68 = 0;
  float next_voice_time_0xc78 = 0.f;
};

struct Host {
  virtual ~Host() = default;
  // Original 0x220A35C0 on Grunt+0xCC0. Native caller ignores return.
  virtual int queue(std::string_view phrase, int argument, unsigned mode) = 0;
  // Preexisting Phase537 assault side effect 0x2235A3A0 invoked for event 0.
  virtual void assault_side_effect() = 0;
};

enum class Outcome { Suppressed, Requested, Delegated, UnmodeledFloat };

// Original vtable +0x5A8: 0 -> older Grunt assault announcement helper;
// 1 -> metric from an engine-owned object, gate strictly > 240.0f,
//      queue GRUNT_FLANK with (argument=0, mode=2);
// other numeric event values -> no action. The original metric accessor's
// full return/side effects are not included in this host-only model.
inline Outcome event_voice_2235ae30(int event, float metric, Host& host) {
  if(event==0) {host.assault_side_effect();return Outcome::Delegated;}
  if(event!=1) return Outcome::Suppressed;
  if(!std::isfinite(metric)) return Outcome::UnmodeledFloat;
  if(metric<=flank_measure_threshold) return Outcome::Suppressed;
  (void)host.queue(grunt_flank, 0, 2u);
  return Outcome::Requested;
}

// Original vtable +0x598: absolute engine time is a float; at a strictly
// later eligible moment it dispatches a Grunt phrase, then writes now+1.0f.
// This safe host refuses invalid signed-ratio divisors/NaN; original x87
// division/exception behavior cannot be claimed without runtime study.
inline Outcome pain_taunt_2235ae90(GruntVoiceState& g, float now, Host& host) {
  if((g.flags_0xbc & 0x10000000u) != 0) return Outcome::Suppressed;
  if(!std::isfinite(now) || !std::isfinite(g.next_voice_time_0xc78))
    return Outcome::UnmodeledFloat;
  if(!(now > g.next_voice_time_0xc78)) return Outcome::Suppressed;
  if((g.flags_0xbc & 0x08000000u) != 0) return Outcome::Suppressed;
  if(g.value_0x98 == 0) return Outcome::UnmodeledFloat;

  const auto ratio = static_cast<double>(g.value_0x9c) /
                     static_cast<double>(g.value_0x98);
  std::string_view token = grunt_pain;  // original getter +0x828
  if((g.flags_0x988 & 0x80000000u) == 0 && ratio > high_ratio_threshold) {
    g.flags_0x988 |= 0x80000000u;
    token = grunt_taunt; // original getter +0x82c
  } else if((g.flags_0x988 & 0x40000000u) == 0 &&
            ratio < low_ratio_threshold) {
    g.flags_0x988 |= 0x40000000u;
    token = grunt_pain; // original getter +0x830, same native literal
  }
  (void)host.queue(token, -1, 0u);
  g.next_voice_time_0xc78 = now + reaction_cooldown;
  return Outcome::Requested;
}

// Original vtable +0x58C: death voice gate. All three explicit field gates
// precede the single choice of GRUNT_DIE vs GRUNT_MUTANTDEATH. The native
// sentence argument is -1, mode 0 in both branches.
inline Outcome death_voice_2235aff0(const GruntVoiceState& g, Host& host) {
  if((g.flag_0xbf & 0x10u) != 0u) return Outcome::Suppressed;
  if(g.flag_0xcb1 == 0) return Outcome::Suppressed;
  if(g.flag_0xc44 != 0) return Outcome::Suppressed;
  const auto phrase = g.dword_0xc68 == 0x0a ? grunt_mutantdeath : grunt_die;
  (void)host.queue(phrase, -1, 0u);
  return Outcome::Requested;
}

} // namespace sin_compat::original_grunt_reactions_phase565