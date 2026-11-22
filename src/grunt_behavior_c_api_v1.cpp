#define SIN_GRUNT_BEHAVIOR_C_EXPORT
#include "sin_compat/grunt_behavior_c_api_v1.h"
#include "sin_compat/original_grunt_behavior_priority_phase606.hpp"
#include "sin_compat/original_grunt_assault_eligibility_phase610.hpp"
#include "sin_compat/original_grunt_assault_deactivate_phase611.hpp"
#include <cmath>
#include <cstdint>
#include <new>

namespace selection=sin_compat::grunt_behavior_priority_phase606;
namespace assault=sin_compat::grunt_assault_eligibility_phase610;
namespace shutdown=sin_compat::grunt_assault_deactivate_phase611;

struct sin_grunt_behavior_session_v1 {
  sin_grunt_behavior_host_v1 host{};
  void *user=nullptr;
  sin_grunt_actor_v1 actor{};
  std::uint64_t owner_token=0;
  std::uint32_t condition3b=0,condition18=0;
  assault::State state{};
  std::int32_t current=-1;
  bool in_api=false;
};

namespace {
struct CallbackFailure {std::int32_t status;};
static bool actor_valid(sin_grunt_actor_v1 a) noexcept {
  return a.index!=0 && a.generation!=0;
}
static bool callbacks_valid(const sin_grunt_behavior_host_v1& h) noexcept {
  return h.version==SIN_GRUNT_BEHAVIOR_ABI_V1 &&
    h.struct_size==sizeof(h) && h.actor_live && h.other_eligible &&
    h.owner_gate && h.has_condition && h.owner_target_nonnull &&
    h.serial_entity_present && h.primary_target_byte &&
    h.set_condition && h.resolve_entity && h.read_entity_link &&
    h.write_entity_link && h.behavior_lifecycle && h.owner_transition;
}
static bool truth(std::int32_t result) {
  if(result!=0 && result!=1)throw CallbackFailure{SIN_GRUNT_BEHAVIOR_HOST_FAILED};
  return result==1;
}
static void success(std::int32_t result) {
  if(result!=1)throw CallbackFailure{SIN_GRUNT_BEHAVIOR_HOST_FAILED};
}
static void live(sin_grunt_behavior_session_v1& s) {
  const auto v=s.host.actor_live(s.user,s.actor);
  if(v==0)throw CallbackFailure{SIN_GRUNT_BEHAVIOR_ACTOR_EXPIRED};
  if(v!=1)throw CallbackFailure{SIN_GRUNT_BEHAVIOR_HOST_FAILED};
}
struct ReentrantGuard {
  sin_grunt_behavior_session_v1 *s;
  bool enabled;
  explicit ReentrantGuard(sin_grunt_behavior_session_v1* ptr)
    :s(ptr),enabled(ptr && !ptr->in_api) {if(enabled)s->in_api=true;}
  ~ReentrantGuard(){if(enabled)s->in_api=false;}
  explicit operator bool() const noexcept{return enabled;}
  ReentrantGuard(const ReentrantGuard&)=delete;
  ReentrantGuard& operator=(const ReentrantGuard&)=delete;
};
static assault::State from_c(const sin_grunt_assault_state_v1& c) {
  assault::State s;
  s.handle_plus_0x14=c.handle_14;
  s.handle_plus_0x18=c.handle_18;
  s.raw.raw_plus_0x1c=c.raw_1c;
  s.raw.raw_plus_0x24=c.byte_24;
  s.raw.raw_plus_0x25=c.byte_25;
  s.byte_plus_0x26=c.byte_26;
  s.time_plus_0x28=c.time_28;
  s.deadline_plus_0x2c=c.deadline_2c;
  return s;
}
static sin_grunt_assault_state_v1 to_c(const assault::State& s) {
  return {s.handle_plus_0x14,s.handle_plus_0x18,s.raw.raw_plus_0x1c,
          s.raw.raw_plus_0x24,s.raw.raw_plus_0x25,s.byte_plus_0x26,0,
          s.time_plus_0x28,s.deadline_plus_0x2c};
}
class Adapter final: public selection::Host, public assault::Host,
                     public shutdown::Host {
  sin_grunt_behavior_session_v1& s;
  float now;
  void old_behavior(int old) {
    if(old<0)return;
    live(s);
    if(old==SIN_GRUNT_ASSAULT_INDEX_V1) {
      shutdown::State raw{s.state.handle_plus_0x18,
                          s.state.raw.raw_plus_0x24,
                          s.state.raw.raw_plus_0x25};
      // The original Assault +0x20 first clears these flags unconditionally.
      s.state.raw.raw_plus_0x24=0;
      s.state.raw.raw_plus_0x25=0;
      shutdown::deactivate(*this,raw,s.owner_token);
      return;
    }
    success(s.host.behavior_lifecycle(s.user,s.actor,
              static_cast<std::uint32_t>(old),2));
  }
public:
  Adapter(sin_grunt_behavior_session_v1& session,float time):s(session),now(time){}
  bool eligible(std::size_t i) override {
    live(s);
    if(i==SIN_GRUNT_ASSAULT_INDEX_V1)
      return assault::eligible(*this,s.state,now);
    return truth(s.host.other_eligible(s.user,s.actor,
                                      static_cast<std::uint32_t>(i)));
  }
  bool owner_accepts(std::size_t) override {
    // Original CNPC_MobGrunt vtable +0x77C -> 0x2216DDE0: always true.
    return true;
  }
  void transition_to(std::size_t index) override {
    live(s);
    const int new_index=static_cast<int>(index);
    const int old=s.current;
    if(old==new_index)return;
    s.current=new_index;
    success(s.host.behavior_lifecycle(s.user,s.actor,
            static_cast<std::uint32_t>(index),0));
    live(s);
    success(s.host.behavior_lifecycle(s.user,s.actor,
            static_cast<std::uint32_t>(index),1));
    old_behavior(old);
    live(s);
    success(s.host.owner_transition(s.user,s.actor,old,new_index));
  }
  void clear_current_and_notify_if_needed() override {
    live(s);
    const int old=s.current;
    if(old<0)return;
    s.current=-1;
    old_behavior(old);
    live(s);
    success(s.host.owner_transition(s.user,s.actor,old,-1));
  }
  bool owner_gate_0x450() override {
    return truth(s.host.owner_gate(s.user,s.actor));
  }
  bool owner_has_condition_0x3b() override {
    return truth(s.host.has_condition(s.user,s.actor,s.condition3b));
  }
  bool owner_virtual_0x154_nonnull() override {
    return truth(s.host.owner_target_nonnull(s.user,s.actor));
  }
  bool serial_checked_entity_present(std::uint32_t index,
                                      std::uint32_t serial) override {
    return truth(s.host.serial_entity_present(s.user,index,serial));
  }
  bool resolve_primary_target_field_0x3b9(std::uint32_t encoded,
                                            std::uint8_t& value) override {
    std::uint8_t field=0;
    const bool okay=truth(s.host.primary_target_byte(s.user,encoded,&field));
    if(okay)value=field;
    return okay;
  }
  void signal_condition_0x18() override {
    success(s.host.set_condition(s.user,s.actor,s.condition18));
  }
  bool resolve_entity(std::uint32_t encoded,
                      std::uint64_t& token) override {
    token=0;
    const bool okay=truth(s.host.resolve_entity(s.user,encoded,&token));
    return okay && token!=0;
  }
  std::uint32_t read_link_handle_plus_0x3bc(std::uint64_t target) override {
    std::uint32_t encoded=shutdown::kInvalidHandle;
    success(s.host.read_entity_link(s.user,target,&encoded));
    return encoded;
  }
  void write_link_handle_plus_0x3bc(std::uint64_t target,
                                     std::uint32_t value) override {
    success(s.host.write_entity_link(s.user,target,value));
  }
};
} // namespace

extern "C" {
uint32_t sin_grunt_behavior_abi_version_v1(void) {
  return SIN_GRUNT_BEHAVIOR_ABI_V1;
}
sin_grunt_behavior_session_v1 *sin_grunt_behavior_create_v1(
    const sin_grunt_behavior_host_v1* h,void* user,sin_grunt_actor_v1 actor,
    std::uint64_t owner_token,std::uint32_t condition3b,
    std::uint32_t condition18,const sin_grunt_assault_state_v1* initial) {
  if(!h || !initial || !actor_valid(actor) || owner_token==0 ||
     !callbacks_valid(*h) || condition3b==condition18 ||
     condition3b==UINT32_MAX || condition18==UINT32_MAX ||
     !std::isfinite(initial->time_28) ||
     !std::isfinite(initial->deadline_2c))return nullptr;
  try {
    auto* s=new(std::nothrow)sin_grunt_behavior_session_v1;
    if(!s)return nullptr;
    s->host=*h;s->user=user;s->actor=actor;s->owner_token=owner_token;
    s->condition3b=condition3b;s->condition18=condition18;
    s->state=from_c(*initial);
    return s;
  } catch(...) {return nullptr;}
}
int32_t sin_grunt_behavior_destroy_v1(sin_grunt_behavior_session_v1* s) {
  if(!s)return SIN_GRUNT_BEHAVIOR_INVALID;
  if(s->in_api)return SIN_GRUNT_BEHAVIOR_REENTRANT;
  delete s;return SIN_GRUNT_BEHAVIOR_OK;
}
int32_t sin_grunt_behavior_step_v1(sin_grunt_behavior_session_v1* s,
                                   float now,int32_t* out_index) {
  if(!s)return SIN_GRUNT_BEHAVIOR_INVALID;
  ReentrantGuard guard(s);
  if(!guard)return SIN_GRUNT_BEHAVIOR_REENTRANT;
  if(!out_index || !std::isfinite(now))return SIN_GRUNT_BEHAVIOR_INVALID;
  *out_index=s->current;
  try {
    live(*s);
    Adapter adapter(*s,now);
    const auto result=selection::choose_registered_behavior(adapter);
    *out_index=s->current;
    return result==selection::SelectionResult::Selected ?
      SIN_GRUNT_BEHAVIOR_OK:SIN_GRUNT_BEHAVIOR_NO_ELIGIBLE;
  }catch(const CallbackFailure& failure) {
    *out_index=s->current;
    return failure.status;
  }catch(...) {
    *out_index=s->current;
    return SIN_GRUNT_BEHAVIOR_HOST_FAILED;
  }
}
int32_t sin_grunt_behavior_snapshot_v1(sin_grunt_behavior_session_v1* s,
                                       sin_grunt_snapshot_v1* out) {
  if(!s)return SIN_GRUNT_BEHAVIOR_INVALID;
  ReentrantGuard guard(s);
  if(!guard)return SIN_GRUNT_BEHAVIOR_REENTRANT;
  if(!out)return SIN_GRUNT_BEHAVIOR_INVALID;
  *out={s->current,to_c(s->state)};
  return SIN_GRUNT_BEHAVIOR_OK;
}
} // extern C
