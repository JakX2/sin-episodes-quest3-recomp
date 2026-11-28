// Phase1682: native owner-original-free MSVC x86 CPrediction vtable
// slot6 RET4 with the newly authenticated 58B final cl_predict
// state-transition gate. The 2006 retail method remains 1039B
// SHA-pinned; only the additional +0x10/+0x24/+0x28/+0x2C
// writes and the opaque disabled->enabled call are modeled here.
// All other post-loop debug/refresh/correction paths stay opaque.
#include "native_client_prediction23_init_probe.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1682 requires genuine Win32 MSVC x86 native ABI tests
#endif
namespace pred=sin_independent::client_prediction23_diagnostic;
namespace {
int fail(int code,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1682 source tail CPrediction slot6 x86 #%d: %s\n",
      code,why);
    return code;
}
struct Trace {
    unsigned enabled_callbacks=0u;
    unsigned opaque_tail_callbacks=0u;
    unsigned entity_counts=0u;
    unsigned bad=0u;
};
void on_enable(void* user) noexcept {
    auto* t=static_cast<Trace*>(user);
    if(t)++t->enabled_callbacks;
}
void on_opaque_tail(void* user) noexcept {
    auto* t=static_cast<Trace*>(user);
    if(t)++t->opaque_tail_callbacks;
}
std::int32_t get_count(void* user) noexcept {
    auto* t=static_cast<Trace*>(user);
    if(t)++t->entity_counts;
    return 0;
}
void* get_entity(void*,std::int32_t) noexcept {return nullptr;}
bool predictable(void*,void*) noexcept {return false;}
std::uint8_t reconcile(void*,void*,std::uint32_t) noexcept {return 0u;}
bool invoke_slot6(void* owner,void* fn,
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
void set_member(std::uint8_t* owner,unsigned offset,
                std::uint32_t value) noexcept {
    // All storage is inside the owned synthetic fixture, not real
    // retail game memory. memcpy writes actual native DWORD fields.
    std::memcpy(owner+offset,&value,sizeof(value));
}
}
int main() {
    pred::events={};
    auto p=std::make_unique<pred::Prediction23Fixture>(nullptr);
    auto* owner=p.get();
    auto** table=*reinterpret_cast<void***>(owner);
    if(!table[6] || table[6]==table[7] ||
       pred::kOriginalPostNetworkMethodVa!=0x241960E0u)
        return fail(1,"original prediction vtable slot6 or method VA drift");
    const auto base=reinterpret_cast<std::uintptr_t>(owner);
    if(owner->value_b_address()-base!=0x2Cu ||
       owner->postnetwork_any_reconciled_address()-base!=0x30u)
        return fail(2,"retail prediction field offsets moved");

    Trace t{};
    pred::PostNetworkOps ops{};
    ops.user=&t;
    ops.local_player_present=true;
    ops.cl_predict=1;
    ops.cl_pred_optimize=0;
    ops.get_count=&get_count;
    ops.get_entity=&get_entity;
    ops.is_predictable=&predictable;
    ops.reconcile_entity=&reconcile;
    ops.on_opaque_remaining_branches=&on_opaque_tail;
    ops.model_original_tail_transition=true;
    ops.on_disabled_to_enabled=&on_enable;
    owner->set_postnetwork_ops(&ops);

    // From source old this+0x10=0, new parent value!=0:
    // call 0x24195C20-like backend FIRST, then clear two
    // words and set +0x24=-1, finally +0x10=1.
    set_member(reinterpret_cast<std::uint8_t*>(owner),0x24u,25u);
    set_member(reinterpret_cast<std::uint8_t*>(owner),0x28u,42u);
    if(!invoke_slot6(owner,table[6],101u) ||
       owner->enabled()!=1u || owner->highest()!=-1 ||
       owner->value_a()!=0u || owner->value_b()!=0u ||
       t.enabled_callbacks!=1u || t.opaque_tail_callbacks!=1u ||
       pred::events.postnetwork_source_tail_transition_checks!=1u ||
       pred::events.postnetwork_source_tail_resets!=1u ||
       pred::events.postnetwork_source_tail_reenable_opaque_calls!=1u ||
       t.bad)
        return fail(3,"retail disabled->enabled call-before-reset state");
    // Stable nonzero cl_predict: original must KEEP new frame
    // at this+0x2C and MUST NOT clear any other state.
    set_member(reinterpret_cast<std::uint8_t*>(owner),0x24u,9u);
    set_member(reinterpret_cast<std::uint8_t*>(owner),0x28u,17u);
    if(!invoke_slot6(owner,table[6],102u) ||
       owner->enabled()!=1u || owner->highest()!=9 ||
       owner->value_a()!=17u || owner->value_b()!=102u ||
       t.enabled_callbacks!=1u ||
       pred::events.postnetwork_source_tail_resets!=1u)
        return fail(4,"stable enabled prediction preserves frame/index");

    // Enabled->disabled cl_predict transition resets the same
    // three fields but does NOT call disabled->enabled helper.
    ops.cl_predict=0;
    if(!invoke_slot6(owner,table[6],103u) ||
       owner->enabled()!=0u || owner->highest()!=-1 ||
       owner->value_a()!=0u || owner->value_b()!=0u ||
       t.enabled_callbacks!=1u ||
       pred::events.postnetwork_source_tail_resets!=2u)
        return fail(5,"enabled->disabled reset without reenable callback");
    // Stable disabled state preserves original entry+0x2C.
    set_member(reinterpret_cast<std::uint8_t*>(owner),0x28u,0x3456u);
    if(!invoke_slot6(owner,table[6],104u) ||
       owner->enabled()!=0u || owner->value_a()!=0x3456u ||
       owner->value_b()!=104u ||
       pred::events.postnetwork_source_tail_resets!=2u)
        return fail(6,"stable disabled prediction retains input frame");

    // Original local-player-global NULL goes DIRECT to final
    // profiler exit before this new tail gate, so no reset
    // even if cl_predict toggles, despite entry frame write.
    ops.local_player_present=false;
    ops.cl_predict=1;
    const auto checked=pred::events.postnetwork_source_tail_transition_checks;
    if(!invoke_slot6(owner,table[6],105u) ||
       owner->enabled()!=0u || owner->value_a()!=0x3456u ||
       owner->value_b()!=105u ||
       pred::events.postnetwork_source_tail_transition_checks!=checked ||
       t.enabled_callbacks!=1u)
        return fail(7,"no-local-player early retail epilogue skips tail");

    // Host-only refusal: without injected original 0x24195C20
    // behavior, do not pretend its unknown side effects ran.
    ops.local_player_present=true;
    ops.on_disabled_to_enabled=nullptr;
    const auto missing=pred::events.postnetwork_source_tail_missing_opaque;
    if(!invoke_slot6(owner,table[6],106u) ||
       pred::events.postnetwork_source_tail_missing_opaque!=missing+1u ||
       owner->enabled()!=0u || owner->value_b()!=106u ||
       t.enabled_callbacks!=1u)
        return fail(8,"unknown original reenable helper cannot be invented");
    ops.on_disabled_to_enabled=&on_enable;
    if(!invoke_slot6(owner,table[6],107u) ||
       owner->enabled()!=1u || owner->value_b()!=0u ||
       owner->value_a()!=0u || owner->highest()!=-1 ||
       t.enabled_callbacks!=2u ||
       pred::events.postnetwork_source_tail_resets!=3u)
        return fail(9,"real-native retry with supplied opaque helper");

    // This verifies the earlier original-free Phase1668 fallback
    // still marks that remaining postloop behavior is unknown.
    if(t.opaque_tail_callbacks!=7u ||
       pred::events.postnetwork_unmodeled_tail_calls!=7u ||
       t.entity_counts!=6u)
        return fail(10,"unmodeled tail callback/Count guards drift");
    // In particular, owner+0x18/+0x1C/+0x20 snapshot writes from
    // later original 0x240C4940 are NOT implemented or claimed.
    p.reset();
    if(pred::events.destructor_calls!=1u)
        return fail(11,"original-free prediction fixture lifetime drift");
    std::puts("PASS Phase1682 original-free MSVC x86: 1039B "
              "SHA-attested CPrediction slot6 RET4, opt-in "
              "original 58B tail bool cl_predict vs this+0x10, "
              "disabled->enabled opaque call before +0x24/+0x28/"
              "+0x2C reset, stable-state no-reset, local-player "
              "early epilogue and test-only missing-helper refusal; "
              "all other postloop child methods remain opaque");
    return 0;
}
