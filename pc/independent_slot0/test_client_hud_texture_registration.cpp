// Native x86 typed three-slot property converter registration fixture.
// Genuine client 0x24009340 duplicate semantics, without commercial code.
#include "client_hud_texture_registration.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <type_traits>
namespace hud=sin_independent::hud_texture;
struct State {
    const char* name=nullptr;
    hud::AnimationConverterAbi* converter=nullptr;
    int finds=0,inserts=0,warnings=0;
    bool reject_insert=false;
    bool reenter=false;
    hud::NativeRegistrationOwner* owner=nullptr;
    hud::Registry* host=nullptr;
};
static hud::AnimationConverterAbi* Find(void* context,const char* name) noexcept {
    auto& s=*static_cast<State*>(context);
    ++s.finds;
    if(s.reenter && s.owner && s.host &&
       (!s.owner->started() || s.owner->Start(*s.host)!=hud::AddResult::Invalid))
        std::abort(); // inner call MUST be already started; see below
    return s.name && !std::strcmp(s.name,name)?s.converter:nullptr;
}
static bool Insert(void* context,const char* name,hud::AnimationConverterAbi* converter) noexcept {
    auto& s=*static_cast<State*>(context);
    ++s.inserts;
    if(s.reject_insert)return false;
    s.name=name;s.converter=converter;
    return true;
}
static void Warn(void* context,const char*) noexcept {
    ++static_cast<State*>(context)->warnings;
}
int main(){
    static_assert(sizeof(void*)==4,"native Win32 x86 test only");
    static_assert(!std::is_copy_constructible<hud::NativeRegistrationOwner>::value,"stable pointer");
    static_assert(!std::is_move_constructible<hud::NativeRegistrationOwner>::value,"stable pointer");
    State s;
    hud::Registry host{&s,&Find,&Insert,&Warn};
    hud::NativeRegistrationOwner first(hud::Hooks{});
    s.owner=&first;s.host=&host;
    if(first.started() || first.Start({})!=hud::AddResult::Invalid || first.started())return 1;
    if(first.Start(host)!=hud::AddResult::Inserted || !first.started() ||
       s.inserts!=1 || s.warnings || s.converter!=first.interface() ||
       std::strcmp(s.name,"CHudTextureHandle"))return 2;
    // Repeat on the same owner must not invoke any registry callback.
    if(first.Start(host)!=hud::AddResult::Inserted || s.finds!=1 || s.inserts!=1)return 3;
    hud::NativeRegistrationOwner second(hud::Hooks{});
    if(second.Start(host)!=hud::AddResult::DuplicateIgnored || s.warnings!=1 ||
       s.inserts!=1 || s.converter!=first.interface())return 4;
    if(second.Start(host)!=hud::AddResult::DuplicateIgnored || s.warnings!=1)return 5;
    // Even without a once flag, a repeated key leaves the old pointer.
    if(hud::RegisterInto(host,"CHudTextureHandle",second.interface())!=
       hud::AddResult::DuplicateIgnored || s.warnings!=2 || s.inserts!=1 ||
       s.converter!=first.interface())return 6;
    if(hud::RegisterInto(host,nullptr,first.interface())!=hud::AddResult::Invalid ||
       hud::RegisterInto(host,"CHudTextureHandle",nullptr)!=hud::AddResult::Invalid)return 7;
    State failed;
    failed.reject_insert=true;
    hud::Registry rejected{&failed,&Find,&Insert,&Warn};
    hud::NativeRegistrationOwner third(hud::Hooks{});
    if(third.Start(rejected)!=hud::AddResult::InsertRejected || !third.started() ||
       failed.inserts!=1 || failed.converter)return 8;
    failed.reject_insert=false;
    if(third.Start(rejected)!=hud::AddResult::InsertRejected || failed.inserts!=1)return 9;
    // Reentrant registry find observes owner's once bit before callback.
    State recurse;
    hud::Registry reentrant{&recurse,&Find,&Insert,&Warn};
    hud::NativeRegistrationOwner fourth(hud::Hooks{});
    recurse.owner=&fourth;recurse.host=&reentrant;recurse.reenter=true;
    if(fourth.Start(reentrant)!=hud::AddResult::Inserted || recurse.finds!=1 ||
       recurse.inserts!=1 || recurse.warnings)return 10;
    std::puts("PASS original CHudTextureHandle duplicate-ignore, static pointer, reentrant flag, failed insert");
    return 0;
}
