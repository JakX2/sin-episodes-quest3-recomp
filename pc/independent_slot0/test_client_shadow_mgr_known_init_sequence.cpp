// Phase 480 original-free MSVC Win32 CClientShadowMgr known Init
// sequence fixture. No original shadow objects, shaders or engine calls.
#include "client_shadow_mgr_known_init_sequence.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace sh=sin_independent::client_shadow_init;
static int Fail(int code) {
    std::fprintf(stderr,"known ShadowMgr Init sequence failed=%d\n",code);
    return code;
}
struct Event final {
    unsigned id=0;
    std::uint32_t a=0,b=0,c=0;
};
struct Capture final {
    std::array<Event,12> entries{};
    std::size_t count=0;
    bool overflow=false;
    void Push(unsigned id,std::uint32_t a=0,std::uint32_t b=0,
              std::uint32_t c=0) noexcept {
        if(count>=entries.size()){overflow=true;return;}
        entries[count++]={id,a,b,c};
    }
};
static void Member(void* object,std::uint32_t tag1,
                   std::uint32_t tag2) noexcept {
    static_cast<Capture*>(object)->Push(1,tag1,tag2);
}
static void Direction(void* object,
                      const std::array<std::uint32_t,3>& value) noexcept {
    static_cast<Capture*>(object)->Push(2,value[0],value[1],value[2]);
}
static void Length(void* object,std::uint32_t raw) noexcept {
    static_cast<Capture*>(object)->Push(3,raw);
}
static void Bias(void* object,std::uint32_t raw) noexcept {
    static_cast<Capture*>(object)->Push(4,raw);
}
static void Conditional(void* object) noexcept {
    static_cast<Capture*>(object)->Push(5);
}
static void Register(void* object,std::uint32_t callback) noexcept {
    static_cast<Capture*>(object)->Push(6,callback);
}
static sh::Hooks MakeHooks(Capture& cap,bool originalFlag) {
    sh::Hooks h{};
    h.context=&cap;
    h.initialize_member14=&Member;
    h.set_direction_slot60=&Direction;
    h.set_length_slot6c=&Length;
    h.set_bias_slot70=&Bias;
    h.initialize_conditional_shadows=&Conditional;
    h.register_engine_callback_slot1d0=&Register;
    h.original_external_shadow_flag=originalFlag;
    return h;
}
int main(){
    static_assert(sh::kOriginalInitVA==0x24118880u,"original method identity");
    static_assert(sh::kOriginalFlagCDOffset==0xcdu &&
                  sh::kOriginalModeD8Offset==0xd8u,
                  "only two observed owner fields");
    static_assert(sh::kOriginalDirectionSlotBytes==0x60u &&
                  sh::kOriginalLengthSlotBytes==0x6cu &&
                  sh::kOriginalBiasSlotBytes==0x70u &&
                  sh::kOriginalEngineRegistrationSlotBytes==0x1d0u,
                  "source-authenticated virtual slot routing");
    Capture out{};
    sh::OwnedFlags own{};
    own.shadow_initialized=1;
    own.mode=0xabcdu;
    auto hooks=MakeHooks(out,false);
    hooks.initialize_conditional_shadows=nullptr; // not needed in false branch
    if(sh::InvokeKnownSequence(own,hooks)!=sh::Result::KnownSequenceInvoked ||
       own.shadow_initialized!=0 || own.mode!=8 || out.count!=5 ||
       out.overflow)return Fail(1);
    const std::array<unsigned,5> expectedNoConditional{{1,2,3,4,6}};
    for(std::size_t i=0;i<expectedNoConditional.size();++i)
        if(out.entries[i].id!=expectedNoConditional[i])return Fail(2);
    if(out.entries[0].a!=sh::kOriginalMember14Tag1 ||
       out.entries[0].b!=sh::kOriginalMember14Tag2 ||
       out.entries[1].a!=0x3dcccccdu ||
       out.entries[1].b!=0x3dcccccdu ||
       out.entries[1].c!=0xbf800000u ||
       out.entries[2].a!=0x42480000u ||
       out.entries[3].a!=0x3ba3d70au ||
       out.entries[4].a!=sh::kOriginalEngineCallbackVA)
        return Fail(3);

    Capture withConditional{};
    sh::OwnedFlags flagged{};
    flagged.shadow_initialized=0xff;
    flagged.mode=0x1234;
    hooks=MakeHooks(withConditional,true);
    if(sh::InvokeKnownSequence(flagged,hooks)!=sh::Result::KnownSequenceInvoked ||
       flagged.shadow_initialized!=0 || flagged.mode!=8 ||
       withConditional.count!=6)return Fail(4);
    const std::array<unsigned,6> expectedWithConditional{{1,2,3,4,5,6}};
    for(std::size_t i=0;i<expectedWithConditional.size();++i)
        if(withConditional.entries[i].id!=expectedWithConditional[i])
            return Fail(5);
    if(withConditional.entries[5].a!=sh::kOriginalEngineCallbackVA)
        return Fail(6);

    // Optional callback becomes mandatory when source global flag
    // indicates conditional shadow initialization. Refuse *before*
    // mutating two owned fields or executing any callback.
    Capture rejected{};
    sh::OwnedFlags untouched{};
    untouched.shadow_initialized=0xa5;
    untouched.mode=0xdeadbeefu;
    hooks=MakeHooks(rejected,true);
    hooks.initialize_conditional_shadows=nullptr;
    if(sh::InvokeKnownSequence(untouched,hooks)!=
           sh::Result::MissingRequiredHooks ||
       rejected.count!=0 || untouched.shadow_initialized!=0xa5 ||
       untouched.mode!=0xdeadbeefu)return Fail(7);

    hooks=MakeHooks(rejected,false);
    hooks.set_direction_slot60=nullptr;
    if(sh::InvokeKnownSequence(untouched,hooks)!=
           sh::Result::MissingRequiredHooks ||
       rejected.count!=0 || untouched.shadow_initialized!=0xa5)
        return Fail(8);
    hooks=MakeHooks(rejected,false);
    hooks.register_engine_callback_slot1d0=nullptr;
    if(sh::InvokeKnownSequence(untouched,hooks)!=
           sh::Result::MissingRequiredHooks || rejected.count!=0)
        return Fail(9);

    // Independent repeated sequence must deterministically overwrite
    // the two known state fields; original registry lifetime is not
    // claimed, and the caller owns every external renderer callback.
    Capture replay{};
    hooks=MakeHooks(replay,false);
    untouched.shadow_initialized=0xff;
    if(sh::InvokeKnownSequence(untouched,hooks)!=sh::Result::KnownSequenceInvoked ||
       sh::InvokeKnownSequence(untouched,hooks)!=sh::Result::KnownSequenceInvoked ||
       replay.count!=10 || untouched.mode!=8 ||
       untouched.shadow_initialized!=0)return Fail(10);
    for(std::size_t i=0;i<10;++i)
        if(replay.entries[i].id!=expectedNoConditional[i%5])return Fail(11);

    std::puts("PASS original-known CClientShadowMgr Init: 5 or 6 ordered callbacks, bitwise configuration, 2 state fields, guarded missing-renderer preflight");
    return 0;
}
