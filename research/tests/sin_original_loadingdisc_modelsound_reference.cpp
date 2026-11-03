#include "sin_compat/original_loadingdisc_modelsound_events.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <vector>
#include <string>
using namespace sin_compat::phase94;
int main(){
    static_assert(loadingdisc_allocation_bytes==220);
    static_assert(loadingdisc_methods.size()==4);
    LoadingDiscFields s{};int a=1,b=2,ctx=3;s.context=&ctx;
    std::vector<std::string> trace;int creations=0;
    auto create=[&](std::size_t bytes,void* context)->void* {
      assert(bytes==220 && context==&ctx);trace.push_back("create");
      return creations++==0?static_cast<void*>(&a):static_cast<void*>(&b);};
    auto attach=[&](void* p){trace.push_back("attach");assert(p!=nullptr);};
    auto configure=[&](void* p,int off,std::uint32_t va){
      assert(p==&b && off==0x28c && va==0x242ac8c0u);trace.push_back("configure");};
    auto visible=[&](void* p,bool on,int off){
      assert(p!=nullptr && off==0x78);trace.push_back(on?"show":"hide");};
    assert(toggle_loading_disc(s,false,true,create,attach,configure,visible));
    assert(s.first==&a && !s.second && creations==1);
    assert((trace==std::vector<std::string>{"create","attach","show"}));
    trace.clear();assert(toggle_loading_disc(s,false,false,create,attach,configure,visible));
    assert((trace==std::vector<std::string>{"hide"}) && s.first==&a);
    trace.clear();assert(toggle_loading_disc(s,true,true,create,attach,configure,visible));
    assert((trace==std::vector<std::string>{"create","attach","configure","show"}));
    assert(s.second==&b && creations==2);
    trace.clear();assert(toggle_loading_disc(s,true,false,create,attach,configure,visible));
    assert((trace==std::vector<std::string>{"hide"}) && s.second==&b);
    trace.clear();assert(toggle_loading_disc(s,true,true,create,attach,configure,visible));
    assert((trace==std::vector<std::string>{"show"}) && creations==2);
    LoadingDiscFields failed{};assert(!toggle_loading_disc(failed,true,true,
      [](std::size_t,void*)->void*{return nullptr;},[](void*){},
      [](void*,int,std::uint32_t){assert(false);},[](void*,bool,int){assert(false);}));
    std::vector<std::string> names;int warnings=0;
    examine_model_sound_events({{14,0x10,"boom"},{14,0,"skip"},{14,0x10,""},
      {1004,0,"item1004"},{1008,0,"item1008"},{5004,0,"item5004"},
      {6004,0,""},{6005,0,""},{42,0,"ignored"}},"TestModel",
      [&](const std::string& name){names.push_back(name);},
      [&](std::uint32_t id){assert(id==14);++warnings;});
    assert(warnings==1 && names.size()==12);
    assert(names[0]=="boom" && names[1]=="item1004" && names[2]=="item1008" && names[3]=="item5004");
    assert(names[4]=="TestModel.FootstepRight" && names[7]=="TestModel.RunFootstepLeft");
    assert(names[8]=="TestModel.FootstepRight" && names[11]=="TestModel.RunFootstepLeft");
    names.clear();examine_model_sound_events({{6004,0,""}},"",
       [&](const std::string& name){names.push_back(name);},[](std::uint32_t){});
    assert(names[0]=="NPC_grunt.FootstepRight");
    std::vector<std::uint32_t> tokens;
    auto resolve=[](std::string_view s)->std::uint32_t {
       return s=="unknown"?unresolved_model_sound_token:s=="alpha"?7:9;};
    insert_unique_sound_token(tokens,"alpha",resolve);
    insert_unique_sound_token(tokens,"alpha",resolve);
    insert_unique_sound_token(tokens,"unknown",resolve);
    insert_unique_sound_token(tokens,"beta",resolve);
    assert((tokens==std::vector<std::uint32_t>{7,9}));
}
