#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include "sin_compat/original_client_snapshot_ehandle.hpp"

namespace native=sin_compat::original_client_snapshot_ehandle;
using Bytes=native::Bytes;
using Slot=native::OriginalEntitySlot;

struct Registry final:native::IEntityListHost {
    std::map<std::uint32_t,Slot> entries{
        {1u,{0x11110000u,1u}},
        {2u,{0x11110000u,1u}}, // Distinct handle, SAME resolved entity pointer.
        {3u,{0x22220000u,2u}},
        {4u,{0x33330000u,1u}},
        {5u,{0u,1u}},         // Valid serial but null entity pointer.
    };
    bool empty_first_name{false};
    int slot_calls{};
    int name_calls{};
    int fallback_calls{};
    std::optional<Slot> slot(std::uint32_t index) const override {
        auto it=entries.find(index);
        if(it==entries.end())return std::nullopt;
        return it->second;
    }
    std::string entity_name(std::uint32_t entity_pointer) override {
        ++name_calls;
        if(empty_first_name)return {};
        if(entity_pointer==0x11110000u)return "npc_test";
        if(entity_pointer==0x22220000u)return "npc_other";
        return "npc_unknown";
    }
    std::string fallback_entity_name(std::uint32_t entity_pointer) override {
        ++fallback_calls;
        assert(entity_pointer!=0u);
        return "fallback_entity";
    }
};
struct Sink final:native::ITraceSink {
    std::vector<native::TraceEvent> events;
    void on_event(const native::TraceEvent& event) override {
        events.push_back(event);
    }
};
static Bytes handles(std::initializer_list<std::uint32_t> hs) {
    Bytes result;
    for(const auto value:hs)
        for(const unsigned shift:{0u,8u,16u,24u})
            result.push_back(static_cast<std::uint8_t>((value>>shift)&0xffu));
    return result;
}
static native::OriginalFields fields() {
    return {1u,0u,1u,0xABCDu,0xDCBAu};
}
static void original_addresses_and_slot_layout() {
    static_assert(native::kCompareVA==0x24199060u);
    static_assert(native::kCopyVA==0x24197E90u);
    static_assert(native::kDiagnosticsVA==0x241994C0u);
    static_assert(native::kPreviewVA==0x241995E0u);
    static_assert(native::kCompareCall==0x24199A93u);
    static_assert(native::kCopyCall==0x24199AA2u);
    static_assert(native::kDiagnosticsCall==0x24199AB1u);
    static_assert(native::kPreviewCall==0x24199AC0u);
    static_assert(native::kOriginalEntityListGlobal==0x242FA734u);
    static_assert(native::kEntryIndexMask==0xFFFu);
    static_assert(native::kSerialShift==12u);
    static_assert(native::kEntryStrideBytes==16u);
    static_assert(native::kEntryBaseOffset==4u);
}
static void handle_resolution_uses_slot_and_serial() {
    Registry r;
    assert(native::resolve_entity(0x1001u,r)==0x11110000u);
    assert(native::resolve_entity(0x1002u,r)==0x11110000u);
    assert(native::resolve_entity(0x2003u,r)==0x22220000u);
    assert(native::resolve_entity(0x3001u,r)==0u);  // Stale serial.
    assert(native::resolve_entity(0x1006u,r)==0u);  // Missing slot.
    assert(native::resolve_entity(0x1005u,r)==0u);  // Null pointer in valid slot.
    assert(native::resolve_entity(0xFFFFFFFFu,r)==0u); // Invalid sentinel.
    assert(native::load_u32(handles({0x12345678u}),0u)==0x12345678u);
}
static void comparator_checks_entities_not_handle_bits() {
    auto f=fields();Registry r;
    // Two different raw handles resolve the SAME original entity pointer.
    assert(native::compare_resolved_entities(
        f,handles({0x1001u}),0,handles({0x1002u}),0,1,r)==1u);
    // Both stale/missing handle values resolve NULL, so compare equal.
    assert(native::compare_resolved_entities(
        f,handles({0x3001u}),0,handles({0xFFFFFFFFu}),0,1,r)==1u);
    assert(native::compare_resolved_entities(
        f,handles({0x1001u}),0,handles({0x2003u}),0,1,r)==0u);
    assert(native::compare_resolved_entities(
        f,handles({0x1001u,0x2003u}),0,
          handles({0x1002u,0x1004u}),0,2,r)==0u);
    // Early first-pointer mismatch: native never dereferences later entries.
    assert(native::compare_resolved_entities(
        f,handles({0x1001u}),0,handles({0x2003u}),0,999,r)==0u);
    assert(native::compare_resolved_entities(f,{},99,{},99,0,r)==1u);
    assert(native::compare_resolved_entities(f,{},99,{},99,-1,r)==1u);
    f.descriptor_flags_13=4u;
    assert(native::compare_resolved_entities(f,{},99,{},99,10,r)==1u);
    f.enabled_14=0u;
    assert(native::compare_resolved_entities(f,{},99,{},99,10,r)==0u);
    f.enabled_14=1u;f.descriptor_flags_13=0u;
    bool threw=false;
    try {native::compare_resolved_entities(
        f,handles({0x1001u}),0,handles({0x1001u}),0,2,r);}
    catch(const std::out_of_range&){threw=true;}
    assert(threw);
}
static void copy_gate_signed_count_and_forward_overlap() {
    auto f=fields(); Bytes pred=handles({0u,0u,0u});
    const Bytes source=handles({0x1001u,0x2003u});
    native::copy_ehandles(f,0u,source,0u,pred,0u,2);
    assert(native::load_u32(pred,0)==0x1001u);
    assert(native::load_u32(pred,4)==0x2003u);
    const auto old=pred;
    native::copy_ehandles(f,1u,{},999,pred,999,100);
    assert(pred==old);
    f.copy_enabled_2c=0u;
    native::copy_ehandles(f,0u,{},999,pred,999,100);
    assert(pred==old);
    f.copy_enabled_2c=1u;
    native::copy_ehandles(f,0u,{},999,pred,999,-1);
    assert(pred==old);
    native::copy_ehandles(f,0u,{},999,pred,999,0);
    assert(pred==old);
    bool refused=false;
    try {native::copy_ehandles(f,0u,source,0u,pred,0u,0x40000000);}
    catch(const std::overflow_error&){refused=true;}
    assert(refused);
    Bytes overlapping=handles({11u,22u,33u});
    native::copy_ehandles(f,0u,overlapping,0u,overlapping,4u,2);
    // Original forward DWORD copy cascades the first written DWORD.
    assert(native::load_u32(overlapping,0)==11u);
    assert(native::load_u32(overlapping,4)==11u);
    assert(native::load_u32(overlapping,8)==11u);
}
static void diagnostics_and_preview_name_fallback() {
    auto f=fields();Registry registry;Sink sink;
    native::diagnostics(f,0u,handles({0x1001u}),0,
                        handles({0x2003u}),0,registry,sink);
    assert(sink.events.size()==2u);
    assert(sink.events[0].kind==native::TraceKind::Difference);
    assert(sink.events[0].net_pointer==0x22220000u);
    assert(sink.events[0].predicted_pointer==0x11110000u);
    assert(sink.events[1].kind==native::TraceKind::DiagnosticEntity);
    assert(sink.events[1].entity_name=="npc_test");
    assert(registry.fallback_calls==0);
    sink.events.clear();
    registry.empty_first_name=true;
    native::diagnostics(f,1u,handles({0x1001u}),0,
                        {},999,registry,sink);
    assert(sink.events.size()==1u);
    assert(sink.events[0].entity_name=="fallback_entity");
    assert(registry.fallback_calls==1);
    sink.events.clear();
    native::diagnostics(f,1u,handles({0xFFFFFFFFu}),0,
                        {},999,registry,sink);
    assert(sink.events.size()==1u);
    assert(sink.events[0].kind==native::TraceKind::DiagnosticNull);
    sink.events.clear();
    f.enabled_14=0u;
    native::diagnostics(f,0u,{},999,{},999,registry,sink);
    assert(sink.events.empty());
    native::preview(f,{},999,registry,sink);
    assert(sink.events.empty());
    f.preview_token_34=f.descriptor_token_18;
    native::preview(f,handles({0xFFFFFFFFu}),0,registry,sink);
    assert(sink.events.size()==1u);
    assert(sink.events[0].kind==native::TraceKind::PreviewNull);
    native::preview(f,handles({0x1001u}),0,registry,sink);
    assert(sink.events.size()==2u);
    assert(sink.events[1].kind==native::TraceKind::PreviewEntity);
    assert(sink.events[1].entity_name=="fallback_entity");
}
static void all_four_original_type13_direct_calls() {
    auto f=fields();Registry registry;Sink sink;
    Bytes predicted=handles({0x2003u}); // Other native entity.
    const Bytes net=handles({0x1001u});
    const auto mismatch=native::run_type13_direct_calls(
        f,net,0,predicted,0,1,registry,sink);
    assert(mismatch==0u && native::load_u32(predicted,0)==0x1001u);
    assert(sink.events.size()==2u);
    // Diagnostics come AFTER copy: now net/pred resolve SAME entity.
    assert(sink.events[0].kind==native::TraceKind::Difference);
    assert(sink.events[0].net_pointer==0x11110000u);
    assert(sink.events[0].predicted_pointer==0x11110000u);
    assert(sink.events[1].kind==native::TraceKind::DiagnosticEntity);
    sink.events.clear();
    const auto exact=native::run_type13_direct_calls(
        f,net,0,predicted,0,1,registry,sink);
    assert(exact==1u && sink.events.size()==1u);
    sink.events.clear();
    f.copy_enabled_2c=0u;
    predicted=handles({0x2003u});
    const auto no_copy=native::run_type13_direct_calls(
        f,net,0,predicted,0,1,registry,sink);
    assert(no_copy==0u && native::load_u32(predicted,0)==0x2003u);
    assert(sink.events[0].predicted_pointer==0x22220000u);
    assert(sink.events[0].net_pointer==0x11110000u);
    sink.events.clear();
    f.copy_enabled_2c=1u;f.enabled_14=0u;
    f.preview_token_34=f.descriptor_token_18;
    const auto disabled=native::run_type13_direct_calls(
        f,net,0,predicted,0,1,registry,sink);
    assert(disabled==0u && native::load_u32(predicted,0)==0x1001u);
    assert(sink.events.size()==1u);
    assert(sink.events[0].kind==native::TraceKind::PreviewEntity);
    sink.events.clear();
    f.enabled_14=1u;
    const auto empty=native::run_type13_direct_calls(
        f,net,0,predicted,0,-1,registry,sink);
    assert(empty==1u && sink.events.size()==2u);
    assert(sink.events[0].kind==native::TraceKind::DiagnosticEntity);
    assert(sink.events[1].kind==native::TraceKind::PreviewEntity);
}
int main() {
    original_addresses_and_slot_layout();
    handle_resolution_uses_slot_and_serial();
    comparator_checks_entities_not_handle_bits();
    copy_gate_signed_count_and_forward_overlap();
    diagnostics_and_preview_name_fallback();
    all_four_original_type13_direct_calls();
}
