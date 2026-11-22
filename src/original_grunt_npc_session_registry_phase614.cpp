#include "sin_compat/original_grunt_npc_session_registry_phase614.hpp"
#include <new>

namespace sin_compat::grunt_npc_session_registry_phase614 {

namespace {
std::string_view canonical_class(std::string_view name) noexcept {
  // Literal-backed names keep their lifetime after the caller's buffer dies.
  if(name=="npc_druglab_grunt_pistol")return "npc_druglab_grunt_pistol";
  if(name=="npc_druglab_grunt_scattergun")return "npc_druglab_grunt_scattergun";
  if(name=="npc_druglab_grunt_assault_rifle")return "npc_druglab_grunt_assault_rifle";
  return {};
}
} // namespace
bool Registry::supports_original_class(std::string_view name) noexcept {
  return !canonical_class(name).empty();
}
int Registry::reattest(const Slot& slot,sin_grunt_actor_v1 actor) noexcept {
  try {
    // Not just an index check: the real host must still own exactly this
    // classname, generation, and opaque NPC identity token.
    return host_.confirmed_existing_native_npc(slot.original_classname,actor,
                                               slot.owner_entity_token)?1:0;
  } catch(...) {
    return -1; // Fail closed; never execute behavior after a host exception.
  }
}
Registry::~Registry() noexcept {
  // This owns only independently allocated behavior sessions, never entities.
  for(auto& slot:slots_) (void)sin_grunt_behavior_destroy_v1(slot.second.session);
}
Status Registry::attach_existing_native(const Attachment& a) noexcept {
  Guard guard(busy_);
  if(!guard)return Status::Reentrant;
  if(!supports_original_class(a.original_classname))return Status::UnsupportedClass;
  if(a.actor.index==0 || a.actor.generation==0 || a.owner_entity_token==0)
    return Status::InvalidIdentity;
  if(slots_.find(a.actor.index)!=slots_.end())
    return Status::DuplicateIndex; // even if another generation owns this slot
  try {
    // Merely knowing the classname is insufficient. No fake Grunt spawn.
    if(!host_.confirmed_existing_native_npc(a.original_classname,a.actor,
                                            a.owner_entity_token))
      return Status::HostRejected;
    if(!a.callbacks.actor_live ||
       a.callbacks.actor_live(a.callback_userdata,a.actor)!=1)
      return Status::HostRejected;
    auto* session=sin_grunt_behavior_create_v1(
        &a.callbacks,a.callback_userdata,a.actor,a.owner_entity_token,
        a.registered_condition_3b,a.registered_condition_18,
        &a.initial_assault);
    if(!session)return Status::HostRejected;
    try {
      slots_.emplace(a.actor.index,Slot{a.actor.generation,session,
          a.owner_entity_token,canonical_class(a.original_classname)});
    } catch(...) {
      (void)sin_grunt_behavior_destroy_v1(session);
      throw;
    }
    return Status::Attached;
  } catch(const std::bad_alloc&) {
    return Status::AllocationFailed;
  } catch(...) {
    return Status::HostRejected;
  }
}
Advance Registry::step(sin_grunt_actor_v1 actor,float now) noexcept {
  Guard guard(busy_);
  if(!guard)return {Status::Reentrant,-1,SIN_GRUNT_BEHAVIOR_REENTRANT};
  auto it=slots_.find(actor.index);
  if(it==slots_.end())return {Status::NotFound,-1,SIN_GRUNT_BEHAVIOR_INVALID};
  if(it->second.generation!=actor.generation)
    return {Status::StaleGeneration,-1,SIN_GRUNT_BEHAVIOR_INVALID};
  const int confirmed=reattest(it->second,actor);
  if(confirmed!=1)
    return {Status::HostRejected,-1,confirmed==0
            ?SIN_GRUNT_BEHAVIOR_ACTOR_EXPIRED:SIN_GRUNT_BEHAVIOR_HOST_FAILED};
  int32_t selected=-1;
  const auto code=sin_grunt_behavior_step_v1(it->second.session,now,&selected);
  return {code==SIN_GRUNT_BEHAVIOR_OK||code==SIN_GRUNT_BEHAVIOR_NO_ELIGIBLE
          ?Status::Advanced:Status::HostRejected,selected,code};
}
Status Registry::snapshot(sin_grunt_actor_v1 actor,
                          sin_grunt_snapshot_v1& out) noexcept {
  Guard guard(busy_);
  if(!guard)return Status::Reentrant;
  auto it=slots_.find(actor.index);
  if(it==slots_.end())return Status::NotFound;
  if(it->second.generation!=actor.generation)return Status::StaleGeneration;
  // A snapshot of a recycled NPC slot is misleading even without AI effects.
  if(reattest(it->second,actor)!=1)return Status::HostRejected;
  return sin_grunt_behavior_snapshot_v1(it->second.session,&out)==
      SIN_GRUNT_BEHAVIOR_OK?Status::Advanced:Status::HostRejected;
}
Status Registry::detach(sin_grunt_actor_v1 actor) noexcept {
  Guard guard(busy_);
  if(!guard)return Status::Reentrant;
  auto it=slots_.find(actor.index);
  if(it==slots_.end())return Status::NotFound;
  if(it->second.generation!=actor.generation)return Status::StaleGeneration;
  if(sin_grunt_behavior_destroy_v1(it->second.session)!=SIN_GRUNT_BEHAVIOR_OK)
    return Status::HostRejected;
  slots_.erase(it);
  return Status::Detached;
}
} // namespace sin_compat::grunt_npc_session_registry_phase614
