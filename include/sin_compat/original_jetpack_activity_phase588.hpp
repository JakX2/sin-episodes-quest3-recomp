#pragma once
// Standalone original-free model of the 2006 Jetpack Grunt ACT override.
// RTTI and dispatch IDs are pinned by the matching original-DLL auditor.
#include "sin_compat/original_grunt_activity_phase587.hpp"
#include <cmath>
#include <optional>

namespace sin_compat::jetpack_activity_phase588 {
using grunt_activity_phase587::Engine;
using grunt_activity_phase587::GruntState;
using grunt_activity_phase587::ACT_IDLE;
using grunt_activity_phase587::ACT_WALK_AIM;
using grunt_activity_phase587::ACT_RANGE_ATTACK1;
using grunt_activity_phase587::ACT_RELOAD;

// Numerals come from the original shared 921-entry ACT registration, and
// appear as immediates in the original jetpack override switch.
enum JetpackActivity : int {
  ACT_JETPACK_IDLE = 208,
  ACT_JETPACK_MOVE = 213,
  ACT_JETPACK_RELOAD = 218,
};
struct Vec3 { float x=0, y=0, z=0; };
enum class ResultStatus {
  Selected,
  MissingNativeAttachment,
  ExceptionalNativeFloatNotModeled,
};
struct Result { ResultStatus status; int activity; bool delegated; };

// The native function first dereferences an entity serial handle at owner
// +0x1024 and reads THREE floats at entity +0xC18/+0xC1C/+0xC20, even when
// the input ultimately takes the reload or Grunt-base fallback branches.
// On invalid handles the retail DLL can dereference null: safe host code
// reports a missing dependency instead of claiming crash parity.
inline Result jetpack_activity_338(GruntState& state, int incoming,
   const std::optional<Vec3>& attached_vector, Engine& callbacks) {
  if(!attached_vector)return {ResultStatus::MissingNativeAttachment,incoming,false};
  const auto& v=*attached_vector;
  if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z))
    return {ResultStatus::ExceptionalNativeFloatNotModeled,incoming,false};
  const double magnitude=std::sqrt(double(v.x)*v.x+double(v.y)*v.y+double(v.z)*v.z);
  if(!std::isfinite(magnitude))return {ResultStatus::ExceptionalNativeFloatNotModeled,incoming,false};
  switch(incoming) {
   case ACT_IDLE:
   case ACT_WALK_AIM:
   case ACT_RANGE_ATTACK1:
     return {ResultStatus::Selected,magnitude>50.0?ACT_JETPACK_MOVE:ACT_JETPACK_IDLE,false};
   case ACT_RELOAD:
     return {ResultStatus::Selected,ACT_JETPACK_RELOAD,false};
   default:
     return {ResultStatus::Selected,
       grunt_activity_phase587::grunt_activity_338(state,incoming,callbacks),true};
  }
}
}