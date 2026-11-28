// Phase1667: actual MSVC x86 original-free C_BaseEntity vtable+0x44
// and 61-byte predictable teardown call ordering, including from the
// existing disabled CPrediction packet fallback. No original DLL code.
#include "native_original_predictable_entity_teardown.h"
#include "native_original_prediction_disabled_shutdown.h"
#include "native_client_prediction23_init_probe.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1667 original predictable entity teardown requires MSVC x86
#endif
namespace td=sin_independent::original_predictable_entity_teardown;
namespace fallback=sin_independent::original_prediction_disabled_shutdown;
namespace pred=sin_independent::client_prediction23_diagnostic;
namespace {
int fail(int id,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1667 original-free C_BaseEntity teardown native x86 "
      "#%d: %s\n",id,why);
    return id;
}
struct Trace {
    std::array<char,128> events{};
    std::array<int,12> visited{};
    unsigned size=0u;
    unsigned visit_count=0u;
    unsigned bad=0u;
    unsigned predictable_shutdowns=0u;
    unsigned client_shutdowns=0u;
    unsigned summary_calls=0u;
    unsigned summary_predictable=0u;
    unsigned summary_created=0u;
    std::uint32_t last_handle=0u;
    std::uint32_t last_final_token=0u;
    fallback::Result fallback_result{};
    void emit(char code) noexcept {
        if(size<events.size())events[size++]=code;
    }
    bool matches(const char* x,unsigned n) const noexcept {
        if(size!=n)return false;
        for(unsigned i=0u;i<n;++i)
            if(events[i]!=x[i])return false;
        return true;
    }
    void reset() noexcept {
        size=0u; events.fill(0);
        visited.fill(-1); visit_count=0u; bad=0u;
        predictable_shutdowns=0u; client_shutdowns=0u;
        summary_calls=0u;summary_predictable=0u;summary_created=0u;
        last_handle=0u;last_final_token=0u;
    }
};

class FakeCBaseEntity18 {
public:
#define SIN_INERT_ENTITY_SLOT(N) \
    virtual std::uint32_t __thiscall InertSlot##N() noexcept { \
        return 0xD0000000u+static_cast<std::uint32_t>(N); \
    }
    SIN_INERT_ENTITY_SLOT(0)
    SIN_INERT_ENTITY_SLOT(1)
    SIN_INERT_ENTITY_SLOT(2)
    SIN_INERT_ENTITY_SLOT(3)
    SIN_INERT_ENTITY_SLOT(4)
    SIN_INERT_ENTITY_SLOT(5)
    SIN_INERT_ENTITY_SLOT(6)
    SIN_INERT_ENTITY_SLOT(7)
    SIN_INERT_ENTITY_SLOT(8)
    SIN_INERT_ENTITY_SLOT(9)
    SIN_INERT_ENTITY_SLOT(10)
    SIN_INERT_ENTITY_SLOT(11)
    SIN_INERT_ENTITY_SLOT(12)
    SIN_INERT_ENTITY_SLOT(13)
    SIN_INERT_ENTITY_SLOT(14)
    SIN_INERT_ENTITY_SLOT(15)
    SIN_INERT_ENTITY_SLOT(16)
#undef SIN_INERT_ENTITY_SLOT

    // Original C_BaseEntity primary virtual offset +0x44 (slot17).
    // Method semantics are opaque. It returns an invented token to
    // verify the original cleanup wrapper forwards EAX to final helper.
    virtual std::uint32_t __thiscall CallbackSlot17() noexcept {
        if(trace_) {
            trace_->emit('V');
            if(predictable_byte_!=0u)++trace_->bad;
        }
        return virtual_token_;
    }

    FakeCBaseEntity18() noexcept =default;
    void bind(Trace* trace,unsigned id,std::uint32_t handle,
              std::uint32_t token,std::uint8_t predictable) noexcept {
        trace_=trace;id_=id;handle_=handle;
        virtual_token_=token;predictable_byte_=predictable;
    }
    std::uint32_t handle() const noexcept {return handle_;}
    std::uint8_t flag() const noexcept {return predictable_byte_;}
    void clear_predictable() noexcept {predictable_byte_=0u;}
    unsigned id() const noexcept {return id_;}
    const void* handle_addr() const noexcept {return &handle_;}
    const void* flag_addr() const noexcept {return &predictable_byte_;}
private:
    // The primary MSVC 32-bit vfptr is at native offset +0x00.
    // The original handle DWORD is at +0xB4.
    std::array<std::uint8_t,0xB0u> unknown4toB4_{};
    std::uint32_t handle_=0u;
    std::array<std::uint8_t,0x230u> unknownB8to2E8_{};
    std::uint8_t predictable_byte_=0u;
    Trace* trace_=nullptr;
    std::uint32_t virtual_token_=0u;
    unsigned id_=0u;
};

bool invoke_no_args(void* object,void* fn,
                    std::uint32_t& result) noexcept {
    std::uintptr_t before=0u,after=0u;
    std::uint32_t ret=0u;
    __asm {
        mov before,esp
        mov ecx,object
        mov eax,fn
        call eax
        mov ret,eax
        mov after,esp
    }
    result=ret;
    return before==after;
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
std::uint32_t on_read_handle(void* user,void* entity) noexcept {
    auto* t=static_cast<Trace*>(user);
    auto* e=static_cast<FakeCBaseEntity18*>(entity);
    t->emit('H');
    return e->handle();
}
void on_unregister(void* user,void* entity,
                   std::uint32_t handle) noexcept {
    auto* t=static_cast<Trace*>(user);
    auto* e=static_cast<FakeCBaseEntity18*>(entity);
    t->emit('U');
    t->last_handle=handle;
    if(handle!=e->handle())++t->bad;
}
void on_primary(void* user,void* entity) noexcept {
    auto* t=static_cast<Trace*>(user);
    auto* e=static_cast<FakeCBaseEntity18*>(entity);
    t->emit('I');
    if(e->flag()==0u)++t->bad; // clear must happen AFTER primary
}
void on_clear(void* user,void* entity) noexcept {
    auto* t=static_cast<Trace*>(user);
    t->emit('F');
    static_cast<FakeCBaseEntity18*>(entity)->clear_predictable();
}
std::uint32_t on_vcall(void* user,void* entity) noexcept {
    auto* t=static_cast<Trace*>(user);
    auto** table=*reinterpret_cast<void***>(entity);
    if(!table[td::kOriginalCallbackVtableSlot]) {
        ++t->bad;return 0u;
    }
    std::uint32_t result=0u;
    if(!invoke_no_args(entity,table[td::kOriginalCallbackVtableSlot],result))
        ++t->bad;
    return result;
}
void on_final(void* user,void* entity,
              std::uint32_t token) noexcept {
    auto* t=static_cast<Trace*>(user);
    t->emit('T');
    t->last_final_token=token;
    auto* e=static_cast<FakeCBaseEntity18*>(entity);
    if(e->flag()!=0u)++t->bad;
}
td::Ops make_ops(Trace& t) noexcept {
    td::Ops ops{};
    ops.user=&t;
    ops.read_handle=&on_read_handle;
    ops.unregister_handle=&on_unregister;
    ops.primary_cleanup=&on_primary;
    ops.clear_predictable=&on_clear;
    ops.entity_virtual_17=&on_vcall;
    ops.final_cleanup=&on_final;
    return ops;
}

struct ShutdownList {
    Trace* trace=nullptr;
    std::array<FakeCBaseEntity18*,3> entities{};
    td::Ops teardown{};
    unsigned count_calls=0u;
    std::int32_t reported_count=3;
};
std::int32_t list_count(void* ctx) noexcept {
    auto* list=static_cast<ShutdownList*>(ctx);
    ++list->count_calls;
    list->trace->emit('N');
    return list->reported_count;
}
void* list_get(void* ctx,std::int32_t index) noexcept {
    auto* list=static_cast<ShutdownList*>(ctx);
    auto* t=list->trace;
    t->emit('g');
    if(t->visit_count<t->visited.size())
        t->visited[t->visit_count++]=index;
    if(index<0 || index>=static_cast<int>(list->entities.size()))
        return nullptr;
    return list->entities[static_cast<std::size_t>(index)];
}
bool list_predictable(void* ctx,void* entity) noexcept {
    auto* list=static_cast<ShutdownList*>(ctx);
    list->trace->emit('f');
    return static_cast<FakeCBaseEntity18*>(entity)->flag()!=0u;
}
void list_on_predictable(void* ctx,void* entity) noexcept {
    auto* list=static_cast<ShutdownList*>(ctx);
    auto* t=list->trace;
    const auto ret=td::Run(entity,list->teardown);
    ++t->predictable_shutdowns;
    if(ret.status!=td::Status::Complete ||
       ret.completed_stages!=5u ||
       static_cast<FakeCBaseEntity18*>(entity)->flag()!=0u)
        ++t->bad;
}
void list_on_created(void* ctx,void* entity) noexcept {
    auto* list=static_cast<ShutdownList*>(ctx);
    auto* t=list->trace;
    t->emit('C');
    ++t->client_shutdowns;
    if(static_cast<FakeCBaseEntity18*>(entity)->flag()!=0u)
        ++t->bad;
}
void list_summary(void* ctx,unsigned predict_count,
                  unsigned created_count) noexcept {
    auto* list=static_cast<ShutdownList*>(ctx);
    auto* t=list->trace;
    t->emit('S');
    ++t->summary_calls;
    t->summary_predictable=predict_count;
    t->summary_created=created_count;
}
fallback::Ops make_list_ops(ShutdownList& list) noexcept {
    fallback::Ops ops{};
    ops.user=&list;
    ops.count=&list_count;
    ops.get=&list_get;
    ops.predictable=&list_predictable;
    ops.shutdown_predictable=&list_on_predictable;
    ops.shutdown_client_created=&list_on_created;
    ops.print_summary=&list_summary;
    return ops;
}
void disabled_pre_callback(void* ctx) noexcept {
    auto* list=static_cast<ShutdownList*>(ctx);
    const auto r=fallback::Run(make_list_ops(*list));
    list->trace->emit('D');
    if(r.status!=fallback::Status::Complete ||
       r.predictable_shutdowns!=2u ||
       r.client_created_shutdowns!=1u)
        ++list->trace->bad;
}
} // namespace
int main() {
    pred::events={};
    Trace trace{};
    auto first=std::make_unique<FakeCBaseEntity18>();
    auto second=std::make_unique<FakeCBaseEntity18>();
    auto third=std::make_unique<FakeCBaseEntity18>();
    first->bind(&trace,0u,0x11223344u,0xABCD1234u,1u);
    second->bind(&trace,1u,0x22334455u,0xBCDE2345u,0u);
    third->bind(&trace,2u,0x33445566u,0xCDEF3456u,0xFFu);
    const auto base=reinterpret_cast<std::uintptr_t>(first.get());
    if(reinterpret_cast<std::uintptr_t>(first->handle_addr())-base!=
       td::kOriginalHandleMemberOffset ||
       reinterpret_cast<std::uintptr_t>(first->flag_addr())-base!=
       td::kOriginalPredictableByteOffset)
        return fail(1,"original entity +0xB4 handle/+0x2E8 flag offsets differ");
    auto** table=*reinterpret_cast<void***>(first.get());
    if(!table[0] || !table[17] || table[0]==table[17])
        return fail(2,"MSVC x86 entity virtual slot17 at +0x44 missing");
    // Pure original-free 61-byte sequence: handle -> unregister ->
    // primary cleanup -> flag clear -> slot17 -> final with EAX.
    const auto r=td::Run(first.get(),make_ops(trace));
    if(r.status!=td::Status::Complete ||
       r.completed_stages!=5u ||
       r.saved_handle!=0x11223344u ||
       r.virtual_return!=0xABCD1234u ||
       first->flag()!=0u ||
       !trace.matches("HUIFVT",6u) ||
       trace.last_handle!=r.saved_handle ||
       trace.last_final_token!=r.virtual_return ||
       trace.bad!=0u)
        return fail(3,"original full predictable teardown stages/order/return");
    trace.reset();
    auto ops=make_ops(trace);
    ops.primary_cleanup=nullptr;
    const auto refused=td::Run(third.get(),ops);
    if(refused.status!=td::Status::MissingCallbacks ||
       third->flag()!=0xFFu || trace.size!=0u)
        return fail(4,"missing opaque original callback must fail before writes");
    const auto null=td::Run(nullptr,make_ops(trace));
    if(null.status!=td::Status::MissingEntity || trace.size!=0u)
        return fail(5,"absent original entity should be rejected");

    // Integrate with original CPrediction Pre slot4 cl_predict=0:
    // descending list indices 2 -> 1 -> 0. First was already
    // shut down, so re-arm synthetic flag before packet test.
    first->bind(&trace,0u,0x11223344u,0xABCD1234u,1u);
    third->bind(&trace,2u,0x33445566u,0xCDEF3456u,0xFFu);
    ShutdownList list{};
    list.trace=&trace;
    list.entities={{first.get(),second.get(),third.get()}};
    list.teardown=make_ops(trace);
    auto packet=std::make_unique<pred::Prediction23Fixture>(nullptr);
    pred::PacketEntityOps packet_ops{};
    packet_ops.user=&list;
    packet_ops.cl_predict=0;
    packet_ops.local_player_present=false;
    packet_ops.pre_disabled_fallback=&disabled_pre_callback;
    packet->set_packet_entity_ops(&packet_ops);
    auto** ptable=*reinterpret_cast<void***>(packet.get());
    trace.reset();
    if(!invoke_two(packet.get(),ptable[4],0x1234u,0x9876u) ||
       trace.bad!=0u || first->flag()!=0u || third->flag()!=0u ||
       trace.predictable_shutdowns!=2u ||
       trace.client_shutdowns!=1u ||
       trace.summary_calls!=1u ||
       trace.summary_predictable!=2u ||
       trace.summary_created!=1u ||
       trace.visit_count!=3u ||
       trace.visited[0]!=2 ||
       trace.visited[1]!=1 ||
       trace.visited[2]!=0 ||
       !trace.matches("NgfHUIFVTgfCgfHUIFVTSD",23u))
        return fail(6,"Pre disabled packet fallback -> two native 18-slot entity teardowns");
    trace.reset();
    packet_ops.cl_predict=1;
    if(!invoke_two(packet.get(),ptable[4],1u,2u) ||
       trace.size!=0u || trace.bad!=0u)
        return fail(7,"enabled Pre no-player must not rerun teardown");
    packet->set_packet_entity_ops(nullptr);
    packet.reset();
    if(pred::events.destructor_calls!=1u)
        return fail(8,"synthetic prediction teardown lifetime differs");
    std::puts("PASS Phase1667 native MSVC x86 original-free: "
              "61-byte C_BaseEntity predictable cleanup, original "
              "+0xB4 handle, +0x2E8 flag cleared before primary "
              "vtable[17] +0x44, exact registry->primary->clear->"
              "virtual->final callback ordering and EAX token, "
              "missing callback refusal, nested Phase1666 reverse "
              "disabled Pre packet cleanup of two eligible entities "
              "plus one client-created, original-free host ownership");
    return 0;
}
