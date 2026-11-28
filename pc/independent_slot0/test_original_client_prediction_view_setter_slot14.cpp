// Phase1663: native original-free Win32 x86 CPrediction slots13/14
// round trip C_BasePlayer +0xF04 three-DWORD view state.
//
// No original DLL, original game memory, original player method, or
// engine CreateInterface virtual invocation. Only invented host data.
#include "native_client_prediction23_init_probe.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1663 requires MSVC x86 native prediction view setter ABI
#endif
namespace pred=sin_independent::client_prediction23_diagnostic;
namespace {
int fail(int id,const char* why) {
    std::fprintf(stderr,
       "FAIL Phase1663 CPrediction x86 view slot13/14 #%d: %s\n",id,why);
    return id;
}
bool one_arg_thiscall(void* owner,void* fn,
                      pred::ViewBits3* argument) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm {
        mov before,esp
        push argument
        mov ecx,owner
        mov eax,fn
        call eax
        mov after,esp
    }
    return before==after;
}
bool same(const pred::ViewBits3& a,
          const pred::ViewBits3& b) noexcept {
    return a.x==b.x && a.y==b.y && a.z==b.z;
}
struct Guarded {
    std::uint32_t before=0x65FAB123u;
    pred::ViewBits3 payload{};
    std::uint32_t after=0xC0FFEE77u;
};
bool safe(const Guarded& g) noexcept {
    return g.before==0x65FAB123u &&
           g.after==0xC0FFEE77u;
}
} // namespace
int main() {
    pred::events={};
    auto prediction=std::make_unique<pred::Prediction23Fixture>(nullptr);
    auto* receiver=prediction.get();
    auto** table=*reinterpret_cast<void***>(receiver);
    if(!table[13] || !table[14] || table[13]==table[14])
        return fail(1,"original prediction slot13 getter and slot14 setter missing");

    Guarded in{};
    in.payload={0x3F800000u,0x7FC01234u,0x80000000u};
    const auto untouched=in.payload;
    // Original slot14 returns without touching the pointer when the
    // current C_BasePlayer global is NULL.
    if(!one_arg_thiscall(receiver,table[14],&in.payload) ||
       !same(in.payload,untouched) || !safe(in) ||
       pred::events.missing_view_setter_inputs!=0u)
        return fail(2,"null player setter no-op or x86 RET4 failure");
    Guarded out{};
    out.payload={0xFFFFFFFFu,0xFFFFFFFFu,0xFFFFFFFFu};
    if(!one_arg_thiscall(receiver,table[13],&out.payload) ||
       !same(out.payload,{0u,0u,0u}) || !safe(out))
        return fail(3,"null host slot13 must fill all three words zero");

    auto owner=std::make_unique<pred::HostSnapshot>();
    if(reinterpret_cast<std::uintptr_t>(&owner->view)-
       reinterpret_cast<std::uintptr_t>(owner.get())!=
       pred::kOriginalHostViewOffset)
        return fail(4,"original player view +0xF04 offset not preserved");
    prediction->set_writable_host_view(owner.get());
    if(!one_arg_thiscall(receiver,table[14],&in.payload) ||
       !same(owner->view,untouched) ||
       !one_arg_thiscall(receiver,table[13],&out.payload) ||
       !same(out.payload,untouched) ||
       !safe(in) || !safe(out))
        return fail(5,"slot14 writer -> same +0xF04 bytes -> slot13 getter");
    // All values are moved as raw DWORDs. The original setter does
    // not apply floating equality, unlike player orientation setters
    // in the separate Phase1662 C_BasePlayer virtual pair.
    in.payload={0xFFFFFFFFu,0x00000000u,0x7F800000u};
    if(!one_arg_thiscall(receiver,table[14],&in.payload) ||
       !one_arg_thiscall(receiver,table[13],&out.payload) ||
       !same(out.payload,in.payload) ||
       !same(owner->view,in.payload))
        return fail(6,"zero / infinity / NaN raw-bit round trip differs");
    // Native original would crash on nonnull host and null input.
    // Diagnostic host-only implementation refuses it instead.
    if(!one_arg_thiscall(receiver,table[14],nullptr) ||
       pred::events.missing_view_setter_inputs!=1u ||
       !same(owner->view,in.payload))
        return fail(7,"invalid source pointer not safely refused");

    auto other=std::make_unique<pred::HostSnapshot>();
    other->view={0x41u,0x42u,0x43u};
    const auto old_view=owner->view;
    prediction->set_writable_host_view(other.get());
    in.payload={0x91u,0x92u,0x93u};
    if(!one_arg_thiscall(receiver,table[14],&in.payload) ||
       !same(other->view,in.payload) ||
       !same(owner->view,old_view) ||
       !one_arg_thiscall(receiver,table[13],&out.payload) ||
       !same(out.payload,in.payload))
        return fail(8,"replaced owned player should not mutate prior host");
    // Downgrading an attachment to read-only must REVOKE prior
    // write authority while retaining an observable getter target.
    const auto readonly_view=other->view;
    prediction->set_view_host(other.get());
    in.payload={0xDEADBEEFu,0xC0FFEE00u,0x7FC0A11Au};
    if(!one_arg_thiscall(receiver,table[14],&in.payload) ||
       !same(other->view,readonly_view) ||
       !one_arg_thiscall(receiver,table[13],&out.payload) ||
       !same(out.payload,readonly_view))
        return fail(9,"read-only attachment downgrade retained mutable access");
    prediction->set_writable_host_view(nullptr);
    owner.reset();
    other.reset();
    out.payload={0xAAu,0xBBu,0xCCu};
    if(!one_arg_thiscall(receiver,table[13],&out.payload) ||
       !same(out.payload,{0u,0u,0u}) ||
       !one_arg_thiscall(receiver,table[14],&in.payload) ||
       pred::events.missing_orientation_callbacks!=0u ||
       pred::events.init_calls!=0u)
        return fail(10,"retired host was still accessed by prediction");
    prediction.reset();
    if(pred::events.destructor_calls!=1u)
        return fail(11,"synthetic prediction instance destroyed incorrectly");
    std::puts("PASS Phase1663 original-free MSVC x86: 23-slot "
              "CPrediction slot13 getter/slot14 setter both RET4, "
              "C_BasePlayer raw-bit 3-word +0xF04 view read/write "
              "roundtrip, no-player no-op and getter zeros, original "
              "field offset, IEEE special-bit preservation, guard "
              "canaries, null refusal, replacement and teardown");
    return 0;
}
