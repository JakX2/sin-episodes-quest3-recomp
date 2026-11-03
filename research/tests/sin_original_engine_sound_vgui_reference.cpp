#include "sin_compat/original_engine_sound_vgui_abi.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
using namespace sin_compat::phase98;
int main(){
  static_assert(sound_candidates.size()==2);
  static_assert(original_emitter_vtable==0x2c019c88u);
  static_assert(original_input_vtable==0x100472acu);
  assert(sound_candidates[0].old_index==3u && sound_candidates[0].native_index==7u);
  assert(sound_candidates[1].old_index==6u && sound_candidates[1].native_index==10u);
  assert(sound_candidates[0].old_x86_va==0x2c00a830u);
  assert(sound_candidates[1].native_aarch64_va==0x13570u);
  assert(!normalize_candidate(-1).valid);
  assert(normalize_candidate(-1).token==invalid_token);
  assert(normalize_candidate(0).valid);
  assert(normalize_candidate(65534).valid);
  assert(!normalize_candidate(65535).valid);
  assert(!normalize_candidate(65536).valid);
  assert(candidate_token_range(65534u));
  assert(!candidate_token_range(65535u));
  assert(vgui_input_slot56==ABIStatus::ProvenBehaviorMismatch);
  assert(vgui_system_slot24==ABIStatus::ProvenBehaviorMismatch);
  assert(old_input_slot56_va==0x10007d90u && native_input_slot56_va==0x25ca0u);
  assert(old_system_slot24_va==0x1001cb60u && native_system_slot24_va==0x3de30u);
  for(const auto& e:sound_candidates) {
    assert(e.status==ABIStatus::FunctionalCandidateNeedsABIVerification);
    assert(!native_call_permitted(e.status));
  }
  assert(native_call_permitted(ABIStatus::ProvenABICompatible));
}
