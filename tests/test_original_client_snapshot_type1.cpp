#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

#include "sin_compat/original_client_snapshot_type1.hpp"

namespace native=sin_compat::original_client_snapshot_type1;
using Bytes=native::Bytes;
struct Trace final : native::ITraceSink {
    std::vector<native::TraceEvent> entries;
    void on_event(const native::TraceEvent& event) override { entries.push_back(event); }
};
static Bytes floats(std::initializer_list<float> list) {
    Bytes bytes;
    for(float f:list) {
        std::uint32_t bits=0;
        std::memcpy(&bits,&f,sizeof(bits));
        for(unsigned shift:{0u,8u,16u,24u})
            bytes.push_back(static_cast<std::uint8_t>((bits>>shift)&0xffu));
    }
    return bytes;
}
static native::OriginalFields fields() {
    return {1u,0u,1u,0.5f,0x1234u,0x5678u};
}
static void assert_original_entries() {
    static_assert(native::kComparisonVA==0x24198AA0u);
    static_assert(native::kDiagnosticsVA==0x24197CA0u);
    static_assert(native::kPreviewVA==0x24198470u);
}
static void test_native_three_way_status() {
    auto f=fields();
    assert(native::compare_status(f,floats({1.0f}),0,floats({1.0f}),0,1)==1u);
    assert(native::compare_status(f,floats({1.0f}),0,floats({1.25f}),0,1)==2u);
    assert(native::compare_status(f,floats({1.0f}),0,floats({1.5f}),0,1)==2u);
    assert(native::compare_status(f,floats({1.0f}),0,floats({1.75f}),0,1)==0u);
    assert(native::compare_status(f,floats({1.0f,10.0f}),0,
                                    floats({1.25f,10.5f}),0,2)==2u);
    assert(native::compare_status(f,floats({1.0f,10.0f}),0,
                                    floats({1.25f,10.75f}),0,2)==0u);
    assert(native::compare_status(f,floats({1.0f}),0,floats({2.0f}),0,0)==1u);
    assert(native::compare_status(f,{},100,{},100,-5)==1u);
    f.descriptor_tolerance_30=0.0f;
    assert(native::compare_status(f,floats({1.0f}),0,floats({1.25f}),0,1)==0u);
    f.descriptor_tolerance_30=-0.5f;
    assert(native::compare_status(f,floats({1.0f}),0,floats({1.25f}),0,1)==0u);
    f.descriptor_tolerance_30=std::numeric_limits<float>::quiet_NaN();
    assert(native::compare_status(f,floats({1.0f}),0,floats({1.25f}),0,1)==0u);
    f.descriptor_tolerance_30=std::numeric_limits<float>::infinity();
    assert(native::compare_status(f,floats({1.0f}),0,floats({2.0f}),0,1)==2u);
}
static void test_native_x87_special_values() {
    auto f=fields();
    const float nan=std::numeric_limits<float>::quiet_NaN();
    const float inf=std::numeric_limits<float>::infinity();
    assert(native::compare_status(f,floats({nan}),0,floats({nan}),0,1)==0u);
    assert(native::compare_status(f,floats({1.0f}),0,floats({nan}),0,1)==0u);
    assert(native::compare_status(f,floats({inf}),0,floats({inf}),0,1)==1u);
    assert(native::compare_status(f,floats({inf}),0,floats({-inf}),0,1)==0u);
    assert(native::compare_status(f,floats({+0.0f}),0,floats({-0.0f}),0,1)==1u);
    f.descriptor_flags_13=4u;
    assert(native::compare_status(f,{},99,{},99,100)==1u);
    f.enabled_14=0u;
    assert(native::compare_status(f,{},99,{},99,100)==0u);
}
static void test_diag_and_preview_callback_gates() {
    auto f=fields();
    Trace trace;
    native::diagnostics(f,0u,floats({2.0f}),0,floats({1.0f}),0,trace);
    assert(trace.entries.size()==2u);
    assert(trace.entries[0].kind==native::TraceKind::Difference);
    assert(trace.entries[0].net==1.0 && trace.entries[0].predicted==2.0);
    assert(trace.entries[0].predicted_minus_net==1.0);
    assert(trace.entries[1].kind==native::TraceKind::Status);
    assert(trace.entries[1].predicted==2.0);
    trace.entries.clear();
    native::diagnostics(f,2u,floats({3.0f}),0,{},999,trace);
    assert(trace.entries.size()==1u);
    assert(trace.entries[0].operation==2u);
    trace.entries.clear();
    native::preview(f,{},999,trace);
    assert(trace.entries.empty());
    f.preview_token_34=f.descriptor_token_18;
    native::preview(f,floats({4.5f}),0,trace);
    assert(trace.entries.size()==1u);
    assert(trace.entries[0].kind==native::TraceKind::Preview);
    assert(trace.entries[0].predicted==4.5);
    f.enabled_14=0u;
    native::diagnostics(f,0u,{},999,{},999,trace);
    assert(trace.entries.size()==1u);
    native::preview(f,floats({3.0f}),0,trace);
    assert(trace.entries.size()==2u);
}
static void test_exact_type1_four_call_sequence() {
    auto f=fields();
    Trace trace;
    Bytes predicted=floats({1.0f,2.0f});
    const Bytes net=floats({1.25f,2.0f});
    const auto status=native::run_direct_type1_calls(f,net,0,predicted,0,2,trace);
    assert(status==2u);
    assert(native::load_f32_le(predicted,0)==1.25f);
    assert(trace.entries.size()==1u && trace.entries[0].operation==2u);
    assert(trace.entries[0].predicted==1.25);
    trace.entries.clear();
    predicted=floats({1.0f});
    const auto different=native::run_direct_type1_calls(
        f,floats({5.0f}),0,predicted,0,1,trace);
    assert(different==0u);
    assert(native::load_f32_le(predicted,0)==5.0f);
    assert(trace.entries.size()==2u);
    // Original diagnostic occurs AFTER the Phase213 copy, not before.
    assert(trace.entries[0].predicted==5.0 && trace.entries[0].net==5.0);
    assert(trace.entries[0].predicted_minus_net==0.0);
    trace.entries.clear();
    f.copy_enabled_2c=0u;
    predicted=floats({1.0f});
    native::run_direct_type1_calls(f,floats({5.0f}),0,predicted,0,1,trace);
    assert(native::load_f32_le(predicted,0)==1.0f);
    assert(trace.entries[0].predicted_minus_net==-4.0);
    trace.entries.clear();
    f.enabled_14=0u;f.copy_enabled_2c=1u;
    predicted=floats({1.0f});
    const auto disabled=native::run_direct_type1_calls(
        f,floats({5.0f}),0,predicted,0,1,trace);
    assert(disabled==0u && native::load_f32_le(predicted,0)==5.0f);
    assert(trace.entries.empty());
}
static void test_host_only_refusals() {
    auto f=fields();
    bool threw=false;
    try {native::compare_status(f,{},0,{},0,1);}
    catch(const std::out_of_range&) {threw=true;}
    assert(threw);
    threw=false;
    Trace trace;
    Bytes dest=floats({1.0f});
    try {native::run_direct_type1_calls(f,floats({1.0f}),0,
                                       dest,0,-1,trace);}
    catch(const std::invalid_argument&) {threw=true;}
    assert(threw);
}
int main() {
    assert_original_entries();
    test_native_three_way_status();
    test_native_x87_special_values();
    test_diag_and_preview_callback_gates();
    test_exact_type1_four_call_sequence();
    test_host_only_refusals();
}
