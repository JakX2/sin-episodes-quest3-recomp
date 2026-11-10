#include "sin_compat/original_server_scattergun_secondary_fire.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
namespace s=sin_compat::original_server_scattergun_secondary_fire;
namespace {
struct Host {
    s::SecondaryState& state;
    std::vector<std::string> log{};
    std::uintptr_t owner=1;
    std::uint32_t mode=0;
    bool capable=true,vectors=true,secondary_clip=true,flip_clip=false;
    unsigned clip_calls=0,times=0;
    float clock1=10,clock2=12,interval=2;
    std::uint32_t ammo_type=92,selection=71,ammo_count=19;
    std::uintptr_t find_owner(){log.emplace_back("owner");return owner;}
    bool owner_action_capable(std::uintptr_t v){assert(v==owner);log.emplace_back("cap");return capable;}
    std::uint32_t owner_mode_298(std::uintptr_t v){assert(v==owner);log.emplace_back("mode");return mode;}
    void dispatch_state_event(std::uint32_t a,std::uint32_t b){assert(b==0);log.emplace_back(a==3?"event3":"event0");}
    float read_game_time(){log.emplace_back("clock");return times++==0?clock1:clock2;}
    bool acquire_fire_vectors(){log.emplace_back("vectors");return vectors;}
    bool weapon_uses_secondary_clip(){log.emplace_back("clip?");return flip_clip && ++clip_calls>1?!secondary_clip:secondary_clip;}
    void weapon_empty_callback(){log.emplace_back("empty");}
    std::uint32_t weapon_fire_selection(){log.emplace_back("choose");return selection;}
    void dispatch_selection(std::uint32_t v){assert(v==selection);log.emplace_back("selection");}
    float timing_interval(std::uint32_t arg){assert(arg==state.timing_arg_3ec);log.emplace_back("interval");return interval;}
    void owner_fire_callback(std::uintptr_t){log.emplace_back("firecb");}
    void owner_animation(std::uintptr_t,std::uint32_t n){assert(n==5u);log.emplace_back("anim5");}
    void owner_consume_ammo(std::uintptr_t,std::uint32_t amount,std::uint32_t id){assert(amount==1u&&id==state.alternate_ammo_type_4e8);log.emplace_back("consumeAlt");}
    std::uint32_t weapon_secondary_ammo_type(){log.emplace_back("type");return ammo_type;}
    std::uint32_t owner_ammo_count(std::uintptr_t,std::uint32_t id){assert(id==ammo_type);log.emplace_back("ammoCount");return ammo_count;}
    void weapon_post_fire(){log.emplace_back("post");}
    void weapon_feedback(std::uint32_t id){assert(id==6u);log.emplace_back("feedback6");}
    void prepare_pellet_frame(){log.emplace_back("spreadSetup");}
    void process_original_pellet(std::uint32_t n,std::uint32_t pattern,float one,float dist){
        assert(n<8&&pattern==s::kSpreadPatternAddress&&one==1.0f&&dist==1500.0f);
        log.emplace_back("pellet"+std::to_string(n));
    }
};
struct VectorHost {
    std::vector<std::string> log{};
    std::uintptr_t owner=1,fallback=2;
    bool direct=true;
    std::uintptr_t find_owner_for_vectors(){log.emplace_back("owner");return owner;}
    bool owner_uses_direct_vector(std::uintptr_t v){assert(v==owner);log.emplace_back("direct?");return direct;}
    s::NativeVector3 owner_origin_vector(std::uintptr_t){log.emplace_back("origin");return {{1,2,3}};}
    s::NativeVector3 owner_offset_vector(std::uintptr_t,std::uint32_t bits){
        assert(bits==s::kOriginalVectorArgumentBits);log.emplace_back("offset");return {{4,5,6}};
    }
    std::uintptr_t find_alternative_vector_source(){log.emplace_back("fallback");return fallback;}
    s::NativeVector3 candidate_vector(std::uintptr_t h,const s::NativeVector3& first){
        assert(h==fallback&&first.words[0]==1);log.emplace_back("candidate");return {{7,8,9}};
    }
};
void vectors(){
    s::NativeVector3 first{{0x7fc00001u,0xdeadbeefu,0}},second{{44,55,66}};
    VectorHost h{};h.owner=0;
    assert(!s::original_obtain_fire_vectors(first,second,h));
    assert(first.words[0]==0x7fc00001u&&second.words[0]==44u);
    assert((h.log==std::vector<std::string>{"owner"}));
    h=VectorHost{};
    assert(s::original_obtain_fire_vectors(first,second,h));
    assert(first.words[0]==1&&second.words[0]==4);
    assert((h.log==std::vector<std::string>{"owner","direct?","origin","offset"}));
    h=VectorHost{};h.direct=false;h.fallback=0;
    first={{11,12,13}};second={{14,15,16}};
    assert(!s::original_obtain_fire_vectors(first,second,h));
    assert(first.words[0]==11&&second.words[0]==14);
    h=VectorHost{};h.direct=false;
    assert(s::original_obtain_fire_vectors(first,second,h));
    assert(first.words[2]==3&&second.words[2]==9);
    assert((h.log==std::vector<std::string>{"owner","direct?","fallback","origin","candidate"}));
}
void early_paths(){
    s::SecondaryState st{};st.other_timer_4a0=1.0f;
    Host h{st};h.owner=0;
    assert(s::original_scattergun_secondary_fire(st,h)==s::Outcome::MissingOwner);
    assert((h.log==std::vector<std::string>{"owner"}));
    Host h2{st};h2.mode=3;h2.clock1=7.0f;
    assert(s::original_scattergun_secondary_fire(st,h2)==s::Outcome::OwnerModeCooldown);
    assert((h2.log==std::vector<std::string>{"owner","cap","mode","event0","clock"}));
    assert(st.other_timer_4a0>7.19f&&st.other_timer_4a0<7.21f&&st.gate_timer_49c==0.0f);
    st.clip.secondary_clip_4f0=0;
    Host h3{st};h3.vectors=false;
    assert(s::original_scattergun_secondary_fire(st,h3)==s::Outcome::NoFireVectors);
    Host h4{st};
    assert(s::original_scattergun_secondary_fire(st,h4)==s::Outcome::EmptyTrackedClip);
    assert((h4.log==std::vector<std::string>{"owner","cap","mode","vectors","clip?","empty"}));
    st.clip.secondary_clip_4f0=0x80000000u;
    Host h5{st};
    assert(s::original_scattergun_secondary_fire(st,h5)==s::Outcome::EmptyTrackedClip);
}
void full_fire(){
    s::SecondaryState st{};st.clip.secondary_clip_4f0=9;st.timing_arg_3ec=77;
    st.alternate_ammo_type_4e8=41;std::uint32_t flags=0;
    st.notify.external_flags_16c=&flags;
    st.clip.event_flags_at_pointer_16c=&flags;
    Host h{st};
    assert(s::original_scattergun_secondary_fire(st,h)==s::Outcome::Fired);
    assert(st.clip.secondary_clip_4f0==8u&&st.gate_timer_49c==12.0f&&st.other_timer_4a0==14.0f);
    assert(st.observed_secondary_584==19u&&flags==1u);
    const std::vector<std::string> expected={
        "owner","cap","mode","vectors","clip?","event3","choose","selection",
        "interval","clock","clock","firecb","anim5","clip?","type","ammoCount",
        "post","feedback6","spreadSetup"};
    assert(h.log.size()==expected.size()+8u);
    for(std::size_t i=0;i<expected.size();++i)assert(h.log[i]==expected[i]);
    for(unsigned i=0;i<8;++i)assert(h.log[expected.size()+i]=="pellet"+std::to_string(i));
}
void owner_side_effects_disabled(){
    s::SecondaryState st{};st.clip.secondary_clip_4f0=3;
    Host h{st};h.capable=false;
    assert(s::original_scattergun_secondary_fire(st,h)==s::Outcome::Fired);
    assert(st.clip.secondary_clip_4f0==3u&&st.observed_secondary_584==0);
    assert(h.log.size()==21u);
}
void recheck_clip_mode(){
    s::SecondaryState st{};st.clip.secondary_clip_4f0=7;st.alternate_ammo_type_4e8=23;
    Host h{st};h.flip_clip=true;
    assert(s::original_scattergun_secondary_fire(st,h)==s::Outcome::Fired);
    assert(st.clip.secondary_clip_4f0==7u);
    bool used_alt=false;
    for(const auto& e:h.log)if(e=="consumeAlt")used_alt=true;
    assert(used_alt&&h.clip_calls==2);
}
}
int main(){vectors();early_paths();full_fire();owner_side_effects_disabled();recheck_clip_mode();}
