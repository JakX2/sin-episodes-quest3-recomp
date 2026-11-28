// Phase1684: actual Win32 MSVC x86 native __thiscall RET4 entity
// guard 0x240CC2D0, nested under Phase1683 ReinitPredictables
// RET0 and existing Phase1682 CPrediction virtual slot6 RET4.
//
// The original 126B helper and two separate full 7B/8B embedded
// state helpers are SHA-authenticated against complete retail DLLs.
// All memory, functions, "entities", globals and outputs here are
// synthetic host-owned stand-ins. No original DLL is executed.
#include "native_original_prediction_entity_reinit_guard.h"
#include "native_original_prediction_reinit_predictables.h"
#include "native_client_prediction23_init_probe.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1684 requires real native MSVC Windows x86 tests
#endif
namespace guard=sin_independent::original_prediction_entity_reinit_guard;
namespace reinit=sin_independent::original_prediction_reinit_predictables;
namespace pred=sin_independent::client_prediction23_diagnostic;
namespace {
int fail(int number,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1684 native entity-reinit RET4 #%d: %s\n",number,why);
    return number;
}
struct Trace {
    std::array<guard::Fixture,4> entities{};
    reinit::Fixture batch{};
    pred::Prediction23Fixture* prediction=nullptr;
    unsigned bad=0u;
    std::array<char,128> order{};
    unsigned order_count=0u;
    unsigned total_messages=0u;
    unsigned total_reinit_entities=0u;
    unsigned opaque_final_calls=0u;
    unsigned batch_counts=0u;
    unsigned enable_callbacks=0u;
    std::uint32_t mode_result=6u;
    std::uint8_t negative_virtual_result=0u;
    std::uint8_t positive_virtual_result=1u;
    std::int32_t maximum=3;
    std::int32_t separate_entity_count=99;
    std::uint32_t expected_pre_reset_frame=0u;
    guard::Fixture* expected_entity=nullptr;
    void append(char c) noexcept {
        if(order_count>=order.size())++bad;
        else order[order_count++]=c;
    }
    void reset_order() noexcept {
        order.fill(0);order_count=0u;
    }
    bool matches(const char* x) const noexcept {
        return order_count==std::strlen(x) &&
               std::memcmp(order.data(),x,order_count)==0;
    }
    unsigned index_of(guard::Fixture* p) const noexcept {
        for(unsigned i=0u;i<entities.size();++i)
            if(p==&entities[i])return i;
        return 0xFFFFFFFFu;
    }
};
std::uint32_t get_mode(void* user) noexcept {
    auto* t=static_cast<Trace*>(user);
    t->append('M');
    return t->mode_result;
}
std::uint8_t virtual_1a0(void* user,guard::Fixture* entity) noexcept {
    auto* t=static_cast<Trace*>(user);
    t->append('A');
    if(t->index_of(entity)>=4u)++t->bad;
    return t->negative_virtual_result;
}
std::uint8_t virtual_1bc(void* user,guard::Fixture* entity) noexcept {
    auto* t=static_cast<Trace*>(user);
    t->append('B');
    if(t->index_of(entity)>=4u)++t->bad;
    return t->positive_virtual_result;
}
void final_opaque(void* user,guard::Fixture* entity) noexcept {
    auto* t=static_cast<Trace*>(user);
    t->append('F');
    if(t->index_of(entity)>=4u || entity->final_gate_454!=0u)
        ++t->bad;
    ++t->opaque_final_calls;
}
bool call_native(guard::Fixture& entity,
                 const char* reason) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    entity.ReinitEntity(reason);
    __asm mov after,esp
    return before==after;
}
bool call_batch(reinit::Fixture& batch) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    batch.Reinitialize();
    __asm mov after,esp
    return before==after;
}
bool call_slot6(void* owner,void* fn,
                std::uint32_t frame) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm {
        mov before,esp
        push frame
        mov ecx,owner
        mov eax,fn
        call eax
        mov after,esp
    }
    return before==after;
}
std::int32_t batch_max_index(void* user) noexcept {
    return static_cast<Trace*>(user)->maximum;
}
void* batch_find_index(void* user,
                       std::int32_t index) noexcept {
    auto* t=static_cast<Trace*>(user);
    if(index<0 || index>3) {
        ++t->bad;
        return nullptr;
    }
    // Invented sparse entity registry: index2 is absent.
    return index==2 ? nullptr : &t->entities[
        static_cast<unsigned>(index)];
}
std::uint8_t batch_is_predictable(
        void* user,void* entity) noexcept {
    auto* t=static_cast<Trace*>(user);
    const auto index=t->index_of(
        static_cast<guard::Fixture*>(entity));
    if(index>=4u)++t->bad;
    // Invented index1 is predictable. Phase1683 source only
    // calls reinit for nonnull entities with FALSE predicate.
    return index==1u?1u:0u;
}
void batch_entity_callback(void* user,void* entity,
                           const char* reason) noexcept {
    auto* t=static_cast<Trace*>(user);
    ++t->total_reinit_entities;
    if(!reason || std::strcmp(reason,
               reinit::kOriginalReason)!=0)
        ++t->bad;
    auto* current=static_cast<guard::Fixture*>(entity);
    if(t->index_of(current)>=4u) {
        ++t->bad;return;
    }
    // NEW: forward actual Phase1683 parent one-pointer argument
    // into the reconstructed native Phase1684 RET4 child.
    if(!call_native(*current,reason))++t->bad;
}
std::int32_t batch_count_for_log(void* user) noexcept {
    auto* t=static_cast<Trace*>(user);
    ++t->batch_counts;
    return t->separate_entity_count;
}
void batch_msg(void* user,const char* format,
               std::int32_t count) noexcept {
    auto* t=static_cast<Trace*>(user);
    ++t->total_messages;
    if(!format ||
       std::strcmp(format,reinit::kOriginalMsgLiteral)!=0 ||
       count!=t->separate_entity_count)
        ++t->bad;
}
std::int32_t pred_entity_count(void*) noexcept {return 0;}
void* pred_entity_by_index(void*,std::int32_t) noexcept {return nullptr;}
bool pred_predictable(void*,void*) noexcept {return false;}
std::uint8_t pred_reconcile(void*,void*,std::uint32_t) noexcept {
    return 0u;
}
void on_enable_prediction(void* user) noexcept {
    auto* t=static_cast<Trace*>(user);
    ++t->enable_callbacks;
    if(!t->prediction || t->prediction->enabled()!=0u ||
       t->prediction->value_b()!=t->expected_pre_reset_frame)
        ++t->bad;
    // Parent 0x24195C20 RET0 executed before slot6 clears fields.
    if(!call_batch(t->batch))++t->bad;
}
void set_callbacks(guard::Fixture& e,Trace& t) noexcept {
    guard::Ops o{};
    o.user=&t;
    o.cl_predict_numeric=1;
    o.mode_provider=&get_mode;
    o.predicate_virtual_1a0=&virtual_1a0;
    o.predicate_virtual_1bc=&virtual_1bc;
    o.on_final_opaque=&final_opaque;
    e.ops=o;
}
} // namespace
int main() {
    pred::events={};
    Trace t{};
    for(auto& entity:t.entities)set_callbacks(entity,t);
    auto& e=t.entities[0];
    const auto base=reinterpret_cast<std::uintptr_t>(&e);
    if(reinterpret_cast<std::uintptr_t>(&e.embedded_state)-base!=0xA4u ||
       reinterpret_cast<std::uintptr_t>(&e.latched_c0)-base!=0xC0u ||
       reinterpret_cast<std::uintptr_t>(&e.final_gate_454)-base!=0x454u)
        return fail(1,"original entity source offsets drift");

    // Guard 1: original global cl_predict parent value zero exits
    // before ever reading/vcalling embedded state or entity flags.
    t.reset_order();e.ops.cl_predict_numeric=0;
    if(!call_native(e,reinit::kOriginalReason) ||
       !t.matches("") || e.latched_c0!=0u ||
       e.events.cl_predict_off_skips!=1u || t.bad)
        return fail(2,"retail cl_predict global numeric gate");
    e.ops.cl_predict_numeric=1;
    // Guard 2: original 7B helper checks full DWORD at this+0xA4
    // for ZERO. No index provider or callback should execute.
    t.reset_order();e.embedded_state=0u;
    if(!call_native(e,reinit::kOriginalReason) ||
       !t.matches("") || e.events.state_zero_skips!=1u ||
       e.latched_c0!=0u)
        return fail(3,"source 7B embedded-state nonzero guard");

    // Index helper 8B: ((state >>1)&0x1F), requiring provider
    // virtual+0x30 result-1 to match the encoded index.
    e.embedded_state=11u; // source index=5, required provider=6
    t.reset_order();t.mode_result=7u;
    if(!call_native(e,reinit::kOriginalReason) ||
       !t.matches("M") || e.latched_c0!=0u ||
       e.events.mode_index_mismatch_skips!=1u)
        return fail(4,"source global provider minus-one mode mismatch");
    t.mode_result=6u;
    t.negative_virtual_result=1u;
    t.reset_order();
    if(!call_native(e,reinit::kOriginalReason) ||
       !t.matches("MA") || e.latched_c0!=1u ||
       e.events.newly_latched!=1u ||
       e.events.negative_virtual_blocked!=1u)
        return fail(5,"source latch set BEFORE virtual +0x1A0 check");

    // If already latched, source skips ALL state+0xA4/helper/
    // global-provider checks. The two virtual gates still run.
    e.embedded_state=0u;
    e.ops.mode_provider=nullptr;
    t.negative_virtual_result=0u;
    t.positive_virtual_result=0u;
    t.reset_order();
    if(!call_native(e,reinit::kOriginalReason) ||
       !t.matches("AB") || e.events.previously_latched!=1u ||
       e.events.positive_virtual_blocked!=1u)
        return fail(6,"latched fast path and virtual+0x1BC gate");
    t.positive_virtual_result=1u;
    e.final_gate_454=1u;
    t.reset_order();
    if(!call_native(e,reinit::kOriginalReason) ||
       !t.matches("AB") || e.events.final_gate_blocked!=1u ||
       t.opaque_final_calls!=0u)
        return fail(7,"nonzero source this+0x454 suppresses final call");
    e.final_gate_454=0u;
    t.reset_order();
    if(!call_native(e,reinit::kOriginalReason) ||
       !t.matches("ABF") || t.opaque_final_calls!=1u ||
       e.events.final_helper_calls!=1u || t.bad)
        return fail(8,"source final original 0x240CAF40 callback order");

    // An unknown final engine helper is not invented. Source
    // has already set latch by this point, so preserve it.
    e.ops.on_final_opaque=nullptr;
    t.reset_order();
    const auto prev_missing=e.events.missing_backend_refusals;
    if(!call_native(e,reinit::kOriginalReason) ||
       !t.matches("AB") ||
       e.events.missing_backend_refusals!=prev_missing+1u ||
       e.latched_c0!=1u)
        return fail(9,"host-only unknown 0x240CAF40 refused");
    e.ops.on_final_opaque=&final_opaque;

    // Re-test fresh initialization with high source bits.
    // 0xFFFFFFFF -> ((state>>1)&31)==31; provider must be 32.
    auto& high=t.entities[3];
    high.embedded_state=0xFFFFFFFFu;
    t.mode_result=32u;
    t.reset_order();
    if(!call_native(high,reinit::kOriginalReason) ||
       !t.matches("MABF") || high.latched_c0!=1u ||
       high.events.final_helper_calls!=1u)
        return fail(10,"encoded state upper-bit masking/virtual gate");
    t.mode_result=6u;

    // Compose original 110B Phase1683 parent and real compiler-
    // generated Phase1684 one-arg RET4 child with actual entity
    // index iteration. Each call checks ESP and source predicate.
    t.reset_order();
    e.embedded_state=11u;
    e.latched_c0=0u;
    // Earlier missing-provider test removed the callback; a fresh
    // source-authenticated latch must restore this service.
    e.ops.mode_provider=&get_mode;
    high.latched_c0=1u; // already cached, no global mode check
    high.embedded_state=0u;
    reinit::Ops batch_ops{};
    batch_ops.user=&t;
    batch_ops.max_index_inclusive=&batch_max_index;
    batch_ops.entity_by_index=&batch_find_index;
    batch_ops.is_predictable=&batch_is_predictable;
    batch_ops.reinitialize_opaque=&batch_entity_callback;
    batch_ops.total_count_for_log=&batch_count_for_log;
    batch_ops.on_msg=&batch_msg;
    t.batch.ops=batch_ops;
    if(!call_batch(t.batch) || t.bad ||
       !t.matches("MABFABF") ||
       t.total_reinit_entities!=2u ||
       t.opaque_final_calls!=4u ||
       t.total_messages!=1u || t.batch_counts!=1u ||
       t.batch.events.index_lookup_calls!=4u ||
       t.batch.events.already_predictable_skips!=1u ||
       t.batch.events.null_entity_skips!=1u ||
       t.batch.events.reinitialize_calls!=2u)
        return fail(11,"native RET0 parent -> RET4 child two indexed entities");

    // Integrate with the already-authenticated original
    // CPrediction virtual slot6 RET4 disabled->enabled transition:
    // slot6 -> parent RET0 -> entity RET4 -> final opaque, then
    // slot6 resets original this+0x24/+0x28/+0x2C.
    auto prediction=std::make_unique<pred::Prediction23Fixture>(nullptr);
    t.prediction=prediction.get();
    auto** table=*reinterpret_cast<void***>(t.prediction);
    pred::PostNetworkOps pops{};
    pops.user=&t;
    pops.local_player_present=true;
    pops.cl_predict=1;
    pops.cl_pred_optimize=0;
    pops.get_count=&pred_entity_count;
    pops.get_entity=&pred_entity_by_index;
    pops.is_predictable=&pred_predictable;
    pops.reconcile_entity=&pred_reconcile;
    pops.model_original_tail_transition=true;
    pops.on_disabled_to_enabled=&on_enable_prediction;
    t.prediction->set_postnetwork_ops(&pops);
    t.reset_order();
    t.expected_pre_reset_frame=505u;
    if(!call_slot6(t.prediction,table[6],505u) ||
       t.bad || !t.matches("ABFABF") ||
       t.prediction->enabled()!=1u ||
       t.prediction->value_b()!=0u ||
       t.prediction->value_a()!=0u ||
       t.prediction->highest()!=-1 ||
       t.enable_callbacks!=1u ||
       t.total_messages!=2u ||
       t.total_reinit_entities!=4u ||
       t.opaque_final_calls!=6u)
        return fail(12,"CPrediction slot6 RET4 -> reinit RET0 -> entity RET4");

    // Stable cl_predict does not call either reinit method.
    t.reset_order();
    if(!call_slot6(t.prediction,table[6],506u) ||
       !t.matches("") || t.enable_callbacks!=1u ||
       t.prediction->value_b()!=506u ||
       t.total_messages!=2u)
        return fail(13,"stable enabled prediction avoids entity reinit");

    // Disabling prediction resets without reinitializing.
    pops.cl_predict=0;
    t.reset_order();
    if(!call_slot6(t.prediction,table[6],507u) ||
       !t.matches("") || t.enable_callbacks!=1u ||
       t.prediction->enabled()!=0u ||
       t.prediction->value_b()!=0u)
        return fail(14,"disable transition never reinitializes entities");
    prediction.reset();
    if(pred::events.destructor_calls!=1u || t.bad)
        return fail(15,"synthetic prediction fixture lifetime or callback");

    std::puts("PASS Phase1684 original-free native Win32 MSVC x86: "
              "126-byte entity Reinit RET4, two 7B/8B original "
              "encoded-state helpers, cl_predict/byte+0xC0 latch, "
              "global virtual mode index, source virtual+0x1A0/"
              "+0x1BC and DWORD+0x454 gates, opaque final helper, "
              "full CPrediction slot6 RET4 -> batch reinit RET0 "
              "-> per-entity RET4 integration with stable/disable "
              "guards; zero original game code executed");
    return 0;
}
