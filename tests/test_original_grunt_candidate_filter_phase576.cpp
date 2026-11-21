#include "sin_compat/original_grunt_candidate_filter_phase576.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace sin_compat::phase576;

struct Host final : IProbeHost {
    bool admit = true;
    bool debug_admit = true;
    std::vector<int> events;
    bool original_probe_predicate(bool debug_path) override {
        events.push_back(debug_path ? 8 : 2);
        return debug_path ? debug_admit : admit;
    }
    void original_debug_marker() override { events.push_back(99); }
};

struct AssociationHost final : ICallerAssociationHost {
    bool accepted_type = false;
    bool recognized = false;
    std::uintptr_t linked_key = 0;
    std::vector<int> events;
    bool virtual_120(std::uintptr_t) override {
        events.push_back(120);return accepted_type;
    }
    bool native_22035100(std::uintptr_t) override {
        events.push_back(35100);return recognized;
    }
    std::uintptr_t native_22035090_virtual_28(std::uintptr_t) override {
        events.push_back(35090);return linked_key;
    }
};

static void approx(float got,float expected,float tolerance=0.00001f) {
    assert(std::fabs(got-expected) <= tolerance);
}

int main() {
    {
        auto r=normalize_xy(3,4);
        assert(r.supported_finite);
        approx(r.length,5);
        approx(r.direction.x,0.6f);
        approx(r.direction.y,0.8f);
        auto z=normalize_xy(0,0);
        assert(z.supported_finite && z.length==0.0f);
        assert(z.direction.x==0.0f && z.direction.y==0.0f);
        assert(!normalize_xy(std::numeric_limits<float>::infinity(),1).supported_finite);
        assert(!normalize_xy(std::numeric_limits<float>::quiet_NaN(),1).supported_finite);
    }
    {
        // Original entity helper preserves the entity when virtual+0xF4 true.
        assert(gated_entity(0x1234u,true)==0x1234u);
        assert(gated_entity(0x1234u,false)==0);
        assert(gated_entity(0,true)==0);
        assert(selected_entity_status(true,false)==-3);
        assert(selected_entity_status(false,false)==-2);
        assert(selected_entity_status(false,true)==-1);
        assert(candidate_search_upper_bound()==16);
    }
    {
        Host host;
        Output out;
        out.status=14;out.entity_key=18;out.distance_delta=43;out.accumulation=12;
        Inputs in;
        in.source={1,2,3}; in.destination={1,2,90};
        in.default_vector={4,5,6};
        in.budget=20;
        auto r=begin_candidate_filter(in,out,host);
        assert(r.state==ResultState::ImmediateTrue);
        assert(host.events.empty());
        assert(out.status==0 && out.entity_key==0);
        assert(out.planar_length==0 && out.distance_delta==0 && out.accumulation==0);
        assert(out.source.x==1 && out.source.y==2 && out.source.z==3);
        assert(out.default_vector.x==4 && out.default_vector.y==5 && out.default_vector.z==6);
    }
    {
        Host host; Output out; Inputs in;
        in.source={10,20,30};in.destination={13,24,35};in.budget=12;
        auto r=begin_candidate_filter(in,out,host);
        assert(r.state==ResultState::NeedsNativeCandidateSearch);
        assert(r.selection==1 && r.working_budget==12);
        assert(host.events==std::vector<int>{2});
        approx(out.planar_length,5);
        approx(r.horizontal.direction.x,0.6f);
        approx(r.horizontal.direction.y,0.8f);
    }
    {
        Host host;host.admit=false;Output out;Inputs in;
        in.destination={3,4,0};in.budget=10;
        auto r=begin_candidate_filter(in,out,host);
        assert(r.selection==2 && host.events==std::vector<int>{2});
        const bool b=complete_selected_candidate(out,0xABCD,false,true,3.0f);
        assert(!b && out.status==-1 && out.entity_key==0xABCD);
        approx(out.distance_delta,2.0f);
    }
    {
        // Literal threshold *strictness*, and the original flag short-circuit.
        for(const auto budget : {0.0f,-1.0f,0.00099f}) {
            Host host;Output out;Inputs in;in.destination={6,8,0};in.budget=budget;
            auto r=begin_candidate_filter(in,out,host);
            assert(r.selection==0 && r.working_budget==0);
            assert(host.events.empty());
        }
        Host host;Output out;Inputs in;in.destination={6,8,0};in.budget=0.001f;
        auto r=begin_candidate_filter(in,out,host);
        assert(r.selection==1 && host.events==std::vector<int>{2});
        host.events.clear();
        in.flags=1u;in.budget=20;
        r=begin_candidate_filter(in,out,host);
        assert(r.selection==0 && r.working_budget==0 && host.events.empty());
        in.flags=2u;in.budget=20;
        r=begin_candidate_filter(in,out,host);
        assert(r.selection==1 && r.working_budget==20 && host.events.empty());
        in.flags=2u;in.budget=1000;
        r=begin_candidate_filter(in,out,host);
        assert(r.selection==1 && r.working_budget==100);
    }
    {
        // Debug bit 8 causes an additional 0x2206F500, independent of mode.
        Host host;host.admit=false;host.debug_admit=false;
        Output out;Inputs in;in.destination={6,8,0};in.budget=4;in.flags=8;
        auto r=begin_candidate_filter(in,out,host);
        assert(r.selection==2);
        assert((host.events==std::vector<int>{2,8,99}));
        host.events.clear();host.debug_admit=true;
        in.flags=1u|8u;
        r=begin_candidate_filter(in,out,host);
        assert(r.selection==0 && (host.events==std::vector<int>{8}));
    }
    {
        Host host;Output out;Inputs in;in.destination={3,4,0};
        in.budget=std::numeric_limits<float>::infinity();
        auto r=begin_candidate_filter(in,out,host);
        assert(r.state==ResultState::UnsupportedInput && host.events.empty());
        in.budget=4; in.source.x=std::numeric_limits<float>::quiet_NaN();
        r=begin_candidate_filter(in,out,host);
        assert(r.state==ResultState::UnsupportedInput && host.events.empty());
    }
    {
        Output out;out.status=-3;out.entity_key=0x1234u;
        out.distance_delta=9;out.default_vector={3,4,5};out.source={8,9,10};
        AssociationHost host;
        // Source pointer null => original caller does not inspect anything.
        assert(!original_caller_clear_matching_association(out,0,{11,12,13},host));
        assert(host.events.empty() && out.status==-3 && out.entity_key==0x1234u);
        // Direct equality must still evaluate +0x120 before equality branch.
        assert(original_caller_clear_matching_association(out,0x1234u,{11,12,13},host));
        assert((host.events==std::vector<int>{120}));
        assert(out.status==0 && out.entity_key==0 && out.distance_delta==0);
        assert(out.default_vector.x==11 && out.default_vector.y==12 && out.default_vector.z==13);
        assert(out.source.x==8 && out.source.y==9 && out.source.z==10);
    }
    {
        Output out;out.status=-1;out.entity_key=0xABCD;out.distance_delta=10;
        AssociationHost host;host.accepted_type=true;host.recognized=true;
        host.linked_key=0xABCD;
        assert(original_caller_clear_matching_association(out,0x1234u,{0,0,0},host));
        assert((host.events==std::vector<int>{120,35100,35090}));
        assert(out.status==0 && out.entity_key==0 && out.distance_delta==0);
    }
    {
        Output out;out.status=-2;out.entity_key=0x1234;out.distance_delta=4;
        AssociationHost host;host.accepted_type=true;host.recognized=true;
        host.linked_key=0x5678;
        assert(!original_caller_clear_matching_association(out,0x9ABC,{9,9,9},host));
        assert((host.events==std::vector<int>{120,35100,35090}));
        assert(out.status==-2 && out.entity_key==0x1234 && out.distance_delta==4);
        host.events.clear();host.recognized=false;
        assert(!original_caller_clear_matching_association(out,0x9ABC,{9,9,9},host));
        assert((host.events==std::vector<int>{120,35100}));
        // Even a failed intermediate association predicate does not prevent
        // direct pointer-match invalidation in the native caller.
        host.events.clear();
        assert(original_caller_clear_matching_association(out,0x1234,{1,2,3},host));
        assert((host.events==std::vector<int>{120,35100}));
    }
    std::cout << "PHASE576_HOST_PASS\n";
}