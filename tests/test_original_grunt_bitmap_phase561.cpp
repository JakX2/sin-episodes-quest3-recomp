#include "sin_compat/original_grunt_bitmap_phase561.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
using namespace sin_compat::original_grunt_bitmap_phase561;
struct Refresher:IRefreshHost {
  unsigned calls=0;
  void refresh_eligible_members() override {++calls;}
};
int main(){
  static_assert(kNativeRefreshPeriodSeconds==30.0f,"native literal");
  assert(!refresh_due(1.0f,2.0f));
  assert(!refresh_due(2.0f,2.0f));
  assert(refresh_due(2.0001f,2.0f));
  assert(!refresh_due(std::numeric_limits<float>::quiet_NaN(),2.0f));
  assert(!refresh_due(2.0f,std::numeric_limits<float>::quiet_NaN()));
  assert(next_deadline(100.5f)==130.5f);
  Bitmap words(2);
  assert(!words.test(0));
  assert(words.set(0)&&words.test(0));
  assert(words.set(31)&&words.test(31));
  assert(words.set(32)&&words.test(32));
  assert(words.set(63)&&words.test(63));
  assert(!words.test(62));
  assert(words.words[0]==0x80000001u && words.words[1]==0x80000001u);
  assert(words.clear(31) && !words.test(31));
  assert(words.test(0) && words.test(32) && words.test(63));
  assert(words.clear(63)&&!words.test(63));
  assert(words.words[0]==1u && words.words[1]==1u);
  assert(!words.test(-1)&&!words.set(-1)&&!words.clear(-1));
  assert(!words.test(64)&&!words.set(64)&&!words.clear(64));
  assert(!words.set(std::numeric_limits<int>::max()));

  Refresher r;
  TimedBitmapHost cache(r,10.0f,64);
  assert(cache.deadline()==10.0f && cache.record_count()==0);
  assert(cache.set(1,31,9.0f));
  assert(cache.refresh_gate_executions()==0 && r.calls==0);
  assert(cache.record_count()==1 && cache.last_owner_id()==1);
  assert(cache.set(1,32,10.0f));
  assert(cache.refresh_gate_executions()==0 && cache.test(1,32,10.0f));
  assert(cache.test(1,31,10.1f));
  assert(cache.refresh_gate_executions()==1 && r.calls==1);
  assert(cache.deadline()==40.1f);
  assert(cache.test(1,32,40.1f));
  assert(cache.refresh_gate_executions()==1);
  assert(!cache.test(2,31,40.1f));
  assert(cache.record_count()==2);
  assert(cache.set(2,0,41.0f));
  assert(cache.refresh_gate_executions()==2 && r.calls==2);
  assert(cache.test(1,31,41.0f)); // host retains bits until callback acts
  assert(cache.test(2,0,41.0f));
  assert(cache.clear(1,31,41.0f));
  assert(!cache.test(1,31,41.0f) && cache.test(1,32,41.0f));
  cache.remove_owner(1);
  assert(cache.record_count()==1);
  assert(!cache.test(1,32,41.0f));
  assert(cache.record_count()==2);
  assert(cache.set(3,63,41.0f));
  assert(cache.test(3,63,41.0f));
  assert(!cache.test(3,64,41.0f));

  // Native when cache has no entries at a due gate: it still advances
  // deadline, but the member revalidation callback is not invoked.
  Refresher emptyRefresher;
  TimedBitmapHost empty(emptyRefresher,0.0f,32);
  assert(!empty.test(8,0,4.0f));
  assert(emptyRefresher.calls==0 && empty.deadline()==34.0f);
  assert(empty.refresh_gate_executions()==1);
}