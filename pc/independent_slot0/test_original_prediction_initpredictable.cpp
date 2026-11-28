// Phase1685: whole original 90B InitPredictable __thiscall RET0
// behavior as original-free native Win32 MSVC x86 reconstruction.
//
// Actual nested native call path tested:
// CPrediction::PostNetworkDataReceived slot6 RET4 ->
// ReinitPredictables 110B RET0 -> entity guard 126B RET4 ->
// InitPredictable 90B RET0 -> 90 indexed opaque field calls.
//
// All engine callbacks, globals, entity memory, and services are
// synthetic host-owned stand-ins. Retail game code never runs.
#include "native_original_prediction_initpredictable.h"
#include "native_original_prediction_reinit_predictables.h"
#include "native_client_prediction23_init_probe.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1685 must test actual native MSVC Windows x86
#endif
namespace init=sin_independent::original_prediction_initpredictable;
namespace guard=sin_independent::original_prediction_entity_reinit_guard;
namespace batch=sin_independent::original_prediction_reinit_predictables;
namespace pred=sin_independent::client_prediction23_diagnostic;
namespace {
int fail(int number,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1685 native InitPredictable x86 #%d: %s\n",number,why);
    return number;
}
struct Trace {
    std::array<init::Fixture,4> entities{};
    batch::Fixture parent{};
    pred::Prediction23Fixture* prediction=nullptr;
    unsigned bad=0u;
    unsigned init_total=0u;
    unsigned nested_final=0u;
    unsigned parent_entity_calls=0u;
    unsigned batch_msgs=0u;
    unsigned batch_log_count=0u;
    unsigned enable_calls=0u;
    std::uint32_t expected_pre_reset_frame=0u;
    std::uint32_t global_mode_result=6u;
    std::uint32_t vcall_return=0xABCDEF01u;
    std::int32_t maximum=3;
    std::int32_t log_count=77;
    std::array<unsigned,4> current_field_index{};
    std::array<unsigned,4> init_call_count{};
    std::array<unsigned,4> fields_completed{};
    std::array<unsigned,4> ordered_step{};
    unsigned index_of(guard::Fixture* p) const noexcept {
        for(unsigned i=0u;i<entities.size();++i)
            if(static_cast<const guard::Fixture*>(&entities[i])==p)
                return i;
        return 0xFFFFFFFFu;
    }
    unsigned index_of(init::Fixture* p) const noexcept {
        return index_of(static_cast<guard::Fixture*>(p));
    }
};
std::uint32_t global_mode(void* user) noexcept {
    return static_cast<Trace*>(user)->global_mode_result;
}
std::uint8_t negative_virtual(void*,guard::Fixture*) noexcept {return 0u;}
std::uint8_t positive_virtual(void*,guard::Fixture*) noexcept {return 1u;}
std::uint32_t entity_vcall44(void* user,init::Fixture* entity) noexcept {
    auto* t=static_cast<Trace*>(user);
    const auto i=t->index_of(entity);
    if(i>=4u) {++t->bad;return 0u;}
    if(entity->predictable_flag()!=1u)++t->bad;
    t->current_field_index[i]=0u;
    t->ordered_step[i]=1u;
    ++t->init_call_count[i];
    return t->vcall_return;
}
void apply_virtual_result(void* user,init::Fixture* entity,
                          std::uint32_t value) noexcept {
    auto* t=static_cast<Trace*>(user);
    const auto i=t->index_of(entity);
    if(i>=4u || t->ordered_step[i]!=1u ||
       value!=t->vcall_return) {++t->bad;return;}
    t->ordered_step[i]=2u;
}
void secondary_init(void* user,init::Fixture* entity) noexcept {
    auto* t=static_cast<Trace*>(user);
    const auto i=t->index_of(entity);
    if(i>=4u || t->ordered_step[i]!=2u) {++t->bad;return;}
    t->ordered_step[i]=3u;
}
void notify_global_handle(void* user,std::uint32_t handle) noexcept {
    auto* t=static_cast<Trace*>(user);
    // Each synthetic owner uses same source-observable handle.
    if(handle!=0x13572468u)++t->bad;
    // The global receives no 'this' entity parameter, so
    // verify its position using exactly one pending owner.
    unsigned candidates=0u;
    for(unsigned i=0u;i<4u;++i) {
        if(t->ordered_step[i]==3u) {
            ++candidates;
            t->ordered_step[i]=4u;
        }
    }
    if(candidates!=1u)++t->bad;
}
void reconcile_zero(void* user,init::Fixture* entity,
                    std::uint32_t input) noexcept {
    auto* t=static_cast<Trace*>(user);
    const auto i=t->index_of(entity);
    if(i>=4u || t->ordered_step[i]!=4u || input!=0u)
        {++t->bad;return;}
    t->ordered_step[i]=5u;
}
void initialize_one_field(void* user,init::Fixture* entity,
                          const char* name,std::uint32_t index,
                          std::uint32_t third) noexcept {
    auto* t=static_cast<Trace*>(user);
    const auto i=t->index_of(entity);
    if(i>=4u || t->ordered_step[i]!=5u ||
       !name || std::strcmp(name,init::kOriginalFieldLabel)!=0 ||
       index!=t->current_field_index[i] ||
       index>=init::kOriginalFieldCount || third!=0u) {
        ++t->bad;return;
    }
    ++t->current_field_index[i];
    ++t->fields_completed[i];
    ++t->init_total;
}
bool call_native_init(init::Fixture& entity) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    entity.InitializePredictable();
    __asm mov after,esp
    return before==after;
}
bool call_native_guard(guard::Fixture& entity,
                       const char* reason) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    entity.ReinitEntity(reason);
    __asm mov after,esp
    return before==after;
}
bool call_native_batch(batch::Fixture& parent) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    parent.Reinitialize();
    __asm mov after,esp
    return before==after;
}
bool call_virtual_slot6(void* owner,void* fn,
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
void final_from_guard(void* user,guard::Fixture* source) noexcept {
    auto* t=static_cast<Trace*>(user);
    const auto index=t->index_of(source);
    if(index>=4u) {++t->bad;return;}
    if(source->latched_c0==0u || source->final_gate_454!=0u)
        ++t->bad;
    ++t->nested_final;
    if(!call_native_init(t->entities[index]))++t->bad;
    if(t->current_field_index[index]!=init::kOriginalFieldCount ||
       t->ordered_step[index]!=5u)
        ++t->bad;
}
void install(init::Fixture& entity,Trace& t) noexcept {
    guard::Ops g{};
    g.user=&t;
    g.cl_predict_numeric=1;
    g.mode_provider=&global_mode;
    g.predicate_virtual_1a0=&negative_virtual;
    g.predicate_virtual_1bc=&positive_virtual;
    g.on_final_opaque=&final_from_guard;
    entity.ops=g;
    init::Ops o{};
    o.user=&t;
    o.virtual_44=&entity_vcall44;
    o.apply_virtual_result=&apply_virtual_result;
    o.secondary_init=&secondary_init;
    o.notify_global_handle=&notify_global_handle;
    o.reconcile_zero=&reconcile_zero;
    o.initialize_field=&initialize_one_field;
    entity.init_ops=o;
    entity.set_owner_handle(0x13572468u);
}
std::int32_t get_max(void* user) noexcept {
    return static_cast<Trace*>(user)->maximum;
}
void* indexed_entity(void* user,std::int32_t index) noexcept {
    auto* t=static_cast<Trace*>(user);
    if(index<0 || index>=4) {++t->bad;return nullptr;}
    return index==2?nullptr:
        static_cast<guard::Fixture*>(&t->entities[
            static_cast<unsigned>(index)]);
}
std::uint8_t is_predictable(void* user,void* entity) noexcept {
    auto* t=static_cast<Trace*>(user);
    const auto i=t->index_of(
        static_cast<guard::Fixture*>(entity));
    if(i>=4u)++t->bad;
    return i==1u?1u:0u; // synthetic index1 skip
}
void call_entity_guard(void* user,void* entity,
                       const char* label) noexcept {
    auto* t=static_cast<Trace*>(user);
    ++t->parent_entity_calls;
    if(!label || std::strcmp(label,
                            batch::kOriginalReason)!=0)
        ++t->bad;
    if(!call_native_guard(*static_cast<guard::Fixture*>(entity),
                          label))++t->bad;
}
std::int32_t get_log_count(void* user) noexcept {
    return static_cast<Trace*>(user)->log_count;
}
void emit_msg(void* user,const char* format,
              std::int32_t count) noexcept {
    auto* t=static_cast<Trace*>(user);
    ++t->batch_msgs;
    t->batch_log_count=static_cast<unsigned>(count);
    if(!format ||
       std::strcmp(format,batch::kOriginalMsgLiteral)!=0 ||
       count!=t->log_count)++t->bad;
}
std::int32_t pred_count(void*) noexcept {return 0;}
void* pred_get(void*,std::int32_t) noexcept {return nullptr;}
bool pred_is_predictable(void*,void*) noexcept {return false;}
std::uint8_t pred_reconcile(void*,void*,std::uint32_t) noexcept {
    return 0u;
}
void on_enable_prediction(void* user) noexcept {
    auto* t=static_cast<Trace*>(user);
    ++t->enable_calls;
    if(!t->prediction || t->prediction->enabled()!=0u ||
       t->prediction->value_b()!=t->expected_pre_reset_frame)
        ++t->bad;
    if(!call_native_batch(t->parent))++t->bad;
}
} // namespace
int main() {
    pred::events={};
    Trace t{};
    for(auto& entity:t.entities)install(entity,t);
    auto& e=t.entities[0];
    const auto base=reinterpret_cast<std::uintptr_t>(&e);
    if(reinterpret_cast<std::uintptr_t>(&e.latched_c0)-base!=0xC0u ||
       reinterpret_cast<std::uintptr_t>(&e.final_gate_454)-base!=0x454u ||
       e.owner_handle()!=0x13572468u ||
       e.predictable_flag()!=0u)
        return fail(1,"same native owner source-offset +0xB4/+0x2E8 drift");
    // Standalone real CDECL? Original is thiscall RET0 with zero
    // stack args. Prove 90 exact original per-index calls.
    if(!call_native_init(e) || t.bad ||
       e.predictable_flag()!=1u ||
       e.init_events.virtual_calls!=1u ||
       e.init_events.result_helper_calls!=1u ||
       e.init_events.secondary_calls!=1u ||
       e.init_events.global_handle_calls!=1u ||
       e.init_events.reconcile_calls!=1u ||
       e.init_events.per_field_calls!=90u ||
       t.fields_completed[0]!=90u ||
       t.init_total!=90u ||
       e.init_events.completed!=1u)
        return fail(2,"retail InitPredictable 90-item native RET0 call order");
    // Source flag write precedes ALL injected engine methods;
    // missing callback is host-refused without inventing effects.
    auto& missing=t.entities[2];
    missing.init_ops.virtual_44=nullptr;
    if(!call_native_init(missing) ||
       missing.predictable_flag()!=1u ||
       missing.init_events.flag_written!=1u ||
       missing.init_events.safe_refusals!=1u ||
       missing.init_events.per_field_calls!=0u)
        return fail(3,"early flag write and unknown vcall refusal");
    missing.init_ops.virtual_44=&entity_vcall44;

    // Original entity guard 126B RET4 invokes the SAME owner
    // Phase1685 method no-arg RET0 only after its gates succeed.
    e.latched_c0=0u;
    e.embedded_state=11u;
    if(!call_native_guard(e,batch::kOriginalReason) ||
       t.bad || e.latched_c0!=1u ||
       e.init_events.completed!=2u ||
       e.init_events.per_field_calls!=180u ||
       t.nested_final!=1u || t.init_total!=180u)
        return fail(4,"entity RET4 -> InitPredictable RET0");

    // Parent ReinitPredictables RET0 indexes 0..max inclusive,
    // skips sparse null index2/predictable index1, invokes each
    // child RET4 and sends its Count through separate Msg.
    auto& e3=t.entities[3];
    e3.latched_c0=0u;
    e3.embedded_state=11u;
    batch::Ops bops{};
    bops.user=&t;
    bops.max_index_inclusive=&get_max;
    bops.entity_by_index=&indexed_entity;
    bops.is_predictable=&is_predictable;
    bops.reinitialize_opaque=&call_entity_guard;
    bops.total_count_for_log=&get_log_count;
    bops.on_msg=&emit_msg;
    t.parent.ops=bops;
    if(!call_native_batch(t.parent) || t.bad ||
       t.parent_entity_calls!=2u ||
       t.nested_final!=3u ||
       t.init_total!=360u ||
       t.fields_completed[0]!=270u ||
       t.fields_completed[3]!=90u ||
       t.batch_msgs!=1u || t.batch_log_count!=77u ||
       t.parent.events.null_entity_skips!=1u ||
       t.parent.events.already_predictable_skips!=1u)
        return fail(5,"batch RET0 -> two entity RET4 -> two 90-call RET0");

    // VClientPrediction001 slot6 native virtual RET4 now
    // composes all four source-authenticated stack shapes and
    // transfers the full 90-item init under a 0->1 transition.
    auto prediction=std::make_unique<pred::Prediction23Fixture>(nullptr);
    t.prediction=prediction.get();
    auto** table=*reinterpret_cast<void***>(t.prediction);
    pred::PostNetworkOps p{};
    p.user=&t;
    p.local_player_present=true;
    p.cl_predict=1;
    p.cl_pred_optimize=0;
    p.get_count=&pred_count;
    p.get_entity=&pred_get;
    p.is_predictable=&pred_is_predictable;
    p.reconcile_entity=&pred_reconcile;
    p.model_original_tail_transition=true;
    p.on_disabled_to_enabled=&on_enable_prediction;
    t.prediction->set_postnetwork_ops(&p);
    t.expected_pre_reset_frame=901u;
    if(!call_virtual_slot6(t.prediction,table[6],901u) ||
       t.bad || t.enable_calls!=1u ||
       t.prediction->enabled()!=1u ||
       t.prediction->value_b()!=0u ||
       t.prediction->highest()!=-1 ||
       t.nested_final!=5u ||
       t.init_total!=540u ||
       t.batch_msgs!=2u ||
       t.parent_entity_calls!=4u ||
       t.entities[0].init_events.completed!=4u ||
       t.entities[3].init_events.completed!=2u)
        return fail(6,"full prediction slot6->batch->entity->90 init");
    // Stable cl_predict and disable transitions do NOT re-enter
    // the 90-item initialization pipeline.
    if(!call_virtual_slot6(t.prediction,table[6],902u) ||
       t.prediction->value_b()!=902u ||
       t.init_total!=540u || t.enable_calls!=1u)
        return fail(7,"stable prediction reinitialization improperly repeated");
    p.cl_predict=0;
    if(!call_virtual_slot6(t.prediction,table[6],903u) ||
       t.prediction->enabled()!=0u ||
       t.prediction->value_b()!=0u ||
       t.init_total!=540u || t.enable_calls!=1u)
        return fail(8,"enabled-to-disabled must not run InitPredictable");
    prediction.reset();
    if(pred::events.destructor_calls!=1u || t.bad)
        return fail(9,"owned native prediction fixture lifetime");
    std::puts("PASS Phase1685 original-free native Win32 MSVC x86: "
              "whole-original 90B InitPredictable RET0, exact "
              "source +0x2E8 flag / +0xB4 handle, original five "
              "downstream callback order, ninety 0..89 indexed "
              "InitPredictable field calls, unknown-backend refusal, "
              "real CPrediction slot6 RET4->batch RET0->entity "
              "RET4->Init RET0 nested native stack invariants and "
              "stable/disabled gates; zero retail game code executed");
    return 0;
}
