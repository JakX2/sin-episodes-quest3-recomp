// Phase1664: original-free native Win32 x86 CPrediction::Update
// virtual slot3 -> opaque slot22, preserving 11-DWORD global work buffer.
//
// Entirely invented owner data and callbacks. No original DLL, runtime
// original CMoveData, Source engine command, or virtual method is called.
#include "native_client_prediction23_init_probe.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1664 requires MSVC x86 native prediction Update ABI
#endif
namespace p=sin_independent::client_prediction23_diagnostic;
namespace {
int fail(int id,const char* msg) {
    std::fprintf(stderr,
      "FAIL Phase1664 original-free CPrediction Update slot3 x86 #%d: %s\n",
      id,msg);
    return id;
}
using Buffer=std::array<std::uint32_t,11>;
bool invoke_four(void* object,void* fn,
                 std::uint32_t a1,std::uint32_t a2,
                 std::uint32_t a3,std::uint32_t a4) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm {
        mov before,esp
        push a4
        push a3
        push a2
        push a1
        mov ecx,object
        mov eax,fn
        call eax
        mov after,esp
    }
    return before==after;
}
struct Trace {
    p::UpdateFrameContext* ctx=nullptr;
    p::Prediction23Fixture* fixture=nullptr;
    Buffer* original=nullptr;
    Buffer* replacement=nullptr;
    Buffer expected{};
    std::array<char,64> sequence{};
    std::array<std::uint32_t,4> received_args{};
    unsigned count=0u;
    unsigned bad=0u;
    unsigned inner_calls=0u;
    unsigned enter_calls=0u;
    unsigned exit_calls=0u;
    bool retarget=false;
    bool drop_target=false;
    std::int32_t expected_prior_index=-1;
    std::uint32_t expected_new_index=0u;
    void emit(char code) noexcept {
        if(count<sequence.size())sequence[count++]=code;
    }
    bool matches(const char* x,unsigned n) const noexcept {
        if(count!=n)return false;
        for(unsigned i=0u;i<n;++i)
            if(sequence[i]!=x[i])return false;
        return true;
    }
    void reset_events() noexcept {
        count=0u;
        sequence.fill(0);
    }
};
void on_enter(void* obj) noexcept {
    auto* t=static_cast<Trace*>(obj);
    t->emit('E');
    ++t->enter_calls;
    // The original enters the profile BEFORE writing this+0x24.
    if(t->fixture->highest()!=t->expected_prior_index)
        ++t->bad;
}
void on_inner(void* obj,std::uint32_t work0,
              std::uint32_t a2,std::uint32_t a3,
              std::uint32_t a4) noexcept {
    auto* t=static_cast<Trace*>(obj);
    t->emit('C');
    ++t->inner_calls;
    t->received_args={{work0,a2,a3,a4}};
    if(static_cast<std::uint32_t>(t->fixture->highest())!=
       t->expected_new_index)
        ++t->bad;
    if(!t->ctx || !t->ctx->scratch ||
       *t->ctx->scratch!=t->expected)
        ++t->bad;
    if(t->ctx && t->ctx->scratch) {
        for(unsigned i=0u;i<11u;++i)
            (*t->ctx->scratch)[i]^=(0x7F000000u+i);
    }
    if(t->retarget) {
        if(!t->replacement)
            ++t->bad;
        else
            t->ctx->scratch=t->replacement;
    }
    if(t->drop_target)t->ctx->scratch=nullptr;
}
void on_exit(void* obj) noexcept {
    auto* t=static_cast<Trace*>(obj);
    t->emit('X');
    ++t->exit_calls;
    if(t->ctx && t->ctx->scratch &&
       *t->ctx->scratch!=t->expected)
        ++t->bad;
}
Buffer invented(std::uint32_t salt) noexcept {
    Buffer b{};
    for(unsigned i=0u;i<11u;++i)
        b[i]=salt+(0x11111111u*i);
    return b;
}
} // namespace
int main() {
    p::events={};
    auto prediction=std::make_unique<p::Prediction23Fixture>(nullptr);
    auto* instance=prediction.get();
    auto** table=*reinterpret_cast<void***>(instance);
    if(!table[3] || !table[22] || table[3]==table[22])
        return fail(1,"original Update and opaque slot22 entry missing");
    if(reinterpret_cast<std::uintptr_t>(instance->highest_address())-
       reinterpret_cast<std::uintptr_t>(instance)!=
       p::kOriginalUpdateLastIndexOffset)
        return fail(2,"prediction original previous-frame field +0x24 drift");

    Buffer scratch=invented(0x10101010u);
    Buffer other=invented(0xA1A1A1A1u);
    p::UpdateFrameContext ctx{};
    Trace trace{};
    ctx.scratch=&scratch;
    ctx.update_gate_convar=1;
    ctx.cl_predict_convar=1;
    ctx.user=&trace;
    ctx.on_scope_enter=&on_enter;
    ctx.on_scope_exit=&on_exit;
    ctx.on_inner_slot22=&on_inner;
    trace.ctx=&ctx;
    trace.fixture=instance;
    trace.original=&scratch;
    trace.replacement=&other;
    trace.expected=scratch;
    trace.expected_prior_index=-1;
    trace.expected_new_index=17u;
    instance->set_update_frame_context(&ctx);
    if(!invoke_four(instance,table[3],17u,0x21u,0x31u,0x41u) ||
       instance->highest()!=17 ||
       !instance->last_update_gate_flag_for_test_only() ||
       !trace.matches("ECX",3u) || trace.bad!=0u ||
       trace.inner_calls!=1u ||
       trace.received_args!=std::array<std::uint32_t,4>{
           {trace.expected[0],0x21u,0x31u,0x41u}} ||
       scratch!=trace.expected ||
       p::events.opaque_update_inner_calls!=1u)
        return fail(3,"native slot3/22 ECX ESP RET16 / 11-DWORD restore");

    // Same frame with both ConVars nonzero sets original local
    // update gate byte to 0; crucially still calls virtual slot22.
    trace.reset_events();
    trace.expected_prior_index=17;
    trace.expected_new_index=17u;
    if(!invoke_four(instance,table[3],17u,0x22u,0x32u,0x42u) ||
       instance->last_update_gate_flag_for_test_only() ||
       !trace.matches("ECX",3u) || trace.bad!=0u ||
       scratch!=trace.expected || trace.inner_calls!=2u)
        return fail(4,"same-index both-cvars original gate boolean drift");

    // Disabling either original ConVar sets the local gate byte true
    // even if the frame index did not advance.
    ctx.update_gate_convar=0;
    trace.reset_events();
    if(!invoke_four(instance,table[3],17u,1u,2u,3u) ||
       !instance->last_update_gate_flag_for_test_only() ||
       !trace.matches("ECX",3u) || trace.bad!=0u ||
       scratch!=trace.expected || trace.inner_calls!=3u)
        return fail(5,"update-gate ConVar did not override same-frame flag");
    ctx.update_gate_convar=1;
    ctx.cl_predict_convar=0;
    trace.reset_events();
    if(!invoke_four(instance,table[3],17u,2u,3u,4u) ||
       !instance->last_update_gate_flag_for_test_only() ||
       !trace.matches("ECX",3u) || trace.bad!=0u ||
       scratch!=trace.expected || trace.inner_calls!=4u)
        return fail(6,"cl_predict ConVar did not override same-frame flag");
    ctx.cl_predict_convar=1;

    // Original code loads *g_pMoveData again AFTER slot22. If the
    // opaque callback retargets the pointer, restore targets the
    // NEW array; old scratch remains modified. Model deliberately.
    scratch=invented(0x20202020u);
    other=invented(0x30303030u);
    trace.expected=scratch;
    trace.expected_prior_index=17;
    trace.expected_new_index=18u;
    trace.retarget=true;
    trace.reset_events();
    const Buffer saved_before_change=scratch;
    if(!invoke_four(instance,table[3],18u,0x111u,0x222u,0x333u) ||
       instance->highest()!=18 ||
       !instance->last_update_gate_flag_for_test_only() ||
       !trace.matches("ECX",3u) || trace.bad!=0u ||
       ctx.scratch!=&other || other!=saved_before_change ||
       scratch==saved_before_change ||
       trace.received_args!=std::array<std::uint32_t,4>{
           {saved_before_change[0],0x111u,0x222u,0x333u}})
        return fail(7,"original reload-after-vcall global pointer retarget semantics");
    trace.retarget=false;

    // A null NEW destination would crash original x86 during second
    // REP MOVSD. Synthetic fixture explicitly refuses the write.
    ctx.scratch=&other;
    trace.expected=other;
    trace.expected_prior_index=18;
    trace.expected_new_index=19u;
    trace.drop_target=true;
    trace.reset_events();
    const unsigned prev_refusals=p::events.missing_update_restore_target;
    if(!invoke_four(instance,table[3],19u,7u,8u,9u) ||
       !trace.matches("ECX",3u) || trace.bad!=0u ||
       ctx.scratch!=nullptr ||
       p::events.missing_update_restore_target!=prev_refusals+1u)
        return fail(8,"null relocated work buffer not safely refused");
    trace.drop_target=false;

    // Null ORIGINAL pointer must fail before entering profile or
    // reading arbitrary memory; frame index remains unchanged.
    trace.reset_events();
    const unsigned before_missing=p::events.missing_update_scratch;
    if(!invoke_four(instance,table[3],20u,0u,0u,0u) ||
       instance->highest()!=19 ||
       trace.count!=0u ||
       p::events.missing_update_scratch!=before_missing+1u)
        return fail(9,"null source work buffer did not fail before native call");
    instance->set_update_frame_context(nullptr);
    if(!invoke_four(instance,table[3],21u,0u,0u,0u) ||
       p::events.missing_update_scratch!=before_missing+2u)
        return fail(10,"retired update context still called");

    prediction.reset();
    if(p::events.destructor_calls!=1u ||
       p::events.opaque_update_inner_calls!=6u ||
       trace.enter_calls!=6u || trace.exit_calls!=6u)
        return fail(11,"original-free profiler scope/count/lifetime drift");
    std::puts("PASS Phase1664 MSVC Win32 x86 original-free: actual "
              "CPrediction vtable slot3 RET16, frame+0x24 and two-ConVar "
              "gate flag, profile enter/call/exit ordering, full 11-DWORD "
              "owner buffer save/restore across virtual slot22 RET16, "
              "saved work0 + args2-4, restore-time global retarget, "
              "null safety and clean detach, zero retail methods");
    return 0;
}
