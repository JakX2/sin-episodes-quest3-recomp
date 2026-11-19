// Original-SDK algorithm tests, intentionally no live engine registration.
#include "client_hud_texture_converter_core.h"
#include <cstdio>
#include <cstring>
#include <string>
namespace hud=sin_independent::hud_texture;
struct Icon {const char* name;};
struct Entry {const char* name;const char* initial;};
struct Values {std::string property;std::string value;};
struct State {
    hud::Handle handle;
    Icon first{"first_icon"},second{"second_icon"};
    int writes=0,reads=0,lookups=0,registers=0;
    const char* registeredType=nullptr;
    hud::Converter* registeredConverter=nullptr;
    hud::OneTimeOwner* owner=nullptr;
};
static hud::Handle* Lookup(void* ctx,void* panel,void* entry) noexcept {
    if(!panel || !entry)return nullptr;
    return &static_cast<State*>(ctx)->handle;
}
static const char* EntryName(void*,void* e) noexcept {
    return static_cast<Entry*>(e)->name;
}
static const char* EntryDefault(void*,void* e) noexcept {
    return static_cast<Entry*>(e)->initial;
}
static const char* Read(void* ctx,void* kv,const char* key) noexcept {
    auto* st=static_cast<State*>(ctx);
    ++st->reads;
    auto* vals=static_cast<Values*>(kv);
    return vals && vals->property==key?vals->value.c_str():nullptr;
}
static void Write(void* ctx,void* kv,const char* key,const char* value) noexcept {
    auto* st=static_cast<State*>(ctx);
    ++st->writes;
    auto* vals=static_cast<Values*>(kv);
    vals->property=key;
    vals->value=value?value:"";
}
static void* Find(void* ctx,const char* name) noexcept {
    auto* st=static_cast<State*>(ctx);
    ++st->lookups;
    if(!std::strcmp(name,st->first.name))return &st->first;
    if(!std::strcmp(name,st->second.name))return &st->second;
    return nullptr;
}
static const char* ShortName(void*,void* icon) noexcept {
    return static_cast<Icon*>(icon)->name;
}
static void Register(void* ctx,const char* type,hud::Converter* converter) noexcept {
    auto* st=static_cast<State*>(ctx);
    ++st->registers;
    st->registeredType=type;
    st->registeredConverter=converter;
    // Reentrancy must observe initialized=true and not register twice.
    if(st->owner) st->owner->Register(ctx,&Register);
}
int main() {
    State state;
    hud::Hooks hooks{&state,&Lookup,&EntryName,&EntryDefault,&Read,&Write,&Find,&ShortName};
    hud::OneTimeOwner owner(hooks);
    state.owner=&owner;
    if(owner.initialized() || owner.Register(&state,nullptr))return 1;
    if(!owner.Register(&state,&Register) || !owner.initialized() ||
       state.registers!=1 || state.registeredConverter!=owner.converter() ||
       std::strcmp(state.registeredType,hud::kTypeName))return 2;
    if(owner.Register(&state,&Register) || state.registers!=1)return 3;
    Entry entry{"myTexture","first_icon"};
    Values kv{"myTexture","second_icon"};
    int panel=1;
    if(!owner.converter()->InitFromDefault(&panel,&entry) ||
       state.handle.Get()!=&state.first)return 4;
    if(!owner.converter()->GetData(&panel,&kv,&entry) ||
       kv.value!="first_icon" || state.writes!=1)return 5;
    kv.value="second_icon";
    if(!owner.converter()->SetData(&panel,&kv,&entry) ||
       state.handle.Get()!=&state.second || state.reads!=1)return 6;
    kv.value="";
    if(!owner.converter()->SetData(&panel,&kv,&entry) ||
       state.handle.Get()!=nullptr)return 7;
    kv.value="does_not_exist";
    if(!owner.converter()->SetData(&panel,&kv,&entry) ||
       state.handle.Get()!=nullptr)return 8;
    entry.initial=nullptr;
    if(!owner.converter()->InitFromDefault(&panel,&entry) ||
       state.handle.Get()!=nullptr)return 9;
    if(owner.converter()->InitFromDefault(nullptr,&entry) ||
       owner.converter()->GetData(nullptr,&kv,&entry))return 10;
    state.handle.Set(&state.first);
    if(!owner.converter()->GetData(&panel,&kv,&entry) ||
       kv.value!="first_icon")return 11;
    hud::OneTimeOwner incomplete(hud::Hooks{});
    if(incomplete.converter()->GetData(&panel,&kv,&entry) ||
       incomplete.converter()->SetData(&panel,&kv,&entry) ||
       incomplete.converter()->InitFromDefault(&panel,&entry))return 12;
    std::puts("PASS owned CHudTextureHandle GetData/SetData/InitFromDefault and one-time registration");
    return 0;
}
