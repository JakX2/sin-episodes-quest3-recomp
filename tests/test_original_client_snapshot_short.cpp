#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_client_snapshot_short.hpp"

namespace native=sin_compat::original_client_snapshot_short;
using Bytes=native::Bytes;
struct Sink final : native::ITraceSink {
    std::vector<native::TraceEvent> log;
    void on_event(const native::TraceEvent& e) override { log.push_back(e); }
};
static native::OriginalFields active() {
    return {1u,0u,1u,0xabcdef01u,0x01234567u};
}
static void verify_authentic_addresses() {
    static_assert(native::kCompareVA==0x24197D50u);
    static_assert(native::kWordCopyVA==0x241989E0u);
    static_assert(native::kDiagnosticsVA==0x24197B10u);
    static_assert(native::kPreviewVA==0x24198380u);
    static_assert(native::kCompareCaller==0x241999F4u);
    static_assert(native::kCopyCaller==0x24199A05u);
    static_assert(native::kDiagnosticsCaller==0x24199A10u);
    static_assert(native::kPreviewCaller==0x24199A1Fu);
}
static void test_16bit_comparison_signed_count_and_guards() {
    auto state=active();
    assert(native::compare_words(state,Bytes{0xfe,0xff,0x01,0x00},0,
                                       Bytes{0xfe,0xff,0x01,0x00},0,2)==1u);
    assert(native::compare_words(state,Bytes{0xfe,0xff,0x01,0x00},0,
                                       Bytes{0xfe,0xff,0x00,0x00},0,2)==0u);
    assert(native::compare_words(state,Bytes{1,0},0,Bytes{2,0},0,2)==0u);
    assert(native::compare_words(state,{},999,{},999,0)==1u);
    assert(native::compare_words(state,{},999,{},999,-1)==1u);
    state.descriptor_flags_13=4u;
    assert(native::compare_words(state,{},999,{},999,100)==1u);
    state.enabled_14=0u;
    assert(native::compare_words(state,{},999,{},999,100)==0u);
    state.enabled_14=1u;state.descriptor_flags_13=0u;
    bool failed=false;
    try {native::compare_words(state,Bytes{1,0},0,Bytes{1,0},0,2);}
    catch(const std::out_of_range&) { failed=true; }
    assert(failed);
}
static void test_exact_signed_short_reads() {
    assert(native::read_signed_short(Bytes{0,0},0)==0);
    assert(native::read_signed_short(Bytes{0xff,0x7f},0)==32767);
    assert(native::read_signed_short(Bytes{0x00,0x80},0)==-32768);
    assert(native::read_signed_short(Bytes{0xff,0xff},0)==-1);
    assert(native::read_signed_short(Bytes{9,0xfe,0xff},1)==-2);
    bool failed=false;
    try {native::read_signed_short(Bytes{0xff},0);}
    catch(const std::out_of_range&) { failed=true; }
    assert(failed);
}
static void test_exact_word_count_copy_and_reps() {
    const auto state=active();
    const Bytes net{1,2,3,4,5,6,7,8};
    Bytes pred(10,0xeeu);
    native::copy_words(state,0u,net,0u,pred,1u,3u);
    assert((Bytes(pred.begin()+1,pred.begin()+7)==Bytes{1,2,3,4,5,6}));
    assert(pred[0]==0xeeu && pred[7]==0xeeu);
    const auto old=pred;
    native::copy_words(state,1u,net,999u,pred,999u,0xffffffffu);
    assert(pred==old);
    auto disabled=state;
    disabled.copy_enabled_2c=0u;
    native::copy_words(disabled,0u,net,999u,pred,999u,0xffffffffu);
    assert(pred==old);
    bool failed=false;
    try {native::copy_words(state,0u,net,0u,pred,0u,0x80000000u);}
    catch(const std::overflow_error&) {failed=true;}
    assert(failed);
    // x86 REP MOVSD snapshots a whole DWORD before each forward write.
    Bytes overlap{0,1,2,3,4,5,6,7,8,9};
    native::copy_words(state,0u,overlap,0u,overlap,1u,3u);
    // Tail REP MOVSB cascades too: the first tail store overwrites the\n    // source of the second tail read in this +1 overlap.\n    assert((overlap==Bytes{0,0,1,2,3,3,3,7,8,9}));
}
static void test_signed_diagnostics_original_order() {
    const auto state=active();
    Sink sink;
    native::diagnostics(state,0u,Bytes{0x00,0x80},0u,
                        Bytes{0xff,0x7f},0u,sink);
    assert(sink.log.size()==2u);
    assert(sink.log[0].kind==native::TraceKind::Difference);
    assert(sink.log[0].predicted==-32768 &&
           sink.log[0].net==32767 &&
           sink.log[0].predicted_minus_net==-65535);
    assert(sink.log[1].kind==native::TraceKind::Status);
    assert(sink.log[1].predicted==-32768 && sink.log[1].status==0u);
    sink.log.clear();
    native::diagnostics(state,1u,Bytes{0xfe,0xff},0u,{},999u,sink);
    assert(sink.log.size()==1u &&
           sink.log[0].kind==native::TraceKind::Status &&
           sink.log[0].predicted==-2);
    sink.log.clear();
    auto skip=state;skip.enabled_14=0u;
    native::diagnostics(skip,0u,{},999u,{},999u,sink);
    assert(sink.log.empty());
}
static void test_preview_separate_state_gate() {
    auto state=active();
    Sink sink;
    native::preview(state,{},999u,sink);
    assert(sink.log.empty());
    state.preview_token_34=state.descriptor_token_18;
    state.enabled_14=0u;
    native::preview(state,Bytes{0x00,0x80},0u,sink);
    assert(sink.log.size()==1u);
    assert(sink.log[0].kind==native::TraceKind::Preview &&
           sink.log[0].predicted==-32768);
}
static void test_phase212_real_four_calls_postcopy_trace() {
    auto state=active();
    Sink sink;
    Bytes predicted{0x34,0x12,0xfe,0xff};
    const Bytes net{0xff,0xff,0xfe,0xff};
    const auto status=native::run_type7_direct_calls(
        state,net,0u,predicted,0u,2,sink);
    assert(status==0u && predicted==net);
    assert(sink.log.size()==2u);
    assert(sink.log[0].kind==native::TraceKind::Difference);
    assert(sink.log[0].predicted==-1 && sink.log[0].net==-1);
    assert(sink.log[0].predicted_minus_net==0);
    assert(sink.log[1].predicted==-1);
    sink.log.clear();
    const auto exact=native::run_type7_direct_calls(
        state,net,0u,predicted,0u,2,sink);
    assert(exact==1u && sink.log.size()==1u &&
           sink.log[0].kind==native::TraceKind::Status);
    sink.log.clear();
    state.copy_enabled_2c=0u;
    predicted={0x34,0x12,0xfe,0xff};
    const auto notcopied=native::run_type7_direct_calls(
        state,net,0u,predicted,0u,2,sink);
    assert(notcopied==0u && predicted[0]==0x34u);
    assert(sink.log[0].predicted==0x1234 &&
           sink.log[0].net==-1 &&
           sink.log[0].predicted_minus_net==0x1235);
    sink.log.clear();
    state.preview_token_34=state.descriptor_token_18;
    state.enabled_14=0u;
    state.copy_enabled_2c=1u;
    predicted={0x34,0x12,0xfe,0xff};
    const auto disabled=native::run_type7_direct_calls(
        state,net,0u,predicted,0u,2,sink);
    assert(disabled==0u && predicted==net);
    assert(sink.log.size()==1u &&
           sink.log[0].kind==native::TraceKind::Preview);
}
static void test_negative_count_host_only_refusal() {
    auto state=active();
    Sink sink;
    Bytes pred{1,0};
    const Bytes net{1,0};
    const auto skipped=native::run_type7_direct_calls(
        state,net,0u,pred,0u,-1,sink);
    assert(skipped==1u && sink.log.size()==1u);
    state.enabled_14=0u;
    bool rejected=false;
    try {native::run_type7_direct_calls(state,net,0u,pred,0u,-1,sink);}
    catch(const std::overflow_error&) {rejected=true;}
    assert(rejected);
}
int main() {
    verify_authentic_addresses();
    test_16bit_comparison_signed_count_and_guards();
    test_exact_signed_short_reads();
    test_exact_word_count_copy_and_reps();
    test_signed_diagnostics_original_order();
    test_preview_separate_state_gate();
    test_phase212_real_four_calls_postcopy_trace();
    test_negative_count_host_only_refusal();
}
