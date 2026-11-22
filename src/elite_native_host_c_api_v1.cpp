#define SIN_ELITE_NATIVE_HOST_C_EXPORT
#include "sin_compat/elite_native_host_c_api_v1.h"
#include "sin_compat/original_elite_grunt_spawn_phase623.hpp"
#include <cstdint>
#include <cstring>
#include <map>
#include <new>
#include <string_view>

namespace elite=sin_compat::original_elite_grunt_spawn_phase623;
namespace {
inline constexpr const char* kClass="npc_druglab_grunt_elite";

struct Guard final {
  bool& busy;
  bool acquired;
  explicit Guard(bool& b)noexcept:busy(b),acquired(!b){
    if(acquired)busy=true;
  }
  ~Guard()noexcept {if(acquired)busy=false;}
  explicit operator bool()const noexcept{return acquired;}
};
struct Slot {
  sin_elite_actor_v1 actor{};
  std::uint64_t owner=0;
  bool spawn_attempted=false;
  bool spawn_completed=false;
  std::uint32_t handle_fdc=SIN_ELITE_HANDLE_INVALID_V1;
  std::uint8_t predicate_fd9=0u;
};
bool valid_actor(sin_elite_actor_v1 actor)noexcept{
  return actor.index!=0 && actor.generation!=0;
}
bool valid_spawn_callbacks(const sin_elite_native_spawn_callbacks_v1* c)
    noexcept {
  return c && c->version==SIN_ELITE_NATIVE_ABI_V1 &&
    c->struct_size==sizeof(*c) &&
    c->assign_original_weapon_symbol &&
    c->native_virtual_68 &&
    c->native_virtual_1c_then_6c &&
    c->inherited_npc_spawn &&
    c->clear_byte_fd9 &&
    c->clear_byte_cb2_zero &&
    c->read_original_convar_float_28 &&
    c->virtual_1b4_then_write_field_9c &&
    c->virtual_1b0_then_write_field_98 &&
    c->commit_flags_b4 &&
    c->resolve_chaingun_encoded_handle &&
    c->write_field_fdc;
}
bool valid_event_callbacks(const sin_elite_native_event43_callbacks_v1* c)
    noexcept {
  return c && c->version==SIN_ELITE_NATIVE_ABI_V1 &&
    c->struct_size==sizeof(*c) &&
    c->resolve_serial_handle && c->original_gate43 &&
    c->publish_byte_fd9 && c->forward_inherited_event43;
}
std::uint32_t dirty_to_host(elite::DirtyPath marker)noexcept {
  switch(marker) {
    case elite::DirtyPath::NativeField200:
      return SIN_ELITE_DIRTY_NATIVE_200_V1;
    case elite::DirtyPath::EdictStateBit0:
      return SIN_ELITE_DIRTY_EDICT_BIT0_V1;
    case elite::DirtyPath::NoMarker:
      return SIN_ELITE_DIRTY_NONE_V1;
  }
  return SIN_ELITE_DIRTY_NONE_V1;
}
class SpawnAdapter final:public elite::IHost {
public:
  SpawnAdapter(const sin_elite_native_spawn_callbacks_v1& c,void* u)noexcept
     :c_(c),userdata_(u){}
  bool assign_original_weapon_symbol(std::string_view name)override {
    // The native original model only ever requests this source-backed
    // literal, with static NUL lifetime. Never forward an arbitrary view.
    return name==elite::kOriginalWeapon &&
           c_.assign_original_weapon_symbol(userdata_,elite::kOriginalWeapon)==1;
  }
  bool native_virtual_68()override{
    return c_.native_virtual_68(userdata_)==1;
  }
  bool native_virtual_1c_then_6c()override{
    return c_.native_virtual_1c_then_6c(userdata_)==1;
  }
  bool inherited_npc_spawn_2235F2A0()override{
    return c_.inherited_npc_spawn(userdata_)==1;
  }
  bool clear_byte_fd9()override{
    return c_.clear_byte_fd9(userdata_)==1;
  }
  bool clear_byte_cb2_zero()override{
    return c_.clear_byte_cb2_zero(userdata_)==1;
  }
  bool read_original_convar_float_28(float& out)override{
    return c_.read_original_convar_float_28(userdata_,&out)==1;
  }
  bool virtual_1b4_then_write_field_9c(std::int32_t n)override{
    return c_.virtual_1b4_then_write_field_9c(userdata_,n)==1;
  }
  bool virtual_1b0_then_write_field_98(std::int32_t n)override{
    return c_.virtual_1b0_then_write_field_98(userdata_,n)==1;
  }
  bool commit_flags_b4(std::uint32_t n,elite::DirtyPath d)override{
    return c_.commit_flags_b4(userdata_,n,dirty_to_host(d))==1;
  }
  int resolve_chaingun_encoded_handle(std::uint32_t& out)override{
    return c_.resolve_chaingun_encoded_handle(userdata_,&out);
  }
  bool write_field_fdc(std::uint32_t n)override {
    return c_.write_field_fdc(userdata_,n)==1;
  }
private:
  const sin_elite_native_spawn_callbacks_v1& c_;
  void* userdata_;
};
int32_t outcome(elite::Code c)noexcept {
  switch(c) {
    case elite::Code::Applied:return SIN_ELITE_NATIVE_OK;
    case elite::Code::UnsupportedClass:return SIN_ELITE_NATIVE_UNSUPPORTED_CLASS;
    case elite::Code::UnsupportedOriginalX87:
      return SIN_ELITE_NATIVE_UNSUPPORTED_X87;
    case elite::Code::Reentrant:return SIN_ELITE_NATIVE_REENTRANT;
    case elite::Code::HostRejected:return SIN_ELITE_NATIVE_HOST_REJECTED;
  }
  return SIN_ELITE_NATIVE_HOST_REJECTED;
}
} // namespace

/* This registry never owns/constructs/destroys a native game NPC. */
struct sin_elite_native_registry_v1 final {
  sin_elite_native_host_v1 host{};
  void* userdata=nullptr;
  bool busy=false;
  std::map<std::uint32_t,Slot> slots;
  sin_elite_native_registry_v1(const sin_elite_native_host_v1& h,void* u)
      :host(h),userdata(u){}
  bool confirm(const Slot& slot) {
    return host.confirm_existing_native_elite(
       userdata,kClass,slot.actor,slot.owner)==1;
  }
  int32_t lookup(sin_elite_actor_v1 actor,Slot*& out) noexcept {
    out=nullptr;
    if(!valid_actor(actor))return SIN_ELITE_NATIVE_INVALID_IDENTITY;
    auto it=slots.find(actor.index);
    if(it==slots.end())return SIN_ELITE_NATIVE_NOT_FOUND;
    if(it->second.actor.generation!=actor.generation)
      return SIN_ELITE_NATIVE_STALE_GENERATION;
    out=&it->second;
    return SIN_ELITE_NATIVE_OK;
  }
};

extern "C" {
uint32_t sin_elite_native_abi_version_v1(void) {
  return SIN_ELITE_NATIVE_ABI_V1;
}
sin_elite_native_registry_v1* sin_elite_native_create_v1(
    const sin_elite_native_host_v1* h,void* userdata) {
  if(!h || h->version!=SIN_ELITE_NATIVE_ABI_V1 ||
     h->struct_size!=sizeof(*h) || !h->confirm_existing_native_elite)
    return nullptr;
  try {
    return new(std::nothrow) sin_elite_native_registry_v1(*h,userdata);
  } catch(...) {return nullptr;}
}
int32_t sin_elite_native_destroy_v1(sin_elite_native_registry_v1* r) {
  if(!r)return SIN_ELITE_NATIVE_INVALID_ARGUMENT;
  if(r->busy)return SIN_ELITE_NATIVE_REENTRANT;
  delete r; // ONLY local model sessions, NEVER an engine entity.
  return SIN_ELITE_NATIVE_OK;
}
int32_t sin_elite_native_attach_v1(
    sin_elite_native_registry_v1* r,
    const sin_elite_native_attachment_v1* a) {
  if(!r || !a || a->version!=SIN_ELITE_NATIVE_ABI_V1 ||
     a->struct_size!=sizeof(*a) || !a->original_classname)
    return SIN_ELITE_NATIVE_INVALID_ARGUMENT;
  if(std::strcmp(a->original_classname,kClass)!=0)
    return SIN_ELITE_NATIVE_UNSUPPORTED_CLASS;
  if(!valid_actor(a->actor) || !a->owner_entity_token)
    return SIN_ELITE_NATIVE_INVALID_IDENTITY;
  Guard guard(r->busy);
  if(!guard)return SIN_ELITE_NATIVE_REENTRANT;
  try {
    if(r->slots.find(a->actor.index)!=r->slots.end())
      return SIN_ELITE_NATIVE_DUPLICATE_INDEX;
    Slot slot{};
    slot.actor=a->actor;
    slot.owner=a->owner_entity_token;
    if(!r->confirm(slot))return SIN_ELITE_NATIVE_HOST_REJECTED;
    r->slots.emplace(slot.actor.index,slot);
    return SIN_ELITE_NATIVE_OK;
  } catch(...) {return SIN_ELITE_NATIVE_HOST_REJECTED;}
}
int32_t sin_elite_native_spawn_v1(
    sin_elite_native_registry_v1* r,sin_elite_actor_v1 actor,
    const sin_elite_native_spawn_snapshot_v1* s,
    const sin_elite_native_spawn_callbacks_v1* callbacks,void* userdata,
    sin_elite_native_spawn_result_v1* out) {
  if(!r || !s || !out || s->version!=SIN_ELITE_NATIVE_ABI_V1 ||
     s->struct_size!=sizeof(*s) || !valid_spawn_callbacks(callbacks) ||
     out->version!=SIN_ELITE_NATIVE_ABI_V1 ||
     out->struct_size!=sizeof(*out))
    return SIN_ELITE_NATIVE_INVALID_ARGUMENT;
  out->failed_stage=0;
  out->flags_after=0;
  out->handle_fdc=SIN_ELITE_HANDLE_INVALID_V1;
  out->wrote_handle=0;
  Guard guard(r->busy);
  if(!guard)return SIN_ELITE_NATIVE_REENTRANT;
  Slot* slot=nullptr;
  const auto code=r->lookup(actor,slot);
  if(code!=SIN_ELITE_NATIVE_OK)return code;
  try {
    if(!r->confirm(*slot))return SIN_ELITE_NATIVE_HOST_REJECTED;
    if(slot->spawn_attempted)return SIN_ELITE_NATIVE_ALREADY_SPAWNED;
    slot->spawn_attempted=true; // one shot even on partial host failure
    SpawnAdapter adapter(*callbacks,userdata);
    elite::SpawnReference original_spawn(adapter);
    const elite::Snapshot input{
      kClass,
      s->current_weapon_symbol?s->current_weapon_symbol:"",
      s->original_field_9c,s->original_field_98,
      s->original_flags_b4,s->original_flag_1f0!=0,
      s->original_edict_16c_present!=0
    };
    const elite::Result result=original_spawn.apply(input);
    out->failed_stage=static_cast<std::int32_t>(result.failed_stage);
    out->flags_after=result.flags_after;
    out->handle_fdc=result.final_handle_fdc;
    out->wrote_handle=result.wrote_handle?1:0;
    if(result.ok()) {
      slot->spawn_completed=true;
      slot->handle_fdc=result.final_handle_fdc;
      slot->predicate_fd9=0u; // source Spawn cleared native byte
    }
    return outcome(result.code);
  } catch(...) {return SIN_ELITE_NATIVE_HOST_REJECTED;}
}
int32_t sin_elite_native_event43_v1(
    sin_elite_native_registry_v1* r,sin_elite_actor_v1 actor,
    float original_parameter,
    const sin_elite_native_event43_callbacks_v1* callbacks,
    void* userdata) {
  if(!r || !valid_event_callbacks(callbacks))
    return SIN_ELITE_NATIVE_INVALID_ARGUMENT;
  Guard guard(r->busy);
  if(!guard)return SIN_ELITE_NATIVE_REENTRANT;
  Slot* slot=nullptr;
  const auto code=r->lookup(actor,slot);
  if(code!=SIN_ELITE_NATIVE_OK)return code;
  try {
    if(!r->confirm(*slot))return SIN_ELITE_NATIVE_HOST_REJECTED;
    if(!slot->spawn_completed)return SIN_ELITE_NATIVE_SPAWN_REQUIRED;

    // Source Phase593 event 0x43: resolve original +0xFDC if valid,
    // gate even on null entity, publish +0xFD9, then optional inheritance.
    // No other original Elite animation events are implemented by this ABI.
    std::uint64_t resolved=0;
    if(slot->handle_fdc!=SIN_ELITE_HANDLE_INVALID_V1) {
      const auto lookup=callbacks->resolve_serial_handle(
          userdata,slot->handle_fdc,&resolved);
      if(lookup!=0 && lookup!=1)return SIN_ELITE_NATIVE_HOST_REJECTED;
      if(lookup==0)resolved=0;
    }
    const auto gate=callbacks->original_gate43(userdata,resolved);
    if(gate!=0 && gate!=1)return SIN_ELITE_NATIVE_HOST_REJECTED;
    const std::uint8_t predicate=(gate==1)?1u:0u;
    if(callbacks->publish_byte_fd9(userdata,predicate)!=1)
      return SIN_ELITE_NATIVE_HOST_REJECTED;
    slot->predicate_fd9=predicate; // native write occurred, even if forward fails
    if(!gate)return SIN_ELITE_NATIVE_EVENT_BLOCKED;
    if(callbacks->forward_inherited_event43(userdata,original_parameter)!=1)
      return SIN_ELITE_NATIVE_HOST_REJECTED;
    return SIN_ELITE_NATIVE_EVENT_FORWARDED;
  } catch(...) {return SIN_ELITE_NATIVE_HOST_REJECTED;}
}
int32_t sin_elite_native_snapshot_v1(
    sin_elite_native_registry_v1* r,sin_elite_actor_v1 actor,
    sin_elite_native_state_v1* out) {
  if(!r || !out || out->version!=SIN_ELITE_NATIVE_ABI_V1 ||
     out->struct_size!=sizeof(*out))
    return SIN_ELITE_NATIVE_INVALID_ARGUMENT;
  Guard guard(r->busy);
  if(!guard)return SIN_ELITE_NATIVE_REENTRANT;
  Slot* slot=nullptr;
  const auto code=r->lookup(actor,slot);
  if(code!=SIN_ELITE_NATIVE_OK)return code;
  try {
    if(!r->confirm(*slot))return SIN_ELITE_NATIVE_HOST_REJECTED;
    out->spawn_attempted=slot->spawn_attempted?1:0;
    out->spawn_completed=slot->spawn_completed?1:0;
    out->serial_handle_fdc=slot->handle_fdc;
    out->predicate_fd9=slot->predicate_fd9;
    out->reserved[0]=out->reserved[1]=out->reserved[2]=0;
    return SIN_ELITE_NATIVE_OK;
  } catch(...) {return SIN_ELITE_NATIVE_HOST_REJECTED;}
}
int32_t sin_elite_native_detach_v1(
    sin_elite_native_registry_v1* r,sin_elite_actor_v1 actor) {
  if(!r)return SIN_ELITE_NATIVE_INVALID_ARGUMENT;
  Guard guard(r->busy);
  if(!guard)return SIN_ELITE_NATIVE_REENTRANT;
  Slot* slot=nullptr;
  const auto code=r->lookup(actor,slot);
  if(code!=SIN_ELITE_NATIVE_OK)return code;
  // The real NPC can be dead here, so do NOT call host confirm.
  r->slots.erase(actor.index);
  return SIN_ELITE_NATIVE_OK;
}
} // extern C
