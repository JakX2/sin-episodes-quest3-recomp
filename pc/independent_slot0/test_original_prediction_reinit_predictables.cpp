// Phase1683: true native MSVC x86 source-bounded reinit helper,
// ORIGINAL retail method 0x24195C20 110B __thiscall RET0.
// Integrates with Phase1682 authenticated CPrediction vtable[6]
// 0->1 cl_predict state transition RET4, actual original source
// callsite 0x24196485. All objects/data/callbacks are invented.
// No 2006 original executable DLL or engine state is loaded.
#include "native_original_prediction_reinit_predictables.h"
#include "native_client_prediction23_init_probe.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1683 actual native Win32 MSVC x86 test required
#endif
namespace rr=sin_independent::original_prediction_reinit_predictables;
namespace pred=sin_independent::client_prediction23_diagnostic;
namespace {
int fail(int code,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1683 native prediction reinit #%d: %s\n",code,why);
    return code;
}
struct FakeEntity {
    unsigned id=0u;
    std::uint8_t predictable=0u;
};
struct Trace {
    rr::Fixture reinit{};
    pred::Prediction23Fixture* prediction=nullptr;
    std::array<FakeEntity,4> entities{};
    std::array<unsigned,16> accessed_indexes{};
    std::array<unsigned,16> reinitialized_ids{};
    unsigned index_calls=0u;
    unsigned reinit_callbacks=0u;
    unsigned msg_callbacks=0u;
    unsigned enable_callbacks=0u;
    unsigned count_calls=0u;
    unsigned prediction_count_calls=0u;
    unsigned bad=0u;
    std::int32_t maximum=3;
    std::int32_t message_count=11;
    std::int32_t last_message_count=0;
    std::uint32_t expected_pre_reset_frame=0u;
    std::array<char,100> last_message{};
    void reset_log() noexcept {
        index_calls=0u;
        reinit_callbacks=0u;
        msg_callbacks=0u;
        count_calls=0u;
        accessed_indexes.fill(0u);
        reinitialized_ids.fill(0u);
        last_message.fill(0);
    }
};
std::int32_t max_index(void* user) noexcept {
    auto* t=static_cast<Trace*>(user);
    return t->maximum;
}
void* indexed_entity(void* user,std::int32_t index) noexcept {
    auto* t=static_cast<Trace*>(user);
    if(index<0 || index>=4) {
        ++t->bad;
        return nullptr;
    }
    if(t->index_calls<t->accessed_indexes.size())
        t->accessed_indexes[t->index_calls]=
            static_cast<unsigned>(index);
    ++t->index_calls;
    // Original code explicitly skips null entities; index2 is
    // absent in our invented sparse registry.
    if(index==2)return nullptr;
    return &t->entities[static_cast<std::size_t>(index)];
}
std::uint8_t is_predictable(void* user,void* entity) noexcept {
    auto* t=static_cast<Trace*>(user);
    if(!entity){++t->bad;return 0u;}
    auto* obj=static_cast<FakeEntity*>(entity);
    if(obj<&t->entities[0] || obj>&t->entities[3]) {
        ++t->bad;return 0u;
    }
    return obj->predictable;
}
void reinitialize_opaque(void* user,void* entity,
                         const char* reason) noexcept {
    auto* t=static_cast<Trace*>(user);
    if(!entity || reason!=rr::kOriginalReason ||
       std::strcmp(reason,"ReinitPredictables")!=0) {
        ++t->bad;return;
    }
    const auto* obj=static_cast<FakeEntity*>(entity);
    if(obj->predictable!=0u){++t->bad;return;}
    if(t->reinit_callbacks<t->reinitialized_ids.size())
        t->reinitialized_ids[t->reinit_callbacks]=obj->id;
    ++t->reinit_callbacks;
}
std::int32_t count_for_msg(void* user) noexcept {
    auto* t=static_cast<Trace*>(user);
    ++t->count_calls;
    return t->message_count;
}
void on_msg(void* user,const char* format,
            std::int32_t source_count) noexcept {
    auto* t=static_cast<Trace*>(user);
    if(!format ||
       std::strcmp(format,rr::kOriginalMsgLiteral)!=0) {
        ++t->bad;return;
    }
    ++t->msg_callbacks;
    t->last_message_count=source_count;
    const auto n=std::snprintf(t->last_message.data(),
                              t->last_message.size(),
                              format,source_count);
    if(n<0 || static_cast<std::size_t>(n)>=t->last_message.size())
        ++t->bad;
}
std::int32_t pred_entity_count(void* user) noexcept {
    auto* t=static_cast<Trace*>(user);
    ++t->prediction_count_calls;
    return 0;
}
void* pred_entity(void*,std::int32_t) noexcept {return nullptr;}
bool pred_predictable(void*,void*) noexcept {return false;}
std::uint8_t pred_reconcile(void*,void*,std::uint32_t) noexcept {
    return 0u;
}
void on_disabled_to_enabled(void* user) noexcept {
    auto* t=static_cast<Trace*>(user);
    ++t->enable_callbacks;
    if(!t->prediction || t->prediction->enabled()!=0u ||
       t->prediction->value_b()!=t->expected_pre_reset_frame)
        ++t->bad; // original helper runs BEFORE slot6 field reset
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    t->reinit.Reinitialize();
    __asm mov after,esp
    if(before!=after)++t->bad; // native __thiscall no-stack RET0
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
bool call_reinit(rr::Fixture& fixture) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    fixture.Reinitialize();
    __asm mov after,esp
    return before==after;
}
} // namespace
int main() {
    pred::events={};
    Trace t{};
    for(unsigned i=0u;i<t.entities.size();++i)
        t.entities[i].id=i;
    t.entities[1].predictable=1u;
    rr::Ops rops{};
    rops.user=&t;
    rops.max_index_inclusive=&max_index;
    rops.entity_by_index=&indexed_entity;
    rops.is_predictable=&is_predictable;
    rops.reinitialize_opaque=&reinitialize_opaque;
    rops.total_count_for_log=&count_for_msg;
    rops.on_msg=&on_msg;
    t.reinit.ops=rops;

    // Original per-index bound is INCLUSIVE, nulls are skipped,
    // predictable TRUE skips opaque reinitialization. Source
    // Msg count (11) intentionally differs from actual 2 calls.
    if(!call_reinit(t.reinit) || t.bad ||
       t.index_calls!=4u || t.reinit_callbacks!=2u ||
       t.reinitialized_ids[0]!=0u || t.reinitialized_ids[1]!=3u ||
       t.accessed_indexes[0]!=0u || t.accessed_indexes[3]!=3u ||
       t.count_calls!=1u || t.msg_callbacks!=1u ||
       t.last_message_count!=11 ||
       std::strcmp(t.last_message.data(),
          "Reinitialized 11 predictable entities\n")!=0 ||
       t.reinit.events.already_predictable_skips!=1u ||
       t.reinit.events.null_entity_skips!=1u)
        return fail(1,"original inclusive entity indices and separate Msg Count");

    // A MAX index of 0 still visits ONE entity.
    t.reset_log();t.maximum=0;t.message_count=7;
    if(!call_reinit(t.reinit) || t.bad ||
       t.index_calls!=1u || t.reinit_callbacks!=1u ||
       t.reinitialized_ids[0]!=0u || t.last_message_count!=7)
        return fail(2,"source max index zero must still visit index0");
    // A signed negative MAX skips entity enumeration, but
    // STILL reports the separate engine Count.
    t.reset_log();t.maximum=-1;t.message_count=0;
    if(!call_reinit(t.reinit) || t.bad ||
       t.index_calls!=0u || t.reinit_callbacks!=0u ||
       t.msg_callbacks!=1u || t.count_calls!=1u ||
       t.last_message_count!=0)
        return fail(3,"negative maximum must still print Count");
    // Safe-only oversize invalid global index is refused without
    // invoking callbacks on unknown original game pointers.
    t.reset_log();t.maximum=4096;
    const auto over=t.reinit.events.oversized_max_index_refusals;
    if(!call_reinit(t.reinit) || t.bad ||
       t.reinit.events.oversized_max_index_refusals!=over+1u ||
       t.index_calls!=0u || t.msg_callbacks!=0u)
        return fail(4,"oversized original entity-list index not refused");
    // Missing opaque engine backend must NOT be silently replaced.
    t.reinit.ops.reinitialize_opaque=nullptr;
    t.maximum=3;
    const auto refused=t.reinit.events.safe_refusals;
    if(!call_reinit(t.reinit) ||
       t.reinit.events.safe_refusals!=refused+1u)
        return fail(5,"absent original 0x240CC2D0 engine helper not refused");
    t.reinit.ops=rops;

    // Now execute the full vtable slot6 0->1 transition,
    // source-authenticated CALL 0x24195C20 and native reinit
    // member, then exact post-helper reset of +0x24/28/2C.
    auto prediction=std::make_unique<pred::Prediction23Fixture>(nullptr);
    t.prediction=prediction.get();
    auto** table=*reinterpret_cast<void***>(t.prediction);
    pred::PostNetworkOps pops{};
    pops.user=&t;
    pops.local_player_present=true;
    pops.cl_predict=1;
    pops.cl_pred_optimize=0;
    pops.get_count=&pred_entity_count;
    pops.get_entity=&pred_entity;
    pops.is_predictable=&pred_predictable;
    pops.reconcile_entity=&pred_reconcile;
    pops.model_original_tail_transition=true;
    pops.on_disabled_to_enabled=&on_disabled_to_enabled;
    t.prediction->set_postnetwork_ops(&pops);
    t.reset_log();t.maximum=3;t.message_count=11;
    t.expected_pre_reset_frame=201u;
    if(!call_slot6(t.prediction,table[6],201u) ||
       t.bad || t.enable_callbacks!=1u ||
       t.reinit_callbacks!=2u || t.last_message_count!=11 ||
       t.prediction->enabled()!=1u ||
       t.prediction->value_b()!=0u ||
       t.prediction->highest()!=-1 ||
       pred::events.postnetwork_source_tail_reenable_opaque_calls!=1u ||
       pred::events.postnetwork_source_tail_resets!=1u)
        return fail(6,"retail slot6 RET4 -> reinit RET0 BEFORE state reset");

    // Stable enable must preserve original input frame and NOT
    // re-run 0x24195C20. Next disable resets, without reinit.
    t.reset_log();
    if(!call_slot6(t.prediction,table[6],202u) ||
       t.prediction->enabled()!=1u ||
       t.prediction->value_b()!=202u ||
       t.enable_callbacks!=1u || t.msg_callbacks!=0u)
        return fail(7,"stable prediction does not reinitialize");
    pops.cl_predict=0;
    if(!call_slot6(t.prediction,table[6],203u) ||
       t.prediction->enabled()!=0u ||
       t.prediction->value_b()!=0u ||
       t.enable_callbacks!=1u || t.msg_callbacks!=0u)
        return fail(8,"enabled-to-disabled does not reinitialize");

    // Disabled->enabled next time invokes the native helper again,
    // even if the global entity registry has negative max.
    pops.cl_predict=1;
    t.maximum=-1;
    t.expected_pre_reset_frame=204u;
    if(!call_slot6(t.prediction,table[6],204u) ||
       t.bad || t.enable_callbacks!=2u ||
       t.msg_callbacks!=1u || t.reinit_callbacks!=0u ||
       t.prediction->enabled()!=1u ||
       t.prediction->value_b()!=0u)
        return fail(9,"second 0->1 transition executes empty registry log");
    prediction.reset();
    if(pred::events.destructor_calls!=1u)
        return fail(10,"owned native prediction lifetime regression");

    std::puts("PASS Phase1683 original-free MSVC x86: original 110B "
              "CPrediction ReinitPredictables ECX=this RET0, exact "
              "0..max inclusive index search/null/IsPredictable-false "
              "opaque calls, separate engine Count tier0 Msg literal, "
              "real vtable slot6 RET4 -> reinit RET0 before +0x24/"
              "+0x28/+0x2C reset, stable/disabled guards and safe "
              "backend refusal, zero retail engine methods executed");
    return 0;
}
