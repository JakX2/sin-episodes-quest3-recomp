#include "native_server_map_entity.h"
#include <cstring>
extern "C" const sin_port::native_resupply::Api* __cdecl SiN_GetServerDynamicResupplyApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_map {
namespace {
bool delim(char c) noexcept {return c=='{'||c=='}'||c=='('||c==')'||c=='\'';}
bool space(char c) noexcept {return static_cast<signed char>(c)<=32;}
int __cdecl token(const char* s,unsigned n,unsigned at,Token* o) noexcept {
    if(!o)return 0;*o={};
    if(!s||at>n)return 0;
    for(;;){
        while(at<n&&s[at]&&space(s[at]))++at;
        if(at==n||!s[at]){o->kind=Kind::End;o->begin=o->next=at;return 1;}
        if(s[at]=='/'&&at+1<n&&s[at+1]=='/'){
            while(at<n&&s[at]&&s[at]!='\n')++at;
        }else break;
    }
    o->begin=at;unsigned used=0;
    auto push=[&](char ch){if(used>=token_capacity-1)return false;o->text[used++]=ch;return true;};
    auto fail=[&](){o->kind=Kind::Malformed;o->next=at;return 0;};
    if(s[at]=='"'){
        ++at;
        while(at<n&&s[at]&&s[at]!='"'){if(!push(s[at++]))return fail();}
        if(at==n||!s[at])return fail();++at;
    }else if(delim(s[at])){push(s[at++]);}
    else{
        while(at<n&&s[at]&&!space(s[at])&&!delim(s[at])){
            if(!push(s[at++]))return fail();
        }
    }
    o->kind=Kind::Word;o->next=at;return 1;
}
int __cdecl slice(const char* s,unsigned n,unsigned at,EntitySlice* out) noexcept {
    if(!out)return 0;*out={};
    Token t{};
    if(!token(s,n,at,&t)||t.kind!=Kind::Word||std::strcmp(t.text,"{"))return 0;
    out->begin=t.begin;out->body=t.next;
    unsigned depth=1;
    while(depth){
        if(!token(s,n,t.next,&t)||t.kind!=Kind::Word)return 0;
        // Original skip helper compares token text, even quoted braces.
        if(!std::strcmp(t.text,"{")){if(++depth>4096)return 0;}
        else if(!std::strcmp(t.text,"}"))--depth;
    }
    out->end=t.next;return 1;
}
bool next_pair(const char* s,unsigned n,unsigned& cursor,Pair& p,bool& end) noexcept {
    Token key{},value{};
    end=false;
    if(!token(s,n,cursor,&key)||key.kind!=Kind::Word)return false;
    // Original GetNextKey stops on first character '}', preserving cursor.
    if(key.text[0]=='}'){end=true;return true;}
    if(!token(s,n,key.next,&value)||value.kind!=Kind::Word||value.text[0]=='}')return false;
    std::memcpy(p.key,key.text,sizeof(p.key));std::memcpy(p.value,value.text,sizeof(p.value));
    unsigned len=static_cast<unsigned>(std::strlen(p.key));while(len&&p.key[len-1]==' ')p.key[--len]=0;
    cursor=value.next;return true;
}
bool valid_slice(const EntitySlice& e,unsigned n) noexcept{return e.begin<e.body&&e.body<e.end&&e.end<=n;}
int __cdecl lookup(const char* s,unsigned n,const EntitySlice* e,const char* key,char* out,unsigned cap,bool* found) noexcept {
    if(!s||!e||!valid_slice(*e,n)||!key||!out||!cap||!found)return 0;
    *found=false;out[0]=0;
    unsigned cursor=e->body;
    // Original FindKey does not trim keys and returns the first exact
    // case-sensitive match, independent of GetNextKey's normalization.
    for(;;){
        Token k{},v{};
        if(!token(s,e->end,cursor,&k)||k.kind!=Kind::Word)return 0;
        if(k.text[0]=='}')return 1;
        if(!token(s,e->end,k.next,&v)||v.kind!=Kind::Word||v.text[0]=='}')return 0;
        if(!std::strcmp(k.text,key)){
            if(std::strlen(v.text)>=cap)return 0;
            std::strcpy(out,v.text);*found=true;return 1;
        }
        cursor=v.next;
    }
}
using namespace native_resupply;
constexpr FactorySpec native_specs[]{
    {resupply_classname,Family::Resupply,resupply_factory_object,resupply_factory_vtable,resupply_extent},
    {ammo_types[0].classname,Family::Pickup,pickup_factory_specs[0].object,pickup_factory_specs[0].vtable,pickup_factory_specs[0].extent},
    {ammo_types[1].classname,Family::Pickup,pickup_factory_specs[1].object,pickup_factory_specs[1].vtable,pickup_factory_specs[1].extent},
    {ammo_types[2].classname,Family::Pickup,pickup_factory_specs[2].object,pickup_factory_specs[2].vtable,pickup_factory_specs[2].extent},
    {ammo_types[3].classname,Family::Pickup,pickup_factory_specs[3].object,pickup_factory_specs[3].vtable,pickup_factory_specs[3].extent},
    {ammo_types[4].classname,Family::Pickup,pickup_factory_specs[4].object,pickup_factory_specs[4].vtable,pickup_factory_specs[4].extent},
    {ammo_types[5].classname,Family::Pickup,pickup_factory_specs[5].object,pickup_factory_specs[5].vtable,pickup_factory_specs[5].extent},
    {health_classname,Family::Pickup,pickup_factory_specs[6].object,pickup_factory_specs[6].vtable,pickup_factory_specs[6].extent},
    {megahealth_classname,Family::Pickup,pickup_factory_specs[7].object,pickup_factory_specs[7].vtable,pickup_factory_specs[7].extent},
    {antigen_factories[0].classname,Family::Antigen,antigen_factories[0].object,antigen_factories[0].vtable,antigen_factories[0].extent},
    {antigen_factories[1].classname,Family::Antigen,antigen_factories[1].object,antigen_factories[1].vtable,antigen_factories[1].extent},
    {antigen_factories[2].classname,Family::Antigen,antigen_factories[2].object,antigen_factories[2].vtable,antigen_factories[2].extent}
};
bool equal_class(const char* a,const char* b) noexcept {
    if(!a||!b)return false;
    auto fold=[](unsigned char v){return v>='A'&&v<='Z'?v+32:v;};
    while(*a&&*b&&fold(*a)==fold(*b)){++a;++b;}return fold(*a)==fold(*b);
}
int __cdecl register_factories(FactoryDictionary* d) noexcept {
    if(!d||d->registered||d->count)return 0;
    for(const auto& spec:native_specs)d->slots[d->count++]=&spec;
    d->registered=true;return 1;
}
const FactorySpec* __cdecl find_factory(const FactoryDictionary* d,const char* name) noexcept {
    if(!d||!d->registered||d->count!=12||!name)return nullptr;
    for(unsigned i=0;i<12;++i){
        // Native descriptor pointers are immutable and authenticated.
        if(d->slots[i]!=&native_specs[i])return nullptr;
    }
    for(const auto& spec:native_specs)if(equal_class(name,spec.name))return &spec;
    return nullptr;
}
int __cdecl create_factory(const char* name,const FactoryBindings* b,FactoryResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!name||!b||!b->dictionary||!b->dictionary->registered||b->dictionary->count!=12)return 0;
    for(unsigned i=0;i<12;++i)if(b->dictionary->slots[i]!=&native_specs[i])return 0;
    o->spec=find_factory(b->dictionary,name);
    if(!o->spec){
        if(!b->foreign_create||!b->foreign_create(b->context,name,&o->actor,&o->bytes))return 0;
        o->completed=1;return 1;
    }
    o->matched=1;
    const auto* api=SiN_GetServerDynamicResupplyApi(1,sizeof(native_resupply::Api));if(!api)return 0;
    bool ok=false;
    if(o->spec->family==Family::Resupply){
        native_resupply::FactoryResult q{};ok=api->factory_create(name,b->resupply,&q)!=0;
        o->actor=q.actor;o->bytes=q.bytes;o->networkable=q.networkable;
    }else if(o->spec->family==Family::Pickup){
        PickupFactoryResult q{};ok=api->pickup_factory_create(name,b->pickup,&q)!=0;
        o->actor=q.actor;o->bytes=q.bytes;o->networkable=q.networkable;
    }else{
        AntigenFactoryResult q{};ok=api->antigen_factory_create(name,b->antigen,&q)!=0;
        o->actor=q.actor;o->bytes=q.bytes;o->networkable=q.networkable;
    }
    o->completed=ok;return int(ok);
}
int __cdecl parse(const char* s,unsigned n,const Bindings* b,Result* o) noexcept {
    if(!o)return 0;*o={};
    if(!s||!b||(!b->create&&!b->native_factory)||!b->key_value||!b->accept_entity)return 0;
    unsigned cursor=0;
    for(;;){
        Token top{};if(!token(s,n,cursor,&top))return 0;
        o->offset=top.begin;
        if(top.kind==Kind::End){o->completed=1;return 1;}
        if(o->entities>=4096)return 0;
        EntitySlice e{};if(!slice(s,n,cursor,&e))return 0;
        ++o->entities;
        char classname[token_capacity]{};bool found=false;
        if(!lookup(s,n,&e,"classname",classname,sizeof(classname),&found))return 0;
        if(!found){++o->missing_classname;if(!b->missing||!b->missing(b->context,nullptr))return 0;return 0;}
        bool create=true;
        if(b->should_create&&!b->should_create(b->context,classname,&create))return 0;
        void* actor=nullptr;
        if(create){
            if(b->create){if(!b->create(b->context,classname,&actor))return 0;}
            else{
                if(!create_factory(classname,b->native_factory,&o->last_factory))return 0;
                actor=o->last_factory.actor;
                if(o->last_factory.matched)++o->native_creates;else ++o->foreign_creates;
            }
            if(!b->should_create){
                if(!b->clear_factory_global||!b->clear_factory_global(b->context))return 0;
                ++o->clears;
            }
            if(!actor){++o->missing_factory;if(!b->missing||!b->missing(b->context,classname))return 0;}
            else ++o->created;
        }else ++o->filtered;
        unsigned p=e.body;
        for(;;){
            Pair pair{};bool end=false;
            if(!next_pair(s,e.end,p,pair,end))return 0;if(end)break;
            ++o->pairs;
            if(actor&&!b->key_value(b->context,actor,pair.key,pair.value))return 0;
        }
        if(actor&&!b->accept_entity(b->context,actor,s,n,&e))return 0;
        cursor=e.end;
    }
}
const Api api{1,sizeof(Api),&token,&slice,&lookup,&parse,&register_factories,&find_factory,&create_factory};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerMapEntityApi=_SiN_GetServerMapEntityApi")
extern "C" __declspec(dllexport) const sin_port::native_map::Api* __cdecl SiN_GetServerMapEntityApi(std::uint32_t v,std::uint32_t n) noexcept {
    using namespace sin_port::native_map;return v==1&&n==sizeof(Api)?&api:nullptr;
}
