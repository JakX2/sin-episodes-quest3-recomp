// Phase1665: original-free native Win32 x86 prediction packet lifecycle.
// Actual MSVC 23-slot prediction vtable at entries 4/5, C_BaseEntity
// predictable flag byte +0x2E8, original count/get enumeration ordering.
//
// The entities, counts, ConVar and local-player signals are invented and
// owned by this test. Never loads or runs any original SiN DLL/engine.
#include "native_client_prediction23_init_probe.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1665 packet entity lifecycle ABI test requires native MSVC x86
#endif
namespace pred=sin_independent::client_prediction23_diagnostic;
namespace {
int fail(int code,const char* why) {
    std::fprintf(stderr,
       "FAIL Phase1665 native original-free prediction packet slots4/5 "
       "#%d: %s\n",code,why);
    return code;
}
bool invoke_two(void* object,void* fn,
                std::uint32_t arg1,std::uint32_t arg2) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm {
        mov before,esp
        push arg2
        push arg1
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
bool invoke_getter(void* object,void* fn,std::uint32_t& result) noexcept {
    std::uintptr_t before=0u,after=0u;
    std::uint32_t value=0u;
    __asm {
        mov before,esp
        mov ecx,object
        mov eax,fn
        call eax
        mov value,eax
        mov after,esp
    }
    result=value;
    return before==after;
}
struct FakeEntity {
    std::array<std::uint8_t,0x2E8u> unknown{};
    std::uint8_t is_predictable=0u;
};
static_assert(offsetof(FakeEntity,is_predictable)==0x2E8u,
              "original C_BaseEntity predictable byte offset differs");
struct Trace {
    std::array<FakeEntity,4> entities{};
    std::array<FakeEntity*,5> slots{};
    std::array<char,128> events{};
    std::array<std::uint32_t,5> pre_values{};
    unsigned size=0u;
    unsigned pre_count=0u;
    unsigned post_count=0u;
    unsigned count_calls=0u;
    unsigned get_calls=0u;
    unsigned predicate_calls=0u;
    unsigned fallback_calls=0u;
    std::int32_t reported_count=5;
    bool bad_scope=false;
    void append(char c) noexcept {
        if(size<events.size())events[size++]=c;
    }
    void reset() noexcept {
        size=0u;
        events.fill(0);
        pre_count=0u;
        post_count=0u;
        count_calls=0u;
        get_calls=0u;
        predicate_calls=0u;
        fallback_calls=0u;
        bad_scope=false;
    }
    bool matches(const char* x,unsigned n) const noexcept {
        if(size!=n)return false;
        for(unsigned i=0u;i<n;++i)
            if(events[i]!=x[i])return false;
        return true;
    }
};
std::int32_t count_entities(void* context) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->append('C');
    ++t->count_calls;
    return t->reported_count;
}
void* indexed_entity(void* context,std::int32_t index) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->append('g');
    ++t->get_calls;
    if(index<0 || index>=static_cast<std::int32_t>(t->slots.size()))
        return nullptr;
    return t->slots[static_cast<std::size_t>(index)];
}
bool predictable(void* context,void* entity) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->append('f');
    ++t->predicate_calls;
    auto* e=static_cast<FakeEntity*>(entity);
    return e && e->is_predictable!=0u;
}
void on_pre(void* context,void* entity,std::uint32_t a1) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->append('P');
    if(t->pre_count<t->pre_values.size())
        t->pre_values[t->pre_count]=a1;
    ++t->pre_count;
    if(!static_cast<FakeEntity*>(entity)->is_predictable)
        t->bad_scope=true;
}
void on_post(void* context,void* entity) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->append('Q');
    ++t->post_count;
    if(!static_cast<FakeEntity*>(entity)->is_predictable)
        t->bad_scope=true;
}
void on_disabled_pre(void* context) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->append('F');
    ++t->fallback_calls;
}
void on_enter(void* context,unsigned slot) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->append(slot==4u?'E':slot==5u?'e':'?');
    if(slot!=4u && slot!=5u)t->bad_scope=true;
}
void on_exit(void* context,unsigned slot) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->append(slot==4u?'X':slot==5u?'x':'?');
    if(slot!=4u && slot!=5u)t->bad_scope=true;
}
pred::PacketEntityOps make_ops(Trace& t) noexcept {
    pred::PacketEntityOps ops{};
    ops.user=&t;
    ops.cl_predict=1;
    ops.local_player_present=true;
    ops.get_count=&count_entities;
    ops.get_entity=&indexed_entity;
    ops.is_predictable=&predictable;
    ops.pre_entity=&on_pre;
    ops.post_entity=&on_post;
    ops.pre_disabled_fallback=&on_disabled_pre;
    ops.scope_enter=&on_enter;
    ops.scope_exit=&on_exit;
    return ops;
}
} // namespace
int main() {
    pred::events={};
    auto prediction=std::make_unique<pred::Prediction23Fixture>(nullptr);
    auto* owner=prediction.get();
    auto** table=*reinterpret_cast<void***>(owner);
    if(!table[4] || !table[5] || table[4]==table[5])
        return fail(1,"native CPrediction packet virtual indices 4/5 missing");
    if(owner->memberC_address()-reinterpret_cast<std::uintptr_t>(owner)!=
       pred::kOriginalPacketLastArg2MemberOffset)
        return fail(2,"prediction packet arg2 store this+0x0C offset differs");

    Trace trace{};
    trace.entities[0].is_predictable=1u;
    trace.entities[1].is_predictable=0u;
    trace.entities[2].is_predictable=0xFFu;
    trace.entities[3].is_predictable=1u;
    trace.slots={{&trace.entities[0],nullptr,&trace.entities[1],
                  &trace.entities[2],&trace.entities[3]}};
    auto ops=make_ops(trace);
    prediction->set_packet_entity_ops(&ops);
    const std::uint32_t forwarded=0xA1B2C3D4u;
    const std::uint32_t stored=0x55667788u;
    std::uint32_t actual_member=0u;
    if(!invoke_two(owner,table[4],forwarded,stored) ||
       !invoke_getter(owner,table[17],actual_member) ||
       actual_member!=stored ||
       !trace.matches("ECgfPggfgfPgfPX",15u) ||
       trace.bad_scope || trace.count_calls!=1u ||
       trace.get_calls!=5u || trace.predicate_calls!=4u ||
       trace.pre_count!=3u || trace.post_count!=0u ||
       trace.pre_values[0]!=forwarded ||
       trace.pre_values[1]!=forwarded ||
       trace.pre_values[2]!=forwarded)
        return fail(3,"native slot4 arg mapping, count/get, IsPredictable or RET8");

    trace.reset();
    if(!invoke_zero(owner,table[5]) ||
       !trace.matches("eCgfQggfgfQgfQx",15u) ||
       trace.bad_scope || trace.count_calls!=1u ||
       trace.get_calls!=5u || trace.predicate_calls!=4u ||
       trace.post_count!=3u || trace.pre_count!=0u)
        return fail(4,"native slot5 eligible callbacks, profile exit RET0");

    // With cl_predict disabled, Pre calls an opaque original fallback
    // 0x24195720, but Post DOES NOT. Both must still profile enter/exit.
    ops.cl_predict=0;
    trace.reset();
    if(!invoke_two(owner,table[4],0xAAu,0xBBu) ||
       !invoke_getter(owner,table[17],actual_member) ||
       actual_member!=0xBBu ||
       !trace.matches("EFX",3u) ||
       trace.fallback_calls!=1u || trace.count_calls!=0u)
        return fail(5,"Pre disabled ConVar fallback or arg2 store drift");
    trace.reset();
    if(!invoke_zero(owner,table[5]) ||
       !trace.matches("ex",2u) ||
       trace.fallback_calls!=0u || trace.count_calls!=0u)
        return fail(6,"Post disabled ConVar must no-op before entity count");

    // Enabled but no local C_BasePlayer: both return without count.
    ops.cl_predict=1;
    ops.local_player_present=false;
    trace.reset();
    if(!invoke_two(owner,table[4],0x11u,0x22u) ||
       !trace.matches("EX",2u) || trace.count_calls!=0u)
        return fail(7,"Pre no current local player must skip entities");
    trace.reset();
    if(!invoke_zero(owner,table[5]) ||
       !trace.matches("ex",2u) || trace.count_calls!=0u)
        return fail(8,"Post no current local player must skip entities");

    ops.local_player_present=true;
    trace.reported_count=0;
    trace.reset();
    if(!invoke_two(owner,table[4],1u,2u) ||
       !trace.matches("ECX",3u) || trace.count_calls!=1u ||
       trace.get_calls!=0u)
        return fail(9,"empty entity list must not request elements");
    trace.reset();
    if(!invoke_zero(owner,table[5]) ||
       !trace.matches("eCx",3u) || trace.get_calls!=0u)
        return fail(10,"Post empty entity list unexpectedly enumerated");

    // Original loop skips a negative count naturally. The original-free
    // fixture explicitly records refusal for invalid/huge counts.
    const unsigned before_bad=pred::events.refused_packet_entity_count;
    trace.reported_count=-1;
    trace.reset();
    if(!invoke_two(owner,table[4],1u,2u) ||
       !trace.matches("ECX",3u) ||
       pred::events.refused_packet_entity_count!=before_bad+1u)
        return fail(11,"negative list size was not rejected by safe fixture");
    trace.reported_count=4100;
    trace.reset();
    if(!invoke_zero(owner,table[5]) ||
       !trace.matches("eCx",3u) ||
       pred::events.refused_packet_entity_count!=before_bad+2u)
        return fail(12,"unbounded entity count was not safely capped");

    // No original CClientEntityList callbacks are guessed: missing
    // backend fails closed rather than dereferencing a function pointer.
    ops.local_player_present=true;
    ops.cl_predict=1;
    ops.post_entity=nullptr;
    trace.reset();
    const unsigned before_backend=pred::events.refused_packet_entity_backend;
    if(!invoke_zero(owner,table[5]) ||
       !trace.matches("ex",2u) ||
       pred::events.refused_packet_entity_backend!=before_backend+1u)
        return fail(13,"missing entity callback backend not safely refused");

    prediction->set_packet_entity_ops(nullptr);
    trace.reset();
    const unsigned before_missing=pred::events.missing_packet_context;
    if(!invoke_two(owner,table[4],8u,9u) ||
       !invoke_zero(owner,table[5]) ||
       pred::events.missing_packet_context!=before_missing+2u ||
       trace.size!=0u)
        return fail(14,"retired packet context unexpectedly invoked callbacks");
    prediction.reset();
    if(pred::events.destructor_calls!=1u)
        return fail(15,"synthetic prediction instance lifetime drift");

    std::puts("PASS Phase1665 original-free MSVC x86: CPrediction "
              "slots4 PRE 2args RET8 /5 POST 0args RET0, exact "
              "this+0xC argument2, per-eligible argument1, 2006 "
              "entity-list Count(vptr+4) then Get(vptr+0), "
              "C_BaseEntity +0x2E8 predictable flag, "
              "cl_predict disabled Pre-only fallback, no-player, "
              "empty, null entities, negative/huge count safety, "
              "missing callback refusal and detached ownership");
    return 0;
}
