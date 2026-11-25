#pragma once
#include <cstdint>
#include "native_server_npc_weapon_scene.h"
#include "native_server_cached_facial_think.h"
#include "native_npc_scene_playback.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_animation_scene_export[]="SiN_GetServerAnimationSceneApi";
struct ServerAnimationSceneBindings {
    void* context;
    int (__cdecl *lookup)(void*,void* actor,const char*,int*) noexcept;
    int (__cdecl *missing)(void*,void* actor,std::uint32_t choreo_actor,unsigned type,const char*) noexcept;
    int (__cdecl *remove)(void*,void* actor,int layer) noexcept;
    int (__cdecl *blend_out)(void*,void* actor,int layer,float duration,float delay) noexcept;
};
struct ServerAnimationSceneApi {
    std::uint32_t version,size;
    // 1 handled (accepted is original AL), 0 unrelated, -1 invalid/host failure.
    int (__cdecl *begin)(void* actor,void* record,unsigned,const void* event,unsigned,std::uint32_t choreo_actor,const ServerAnimationSceneBindings*,unsigned*) noexcept;
    int (__cdecl *stop)(void* actor,const void* record,unsigned,const void* event,unsigned,int immediate,int forced,const ServerAnimationSceneBindings*,unsigned*) noexcept;
    int (__cdecl *check)(const void* event,unsigned,unsigned*) noexcept;
};
using GetServerAnimationSceneApi=const ServerAnimationSceneApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_animation_layer_owner_export[]="SiN_GetServerAnimationLayerOwnerApi";
struct ServerAnimationLayerOwnerApi {
    std::uint32_t version,size;
    // Original 220f4e20 / 220f5120. Host supplies resolved sequence flags,
    // current global time and raw PE32 vector allocation/storage only.
    int (__cdecl *allocate)(void* actor,unsigned bytes,int sequence,int priority,float now,unsigned sequence_flags,
        const ChoreographyVectorBindings*,int* layer) noexcept;
    // Original 220f4060 and 220f3ff0; negative/inactive index is a no-op.
    int (__cdecl *remove)(void* actor,unsigned bytes,int layer,float now,const ChoreographyVectorBindings*) noexcept;
    int (__cdecl *blend_out)(void* actor,unsigned bytes,int layer,float duration,float delay,const ChoreographyVectorBindings*) noexcept;
};
using GetServerAnimationLayerOwnerApi=const ServerAnimationLayerOwnerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_animation_layer_tick_export[]="SiN_GetServerAnimationLayerTickApi";
struct ServerAnimationLayerTickBindings {
    ChoreographyVectorBindings vectors;
    void* context;
    // Original sequence cycle rate 220eee90; resolved by engine metadata.
    int (__cdecl *cycle_rate)(void*,void* actor,int sequence,double*) noexcept;
};
struct ServerAnimationLayerTickApi {
    std::uint32_t version,size;
    // 220f44e0 row loop after base animation advancement, 220f31e0
    // per-layer cycle/envelope. dt is original resolved frame interval.
    int (__cdecl *run)(void* actor,unsigned bytes,float dt,float now,const ServerAnimationLayerTickBindings*) noexcept;
};
using GetServerAnimationLayerTickApi=const ServerAnimationLayerTickApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_animation_frame_clock_export[]="SiN_GetServerAnimationFrameClockApi";
struct ServerAnimationFrameClockBindings {
    ServerAnimationLayerTickBindings layers;
    void* context;
    // 220f2160 m_flModelWidthScale transition and 220f17f0 base-cycle step.
    int (__cdecl *transition)(void*,void* actor) noexcept;
    int (__cdecl *duration)(void*,void* actor,int sequence,double*) noexcept;
    int (__cdecl *step)(void*,void* actor,float cycle_delta) noexcept;
};
struct ServerAnimationFrameClockApi {
    std::uint32_t version,size;
    int (__cdecl *interval)(const void* actor,unsigned,float now,float*) noexcept;
    int (__cdecl *base)(void* actor,unsigned,float now,const ServerAnimationFrameClockBindings*) noexcept;
    // NPC wrapper 22028280 clears +e08 bits 0/1 before overlay advancement.
    int (__cdecl *run)(void* actor,unsigned,float now,unsigned npc,const ServerAnimationFrameClockBindings*) noexcept;
};
using GetServerAnimationFrameClockApi=const ServerAnimationFrameClockApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_base_animation_cycle_export[]="SiN_GetServerBaseAnimationCycleApi";
struct ServerBaseAnimationCycleBindings {
    ChoreographyVectorBindings vectors;
    void* context;
    // 220eeec0, 220eede0, 220eece0: resolved sequence metadata.
    int (__cdecl *finish_threshold)(void*,void*,int,double*) noexcept;
    int (__cdecl *duration)(void*,void*,int,double*) noexcept;
    int (__cdecl *movement_distance)(void*,void*,int,double*) noexcept;
    // 2214b620 (physics dirty notification flag 8) and 220fc5a0 /
    // 22101af0 bone-cache refresh/invalidation with current clock.
    int (__cdecl *invalidate)(void*,void*,unsigned) noexcept;
    int (__cdecl *cache)(void*,void*,std::uint32_t,float) noexcept;
};
struct ServerBaseAnimationCycleApi {
    std::uint32_t version,size;
    // 220f17f0. Signed delta, sticky completion byte, base movement rate.
    int (__cdecl *step)(void* actor,unsigned bytes,float delta,float now,const ServerBaseAnimationCycleBindings*) noexcept;
};
using GetServerBaseAnimationCycleApi=const ServerBaseAnimationCycleApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_animation_layer_reserve_export[]="SiN_GetServerAnimationLayerReserveApi";
struct ServerAnimationLayerReserveApi {
    std::uint32_t version,size;
    // 220f4180/220f3960/220f2f40. Ensure exact requested minimum
    // capacity and reset dormant rows [logical count, target), without
    // changing logical count or clearing unrelated original fields.
    int (__cdecl *reserve)(void* actor,unsigned bytes,int target,float now,const ChoreographyVectorBindings*) noexcept;
};
using GetServerAnimationLayerReserveApi=const ServerAnimationLayerReserveApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_animation_scene_frame_export[]="SiN_GetServerAnimationSceneFrameApi";
struct ServerAnimationSceneFrameBindings {
    void* context;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // Optional override for 2210e450. Null selects the native absolute-tag warp.
    int (__cdecl *gesture_cycle)(void*,const void* event,float,float*) noexcept;
};
struct ServerAnimationSceneFrameInput {
    float scene_time,now,intensity;
    bool suppressed,first_sequence_frame;
    unsigned sequence_flags;
    double sequence_duration;
};
struct ServerAnimationSceneFrameApi {
    std::uint32_t version,size;
    // Post-initialization numeric stage only. Suppression and event intensity
    // are already resolved. No priority, NPC keepalive or layer allocation.
    int (__cdecl *update)(void* actor,unsigned,void* record,unsigned,const void* event,unsigned,
        const ServerAnimationSceneFrameInput*,const ServerAnimationSceneFrameBindings*,unsigned*) noexcept;
};
using GetServerAnimationSceneFrameApi=const ServerAnimationSceneFrameApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_gesture_tag_warp_export[]="SiN_GetServerGestureTagWarpApi";
struct ServerGestureTagWarpApi {
    std::uint32_t version,size;
    // Original 2210e450, bank0->bank1, valid finite sorted source tags.
    int (__cdecl *map)(const void* event,unsigned,float fraction,const ServerAnimationSceneFrameBindings*,float*) noexcept;
};
using GetServerGestureTagWarpApi=const ServerGestureTagWarpApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_gesture_metadata_export[]="SiN_GetServerGestureMetadataApi";
struct ServerGestureMetadata {
    // Sequence metadata was available, even if faceposer subsection absent.
    unsigned available;
    const char* type;      // null = absent
    const char* startloop; // null = default "loop", empty = explicit empty name
    const char* endloop;   // null = default "end"
};
struct ServerGestureMetadataApi {
    std::uint32_t version,size;
    // Metadata portion of 220f7db0 only, after successful repeat lookup.
    // Does not allocate layers or load model/sequence metadata.
    int (__cdecl *apply)(void* record,unsigned,void* event,unsigned,const ServerGestureMetadata*,unsigned suppressed,
        const ServerAnimationSceneFrameBindings*) noexcept;
};
using GetServerGestureMetadataApi=const ServerGestureMetadataApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_animation_record_export[]="SiN_GetServerNpcAnimationRecordApi";
struct ServerNpcAnimationRecordBindings {
    ServerNpcSceneRecordBindings npc;
    ServerAnimationSceneBindings animation;
};
struct ServerNpcAnimationRecordApi {
    std::uint32_t version,size;
    int (__cdecl *adapt)(const ServerNpcAnimationRecordBindings*,ActorEventRecordBindings*) noexcept;
};
using GetServerNpcAnimationRecordApi=const ServerNpcAnimationRecordApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_animation_priority_export[]="SiN_GetServerAnimationPriorityApi";
struct ServerAnimationPriorityApi {
    std::uint32_t version,size;
    // 22107170: channel index in choreography actor, -1 if absent.
    int (__cdecl *channel)(const void* choreo_actor,unsigned,std::uint32_t channel,
        const ServerAnimationSceneFrameBindings*,int*) noexcept;
    // 220f7cc0: preceding attached scenes' +34 count, stopping at current.
    int (__cdecl *prefix)(const void* actor,unsigned,std::uint32_t scene,
        const ServerAnimationSceneFrameBindings*,std::uint32_t*) noexcept;
    // 220f3a60: priority insertion into active layer order.
    int (__cdecl *reorder)(void* actor,unsigned,int layer,int priority,float now,
        const ServerAnimationSceneFrameBindings*) noexcept;
    // Recompute from record+14 and scene prefix when actor+600 is enabled.
    int (__cdecl *refresh)(void* actor,unsigned,const void* record,unsigned,float now,
        const ServerAnimationSceneFrameBindings*) noexcept;
};
using GetServerAnimationPriorityApi=const ServerAnimationPriorityApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_animation_scene_init_export[]="SiN_GetServerAnimationSceneInitApi";
struct ServerAnimationSceneInitBindings {
    ServerAnimationSceneBindings admission;
    ServerAnimationSceneFrameBindings frame;
    void* context;
    int (__cdecl *metadata)(void*,void*,int sequence,ServerGestureMetadata*) noexcept;
    int (__cdecl *sequence_gate)(void*,void*,unsigned*) noexcept;
    int (__cdecl *suppressed)(void*,void*,unsigned*) noexcept;
    // Engine allocation/initialization of active row, 220f5120.
    int (__cdecl *allocate_layer)(void*,void*,int sequence,int priority,int*) noexcept;
    int (__cdecl *sequence_info)(void*,void*,int sequence,unsigned* flags,double* duration) noexcept;
};
struct ServerAnimationSceneInitApi {
    std::uint32_t version,size;
    // 1 handled (accepted original initializer AL), 0 unrelated, -1 failure.
    // Only consumes deferred layer sentinel -2. No post-init frame or keepalive.
    int (__cdecl *initialize)(void* actor,unsigned,void* record,unsigned,void* event,unsigned,float scene_time,float now,
        const ServerAnimationSceneInitBindings*,unsigned*) noexcept;
};
using GetServerAnimationSceneInitApi=const ServerAnimationSceneInitApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_animation_think_export[]="SiN_GetServerNpcAnimationThinkApi";
struct ServerNpcAnimationThinkBindings {
    ServerCachedFacialThinkBindings facial; // optional paired bank/file cache
    ServerAnimationSceneInitBindings animation;
    void* context;
    // Resolve original NPC identity and condition 0x3c/1, activity +878.
    int (__cdecl *npc_state)(void*,void* actor,void** npc,unsigned* condition,int* activity) noexcept;
    int (__cdecl *keepalive)(void*,void* npc,float duration) noexcept; // optional override; null writes native deadline
    int (__cdecl *interrupted)(void*,void* actor) noexcept; // optional warning
};
struct ServerNpcAnimationThinkApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerNpcAnimationThinkBindings*) noexcept;
};
using GetServerNpcAnimationThinkApi=const ServerNpcAnimationThinkApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_animation_gate_export[]="SiN_GetServerNpcAnimationGateApi";
struct ServerNpcAnimationGateBindings {
    void* context;
    int (__cdecl *npc)(void*,void* actor,void** npc,unsigned* extent) noexcept;
    int (__cdecl *condition)(void*,void* npc,int condition,int flag,unsigned*) noexcept;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *can_enter)(void*,void* npc,unsigned*) noexcept; // virtual +450
    int (__cdecl *resolve)(void*,std::uint32_t handle,void**) noexcept;
    int (__cdecl *release_current)(void*,void* npc) noexcept; // 2203f2b0
    int (__cdecl *set_condition)(void*,void* npc,int condition) noexcept; // 2204fe40, return ignored
};
struct ServerNpcAnimationGateApi {
    std::uint32_t version,size;
    // Compatible with ServerAnimationSceneInitBindings.sequence_gate.
    int (__cdecl *admit)(void* bindings,void* actor,unsigned*) noexcept;
    // Original 22027e40, monotonic deadline at NPC+97c.
    int (__cdecl *keepalive)(void* npc,unsigned,float now,float duration) noexcept;
};
using GetServerNpcAnimationGateApi=const ServerNpcAnimationGateApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_target_scene_export[]="SiN_GetServerTargetSceneApi";
struct ServerTargetSceneBindings {
    void* context;
    int (__cdecl *npc)(void*,void* actor,void** npc) noexcept;
    int (__cdecl *resolve)(void*,std::uint32_t handle,void**) noexcept;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *look)(void*,void* npc,void* target,float weight,float duration,int flags) noexcept;
};
struct ServerTargetSceneApi {
    std::uint32_t version,size;
    // Type3 update 220f7020; missing NPC/target is accepted, not a host error.
    // Debug drawing after the look request is intentionally excluded.
    int (__cdecl *look_at)(void* actor,const void* record,unsigned,const void* event,unsigned,float,
        const ServerTargetSceneBindings*,unsigned*) noexcept;
};
using GetServerTargetSceneApi=const ServerTargetSceneApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_face_scene_export[]="SiN_GetServerFaceSceneApi";
struct ServerFaceSceneBindings {
    ServerTargetSceneBindings target; // target.look is NPC virtual +5ec here, not +63c
    void* context;
    int (__cdecl *admit)(void*,void* actor,unsigned*) noexcept;
    int (__cdecl *suppressed)(void*,void* actor,unsigned*) noexcept;
    int (__cdecl *target_yaw)(void*,void* actor,void* target,float*) noexcept;
};
struct ServerFaceSceneApi {
    std::uint32_t version,size;
    // CAI_BaseActor type8 update, 2202a24e; no debug drawing.
    int (__cdecl *update)(void* actor,unsigned,void* record,unsigned,const void* event,unsigned,float time,float now,
        const ServerFaceSceneBindings*,unsigned*) noexcept;
};
using GetServerFaceSceneApi=const ServerFaceSceneApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_target_think_export[]="SiN_GetServerNpcTargetThinkApi";
struct ServerNpcTargetThinkBindings {
    ServerNpcAnimationThinkBindings animation;
    ServerTargetSceneBindings look;
    ServerFaceSceneBindings face;
};
struct ServerBodyTurnBindings {
    void* context;
    int (__cdecl *condition)(void*,void* actor,int id,int flag,unsigned*) noexcept;
    int (__cdecl *suppressed)(void*,void* actor,unsigned*) noexcept;
    int (__cdecl *motor_yaw)(void*,void* actor,float yaw,float speed) noexcept;
};
struct ServerNpcTargetThinkApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerNpcTargetThinkBindings*) noexcept;
    // Separate original actor maintenance leaf 22029a10; not called by run.
    int (__cdecl *consume_body)(void*,unsigned,const ServerBodyTurnBindings*,unsigned*) noexcept;
};
using GetServerNpcTargetThinkApi=const ServerNpcTargetThinkApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_move_scene_export[]="SiN_GetServerMoveSceneApi";
struct ServerMoveSceneBindings {
    void* context;
    int (__cdecl *npc)(void*,void* actor,void** npc,unsigned* bytes) noexcept;
    int (__cdecl *admit)(void*,void* actor,unsigned*) noexcept;
    int (__cdecl *moving)(void*,void* npc,unsigned*) noexcept; // virtual +108
    int (__cdecl *cancel_navigation)(void*,void* npc,int clear) noexcept;
    int (__cdecl *has_goal)(void*,void* npc,unsigned*) noexcept; // 220740a0
    int (__cdecl *travel_time)(void*,void* npc,double*) noexcept; // virtual +5d4
    int (__cdecl *activity)(void*,const char*,int*) noexcept; // 220e6cb0
    int (__cdecl *intern)(void*,const char*,std::uint32_t*) noexcept;
};
struct ServerMoveSceneApi {
    std::uint32_t version,size;
    // Original 220f9570: ordinary rejection returned via accepted.
    int (__cdecl *begin)(void* actor,void* record,unsigned,const void* event,unsigned,void* target,float time,float now,
        const ServerMoveSceneBindings*,unsigned*) noexcept;
    int (__cdecl *check)(void* actor,const void* record,unsigned,const void* event,unsigned,float time,
        const ServerMoveSceneBindings*,unsigned*) noexcept;
    int (__cdecl *stop)(void* actor,const void* event,unsigned,int forced,const ServerMoveSceneBindings*,unsigned*) noexcept;
    // Original 220f5aa0, parameter2 first token and custom sequence fallback.
    int (__cdecl *activity)(void* npc,unsigned,const void* event,unsigned,const ServerMoveSceneBindings*,int*) noexcept;
};
using GetServerMoveSceneApi=const ServerMoveSceneApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_move_scene_frame_export[]="SiN_GetServerMoveSceneFrameApi";
struct ServerMoveSceneFrameBindings {
    ServerMoveSceneBindings move;
    void* context;
    int (__cdecl *resolve)(void*,std::uint32_t,void**) noexcept;
    int (__cdecl *handle)(void*,void* target,std::uint32_t*) noexcept;
    // Current task identifier, -1 when absent (2204e5b0).
    int (__cdecl *task)(void*,void* npc,int*) noexcept;
    int (__cdecl *hull_width)(void*,void* npc,float*) noexcept;
    // Target collision nearest-point relative to actor origin, horizontal components.
    int (__cdecl *horizontal_delta)(void*,void* actor,void* target,float*,float*) noexcept;
    // Engine navigator SetGoal with original type1/flags2; accepted is original AL.
    int (__cdecl *submit_goal)(void*,void* npc,void* target,int activity,float half_width,unsigned*) noexcept;
    int (__cdecl *goal_activity)(void*,void* npc,int activity) noexcept;
    int (__cdecl *goal_tolerance)(void*,void* npc,float tolerance) noexcept;
};
struct ServerMoveSceneFrameApi {
    std::uint32_t version,size;
    // 220f9a40, with debug overlays omitted. Return 1 handled, accepted is original AL.
    int (__cdecl *update)(void* actor,void* record,unsigned,const void* event,unsigned,float time,float now,
        const ServerMoveSceneFrameBindings*,unsigned*) noexcept;
};
using GetServerMoveSceneFrameApi=const ServerMoveSceneFrameApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_movement_scene_export[]="SiN_GetServerNpcMovementSceneApi";
struct ServerNpcMovementRecordBindings {
    ServerNpcAnimationRecordBindings animation;
    ServerMoveSceneBindings move;
};
struct ServerNpcMovementThinkBindings {
    ServerNpcTargetThinkBindings target;
    ServerMoveSceneFrameBindings move;
};
struct ServerNpcMovementSceneApi {
    std::uint32_t version,size;
    int (__cdecl *adapt)(const ServerNpcMovementRecordBindings*,ActorEventRecordBindings*) noexcept;
    int (__cdecl *run)(void*,unsigned,float,const ServerNpcMovementThinkBindings*) noexcept;
};
using GetServerNpcMovementSceneApi=const ServerNpcMovementSceneApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
