#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include "sin_compat/original_client_snapshot_boolean.hpp"

namespace native=sin_compat::original_client_snapshot_boolean;
using Bytes=native::Bytes;
struct RecordingSink final: native::ITraceSink {
    std::vector<native::TraceEvent> events;
    void on_event(const native::TraceEvent& event) override {
        events.push_back(event);
    }
};
static native::OriginalFields fields() {
    return {1u,0u,1u,0x12340000u,0x20000000u};
}
static void verify_original_function_addresses() {
    static_assert(native::kCompareVA==0x24197E30u);
    static_assert(native::kCopyVA==0x24198A40u);
    static_assert(native::kDiagnosticsVA==0x24197C30u);
    static_assert(native::kPreviewVA==0x24198440u);
    static_assert(native::kCompareCaller==0x24199982u);
    static_assert(native::kCopyCaller==0x24199993u);
    static_assert(native::kDiagnosticsCaller==0x2419999Eu);
    static_assert(native::kPreviewCaller==0x241999ADu);
}
static void compare_raw_byte_not_boolean_truth() {
    auto f=fields();
    assert(native::compare(f,Bytes{0,1,0xff},0,Bytes{0,1,0xff},0,3)==1u);
    assert(native::compare(f,Bytes{0,2},0,Bytes{0,1},0,2)==0u);
    assert(native::compare(f,Bytes{0xff},0,Bytes{1},0,1)==0u);
    assert(std::string(native::boolean_text(0xff))=="true");
    assert(std::string(native::boolean_text(1))=="true");
    assert(std::string(native::boolean_text(0))=="false");
    assert(native::compare(f,Bytes{0},0,Bytes{1},0,100)==0u);
    assert(native::compare(f,{},99,{},99,0)==1u);
    assert(native::compare(f,{},99,{},99,-1)==1u);
    f.descriptor_flags_13=4u;
    assert(native::compare(f,{},99,{},99,100)==1u);
    f.enabled_14=0u;
    assert(native::compare(f,{},99,{},99,100)==0u);
    f.enabled_14=1u;
    f.descriptor_flags_13=0u;
    bool refused=false;
    try {native::compare(f,Bytes{0},0,Bytes{0},0,2);}
    catch(const std::out_of_range&) {refused=true;}
    assert(refused);
}
static void diagnostic_status_and_difference_order() {
    auto f=fields(); RecordingSink sink;
    native::diagnostics(f,0u,Bytes{0},0,Bytes{1},0,sink);
    assert(sink.events.size()==2u);
    assert(sink.events[0].kind==native::TraceKind::Difference);
    assert(sink.events[0].net=="false");
    assert(sink.events[0].predicted=="true");
    assert(sink.events[1].kind==native::TraceKind::Status);
    assert(sink.events[1].predicted=="true");
    sink.events.clear();
    native::diagnostics(f,1u,{},999,Bytes{0},0,sink);
    assert(sink.events.size()==1u);
    assert(sink.events[0].kind==native::TraceKind::Status);
    assert(sink.events[0].predicted=="false");
    sink.events.clear();
    f.enabled_14=0u;
    native::diagnostics(f,0u,{},999,{},999,sink);
    assert(sink.events.empty());
}
static void preview_uses_separate_pointer_token_gate() {
    auto f=fields(); RecordingSink sink;
    native::preview(f,{},999,sink);
    assert(sink.events.empty());
    f.enabled_14=0u;
    f.preview_token_34=f.descriptor_token_18;
    native::preview(f,Bytes{0},0,sink);
    assert(sink.events.size()==1u);
    assert(sink.events[0].kind==native::TraceKind::Preview);
    assert(sink.events[0].predicted=="false");
    native::preview(f,Bytes{0xfe},0,sink);
    assert(sink.events.size()==2u);
    assert(sink.events[1].predicted=="true");
}
static void type6_four_call_order_and_post_copy_tracing() {
    auto f=fields(); RecordingSink sink;
    Bytes predicted{0,0,0,0,0}; Bytes source{1,0xff,0,1,0};
    const auto mismatch=native::run_type6_direct_calls(
        f,source,0u,predicted,0u,5u,sink);
    assert(mismatch==0u && predicted==source);
    assert(sink.events.size()==2u);
    assert(sink.events[0].kind==native::TraceKind::Difference);
    assert(sink.events[0].net=="true" && sink.events[0].predicted=="true");
    assert(sink.events[1].predicted=="true");
    sink.events.clear();
    const auto equal=native::run_type6_direct_calls(
        f,source,0u,predicted,0u,5u,sink);
    assert(equal==1u && sink.events.size()==1u);
    assert(sink.events[0].operation==1u);
    sink.events.clear();
    f.copy_enabled_2c=0u;
    predicted=Bytes{0,0,0,0,0};
    const auto skipped=native::run_type6_direct_calls(
        f,source,0u,predicted,0u,5u,sink);
    assert(skipped==0u && predicted[0]==0u);
    assert(sink.events.size()==2u);
    assert(sink.events[0].net=="true" && sink.events[0].predicted=="false");
    sink.events.clear();
    f.copy_enabled_2c=1u;
    f.enabled_14=0u;
    f.preview_token_34=f.descriptor_token_18;
    const auto disabled=native::run_type6_direct_calls(
        f,source,0u,predicted,0u,5u,sink);
    assert(disabled==0u && predicted==source);
    assert(sink.events.size()==1u);
    assert(sink.events[0].kind==native::TraceKind::Preview);
}
static void type6_overlap_and_native_count_limits() {
    auto f=fields(); RecordingSink sink;
    Bytes overlap{0,1,2,3,4,5,6,7,8};
    const auto operation=native::run_type6_direct_calls(
        f,overlap,0u,overlap,1u,6u,sink);
    assert(operation==0u);
    const Bytes expected{0,0,1,2,3,3,3,7,8};
    assert(overlap==expected);
    assert(sink.events[0].net=="false");
    assert(sink.events[0].predicted=="false");

    f.descriptor_flags_13=4u;
    sink.events.clear();
    Bytes pred{1}; const Bytes net{1};
    const auto forced=native::run_type6_direct_calls(
        f,net,0u,pred,0u,0xffffffffu,sink);
    assert(forced==1u);
    assert(sink.events.size()==1u);
    // Sign bit set: compare signed count <= 0 but copy uses unsigned
    // byte count. The host refuses the unsafe native memory copy.
    f.descriptor_flags_13=0u;
    f.enabled_14=0u;
    bool refused=false;
    try {native::run_type6_direct_calls(
        f,net,0u,pred,0u,0xffffffffu,sink);}
    catch(const std::overflow_error&) {refused=true;}
    assert(refused);
    f.copy_enabled_2c=0u;
    const auto skipped=native::run_type6_direct_calls(
        f,net,0u,pred,0u,0xffffffffu,sink);
    assert(skipped==0u);
    assert(pred==Bytes{1});
}
int main() {
    verify_original_function_addresses();
    compare_raw_byte_not_boolean_truth();
    diagnostic_status_and_difference_order();
    preview_uses_separate_pointer_token_gate();
    type6_four_call_order_and_post_copy_tracing();
    type6_overlap_and_native_count_limits();
}
