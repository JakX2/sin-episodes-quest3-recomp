#pragma once
// Bounded, original-free host reference of original SiN 2006 linked group
// construction, membership and leave order. Not an MSVC x86 group object.
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace sin_compat::original_grunt_group_lifecycle_phase562 {

inline constexpr std::size_t kOriginalAllocatedGroupBytes=0x78;
inline constexpr std::size_t kNativeMemberLimit=0x10;
inline constexpr std::uint32_t kOriginalInvalidIndex=0xFFFFFFFFu;

struct Group;
struct Owner {
  std::uint32_t id=0;
  bool blocked_field_0x8c=false;
  std::uint32_t group_identity_field_0x9c8=0;
  Group* linked_group_field_0x9c4=nullptr;
};

struct Group {
  std::uint32_t name_token=0;
  std::vector<Owner*> members;
  std::uint32_t unused_index_0x58=kOriginalInvalidIndex;
  float refresh_deadline_0x74=0.0f;
  std::size_t cached_records_0x6c=0;
  explicit Group(std::uint32_t name):name_token(name) {}
};

struct IGroupHost {
  virtual ~IGroupHost()=default;
  virtual void full_group_encountered(const Group& group)=0;
  virtual void evicted_last_member(Owner& previous)=0;
  virtual void membership_virtual_0x690(Owner& owner,const Group* group)=0;
  virtual void survivor_virtual_0x750(Owner& member,Owner& removed)=0;
  virtual void group_multi_member_followup(Group& group,Owner& joined)=0;
  virtual void missing_member_diagnostic(const Group& group,const Owner& owner)=0;
};

// Source-bounded host translation of 0x220AA0D0. Native semantics require
// serial-checked EHANDLE resolution, vector compaction and opaque virtual
// dispatch. We keep observable callback order; all reference pointers are
// owned by the host, not pointers into the 2006 retail process.
inline bool detach(Group& group,Owner* owner,bool notify_survivors,IGroupHost& host){
  if (!owner) return false;
  std::size_t index=group.members.size();
  for(std::size_t i=0;i<group.members.size();++i){
    if(group.members[i]==owner){index=i;break;}
  }
  if(index==group.members.size()) {
    host.missing_member_diagnostic(group,*owner);
    return false;
  }
  group.members.erase(group.members.begin()+static_cast<std::ptrdiff_t>(index));
  if(notify_survivors) {
    for(auto* member:group.members)if(member)
      host.survivor_virtual_0x750(*member,*owner);
  }
  host.membership_virtual_0x690(*owner,nullptr);
  owner->linked_group_field_0x9c4=nullptr;
  owner->group_identity_field_0x9c8=0;
  return true;
}

// Source-bounded owner admission via 0x220AA1B0. Exact virtual +0x678 and
// related-entity target iteration are represented only by the final hook.
// Native 0x220A8670's full-slot eviction semantics are still unresolved:
// removing it from our host representation is a defensive, explicit choice.
inline bool attach(Group& group,Owner* owner,IGroupHost& host){
  if(!owner||owner->blocked_field_0x8c||owner->linked_group_field_0x9c4==&group)
    return false;
  if(owner->linked_group_field_0x9c4) {
    if(!detach(*owner->linked_group_field_0x9c4,owner,false,host))
      return false; // host consistency guard, not a native failure branch
  }
  if(group.members.size()==kNativeMemberLimit){
    host.full_group_encountered(group);
    Owner* removed=group.members.back();
    group.members.pop_back();
    if(removed){
      removed->linked_group_field_0x9c4=nullptr; // host-side consistency
      removed->group_identity_field_0x9c8=0;
      host.evicted_last_member(*removed);
    }
  }
  group.members.push_back(owner);
  owner->linked_group_field_0x9c4=&group;
  host.membership_virtual_0x690(*owner,&group);
  owner->group_identity_field_0x9c8=group.name_token;
  if(group.members.size()>1)host.group_multi_member_followup(group,*owner);
  return true;
}

class Registry {
 public:
  Group* lookup(std::uint32_t name) const {
    for(const auto& x:groups_) if(x->name_token==name)return x.get();
    return nullptr;
  }
  Group& find_or_create(std::uint32_t name){
    if(auto* p=lookup(name)) return *p;
    groups_.push_back(std::make_unique<Group>(name));
    return *groups_.back();
  }
  bool destroy_if_empty(std::uint32_t name){
    for(auto it=groups_.begin();it!=groups_.end();++it){
      if((*it)->name_token==name){
        if(!(*it)->members.empty())return false;
        groups_.erase(it);return true;
      }
    }
    return false;
  }
  std::size_t count() const {return groups_.size();}
 private:
  std::vector<std::unique_ptr<Group>> groups_;
};

} // namespace sin_compat::original_grunt_group_lifecycle_phase562