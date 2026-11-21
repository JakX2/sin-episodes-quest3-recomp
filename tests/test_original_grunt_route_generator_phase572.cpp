#include "sin_compat/original_grunt_route_generator_phase572.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
using namespace sin_compat::phase572;
struct Fake {
    bool active = true, produce = false, built = false;
    NativeGateInputs gate{0, 0.5f, 0.f};
    std::vector<std::string> trace;
    void clear_candidate_holder(){trace.push_back("clear");}
    bool has_active_first(){trace.push_back("active"); return active;}
    NativeGateInputs gate_inputs(){trace.push_back("metric");return gate;}
    bool build_using_native_opaque_callbacks(){trace.push_back("produce");return produce;}
    bool has_built_candidate_first(){trace.push_back("built");return built;}
    void commit_first_pointer(){trace.push_back("commit");}
};
int main(){
    {
        Fake h;h.active=false;
        assert(generate_candidate_prefix(h)==Result::NoActiveWaypoint);
        assert((h.trace==std::vector<std::string>{"clear","active"}));
    }
    {
        Fake h;h.gate={0,0.1f,0.f};
        assert(generate_candidate_prefix(h)==Result::RejectedMetric);
        assert((h.trace==std::vector<std::string>{"clear","active","metric"}));
        h.trace.clear();h.gate.owner_measurement=0.1001f;
        assert(generate_candidate_prefix(h)==Result::NoCandidateProduced);
        assert(h.trace.back()=="produce");
    }
    {
        Fake h;h.gate={1,0.009f,0.01f};
        assert(generate_candidate_prefix(h)==Result::RejectedMetric);
        h.trace.clear();h.gate.computed_special_distance=0.0101f;
        h.produce=true;h.built=true;
        assert(generate_candidate_prefix(h)==Result::Installed);
        assert((h.trace==std::vector<std::string>{"clear","active","metric","produce","built","commit"}));
    }
    {
        Fake h;h.gate={3,0.011f,0.005f};h.produce=true;h.built=false;
        assert(generate_candidate_prefix(h)==Result::NoCandidateProduced);
        assert(h.trace.back()=="built");
        h.trace.clear();h.built=true;
        assert(generate_candidate_prefix(h)==Result::Installed);
        assert(h.trace.back()=="commit");
    }
    {
        Fake h;h.gate={2,0.10001f,0.f};h.produce=true;h.built=true;
        assert(generate_candidate_prefix(h)==Result::Installed);
        h.trace.clear();h.gate.owner_measurement=0.f;
        assert(generate_candidate_prefix(h)==Result::RejectedMetric);
    }
    {
        Fake h;h.gate={1,0.009f,0.5f};h.produce=true;h.built=true;
        assert(generate_candidate_prefix(h)==Result::Installed);
        h.trace.clear();h.gate.owner_measurement=std::nanf("");
        assert(generate_candidate_prefix(h)==Result::RejectedMetric);
        h.trace.clear();h.gate.owner_measurement=0.2f;
        h.gate.computed_special_distance=std::nanf("");
        assert(generate_candidate_prefix(h)==Result::RejectedMetric);
    }
}