#include "sin_compat/original_hud_ctor_cache_slot2.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <string_view>
using namespace sin_compat::phase93;
int main(){
    static_assert(hud_ctors.size()==8);
    for(const auto& ctor:hud_ctors){
        OriginalHudX86Words object{0,0xdeadbeefu,0x12345678u};
        apply_verified_constructor(object,ctor);
        assert(object.vtable==ctor.original_vtable);
        assert(object.member_4==0 && object.member_8==0x12345678u);
    }
    assert(hud_ctors[4].original_class==std::string_view("CLoadingDisc"));
    int scene_called=0;
    auto scene=[&](std::uint32_t token){assert(token==7);++scene_called;};
    assert(scene_candidate_gate(true,5,false,true,7,scene));
    assert(!scene_candidate_gate(false,5,false,true,7,scene));
    assert(!scene_candidate_gate(true,4,false,true,7,scene));
    assert(!scene_candidate_gate(true,5,true,true,7,scene));
    assert(!scene_candidate_gate(true,5,false,false,7,scene));
    assert(!scene_candidate_gate(true,5,false,true,unresolved_token,scene));
    assert(scene_called==1);
    int model_called=0;
    auto model=[&](){++model_called;};
    assert(model_sound_candidate_gate(5,true,3,true,model));
    assert(!model_sound_candidate_gate(unresolved_token,true,3,true,model));
    assert(!model_sound_candidate_gate(5,false,3,true,model));
    assert(!model_sound_candidate_gate(5,true,5,true,model));
    assert(!model_sound_candidate_gate(5,true,3,false,model));
    assert(model_called==1);
}
