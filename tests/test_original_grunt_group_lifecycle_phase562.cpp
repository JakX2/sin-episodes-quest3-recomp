#include "sin_compat/original_grunt_group_lifecycle_phase562.hpp"
#include <cassert>
#include <string>
#include <vector>
using namespace sin_compat::original_grunt_group_lifecycle_phase562;
struct Host:IGroupHost {
  std::vector<std::string> events;
  void full_group_encountered(const Group&)override {events.push_back("full");}
  void evicted_last_member(Owner& who)override {events.push_back("evict:"+std::to_string(who.id));}
  void membership_virtual_0x690(Owner& who,const Group* group)override {
    events.push_back(std::string(group?"bind:":"unbind:")+std::to_string(who.id));
  }
  void survivor_virtual_0x750(Owner& who,Owner& removed)override {
    events.push_back("notify:"+std::to_string(who.id)+"<-"+std::to_string(removed.id));
  }
  void group_multi_member_followup(Group&,Owner& who)override {
    events.push_back("followup:"+std::to_string(who.id));
  }
  void missing_member_diagnostic(const Group&,const Owner& who)override {
    events.push_back("missing:"+std::to_string(who.id));
  }
};
int main() {
  static_assert(kOriginalAllocatedGroupBytes==120,"original 0x78 allocation");
  static_assert(kNativeMemberLimit==16,"original compare 0x10");
  Registry r;
  Group& a=r.find_or_create(900);
  assert(r.count()==1 && r.lookup(900)==&a);
  assert(&r.find_or_create(900)==&a && r.count()==1);
  assert(a.members.empty() && a.name_token==900);
  assert(a.unused_index_0x58==kOriginalInvalidIndex && a.refresh_deadline_0x74==0.0f);
  Group& b=r.find_or_create(901);
  assert(r.count()==2 && b.name_token==901);
  Host host;
  Owner x{1},y{2},blocked{3};
  blocked.blocked_field_0x8c=true;
  assert(!attach(a,nullptr,host));
  assert(!attach(a,&blocked,host));
  assert(host.events.empty() && a.members.empty());
  assert(attach(a,&x,host));
  assert(x.linked_group_field_0x9c4==&a && x.group_identity_field_0x9c8==900);
  assert(a.members.size()==1);
  assert((host.events==std::vector<std::string>{"bind:1"}));
  assert(!attach(a,&x,host));
  assert(attach(a,&y,host));
  assert((host.events==std::vector<std::string>{"bind:1","bind:2","followup:2"}));
  host.events.clear();
  assert(detach(a,&y,true,host));
  assert(y.linked_group_field_0x9c4==nullptr && y.group_identity_field_0x9c8==0);
  assert((host.events==std::vector<std::string>{"notify:1<-2","unbind:2"}));
  host.events.clear();
  assert(!detach(a,&y,false,host));
  assert((host.events==std::vector<std::string>{"missing:2"}));
  host.events.clear();
  // Moving a member to another group first detaches from previous group.
  assert(attach(b,&x,host));
  assert(a.members.empty()&&b.members.size()==1&&b.members[0]==&x);
  assert(x.linked_group_field_0x9c4==&b && x.group_identity_field_0x9c8==901);
  assert((host.events==std::vector<std::string>{"unbind:1","bind:1"}));
  host.events.clear();
  assert(detach(b,&x,false,host));
  assert((host.events==std::vector<std::string>{"unbind:1"}));
  assert(r.destroy_if_empty(900));
  assert(r.lookup(900)==nullptr && r.count()==1);
  assert(r.destroy_if_empty(901));
  assert(r.count()==0);

  // Native owner collection is capacity 16. Exact native forced-eviction
  // engine consequences are still unresolved; this host expires the last
  // owner safely so no synthetic entity keeps a dangling group pointer.
  Group cap(14);
  std::vector<Owner> members;
  members.reserve(18);
  for(unsigned i=0;i<16;i++)members.push_back(Owner{100u+i});
  for(int i=0;i<16;i++)assert(attach(cap,&members[i],host));
  assert(cap.members.size()==16);
  host.events.clear();
  Owner incoming{999};
  assert(attach(cap,&incoming,host));
  assert(cap.members.size()==16 && cap.members.back()==&incoming);
  assert(cap.members.front()==&members[0]);
  assert(members[15].linked_group_field_0x9c4==nullptr);
  assert((host.events==std::vector<std::string>{"full","evict:115","bind:999","followup:999"}));
  // The insertion rejection is a special owner state, not simply capacity.
  assert(!attach(cap,&blocked,host));
  host.events.clear();
  // Remove a middle owner without notification preserves other members.
  assert(detach(cap,&members[3],false,host));
  assert(cap.members.size()==15);
  assert(cap.members[3]==&members[4]);
  assert((host.events==std::vector<std::string>{"unbind:103"}));
}