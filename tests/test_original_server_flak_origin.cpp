#include "sin_compat/original_server_flak_origin.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>
namespace s = sin_compat::original_server_flak_origin;
namespace {
s::RawVector3 vec(float a,float b,float c){return {{a,b,c}};}
bool eq(const s::RawVector3& v,float x,float y,float z){
    return v.xyz[0]==x && v.xyz[1]==y && v.xyz[2]==z;
}
struct FlakHost {
    std::vector<std::string> log{};
    s::FlakSpawnRecord spawned{};
    std::uintptr_t virtual_source=7,binding=8;
    std::uint32_t read_value=0x12340001u;
    unsigned virtual_queries=0;
    s::FlakSpawnRecord* create_flak(const char* name,std::uint32_t index) {
        log.emplace_back("create");
        assert(std::string(name)=="flak" && index==0xffffffffu);
        return &spawned;
    }
    void initialize_flak(s::FlakSpawnRecord* p,const s::RawVector3& v,std::uint32_t zero){
        log.emplace_back("init");
        assert(p==&spawned&&zero==0&&eq(v,1,2,3));
    }
    std::uintptr_t source_virtual_fc(std::uintptr_t src){
        log.emplace_back("source+FC");
        assert(src==55u);
        ++virtual_queries;
        return virtual_source;
    }
    std::uintptr_t lookup_binding_222AA640(std::uintptr_t src){
        log.emplace_back("binding");
        assert(src==virtual_source);
        return binding;
    }
    std::uint32_t read_binding_word_after_v8(std::uintptr_t id){
        log.emplace_back("binding+8");
        assert(id==binding);
        return read_value;
    }
    void created_virtual_50(s::FlakSpawnRecord* p,std::uintptr_t source) {
        log.emplace_back("flak+50");
        assert(p==&spawned&&(source==0u||source==55u));
    }
    void set_flak_absolute_angles_221439C0(s::FlakSpawnRecord* p,
                                           const s::RawVector3& v){
        log.emplace_back("angles");
        assert(p==&spawned&&eq(v,4,5,6));
    }
    void created_virtual_64(s::FlakSpawnRecord* p){
        log.emplace_back("flak+64");assert(p==&spawned);
    }
};
void test_flak_spawn_order_and_optional_binding(){
    const auto first=vec(1,2,3),second=vec(4,5,6);
    FlakHost host{};
    auto* entity=s::original_spawn_flak(first,second,55,host);
    assert(entity==&host.spawned&&entity->binding_794==host.read_value);
    assert(host.virtual_queries==2);
    assert((host.log==std::vector<std::string>{"create","init","source+FC",
        "source+FC","binding","binding+8","flak+50","angles","flak+64"}));
    host=FlakHost{};
    entity=s::original_spawn_flak(first,second,0,host);
    assert(entity==&host.spawned&&entity->binding_794==0xffffffffu);
    assert((host.log==std::vector<std::string>{"create","init","flak+50",
        "vector","flak+64"}));
    host=FlakHost{};host.virtual_source=0;
    s::original_spawn_flak(first,second,55,host);
    assert(host.virtual_queries==1);
    assert((host.log==std::vector<std::string>{"create","init",
        "source+FC","flak+50","angles","flak+64"}));
    host=FlakHost{};host.binding=0;host.spawned.binding_794=17;
    s::original_spawn_flak(first,second,55,host);
    assert(host.spawned.binding_794==0xffffffffu);
}
struct OriginHost{
    std::vector<std::string> log{};
    s::ParentSnapshot parent{};
    std::uint32_t known_handle=0x1234u;
    bool parent_enabled=false;
    bool refresh_moves_parent=false;
    s::ParentSnapshot* resolve_parent(std::uint32_t handle){
        log.emplace_back("lookup_parent");
        return parent_enabled&&handle==known_handle?&parent:nullptr;
    }
    void notify_position_change(s::OriginState& state,std::uint32_t kind){
        log.emplace_back("notify");assert(kind==4u);
        assert((state.flags_b9&0x10u)!=0);
    }
    void parent_refresh_bit12(s::ParentSnapshot& p){
        log.emplace_back("refresh12");
        if(refresh_moves_parent)p.world_origin_250.xyz[0]=4.0f;
    }
    void parent_refresh_bit11(s::ParentSnapshot& p){
        log.emplace_back("refresh11");
        assert(p.world_origin_250.xyz[0]==4.0f||!refresh_moves_parent);
    }
    s::RawVector3 transform_with_parent_matrix(s::ParentSnapshot& p,
                                                const s::RawVector3& delta){
        log.emplace_back("matrix");
        assert(&p==&parent);
        return vec(delta.xyz[0]*2,delta.xyz[1]*2,delta.xyz[2]*2);
    }
    void set_local_origin(s::OriginState& state,const s::RawVector3& value){
        log.emplace_back("local_set");
        s::copy_native_vector(state.local_origin_2c8,value);
    }
};
void test_x87_component_equality_and_raw_copy(){
    s::RawVector3 a=vec(+0.0f,1.0f,-0.0f);
    s::RawVector3 b=vec(-0.0f,1.0f,+0.0f);
    assert(s::native_float_vector_equal(a,b));
    s::OriginState state{};s::copy_native_vector(state.world_origin_250,a);
    state.flags_b9=0xFFu;
    OriginHost h{};
    assert(s::original_set_world_origin(state,b,h)==s::OriginOutcome::Unchanged);
    assert(h.log.empty());
    assert(!std::signbit(state.world_origin_250.xyz[0]));
    assert(std::signbit(state.world_origin_250.xyz[2]));
    const std::uint32_t payload=0x7fc12345u;
    float nan{};
    std::memcpy(&nan,&payload,sizeof(nan));
    b.xyz[1]=nan;
    assert(s::original_set_world_origin(state,b,h)==
           s::OriginOutcome::ChangedNoValidParent);
    assert((h.log==std::vector<std::string>{"notify","local_set"}));
    std::uint32_t copy{};
    std::memcpy(&copy,&state.world_origin_250.xyz[1],sizeof(copy));
    assert(copy==payload);assert(copy==payload);
    assert((state.flags_b9&0x10u)==0);
    // Native x87 unordered compare triggers an update even with same NaN.
    h.log.clear();state.flags_b9|=0x10u;
    assert(s::original_set_world_origin(state,b,h)==
           s::OriginOutcome::ChangedNoValidParent);
    assert(h.log.size()==2);
}
void test_parent_lookup_and_relative_transform_order(){
    s::OriginState state{};
    state.flags_b9=0xFFu;
    state.parent_handle_f0=0x1234;
    OriginHost host{};
    auto result=s::original_set_world_origin(state,vec(10,20,30),host);
    assert(result==s::OriginOutcome::ChangedNoValidParent);
    assert(eq(state.local_origin_2c8,10,20,30));
    assert((host.log==std::vector<std::string>{"notify",
        "lookup_parent","local_set"}));
    host.log.clear();state.flags_b9|=0x10u;host.parent_enabled=true;
    host.parent.flags_b8=0x1800u;
    host.parent.world_origin_250=vec(1,2,3);
    host.refresh_moves_parent=true;
    result=s::original_set_world_origin(state,vec(15,25,35),host);
    assert(result==s::OriginOutcome::ChangedWithParent);
    assert(eq(state.local_origin_2c8,22,46,64));
    assert((host.log==std::vector<std::string>{"notify","lookup_parent",
        "refresh12","refresh11","matrix","local_set"}));
    assert(state.flags_b9==0xefu);
}
void test_parent_flags_off_and_valid_unchanged(){
    s::OriginState state{};state.flags_b9=0x10u;
    state.parent_handle_f0=0x1234;
    OriginHost host{};host.parent_enabled=true;
    host.parent.world_origin_250=vec(1,2,3);
    assert(s::original_set_world_origin(state,vec(5,5,5),host)==
           s::OriginOutcome::ChangedWithParent);
    assert(eq(state.local_origin_2c8,8,6,4));
    assert((host.log==std::vector<std::string>{"notify","lookup_parent",
        "matrix","local_set"}));
    host.log.clear();
    assert(s::original_set_world_origin(state,vec(5,5,5),host)==
           s::OriginOutcome::Unchanged);
    assert(host.log.empty());
}
}
int main(){
    test_flak_spawn_order_and_optional_binding();
    test_x87_component_equality_and_raw_copy();
    test_parent_lookup_and_relative_transform_order();
    test_parent_flags_off_and_valid_unchanged();
}
