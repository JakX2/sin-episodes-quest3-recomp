#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_client_snapshot_quaternion.hpp"

namespace q=sin_compat::original_client_snapshot_quaternion;
using Bytes=q::Bytes;
struct Sink final:q::ITraceSink {
    std::vector<q::TraceEvent> events;
    void on_event(const q::TraceEvent& value) override {events.push_back(value);}
};
static Bytes encoded(std::initializer_list<float> floats) {
    Bytes result;
    for(float f:floats) {
        std::uint32_t bits{};
        std::memcpy(&bits,&f,sizeof(bits));
        for(unsigned shift:{0u,8u,16u,24u})
            result.push_back(static_cast<std::uint8_t>(bits>>shift));
    }
    return result;
}
static q::OriginalFields fields() {
    return {1u,0u,1u,0.5f,0x100u,0x200u};
}
static void addresses() {
    static_assert(q::kCompareVA==0x24198F50u);
    static_assert(q::kCopyVA==0x24198DE0u);
    static_assert(q::kDiagnosticsVA==0x241987C0u);
    static_assert(q::kPreviewVA==0x241989A0u);
    static_assert(q::kCompareCall==0x241998ECu);
    static_assert(q::kCopyCall==0x241998FDu);
    static_assert(q::kDiagnosticsCall==0x24199908u);
    static_assert(q::kPreviewCall==0x24199917u);
}
static void original_exact_forced_and_disabled_gates() {
    auto f=fields();
    const auto source=encoded({1,2,3,4});
    assert(q::compare(f,source,0,source,0,1)==1u);
    assert(q::compare(f,{},999,{},999,0)==1u);
    assert(q::compare(f,{},999,{},999,-7)==1u);
    f.descriptor_flags_13=4u;
    assert(q::compare(f,{},999,{},999,7)==1u);
    f.enabled_14=0u;
    assert(q::compare(f,{},999,{},999,7)==0u);
    f.enabled_14=1u;f.descriptor_flags_13=0u;
    bool refused=false;
    try{q::compare(f,source,0,source,0,2);}
    catch(const std::out_of_range&){refused=true;}
    assert(refused);
}
static void original_unordered_x87_parity_gate() {
    const float nan=std::numeric_limits<float>::quiet_NaN();
    assert(!q::native_ordered_equal(nan,1.0f));
    assert(!q::native_ordered_equal(nan,nan));
    assert(!q::native_ordered_equal(1.0f,2.0f));
    assert(q::native_ordered_equal(+0.0f,-0.0f));
    auto f=fields();
    // Original TEST AH,0x44; JP is TAKEN for unordered: AH&0x44
    // has TWO set bits (even parity). Without positive tolerance,
    // this is a status-0 mismatch; with tolerance, scratch is needed.
    f.descriptor_tolerance_30=0.0f;
    assert(q::compare(f,encoded({nan,2,3,4}),0,
                        encoded({1,2,3,4}),0,1)==0u);
    f.descriptor_tolerance_30=0.5f;
    bool missing=false;
    try {q::compare(f,encoded({nan,2,3,4}),0,
                      encoded({1,2,3,4}),0,1);}
    catch(const q::MissingOriginalScratch&) {missing=true;}
    assert(missing);
    assert(q::compare(f,encoded({+0.0f,2,3,4}),0,
                        encoded({-0.0f,2,3,4}),0,1)==1u);
}
static void scratch_dependent_tolerance_not_fabricated_vec4_math() {
    auto f=fields();
    const auto net=encoded({2,2,3,4});
    const auto predicted=encoded({1,2,3,4});
    bool missing=false;
    try {q::compare(f,predicted,0,net,0,1);}
    catch(const q::MissingOriginalScratch&){missing=true;}
    assert(missing);
    // Component X mismatches but original computes/stores only W delta,
    // at local stack index [record], then tests all four scratch values.
    q::ExplicitScratch scratch{{0.0f,0.0f,0.0f,0.0f}};
    assert(q::compare(f,predicted,0,net,0,1,&scratch)==2u);
    assert(scratch.slots[0]==0.0f);
    scratch={{0.0f,10.0f,0.0f,0.0f}};
    assert(q::compare(f,predicted,0,net,0,1,&scratch)==0u);
    assert(scratch.slots[1]==10.0f);
    scratch={{0.0f,0.0f,0.0f,0.0f}};
    assert(q::compare(f,encoded({1,2,3,6}),0,net,0,1,&scratch)==0u);
    assert(scratch.slots[0]==2.0f);
    // If signed count exceeds four, only a nonexact record with index>=4
    // attempts an original store outside these four local scratch slots.
    const auto many=encoded({1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4});
    const auto changed=encoded({1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4,2,2,3,4});
    bool unsafe=false;
    try {q::compare(f,many,0,changed,0,5,&scratch);}
    catch(const std::overflow_error&){unsafe=true;}
    assert(unsafe);
    // Tolerance<=0 returns mismatch even without a caller-provided scratch.
    f.descriptor_tolerance_30=0.0f;
    assert(q::compare(f,predicted,0,net,0,1)==0u);
    f.descriptor_tolerance_30=-1.0f;
    assert(q::compare(f,predicted,0,net,0,1)==0u);
}
static void diagnostics_first_quaternion_and_preview_gates() {
    auto f=fields();Sink sink;
    q::diagnostics(f,0u,encoded({2,4,6,8}),0,
                     encoded({1,3,5,7}),0,sink);
    assert(sink.events.size()==2u);
    assert(sink.events[0].kind==q::TraceKind::Difference);
    assert(sink.events[0].net[0]==1.0f);
    assert(sink.events[0].predicted[3]==8.0f);
    assert(sink.events[0].verified_fourth_component_delta==1.0f);
    assert(!sink.events[0].native_format_argument_order_fully_reconstructed);
    assert(sink.events[1].kind==q::TraceKind::Status);
    sink.events.clear();
    q::diagnostics(f,2u,encoded({1,2,3,4}),0,{},999,sink);
    assert(sink.events.size()==1u && sink.events[0].status==2u);
    sink.events.clear();f.enabled_14=0u;
    q::diagnostics(f,0u,{},999,{},999,sink);
    assert(sink.events.empty());
    q::preview(f,{},999,sink);
    assert(sink.events.empty());
    f.preview_token_34=f.descriptor_token_18;
    q::preview(f,encoded({9,8,7,6}),0,sink);
    assert(sink.events.size()==1u);
    assert(sink.events[0].kind==q::TraceKind::Preview);
    assert(sink.events[0].predicted[2]==7.0f);
}
static void four_router_calls_post_copy_observation() {
    auto f=fields();Sink sink;
    Bytes pred=encoded({1,2,3,4});
    const auto net=encoded({2,2,3,4});
    q::ExplicitScratch scratch{{0.0f,0.0f,0.0f,0.0f}};
    auto status=q::run_direct_type4_calls(f,net,0,pred,0,1,sink,&scratch);
    assert(status==2u && pred==net);
    assert(sink.events.size()==1u &&
           sink.events[0].kind==q::TraceKind::Status);
    sink.events.clear();
    pred=encoded({1,2,3,5});
    const auto different=encoded({2,2,3,4});
    scratch={{0.0f,0.0f,0.0f,0.0f}};
    status=q::run_direct_type4_calls(f,different,0,pred,0,1,sink,&scratch);
    assert(status==0u && pred==different);
    assert(sink.events.size()==2u);
    assert(sink.events[0].predicted[0]==2.0f);
    assert(sink.events[0].net[0]==2.0f);
    assert(sink.events[0].verified_fourth_component_delta==0.0f);
    sink.events.clear();f.copy_enabled_2c=0u;
    pred=encoded({1,2,3,5});
    scratch={{0.0f,0.0f,0.0f,0.0f}};
    status=q::run_direct_type4_calls(f,different,0,pred,0,1,sink,&scratch);
    assert(status==0u && pred!=different);
    assert(sink.events[0].verified_fourth_component_delta==1.0f);
    sink.events.clear();f.enabled_14=0u;f.copy_enabled_2c=1u;
    f.preview_token_34=f.descriptor_token_18;
    status=q::run_direct_type4_calls(f,different,0,pred,0,1,sink);
    assert(status==0u && pred==different);
    assert(sink.events.size()==1u &&
           sink.events[0].kind==q::TraceKind::Preview);
}
int main() {
    addresses();
    original_exact_forced_and_disabled_gates();
    original_unordered_x87_parity_gate();
    scratch_dependent_tolerance_not_fabricated_vec4_math();
    diagnostics_first_quaternion_and_preview_gates();
    four_router_calls_post_copy_observation();
}
