// Phase 1659: synthetic MSVC x86 ABI regression for original CPrediction
// vtable slot8 host DWORD and slot13 one-outpointer 3-DWORD vector reader.
// All state is generated and owned in this test, never a retail game object.
#include "native_client_prediction23_init_probe.h"
#include <cstdint>
#include <cstdio>
#include <memory>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1659 original prediction host reader requires MSVC x86
#endif
namespace abi=sin_independent::client_prediction23_diagnostic;
namespace {
int fail(int n,const char* why) {
    std::fprintf(stderr,
        "FAIL Phase1659 native prediction host slot8/13 #%d: %s\n",n,why);
    return n;
}
bool invoke_slot8(void* object,void* method,std::uint32_t& result) noexcept {
    std::uintptr_t before=0u,after=0u;
    std::uint32_t value=0u;
    __asm {
        mov before,esp
        mov ecx,object
        mov eax,method
        call eax
        mov value,eax
        mov after,esp
    }
    result=value;
    return before==after;
}
bool invoke_slot13(void* object,void* method,
                   abi::ViewBits3* output) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm {
        mov before,esp
        push output
        mov ecx,object
        mov eax,method
        call eax
        mov after,esp
    }
    return before==after;
}
struct GuardedVec {
    std::uint32_t before=0x1234ABCDu;
    abi::ViewBits3 out{};
    std::uint32_t after=0xC0FFEE77u;
};
bool boundaries(const GuardedVec& guard) noexcept {
    return guard.before==0x1234ABCDu && guard.after==0xC0FFEE77u;
}
} // namespace
int main() {
    abi::events={};
    auto host=std::make_unique<abi::HostSnapshot>();
    if(reinterpret_cast<std::uintptr_t>(&host->field1dc)-
       reinterpret_cast<std::uintptr_t>(host.get())!=
       abi::kOriginalHostFlagOffset)
        return fail(1,"original host field +0x1DC layout mismatch");
    if(reinterpret_cast<std::uintptr_t>(&host->view)-
       reinterpret_cast<std::uintptr_t>(host.get())!=
       abi::kOriginalHostViewOffset)
        return fail(2,"original host view +0xF04 layout mismatch");
    auto owner=std::make_unique<abi::Prediction23Fixture>(nullptr);
    auto* const receiver=owner.get();
    auto** table=*reinterpret_cast<void***>(receiver);
    if(!table || !table[abi::kOriginalHostFlagSlot] ||
       !table[abi::kOriginalHostViewSlot] ||
       table[abi::kOriginalHostFlagSlot]==table[abi::kOriginalHostViewSlot])
        return fail(3,"original prediction ABI slot8/13 vtable omitted/aliased");
    // Absent host: retail slot8 returns EAX=0, slot13 writes 12
    // zero bytes to caller's buffer. Never invent host values.
    std::uint32_t field=0xFFFFFFFFu;
    GuardedVec guard{};
    guard.out={0xFFFFFFFFu,0xFFFFFFFFu,0xFFFFFFFFu};
    if(!invoke_slot8(receiver,table[8],field) || field!=0u ||
       !invoke_slot13(receiver,table[13],&guard.out) ||
       guard.out.x!=0u || guard.out.y!=0u || guard.out.z!=0u ||
       !boundaries(guard))
        return fail(4,"null original host defaults not 0/Vec3(0)");
    host->field1dc=0xA1B2C3D4u;
    host->view={0x3F800000u,0xBF800000u,0x7FC01234u};
    owner->set_view_host(host.get());
    guard.out={0u,0u,0u};
    if(!invoke_slot8(receiver,table[8],field) ||
       field!=0xA1B2C3D4u ||
       !invoke_slot13(receiver,table[13],&guard.out) ||
       guard.out.x!=0x3F800000u ||
       guard.out.y!=0xBF800000u ||
       guard.out.z!=0x7FC01234u ||
       !boundaries(guard))
        return fail(5,"host flag/vector DWORD exact-bit copies differ");
    // Changing only one axis must not mutate the other two, even for
    // NaN bit patterns. The original copies all three DWORDs directly.
    host->view.y=0x7F800000u;
    host->field1dc=0u;
    if(!invoke_slot13(receiver,table[13],&guard.out) ||
       guard.out.x!=0x3F800000u ||
       guard.out.y!=0x7F800000u ||
       guard.out.z!=0x7FC01234u ||
       !invoke_slot8(receiver,table[8],field) ||
       field!=0u || !boundaries(guard))
        return fail(6,"separate axes / host scalar aliasing");
    // Passing a nonexistent OUT pointer to the native implementation
    // would dereference null. The host-owned model safely refuses it.
    if(!invoke_slot13(receiver,table[13],nullptr) ||
       abi::events.missing_output_refusals!=1u)
        return fail(7,"null synthetic OUT pointer was not refused");
    // Retire host object: no borrowed engine pointer remains.
    owner->set_view_host(nullptr);
    host.reset();
    if(!invoke_slot8(receiver,table[8],field) || field!=0u ||
       !invoke_slot13(receiver,table[13],&guard.out) ||
       guard.out.x!=0u || guard.out.y!=0u || guard.out.z!=0u ||
       !boundaries(guard))
        return fail(8,"retired host state was still observable");
    if(abi::events.init_calls!=0u)
        return fail(9,"reading host view unexpectedly invoked Init");
    owner.reset();
    if(abi::events.destructor_calls!=1u)
        return fail(10,"synthetic prediction destroyed incorrect number of times");
    std::puts("PASS Phase1659 original-free MSVC x86: prediction slot8 "
              "host +0x1DC DWORD RET0, slot13 host +0xF04 3-DWORD "
              "output RET4, missing-host zeros, float-bit copies, "
              "output canaries, null-output refusal and retired host");
    return 0;
}
