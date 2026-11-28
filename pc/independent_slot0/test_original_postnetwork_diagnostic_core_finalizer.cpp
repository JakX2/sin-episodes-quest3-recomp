// Phase1671: genuine native MSVC x86 original-free three-argument
// C++ __thiscall diagnostic core RET12 and two-argument linked-list
// finalizer RET8, both verified from unchanged original client.dll.
//
// No original DLL, runtime Source engine state, proprietary native
// method or opaque formatting/per-entry helper is executed.
#include "native_original_postnetwork_diagnostic_core_finalizer.h"
#include "native_original_postnetwork_compare_helpers.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1671 requires real Win32 MSVC x86 native ABI tests
#endif
namespace diag=sin_independent::original_postnetwork_diagnostic_core_finalizer;
namespace cmp=sin_independent::original_postnetwork_compare_helpers;
namespace {
int fail(int id,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1671 original-free diagnostic/final native x86 "
      "#%d: %s\n",id,why);
    return id;
}
struct Trace {
    std::array<char,64> events{};
    std::array<std::uint32_t,32> node_sequences{};
    std::array<std::uint32_t,32> node_words0{};
    std::array<std::uint32_t,32> node_words4{};
    unsigned size=0u;
    unsigned formatted=0u;
    unsigned processed=0u;
    unsigned bad=0u;
    const char* expected_label=nullptr;
    void* expected_context=nullptr;
    std::uint32_t formatter_return=0u;
    std::uint32_t expected_sequence=0u;
    diag::ListNode* mutate_link_in_node=nullptr;
    diag::ListNode* mutate_link_to=nullptr;
    diag::ListFinalizerFixture* finalizer=nullptr;
    diag::ListNode* first=nullptr;
    cmp::WrapperFixture* wrapper=nullptr;
    void emit(char c) noexcept {
        if(size<events.size())events[size++]=c;
    }
    void reset() noexcept {
        size=0u; events.fill(0);
        formatted=0u; processed=0u; bad=0u;
        node_sequences.fill(0u);
        node_words0.fill(0u);node_words4.fill(0u);
    }
    bool matches(const char* x,unsigned n) const noexcept {
        if(size!=n)return false;
        for(unsigned i=0u;i<n;++i)
            if(events[i]!=x[i])return false;
        return true;
    }
};
std::uint32_t format_opaque(void* context,
                            const char* label,void* input) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->emit('F');++t->formatted;
    if(label!=t->expected_label || input!=t->expected_context)
        ++t->bad;
    return t->formatter_return;
}
void apply_node(void* context,std::uint32_t sequence,
                diag::ListNode* node,std::uint32_t w0,
                std::uint32_t w4) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->emit(static_cast<char>('0'+(w0&7u)));
    if(t->processed<t->node_sequences.size()) {
        t->node_sequences[t->processed]=sequence;
        t->node_words0[t->processed]=w0;
        t->node_words4[t->processed]=w4;
    }
    ++t->processed;
    if(sequence!=t->expected_sequence ||
       !node || node->word0!=w0 || node->word4!=w4)
        ++t->bad;
    // The real method re-reads node+0xC AFTER the callback.
    // Mutate the link from inside an invented opaque helper and
    // verify the replacement next pointer is observed.
    if(node==t->mutate_link_in_node)
        node->next=t->mutate_link_to;
}
void wrapper_preflight(void* context,cmp::ComparisonInput*) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->emit('P');
}
void wrapper_core(void* context,cmp::WrapperFixture* owner,
                  std::uint32_t,std::uint32_t,
                  cmp::ComparisonInput*) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->emit('C');
    if(owner!=t->wrapper)++t->bad;
    owner->set_result(0x1234u);
}
void wrapper_finalize(void* context,cmp::WrapperFixture* owner,
                      std::uint32_t sequence,
                      cmp::ComparisonInput*) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->emit('L');
    if(owner!=t->wrapper || !t->finalizer || !t->first ||
       sequence!=t->expected_sequence)++t->bad;
    if(t->finalizer && t->first)
        t->finalizer->Finalize(sequence,t->first);
    owner->set_result(0xFFFF0001u); // read AFTER finalizer, raw DWORD
}
} // namespace
int main() {
    diag::core_events={};
    diag::final_events={};
    cmp::events={};

    diag::DiagnosticCoreFixture core{};
    const auto core_base=reinterpret_cast<std::uintptr_t>(&core);
    if(core.format_result_address()-core_base!=diag::kCoreOutputOffset ||
       core.input_pointer_address()-core_base!=diag::kCoreInputOffset)
        return fail(1,"original core this+0x34/+0x38 native layout differs");
    Trace t{};
    int owned_context=7;
    t.expected_context=&owned_context;
    static const char candidate[]="entity";
    static const char configured[]="configured";
    static const char flagged[]="flagged";
    static const char blank[]="";
    diag::DiagnosticGateOps gate{};
    gate.user=&t;
    gate.expected_index=7;
    gate.debug_flags=0;
    gate.configured_label=configured;
    gate.flagged_label=flagged;
    gate.format=&format_opaque;
    t.expected_label=configured;
    t.formatter_return=0xCAFED00Du;
    core.set_ops(gate);
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    core.BuildDiagnostic(candidate,7u,&owned_context);
    __asm mov after,esp
    if(before!=after || core.format_result!=0xCAFED00Du ||
       core.input_pointer!=static_cast<std::uint32_t>(
           reinterpret_cast<std::uintptr_t>(candidate)) ||
       !t.matches("F",1u) || t.formatted!=1u || t.bad)
        return fail(2,"native three-arg RET12 core configured-label path");

    // Store first pointer even when later guard fails, while
    // always clearing this+0x34 before diagnostics.
    t.reset();core.format_result=0xFFFFFFFFu;
    __asm mov before,esp
    core.BuildDiagnostic(nullptr,7u,&owned_context);
    __asm mov after,esp
    if(before!=after || core.format_result!=0u ||
       core.input_pointer!=0u || t.size)
        return fail(3,"null first pointer should clear/write fields and skip");
    t.reset();
    core.BuildDiagnostic(blank,7u,&owned_context);
    if(core.format_result!=0u || t.size)
        return fail(4,"empty first string should skip opaque formatter");
    t.reset();
    core.BuildDiagnostic(candidate,8u,&owned_context);
    if(core.format_result!=0u || t.size)
        return fail(5,"mismatched signed debug ConVar index still formatted");
    gate.expected_index=-1;
    core.set_ops(gate);t.reset();
    core.BuildDiagnostic(candidate,0xFFFFFFFFu,&owned_context);
    if(core.format_result!=0u || t.size)
        return fail(6,"negative debug ConVar should disable filter");
    gate.expected_index=7;
    gate.configured_label=blank;
    core.set_ops(gate);t.reset();
    core.BuildDiagnostic(candidate,7u,&owned_context);
    if(core.format_result!=0u || t.size)
        return fail(7,"empty unflagged global label should disable format");
    // 0x1000 mode uses the original hard-coded text branch.
    gate.debug_flags=0x1000u;
    t.expected_label=flagged;
    t.formatter_return=0xDEADC0DEu;
    core.set_ops(gate);t.reset();
    __asm mov before,esp
    core.BuildDiagnostic(candidate,7u,&owned_context);
    __asm mov after,esp
    if(before!=after || core.format_result!=0xDEADC0DEu ||
       !t.matches("F",1u) || t.bad)
        return fail(8,"0x1000 direct-label path or RET12 differs");
    gate.format=nullptr;
    core.set_ops(gate);t.reset();
    const unsigned prev_refusals=diag::core_events.safe_refusals;
    core.BuildDiagnostic(candidate,7u,&owned_context);
    if(core.format_result!=0u || t.size ||
       diag::core_events.safe_refusals!=prev_refusals+1u)
        return fail(9,"missing original formatting backend not safely refused");

    // Finalizer: process FIRST head and then each +0xC next pointer.
    // Its opaque per-entry helper consumes sequence, pointer,
    // entry+0 DWORD and entry+4 DWORD in source push order.
    diag::ListNode n1{1u,0xA1B2C3D4u,0u,nullptr};
    diag::ListNode n2{2u,0xB1C2D3E4u,0u,nullptr};
    diag::ListNode n3{3u,0xC1D2E3F4u,0u,nullptr};
    n1.next=&n2;n2.next=&n3;
    diag::ListFinalizerFixture list{};
    diag::FinalizerOps fops{};
    fops.user=&t;
    fops.apply_entry=&apply_node;
    list.set_ops(fops);
    t.expected_sequence=0x01020304u;
    t.reset();
    __asm mov before,esp
    list.Finalize(t.expected_sequence,&n1);
    __asm mov after,esp
    if(before!=after || !t.matches("123",3u) ||
       t.processed!=3u || t.bad ||
       t.node_sequences[0]!=t.expected_sequence ||
       t.node_words0[1]!=n2.word0 ||
       t.node_words4[2]!=n3.word4 ||
       diag::final_events.processed_entries!=3u)
        return fail(10,"native two-arg RET8 forward linked-list per-node order");

    // Original code reads next AFTER the opaque callback. Mutating
    // n1.next within that callback must skip n2 and visit n3.
    t.reset();
    t.mutate_link_in_node=&n1;
    t.mutate_link_to=&n3;
    n1.next=&n2;
    __asm mov before,esp
    list.Finalize(t.expected_sequence,&n1);
    __asm mov after,esp
    if(before!=after || !t.matches("13",2u) ||
       t.processed!=2u || n1.next!=&n3 || t.bad)
        return fail(11,"original node+0xC reload AFTER callback differs");
    t.mutate_link_in_node=nullptr;
    t.mutate_link_to=nullptr;
    n1.next=&n2;

    t.reset();
    const auto missing_before=diag::final_events.null_head_refusals;
    list.Finalize(1u,nullptr);
    if(t.size || diag::final_events.null_head_refusals!=missing_before+1u)
        return fail(12,"null original node should be refused by host-only fixture");
    fops.apply_entry=nullptr;list.set_ops(fops);
    t.reset();
    const auto backend_before=diag::final_events.missing_backend_refusals;
    list.Finalize(1u,&n1);
    if(t.size ||
       diag::final_events.missing_backend_refusals!=backend_before+1u)
        return fail(13,"missing opaque per-entry helper must fail closed");
    fops.apply_entry=&apply_node;list.set_ops(fops);

    // Integrate with Phase1670 wrapper, which invokes opaque finalize
    // AFTER original core and then reloads this+0x28 for its result.
    cmp::WrapperFixture wrapper{};
    cmp::ComparisonInput wrapper_input{};
    std::uint32_t global_sequence=0x3Fu;
    cmp::WrapperOps wrapper_ops{};
    wrapper_ops.user=&t;
    wrapper_ops.counter=&global_sequence;
    wrapper_ops.preflight=&wrapper_preflight;
    wrapper_ops.core=&wrapper_core;
    wrapper_ops.finalize=&wrapper_finalize;
    wrapper.set_ops(wrapper_ops);
    t.wrapper=&wrapper;
    t.finalizer=&list;
    t.first=&n1;
    t.expected_sequence=0x40u;
    t.reset();
    __asm mov before,esp
    const auto output=wrapper.CompareAndCount(0x11u,0x22u,&wrapper_input);
    __asm mov after,esp
    if(before!=after || output!=0xFFFF0001u ||
       wrapper.result_dword!=output ||
       global_sequence!=0x40u ||
       !t.matches("PCL123",6u) ||
       t.processed!=3u || t.bad)
        return fail(14,"wrapper RET12 -> linked finalizer RET8 chain/result");
    std::puts("PASS Phase1671 original-free native MSVC x86: "
              "139-byte diagnostic gate three-arg RET12 +0x34/+0x38 "
              "state, ConVar-index/debug 0x1000 label dispatch, "
              "65-byte finalizer two-arg RET8, first-node unconditional "
              "then node+0xC POST-callback reload, per-entry "
              "sequence/word0/word4, missing callback/null safety, "
              "nested original-free Phase1670 wrapper count/core/final "
              "and final EAX result, zero retail methods");
    return 0;
}
