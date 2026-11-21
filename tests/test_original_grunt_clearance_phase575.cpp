#include "sin_compat/original_grunt_clearance_phase575.hpp"
#include <cassert>
#include <string>
#include <vector>
using namespace sin_compat::phase575;
struct Host {
    Trace first{},second{};
    std::uint8_t candidate_ah=1;
    bool virtual_f4_reply=false;
    int virtual_f4_calls=0;
    std::vector<std::string> log;
    Trace primary_trace_2206e730(){log.push_back("primary");return first;}
    Trace secondary_trace_2206e730(float added_z){assert(added_z==0.0625f);log.push_back("secondary");return second;}
    bool entity_virtual_f4(const EntityHit&){++virtual_f4_calls;log.push_back("virtual_f4");return virtual_f4_reply;}
    std::uint8_t candidate_distance_comparison_ah(Vec3 source){assert(source.x==4.f);log.push_back("distance_ah");return candidate_ah;}
};
int main(){
    static_assert(original_fraction_branch(0x00));
    static_assert(!original_fraction_branch(0x40));
    static_assert(original_fraction_branch(0x44));
    static_assert(original_near_candidate(0x01));
    static_assert(original_near_candidate(0x04));
    static_assert(!original_near_candidate(0x00));
    const Vec3 src{4.f,7.f,10.f}, chosen{1.f,2.f,3.f};
    {
        Host h;h.first.calculated_point=chosen;
        auto o=clearance_select(h,src);
        assert(o.accepted&&o.path==ReturnPath::TracePointAccepted);
        assert(o.selected.x==1.f&&o.selected.z==3.f&&!o.second_trace_used);
        assert((h.log==std::vector<std::string>{"primary"}));
    }
    {
        Host h;h.first.fraction_compare_ah=0x40;
        auto o=clearance_select(h,src);
        assert(!o.accepted && o.path==ReturnPath::SourcePointRejected);
        assert(o.selected.x==4.f && o.selected.z==10.f);
    }
    {
        // State 6 accepts candidate directly: virtual +0xF4 is NOT queried.
        Host h;h.first.raw_flag_8f=true;h.first.hit=EntityHit{6};
        h.second.raw_flag_8f=true;h.second.calculated_point=chosen;
        auto o=clearance_select(h,src);
        assert(o.accepted && o.path==ReturnPath::SourcePointNearCandidate);
        assert(o.selected.x==4.f && o.second_trace_used);
        assert(h.virtual_f4_calls==0);
        assert((h.log==std::vector<std::string>{"primary","distance_ah","secondary"}));
    }
    {
        // Same near candidate, but second trace flag 8f is cleared:
        // normal trace XYZ wins the native final condition.
        Host h;h.first.raw_flag_8f=true;h.first.hit=EntityHit{6};
        h.second.calculated_point=chosen;
        auto o=clearance_select(h,src);
        assert(o.accepted && o.path==ReturnPath::TracePointAccepted);
        assert(o.selected.z==3.f && o.second_trace_used);
    }
    {
        // Entity state != 6 can still be admitted by its virtual +0xF4.
        Host h;h.first.raw_flag_8f=true;h.first.hit=EntityHit{3};
        h.virtual_f4_reply=true;
        h.second.raw_flag_8e=true;
        auto o=clearance_select(h,src);
        assert(o.accepted && o.path==ReturnPath::SourcePointNearCandidate);
        assert(h.virtual_f4_calls==1);
        assert((h.log==std::vector<std::string>{"primary","virtual_f4","distance_ah","secondary"}));
    }
    {
        // Not eligible and second trace rejects: return caller XYZ + false.
        Host h;h.first.raw_flag_8f=true;h.first.hit=EntityHit{2};
        h.second.raw_flag_8e=true;
        auto o=clearance_select(h,src);
        assert(!o.accepted && o.path==ReturnPath::SourcePointRejected);
        assert(o.selected.x==4.f && o.second_trace_used);
    }
    {
        // Candidate accepted but native FPU near predicate is false.
        Host h;h.first.raw_flag_8f=true;h.first.hit=EntityHit{6};
        h.candidate_ah=0;
        h.second.raw_flag_8f=true;h.second.calculated_point=chosen;
        auto o=clearance_select(h,src);
        assert(o.accepted && o.path==ReturnPath::TracePointAccepted);
    }
    return 0;
}