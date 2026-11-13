#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <limits>
#include <vector>
#include "sin_compat/original_client_snapshot_record_router.hpp"
#include "sin_compat/original_client_prediction_snapshot_dispatch.hpp"

namespace router=sin_compat::original_client_snapshot_record_router;
namespace chain=sin_compat::original_client_prediction_snapshot_dispatch;
struct ObservingHost {
    int filtered=0,noop=0,specialist=0,nested=0,warned=0;
    std::vector<std::uint32_t> case_vas{};
    std::vector<std::uint8_t> case_ids{};
    void on_filtered(const router::RouteInput&) {++filtered;}
    void on_no_action(const router::RouteInput&) {++noop;}
    void on_opaque_specialist(const router::RouteInput&,std::uint8_t id,
                              std::uint32_t va) {
        ++specialist;case_ids.push_back(id);case_vas.push_back(va);
    }
    void on_opaque_nested_record(const router::RouteInput&,
                                 std::uint32_t va) {
        ++nested;case_ids.push_back(10u);case_vas.push_back(va);
    }
    void on_native_warning(const router::RouteInput&,std::uint32_t va) {
        ++warned;case_ids.push_back(12u);case_vas.push_back(va);
    }
};
static void check_exact_machine_code_tables() {
    static_assert(router::kNativeBodyBytes==1036u);
    static_assert(router::kOriginalRecordBytes==0x34u);
    static_assert(router::kNativeTypeSelectorVA==0x24199b30u);
    static_assert(router::kNativeCaseJumpTableVA==0x24199afcu);
    static_assert(router::kNativeNextFunctionVA==chain::kOriginalRecordWalkVA);
    static_assert(router::kNativeWarningIATVA==0x242761e0u);
    const std::uint8_t expected[27]{
        0,1,2,3,4,5,6,7,8,9,10,0,0,11,0,0,
        0,0,0,0,12,0,12,12,12,12,0
    };
    for(unsigned i=0;i<27u;++i) {
        assert(router::kTypeCases[i]==expected[i]);
        const auto r=router::classify({i,0u,0u,false});
        assert(r.case_index==expected[i]);
        assert(r.original_branch_va==router::kCaseDestinations[expected[i]]);
        const auto kind= expected[i]==0u?router::RouteKind::no_action:
                         expected[i]==10u?router::RouteKind::nested_records:
                         expected[i]==12u?router::RouteKind::warning:
                         router::RouteKind::specialist;
        assert(r.kind==kind);
    }
    for(unsigned i=0;i<13u;++i) {
        assert(router::kCaseDestinations[i]>=router::kOriginalRecordWriterVA);
        assert(router::kCaseDestinations[i]<
               router::kOriginalRecordWriterVA+router::kNativeBodyBytes);
    }
}
static void check_all_type_dispatches() {
    ObservingHost host;
    for(std::uint32_t i=0;i<=26u;++i)
        router::dispatch_case({i,0u,0u,false},host);
    assert(host.specialist==10); // cases 1..9,11
    assert(host.nested==1);      // type 10 recursion
    assert(host.warned==5);      // original types 20,22,23,24,25
    assert(host.noop==11);
    assert(host.filtered==0);
    for(const auto type:{27u,28u,255u,65535u,
                         std::numeric_limits<std::uint32_t>::max()}) {
        const auto r=router::classify({type,0u,0u,false});
        assert(r.kind==router::RouteKind::warning &&
               r.original_branch_va==router::kCaseDestinations[12]);
        router::dispatch_case({type,0u,0u,false},host);
    }
    assert(host.warned==10);
    assert(host.case_ids.size()==21u);
    assert(host.case_vas.back()==0x24199ac7u);
}
static void check_x86_DH_flag_filter_exactness() {
    for(const auto type:{1u,2u,10u,13u,20u,27u}) {
        for(const auto flags:{std::uint16_t{0u},std::uint16_t{0x0100u},
                              std::uint16_t{0x0200u},std::uint16_t{0x0300u},
                              std::uint16_t{0x0500u}}) {
            for(const auto mode:{0u,1u,2u,3u}) {
                const router::RouteInput in{type,flags,mode,false};
                const bool expected=type==10u?false:
                    (flags&0x0200u)!=0u ||
                    (mode==1u && (flags&0x0100u)!=0u) ||
                    (mode==2u && (flags&0x0100u)==0u);
                assert(router::filter_out(in)==expected);
                assert((router::classify(in).kind==router::RouteKind::filtered)==expected);
                const router::RouteInput seen{type,flags,mode,true};
                assert(router::filter_out(seen));
                assert(router::classify(seen).kind==router::RouteKind::filtered);
            }
        }
    }
    assert(!router::filter_out({10u,0x0300u,2u,false}));
    assert(router::filter_out({10u,0u,0u,true}));
}
static void check_callbacks_do_not_impersonate_original_serializer() {
    ObservingHost host;
    router::dispatch_case({1u,0u,0u,false},host);
    router::dispatch_case({10u,0x0300u,2u,false},host);
    router::dispatch_case({20u,0u,0u,false},host);
    router::dispatch_case({0u,0u,0u,false},host);
    router::dispatch_case({2u,0x0200u,0u,false},host);
    assert(host.specialist==1 && host.nested==1 && host.warned==1);
    assert(host.noop==1 && host.filtered==1);
    assert(host.case_ids[0]==1u);
    assert(host.case_vas[0]==0x24199847u);
    assert(host.case_ids[1]==10u);
    assert(host.case_vas[1]==0x241997ceu);
    assert(host.case_ids[2]==12u);
    assert(host.case_vas[2]==0x24199ac7u);
}
int main() {
    check_exact_machine_code_tables();
    check_all_type_dispatches();
    check_x86_DH_flag_filter_exactness();
    check_callbacks_do_not_impersonate_original_serializer();
}
