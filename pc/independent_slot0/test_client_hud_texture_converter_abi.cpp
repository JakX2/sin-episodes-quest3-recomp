// Win32 x86 probe of the exact three-slot historical converter ABI.
// Does not import or call any proprietary VGUI engine symbols.
#include "client_hud_texture_converter_abi.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <type_traits>
#include <string>
namespace hud=sin_independent::hud_texture;
struct Icon {const char* shortName;};
struct Entry {const char* key;const char* defaultValue;};
struct Kv {std::string key;std::string value;};
struct Fixture {
    hud::Handle handle;
    Icon blue{"blue"},red{"red"};
};
static hud::Handle* Lookup(void* ctx,void* panel,void* entry) noexcept {
    return panel && entry?&static_cast<Fixture*>(ctx)->handle:nullptr;
}
static const char* Name(void*,void* entry) noexcept {
    return static_cast<Entry*>(entry)->key;
}
static const char* Default(void*,void* entry) noexcept {
    return static_cast<Entry*>(entry)->defaultValue;
}
static const char* Read(void*,void* kv,const char*) noexcept {
    return static_cast<Kv*>(kv)->value.c_str();
}
static void Write(void*,void* kv,const char* name,const char* value) noexcept {
    auto& v=*static_cast<Kv*>(kv);
    v.key=name;v.value=value;
}
static void* Find(void* ctx,const char* name) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    return !std::strcmp(name,"blue")?&f.blue:
        !std::strcmp(name,"red")?&f.red:nullptr;
}
static const char* Short(void*,void* ptr) noexcept {
    return static_cast<Icon*>(ptr)->shortName;
}
int main() {
    static_assert(sizeof(void*)==4,"Only MSVC Win32 x86 original GUI ABI");
    static_assert(std::is_abstract<hud::AnimationConverterAbi>::value,
                  "Interface must provide three pure virtual methods");
    Fixture fixture;
    hud::Hooks hooks{&fixture,&Lookup,&Name,&Default,&Read,&Write,&Find,&Short};
    hud::NativeConverter concrete(hooks);
    hud::AnimationConverterAbi* object=&concrete;
    void** vtable=*reinterpret_cast<void***>(object);
    if(!vtable || !vtable[0] || !vtable[1] || !vtable[2])return 1;
    using Get=void (__thiscall *)(void*,void*,void*,void*);
    using Init=void (__thiscall *)(void*,void*,void*);
    Entry entry{"texture","blue"};Kv kv{"texture","red"};int panel=7;
    reinterpret_cast<Init>(vtable[2])(object,&panel,&entry);
    if(!concrete.lastResult() || fixture.handle.Get()!=&fixture.blue)return 2;
    reinterpret_cast<Get>(vtable[0])(object,&panel,&kv,&entry);
    if(!concrete.lastResult() || kv.value!="blue")return 3;
    kv.value="red";
    reinterpret_cast<Get>(vtable[1])(object,&panel,&kv,&entry);
    if(!concrete.lastResult() || fixture.handle.Get()!=&fixture.red)return 4;
    entry.defaultValue="";
    reinterpret_cast<Init>(vtable[2])(object,&panel,&entry);
    if(!concrete.lastResult() || fixture.handle.Get())return 5;
    reinterpret_cast<Init>(vtable[2])(object,nullptr,&entry);
    if(concrete.lastResult())return 6;
    std::puts("PASS SDK2004 three-slot CHudTextureHandle converter MSVC x86 virtual ABI");
    return 0;
}
