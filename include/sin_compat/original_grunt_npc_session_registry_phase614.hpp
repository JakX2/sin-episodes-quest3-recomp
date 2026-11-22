#pragma once
// Phase614: ATTACH only to real host-owned native Grunt NPCs. No fake
// classname factory, Source vtable, original 2006 pointer or entity allocation.
#include "sin_compat/grunt_behavior_c_api_v1.h"
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <unordered_map>

namespace sin_compat::grunt_npc_session_registry_phase614 {

enum class Status : std::uint8_t {
  Attached, Advanced, Detached, UnsupportedClass, InvalidIdentity,
  HostRejected, DuplicateIndex, StaleGeneration, NotFound,
  Reentrant, AllocationFailed
};
struct Attachment {
  std::string_view original_classname;
  sin_grunt_actor_v1 actor{};
  std::uint64_t owner_entity_token=0;
  std::uint32_t registered_condition_3b=0;
  std::uint32_t registered_condition_18=0;
  sin_grunt_assault_state_v1 initial_assault{};
  sin_grunt_behavior_host_v1 callbacks{};
  void* callback_userdata=nullptr;
};
struct Advance {
  Status status=Status::NotFound;
  std::int32_t selected=-1;
  std::int32_t underlying_abi_status=SIN_GRUNT_BEHAVIOR_INVALID;
};

// The real engine owns the actor object. The registrar NEVER constructs
// one. The host's confirmation must validate the actual NPC's classname,
// native factory ownership and full (index,generation) lifetime.
class NativeHost {
 public:
  virtual ~NativeHost()=default;
  virtual bool confirmed_existing_native_npc(std::string_view classname,
      sin_grunt_actor_v1 actor,std::uint64_t owner_entity_token)=0;
};

class Registry final {
 public:
  explicit Registry(NativeHost& host) noexcept:host_(host){}
  ~Registry() noexcept;
  Registry(const Registry&)=delete;
  Registry& operator=(const Registry&)=delete;
  static bool supports_original_class(std::string_view name) noexcept;
  Status attach_existing_native(const Attachment& desc) noexcept;
  Advance step(sin_grunt_actor_v1 actor,float now) noexcept;
  Status snapshot(sin_grunt_actor_v1 actor,
                  sin_grunt_snapshot_v1& out) noexcept;
  // Detach is allowed after NPC death, since the C session owns no engine
  // entity. It always requires the exact generation to prevent stale teardown.
  Status detach(sin_grunt_actor_v1 actor) noexcept;
  std::size_t count() const noexcept{return slots_.size();}
 private:
  struct Slot {std::uint32_t generation;sin_grunt_behavior_session_v1* session;};
  class Guard final {
   public:
    explicit Guard(bool& flag) noexcept:flag_(flag),entered_(!flag) {
      if(entered_)flag_=true;
    }
    ~Guard(){if(entered_)flag_=false;}
    explicit operator bool() const noexcept{return entered_;}
   private:bool& flag_;bool entered_;
  };
  NativeHost& host_;
  std::unordered_map<std::uint32_t,Slot> slots_;
  bool busy_=false;
};
} // namespace sin_compat::grunt_npc_session_registry_phase614
