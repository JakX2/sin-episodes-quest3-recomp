/* Phase615 synthetic DLL: all five C symbols exist, but ABI version is 2.
 * The original Windows server proxy MUST reject it without disturbing the
 * real original CreateInterface pointer or return codes.
 */
#define SIN_GRUNT_BEHAVIOR_C_EXPORT
#include "sin_compat/grunt_behavior_c_api_v1.h"

uint32_t sin_grunt_behavior_abi_version_v1(void){return 2u;}
sin_grunt_behavior_session_v1* sin_grunt_behavior_create_v1(
  const sin_grunt_behavior_host_v1* host,void* userdata,
  sin_grunt_actor_v1 actor,uint64_t token,uint32_t id3b,uint32_t id18,
  const sin_grunt_assault_state_v1* state){
  (void)host;(void)userdata;(void)actor;(void)token;
  (void)id3b;(void)id18;(void)state;
  return 0;
}
int32_t sin_grunt_behavior_destroy_v1(sin_grunt_behavior_session_v1* session){
  (void)session;return SIN_GRUNT_BEHAVIOR_INVALID;
}
int32_t sin_grunt_behavior_step_v1(sin_grunt_behavior_session_v1* session,
  float now,int32_t* selected){
  (void)session;(void)now;(void)selected;
  return SIN_GRUNT_BEHAVIOR_HOST_FAILED;
}
int32_t sin_grunt_behavior_snapshot_v1(sin_grunt_behavior_session_v1* session,
  sin_grunt_snapshot_v1* out){
  (void)session;(void)out;return SIN_GRUNT_BEHAVIOR_HOST_FAILED;
}
