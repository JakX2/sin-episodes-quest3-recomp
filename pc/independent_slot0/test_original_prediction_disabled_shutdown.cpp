// Phase1666: actual MSVC Win32 x86 disabled-cl_predict prediction
// packet -> original-free 118-byte fallback behavioral contract.
//
// Host-created entities, an invented CClientEntityList and a real
// synthetic TWO-slot embedded vtable at entity+8 check the original
// predictable versus client-created cleanup split. No retail DLL load.
#include "native_original_prediction_disabled_shutdown.h"
#include "native_client_prediction23_init_probe.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1666 disabled prediction shutdown requires actual MSVC x86
#endif
namespace shut=sin_independent::original_prediction_disabled_shutdown;
namespace pred=sin_independent::client_prediction23_diagnostic;
namespace {
int fail(int n,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1666 native original-free disabled prediction "
      "entity shutdown #%d: %s\n",n,why);
    return n;
}
struct Trace;
struct EmbeddedController {
    virtual std::uint32_t __thiscall Slot0() noexcept {
        return 0xA0000131u;
    }
    virtual void __thiscall ShutdownClientCreated() noexcept;
    Trace* trace=nullptr;
    unsigned id=0u;
};
static_assert(sizeof(EmbeddedController)==12u,
              "native synthetic client-created embedded vptr is not x86");
struct Entity {
    std::array<std::uint8_t,8u> initial{};
    EmbeddedController embedded{};
    std::array<std::uint8_t,0x2E8u-8u-sizeof(EmbeddedController)> middle{};
    std::uint8_t predictable=0u;
    unsigned id=0u;
};
struct Trace {
    std::array<Entity,5> all{};
    std::array<Entity*,5> slots{};
    std::array<char,128> events{};
    std::array<std::int32_t,10> indices{};
    unsigned size=0u;
    unsigned visit_count=0u;
    unsigned predictable_shutdowns=0u;
    unsigned created_shutdowns=0u;
    unsigned summary_calls=0u;
    unsigned summary_predictable=0u;
    unsigned summary_created=0u;
    unsigned backend_count_calls=0u;
    std::int32_t reported_count=5;
    bool bad_virtual_abi=false;
    shut::Result last{};
    void emit(char c) noexcept {
        if(size<events.size())events[size++]=c;
    }
    bool matches(const char* x,unsigned n) const noexcept {
        if(size!=n)return false;
        for(unsigned i=0u;i<n;++i)
            if(events[i]!=x[i])return false;
        return true;
    }
    void reset() noexcept {
        size=0u; events.fill(0);
        visit_count=0u; indices.fill(-1);
        predictable_shutdowns=0u;created_shutdowns=0u;
        summary_calls=0u;summary_predictable=0u;summary_created=0u;
        backend_count_calls=0u;bad_virtual_abi=false;
    }
};
void EmbeddedController::ShutdownClientCreated() noexcept {
    if(trace) {
        trace->emit('C');
        ++trace->created_shutdowns;
        if(id!=2u && id!=4u)trace->bad_virtual_abi=true;
    }
}
bool invoke_two(void* object,void* fn,
                std::uint32_t a1,std::uint32_t a2) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm {
        mov before,esp
        push a2
        push a1
        mov ecx,object
        mov eax,fn
        call eax
        mov after,esp
    }
    return before==after;
}
bool invoke_zero(void* object,void* fn) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm {
        mov before,esp
        mov ecx,object
        mov eax,fn
        call eax
        mov after,esp
    }
    return before==after;
}
std::int32_t count(void* context) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->emit('N');
    ++t->backend_count_calls;
    return t->reported_count;
}
void* indexed(void* context,std::int32_t index) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->emit('g');
    if(t->visit_count<t->indices.size())
        t->indices[t->visit_count++]=index;
    if(index<0 || index>=static_cast<std::int32_t>(t->slots.size()))
        return nullptr;
    return t->slots[static_cast<std::size_t>(index)];
}
bool predictable(void* context,void* e) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->emit('f');
    auto* entity=static_cast<Entity*>(e);
    return entity && entity->predictable!=0u;
}
void shutdown_predictable(void* context,void* e) noexcept {
    auto* t=static_cast<Trace*>(context);
    auto* entity=static_cast<Entity*>(e);
    t->emit('P');
    ++t->predictable_shutdowns;
    if(!entity->predictable || (entity->id!=0u && entity->id!=3u))
        t->bad_virtual_abi=true;
}
void shutdown_created(void* context,void* e) noexcept {
    auto* t=static_cast<Trace*>(context);
    auto* entity=static_cast<Entity*>(e);
    auto* ctrl=&entity->embedded;
    if(reinterpret_cast<std::uintptr_t>(ctrl)-
       reinterpret_cast<std::uintptr_t>(entity)!=
       shut::kOriginalClientCreatedEmbeddedVptrOffset)
        t->bad_virtual_abi=true;
    auto** vf=*reinterpret_cast<void***>(ctrl);
    void* fn=vf[shut::kOriginalClientCreatedShutdownSlot];
    std::uintptr_t before=0u,after=0u;
    __asm {
        mov before,esp
        mov ecx,ctrl
        mov eax,fn
        call eax
        mov after,esp
    }
    if(before!=after)t->bad_virtual_abi=true;
}
void summary(void* context,unsigned predictable_count,
             unsigned created_count) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->emit('S');
    ++t->summary_calls;
    t->summary_predictable=predictable_count;
    t->summary_created=created_count;
}
shut::Ops make_ops(Trace& t) noexcept {
    shut::Ops ops{};
    ops.user=&t;
    ops.count=&count;
    ops.get=&indexed;
    ops.predictable=&predictable;
    ops.shutdown_predictable=&shutdown_predictable;
    ops.shutdown_client_created=&shutdown_created;
    ops.print_summary=&summary;
    return ops;
}
void on_disabled_pre(void* context) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->last=shut::Run(make_ops(*t));
}
void on_packet_enter(void* context,unsigned slot) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->emit(slot==4u?'E':'e');
}
void on_packet_exit(void* context,unsigned slot) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->emit(slot==4u?'X':'x');
}
} // namespace
int main() {
    pred::events={};
    Trace trace{};
    for(unsigned i=0u;i<trace.all.size();++i) {
        trace.all[i].id=i;
        trace.all[i].embedded.trace=&trace;
        trace.all[i].embedded.id=i;
    }
    // reverse traversal order 4(C),3(P),2(C),1(NULL),0(P).
    trace.all[0].predictable=1u;
    trace.all[2].predictable=0u;
    trace.all[3].predictable=0xFFu;
    trace.all[4].predictable=0u;
    trace.slots={{&trace.all[0],nullptr,&trace.all[2],
                  &trace.all[3],&trace.all[4]}};
    if(reinterpret_cast<std::uintptr_t>(&trace.all[0].predictable)-
       reinterpret_cast<std::uintptr_t>(&trace.all[0])!=
       shut::kOriginalPredictableFieldOffset)
        return fail(1,"C_BaseEntity +0x2E8 predictable flag offset drift");
    auto model=make_ops(trace);
    trace.last=shut::Run(model);
    if(trace.last.status!=shut::Status::Complete ||
       trace.last.entity_gets!=5u ||
       trace.last.null_entities!=1u ||
       trace.last.predictable_shutdowns!=2u ||
       trace.last.client_created_shutdowns!=2u ||
       !trace.last.printed_summary ||
       trace.backend_count_calls!=1u ||
       trace.predictable_shutdowns!=2u ||
       trace.created_shutdowns!=2u ||
       trace.summary_calls!=1u ||
       trace.summary_predictable!=2u ||
       trace.summary_created!=2u ||
       trace.bad_virtual_abi ||
       !trace.matches("NgfCgfPgfCggfPS",15u))
        return fail(2,"reverse two-category shutdown, embedded x86 vptr or summary");
    for(unsigned i=0u;i<5u;++i)
        if(trace.indices[i]!=4-static_cast<std::int32_t>(i))
            return fail(3,"entity order should run descending index 4..0");

    // A wholly empty list must never print the original two counts.
    trace.reset();
    trace.reported_count=0;
    trace.last=shut::Run(model);
    if(trace.last.status!=shut::Status::Complete ||
       trace.last.entity_gets!=0u ||
       trace.last.printed_summary ||
       !trace.matches("N",1u) || trace.summary_calls!=0u)
        return fail(4,"Count=0 should do no Get and no summary");
    trace.reset();
    trace.reported_count=5;
    const auto saved=trace.slots;
    trace.slots.fill(nullptr);
    trace.last=shut::Run(model);
    if(trace.last.status!=shut::Status::Complete ||
       trace.last.null_entities!=5u || trace.last.printed_summary ||
       trace.summary_calls!=0u || trace.visit_count!=5u)
        return fail(5,"all-null entities must not produce summary");
    trace.slots=saved;
    trace.reset();
    trace.reported_count=-2;
    trace.last=shut::Run(model);
    if(trace.last.status!=shut::Status::UnsafeCount ||
       trace.visit_count!=0u || trace.summary_calls!=0u ||
       !trace.matches("N",1u))
        return fail(6,"negative entity count must safely refuse enumeration");
    trace.reset();
    trace.reported_count=4097;
    trace.last=shut::Run(model);
    if(trace.last.status!=shut::Status::UnsafeCount ||
       trace.visit_count!=0u || !trace.matches("N",1u))
        return fail(7,"oversized synthetic entity count not refused");
    trace.reported_count=5;
    model.get=nullptr;
    trace.reset();
    trace.last=shut::Run(model);
    if(trace.last.status!=shut::Status::MissingBackend ||
       trace.size!=0u)
        return fail(8,"missing host get callback must fail before Count");

    // Connect exact source-authenticated PRE packet slot4 disabled
    // branch to the 118-byte fallback model. The same 23-slot
    // prediction fixture is actually invoked via its native vptr.
    auto prediction=std::make_unique<pred::Prediction23Fixture>(nullptr);
    auto* owner=prediction.get();
    auto** table=*reinterpret_cast<void***>(owner);
    if(!table[4] || !table[5] || !table[17])
        return fail(9,"original prediction packet virtual slots missing");
    pred::PacketEntityOps packet{};
    packet.user=&trace;
    packet.cl_predict=0;
    packet.local_player_present=false; // disabled fallback ignores this
    packet.pre_disabled_fallback=&on_disabled_pre;
    packet.scope_enter=&on_packet_enter;
    packet.scope_exit=&on_packet_exit;
    prediction->set_packet_entity_ops(&packet);
    trace.reset();
    if(!invoke_two(owner,table[4],0x1234ABCDu,0xA1B2C3D4u) ||
       trace.last.status!=shut::Status::Complete ||
       !trace.matches("ENgfCgfPgfCggfPSX",17u) ||
       trace.last.predictable_shutdowns!=2u ||
       trace.last.client_created_shutdowns!=2u ||
       trace.bad_virtual_abi)
        return fail(10,"Pre packet cl_predict-disabled fallback not exact");
    trace.reset();
    if(!invoke_zero(owner,table[5]) ||
       !trace.matches("ex",2u) ||
       trace.backend_count_calls!=0u ||
       trace.summary_calls!=0u)
        return fail(11,"Post packet disabled must NOT invoke cleanup fallback");

    // Pre with cl_predict ENABLED but NO local player does NOT
    // call fallback. Both packet methods must skip entity ops.
    packet.cl_predict=1;
    trace.reset();
    if(!invoke_two(owner,table[4],7u,8u) ||
       !trace.matches("EX",2u) ||
       trace.backend_count_calls!=0u ||
       trace.summary_calls!=0u)
        return fail(12,"Pre enabled/no-player should not clean entities");
    prediction->set_packet_entity_ops(nullptr);
    trace.reset();
    const unsigned before_missing=pred::events.missing_packet_context;
    if(!invoke_two(owner,table[4],2u,3u) ||
       pred::events.missing_packet_context!=before_missing+1u ||
       trace.size!=0u)
        return fail(13,"detached original-free packet callback invoked");
    prediction.reset();
    if(pred::events.destructor_calls!=1u)
        return fail(14,"original-free prediction destruction count drift");

    std::puts("PASS Phase1666 original-free MSVC x86: 118-byte "
              "disabled-cl_predict Pre packet fallback, Count once, "
              "reverse Get(index) 4..0, C_BaseEntity +0x2E8 flag, "
              "predictable direct cleanup vs nonpredictable synthetic "
              "entity+8 embedded 2-slot __thiscall vptr+4 shutdown, "
              "two separate counters and one conditional logger, "
              "native prediction Pre RET8/Post RET0 gating, safety "
              "negative/huge/null count and missing backend, clean "
              "callback teardown; zero retail native invocations");
    return 0;
}
