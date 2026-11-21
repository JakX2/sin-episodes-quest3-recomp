#include "sin_compat/original_grunt_weighted_lookup_phase585.hpp"
#include <cassert>
#include <limits>
#include <string>
#include <vector>
using namespace sin_compat::phase585;
int main(){
    std::vector<std::string> trace;
    Hooks host;
    host.revision_is_current=[&](){trace.push_back("revision");return true;};
    host.refresh_registration=[&](){trace.push_back("rebuild");};
    host.refresh_entry_at_index=[&](int i){trace.push_back("entry"+std::to_string(i));};
    host.random_inclusive=[&](int lo,int hi){trace.push_back("rng"+std::to_string(lo)+":"+std::to_string(hi));return 0;};
    assert(Select(nullptr,18,0,host).returned_index==0 && trace.empty());
    Registry r;
    assert(Select(&r,18,0,host).returned_index==-1 && (trace==std::vector<std::string>{"revision"}));
    trace.clear();
    r.records={{10,5,0x10},{18,5,0},{18,10,0x10},{12,7,0}};
    auto o=Select(&r,18,2,host);
    assert(o.supported && o.returned_index==2);
    assert((trace==std::vector<std::string>{"revision","entry1","rng0:4","rng0:14","entry3"}));
    trace.clear();
    // Native 'weighted replacement': second draw over total=15 excludes item 2.
    int draw_count=0;
    host.random_inclusive=[&](int lo,int hi){assert(lo==0);++draw_count;return hi;};
    o=Select(&r,18,2,host);assert(o.returned_index==1&&draw_count==2);
    trace.clear();
    r.records={{19,-3,0x10},{19,4,0x10}};
    host.random_inclusive=[&](int,int){assert(false);return 0;};
    o=Select(&r,19,0,host);assert(o.supported&&o.returned_index==0);
    // Nonzero third parameter means negative weights join the running sum.
    host.random_inclusive=[](int,int){return 0;};
    o=Select(&r,19,5,host);assert(o.returned_index==1);
    host.revision_is_current=[&](){trace.push_back("revision");return false;};
    host.refresh_registration=[&](){trace.push_back("rebuild");};
    trace.clear();o=Select(&r,99,1,host);assert(o.returned_index==-1);
    assert((trace==std::vector<std::string>{"revision","rebuild"}));
    r.records={{99,0,0x10}};
    int zero_calls=0;
    host.random_inclusive=[&](int lo,int hi){assert(lo==0&&hi==-1);++zero_calls;return 0;};
    o=Select(&r,99,1,host);assert(o.returned_index==0&&zero_calls==1);
    r.records={{99,std::numeric_limits<int32_t>::min(),0x10}};
    o=Select(&r,99,2,host);assert(!o.supported);
    r.records={{99,std::numeric_limits<int32_t>::max(),0x10},{99,1,0x10}};
    host.random_inclusive=[](int,int){return 0;};
    o=Select(&r,99,2,host);assert(!o.supported);
}