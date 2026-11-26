#include "native_server_map_entity.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <string>
using namespace sin_port::native_map;
struct Fixture {unsigned actors[3]{},creates=0,clears=0,missing=0,accepts=0;std::string keys;};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetApi>(GetProcAddress(server,export_name));assert(get&&!GetProcAddress(client,export_name));auto* api=get(1,sizeof(Api));assert(api&&!get(2,sizeof(Api)));
    const char sample[]="// skip\n{ \"targetname\" \"antigen station\" \"classname\" \"item_antigen_dispenser\" \"amount  \" \"42\" \"OnUse\" \"relay,Trigger,,0,-1\" }\n{ classname unknown }\n{ classname item_megahealth }";
    Token tok{};EntitySlice e{};char value[2048]{};bool found=false;
    assert(api->token(sample,sizeof(sample),0,&tok)&&!std::strcmp(tok.text,"{")&&tok.begin==8);
    assert(api->slice(sample,sizeof(sample),0,&e)&&e.body==9);
    assert(api->lookup(sample,sizeof(sample),&e,"classname",value,sizeof(value),&found)&&found&&!std::strcmp(value,"item_antigen_dispenser"));
    assert(api->lookup(sample,sizeof(sample),&e,"Classname",value,sizeof(value),&found)&&!found);
    assert(api->lookup(sample,sizeof(sample),&e,"amount",value,sizeof(value),&found)&&!found);
    assert(api->lookup(sample,sizeof(sample),&e,"amount  ",value,sizeof(value),&found)&&found&&!std::strcmp(value,"42"));
    Fixture f;Bindings b{};b.context=&f;
    b.create=[](void* p,const char* cls,void** a) noexcept ->int{auto& f=*static_cast<Fixture*>(p);++f.creates;*a=std::strcmp(cls,"unknown")?&f.actors[f.creates-1]:nullptr;return 1;};
    b.clear_factory_global=[](void* p) noexcept ->int{++static_cast<Fixture*>(p)->clears;return 1;};
    b.key_value=[](void* p,void*,const char* k,const char* v) noexcept ->int{auto& f=*static_cast<Fixture*>(p);f.keys+=k;f.keys+='=';f.keys+=v;f.keys+=';';return 1;};
    b.missing=[](void* p,const char* cls) noexcept ->int{assert(cls&&!std::strcmp(cls,"unknown"));++static_cast<Fixture*>(p)->missing;return 1;};
    b.accept_entity=[](void* p,void*,const char* s,unsigned n,const EntitySlice* e) noexcept ->int{assert(e->end<=n&&s[e->begin]=='{'&&s[e->end-1]=='}');++static_cast<Fixture*>(p)->accepts;return 1;};
    Result r{};assert(api->parse(sample,sizeof(sample),&b,&r)&&r.completed&&r.entities==3&&r.pairs==6&&r.created==2&&r.missing_factory==1&&r.clears==3&&f.accepts==2);
    assert(f.keys=="targetname=antigen station;classname=item_antigen_dispenser;amount=42;OnUse=relay,Trigger,,0,-1;classname=item_megahealth;");
    Fixture filtered;b.context=&filtered;b.should_create=[](void*,const char* cls,bool* yes) noexcept ->int{*yes=!std::strcmp(cls,"item_megahealth");return 1;};
    assert(api->parse(sample,sizeof(sample),&b,&r)&&r.filtered==2&&r.created==1&&!r.clears&&filtered.keys=="classname=item_megahealth;");
    const char broken[]="{ \"classname\" \"unterminated";assert(!api->slice(broken,sizeof(broken),0,&e));
    std::string huge(2048,'x');assert(!api->token(huge.c_str(),static_cast<unsigned>(huge.size()),0,&tok)&&tok.kind==Kind::Malformed);
    const char delimiters[]="(abc)'";assert(api->token(delimiters,sizeof(delimiters),0,&tok)&&!std::strcmp(tok.text,"("));assert(api->token(delimiters,sizeof(delimiters),tok.next,&tok)&&!std::strcmp(tok.text,"abc"));
    FreeLibrary(server);FreeLibrary(client);
}
