#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_client_snapshot_vec3.hpp"

namespace native=sin_compat::original_client_snapshot_vec3;
using Bytes=native::Bytes;
struct RecordingSink final:native::ITraceSink {
    std::vector<native::TraceEvent> events;
    void on_event(const native::TraceEvent& x) override {events.push_back(x);}
};
static Bytes vec(std::initializer_list<float> values) {
    Bytes result;
    for(const float value:values) {
        std::uint32_t bits{};
        static_assert(sizeof(bits)==sizeof(value));
        std::memcpy(&bits,&value,sizeof(value));
        for(unsigned shift:{0u,8u,16u,24u})
            result.push_back(static_cast<std::uint8_t>(bits>>shift));
    }
    return result;
}
static native::OriginalFields active() {
    return {1u,0u,1u,0.5f,0x100u,0x200u};
}
static void verify_original_entrypoints() {
    static_assert(native::kCompareVA==0x24198E20u);
    static_assert(native::kCopyVA==0x24198DA0u);
    static_assert(native::kDiagnosticsVA==0x241985A0u);
    static_assert(native::kPreviewVA==0x241986C0u);
    static_assert(native::kCompareCall==0x241998B2u);
    static_assert(native::kCopyCall==0x241998C3u);
    static_assert(native::kDiagnosticsCall==0x241998CEu);
    static_assert(native::kPreviewCall==0x241998DDu);
}
static void test_float_exact_and_tolerance_status() {
    auto f=active();
    assert(native::compare(f,vec({1,2,3}),0,vec({1,2,3}),0,1)==1u);
    assert(native::compare(f,vec({1,2,3}),0,vec({1.25f,2.5f,2.875f}),0,1)==2u);
    assert(native::compare(f,vec({1,2,3}),0,vec({1.5f,2,3}),0,1)==2u);
    assert(native::compare(f,vec({1,2,3}),0,vec({1.75f,2,3}),0,1)==0u);
    assert(native::compare(f,vec({1,2,3,4,5,6}),0,
                              vec({1,2,3,4.25f,5.5f,6}),0,2)==2u);
    assert(native::compare(f,vec({1,2,3,4,5,6}),0,
                              vec({1,2,3,4,5,7}),0,2)==0u);
    // First record failure exits, so the second record is never accessed.
    assert(native::compare(f,vec({1,2,3}),0,vec({8,2,3}),0,2)==0u);
    // Signed count <=0 and descriptor override both avoid touching data.
    assert(native::compare(f,{},900,{},900,0)==1u);
    assert(native::compare(f,{},900,{},900,-12)==1u);
    f.descriptor_flags_13=0x04u;
    assert(native::compare(f,{},900,{},900,10)==1u);
    f.enabled_14=0u;
    assert(native::compare(f,{},900,{},900,10)==0u);
}
static void test_x87_special_float_values_and_tolerance_gate() {
    auto f=active();
    const float nan=std::numeric_limits<float>::quiet_NaN();
    const float inf=std::numeric_limits<float>::infinity();
    assert(native::compare(f,vec({+0.0f,inf,1}),0,
                              vec({-0.0f,inf,1}),0,1)==1u);
    assert(native::compare(f,vec({nan,1,1}),0,
                              vec({nan,1,1}),0,1)==0u);
    assert(native::compare(f,vec({1,nan,1}),0,
                              vec({1,2,1}),0,1)==0u);
    f.descriptor_tolerance_30=0.0f;
    assert(native::compare(f,vec({1,2,3}),0,
                              vec({1.25f,2,3}),0,1)==0u);
    f.descriptor_tolerance_30=-1.0f;
    assert(native::compare(f,vec({1,2,3}),0,
                              vec({1.25f,2,3}),0,1)==0u);
    f.descriptor_tolerance_30=nan;
    assert(native::compare(f,vec({1,2,3}),0,
                              vec({1.25f,2,3}),0,1)==0u);
    f.descriptor_tolerance_30=inf;
    assert(native::compare(f,vec({1,2,3}),0,
                              vec({11,22,33}),0,1)==2u);
    // x87 FSTP DWORD rounds each nonidentical component BEFORE comparison.
    const auto delta=native::delta_pred_minus_net(
        {1.0f,3.0f,5.0f},{0.5f,2.0f,4.0f});
    assert(delta[0]==0.5f && delta[1]==1.0f && delta[2]==1.0f);
    bool failed=false;
    try{native::compare(f,vec({1,2,3}),0,vec({1,2,3}),0,2);}
    catch(const std::out_of_range&){failed=true;}
    assert(failed);
}
static void test_diagnostic_and_preview_gates() {
    auto f=active();RecordingSink sink;
    native::diagnostics(f,0u,vec({2,4,6}),0,vec({1,2,3}),0,sink);
    assert(sink.events.size()==2u);
    assert(sink.events[0].kind==native::TraceKind::Difference);
    assert(sink.events[0].net[0]==1.0f &&
           sink.events[0].predicted[1]==4.0f &&
           sink.events[0].delta_pred_minus_net[2]==3.0f);
    assert(sink.events[1].kind==native::TraceKind::Status);
    assert(sink.events[1].predicted[0]==2.0f);
    sink.events.clear();
    native::diagnostics(f,2u,vec({2,4,6}),0,{},900,sink);
    assert(sink.events.size()==1u && sink.events[0].status==2u);
    f.enabled_14=0u;sink.events.clear();
    native::diagnostics(f,0u,{},900,{},900,sink);
    assert(sink.events.empty());
    native::preview(f,{},900,sink);
    assert(sink.events.empty());
    f.preview_token_34=f.descriptor_token_18;
    native::preview(f,vec({-1,0,8}),0,sink);
    assert(sink.events.size()==1u);
    assert(sink.events[0].kind==native::TraceKind::Preview);
    assert(sink.events[0].predicted[2]==8.0f);
}
static void test_phase212_four_direct_calls_and_post_copy_logs() {
    auto f=active();RecordingSink sink;
    Bytes predicted=vec({1,2,3});
    const Bytes net=vec({1.25f,2,3});
    auto status=native::run_type3_direct_calls(f,net,0,predicted,0,1,sink);
    assert(status==2u && predicted==net);
    assert(sink.events.size()==1u);
    assert(sink.events[0].kind==native::TraceKind::Status);
    assert(sink.events[0].predicted[0]==1.25f);
    sink.events.clear();
    predicted=vec({1,2,3});
    const Bytes different=vec({8,4,6});
    status=native::run_type3_direct_calls(f,different,0,predicted,0,1,sink);
    assert(status==0u && predicted==different);
    assert(sink.events.size()==2u);
    assert(sink.events[0].kind==native::TraceKind::Difference);
    // Original diagnostics run AFTER the copy. Both now read the new value.
    assert(sink.events[0].net[0]==8.0f &&
           sink.events[0].predicted[0]==8.0f &&
           sink.events[0].delta_pred_minus_net[0]==0.0f);
    sink.events.clear();
    f.copy_enabled_2c=0u;predicted=vec({1,2,3});
    status=native::run_type3_direct_calls(f,different,0,predicted,0,1,sink);
    assert(status==0u && predicted!=different);
    assert(sink.events[0].delta_pred_minus_net[0]==-7.0f);
    sink.events.clear();
    f.copy_enabled_2c=1u;
    f.enabled_14=0u;f.preview_token_34=f.descriptor_token_18;
    status=native::run_type3_direct_calls(f,different,0,predicted,0,1,sink);
    assert(status==0u && predicted==different);
    assert(sink.events.size()==1u);
    assert(sink.events[0].kind==native::TraceKind::Preview);
}
static void test_host_only_negative_copy_refusal() {
    auto f=active();RecordingSink sink;
    Bytes predicted=vec({1,2,3});const Bytes net=vec({1,2,3});
    assert(native::run_type3_direct_calls(
        f,net,0,predicted,0,-1,sink)==1u);
    assert(sink.events.size()==1u);
    f.enabled_14=0u;
    bool refused=false;
    try {native::run_type3_direct_calls(
        f,net,0,predicted,0,-1,sink);}
    catch(const std::overflow_error&){refused=true;}
    assert(refused);
}
int main() {
    verify_original_entrypoints();
    test_float_exact_and_tolerance_status();
    test_x87_special_float_values_and_tolerance_gate();
    test_diagnostic_and_preview_gates();
    test_phase212_four_direct_calls_and_post_copy_logs();
    test_host_only_negative_copy_refusal();
}
