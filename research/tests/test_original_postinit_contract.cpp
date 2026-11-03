#include "sin_compat/original_postinit_contract.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <array>
#include <string>
#include <vector>
using namespace sin_compat::postinit;
int main() {
    static_assert(client_required_late.size()==7 && server_required_late.size()==4);
    assert(client_required_late[0].label=="MaterialSystemHardwareConfig012");
    assert(client_required_late[4].label=="MatSystemSurface003");
    assert(server_required_late[0].label=="sv_cheats");
    assert(server_event_resources[0]=="resource/gameevents.res");
    assert(server_event_resources[1]=="resource/ModEvents.res");
    std::vector<std::string> trace;
    auto yes=[&](const Gate& step) {trace.emplace_back(step.label);return true;};
    auto c=check_required_late(client_required_late,yes);
    assert(c.all_passed && c.first_failed_index==7 && trace.size()==7);
    trace.clear();
    auto s=check_required_late(server_required_late,yes);
    assert(s.all_passed && s.first_failed_index==4 && trace.size()==4);
    for(std::size_t failed=0;failed<client_required_late.size();++failed) {
        std::size_t index=0;trace.clear();
        auto result=check_required_late(client_required_late,[&](const Gate& step) {
            trace.emplace_back(step.label);return index++!=failed;
        });
        assert(!result.all_passed && result.first_failed_index==failed);
        assert(trace.size()==failed+1);
    }
    for(std::size_t failed=0;failed<server_required_late.size();++failed) {
        std::size_t index=0;trace.clear();
        auto result=check_required_late(server_required_late,[&](const Gate& step) {
            trace.emplace_back(step.label);return index++!=failed;
        });
        assert(!result.all_passed && result.first_failed_index==failed);
        assert(trace.size()==failed+1);
    }
    assert(server_allocation_branch(true)==ServerAllocationBranch::Constructed);
    assert(server_allocation_branch(false)==ServerAllocationBranch::NullFallback);
    assert(client_nonfatal_factory=="VMaterialSystemStub001");
    assert(server_nonfatal_factory=="VDebugOverlay003");
}
