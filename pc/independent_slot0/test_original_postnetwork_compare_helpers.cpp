// Phase1670: actual MSVC Win32 x86 original-free test for two
// SHA-authenticated retail 2006 PE32 comparison helper ABIs.
// 0x241978A0: nonvirtual 10-DWORD __thiscall, RET 0x28.
// 0x24199BA0: nonvirtual 3-DWORD __thiscall, RET 0x0C.
// No retail executable, C_BaseEntity, engine callback or DLL used.
#include "native_original_postnetwork_compare_helpers.h"
#include "native_original_postnetwork_entity_compare.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1670 original helper ABI requires native MSVC x86
#endif
namespace cmp=sin_independent::original_postnetwork_compare_helpers;
namespace entity=sin_independent::original_postnetwork_entity_compare;
namespace {
int fail(int id,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1670 original-free comparison helpers MSVC x86 "
      "#%d: %s\n",id,why);
    return id;
}
void word(std::array<std::uint8_t,0x34u>& a,unsigned off,
          std::uint32_t v) noexcept {
    std::memcpy(a.data()+off,&v,sizeof(v));
}
struct GuardedPayload {
    std::uint32_t before=0xC0FFEE12u;
    cmp::PreparePayload target{};
    std::uint32_t after=0x1234DEADu;
};
bool safe(const GuardedPayload& g) noexcept {
    return g.before==0xC0FFEE12u && g.after==0x1234DEADu;
}
struct Trace {
    std::array<char,32> calls{};
    unsigned size=0u;
    unsigned abi_bad=0u;
    std::uint32_t* counter=nullptr;
    cmp::WrapperFixture* object=nullptr;
    cmp::ComparisonInput* input=nullptr;
    std::uint32_t expected_arg1=0u,expected_arg2=0u;
    std::uint32_t expected_count=0u;
    std::uint32_t finalize_result=0xDEADBEEFu;
    void append(char c) noexcept {
        if(size<calls.size())calls[size++]=c;
    }
    void reset() noexcept {
        size=0u;calls.fill(0);abi_bad=0u;
    }
    bool matches(const char* x,unsigned n) const noexcept {
        if(size!=n)return false;
        for(unsigned i=0u;i<n;++i)
            if(calls[i]!=x[i])return false;
        return true;
    }
};
void on_preflight(void* user,cmp::ComparisonInput* in) noexcept {
    auto* t=static_cast<Trace*>(user);
    t->append('P');
    if(in!=t->input || !t->counter ||
       *t->counter!=t->expected_count ||
       in->preflight_suppressed!=0u)
        ++t->abi_bad;
}
void on_core(void* user,cmp::WrapperFixture* obj,
             std::uint32_t a1,std::uint32_t a2,
             cmp::ComparisonInput* in) noexcept {
    auto* t=static_cast<Trace*>(user);
    t->append('C');
    if(obj!=t->object || in!=t->input ||
       a1!=t->expected_arg1 || a2!=t->expected_arg2 ||
       !t->counter || *t->counter!=t->expected_count)
        ++t->abi_bad;
    // The full retail 0x24199100 is opaque, but the wrapper
    // must read this field AFTER that call and after finalizer.
    obj->set_result(0x55AA55AAu);
}
void on_finalize(void* user,cmp::WrapperFixture* obj,
                 std::uint32_t count,
                 cmp::ComparisonInput* in) noexcept {
    auto* t=static_cast<Trace*>(user);
    t->append('F');
    if(obj!=t->object || in!=t->input ||
       count!=t->expected_count || !t->counter ||
       *t->counter!=count)
        ++t->abi_bad;
    obj->set_result(t->finalize_result);
}
cmp::WrapperOps make_ops(Trace& t,std::uint32_t& counter) noexcept {
    cmp::WrapperOps cb{};
    cb.user=&t;
    cb.counter=&counter;
    cb.preflight=&on_preflight;
    cb.core=&on_core;
    cb.finalize=&on_finalize;
    return cb;
}
struct EntityChain {
    Trace* trace=nullptr;
    cmp::WrapperFixture* wrapper=nullptr;
    cmp::ComparisonInput* input=nullptr;
    unsigned tag_calls=0u;
    unsigned prepare_calls=0u;
    unsigned source_calls=0u;
};
void on_tag(void* user,void* entity_object,
            std::int32_t) noexcept {
    auto* c=static_cast<EntityChain*>(user);
    ++c->tag_calls;
    if(!entity_object)++c->trace->abi_bad;
}
void on_prepare(void* user,void* object,
                std::int32_t,std::uint32_t) noexcept {
    auto* c=static_cast<EntityChain*>(user);
    ++c->prepare_calls;
    if(!object)++c->trace->abi_bad;
}
const void* on_source(void* user,void* object) noexcept {
    auto* c=static_cast<EntityChain*>(user);
    ++c->source_calls;
    return object;
}
std::int32_t on_difference(void* user,void*,const void* source) noexcept {
    auto* c=static_cast<EntityChain*>(user);
    if(!source)++c->trace->abi_bad;
    return static_cast<std::int32_t>(
        c->wrapper->CompareAndCount(
            c->trace->expected_arg1,
            c->trace->expected_arg2,c->input));
}
} // namespace
int main() {
    cmp::events={};

    GuardedPayload guarded{};
    guarded.target.raw.fill(0xCDu);
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    guarded.target.Initialize(
        0x11223344u,0x55667788u,
        0x00000100u,0xA1B2C3D4u,
        0x00000101u,0xDEADAB34u,
        0x01234521u,0xFFEEDDFAu,
        0xAABBCC99u,0x55AA33CCu);
    __asm mov after,esp
    if(before!=after || !safe(guarded))
        return fail(1,"ten-argument __thiscall RET40 stack/canary mismatch");
    std::array<std::uint8_t,0x34u> expected{};
    expected.fill(0xCDu);
    word(expected,0x00u,0x11223344u);
    word(expected,0x04u,0x55667788u);
    word(expected,0x08u,0xA1B2C3D4u);
    word(expected,0x0Cu,0u); // a3 LOW byte=0, despite nonzero DWORD
    word(expected,0x10u,1u); // a5 LOW byte=1
    expected[0x14u]=0x34u;
    expected[0x15u]=0x21u;
    expected[0x16u]=0x99u;
    word(expected,0x18u,0u);
    word(expected,0x1Cu,0u);
    word(expected,0x20u,0u);
    expected[0x24u]=0u;expected[0x25u]=0u;
    word(expected,0x28u,0u);
    expected[0x2Cu]=0xFAu;
    word(expected,0x30u,0x55AA33CCu);
    if(guarded.target.raw!=expected ||
       guarded.target.word(0x0Cu)!=0u ||
       guarded.target.word(0x10u)!=1u)
        return fail(2,"retail 0x34-byte initializer field/padding semantics");
    // Run different inputs into the same owned buffer. Ensure all
    // absolute byte/word writes happen but non-written padding is
    // STILL untouched, never memset-ed by an imagined default ctor.
    __asm mov before,esp
    guarded.target.Initialize(0u,0u,0xFFu,0u,0x100u,
                              0u,0u,0u,0u,0u);
    __asm mov after,esp
    if(before!=after || guarded.target.word(0x0Cu)!=1u ||
       guarded.target.word(0x10u)!=0u ||
       guarded.target.raw[0x17u]!=0xCDu ||
       guarded.target.raw[0x26u]!=0xCDu ||
       guarded.target.raw[0x2Du]!=0xCDu ||
       guarded.target.word(0x30u)!=0u || !safe(guarded))
        return fail(3,"reuse/flag low byte and untouched padding differ");

    // Original 71-byte wrapper: increment global count, optionally
    // preflight if input+0x10==0, core(arg1,arg2,input),
    // finalize(updatedCount,input), return resulting this+0x28.
    cmp::WrapperFixture wrapper{};
    const auto base=reinterpret_cast<std::uintptr_t>(&wrapper);
    if(wrapper.result_address()-base!=cmp::kWrapperResultOffset)
        return fail(4,"original wrapper result DWORD +0x28 layout");
    cmp::ComparisonInput input{};
    cmp::ComparisonInput alternative{};
    std::uint32_t global_count=0u;
    Trace trace{};
    trace.counter=&global_count;trace.object=&wrapper;
    trace.input=&input;
    trace.expected_arg1=0xAA11BB22u;
    trace.expected_arg2=0xCC33DD44u;
    trace.expected_count=1u;
    trace.finalize_result=0xFEDCBA98u;
    wrapper.set_ops(make_ops(trace,global_count));
    std::uint32_t result=0u;
    __asm mov before,esp
    result=wrapper.CompareAndCount(
        trace.expected_arg1,trace.expected_arg2,&input);
    __asm mov after,esp
    if(before!=after || !trace.matches("PCF",3u) ||
       trace.abi_bad || global_count!=1u ||
       result!=trace.finalize_result ||
       wrapper.result_dword!=trace.finalize_result ||
       cmp::events.wrapper_calls!=1u ||
       cmp::events.conditional_preflights!=1u)
        return fail(5,"native 3-argument RET12, preflight/core/final ordering");

    // When input+0x10 nonzero, PRE-FLIGHT MUST NOT RUN. The
    // counter still increments BEFORE the opaque core/final.
    alternative.preflight_suppressed=0xFFu;
    trace.input=&alternative;
    trace.expected_arg1=0x11111111u;
    trace.expected_arg2=0x22222222u;
    trace.expected_count=2u;
    trace.finalize_result=0u;
    trace.reset();
    __asm mov before,esp
    result=wrapper.CompareAndCount(
        trace.expected_arg1,trace.expected_arg2,&alternative);
    __asm mov after,esp
    if(before!=after || !trace.matches("CF",2u) ||
       trace.abi_bad || global_count!=2u || result!=0u ||
       cmp::events.conditional_preflights!=1u ||
       cmp::events.wrapper_calls!=2u)
        return fail(6,"suppressed preflight still called or arg order wrong");

    // A native DWORD increment wraps 0xFFFFFFFF -> 0.
    global_count=0xFFFFFFFFu;
    trace.expected_count=0u;
    trace.finalize_result=0x80000001u;
    trace.reset();
    __asm mov before,esp
    result=wrapper.CompareAndCount(
        trace.expected_arg1,trace.expected_arg2,&alternative);
    __asm mov after,esp
    if(before!=after || global_count!=0u ||
       result!=0x80000001u || !trace.matches("CF",2u) ||
       trace.abi_bad)
        return fail(7,"global DWORD overflow or raw signed result changed");

    // Host-only refusal: original dereferences null pointers.
    const auto count_before=global_count;
    const auto prev_refusals=cmp::events.safe_refusals;
    trace.reset();
    result=wrapper.CompareAndCount(1u,2u,nullptr);
    if(result!=wrapper.result_dword ||
       global_count!=count_before || trace.size!=0u ||
       cmp::events.safe_refusals!=prev_refusals+1u)
        return fail(8,"null invented input not refused before side effects");
    cmp::WrapperOps missing=make_ops(trace,global_count);
    missing.core=nullptr;
    wrapper.set_ops(missing);
    result=wrapper.CompareAndCount(1u,2u,&alternative);
    if(result!=wrapper.result_dword ||
       global_count!=count_before ||
       cmp::events.safe_refusals!=prev_refusals+2u)
        return fail(9,"missing opaque core should not change sequence");

    // Compose with the Phase1669 bounded ORIGINAL-FREE entity result
    // contract to ensure signed comparison counts map to AL precisely.
    wrapper.set_ops(make_ops(trace,global_count));
    std::array<std::uint32_t,90> history{};
    history[0]=0x01020304u;
    EntityChain chain{};
    chain.trace=&trace;chain.wrapper=&wrapper;
    chain.input=&alternative;
    entity::Ops entity_ops{};
    entity_ops.user=&chain;
    entity_ops.history=&history;
    entity_ops.history_active=true;
    entity_ops.on_tag=&on_tag;
    entity_ops.on_prepare=&on_prepare;
    entity_ops.get_virtual16=&on_source;
    entity_ops.difference_count=&on_difference;
    trace.expected_arg1=0x100u;
    trace.expected_arg2=0x200u;
    trace.expected_count=global_count+1u;
    trace.finalize_result=5u;
    trace.input=&alternative;
    trace.reset();
    const auto old_global=global_count;
    const auto positive=entity::Run(&history,1,entity_ops);
    if(positive.status!=entity::Status::Complete ||
       positive.return_al!=1u || positive.history_word!=0x01020304u ||
       positive.observed_difference_count!=5 ||
       global_count!=old_global+1u ||
       chain.tag_calls!=1u ||chain.prepare_calls!=1u ||
       chain.source_calls!=1u || !trace.matches("CF",2u) ||
       trace.abi_bad)
        return fail(10,"bounded entity history -> wrapper -> positive AL1");
    trace.expected_count=global_count+1u;
    trace.finalize_result=0u;
    trace.reset();
    const auto zero=entity::Run(&history,91,entity_ops);
    if(zero.status!=entity::Status::Complete ||
       zero.return_al!=0u || zero.history_index!=0u ||
       zero.observed_difference_count!=0 ||
       global_count!=trace.expected_count ||
       !trace.matches("CF",2u) || trace.abi_bad)
        return fail(11,"zero wrapper result must map to entity AL0");
    std::puts("PASS Phase1670 original-free native MSVC x86: "
              "104-byte nonvirtual ten-arg __thiscall RET40 fills exact "
              "0x34 bytes, low-byte flags and untouched padding, "
              "71-byte three-arg __thiscall RET12 counter wrapper "
              "global++/conditional preflight/core/final/this+0x28 "
              "result, x86 ESP checks, overflow/null/backend safety, "
              "and nested bounded entity PostNetwork AL1/AL0 with "
              "90-slot history; zero retail native method calls");
    return 0;
}
