// Phase1668: actual MSVC Win32 x86 PostNetworkDataReceived slot6 ABI.
// Only original-authenticated entry/reconcile PREFIX is modeled.
// Later 2006 native debug/validation/correction code remains opaque.
//
// No original client/server DLL loaded, original game memory, or
// implementation of 0x240C6900 is executed. All owners are invented.
#include "native_client_prediction23_init_probe.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1668 PostNetworkDataReceived test requires native MSVC x86
#endif
namespace pred=sin_independent::client_prediction23_diagnostic;
namespace {
int fail(int code,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1668 original-free postnetwork slot6 x86 #%d: %s\n",
      code,why);
    return code;
}
bool invoke_one(void* object,void* fn,
                std::uint32_t arg) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm {
        mov before,esp
        push arg
        mov ecx,object
        mov eax,fn
        call eax
        mov after,esp
    }
    return before==after;
}
struct FakeEntity {
    std::array<std::uint8_t,0x2E8u> unknown{};
    std::uint8_t predictable=0u;
    std::uint8_t reconcile_result=0u;
    unsigned id=0u;
};
static_assert(offsetof(FakeEntity,predictable)==0x2E8u,
              "native original C_BaseEntity IsPredictable field moved");
struct Trace {
    std::array<FakeEntity,4> entities{};
    std::array<FakeEntity*,5> slots{};
    std::array<char,128> sequence{};
    std::array<std::uint32_t,8> forwarded{};
    unsigned len=0u;
    unsigned get_calls=0u;
    unsigned count_calls=0u;
    unsigned is_predictable_calls=0u;
    unsigned reconciliation_calls=0u;
    unsigned enter_calls=0u;
    unsigned exit_calls=0u;
    unsigned opaque_entry_calls=0u;
    unsigned opaque_tail_calls=0u;
    std::int32_t reported_count=5;
    void append(char c) noexcept {
        if(len<sequence.size())sequence[len++]=c;
    }
    bool matches(const char* x,unsigned n) const noexcept {
        if(len!=n)return false;
        for(unsigned i=0u;i<n;++i)
            if(sequence[i]!=x[i])return false;
        return true;
    }
    void reset() noexcept {
        len=0u;sequence.fill(0);
        forwarded.fill(0);
        get_calls=0u;count_calls=0u;
        is_predictable_calls=0u;reconciliation_calls=0u;
        enter_calls=0u;exit_calls=0u;
        opaque_entry_calls=0u;opaque_tail_calls=0u;
    }
};
std::int32_t get_count(void* context) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->append('C');++t->count_calls;
    return t->reported_count;
}
void* get_entity(void* context,std::int32_t i) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->append('g');++t->get_calls;
    if(i<0 || i>=static_cast<int>(t->slots.size()))
        return nullptr;
    return t->slots[static_cast<std::size_t>(i)];
}
bool is_predictable(void* context,void* raw) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->append('p');++t->is_predictable_calls;
    return static_cast<FakeEntity*>(raw)->predictable!=0u;
}
std::uint8_t reconcile(void* context,void* raw,
                       std::uint32_t frame) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->append('r');
    if(t->reconciliation_calls<t->forwarded.size())
        t->forwarded[t->reconciliation_calls]=frame;
    ++t->reconciliation_calls;
    return static_cast<FakeEntity*>(raw)->reconcile_result;
}
void on_entry(void* context) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->append('B');++t->opaque_entry_calls;
}
void on_tail(void* context) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->append('T');++t->opaque_tail_calls;
}
void on_scope_enter(void* context,unsigned slot) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->append(slot==6u?'E':'?');++t->enter_calls;
}
void on_scope_exit(void* context,unsigned slot) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->append(slot==6u?'X':'?');++t->exit_calls;
}
pred::PostNetworkOps make_ops(Trace& t) noexcept {
    pred::PostNetworkOps ops{};
    ops.user=&t;
    ops.local_player_present=true;
    ops.cl_predict=1;
    ops.cl_pred_optimize=2; // read by retail, semantics still opaque
    ops.get_count=&get_count;
    ops.get_entity=&get_entity;
    ops.is_predictable=&is_predictable;
    ops.reconcile_entity=&reconcile;
    ops.on_opaque_entry=&on_entry;
    ops.on_opaque_remaining_branches=&on_tail;
    ops.scope_enter=&on_scope_enter;
    ops.scope_exit=&on_scope_exit;
    return ops;
}
} // namespace
int main() {
    pred::events={};
    auto prediction=std::make_unique<pred::Prediction23Fixture>(nullptr);
    auto* owner=prediction.get();
    auto** table=*reinterpret_cast<void***>(owner);
    if(!table[6] || table[6]==table[5] || table[6]==table[7])
        return fail(1,"original prediction slot6 x86 vtable entry missing");
    const auto base=reinterpret_cast<std::uintptr_t>(owner);
    if(prediction->value_b_address()-base!=
       pred::kOriginalPostNetworkInputMemberOffset ||
       prediction->postnetwork_any_reconciled_address()-base!=
       pred::kOriginalPostNetworkAnyReconciledMemberOffset)
        return fail(2,"native prediction +0x2C/+0x30 layout changed");
    // All original-free host pointers must be after the reserved
    // real original +0x30 DWORD, so PostNetwork does not clobber
    // our Phase1659 original-free HostSnapshot pointer.
    pred::HostSnapshot host{};
    prediction->set_view_host(&host);
    Trace t{};
    t.entities[0].predictable=1u;
    t.entities[0].reconcile_result=0u;
    t.entities[1].predictable=0u;
    t.entities[2].predictable=0xFFu;
    t.entities[2].reconcile_result=1u;
    t.entities[3].predictable=1u;
    t.entities[3].reconcile_result=0u;
    t.slots={{&t.entities[0],nullptr,&t.entities[1],
              &t.entities[2],&t.entities[3]}};
    auto ops=make_ops(t);
    prediction->set_postnetwork_ops(&ops);
    const auto input=0xA1B2C3D4u;
    if(!invoke_one(owner,table[6],input) ||
       prediction->value_b()!=input ||
       prediction->postnetwork_any_reconciled()!=1u ||
       t.count_calls!=1u || t.get_calls!=5u ||
       t.is_predictable_calls!=4u || t.reconciliation_calls!=3u ||
       t.opaque_entry_calls!=1u || t.opaque_tail_calls!=1u ||
       t.enter_calls!=1u || t.exit_calls!=1u ||
       !t.matches("EBCgprggpgprgprTX",17u) ||
       t.forwarded[0]!=input || t.forwarded[1]!=input ||
       t.forwarded[2]!=input ||
       pred::events.postnetwork_reconcile_calls!=3u ||
       pred::events.postnetwork_unmodeled_tail_calls!=1u)
        return fail(3,"real slot6 RET4, bounded count/get, eligible call/flag");
    // Source-authenticated x86 member+0x30 stores only a 1/0.
    // The local continuation ALWAYS reaches an opaque tail where
    // retail game diagnostic/correction methods remain unresolved.
    t.reset();
    t.entities[2].reconcile_result=0u;
    if(!invoke_one(owner,table[6],0xFFFFFFFFu) ||
       prediction->value_b()!=0xFFFFFFFFu ||
       prediction->postnetwork_any_reconciled()!=0u ||
       t.reconciliation_calls!=3u ||
       t.forwarded[0]!=0xFFFFFFFFu ||
       !t.matches("EBCgprggpgprgprTX",17u))
        return fail(4,"all predicted entities returned false, raw frame bits");
    // No original local C_BasePlayer: entry bookkeeping still
    // executes but no entity-list vcall occurs.
    ops.local_player_present=false;
    t.reset();
    if(!invoke_one(owner,table[6],9u) ||
       prediction->value_b()!=9u ||
       prediction->postnetwork_any_reconciled()!=0u ||
       t.count_calls!=0u || !t.matches("EBTX",4u))
        return fail(5,"absent local-player guard still writes entry members");
    // cl_predict=0 similarly skips reconciliation, without
    // claiming to implement the unknown retail disabled branch.
    ops.local_player_present=true;
    ops.cl_predict=0;
    t.reset();
    if(!invoke_one(owner,table[6],10u) ||
       prediction->value_b()!=10u ||
       t.count_calls!=0u || !t.matches("EBTX",4u))
        return fail(6,"cl_predict disabled must not enumerate entities");
    ops.cl_predict=1;
    t.reported_count=0;
    t.reset();
    if(!invoke_one(owner,table[6],11u) ||
       !t.matches("EBCTX",5u) || t.get_calls!=0u)
        return fail(7,"empty client entity list unexpectedly enumerated");
    const auto refused_count=pred::events.refused_postnetwork_entity_count;
    t.reported_count=-2;
    t.reset();
    if(!invoke_one(owner,table[6],12u) ||
       !t.matches("EBCTX",5u) ||
       pred::events.refused_postnetwork_entity_count!=refused_count+1u)
        return fail(8,"negative entity count not safely refused");
    t.reported_count=4097;
    t.reset();
    if(!invoke_one(owner,table[6],13u) ||
       !t.matches("EBCTX",5u) ||
       pred::events.refused_postnetwork_entity_count!=refused_count+2u)
        return fail(9,"oversized entity list was not bounded");
    t.reported_count=5;
    const auto refused_backend=pred::events.refused_postnetwork_backend;
    ops.reconcile_entity=nullptr;
    t.reset();
    if(!invoke_one(owner,table[6],14u) ||
       !t.matches("EBTX",4u) ||
       pred::events.refused_postnetwork_backend!=refused_backend+1u)
        return fail(10,"missing opaque reconcile entity callback accessed");
    prediction->set_postnetwork_ops(nullptr);
    t.reset();
    const auto prev_missing=pred::events.missing_postnetwork_context;
    const auto previous_frame=prediction->value_b();
    if(!invoke_one(owner,table[6],15u) ||
       pred::events.missing_postnetwork_context!=prev_missing+1u ||
       prediction->value_b()!=previous_frame || t.len!=0u)
        return fail(11,"detached original-free PostNetwork owner not refused");
    prediction.reset();
    if(pred::events.destructor_calls!=1u)
        return fail(12,"postnetwork synthetic prediction lifetime drift");
    std::puts("PASS Phase1668 MSVC x86 original-free: actual CPrediction "
              "vtable[6] PostNetworkDataReceived one-arg RET4, "
              "source-authenticated +0x2C frame and +0x30 any-reconciled "
              "word, no-player/cl_predict gates, one Count then ascending "
              "Get(index) and +0x2E8 predictable byte, callback "
              "0x240C6900(frame) Boolean result, no false assertion "
              "about later 1039-byte opaque diagnostic/correction "
              "branches, unsafe count/backend refusals, clean teardown");
    return 0;
}
