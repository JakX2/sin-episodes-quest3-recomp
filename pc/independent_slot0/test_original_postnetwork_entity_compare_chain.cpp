// Phase1669: source-authentic original-free nested x86 prediction
// PostNetworkDataReceived -> C_BaseEntity comparison Boolean boundary.
//
// Calls ACTUAL compiler-generated 23-slot Prediction vtable[6] and
// 17-slot synthetic C_BaseEntity primary vtable[16]. Models only
// original signed frame>0 / (frame-1)%90 history / opaque prepare +
// difference count >0, never unknown retail engine helper bodies.
#include "native_original_postnetwork_entity_compare.h"
#include "native_client_prediction23_init_probe.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1669 requires actual native MSVC x86
#endif
namespace per=sin_independent::original_postnetwork_entity_compare;
namespace pred=sin_independent::client_prediction23_diagnostic;
namespace {
int fail(int code,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1669 original-free nested PostNetwork x86 #%d: %s\n",
      code,why);
    return code;
}
class CBaseEntity17Fixture {
public:
#define SIN_OPAQUE_ENTITY_SLOT(N) \
    virtual std::uint32_t __thiscall Unknown##N() noexcept { \
        return 0xD0000000u+static_cast<std::uint32_t>(N); \
    }
    SIN_OPAQUE_ENTITY_SLOT(0)
    SIN_OPAQUE_ENTITY_SLOT(1)
    SIN_OPAQUE_ENTITY_SLOT(2)
    SIN_OPAQUE_ENTITY_SLOT(3)
    SIN_OPAQUE_ENTITY_SLOT(4)
    SIN_OPAQUE_ENTITY_SLOT(5)
    SIN_OPAQUE_ENTITY_SLOT(6)
    SIN_OPAQUE_ENTITY_SLOT(7)
    SIN_OPAQUE_ENTITY_SLOT(8)
    SIN_OPAQUE_ENTITY_SLOT(9)
    SIN_OPAQUE_ENTITY_SLOT(10)
    SIN_OPAQUE_ENTITY_SLOT(11)
    SIN_OPAQUE_ENTITY_SLOT(12)
    SIN_OPAQUE_ENTITY_SLOT(13)
    SIN_OPAQUE_ENTITY_SLOT(14)
    SIN_OPAQUE_ENTITY_SLOT(15)
#undef SIN_OPAQUE_ENTITY_SLOT
    // Original primary virtual at byte+0x40, index16.
    // The full retail virtual implementation is opaque. This is
    // merely a real compiler-generated pointer-return ABI.
    virtual const void* __thiscall GetOpaqueSource() noexcept {
        return &source_;
    }
    std::uintptr_t predictable_addr() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&predictable_);
    }
    std::uintptr_t history_addr() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&history_);
    }
    std::uintptr_t active_addr() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&active_);
    }
    void set_predictable(std::uint8_t b) noexcept {predictable_=b;}
    bool predictable() const noexcept {return predictable_!=0u;}
    void set_history_active(bool active) noexcept {
        active_=active?1u:0u;
    }
    bool history_active() const noexcept {return active_!=0u;}
    std::array<std::uint32_t,90>& history() noexcept {return history_;}
    void set_difference(std::int32_t n) noexcept {difference_=n;}
    std::int32_t difference() const noexcept {return difference_;}
    const void* source() const noexcept {return &source_;}
private:
    // The native x86 4-byte vfptr occupies this+0..+3.
    std::array<std::uint8_t,0x2E8u-4u> opaque04to2E8_{};
    std::uint8_t predictable_=0u; // +0x2E8
    std::array<std::uint8_t,3u> padding2E9to2EC_{};
    std::array<std::uint32_t,90> history_{}; // +0x2EC..+0x450
    std::uint32_t active_=0u; // +0x454
    std::uint32_t source_=0xA1B2C3D4u;
    std::int32_t difference_=0;
};

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
bool invoke_zero_result(void* object,void* fn,
                        const void*& result) noexcept {
    std::uintptr_t before=0u,after=0u;
    const void* out=nullptr;
    __asm {
        mov before,esp
        mov ecx,object
        mov eax,fn
        call eax
        mov out,eax
        mov after,esp
    }
    result=out;
    return before==after;
}
struct Trace {
    std::array<char,128> events{};
    unsigned len=0u;
    unsigned tags=0u;
    unsigned prepares=0u;
    unsigned getters=0u;
    unsigned differences=0u;
    unsigned wrong_abi=0u;
    std::int32_t last_frame=0;
    std::uint32_t last_history_word=0u;
    per::Result last{};
    void emit(char c) noexcept {
        if(len<events.size())events[len++]=c;
    }
    bool matches(const char* x,unsigned n) const noexcept {
        if(len!=n)return false;
        for(unsigned i=0u;i<n;++i)
            if(events[i]!=x[i])return false;
        return true;
    }
    void reset() noexcept {
        len=0u;events.fill(0);
        tags=0u;prepares=0u;getters=0u;
        differences=0u;wrong_abi=0u;
        last_frame=0;last_history_word=0u;
    }
};
void on_tag(void* user,void* object,std::int32_t frame) noexcept {
    auto* t=static_cast<Trace*>(user);
    t->emit('t');++t->tags;t->last_frame=frame;
    if(!object)++t->wrong_abi;
}
void on_prepare(void* user,void* object,
                std::int32_t frame,std::uint32_t history) noexcept {
    auto* t=static_cast<Trace*>(user);
    t->emit('p');++t->prepares;
    t->last_frame=frame;t->last_history_word=history;
    if(!object)++t->wrong_abi;
}
const void* on_get_virtual16(void* user,void* object) noexcept {
    auto* t=static_cast<Trace*>(user);
    t->emit('v');++t->getters;
    auto** table=*reinterpret_cast<void***>(object);
    const void* out=nullptr;
    if(!table[per::kOriginalPrimaryEntityVirtualSlot] ||
       !invoke_zero_result(object,
          table[per::kOriginalPrimaryEntityVirtualSlot],out))
        ++t->wrong_abi;
    return out;
}
std::int32_t on_difference(void* user,void* object,
                           const void* source) noexcept {
    auto* t=static_cast<Trace*>(user);
    t->emit('d');++t->differences;
    auto* entity=static_cast<CBaseEntity17Fixture*>(object);
    if(source!=entity->source())++t->wrong_abi;
    return entity->difference();
}
per::Ops make_ops(Trace& t,CBaseEntity17Fixture& e) noexcept {
    per::Ops ops{};
    ops.user=&t;
    ops.history=&e.history();
    ops.history_active=e.history_active();
    ops.on_tag=&on_tag;
    ops.on_prepare=&on_prepare;
    ops.get_virtual16=&on_get_virtual16;
    ops.difference_count=&on_difference;
    return ops;
}
struct PacketFixture {
    Trace* trace=nullptr;
    std::array<CBaseEntity17Fixture*,4> slots{};
    unsigned count=0u;
    unsigned gets=0u;
    unsigned flags=0u;
    unsigned reconciles=0u;
};
std::int32_t packet_count(void* user) noexcept {
    auto* p=static_cast<PacketFixture*>(user);
    ++p->count;
    return static_cast<std::int32_t>(p->slots.size());
}
void* packet_get(void* user,std::int32_t i) noexcept {
    auto* p=static_cast<PacketFixture*>(user);
    ++p->gets;
    if(i<0 || i>=static_cast<int>(p->slots.size()))
        return nullptr;
    return p->slots[static_cast<std::size_t>(i)];
}
bool packet_predictable(void* user,void* raw) noexcept {
    auto* p=static_cast<PacketFixture*>(user);
    ++p->flags;
    return static_cast<CBaseEntity17Fixture*>(raw)->predictable();
}
std::uint8_t packet_reconcile(void* user,void* raw,
                              std::uint32_t frame) noexcept {
    auto* p=static_cast<PacketFixture*>(user);
    auto* e=static_cast<CBaseEntity17Fixture*>(raw);
    auto ops=make_ops(*p->trace,*e);
    p->trace->last=per::Run(e,static_cast<std::int32_t>(frame),ops);
    ++p->reconciles;
    if(p->trace->last.status!=per::Status::Complete)
        ++p->trace->wrong_abi;
    return p->trace->last.return_al;
}
} // namespace
int main() {
    pred::events={};
    Trace trace{};
    auto first=std::make_unique<CBaseEntity17Fixture>();
    auto second=std::make_unique<CBaseEntity17Fixture>();
    auto third=std::make_unique<CBaseEntity17Fixture>();
    const auto base=reinterpret_cast<std::uintptr_t>(first.get());
    if(first->predictable_addr()-base!=per::kOriginalPredictableFieldOffset ||
       first->history_addr()-base!=per::kOriginalHistoryStartOffset ||
       first->active_addr()-base!=per::kOriginalHistoryActiveOffset)
        return fail(1,"original entity +0x2E8/+0x2EC/+0x454 offsets drift");
    auto** vtable=*reinterpret_cast<void***>(first.get());
    if(!vtable[16] || vtable[15]==vtable[16])
        return fail(2,"synthetic actual x86 entity virtual slot16 missing");
    first->set_predictable(1u);
    first->set_history_active(true);
    first->history()[0]=0xAA11BB22u;
    first->history()[89]=0xCC33DD44u;
    first->set_difference(3);
    auto ops=make_ops(trace,*first);
    auto r=per::Run(first.get(),0,ops);
    if(r.status!=per::Status::Complete || r.return_al!=0u ||
       r.prepared!=0u || !trace.matches("t",1u) ||
       trace.tags!=1u || trace.getters!=0u)
        return fail(3,"frame==0 must tag but skip comparisons and AL=0");
    trace.reset();
    r=per::Run(first.get(),-1,ops);
    if(r.status!=per::Status::Complete || r.return_al!=0u ||
       r.prepared!=0u || !trace.matches("t",1u))
        return fail(4,"negative signed frame must skip ring and comparison");
    trace.reset();
    r=per::Run(first.get(),1,ops);
    if(r.status!=per::Status::Complete ||
       r.history_index!=0u || r.history_word!=0xAA11BB22u ||
       r.return_al!=1u || r.observed_difference_count!=3 ||
       !trace.matches("tpvd",4u) || trace.wrong_abi!=0u)
        return fail(5,"frame1 ring index0 / virtual16 / count>0 AL1");
    trace.reset();
    r=per::Run(first.get(),90,ops);
    if(r.status!=per::Status::Complete ||
       r.history_index!=89u || r.history_word!=0xCC33DD44u ||
       !trace.matches("tpvd",4u))
        return fail(6,"frame90 ring index89 differs");
    trace.reset();
    r=per::Run(first.get(),91,ops);
    if(r.history_index!=0u || r.history_word!=0xAA11BB22u ||
       r.return_al!=1u)
        return fail(7,"frame91 ring wraps modulo90 incorrectly");
    first->set_difference(0);
    trace.reset();
    r=per::Run(first.get(),2,ops);
    if(r.return_al!=0u || r.observed_difference_count!=0)
        return fail(8,"zero difference count must return AL0");
    first->set_difference(-11);
    trace.reset();
    r=per::Run(first.get(),3,ops);
    if(r.return_al!=0u || r.observed_difference_count!=-11)
        return fail(9,"negative difference count must return AL0");
    first->set_difference(1);
    first->set_history_active(false);
    ops=make_ops(trace,*first);
    trace.reset();
    r=per::Run(first.get(),90,ops);
    if(r.return_al!=1u || r.history_word!=0u ||
       !trace.matches("tpvd",4u))
        return fail(10,"inactive history pointer should use zero word");
    ops.difference_count=nullptr;
    trace.reset();
    r=per::Run(first.get(),1,ops);
    if(r.status!=per::Status::MissingComparisonBackend ||
       !trace.matches("t",1u))
        return fail(11,"missing opaque comparison backend not refused");
    trace.reset();
    r=per::Run(nullptr,1,make_ops(trace,*first));
    if(r.status!=per::Status::MissingEntity || trace.len!=0u)
        return fail(12,"missing C_BaseEntity source not safely refused");

    // Compose actual compiler-generated CPrediction vtable[6] RET4
    // and synthetic C_BaseEntity vtable[16] RET0. The vtable[6]
    // reconciliation callback invokes our bounded source 0x240C6900
    // model, whose later compare helper remains opaque.
    auto prediction=std::make_unique<pred::Prediction23Fixture>(nullptr);
    auto* powner=prediction.get();
    auto** ptable=*reinterpret_cast<void***>(powner);
    if(!ptable[6])
        return fail(13,"original prediction PostNetwork slot6 absent");
    PacketFixture packet{};
    packet.trace=&trace;
    packet.slots={{first.get(),nullptr,second.get(),third.get()}};
    first->set_difference(0);
    first->set_history_active(true);
    second->set_predictable(0u);
    third->set_predictable(0xFFu);
    third->set_history_active(false);
    third->set_difference(7);
    pred::PostNetworkOps net{};
    net.user=&packet;
    net.local_player_present=true;
    net.cl_predict=1;
    net.get_count=&packet_count;
    net.get_entity=&packet_get;
    net.is_predictable=&packet_predictable;
    net.reconcile_entity=&packet_reconcile;
    prediction->set_postnetwork_ops(&net);
    trace.reset();
    if(!invoke_one(powner,ptable[6],91u) ||
       prediction->value_b()!=91u ||
       prediction->postnetwork_any_reconciled()!=1u ||
       packet.count!=1u || packet.gets!=4u || packet.flags!=3u ||
       packet.reconciles!=2u || trace.wrong_abi!=0u ||
       trace.tags!=2u || trace.prepares!=2u ||
       trace.getters!=2u || trace.differences!=2u ||
       pred::events.postnetwork_unmodeled_tail_calls!=1u)
        return fail(14,"actual nested prediction[6] / entity[16] comparison ABI");
    third->set_difference(0);
    packet.count=packet.gets=packet.flags=packet.reconciles=0u;
    trace.reset();
    if(!invoke_one(powner,ptable[6],91u) ||
       prediction->postnetwork_any_reconciled()!=0u ||
       packet.reconciles!=2u || trace.differences!=2u ||
       pred::events.postnetwork_unmodeled_tail_calls!=2u)
        return fail(15,"all original entity AL0 results leave prediction+0x30 zero");
    net.local_player_present=false;
    trace.reset();
    const auto previous_count=packet.count;
    if(!invoke_one(powner,ptable[6],92u) ||
       prediction->value_b()!=92u ||
       prediction->postnetwork_any_reconciled()!=0u ||
       packet.count!=previous_count || trace.len!=0u)
        return fail(16,"no local player should not call entity compare");
    prediction->set_postnetwork_ops(nullptr);
    prediction.reset();
    if(pred::events.destructor_calls!=1u)
        return fail(17,"synthetic prediction teardown count changed");
    std::puts("PASS Phase1669 original-free MSVC x86: authentic "
              "C_BaseEntity PostNetwork result 192-byte source contract, "
              "signed one-argument RET4 caller, frame>0, 90-slot "
              "history (frame-1)%90 at +0x2EC with active marker "
              "+0x454, original C_BaseEntity vptr[16] +0x40, "
              "opaque comparison returns AL1 iff count>0; nested "
              "actual 23-slot CPrediction[6] and 17-slot entity[16] "
              "with two eligible entities and bit-exact +0x30 flag, "
              "null/negative/backend safety, zero retail methods");
    return 0;
}
