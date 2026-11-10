#include "sin_compat/original_server_entity_angles.hpp"
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
namespace a = sin_compat::original_server_entity_angles;
namespace {
a::Angle3 ang(float x,float y,float z) {return {{x,y,z}};}
bool is(const a::Angle3& v,float x,float y,float z) {
    return v.xyz[0]==x && v.xyz[1]==y && v.xyz[2]==z;
}
struct Host {
    std::vector<std::string> log{};
    a::ParentAngles parent{};
    bool parent_found = false, mutate_initial_angles = false;
    bool mutate_first_parent_refresh = false, clear_flag_on_first_refresh = false;
    unsigned parent_refreshes = 0;
    unsigned reads = 0;
    float current_time = 19.25f;
    a::Angle3 relative = ang(4,5,6);
    void refresh_absolute_angles(a::EntityAngles& self) {
        log.push_back("self_refresh");
        if(mutate_initial_angles) self.absolute_angles_354=ang(1,2,3);
    }
    void notify_transform_change(a::EntityAngles& self,unsigned type) {
        log.push_back("notify2");assert(type==2u);
        // Original calls before clearing the bit and before copying angles.
        assert((self.flags_b9 & 8u)!=0);
    }
    void rebuild_absolute_angles_matrix(a::EntityAngles& self,const a::Angle3& input){
        log.push_back("matrix_build");
        assert(a::original_angles_equal(self.absolute_angles_354,input));
        assert((self.flags_b9&8u)==0);
    }
    void rebuild_auxiliary_angle_transform(a::EntityAngles&,unsigned n){
        log.push_back("aux3");assert(n==3u);
    }
    a::ParentAngles* resolve_parent(std::uint32_t index,std::uint32_t serial){
        log.push_back("resolve");assert(index==0x123u&&serial==0xA5Bu);
        return parent_found?&parent:nullptr;
    }
    void refresh_parent_absolute_angles(a::ParentAngles& p){
        log.push_back("parent_refresh");assert(&p==&parent);
        ++parent_refreshes;
        if(mutate_first_parent_refresh && parent_refreshes==1)
            p.absolute_angles_354=ang(10,20,30);
        if(clear_flag_on_first_refresh && parent_refreshes==1)
            p.flags_b8=0;
    }
    void invert_parent_rotation(a::ParentAngles& p){
        log.push_back("parent_inverse");assert(&p==&parent);
    }
    void compose_parent_relative_rotation(a::EntityAngles&,a::ParentAngles& p){
        log.push_back("relative_matrix");assert(&p==&parent);
    }
    a::Angle3 extract_relative_angles(a::EntityAngles&,a::ParentAngles& p){
        log.push_back("angle_extract");assert(&p==&parent);return relative;
    }
    float read_game_time(){log.push_back("clock");++reads;return current_time;}
    void on_local_angle_change(a::EntityAngles& self,float game_time){
        log.push_back("time_update");assert(game_time==current_time);
        assert((self.flags_b9&8u)==0);
    }
};
void test_vector_helpers(){
    a::Angle3 plus=ang(0.f,-0.f,2.f),minus=ang(-0.f,0.f,2.f);
    assert(a::original_angles_equal(plus,minus));
    assert(!a::original_angles_different(plus,minus));
    std::uint32_t event=0x20u;
    a::ChangeNotification notify{};notify.external_flags_16c=&event;
    assert(!a::original_assign_tracked_angles(plus,minus,notify));
    assert(event==0x20u&&!std::signbit(plus.xyz[0])&&std::signbit(plus.xyz[1]));
    minus=ang(1.f,3.f,4.f);
    assert(a::original_assign_tracked_angles(plus,minus,notify));
    assert(event==0x21u&&is(plus,1,3,4));
    event=0x40u;notify.mode_byte_1f0=3;
    assert(a::original_assign_tracked_angles(plus,ang(2,3,4),notify));
    assert(notify.dirty_byte_200==1u&&event==0x40u);
    notify.dirty_byte_200=0u;notify.mode_byte_1f0=0;
    notify.external_flags_16c=nullptr;
    assert(a::original_assign_tracked_angles(plus,ang(3,3,4),notify));
    assert(notify.dirty_byte_200==0);
    std::uint32_t payload=0x7fc12345u;
    float nan=0.f;std::memcpy(&nan,&payload,sizeof(nan));
    plus=ang(nan,0,0);
    minus=ang(nan,0,0);
    assert(!a::original_angles_equal(plus,minus));
    assert(a::original_angles_different(plus,minus));
    assert(a::original_assign_tracked_angles(plus,minus,notify));
    std::uint32_t stored=0;std::memcpy(&stored,&plus.xyz[0],sizeof(stored));
    assert(stored==payload);
    auto parts=a::decode_original_parent_handle(0xA5B123u);
    assert(parts.index==0x123u&&parts.serial==0xA5Bu);
}
void test_unconditional_refresh_before_unchanged_gate(){
    a::EntityAngles state{};state.flags_b9=8;
    state.absolute_angles_354=ang(8,9,10);
    Host host{};host.mutate_initial_angles=true;
    assert(a::original_set_absolute_angles(state,ang(1,2,3),host)==
           a::Outcome::Unchanged);
    assert((host.log==std::vector<std::string>{"self_refresh"}));
    assert(state.flags_b9==8);
}
void test_no_parent_changed_and_notification(){
    a::EntityAngles state{};state.flags_b9=0xFFu;
    std::uint32_t event=0x20u;state.notify.external_flags_16c=&event;
    Host host{};
    assert(a::original_set_absolute_angles(state,ang(11,12,13),host)==
           a::Outcome::ChangedNoValidParent);
    assert(is(state.absolute_angles_354,11,12,13));
    assert(is(state.local_angles_36c,11,12,13));
    assert(state.flags_b9==0xF7u && event==0x21u && host.reads==1);
    assert((host.log==std::vector<std::string>{"self_refresh","notify2",
        "matrix_build","aux3","clock","time_update"}));
}
void test_valid_handle_abs_angles_match_parent(){
    a::EntityAngles state{};state.flags_b9=8u;
    state.parent_handle_f0=0xA5B123u;
    state.local_angles_36c=ang(2,3,4);
    Host host{};host.parent_found=true;
    host.parent.flags_b8=0x800u;
    host.parent.absolute_angles_354=ang(1,2,3);
    assert(a::original_set_absolute_angles(state,ang(1,2,3),host)==
           a::Outcome::ChangedParentAnglesMatched);
    assert(host.parent_refreshes==1);
    assert(is(state.local_angles_36c,0,0,0));
    assert((host.log==std::vector<std::string>{"self_refresh","notify2",
        "matrix_build","aux3","resolve","parent_refresh","clock","time_update"}));
}
void test_transformed_parent_branch_twice_and_live_flag_recheck(){
    a::EntityAngles state{};state.flags_b9=8u;
    state.parent_handle_f0=0xA5B123u;
    Host host{};host.parent_found=true;
    host.parent.flags_b8=0x800u;host.parent.absolute_angles_354=ang(7,8,9);
    assert(a::original_set_absolute_angles(state,ang(11,12,13),host)==
           a::Outcome::ChangedParentTransformed);
    assert(host.parent_refreshes==2);
    assert(is(state.local_angles_36c,4,5,6));
    assert((host.log==std::vector<std::string>{"self_refresh","notify2",
        "matrix_build","aux3","resolve","parent_refresh","parent_refresh",
        "parent_inverse","relative_matrix","angle_extract","clock","time_update"}));
    state.flags_b9=8;host.log.clear();host.parent_refreshes=0;
    host.clear_flag_on_first_refresh=true;
    assert(a::original_set_absolute_angles(state,ang(20,21,22),host)==
           a::Outcome::ChangedParentTransformed);
    assert(host.parent_refreshes==1);
}
void test_no_time_update_when_local_angles_unchanged(){
    a::EntityAngles state{};state.flags_b9=8;
    state.local_angles_36c=ang(11,12,13);
    Host host{};
    assert(a::original_set_absolute_angles(state,ang(11,12,13),host)==
           a::Outcome::ChangedNoValidParent);
    assert(host.reads==0);
    assert((host.log==std::vector<std::string>{"self_refresh","notify2",
        "matrix_build","aux3"}));
}
void test_invalid_parent_serial_fallback(){
    a::EntityAngles state{};state.flags_b9=8;state.parent_handle_f0=0xA5B123u;
    Host host{};host.parent_found=false;
    assert(a::original_set_absolute_angles(state,ang(3,4,5),host)==
           a::Outcome::ChangedNoValidParent);
    assert((host.log==std::vector<std::string>{"self_refresh","notify2",
        "matrix_build","aux3","resolve","clock","time_update"}));
}
}
int main(){
    test_vector_helpers();
    test_unconditional_refresh_before_unchanged_gate();
    test_no_parent_changed_and_notification();
    test_valid_handle_abs_angles_match_parent();
    test_transformed_parent_branch_twice_and_live_flag_recheck();
    test_no_time_update_when_local_angles_unchanged();
    test_invalid_parent_serial_fallback();
}