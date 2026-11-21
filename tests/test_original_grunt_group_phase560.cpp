#include "sin_compat/original_grunt_group_phase560.hpp"
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
using namespace sin_compat::original_grunt_group_phase560;

struct Restriction : IRestrictionHost {
  std::vector<std::string> actions;
  std::int32_t select_category=17;
  bool allow=true;
  std::int32_t owner_virtual_0x154(std::int32_t a,std::int32_t b,
                                   std::int32_t& index) override {
    assert(a==b);
    actions.push_back("owner:"+std::to_string(a));
    index=-1;
    return select_category;
  }
  bool linked_group_0x220AAEB0(std::int32_t category,std::int32_t a,
                                std::int32_t b,std::int32_t& index) override {
    actions.push_back("reserve:"+std::to_string(category));
    assert(a==b);
    if (allow) index=a;
    return allow;
  }
};
struct Lifecycle : IGroupLifecycle {
  std::vector<std::string> actions;
  std::uint32_t group=0;
  std::uint32_t find_group(std::uint32_t x) override {
    actions.push_back("find:"+std::to_string(x));return group;
  }
  std::uint32_t create_group(std::uint32_t x) override {
    actions.push_back("create:"+std::to_string(x));return 88;
  }
  void attach_to_owner(std::uint32_t x) override {actions.push_back("attach:"+std::to_string(x));}
  void detach_from_owner(std::uint32_t x) override {actions.push_back("detach:"+std::to_string(x));}
};
struct Dispatch : IDispatchHost {
  std::vector<std::string> calls;
  std::uint32_t accepted=0;
  void activate_special_entity(const Member& x) override {
    calls.push_back("activate:"+std::to_string(x.native_handle));
  }
  bool dispatch_candidate(const Member& x) override {
    calls.push_back("dispatch:"+std::to_string(x.native_handle));
    return x.native_handle==accepted;
  }
};
static Member make_member(int handle,bool live,bool ok,bool blocked,int metric,
                          bool can_dispatch=true,bool owner=false,bool special=false) {
  Member x;
  x.native_handle=static_cast<unsigned>(handle);
  x.serial_resolves=live;x.state_0x204_nonzero=ok;
  x.component_virtual_0x2c_allows=false;x.virtual_0x694_rejects=blocked;
  x.metric_scalar_0x0c=metric;x.field_0x94_nonzero=can_dispatch;
  x.is_owner=owner;x.spawnflag_0x800=special;
  return x;
}
int main() {
  Restriction r;
  int slot=23;
  assert(permits_condition(false,0x0c,slot,r));
  assert(slot==23 && r.actions.empty());
  assert(permits_condition(true,0x0e,slot,r));
  assert(slot==0x0e);
  assert((r.actions==std::vector<std::string>{"owner:14","reserve:17"}));
  r.allow=false;
  assert(!permits_condition(true,0x0c,slot,r));
  assert((r.actions==std::vector<std::string>{"owner:14","reserve:17","owner:12","reserve:17"}));

  Reservations reservations;
  int selected=-1;
  assert(reservations.admit(5,8,10,selected));
  assert(selected==8 && reservations.occupied(5,8));
  // Same current selection within the inclusive range is admitted without
  // searching/reassigning, including when some bits are occupied.
  assert(reservations.admit(5,8,8,selected));
  assert(selected==8);
  reservations.occupy(5,11);reservations.occupy(5,12);
  assert(reservations.admit(5,11,13,selected));
  assert(selected==13 && !reservations.occupied(5,8));
  assert(reservations.occupied(5,13));
  assert(!reservations.admit(5,11,12,selected));
  assert(selected==13);
  // Different categories have independent selected bits.
  int other=-1;
  assert(reservations.admit(6,11,11,other));
  assert(other==11 && reservations.occupied(6,11));
  // Original warns when clearing an unowned previous index; host records it.
  int corrupted=44;
  assert(reservations.admit(7,1,2,corrupted));
  assert(corrupted==1 && reservations.missing_old_bit_notifications()==1);
  assert(!reservations.admit(7,-1,2,corrupted));
  assert(!reservations.admit(7,9,8,corrupted));

  assert(optional_identity_guard(false,123,false,0));
  assert(!optional_identity_guard(true,123,false,0));
  assert(!optional_identity_guard(true,123,true,124));
  assert(optional_identity_guard(true,123,true,123));

  Lifecycle life;
  acquire_group(4,life);
  assert((life.actions==std::vector<std::string>{"find:4","create:4","attach:88"}));
  life.actions.clear();life.group=63;
  acquire_group(5,life);
  assert((life.actions==std::vector<std::string>{"find:5","attach:63"}));
  life.actions.clear();
  std::uint32_t group_id=63;
  clear_group(group_id,life);
  assert(group_id==0 && (life.actions==std::vector<std::string>{"detach:63"}));
  clear_group(group_id,life);
  assert(life.actions.size()==1);

  auto invalid=make_member(1,false,true,false,9);
  auto not_ready=make_member(2,true,false,false,9);
  auto rejecting=make_member(3,true,true,true,9);
  auto owner=make_member(4,true,true,false,9,true,true,false);
  auto skip_no_94=make_member(5,true,true,false,9,false,false,false);
  auto special=make_member(6,true,true,false,9,true,false,true);
  auto remaining=make_member(7,true,true,false,11);
  // Path where +0x204 is 0 but component virtual +0x2C accepts.
  not_ready.component_virtual_0x2c_allows=true;
  Members members({invalid,not_ready,rejecting,owner,skip_no_94,special,remaining});
  Members::Cursor cursor;
  assert(members.first(cursor,true)->native_handle==2);
  assert(cursor.index==1);
  assert(members.next(cursor,true)->native_handle==4);
  assert(members.next(cursor,true)->native_handle==5);
  assert(members.next(cursor,true)->native_handle==6);
  assert(members.next(cursor,true)->native_handle==7);
  assert(members.next(cursor,true)==nullptr);
  assert(cursor.index==7);
  // Unfiltered first at an invalid handle does not magically find a later
  // good entry. This deliberately differs from the filtered path above.
  assert(members.first(cursor,false)==nullptr && cursor.index==0);
  assert(members.next(cursor,false)->native_handle==2);
  assert(members.count_matching(9)==4);
  assert(count_metric_matches(&members,true,9,9)==4);
  assert(count_metric_matches(&members,true,9,11)==1);
  assert(count_metric_matches(nullptr,true,9,9)==1);
  assert(count_metric_matches(nullptr,false,9,9)==0);
  assert(count_metric_matches(nullptr,true,9,10)==0);

  Dispatch d;
  d.accepted=6;
  assert(!dispatch_linked(nullptr,d) && d.calls.empty());
  assert(dispatch_linked(&members,d));
  assert((d.calls==std::vector<std::string>{"dispatch:2","activate:6","dispatch:6"}));
  d.calls.clear();d.accepted=0;
  assert(!dispatch_linked(&members,d));
  assert((d.calls==std::vector<std::string>{"dispatch:2","activate:6","dispatch:6","dispatch:7"}));
  // Unfiltered or rejected member alone is not sent to dispatch.
  Members blocked({invalid,rejecting});
  d.calls.clear();
  assert(!dispatch_linked(&blocked,d));
  assert(d.calls.empty());
}