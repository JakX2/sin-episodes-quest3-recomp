#define SIN_GRUNT_NATIVE_HOST_C_EXPORT
#include "sin_compat/grunt_native_host_c_api_v1.h"
#include "sin_compat/original_grunt_npc_session_registry_phase614.hpp"
#include <cstdint>
#include <new>
#include <string_view>

namespace native=sin_compat::grunt_npc_session_registry_phase614;

/* Deliberately separate from the Phase613 five-function behavior ABI.
 * This registry owns only behavior sessions, NEVER real Source entities.
 */
struct sin_grunt_native_registry_v1 final: native::NativeHost {
  sin_grunt_native_host_v1 host{};
  void* userdata=nullptr;
  native::Registry registry;
  bool busy=false;
  sin_grunt_native_registry_v1(const sin_grunt_native_host_v1& h,void* u)
      :host(h),userdata(u),registry(*this){}
  bool confirmed_existing_native_npc(
      std::string_view name,sin_grunt_actor_v1 actor,
      std::uint64_t owner_token) override {
    // The host receives an immutable, literal-backed NUL terminated name.
    const char* literal=nullptr;
    if(name=="npc_druglab_grunt")
      literal="npc_druglab_grunt";
    else if(name=="npc_druglab_grunt_pistol")
      literal="npc_druglab_grunt_pistol";
    else if(name=="npc_druglab_grunt_scattergun")
      literal="npc_druglab_grunt_scattergun";
    else if(name=="npc_druglab_grunt_assault_rifle")
      literal="npc_druglab_grunt_assault_rifle";
    if(!literal)return false;
    const int32_t result=host.confirm_existing_native_npc(
        userdata,literal,actor,owner_token);
    if(result!=0 && result!=1)throw -1; // Registry catches C++ errors.
    return result==1;
  }
};

namespace {
struct Guard final {
  bool& busy;
  bool entered;
  explicit Guard(bool& b) noexcept:busy(b),entered(!b) {
    if(entered)busy=true;
  }
  ~Guard(){if(entered)busy=false;}
  explicit operator bool() const noexcept{return entered;}
};
int32_t status(native::Status s) noexcept {
  switch(s) {
    case native::Status::Attached:
    case native::Status::Advanced:
    case native::Status::Detached:return SIN_GRUNT_NATIVE_OK;
    case native::Status::UnsupportedClass:return SIN_GRUNT_NATIVE_UNSUPPORTED_CLASS;
    case native::Status::InvalidIdentity:return SIN_GRUNT_NATIVE_INVALID_IDENTITY;
    case native::Status::HostRejected:return SIN_GRUNT_NATIVE_HOST_REJECTED;
    case native::Status::DuplicateIndex:return SIN_GRUNT_NATIVE_DUPLICATE_INDEX;
    case native::Status::StaleGeneration:return SIN_GRUNT_NATIVE_STALE_GENERATION;
    case native::Status::NotFound:return SIN_GRUNT_NATIVE_NOT_FOUND;
    case native::Status::Reentrant:return SIN_GRUNT_NATIVE_REENTRANT;
    case native::Status::AllocationFailed:return SIN_GRUNT_NATIVE_ALLOCATION_FAILED;
  }
  return SIN_GRUNT_NATIVE_HOST_REJECTED;
}
} // namespace

extern "C" {
uint32_t sin_grunt_native_abi_version_v1(void) {
  return SIN_GRUNT_NATIVE_ABI_V1;
}
sin_grunt_native_registry_v1* sin_grunt_native_create_v1(
    const sin_grunt_native_host_v1* h,void* user) {
  if(!h || h->version!=SIN_GRUNT_NATIVE_ABI_V1 ||
     h->struct_size!=sizeof(*h) || !h->confirm_existing_native_npc)
    return nullptr;
  try {
    return new(std::nothrow) sin_grunt_native_registry_v1(*h,user);
  } catch(...) {
    return nullptr;
  }
}
int32_t sin_grunt_native_destroy_v1(sin_grunt_native_registry_v1* r) {
  if(!r)return SIN_GRUNT_NATIVE_INVALID_ARGUMENT;
  if(r->busy)return SIN_GRUNT_NATIVE_REENTRANT;
  delete r; // Frees portable sessions only, never engine entities.
  return SIN_GRUNT_NATIVE_OK;
}
int32_t sin_grunt_native_attach_v1(
    sin_grunt_native_registry_v1* r,
    const sin_grunt_native_attachment_v1* a) {
  if(!r || !a || a->version!=SIN_GRUNT_NATIVE_ABI_V1 ||
     a->struct_size!=sizeof(*a) || !a->original_classname)
    return SIN_GRUNT_NATIVE_INVALID_ARGUMENT;
  Guard g(r->busy);
  if(!g)return SIN_GRUNT_NATIVE_REENTRANT;
  try {
    native::Attachment d{};
    d.original_classname=a->original_classname;
    d.actor=a->actor;
    d.owner_entity_token=a->owner_entity_token;
    d.registered_condition_3b=a->registered_condition_3b;
    d.registered_condition_18=a->registered_condition_18;
    d.initial_assault=a->initial_assault;
    d.callbacks=a->behavior_callbacks;
    d.callback_userdata=a->behavior_userdata;
    return status(r->registry.attach_existing_native(d));
  } catch(...) {
    return SIN_GRUNT_NATIVE_HOST_REJECTED;
  }
}
int32_t sin_grunt_native_step_v1(
    sin_grunt_native_registry_v1* r,sin_grunt_actor_v1 actor,
    float now,int32_t* selected,int32_t* underlying) {
  if(!r || !selected || !underlying)return SIN_GRUNT_NATIVE_INVALID_ARGUMENT;
  *selected=-1;
  *underlying=SIN_GRUNT_BEHAVIOR_INVALID;
  Guard g(r->busy);
  if(!g)return SIN_GRUNT_NATIVE_REENTRANT;
  const auto result=r->registry.step(actor,now);
  *selected=result.selected;
  *underlying=result.underlying_abi_status;
  return status(result.status);
}
int32_t sin_grunt_native_snapshot_v1(
    sin_grunt_native_registry_v1* r,sin_grunt_actor_v1 actor,
    sin_grunt_snapshot_v1* out) {
  if(!r || !out)return SIN_GRUNT_NATIVE_INVALID_ARGUMENT;
  Guard g(r->busy);
  if(!g)return SIN_GRUNT_NATIVE_REENTRANT;
  return status(r->registry.snapshot(actor,*out));
}
int32_t sin_grunt_native_detach_v1(
    sin_grunt_native_registry_v1* r,sin_grunt_actor_v1 actor) {
  if(!r)return SIN_GRUNT_NATIVE_INVALID_ARGUMENT;
  Guard g(r->busy);
  if(!g)return SIN_GRUNT_NATIVE_REENTRANT;
  return status(r->registry.detach(actor));
}
} // extern C
