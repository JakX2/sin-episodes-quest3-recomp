// Phase 1658: native compiler-generated 23-slot prediction vftable test.
// Runs ONLY synthetic locally owned C++ objects, never original game code.
#include "native_client_prediction23_init_probe.h"
#include <cstdint>
#include <cstdio>
#include <memory>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Prediction 23-slot ABI regression requires native MSVC Windows x86.
#endif

namespace api=sin_independent::client_prediction23_diagnostic;
namespace {
int fail(int code,const char* why) {
    std::fprintf(stderr,
        "FAIL Phase1658 synthetic original prediction x86 ABI %d: %s\n",
        code,why);
    return code;
}
bool invoke_init_no_args(void* receiver,void* method) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm {
        mov before,esp
        mov ecx,receiver
        mov eax,method
        call eax
        mov after,esp
    }
    return before==after;
}
} // namespace

int main() {
    api::events={};
    api::ConVarView cl_predict{};
    cl_predict.parent=&cl_predict; // original constructor: m_pParent=this
    auto owner=std::make_unique<api::Prediction23Fixture>(&cl_predict);
    auto* const receiver=owner.get();
    const auto owner_address=reinterpret_cast<std::uintptr_t>(receiver);
    if(receiver->enabled_address()-owner_address!=
       api::kOriginalPredictionEnabledOffset)
        return fail(1,"synthetic enabled byte is not this+0x10");
    auto** vtable=*reinterpret_cast<void***>(receiver);
    if(!vtable)return fail(2,"synthetic prediction has no vfptr");
    for(unsigned i=0u;i<api::kOriginalVirtualCount;++i) {
        if(!vtable[i])
            return fail(3,"missing compiler-emitted prediction slot");
        for(unsigned j=0u;j<i;++j)
            if(vtable[i]==vtable[j])
                return fail(4,"two independently generated prediction slots alias");
    }
    if(vtable[0]==vtable[1])
        return fail(5,"destructor wrongly aliases parameterless Init");
    // Original code reads cl_predict m_pParent at ConVar +0x18 and
    // tests m_pParent->m_nValue (+0x2C). All state is invented here.
    if(!invoke_init_no_args(receiver,vtable[1]) ||
       receiver->enabled()!=0u || api::events.init_calls!=1u)
        return fail(6,"zero-valued service word Init or RET0 stack");
    cl_predict.numeric_value=static_cast<std::int32_t>(0x80000000u);
    if(!invoke_init_no_args(receiver,vtable[1]) ||
       receiver->enabled()!=1u || api::events.init_calls!=2u)
        return fail(7,"nonzero service word boolean Init or RET0 stack");
    cl_predict.numeric_value=0;
    if(!invoke_init_no_args(receiver,vtable[1]) ||
       receiver->enabled()!=0u || api::events.init_calls!=3u)
        return fail(8,"service transition true->false not reflected");
    cl_predict.numeric_value=0x10;
    if(!invoke_init_no_args(receiver,vtable[1]) ||
       receiver->enabled()!=1u || api::events.init_calls!=4u)
        return fail(9,"service transition false->true not reflected");
    cl_predict.parent=nullptr;
    if(!invoke_init_no_args(receiver,vtable[1]) ||
       receiver->enabled()!=1u ||
       api::events.missing_provider_refusals!=1u ||
       api::events.init_calls!=5u)
        return fail(10,"unbound cl_predict m_pParent not safely refused");
    receiver->set_cl_predict(nullptr);
    if(!invoke_init_no_args(receiver,vtable[1]) ||
       receiver->enabled()!=1u ||
       api::events.missing_provider_refusals!=2u ||
       api::events.init_calls!=6u)
        return fail(13,"missing cl_predict ConVar object not refused");
    // Model ConVar aliases: original CPrediction dereferences the
    // *parent* value, not always the numeric value in the input ConVar.
    api::ConVarView alternateParent{};
    alternateParent.parent=&alternateParent;
    alternateParent.numeric_value=0;
    cl_predict.parent=&alternateParent;
    receiver->set_cl_predict(&cl_predict);
    if(!invoke_init_no_args(receiver,vtable[1]) ||
       receiver->enabled()!=0u || api::events.init_calls!=7u)
        return fail(14,"cl_predict must read parent's numeric value");
    // Important distinction: original native slot1 has NO null check.
    // The test's null-ConVar refusal is a deliberate safety deviation.
    if(api::events.destructor_calls!=0u)
        return fail(11,"prediction object destroyed during normal initialization");
    // Only the synthetic object is deleted; no original deleting-destructor
    // method or original client.dll is invoked.
    owner.reset();
    if(api::events.destructor_calls!=1u)
        return fail(12,"synthetic prediction destruction did not occur once");
    std::puts("PASS Phase1658 original-free MSVC Win32 x86: compiler-generated "
              "23-slot prediction table with scalar deleting destructor in "
              "slot0, no-argument __thiscall Init in slot1, ECX/ESP RET0, "
              "cl_predict m_pParent+0x18 / m_nValue+0x2C "
              "to-this+0x10 boolean conversion, "
              "null-service refusal, and local destructor ownership");
    return 0;
}
