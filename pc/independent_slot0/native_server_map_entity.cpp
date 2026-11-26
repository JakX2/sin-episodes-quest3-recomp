#include "native_server_map_entity.h"
#include "native_server_npc_active_weapon_step.h"
#include <cmath>
#include "server_fixed_difficulty_service.h"
extern "C" const sin_port::fixed_service::GlobalStateApi* __cdecl SiN_GetGlobalStateApi(std::uint32_t,std::uint32_t) noexcept;
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <charconv>
extern "C" const sin_port::native_resupply::Api* __cdecl SiN_GetServerDynamicResupplyApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerEntityVoidOutputApi* __cdecl SiN_GetServerEntityVoidOutputApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerEntityNamedEventQueueApi* __cdecl SiN_GetServerEntityNamedEventQueueApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerNpcOuterCadenceApi* __cdecl SiN_GetServerNpcOuterCadenceApi(std::uint32_t,std::uint32_t) noexcept;
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
constexpr FactorySpec relay_spec{"logic_relay",Family::Relay,0x224fda88,0x224228bc,relay_extent};
constexpr FactorySpec auto_spec{"logic_auto",Family::LogicAuto,0x224fb288,0x2241efcc,auto_extent};
constexpr FactorySpec timer_spec{"logic_timer",Family::LogicTimer,0x224fb604,0x2241f590,timer_extent};
constexpr FactorySpec branch_spec{"logic_branch",Family::LogicBranch,0x224fb628,0x2241f5fc,branch_extent};
constexpr FactorySpec case_spec{"logic_case",Family::LogicCase,0x224fb620,0x2241f5e4,case_extent};
bool valid_dictionary(const FactoryDictionary* d) noexcept {
    if(!d||!d->registered||d->count<12||d->count>17)return false;
    for(unsigned i=0;i<12;++i)if(d->slots[i]!=&native_specs[i])return false;
    bool relay=false,logic_auto=false,logic_timer=false,logic_branch=false,logic_case=false;
    for(unsigned i=12;i<17;++i){
        if(i>=d->count){if(d->slots[i])return false;continue;}
        if(d->slots[i]==&relay_spec&&!relay)relay=true;
        else if(d->slots[i]==&auto_spec&&!logic_auto)logic_auto=true;
        else if(d->slots[i]==&timer_spec&&!logic_timer)logic_timer=true;
        else if(d->slots[i]==&branch_spec&&!logic_branch)logic_branch=true;
        else if(d->slots[i]==&case_spec&&!logic_case)logic_case=true;
        else return false;
    }
    return true;
}
int register_optional(FactoryDictionary* d,const FactorySpec* spec) noexcept {
    if(!valid_dictionary(d)||d->count==17)return 0;
    for(unsigned i=12;i<d->count;++i)if(d->slots[i]==spec)return 0;
    d->slots[d->count++]=spec;return 1;
}
int __cdecl register_relay(FactoryDictionary* d) noexcept {return register_optional(d,&relay_spec);}
int __cdecl register_auto(FactoryDictionary* d) noexcept {return register_optional(d,&auto_spec);}
int __cdecl register_timer(FactoryDictionary* d) noexcept {return register_optional(d,&timer_spec);}
int __cdecl register_branch(FactoryDictionary* d) noexcept {return register_optional(d,&branch_spec);}
int __cdecl register_case(FactoryDictionary* d) noexcept {return register_optional(d,&case_spec);}
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
    if(!valid_dictionary(d)||!name)return nullptr;
    for(unsigned i=0;i<d->count;++i)if(equal_class(name,d->slots[i]->name))return d->slots[i];
    return nullptr;
}
int __cdecl create_factory(const char* name,const FactoryBindings* b,FactoryResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!name||!b||!valid_dictionary(b->dictionary))return 0;
    o->spec=find_factory(b->dictionary,name);
    if(!o->spec){
        if(!b->foreign_create||!b->foreign_create(b->context,name,&o->actor,&o->bytes))return 0;
        o->completed=1;return 1;
    }
    o->matched=1;
    if(o->spec->family==Family::Relay){
        const auto* r=b->relay;
        if(!r||!r->allocate||!r->base_construct||!r->initialize||
           !r->allocate(r->context,relay_extent,&o->actor,&o->bytes)||!o->actor||o->bytes<relay_extent)return 0;
        auto* a=static_cast<unsigned char*>(o->actor);
        // Original 2217e080 inlines 2217e000: base constructor argument 1,
        // primary vtable, three output words, then virtual Initialize.
        if(!r->base_construct(r->context,a,1))return 0;
        unsigned head=0;std::memcpy(&head,a+0x3a8,4);
        if(head||a[0x3ac]||a[0x3ad])return 0;
        const unsigned zero=0,invalid=0xffffffffu,vt=relay_vtable;
        std::memcpy(a,&vt,4);std::memcpy(a+0x394,&zero,4);
        std::memcpy(a+0x3a0,&invalid,4);std::memcpy(a+0x3a4,&zero,4);
        if(!r->initialize(r->context,a,name))return 0;
        o->networkable=a+0x160;o->completed=1;return 1;
    }
    if(o->spec->family==Family::LogicCase){
        const auto* r=b->logic_case;
        if(!r||!r->allocate||!r->base_construct||!r->initialize||
           !r->allocate(r->context,case_extent,&o->actor,&o->bytes)||!o->actor||o->bytes<case_extent)return 0;
        auto* a=static_cast<unsigned char*>(o->actor);
        // Original 2217d910 base(1), sixteen case outputs and default.
        if(!r->base_construct(r->context,a,1))return 0;
        for(unsigned off=0x394;off<0x3d4;off+=4){
            unsigned token=0;std::memcpy(&token,a+off,4);if(token)return 0;
        }
        for(unsigned off=0x3d4;off<=0x554;off+=0x18){
            unsigned head=0;std::memcpy(&head,a+off+0x14,4);if(head)return 0;
        }
        const unsigned zero=0,invalid=0xffffffffu,vt=case_vtable;
        std::memcpy(a,&vt,4);
        for(unsigned off=0x3d4;off<=0x554;off+=0x18){
            std::memcpy(a+off,&zero,4);std::memcpy(a+off+0xc,&invalid,4);std::memcpy(a+off+0x10,&zero,4);
        }
        if(!r->initialize(r->context,a,name))return 0;
        o->networkable=a+0x160;o->completed=1;return 1;
    }
    if(o->spec->family==Family::LogicBranch){
        const auto* r=b->logic_branch;
        if(!r||!r->allocate||!r->base_construct||!r->initialize||
           !r->allocate(r->context,branch_extent,&o->actor,&o->bytes)||!o->actor||o->bytes<branch_extent)return 0;
        auto* a=static_cast<unsigned char*>(o->actor);
        if(!r->base_construct(r->context,a,1))return 0;
        if(a[0x394])return 0;
        for(unsigned off=0x398;off<=0x3b0;off+=0x18){
            unsigned head=0;std::memcpy(&head,a+off+0x14,4);if(head)return 0;
        }
        const unsigned zero=0,invalid=0xffffffffu,vt=branch_vtable;
        std::memcpy(a,&vt,4);
        for(unsigned off=0x398;off<=0x3b0;off+=0x18){
            std::memcpy(a+off,&zero,4);std::memcpy(a+off+0xc,&invalid,4);std::memcpy(a+off+0x10,&zero,4);
        }
        if(!r->initialize(r->context,a,name))return 0;
        o->networkable=a+0x160;o->completed=1;return 1;
    }
    if(o->spec->family==Family::LogicTimer){
        const auto* r=b->logic_timer;
        if(!r||!r->allocate||!r->base_construct||!r->initialize||
           !r->allocate(r->context,timer_extent,&o->actor,&o->bytes)||!o->actor||o->bytes<timer_extent)return 0;
        auto* a=static_cast<unsigned char*>(o->actor);
        // Original 2217d330 writes three output triplets after base(1).
        if(!r->base_construct(r->context,a,1))return 0;
        for(unsigned off=0x394;off<=0x3c4;off+=0x18){
            unsigned head=0;std::memcpy(&head,a+off+0x14,4);if(head)return 0;
        }
        // Fresh owner storage must not retain prior timer state.
        for(unsigned off=0x3dc;off<timer_extent;++off)if(a[off])return 0;
        const unsigned zero=0,invalid=0xffffffffu,vt=timer_vtable;
        std::memcpy(a,&vt,4);
        for(unsigned off=0x394;off<=0x3c4;off+=0x18){
            std::memcpy(a+off,&zero,4);std::memcpy(a+off+0xc,&invalid,4);std::memcpy(a+off+0x10,&zero,4);
        }
        if(!r->initialize(r->context,a,name))return 0;
        o->networkable=a+0x160;o->completed=1;return 1;
    }
    if(o->spec->family==Family::LogicAuto){
        const auto* r=b->logic_auto;
        if(!r||!r->allocate||!r->base_construct||!r->initialize||
           !r->allocate(r->context,auto_extent,&o->actor,&o->bytes)||!o->actor||o->bytes<auto_extent)return 0;
        auto* a=static_cast<unsigned char*>(o->actor);
        // Original 221788e0: base constructor argument 0, seven output
        // triplets. The allocator/owner supplies untouched zeroed heads.
        if(!r->base_construct(r->context,a,0))return 0;
        unsigned gate=0;std::memcpy(&gate,a+0x43c,4);if(gate)return 0;
        for(unsigned off=0x394;off<=0x424;off+=0x18){
            unsigned head=0;std::memcpy(&head,a+off+0x14,4);if(head)return 0;
        }
        const unsigned zero=0,invalid=0xffffffffu,vt=auto_vtable;
        std::memcpy(a,&vt,4);
        for(unsigned off=0x394;off<=0x424;off+=0x18){
            std::memcpy(a+off,&zero,4);std::memcpy(a+off+0xc,&invalid,4);std::memcpy(a+off+0x10,&zero,4);
        }
        if(!r->initialize(r->context,a,name))return 0;
        o->networkable=a+0x160;o->completed=1;return 1;
    }
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
template<class T>void write_field(void* a,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(a)+off,&v,sizeof(v));}
unsigned entity_vtable(void* a) noexcept {unsigned vt=0;std::memcpy(&vt,a,4);return vt;}
int __cdecl destroy_relay(void* actor,unsigned bytes,void* networkable,const RelayFactoryBindings* b) noexcept {
    if(!networkable)return 1;
    if(!actor||bytes<relay_extent||entity_vtable(actor)!=relay_vtable||
       networkable!=static_cast<unsigned char*>(actor)+0x160||!b||!b->release_networkable)return 0;
    // Original factory destroy 2217dff0 calls networkable virtual +18.
    // The owner may synchronously destroy the actor; never read it again.
    return b->release_networkable(b->context,networkable);
}
int __cdecl destroy_auto(void* actor,unsigned bytes,void* networkable,const RelayFactoryBindings* b) noexcept {
    if(!networkable)return 1;
    if(!actor||bytes<auto_extent||entity_vtable(actor)!=auto_vtable||
       networkable!=static_cast<unsigned char*>(actor)+0x160||!b||!b->release_networkable)return 0;
    return b->release_networkable(b->context,networkable);
}
int __cdecl destroy_timer(void* actor,unsigned bytes,void* networkable,const RelayFactoryBindings* b) noexcept {
    if(!networkable)return 1;
    if(!actor||bytes<timer_extent||entity_vtable(actor)!=timer_vtable||
       networkable!=static_cast<unsigned char*>(actor)+0x160||!b||!b->release_networkable)return 0;
    return b->release_networkable(b->context,networkable);
}
int __cdecl destroy_branch(void* actor,unsigned bytes,void* networkable,const RelayFactoryBindings* b) noexcept {
    if(!networkable)return 1;
    if(!actor||bytes<branch_extent||entity_vtable(actor)!=branch_vtable||
       networkable!=static_cast<unsigned char*>(actor)+0x160||!b||!b->release_networkable)return 0;
    return b->release_networkable(b->context,networkable);
}
int __cdecl destroy_case(void* actor,unsigned bytes,void* networkable,const RelayFactoryBindings* b) noexcept {
    if(!networkable)return 1;
    if(!actor||bytes<case_extent||entity_vtable(actor)!=case_vtable||
       networkable!=static_cast<unsigned char*>(actor)+0x160||!b||!b->release_networkable)return 0;
    return b->release_networkable(b->context,networkable);
}
bool known_map_vtable(unsigned vt) noexcept {
    if(vt==case_vtable||vt==branch_vtable||vt==timer_vtable||vt==auto_vtable||vt==relay_vtable||vt==resupply_vtable||vt==antigen_vtable||vt==antigen_canister_vtable||vt==vial_vtable||vt==megahealth_vtable)return true;
    for(const auto& item:ammo_pickups)if(vt==item.vtable)return true;return false;
}
// Original 22154830: five comma-delimited fields, no escaping or trim.
// Original 22285ea0 returns the character after a separator, otherwise
// the terminating NUL; empty input writes an empty token.
bool output_field(const char*& p,char (&field)[256]) noexcept {
    unsigned n=0;
    while(*p&&*p!=','){
        if(n==255)return false;
        field[n++]=*p++;
    }
    field[n]=0;if(*p==',')++p;return true;
}
bool attach_output(void* output,const char* value,const KeyBindings* keys) noexcept {
    const auto* b=keys->native_output;
    if(!b||!b->next_id||!b->allocate||!b->release||!keys->intern)return false;
    char fields[5][256]{};
    for(auto& field:fields)if(!output_field(value,field))return false;
    const float delay=fields[3][0]?static_cast<float>(std::strtod(fields[3],nullptr)):0.0f;
    errno=0;const long long count=fields[4][0]?std::strtoll(fields[4],nullptr,10):-1;
    if(errno==ERANGE||count<INT_MIN||count>INT_MAX)return false;
    void* row=nullptr;if(!b->allocate(b->context,0x1c,&row)||!row)return false;
    const auto addr=reinterpret_cast<std::uintptr_t>(row);
    if(addr>UINT32_MAX||(addr&3u)){b->release(b->context,row);return false;}
    unsigned head=0;std::memcpy(&head,static_cast<unsigned char*>(output)+0x14,4);
    if(head==static_cast<unsigned>(addr)){b->release(b->context,row);return false;}
    const unsigned id=++*b->next_id;
    unsigned target=0,input=0,parameter=0;
    const bool ok=(!fields[0][0]||keys->intern(keys->context,fields[0],&target))&&
        keys->intern(keys->context,fields[1][0]?fields[1]:"Use",&input)&&
        (!fields[2][0]||keys->intern(keys->context,fields[2],&parameter));
    if(!ok){b->release(b->context,row);return false;}
    write_field(row,0,target);write_field(row,4,input);write_field(row,8,parameter);
    write_field(row,0xc,delay);write_field(row,0x10,static_cast<int>(count?count:-1));
    write_field(row,0x14,id);write_field(row,0x18,head);
    write_field(output,0x14,static_cast<unsigned>(addr));return true;
}
int __cdecl key_value(void* actor,const char* key,const char* value,const KeyBindings* b,KeyResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!actor||!key||!value||!b||!b->extent)return 0;
    unsigned bytes=0;if(!b->extent(b->context,actor,&bytes)||bytes<4)return 0;
    const unsigned vt=entity_vtable(actor);o->known_class=known_map_vtable(vt);
    auto done=[&](){o->completed=1;return 1;};
    auto foreign=[&](const char* name){
        bool handled=false;
        if(!b->foreign||!b->foreign(b->context,actor,name,value,&handled))return 0;
        o->foreign=1;o->handled=handled;return done();
    };
    if(!o->known_class)return foreign(key);
    // Dynamic resupply override checks bounds BEFORE the base # suffix
    // normalization. Both exact keys are accepted and ignored.
    if(vt==resupply_vtable&&(equal_class(key,"mins")||equal_class(key,"maxs"))){
        if(!b->resupply_bounds_warning||!b->resupply_bounds_warning(b->context,actor))return 0;
        o->ignored_bounds=o->handled=1;return done();
    }
    char name[token_capacity]{};
    unsigned length=0;while(key[length]&&key[length]!='#'){
        if(length==sizeof(name)-1)return 0;name[length]=key[length];++length;
    }
    struct Spec {const char* key;unsigned offset,type;};
    // Original CBaseEntity KeyValue special targetname leaf and common
    // scalar/string datamap entries. Other base fields retain owner dispatch.
    constexpr Spec common[]{
        {"classname",0xc,2},{"globalname",0x10,2},{"parentname",0x14,2},{"targetname",0xc0,2},
        {"speed",0x18,1},{"spawnflags",0xb4,5}
    };
    constexpr Spec dispenser[]{
        {"Antigen",0x49c,1},{"OnCanisterInserted",0x4b8,11},{"OnCanisterEjected",0x4d0,11}
    };
    constexpr Spec canister[]{
        {"Percent",0x628,1},{"ContainerFlags",0x62c,5},{"OnExplode",0x66c,11}
    };
    constexpr Spec relay[]{
        {"StartDisabled",0x3ac,6},{"OnTrigger",0x394,11}
    };
    constexpr Spec auto_keys[]{
        {"globalstate",0x43c,2},{"OnMapSpawn",0x394,11},{"OnViolenceLow",0x3ac,11},
        {"OnViolenceHigh",0x3c4,11},{"OnNewGame",0x3dc,11},{"OnLoadGame",0x3f4,11},
        {"OnMapTransition",0x40c,11},{"OnBackgroundMap",0x424,11}
    };
    constexpr Spec timer_keys[]{
        {"StartDisabled",0x3dc,5},{"RefireTime",0x3e0,1},{"UseRandomTime",0x3e8,5},
        {"LowerRandomBound",0x3ec,1},{"UpperRandomBound",0x3f0,1},
        {"OnTimer",0x394,11},{"OnTimerHigh",0x3ac,11},{"OnTimerLow",0x3c4,11}
    };
    constexpr Spec branch_keys[]{
        {"InitialValue",0x394,6},{"SetValue",0x394,6},
        {"OnTrue",0x398,11},{"OnFalse",0x3b0,11}
    };
    Spec numbered{};
    const Spec* spec=nullptr;
    auto match=[&](const auto& table){for(const auto& s:table)if(equal_class(name,s.key)){spec=&s;break;}};
    if(vt==antigen_vtable)match(dispenser);else if(vt==antigen_canister_vtable)match(canister);else if(vt==relay_vtable)match(relay);else if(vt==auto_vtable)match(auto_keys);else if(vt==timer_vtable)match(timer_keys);else if(vt==branch_vtable)match(branch_keys);
    else if(vt==case_vtable){
        if(equal_class(name,"OnDefault")){numbered={nullptr,0x554,11};spec=&numbered;}
        else for(unsigned i=1;i<=16;++i){
            char key[]="Case00",out[]="OnCase00";
            key[4]=out[6]=char('0'+i/10);key[5]=out[7]=char('0'+i%10);
            if(equal_class(name,key)){numbered={nullptr,0x394+4*(i-1),2};spec=&numbered;break;}
            if(equal_class(name,out)){numbered={nullptr,0x3d4+0x18*(i-1),11};spec=&numbered;break;}
        }
    }
    if(!spec){match(common);if(spec)o->base_field=1;}
    if(!spec)return foreign(name);
    o->offset=spec->offset;o->field_type=spec->type;
    if(bytes<o->offset+(spec->type==6?1u:4u))return 0;
    if(spec->type==2){
        unsigned token=0;if(!b->intern||!b->intern(b->context,value,&token))return 0;
        write_field(actor,o->offset,token);o->interned=1;
    }else if(spec->type==1){
        // Original atof -> float, accepts numeric prefix and empty->0.
        const float scalar=static_cast<float>(std::strtod(value,nullptr));
        write_field(actor,o->offset,scalar);o->scalar=1;
    }else if(spec->type==5||spec->type==6){
        // MSVCRT atoi overflow is not a portable C++ contract. Bound the
        // conversion rather than relying on host undefined behavior.
        errno=0;const long long scalar=std::strtoll(value,nullptr,10);
        if(errno==ERANGE||scalar<INT_MIN||scalar>INT_MAX)return 0;
        if(spec->type==6)write_field(actor,o->offset,static_cast<unsigned char>(scalar!=0));
        else write_field(actor,o->offset,static_cast<int>(scalar));o->scalar=1;
    }else{
        auto* output=static_cast<unsigned char*>(actor)+o->offset;
        if(b->native_output){if(bytes-o->offset<0x18||!attach_output(output,value,b))return 0;}
        else if(!b->output||!b->output(b->context,actor,output,value))return 0;
        o->output=1;
    }
    o->handled=1;return done();
}
using Collector=int (__cdecl *)(void*,void*,const char*,unsigned,const EntitySlice*) noexcept;
int parse_impl(const char* s,unsigned n,const Bindings* b,Result* o,Collector collect,void* context) noexcept {
    if(!o)return 0;*o={};
    if(!s||!b||(!b->create&&!b->native_factory)||(!b->key_value&&!b->native_keys)||(!collect&&!b->accept_entity))return 0;
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
            if(actor){
                if(b->key_value){if(!b->key_value(b->context,actor,pair.key,pair.value))return 0;}
                else {KeyResult key{};if(!key_value(actor,pair.key,pair.value,b->native_keys,&key))return 0;}
                // Original base map parser ignores the bool KeyValue return.
            }
        }
        if(actor){if(b->accept_entity&&!b->accept_entity(b->context,actor,s,n,&e))return 0;if(collect&&!collect(context,actor,s,n,&e))return 0;}
        cursor=e.end;
    }
}
int __cdecl parse(const char* s,unsigned n,const Bindings* b,Result* o) noexcept {
    return parse_impl(s,n,b,o,nullptr,nullptr);
}
template<class T>T read_field(const void* a,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(a)+off,sizeof(v));return v;}
int priority(const char* name) noexcept {
    struct Entry {const char* name;int value;};
    constexpr Entry entries[]{
        {"func_wall",10},{"scripted_sequence",9},{"phys_hinge",8},{"phys_ballsocket",8},
        {"phys_slideconstraint",8},{"phys_constraint",8},{"phys_pulleyconstraint",8},
        {"phys_lengthconstraint",8},{"phys_ragdollconstraint",8},{"info_mass_center",8},{"prop_physics",7}
    };
    for(const auto& e:entries)if(equal_class(name,e.name))return e.value;return -1;
}
int __cdecl compare_rows(const void* a,const void* b) noexcept {
    const auto& x=*static_cast<const SpawnRow*>(a);const auto& y=*static_cast<const SpawnRow*>(b);
    if(x.depth!=y.depth)return x.depth<y.depth?-1:1;
    return x.priority==y.priority?0:x.priority>y.priority?-1:1;
}
bool extent_ok(const SpawnBindings& b,void* a,unsigned minimum,unsigned& n) noexcept {
    return a&&b.extent&&b.extent(b.context,a,&n)&&n>=minimum;
}
bool find_parent(const SpawnBindings& b,SpawnResult& o,void* a,void** parent) noexcept {
    *parent=nullptr;unsigned n=0;if(!extent_ok(b,a,0xbcu,n))return false;
    const unsigned name=read_field<unsigned>(a,0x14);
    if(!name)return true;
    ++o.parent_reads;return b.find_parent&&b.find_parent(b.context,name,parent);
}
int __cdecl timer_spawn(void*,unsigned,const TimerBindings*,TimerResult*) noexcept;
bool native_spawn_leaf(const SpawnBindings& b,void* a,unsigned n,bool& native) noexcept {
    const auto vt=read_field<unsigned>(a,0);
    if(vt==timer_vtable&&b.logic_timer){native=true;TimerResult r{};return timer_spawn(a,n,b.logic_timer,&r)!=0;}
    native=(vt==resupply_vtable&&b.resupply)||(vt==antigen_vtable&&b.dispenser)||(vt==antigen_canister_vtable&&b.canister);
    if(!native)return b.virtual_spawn&&b.virtual_spawn(b.context,a);
    const auto* api=SiN_GetServerDynamicResupplyApi(1,sizeof(native_resupply::Api));if(!api)return false;
    if(vt==resupply_vtable){LifecycleResult r{};return api->spawn(a,n,b.resupply,&r)!=0;}
    if(vt==antigen_vtable){AntigenInteractionResult r{};return api->antigen_spawn(a,n,b.dispenser,&r)!=0;}
    CanisterResult r{};return api->canister_spawn(a,n,b.canister,&r)!=0;
}
int __cdecl dispatch_spawn(void* a,const SpawnBindings* b,DispatchResult* o) noexcept {
    if(!o)return 0;*o={};o->global_index=-1;
    if(!a){o->completed=1;return 1;}
    if(!b||!b->native_dispatch)return 0;
    const auto& d=*b->native_dispatch;
    unsigned n=0;if(!extent_ok(*b,a,0x37c,n)||!d.scope||!d.current_map||
        d.entry_count>sin_independent::kEntityHandleCapacity||(!d.entries&&d.entry_count))return 0;
    if(!d.scope(d.context,true))return 0;o->entered=1;
    auto finish=[&](bool ok){const bool closed=d.scope(d.context,false)!=0;o->closed=closed;o->completed=ok&&closed;return int(o->completed);};
    o->captured_handle=read_field<unsigned>(a,0x378);
    bool native=false;if(!native_spawn_leaf(*b,a,n,native))return finish(false);
    o->spawn_called=1;o->native_spawn=native;
    // Original checks captured serial after Spawn. Independent storage
    // additionally requires the resolved pointer to remain the same.
    if(sin_independent::EntityHandleView(d.entries,d.entry_count).Resolve(o->captured_handle)!=a){
        o->stale=1;o->native_result=-1;return finish(true);
    }
    if(!extent_ok(*b,a,0x37c,n))return finish(false);
    if(read_field<unsigned>(a,0xb8)&1u){o->marked=1;o->native_result=-1;return finish(true);}
    const unsigned name_token=read_field<unsigned>(a,0x10);
    if(name_token){
        const char* name=nullptr;if(!d.resolve_string||!d.resolve_string(d.context,name_token,&name)||!name)return finish(false);
        using namespace sin_port::fixed_service;
        const auto* api=SiN_GetGlobalStateApi(global_state_version,sizeof(GlobalStateApi));if(!api)return finish(false);
        o->global_index=api->find(name);o->global_lookup=1;
        if(o->global_index<0){
            o->global_index=api->add(name,*d.current_map?*d.current_map:"",1);
            if(o->global_index<0)return finish(false);o->global_added=1;o->global_state=1;
        }else{
            char saved_name[2048]{},level[2048]{};
            if(!api->read(o->global_index,saved_name,sizeof(saved_name),level,sizeof(level),&o->global_state))return finish(false);
            if(o->global_state==2){
                o->global_dead=1;if(!d.remove||!d.remove(d.context,a))return finish(false);
                o->removed=1;o->native_result=-1;return finish(true);
            }
            if(!equal_class(*d.current_map?*d.current_map:"",level)){
                // 22144d40 sets dormancy, clears touch/think and collision
                // state. Owner implements this complete base-entity leaf.
                if(!d.dormant||!d.dormant(d.context,a))return finish(false);o->dormant=1;
            }
        }
    }
    if(!d.listener_count)return finish(false);
    unsigned count=*d.listener_count;
    if(count>d.listener_capacity||count>4096||(count&&!d.listeners))return finish(false);
    while(count){
        --count;
        if(count>=*d.listener_count||!d.listeners[count]||!d.notify||!d.notify(d.context,d.listeners[count],a))return finish(false);
        ++o->notified;
    }
    return finish(true);
}
bool spawn_actor(const SpawnBindings& b,SpawnResult& o,void* a,int& code) noexcept {
    if(b.native_dispatch){
        ++o.native_dispatches;
        if(!dispatch_spawn(a,&b,&o.dispatch))return false;
        code=o.dispatch.native_result;o.native_spawns+=o.dispatch.native_spawn;return true;
    }
    unsigned n=0;if(!extent_ok(b,a,4,n))return false;
    const auto vt=read_field<unsigned>(a,0);
    const bool native=(vt==timer_vtable&&b.logic_timer)||(vt==resupply_vtable&&b.resupply)||(vt==antigen_vtable&&b.dispenser)||(vt==antigen_canister_vtable&&b.canister);
    if(!native)return b.spawn&&b.spawn(b.context,a,&code);
    if(!b.native_spawn_begin||!b.native_spawn_end)return false;
    bool proceed=false;if(!b.native_spawn_begin(b.context,a,&proceed))return false;
    if(proceed){
        if(vt==timer_vtable){TimerResult r{};if(!timer_spawn(a,n,b.logic_timer,&r))return false;}
        else {
            const auto* api=SiN_GetServerDynamicResupplyApi(1,sizeof(native_resupply::Api));if(!api)return false;
            if(vt==resupply_vtable){LifecycleResult r{};if(!api->spawn(a,n,b.resupply,&r))return false;}
            else if(vt==antigen_vtable){AntigenInteractionResult r{};if(!api->antigen_spawn(a,n,b.dispenser,&r))return false;}
            else {CanisterResult r{};if(!api->canister_spawn(a,n,b.canister,&r))return false;}
        }
        ++o.native_spawns;
    }
    return b.native_spawn_end(b.context,a,&code);
}
// CLogicAuto Activate original 22178400, virtual +88.
int __cdecl auto_activate(void* actor,unsigned bytes,const AutoBindings* b,AutoResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!actor||bytes<auto_extent||entity_vtable(actor)!=auto_vtable||!b||!b->base_activate||!b->now||!b->schedule)return 0;
    if(!b->base_activate(b->context,actor))return 0;o->base_called=1;
    const float now=*b->now;
    if(!std::isfinite(now))return 0;
    o->next_think=static_cast<float>(double(now)+0.2);
    if(!std::isfinite(o->next_think)||!b->schedule(b->schedule_context?b->schedule_context:b->context,actor,o->next_think))return 0;
    o->scheduled=o->completed=1;return 1;
}
// CLogicAuto Think original 22178460, virtual +bc.
int __cdecl auto_think(void* actor,unsigned bytes,const AutoBindings* b,AutoResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!actor||bytes<auto_extent||entity_vtable(actor)!=auto_vtable||!b)return 0;
    const auto token=read_field<unsigned>(actor,0x43c);
    if(token){
        const char* text=nullptr;if(!b->resolve_string||!b->resolve_string(b->context,token,&text)||!text)return 0;
        using namespace sin_port::fixed_service;
        const auto* globals=SiN_GetGlobalStateApi(global_state_version,sizeof(GlobalStateApi));if(!globals)return 0;
        const int index=globals->find(text);o->global_checked=1;
        if(index>=0){
            char name[2048]{},level[2048]{};
            if(!globals->read(index,name,sizeof(name),level,sizeof(level),&o->global_state))return 0;
        }
        // 221f4690 returns zero for invalid index, not ON.
        if(o->global_state!=1){o->blocked=o->completed=1;return 1;}
    }
    if(!b->load_type||!b->output||!b->low_violence)return 0;
    using namespace sin_port::native_weapon_tables;
    const auto* fire=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));if(!fire)return 0;
    auto emit=[&](unsigned off){
        ServerEntityVoidOutputResult r{};
        if(!fire->fire(static_cast<unsigned char*>(actor)+off,0x18,nullptr,actor,bytes,0.f,b->output,&r))return false;
        ++o->outputs;o->queued+=r.queued;return true;
    };
    o->load_type=*b->load_type;
    switch(o->load_type){
        case 0:o->load_output=0x3dc;break;
        case 1:o->load_output=0x3f4;break;
        case 2:o->load_output=0x40c;break;
        case 3:o->load_output=0x424;break;
        default:break;
    }
    if(o->load_output&&!emit(o->load_output))return 0;
    if(!emit(0x394))return 0;
    bool low=false;if(!b->low_violence(b->context,&low))return 0;
    o->violence_output=low?0x3ac:0x3c4;
    if(!emit(o->violence_output))return 0;
    if(read_field<unsigned>(actor,0xb4)&1u){
        if(!b->remove||!b->remove(b->context,actor))return 0;o->removed=1;
    }
    o->completed=1;return 1;
}
bool auto_primary_valid(void* a,unsigned n,float interval,const AutoPrimaryBindings* b,AutoPrimaryResult* o) noexcept {
    if(!o)return false;*o={};o->due_tick=o->last_tick=-1;
    return a&&n>=auto_extent&&entity_vtable(a)==auto_vtable&&b&&b->behavior&&b->behavior->now&&b->cadence&&
        std::isfinite(interval)&&interval>0&&b->entry_count<=sin_independent::kEntityHandleCapacity&&
        (!b->entry_count||b->entries)&&sin_independent::EntityHandleView(b->entries,b->entry_count).Resolve(read_field<unsigned>(a,0x378))==a;
}
bool auto_tick(void* a,unsigned n,float time,float interval,const AutoPrimaryBindings& b,int* tick=nullptr) noexcept {
    using namespace sin_port::native_weapon_tables;
    const auto* api=SiN_GetServerNpcOuterCadenceApi(1,sizeof(ServerNpcOuterCadenceApi));ServerNpcOuterCadenceResult result{};
    if(!api||!api->set_default(a,n,time,interval,auto_vtable,b.cadence,&result))return false;
    if(tick)*tick=result.tick;return true;
}
int __cdecl auto_primary_activate(void* a,unsigned n,float interval,const AutoPrimaryBindings* b,AutoPrimaryResult* o) noexcept {
    if(!auto_primary_valid(a,n,interval,b,o))return 0;
    struct Route {void* actor;unsigned bytes;float interval;const AutoPrimaryBindings* b;AutoPrimaryResult* o;};
    Route route{a,n,interval,b,o};
    auto behavior=*b->behavior;behavior.schedule_context=&route;
    behavior.schedule=[](void* p,void* actor,float time) noexcept ->int{
        auto& r=*static_cast<Route*>(p);
        if(actor!=r.actor||!auto_tick(actor,r.bytes,time,r.interval,*r.b,&r.o->due_tick))return 0;
        r.o->scheduled=1;return 1;
    };
    if(!auto_activate(a,n,&behavior,&o->behavior))return 0;
    o->native_result=o->completed=1;return 1;
}
// Original 22222c30 -> 22222b90 -> 221658e0 -> virtual +bc.
// CLogicAuto overrides +bc directly; its +8 member is NOT consulted.
int __cdecl auto_primary_run(void* a,unsigned n,int tick,float interval,const AutoPrimaryBindings* b,AutoPrimaryResult* o) noexcept {
    if(!auto_primary_valid(a,n,interval,b,o)||read_field<int>(a,0x50)>0)return 0;
    auto idle=[&](){o->native_result=o->completed=1;return 1;};
    if(read_field<unsigned>(a,0xb8)&0x400000u){o->no_think=1;return idle();}
    o->due_tick=read_field<int>(a,0x74);
    if(o->due_tick<=0||o->due_tick>tick){o->not_due=1;return idle();}
    const float now=*b->behavior->now;
    if(!std::isfinite(now)||!b->record||!b->scope)return 0;
    const double scheduled=double(o->due_tick)*double(interval);
    o->recorded_time=scheduled>double(now)?now:static_cast<float>(scheduled);
    if(!std::isfinite(o->recorded_time)||!b->record(b->context,a,tick,o->recorded_time,read_field<int>(a,0x74)))return 0;
    o->recorded=1;
    // 2214d870 default clear invokes 2214bf30 twice; reuse same native
    // setter so the second call sees owner notification changes.
    if(!auto_tick(a,n,-1.f,interval,*b)||!auto_tick(a,n,-1.f,interval,*b))return 0;
    if(!b->scope(b->context,a,true))return 0;o->entered=1;
    o->invoked=1;const bool ok=auto_think(a,n,b->behavior,&o->behavior)!=0;
    const bool closed=b->scope(b->context,a,false)!=0;o->closed=closed;
    if(!ok||!closed)return 0;
    o->last_time=*b->behavior->now;
    const double last=std::trunc(double(o->last_time)/double(interval)+0.5);
    if(!std::isfinite(last)||last<INT_MIN||last>INT_MAX)return 0;
    o->last_tick=static_cast<int>(last);write_field(a,0x34,o->last_tick);
    o->native_result=(read_field<unsigned>(a,0xb8)&1u)==0;o->completed=1;return 1;
}
int __cdecl spawn_ordinary(SpawnRow* rows,unsigned count,bool activate,const SpawnBindings* b,SpawnResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!b||count>4096||(count&&!rows)||!b->extent||!b->classname||!b->scope||!b->debug_enabled)return 0;
    for(unsigned i=0;i<count;++i){
        auto& row=rows[i];row.depth=1;row.priority=-1;
        if(row.actor){
            unsigned n=0;if(!extent_ok(*b,row.actor,0xbc,n))return 0;
            const char* cls=nullptr;if(!b->classname(b->context,row.actor,&cls))return 0;
            row.priority=priority(cls);
            if(!(read_field<unsigned>(row.actor,0xb8)&2u)){
                void* seen[4096]{};unsigned used=0;void* cur=row.actor;
                for(;;){
                    void* parent=nullptr;if(!find_parent(*b,*o,cur,&parent))return 0;
                    if(!parent)break;
                    if(parent==row.actor||used==4096)return 0;
                    for(unsigned j=0;j<used;++j)if(seen[j]==parent)return 0;
                    seen[used++]=parent;cur=parent;++row.depth;
                }
            }
        }
        ++o->ranked;
    }
    if(count)std::qsort(rows,count,sizeof(SpawnRow),&compare_rows);
    bool debug=false;if(!b->debug_enabled(b->context,&debug))return 0;
    if(debug)for(unsigned i=0;i<count;++i)if(rows[i].actor){
        if(!b->debug_actor||!b->debug_actor(b->context,rows[i].actor))return 0;++o->debugged;
    }
    // Reverse-order hierarchy pass; only attach to edict-backed parent.
    for(unsigned i=count;i>0;){
        void* a=rows[--i].actor;if(!a)continue;void* parent=nullptr;
        if(!find_parent(*b,*o,a,&parent))return 0;
        if(parent){unsigned n=0;if(!extent_ok(*b,parent,0x170,n))return 0;
            if(read_field<unsigned>(parent,0x16c)){
                if(!b->set_parent||!b->set_parent(b->context,a,parent,-1))return 0;++o->parents;
            }
        }
    }
    auto scope=[&](const char* name,bool enter){
        if(!b->scope(b->context,name,enter))return false;
        if(enter)++o->scopes;else ++o->closes;return true;
    };
    for(unsigned i=0;i<count;++i){
        constexpr char name[]="MapEntity_ParseAllEntities_Spawn";
        if(!scope(name,true))return 0;
        bool ok=true;
        if(rows[i].actor){
            int result=0;ok=spawn_actor(*b,*o,rows[i].actor,result);
            if(ok&&result<0){
                ++o->failed;
                // Original dispatch marks dependent actors for deletion.
                // Inspect later rows before the deferred deletion pass.
                for(unsigned j=i+1;j<count&&ok;++j)if(rows[j].actor){
                    unsigned n=0;if(!extent_ok(*b,rows[j].actor,0xbc,n)){ok=false;break;}
                    if(read_field<unsigned>(rows[j].actor,0xb8)&1u){rows[j].actor=nullptr;++o->pruned;}
                }
                if(ok){ok=b->cleanup&&b->cleanup(b->context);if(ok){++o->cleanups;rows[i].actor=nullptr;}}
            }else if(ok)++o->spawned;
        }
        const bool closed=scope(name,false);if(!ok||!closed)return 0;
    }
    if(activate){
        constexpr char name[]="MapEntity_ParseAllEntities_Activate";
        if(!scope(name,true))return 0;bool ok=true;
        for(unsigned i=0;i<count&&ok;++i)if(void* a=rows[i].actor){
            if(!b->engine_scope||!b->engine_scope(b->context,true)){ok=false;break;}++o->engine_scopes;
            unsigned n=0;ok=extent_ok(*b,a,4,n);
            if(ok&&read_field<unsigned>(a,0)==resupply_vtable&&b->resupply){
                const auto* api=SiN_GetServerDynamicResupplyApi(1,sizeof(native_resupply::Api));LifecycleResult r{};
                ok=api&&api->activate(a,n,b->resupply,&r);if(ok)++o->native_activations;
            }else if(ok&&read_field<unsigned>(a,0)==auto_vtable&&b->logic_auto_primary){
                AutoPrimaryResult r{};ok=b->logic_auto_interval&&auto_primary_activate(a,n,*b->logic_auto_interval,b->logic_auto_primary,&r);if(ok)++o->native_activations;
            }else if(ok&&read_field<unsigned>(a,0)==auto_vtable&&b->logic_auto){
                AutoResult r{};ok=auto_activate(a,n,b->logic_auto,&r)!=0;if(ok)++o->native_activations;
            }else if(ok)ok=b->activate&&b->activate(b->context,a);
            const bool closed=b->engine_scope(b->context,false)!=0;if(closed)++o->engine_closes;
            ok=ok&&closed;if(ok)++o->activated;
        }
        const bool closed=scope(name,false);if(!ok||!closed)return 0;
    }
    o->completed=1;return 1;
}
struct Pipeline {
    const PipelineBindings& b;PipelineResult& o;
    SpawnRow rows[4096]{};
    EntitySlice slices[4096]{};
    void* templates[4096]{};
    unsigned ordinary=0,template_count=0;
};
bool pipeline_scope(Pipeline& p,const char* name,bool enter) noexcept {
    if(!p.b.scope||!p.b.scope(p.b.context,name,enter))return false;
    if(enter)++p.o.scopes;else ++p.o.closes;return true;
}
bool pipeline_cleanup(Pipeline& p,void* a,bool remove) noexcept {
    if(remove){if(!p.b.remove||!p.b.remove(p.b.context,a))return false;++p.o.removed;}
    if(!p.b.cleanup||!p.b.cleanup(p.b.context))return false;++p.o.cleanups;return true;
}
bool pipeline_spawn(Pipeline& p,void* a,int& code) noexcept {
    SpawnResult result{};
    if(!spawn_actor(*p.b.spawn,result,a,code))return false;
    ++p.o.immediate_spawns;if(code<0)++p.o.immediate_failures;return true;
}
int __cdecl collect_entity(void* q,void* actor,const char* s,unsigned n,const EntitySlice* e) noexcept {
    auto& p=*static_cast<Pipeline*>(q);const auto& b=p.b;auto& o=p.o;
    Category category=Category::Ordinary;
    if(!b.classify||!b.classify(b.context,actor,&category))return 0;
    if(category==Category::TemplateEntity){
        ++o.template_entities;
        return b.capture_template&&b.capture_template(b.context,actor,s,n,e)&&b.precache&&b.precache(b.context,actor)&&pipeline_cleanup(p,actor,true);
    }
    if(category==Category::PointTemplate){
        if(p.template_count==4096)return 0;p.templates[p.template_count++]=actor;++o.point_templates;return 1;
    }
    if(category==Category::Ordinary){
        if(p.ordinary==4096)return 0;
        p.rows[p.ordinary].actor=actor;p.slices[p.ordinary++]=*e;++o.ordinary;return 1;
    }
    const bool world=category==Category::World;
    if(!world&&category!=Category::Node&&category!=Category::Light)return 0;
    const char* scope=world?"MapEntity_ParseAllEntities_SpawnWorld":"MapEntity_ParseAllEntities_SpawnTransients";
    if(!pipeline_scope(p,scope,true))return 0;
    bool ok=true;int code=0;
    if(world){
        ++o.worlds;unsigned bytes=0;ok=extent_ok(*b.spawn,actor,0x18,bytes);
        if(ok){write_field(actor,0x14,0u);ok=pipeline_spawn(p,actor,code);}
        // Original world branch ignores the DispatchSpawn return.
    }else if(category==Category::Node){
        ++o.nodes;ok=b.spawn_node&&b.spawn_node(b.context,actor,s,n,e,&code);
        if(ok){++o.immediate_spawns;if(code<0)++o.immediate_failures;}
    }else{++o.lights;ok=pipeline_spawn(p,actor,code);}
    if(ok&&!world&&code<0)ok=pipeline_cleanup(p,actor,false);
    const bool closed=pipeline_scope(p,scope,false);return ok&&closed;
}
int __cdecl parse_all(const char* s,unsigned n,bool activate,const PipelineBindings* b,PipelineResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!s||!b||!b->parse||!b->spawn||!b->classify||!b->scope)return 0;
    Pipeline p{*b,*o};constexpr char outer[]="MapEntity_ParseAllEntities",templates[]="MapEntity_ParseAllEntities_SpawnTemplates";
    if(!pipeline_scope(p,outer,true))return 0;
    bool ok=parse_impl(s,n,b->parse,&o->parse,&collect_entity,&p)!=0;
    for(unsigned i=0;i<p.template_count&&ok;++i){
        if(!pipeline_scope(p,templates,true)){ok=false;break;}
        void* a=p.templates[i];int code=0;
        ok=pipeline_spawn(p,a,code);
        if(ok&&code<0)ok=pipeline_cleanup(p,a,true);
        else if(ok){
            int count=0;ok=b->template_prepare&&b->template_prepare(b->context,a,&count)&&count<=4096;
            for(int j=0;j<count&&ok;++j){
                void* member=nullptr;ok=b->template_member&&b->template_member(b->context,a,j,&member);
                if(!ok)break;++o->template_members;
                if(!member)continue;
                for(unsigned k=0;k<p.ordinary;++k)if(p.rows[k].actor==member){
                    bool remove=false;
                    ok=b->template_capture_member&&b->template_capture_member(b->context,a,member,s,n,&p.slices[k],&remove);
                    if(ok){++o->captured;if(remove){ok=pipeline_cleanup(p,member,true);if(ok)p.rows[k].actor=nullptr;}}
                    break;
                }
            }
            if(ok)ok=b->template_finish&&b->template_finish(b->context,a);
        }
        const bool closed=pipeline_scope(p,templates,false);ok=ok&&closed;
    }
    if(ok)ok=spawn_ordinary(p.rows,p.ordinary,activate,b->spawn,&o->spawn)!=0;
    const bool closed=pipeline_scope(p,outer,false);o->completed=ok&&closed;return int(o->completed);
}
template<class T>T read_map_field(const void* a,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(a)+off,sizeof(v));return v;}
bool valid_relay_queue(const RelayQueueBindings& b) noexcept {
    return b.root&&b.root_bytes>=0x38&&b.root_token&&b.queue&&
        b.entry_count<=sin_independent::kEntityHandleCapacity&&(!b.entry_count||b.entries);
}
// Original 221559f0 removes all rows whose resolved caller is the same
// entity. The following name/class compares read the same actor twice;
// stable independent owner storage makes those comparisons reflexive.
// Preflight topology and active-node conflicts before modifying any links.
bool cancel_relay_events(void* actor,const RelayQueueBindings& b,RelayResult& o) noexcept {
    if(!valid_relay_queue(b)||!b.release||!b.queue->resolve)return false;
    const sin_independent::EntityHandleView registry(b.entries,b.entry_count);
    struct Row {unsigned token;void* pointer;bool remove;};
    Row rows[512]{};
    unsigned token=read_map_field<unsigned>(b.root,0x30),prev=b.root_token;
    while(token){
        if(token==b.root_token||o.cancel_visited==512)return false;
        for(unsigned i=0;i<o.cancel_visited;++i)if(rows[i].token==token)return false;
        void* row=nullptr;unsigned bytes=0;
        if(!b.queue->resolve(b.queue->context,token,&row,&bytes)||!row||row==b.root||bytes<0x38||read_map_field<unsigned>(row,0x34)!=prev)return false;
        for(unsigned i=0;i<o.cancel_visited;++i)if(rows[i].pointer==row)return false;
        const bool remove=registry.Resolve(read_map_field<unsigned>(row,0x10))==actor;
        if(remove&&b.protected_node&&token==*b.protected_node){o.protected_match=1;return false;}
        rows[o.cancel_visited++]={token,row,remove};
        prev=token;token=read_map_field<unsigned>(row,0x30);
    }
    void* previous=b.root;prev=b.root_token;
    for(unsigned i=0;i<o.cancel_visited;++i){
        auto& row=rows[i];
        if(!row.remove){previous=row.pointer;prev=row.token;continue;}
        const unsigned next=i+1<o.cancel_visited?rows[i+1].token:0;
        write_field(previous,0x30,next);
        if(next)write_field(rows[i+1].pointer,0x34,prev);
        if(!b.release(b.context,row.token,row.pointer))return false;
        ++o.cancel_removed;
    }
    o.native_cancel=1;return true;
}
bool schedule_relay_refire(void* actor,float delay,const RelayQueueBindings& b,RelayResult& o) noexcept {
    if(!valid_relay_queue(b)||!b.now||!b.handle||!b.intern)return false;
    unsigned handle=0xffffffffu,input=0;
    const sin_independent::EntityHandleView registry(b.entries,b.entry_count);
    if(!b.handle(b.context,actor,&handle)||registry.Resolve(handle)!=actor||
       !b.intern(b.context,"EnableRefire",&input)||!input)return false;
    const float now=*b.now;if(!std::isfinite(now))return false;
    using namespace sin_port::native_weapon_tables;
    const auto* api=SiN_GetServerEntityNamedEventQueueApi(1,sizeof(ServerEntityNamedEventQueueApi));
    unsigned variant[5]{0,0,0,0xffffffffu,0};
    ServerEntityNamedEventQueueResult q{};
    if(!api||!api->enqueue_direct(b.root,b.root_bytes,b.root_token,now,handle,input,handle,handle,variant,sizeof(variant),delay,0,b.queue,&q))return false;
    o.native_refire=1;return true;
}
// 2217ded0..2217dfca: six original CLogicRelay input leaves.
int __cdecl relay_input(void* actor,unsigned bytes,const char* name,void* activator,const RelayBindings* b,RelayResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!actor||bytes<relay_extent||!name||entity_vtable(actor)!=relay_vtable||!b)return 0;
    auto* a=static_cast<unsigned char*>(actor);
    auto done=[&](){o->completed=1;return 1;};
    o->handled=1;
    if(equal_class(name,"Enable")){a[0x3ac]=0;return done();}
    if(equal_class(name,"Disable")){a[0x3ac]=1;return done();}
    if(equal_class(name,"Toggle")){a[0x3ac]=!a[0x3ac];return done();}
    if(equal_class(name,"EnableRefire")){a[0x3ad]=0;return done();}
    if(equal_class(name,"CancelPending")){
        if(b->native_queue){if(!cancel_relay_events(actor,*b->native_queue,*o))return 0;}
        else if(!b->cancel||!b->cancel(b->context,actor))return 0;
        a[0x3ad]=0;o->canceled=1;return done();
    }
    if(!equal_class(name,"Trigger")){o->handled=0;return done();}
    if(a[0x3ac]||a[0x3ad]){o->suppressed=1;return done();}
    using namespace sin_port::native_weapon_tables;
    const auto* fire=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
    ServerEntityVoidOutputResult fr{};
    if(!fire||!b->output||!fire->fire(a+relay_output,0x18,activator,actor,bytes,0.f,b->output,&fr))return 0;
    o->fired=1;o->queued=fr.queued;
    unsigned flags=0;std::memcpy(&flags,a+0xb4,4);
    if(flags&1){if(!b->remove||!b->remove(b->context,actor))return 0;o->removed=1;return done();}
    if(flags&2)return done();
    // Original sets the latch BEFORE computing the maximum of the surviving
    // action list. Finite rows already removed by fire must not count.
    a[0x3ad]=1;
    unsigned token=0,seen[256]{};
    std::memcpy(&token,a+relay_output+0x14,4);
    while(token){
        if(o->actions==256)return 0;
        for(unsigned i=0;i<o->actions;++i)if(seen[i]==token)return 0;
        seen[o->actions++]=token;
        void* row=nullptr;unsigned n=0;
        if(!b->output->resolve||!b->output->resolve(b->output->context,token,&row,&n)||!row||n<0x1c)return 0;
        float delay=0;std::memcpy(&delay,static_cast<unsigned char*>(row)+0xc,4);
        if(!std::isfinite(delay))return 0;if(delay>o->maximum_delay)o->maximum_delay=delay;
        std::memcpy(&token,static_cast<unsigned char*>(row)+0x18,4);
    }
    o->refire_delay=static_cast<float>(double(o->maximum_delay)+0.001);
    if(b->native_queue){if(!schedule_relay_refire(actor,o->refire_delay,*b->native_queue,*o))return 0;}
    else if(!b->refire||!b->refire(b->context,actor,o->refire_delay))return 0;
    o->refire=1;return done();
}
// Original 22178a40: disabled gate, optional random draw, current clock
// plus stored interval. Random bounds are passed through in original order.
bool timer_valid(void* a,unsigned n,const TimerBindings* b,TimerResult* o) noexcept {
    if(!o)return false;*o={};
    if(!a||n<timer_extent||entity_vtable(a)!=timer_vtable||!b)return false;
    if(!b->cadence)return true;
    return b->tick_interval&&std::isfinite(*b->tick_interval)&&*b->tick_interval>0.f&&
        b->entry_count<=sin_independent::kEntityHandleCapacity&&(!b->entry_count||b->entries)&&
        sin_independent::EntityHandleView(b->entries,b->entry_count).Resolve(read_field<unsigned>(a,0x378))==a;
}
bool timer_set_tick(void* a,float time,const TimerBindings& b) noexcept {
    using namespace sin_port::native_weapon_tables;
    const auto* api=SiN_GetServerNpcOuterCadenceApi(1,sizeof(ServerNpcOuterCadenceApi));
    ServerNpcOuterCadenceResult result{};
    return b.cadence&&b.tick_interval&&api&&api->set_default(a,timer_extent,time,*b.tick_interval,timer_vtable,b.cadence,&result);
}
bool timer_schedule(void* a,float time,const TimerBindings& b,TimerResult& o) noexcept {
    if(!std::isfinite(time))return false;
    if(b.cadence){if(!timer_set_tick(a,time,b))return false;}
    else if(!b.schedule||!b.schedule(b.schedule_context?b.schedule_context:b.context,a,time))return false;
    o.next_think=time;o.scheduled=1;return true;
}
bool timer_reschedule(void* a,const TimerBindings& b,TimerResult& o) noexcept {
    if(read_field<int>(a,0x3dc)){o.suppressed=1;return true;}
    if(read_field<int>(a,0x3e8)){
        float value=0;
        if(!b.random||!b.random(b.context,read_field<float>(a,0x3ec),read_field<float>(a,0x3f0),&value)||!std::isfinite(value))return false;
        write_field(a,0x3e0,value);o.randomized=1;
    }
    o.interval=read_field<float>(a,0x3e0);
    return b.now&&timer_schedule(a,static_cast<float>(double(*b.now)+o.interval),b,o);
}
bool timer_enable(void* a,const TimerBindings& b,TimerResult& o) noexcept {
    write_field(a,0x3dc,0);return timer_reschedule(a,b,o);
}
bool timer_disable(void* a,const TimerBindings& b,TimerResult& o) noexcept {
    write_field(a,0x3dc,1);return timer_schedule(a,-1.f,b,o);
}
int __cdecl timer_spawn(void* a,unsigned n,const TimerBindings* b,TimerResult* o) noexcept {
    if(!timer_valid(a,n,b,o))return 0;
    const bool random=read_field<int>(a,0x3e8)!=0;
    // 22179300 clamps fixed intervals (including unordered values).
    if(!random&&!(read_field<float>(a,0x3e0)>=.01f)){
        write_field(a,0x3e0,.01f);o->clamped=1;
    }
    const bool enabled=!read_field<int>(a,0x3dc)&&(random||read_field<float>(a,0x3e0)>0.f);
    if(!(enabled?timer_enable(a,*b,*o):timer_disable(a,*b,*o)))return 0;
    o->handled=o->completed=1;return 1;
}
bool timer_fire(void* a,unsigned n,const TimerBindings& b,TimerResult& o) noexcept {
    if(read_field<int>(a,0x3dc)){o.suppressed=1;return true;}
    const bool oscillate=(read_field<unsigned>(a,0xb4)&1u)!=0;
    const unsigned off=oscillate?(read_field<unsigned char>(a,0x3e4)?0x3ac:0x3c4):0x394;
    using namespace sin_port::native_weapon_tables;
    const auto* fire=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));ServerEntityVoidOutputResult r{};
    if(!fire||!b.output||!fire->fire(static_cast<unsigned char*>(a)+off,0x18,a,a,n,0.f,b.output,&r))return false;
    o.output_offset=off;o.fired=1;o.queued=r.queued;
    if(oscillate)write_field(a,0x3e4,static_cast<unsigned char>(!read_field<unsigned char>(a,0x3e4)));
    return timer_reschedule(a,b,o);
}
int __cdecl timer_input(void* a,unsigned n,const char* name,unsigned type,float value,const TimerBindings* b,TimerResult* o) noexcept {
    if(!timer_valid(a,n,b,o)||!name)return 0;
    bool ok=true;o->handled=1;
    if(equal_class(name,"Enable"))ok=timer_enable(a,*b,*o);
    else if(equal_class(name,"Disable"))ok=timer_disable(a,*b,*o);
    else if(equal_class(name,"Toggle"))ok=read_field<int>(a,0x3dc)?timer_enable(a,*b,*o):timer_disable(a,*b,*o);
    else if(equal_class(name,"FireTimer"))ok=timer_fire(a,n,*b,*o);
    else if(equal_class(name,"RefireTime")){
        // 221793e0 accepts FLOAT >=0.01; everything else becomes 0.01.
        const float candidate=type==1&&value>=.01f?value:.01f;
        o->clamped=type!=1||!(value>=.01f);
        if(candidate!=read_field<float>(a,0x3e0)){
            write_field(a,0x3e0,candidate);o->changed=1;ok=timer_reschedule(a,*b,*o);
        }
    }else o->handled=0;
    o->completed=ok;return int(ok);
}
// Original +bc 2217a120 jumps directly to 22179370; ignore stored +8.
int __cdecl timer_primary_run(void* a,unsigned n,int tick,const TimerPrimaryBindings* b,TimerPrimaryResult* o) noexcept {
    if(!o)return 0;*o={};o->due_tick=o->next_tick=o->last_tick=-1;
    if(!b||!b->behavior||!b->behavior->cadence||!b->behavior->now||
       !timer_valid(a,n,b->behavior,&o->behavior)||read_field<int>(a,0x50)>0)return 0;
    const auto& behavior=*b->behavior;
    auto idle=[&](){o->next_tick=read_field<int>(a,0x74);o->native_result=o->completed=1;return 1;};
    if(read_field<unsigned>(a,0xb8)&0x400000u){o->no_think=1;return idle();}
    o->due_tick=read_field<int>(a,0x74);
    if(o->due_tick<=0||o->due_tick>tick){o->not_due=1;return idle();}
    const float now=*behavior.now,interval=*behavior.tick_interval;
    if(!std::isfinite(now)||!b->record||!b->scope)return 0;
    const double scheduled=double(o->due_tick)*double(interval);
    o->recorded_time=scheduled>double(now)?now:static_cast<float>(scheduled);
    if(!std::isfinite(o->recorded_time)||!b->record(b->context,a,tick,o->recorded_time,read_field<int>(a,0x74)))return 0;
    o->recorded=1;
    // Same default clear and repeated flag check as the auto path.
    if(!timer_set_tick(a,-1.f,behavior)||!timer_set_tick(a,-1.f,behavior))return 0;
    if(!b->scope(b->context,a,true))return 0;o->entered=1;
    o->invoked=1;const bool ok=timer_fire(a,n,behavior,o->behavior);
    o->behavior.handled=1;o->behavior.completed=ok;
    const bool closed=b->scope(b->context,a,false)!=0;o->closed=closed;
    if(!ok||!closed)return 0;
    o->last_time=*behavior.now;
    const double last=std::trunc(double(o->last_time)/double(interval)+0.5);
    if(!std::isfinite(last)||last<INT_MIN||last>INT_MAX)return 0;
    o->last_tick=static_cast<int>(last);write_field(a,0x34,o->last_tick);
    o->next_tick=read_field<int>(a,0x74);
    o->native_result=(read_field<unsigned>(a,0xb8)&1u)==0;o->completed=1;return 1;
}
// Original 22179ba0/22179be0/22179c20 and silent 22178d70.
// SetValue is the direct FIELD_BOOLEAN descriptor, not a handler.
int __cdecl branch_input(void* a,unsigned n,const char* name,void* activator,unsigned type,unsigned char value,const BranchBindings* b,BranchResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!a||n<branch_extent||entity_vtable(a)!=branch_vtable||!name||!b)return 0;
    const auto prior=read_field<unsigned char>(a,0x394);
    auto next=prior;bool fire=false;o->handled=1;
    if(equal_class(name,"SetValue"))next=static_cast<unsigned char>(type==6&&value!=0);
    else if(equal_class(name,"SetValueTest")){next=type==6?value:0;fire=true;}
    else if(equal_class(name,"Toggle"))next=static_cast<unsigned char>(!prior);
    else if(equal_class(name,"ToggleTest")){next=static_cast<unsigned char>(!prior);fire=true;}
    else if(equal_class(name,"Test"))fire=true;
    else o->handled=0;
    if(next!=prior){write_field(a,0x394,next);o->changed=1;}
    o->value=next;
    if(fire){
        using namespace sin_port::native_weapon_tables;
        const auto* api=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
        ServerEntityVoidOutputResult result{};
        o->output_offset=next?0x398:0x3b0;
        if(!api||!b->output||!api->fire(static_cast<unsigned char*>(a)+o->output_offset,0x18,activator,a,n,0.f,b->output,&result))return 0;
        o->fired=1;o->queued=result.queued;
    }
    o->completed=1;return 1;
}
bool case_text(const CaseBindings& b,unsigned token,const char** out) noexcept {
    *out="";if(!token)return true;
    unsigned bytes=0;
    return b.resolve_text&&b.resolve_text(b.context,token,out,&bytes)&&*out&&bytes&&std::memchr(*out,0,bytes<4096?bytes:4096);
}
bool case_variant_text(const CaseBindings& b,const void* variant,char (&buffer)[512],const char** text) noexcept {
    const unsigned type=read_field<unsigned>(variant,0x10);
    if(type==2)return case_text(b,read_field<unsigned>(variant,0),text);
    if(type==0){*text="";return true;}
    if(type==6){*text=read_field<unsigned char>(variant,0)?"true":"false";return true;}
    if(type==5){
        const auto result=std::to_chars(buffer,buffer+sizeof(buffer)-1,read_field<int>(variant,0));
        if(result.ec!=std::errc{})return false;*result.ptr=0;*text=buffer;return true;
    }
    if(type==1||type==3||type==9||type==13){
        if(!b.format_variant||!b.format_variant(b.context,variant,0x14,buffer,sizeof(buffer))||!std::memchr(buffer,0,sizeof(buffer)))return false;
        *text=buffer;return true;
    }
    *text="No conversion to string";return true;
}
// Original CLogicCase 2217ab00 and 22178cb0. Numbered match is
// first populated case in ascending order, regardless of action count.
// Random instead scans actual action lists, regardless of case token.
int __cdecl case_input(void* a,unsigned n,const char* name,void* activator,const void* variant,unsigned vn,const CaseBindings* b,CaseResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!a||n<case_extent||entity_vtable(a)!=case_vtable||!name||!b)return 0;
    using namespace sin_port::native_weapon_tables;
    unsigned off=0;o->handled=1;
    if(equal_class(name,"InValue")){
        if(!variant||vn<0x14)return 0;
        char buffer[512]{};const char* text=nullptr;
        if(!case_variant_text(*b,variant,buffer,&text))return 0;
        for(unsigned i=0;i<16;++i){
            const auto token=read_field<unsigned>(a,0x394+i*4);if(!token)continue;
            const char* key=nullptr;if(!case_text(*b,token,&key))return 0;
            if(equal_class(key,text)){o->matched=1;o->selected=i+1;off=0x3d4+i*0x18;break;}
        }
        if(!off){
            // 22154e40 with FIELD_INPUT (20) takes unsupported-type
            // branch 22154eb9: clear value word and type, retain other words.
            off=0x554;write_field(a,off,0u);write_field(a,off+0x10,0u);o->defaulted=1;
        }
    }else if(equal_class(name,"PickRandom")){
        if(!b->output)return 0;
        unsigned eligible[16]{};
        for(unsigned i=0;i<16;++i){
            unsigned token=read_field<unsigned>(a,0x3d4+i*0x18+0x14);
            if(!token)continue;
            unsigned seen[256]{},count=0;
            while(token){
                if(count==256)return 0;
                for(unsigned j=0;j<count;++j)if(seen[j]==token)return 0;
                seen[count++]=token;++o->visited;
                void* row=nullptr;unsigned bytes=0;
                if(!b->output->resolve||!b->output->resolve(b->output->context,token,&row,&bytes)||!row||bytes<0x1c)return 0;
                token=read_field<unsigned>(row,0x18);
            }
            eligible[o->eligible++]=i;
        }
        if(!o->eligible){
            if(b->empty_warning&&!b->empty_warning(b->context,a))return 0;
            o->warned=o->completed=1;return 1;
        }
        int draw=-1;if(!b->random_int||!b->random_int(b->context,0,int(o->eligible)-1,&draw)||draw<0||unsigned(draw)>=o->eligible)return 0;
        o->randomized=1;o->selected=eligible[draw]+1;off=0x3d4+eligible[draw]*0x18;
    }else{o->handled=0;o->completed=1;return 1;}
    // Existing void output route has canonical unused variant words.
    // OnDefault's stored unused words above retain original state.
    const auto* fire=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
    ServerEntityVoidOutputResult r{};
    if(!fire||!b->output||!fire->fire(static_cast<unsigned char*>(a)+off,0x18,activator,a,n,0.f,b->output,&r))return 0;
    o->fired=1;o->queued=r.queued;o->completed=1;return 1;
}
const sin_port::native_map::Api api{1,sizeof(sin_port::native_map::Api),&token,&slice,&lookup,&parse,&register_factories,&find_factory,&create_factory,&spawn_ordinary,&dispatch_spawn,&key_value,&parse_all,&relay_input,&register_relay,&destroy_relay,&auto_activate,&auto_think,&register_auto,&destroy_auto,&auto_primary_activate,&auto_primary_run,&timer_spawn,&timer_input,&register_timer,&destroy_timer,&timer_primary_run,&branch_input,&register_branch,&destroy_branch,&case_input,&register_case,&destroy_case};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerMapEntityApi=_SiN_GetServerMapEntityApi")
extern "C" __declspec(dllexport) const sin_port::native_map::Api* __cdecl SiN_GetServerMapEntityApi(std::uint32_t v,std::uint32_t n) noexcept {
    using namespace sin_port::native_map;return v==1&&n==sizeof(sin_port::native_map::Api)?&api:nullptr;
}
