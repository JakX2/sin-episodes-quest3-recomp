#include "sin_compat/original_loadingdisc_panel_sound_emitter.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <string>
#include <vector>
using namespace sin_compat::phase95;
int main(){
    static_assert(loading_panel_allocation_bytes==220);
    static_assert(loading_label_allocation_bytes==240);
    static_assert(loading_panel_label_pointer_offset==216);
    assert(loading_panel_resource=="resource/LoadingDiscPanel.res");
    assert(loading_client_scheme=="ClientScheme");
    int panel=1,label=2,context=3,allocations=0;
    LoadingPanelHostState s;
    std::vector<std::string> events;
    bool ok=construct_loading_panel(s,&context,
      [&](std::size_t n)->void*{
        events.push_back("alloc:"+std::to_string(n));
        return allocations++==0?static_cast<void*>(&panel):static_cast<void*>(&label);
      },
      [&](void* p,void* ctx,std::string_view name,std::string_view scheme){
        assert(p==&panel && ctx==&context && name=="CLoadingDiscPanel" && scheme=="ClientScheme");
        events.push_back("panel");
      },
      [&](void* p,void* parent,std::string_view name){
        assert(p==&label && parent==&panel && name=="LoadingLabel");
        events.push_back("label");
      },
      [&](void* p){assert(p==&label);events.push_back("attach");},
      [&](void* p){assert(p==&label);events.push_back("hide");},
      [&](void* p,std::string_view path){assert(p==&panel && path==loading_panel_resource);events.push_back("resource");});
    assert(ok && s.panel==&panel && s.label==&label && s.label_initially_hidden);
    assert((events==std::vector<std::string>{"alloc:220","panel","alloc:240","label","attach","hide","resource"}));
    LoadingPanelHostState fail;
    assert(!construct_loading_panel(fail,nullptr,
       [](std::size_t)->void*{return nullptr;},
       [](void*,void*,std::string_view,std::string_view){assert(false);},
       [](void*,void*,std::string_view){assert(false);},
       [](void*){assert(false);},[](void*){assert(false);},
       [](void*,std::string_view){assert(false);}));
    int factory_count=0;
    auto init=[&](){++factory_count;return true;};
    auto lookup=[](std::string_view n){return n=="step"?42u:missing_sound_token;};
    auto reverse=[](std::uint32_t token)->std::string_view{
      return token==42u?"step":std::string_view{};};
    SoundEmitterReference emitter(init,lookup,reverse);
    assert(emitter.resolve_name("step")==missing_sound_token);
    assert(emitter.resolve_token(42).empty());
    assert(emitter.initialize() && factory_count==1);
    assert(emitter.resolve_name("step")==42);
    assert(emitter.resolve_name("missing")==missing_sound_token);
    assert(emitter.resolve_token(42)=="step");
    assert(emitter.resolve_token(missing_sound_token).empty());
    assert(emitter.state.lookup_calls==2 && emitter.state.reverse_calls==1);
    static_assert(name_to_token_virtual_byte_offset==12);
    static_assert(token_to_name_virtual_byte_offset==24);
}
