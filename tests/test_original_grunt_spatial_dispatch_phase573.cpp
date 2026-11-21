#include "sin_compat/original_grunt_spatial_dispatch_phase573.hpp"
#include <cassert>
#include <string>
#include <vector>
using namespace sin_compat::phase573;
struct Host {
  int returned_status=0;
  std::vector<std::string> calls;
  float initial_distance(std::uint32_t kind){calls.push_back("init:"+std::to_string(kind));return 50.f;}
  float fallback_distance(std::uint32_t kind){calls.push_back("fallback:"+std::to_string(kind));return 18.f;}
  std::int32_t mode_specific_result(Path p, ResultRecord& r){
    calls.push_back("mode:"+std::to_string(static_cast<unsigned>(p)));
    assert(r.initial_distance==50.f && r.other_distance==0.f);
    return returned_status;
  }
};
int main(){
  for(std::uint32_t mode=0;mode<4;++mode){
    Host h;ResultRecord rec{42, 99.f, 10.f};
    h.returned_status=static_cast<int>(mode);
    auto result=check_spatial_mode(h,mode,rec);
    assert(result.allowed && static_cast<unsigned>(result.path)==mode);
    assert(rec.raw_status==static_cast<int>(mode) && rec.initial_distance==50.f);
    assert((h.calls==std::vector<std::string>{"init:"+std::to_string(mode),"mode:"+std::to_string(mode)}));
  }
  {
    Host h;h.returned_status=-2;ResultRecord rec;
    auto r=check_spatial_mode(h,0,rec);
    assert(!r.allowed && r.path==Path::Kind0 && rec.raw_status==-2);
    assert(h.calls.size()==2);
  }
  {
    Host h;h.returned_status=-1;ResultRecord rec;
    auto r=check_spatial_mode(h,3,rec);
    assert(!r.allowed && r.path==Path::Kind3);
  }
  for(auto mode : {4u,9u,0xffffffffu}){
    Host h;h.returned_status=99;ResultRecord rec{100,19.f,20.f};
    auto r=check_spatial_mode(h,mode,rec);
    assert(!r.allowed && r.path==Path::OutsideRange && rec.raw_status==-4);
    assert(rec.initial_distance==50.f && rec.other_distance==18.f);
    assert((h.calls==std::vector<std::string>{"init:"+std::to_string(mode),"fallback:"+std::to_string(mode)}));
  }
}