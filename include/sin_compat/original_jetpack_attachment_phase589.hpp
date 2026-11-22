#pragma once
// Independently authored partial C++17 reference for authenticated SiN 2006
// Jetpack +0x1024 flying entity handle lifecycle. No original game code.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace sin_compat::jetpack_attachment_phase589 {
constexpr uint32_t kInvalid = 0xFFFFFFFFu;
constexpr std::string_view kNativeFlyingClass = "ai_base_physics_flying";

struct Vector3 {float x=0.0f, y=0.0f, z=0.0f;};
struct Owner {
  uint32_t encoded_handle=kInvalid;
  Vector3 position_36c{};
  // Native constructor offsets; named by original raw offsets, not guessed fields.
  uint32_t raw_fe0=0,raw_fe4=kInvalid,raw_fe8=kInvalid;
  uint32_t raw_ff4=kInvalid,raw_1000=kInvalid,raw_1008=0;
  uint32_t flying_1024=kInvalid,raw_1028=kInvalid;
};
struct FlyingEntity {
  uint32_t encoded_handle=kInvalid;
  uint32_t owner_handle_c50=kInvalid;
  Vector3 origin_c24{};
  Vector3 vector_c18{}; // DO NOT assume velocity/acceleration semantics.
};
struct Slot {void* entity=nullptr; uint32_t generation=0;};
struct HandleTable {
  std::vector<Slot> slots;
  static uint32_t encode(std::size_t slot,uint32_t generation) {
    return ((generation&0xFFFFFu)<<12) | (static_cast<uint32_t>(slot)&0xFFFu);
  }
  void* resolve_raw(uint32_t raw) const {
    if(raw==kInvalid) return nullptr;
    const auto index=static_cast<std::size_t>(raw&0xFFFu);
    if(index>=slots.size())return nullptr; // host-only bounds protection
    const auto& s=slots[index];
    return s.generation==(raw>>12)?s.entity:nullptr;
  }
  FlyingEntity* resolve_flying(uint32_t raw) const {
    return static_cast<FlyingEntity*>(resolve_raw(raw));
  }
};
inline uint32_t original_handle_setter(const FlyingEntity* target) {
  // Original 0x22363D20, virtual +0x08 returns pointer to encoded handle.
  return target?target->encoded_handle:kInvalid;
}
inline FlyingEntity* original_handle_resolver(const HandleTable& slots,uint32_t raw) {
  // Original 0x22363D50. See HandleTable for low-12 slot/high-20 serial.
  return slots.resolve_flying(raw);
}
inline void original_owner_link(FlyingEntity& target,const Owner& source) {
  // Original 0x22363DD0 stores raw handle at target+C50 and copies 12 bytes
  // from owner+36C into target+C24 using original 0x22269B30.
  target.owner_handle_c50=source.encoded_handle;
  target.origin_c24=source.position_36c;
}
inline void initialize_original_jetpack_fields(Owner& o) {
  // Original 0x22368F60, modeled only source-bounded member writes.
  o.raw_fe0=0;o.raw_fe4=kInvalid;o.raw_fe8=kInvalid;
  o.raw_ff4=kInvalid;o.raw_1000=kInvalid;
  o.flying_1024=kInvalid;o.raw_1028=kInvalid;o.raw_1008=0;
}

enum class Status {Attached,MissingOriginalFlyingEntity,UnresolvedNativeHandle,MissingOwnerHandle};
struct Result {Status status;int final_callback_result=0;};
struct Host {
  virtual ~Host()=default;
  virtual FlyingEntity* lookup_flying(std::string_view native_key,Owner& owner)=0;
  // Original 0x2214D6F0 is reached even when the handle is unresolved.
  virtual void call_native_setup(FlyingEntity* maybe_entity,int a,int b,int c)=0;
  // Original 0x22363DD0 owner link is reproduced directly in host model.
  virtual void before_owner_link(FlyingEntity& entity,Owner& owner)=0;
  // Original entity virtual +0x64 is invoked after link and re-resolve.
  virtual int call_native_finalize(FlyingEntity& entity)=0;
};
inline Result acquire_original_flying_entity(Owner& owner,const HandleTable& slots,Host& host) {
  // Original 0x22365EE0 calls its owner virtual +1C4 and 0x22144DA0
  // with the literal registration name; input/ABI effects abstracted.
  FlyingEntity* discovered=host.lookup_flying(kNativeFlyingClass,owner);
  owner.flying_1024=original_handle_setter(discovered);
  FlyingEntity* attached=original_handle_resolver(slots,owner.flying_1024);
  host.call_native_setup(attached,0,0,0);
  // The native code continues through null pointer dereferences; this guard
  // is host-only and does NOT claim to match original crash semantics.
  if(!attached)return {discovered?Status::UnresolvedNativeHandle:Status::MissingOriginalFlyingEntity,0};
  if(owner.encoded_handle==kInvalid)return {Status::MissingOwnerHandle,0};
  host.before_owner_link(*attached,owner);
  original_owner_link(*attached,owner);
  FlyingEntity* final_target=original_handle_resolver(slots,owner.flying_1024);
  if(!final_target)return {Status::UnresolvedNativeHandle,0};
  return {Status::Attached,host.call_native_finalize(*final_target)};
}
} // namespace sin_compat::jetpack_attachment_phase589