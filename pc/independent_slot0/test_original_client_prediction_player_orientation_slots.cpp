// Phase1661: native MSVC x86 synthetic CPrediction slots9-12 ABI.
// The original client.dll is NEVER loaded; all player virtual getters,
// setters, mirror fields and notifier effects are host-owned callbacks.
#include "native_client_prediction23_init_probe.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1661 prediction orientation ABI test requires native MSVC x86
#endif
namespace api=sin_independent::client_prediction23_diagnostic;
namespace {
int fail(int code,const char* why) {
    std::fprintf(stderr,
        "FAIL Phase1661 MSVC Win32 x86 prediction slots9-12 #%d: %s\n",
        code,why);
    return code;
}
bool one_argument_thiscall(void* object,void* fn,
                           api::ViewBits3* argument) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm {
        mov before,esp
        push argument
        mov ecx,object
        mov eax,fn
        call eax
        mov after,esp
    }
    return before==after;
}
struct FakeCBasePlayer {
    std::array<std::uint8_t,0x22Cu> prefix{};
    std::uint32_t notifier_a=0x10203040u;
    std::array<std::uint8_t,0x268u-0x230u> middle{};
    std::uint32_t notifier_b=0x50607080u;
    std::array<std::uint8_t,0x2C8u-0x26Cu> tail{};
    api::ViewBits3 mirrored_a{};
    api::ViewBits3 mirrored_b{};
    api::ViewBits3 returned_a{};
    api::ViewBits3 returned_b{};
    std::array<char,48> events{};
    unsigned count=0u;
    bool correct_mirror_before_notify=true;
    void push(char kind) noexcept {
        if(count<events.size())events[count++]=kind;
    }
};
static_assert(offsetof(FakeCBasePlayer,notifier_a)==
              api::kOriginalPlayerNotifierA,
              "native player orientation notifier A offset drift");
static_assert(offsetof(FakeCBasePlayer,notifier_b)==
              api::kOriginalPlayerNotifierB,
              "native player orientation notifier B offset drift");
static_assert(offsetof(FakeCBasePlayer,mirrored_a)==
              api::kOriginalPlayerMirrorA,
              "native player orientation mirror A offset drift");
static_assert(offsetof(FakeCBasePlayer,mirrored_b)==
              api::kOriginalPlayerMirrorB,
              "native player orientation mirror B offset drift");
bool same(const api::ViewBits3& a,const api::ViewBits3& b) noexcept {
    return a.x==b.x && a.y==b.y && a.z==b.z;
}
const api::ViewBits3* get_a(void* ctx) noexcept {
    auto* p=static_cast<FakeCBasePlayer*>(ctx);
    p->push('A');
    return &p->returned_a;
}
const api::ViewBits3* get_b(void* ctx) noexcept {
    auto* p=static_cast<FakeCBasePlayer*>(ctx);
    p->push('B');
    return &p->returned_b;
}
void set_a(void* ctx,api::ViewBits3* in) noexcept {
    auto* p=static_cast<FakeCBasePlayer*>(ctx);
    p->push('a');
    in->x^=1u; // prove player virtual runs BEFORE source mirror
}
void set_b(void* ctx,api::ViewBits3* in) noexcept {
    auto* p=static_cast<FakeCBasePlayer*>(ctx);
    p->push('b');
    in->y^=0x80000000u;
}
void mirror_a(void* ctx,const api::ViewBits3* in) noexcept {
    auto* p=static_cast<FakeCBasePlayer*>(ctx);
    p->push('m');
    p->mirrored_a=*in;
}
void mirror_b(void* ctx,const api::ViewBits3* in) noexcept {
    auto* p=static_cast<FakeCBasePlayer*>(ctx);
    p->push('M');
    p->mirrored_b=*in;
}
void notify_a(void* ctx) noexcept {
    auto* p=static_cast<FakeCBasePlayer*>(ctx);
    p->push('n');
    // Synthetic notifier is NOT an original game virtual. It checks
    // the mirrored words were already written before callback.
    p->correct_mirror_before_notify &=
        same(p->mirrored_a,api::ViewBits3{0x41u,0x42u,0x43u});
}
void notify_b(void* ctx) noexcept {
    auto* p=static_cast<FakeCBasePlayer*>(ctx);
    p->push('N');
    p->correct_mirror_before_notify &=
        same(p->mirrored_b,api::ViewBits3{
            0x91u,0x80000092u,0x93u});
}
api::OrientationOps ops(FakeCBasePlayer& p) noexcept {
    api::OrientationOps out{};
    out.user=&p;
    out.get_a=&get_a;out.get_b=&get_b;
    out.set_a=&set_a;out.set_b=&set_b;
    out.mirror_a=&mirror_a;out.mirror_b=&mirror_b;
    out.notify_a=&notify_a;out.notify_b=&notify_b;
    return out;
}
bool sequence(const FakeCBasePlayer& p,
              const char* expected,unsigned count) noexcept {
    if(p.count!=count)return false;
    for(unsigned i=0u;i<count;++i)
        if(p.events[i]!=expected[i])return false;
    return true;
}
} // namespace
int main() {
    api::events={};
    auto prediction=std::make_unique<api::Prediction23Fixture>(nullptr);
    auto* receiver=prediction.get();
    auto** table=*reinterpret_cast<void***>(receiver);
    for(unsigned slot=9u;slot<=12u;++slot)
        if(!table[slot])return fail(1,"original prediction ABI slot missing");
    // No original C_BasePlayer: getters zero all OUT DWORDs and
    // setters must do nothing without reading the input.
    api::ViewBits3 output{0xFFFFFFFFu,0xFFFFFFFFu,0xFFFFFFFFu};
    if(!one_argument_thiscall(receiver,table[9],&output) ||
       !same(output,{0u,0u,0u}))
        return fail(2,"slot9 absent player zero vector or RET4 failed");
    output={0xFFFFFFFFu,0xFFFFFFFFu,0xFFFFFFFFu};
    if(!one_argument_thiscall(receiver,table[11],&output) ||
       !same(output,{0u,0u,0u}))
        return fail(3,"slot11 absent player zero vector or RET4 failed");
    auto input_a=api::ViewBits3{0x40u,0x42u,0x43u};
    if(!one_argument_thiscall(receiver,table[10],&input_a) ||
       !one_argument_thiscall(receiver,table[12],&input_a) ||
       !same(input_a,{0x40u,0x42u,0x43u}))
        return fail(4,"setters touched input without current player");

    FakeCBasePlayer player{};
    player.returned_a={0x3F800000u,0x7FC01234u,0x80000000u};
    player.returned_b={0xBF800000u,0x7F800000u,0x12345678u};
    prediction->set_orientation_ops(ops(player));
    if(!one_argument_thiscall(receiver,table[9],&output) ||
       !same(output,player.returned_a) ||
       !one_argument_thiscall(receiver,table[11],&output) ||
       !same(output,player.returned_b) ||
       !sequence(player,"AB",2u))
        return fail(5,"player virtual getters or exact 3-DWORD OUT copies");
    input_a={0x40u,0x42u,0x43u};
    api::ViewBits3 input_b{0x91u,0x92u,0x93u};
    if(!one_argument_thiscall(receiver,table[10],&input_a) ||
       !same(input_a,{0x41u,0x42u,0x43u}) ||
       !same(player.mirrored_a,input_a) ||
       !one_argument_thiscall(receiver,table[12],&input_b) ||
       !same(input_b,{0x91u,0x80000092u,0x93u}) ||
       !same(player.mirrored_b,input_b) ||
       !player.correct_mirror_before_notify ||
       !sequence(player,"ABamnbMN",8u)) {
        return fail(6,"setter virtual->mirror->notifier native sequence");
    }
    // Both native methods are one DWORD thiscall, clean stack by RET4.
    // The test's null safety is a deliberate divergence from retail.
    if(!one_argument_thiscall(receiver,table[9],nullptr) ||
       !one_argument_thiscall(receiver,table[11],nullptr) ||
       !one_argument_thiscall(receiver,table[10],nullptr) ||
       !one_argument_thiscall(receiver,table[12],nullptr) ||
       api::events.missing_output_refusals!=2u ||
       api::events.missing_orientation_inputs!=2u)
        return fail(7,"synthetic invalid input/out refusing original crash");
    // Deleting/retiring client-player binding must not leave a dangling
    // callable callback, even if a stale caller repeats the virtuals.
    prediction->set_orientation_ops({});
    output={1u,2u,3u};
    if(!one_argument_thiscall(receiver,table[9],&output) ||
       !same(output,{0u,0u,0u}) ||
       !one_argument_thiscall(receiver,table[10],&input_a) ||
       player.count!=8u)
        return fail(8,"retired original local player callbacks still called");
    prediction.reset();
    if(api::events.destructor_calls!=1u)
        return fail(9,"synthetic prediction destructor mismatch");
    std::puts("PASS Phase1661 original-free native Win32 x86: CPrediction "
              "slots9,10,11,12 one-OUT-pointer __thiscall RET4, actual "
              "vtable indices, C_BasePlayer virtual getter/setter "
              "callback order, exact two 3-DWORD mirrors at +0x2C8/"
              "+0x2D4 and notifier at +0x22C/+0x268, no-player "
              "zeros/noops, bitwise vectors, null refusal and teardown");
    return 0;
}
