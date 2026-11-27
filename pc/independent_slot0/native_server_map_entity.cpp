#include "native_server_map_entity.h"
#include "native_server_npc_active_weapon_step.h"
#include "native_server_activity_assignment.h"
#include "native_server_npc_task_lifecycle.h"
extern "C" const sin_port::native_weapon_tables::ServerNpcTaskLifecycleApi* __cdecl SiN_GetServerNpcTaskLifecycleApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerNpcActivityAssignmentApi* __cdecl SiN_GetServerNpcActivityAssignmentApi(std::uint32_t,std::uint32_t) noexcept;
#include <cmath>
#include "server_fixed_difficulty_service.h"
int __cdecl SinPublishCampaignEventInternal(void*,const sin_port::fixed_service::NativeTriggerEvent*,
    int (__cdecl *)(void*,int*) noexcept,void*,const unsigned char*,void*,void*) noexcept;
bool __cdecl SinReadCampaignTouchMarkerInternal(const void*,unsigned*) noexcept;
bool __cdecl SinReadCampaignHandleInternal(const void*,unsigned,unsigned*) noexcept;
bool __cdecl SinCampaignStorageReadableInternal(const void*,unsigned) noexcept;
extern "C" const sin_port::fixed_service::GlobalStateApi* __cdecl SiN_GetGlobalStateApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" const sin_port::fixed_service::Api* __cdecl SiN_GetFixedDifficultyApi(std::uint32_t,std::uint32_t) noexcept;
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <limits>
#include <charconv>
extern "C" const sin_port::native_resupply::Api* __cdecl SiN_GetServerDynamicResupplyApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerEntityVoidOutputApi* __cdecl SiN_GetServerEntityVoidOutputApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerEntityNamedEventQueueApi* __cdecl SiN_GetServerEntityNamedEventQueueApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerNpcOuterCadenceApi* __cdecl SiN_GetServerNpcOuterCadenceApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" const sin_port::native_weapon_tables::ServerEntitySearchApi* __cdecl SiN_GetServerEntitySearchApi(std::uint32_t,std::uint32_t) noexcept;
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
constexpr FactorySpec compare_spec{"logic_compare",Family::LogicCompare,0x224fb624,0x2241f5f0,compare_extent};
constexpr FactorySpec counter_spec{"math_counter",Family::MathCounter,0x224fb61c,0x2241f5d8,counter_extent};
constexpr FactorySpec menu_spec{"logic_menulistener",Family::LogicMenuListener,0x224fb634,0x2241f620,menu_extent};
constexpr FactorySpec navigation_spec{"logic_navigation",Family::LogicNavigation,0x224fb0e0,0x2241ece4,navigation_extent};
constexpr FactorySpec autosave_spec{"logic_autosave",Family::LogicAutosave,0x224fb62c,0x2241f608,autosave_extent};
constexpr FactorySpec multicompare_spec{"logic_multicompare",Family::LogicMultiCompare,0x224fb600,0x2241f584,multicompare_extent};
constexpr FactorySpec remap_spec{"math_remap",Family::MathRemap,0x224fb60c,0x2241f5a8,remap_extent};
constexpr FactorySpec colorblend_spec{"math_colorblend",Family::MathColorBlend,0x224fb610,0x2241f5b4,colorblend_extent};
constexpr FactorySpec dynamic_trigger_spec{"trigger_dynamic_difficulty",Family::TriggerDynamic,0x2250b404,0x22437fcc,dynamic_trigger_extent};
constexpr FactorySpec secret_trigger_spec{"trigger_secret",Family::TriggerSecret,0x2250b408,0x22437fd8,secret_trigger_extent};
bool valid_dictionary(const FactoryDictionary* d) noexcept {
    if(!d||!d->registered||d->count<12||d->count>27)return false;
    for(unsigned i=0;i<12;++i)if(d->slots[i]!=&native_specs[i])return false;
    bool relay=false,logic_auto=false,logic_timer=false,logic_branch=false,logic_case=false,logic_compare=false,math_counter=false,logic_menu=false,logic_navigation=false,logic_autosave=false,logic_multicompare=false,math_remap=false,math_colorblend=false,dynamic_trigger=false,secret_trigger=false;
    for(unsigned i=12;i<27;++i){
        if(i>=d->count){if(d->slots[i])return false;continue;}
        if(d->slots[i]==&relay_spec&&!relay)relay=true;
        else if(d->slots[i]==&auto_spec&&!logic_auto)logic_auto=true;
        else if(d->slots[i]==&timer_spec&&!logic_timer)logic_timer=true;
        else if(d->slots[i]==&branch_spec&&!logic_branch)logic_branch=true;
        else if(d->slots[i]==&case_spec&&!logic_case)logic_case=true;
        else if(d->slots[i]==&compare_spec&&!logic_compare)logic_compare=true;
        else if(d->slots[i]==&counter_spec&&!math_counter)math_counter=true;
        else if(d->slots[i]==&menu_spec&&!logic_menu)logic_menu=true;
        else if(d->slots[i]==&navigation_spec&&!logic_navigation)logic_navigation=true;
        else if(d->slots[i]==&autosave_spec&&!logic_autosave)logic_autosave=true;
        else if(d->slots[i]==&multicompare_spec&&!logic_multicompare)logic_multicompare=true;
        else if(d->slots[i]==&remap_spec&&!math_remap)math_remap=true;
        else if(d->slots[i]==&colorblend_spec&&!math_colorblend)math_colorblend=true;
        else if(d->slots[i]==&dynamic_trigger_spec&&!dynamic_trigger)dynamic_trigger=true;
        else if(d->slots[i]==&secret_trigger_spec&&!secret_trigger)secret_trigger=true;
        else return false;
    }
    return true;
}
int register_optional(FactoryDictionary* d,const FactorySpec* spec) noexcept {
    if(!valid_dictionary(d)||d->count==27)return 0;
    for(unsigned i=12;i<d->count;++i)if(d->slots[i]==spec)return 0;
    d->slots[d->count++]=spec;return 1;
}
int __cdecl register_relay(FactoryDictionary* d) noexcept {return register_optional(d,&relay_spec);}
int __cdecl register_auto(FactoryDictionary* d) noexcept {return register_optional(d,&auto_spec);}
int __cdecl register_timer(FactoryDictionary* d) noexcept {return register_optional(d,&timer_spec);}
int __cdecl register_branch(FactoryDictionary* d) noexcept {return register_optional(d,&branch_spec);}
int __cdecl register_case(FactoryDictionary* d) noexcept {return register_optional(d,&case_spec);}
int __cdecl register_compare(FactoryDictionary* d) noexcept {return register_optional(d,&compare_spec);}
int __cdecl register_counter(FactoryDictionary* d) noexcept {return register_optional(d,&counter_spec);}
int __cdecl register_menu(FactoryDictionary* d) noexcept {return register_optional(d,&menu_spec);}
int __cdecl register_navigation(FactoryDictionary* d) noexcept {return register_optional(d,&navigation_spec);}
int __cdecl register_autosave(FactoryDictionary* d) noexcept {return register_optional(d,&autosave_spec);}
int __cdecl register_multicompare(FactoryDictionary* d) noexcept {return register_optional(d,&multicompare_spec);}
int __cdecl register_remap(FactoryDictionary* d) noexcept {return register_optional(d,&remap_spec);}
int __cdecl register_colorblend(FactoryDictionary* d) noexcept {return register_optional(d,&colorblend_spec);}
int __cdecl register_campaign_triggers(FactoryDictionary* d) noexcept {
    if(!valid_dictionary(d)||d->count>25)return 0;
    for(unsigned i=12;i<d->count;++i)if(d->slots[i]==&dynamic_trigger_spec||d->slots[i]==&secret_trigger_spec)return 0;
    d->slots[d->count++]=&dynamic_trigger_spec;d->slots[d->count++]=&secret_trigger_spec;return 1;
}
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
    if(o->spec->family==Family::TriggerDynamic||o->spec->family==Family::TriggerSecret){
        const auto* r=b->campaign_triggers;const unsigned extent=o->spec->extent;
        if(!r||!r->allocate||!r->base_construct||!r->initialize||
           !r->allocate(r->context,extent,&o->actor,&o->bytes)||!o->actor||o->bytes<extent)return 0;
        auto* a=static_cast<unsigned char*>(o->actor);
        if(!r->base_construct(r->context,a))return 0;
        // Retail derived construction writes only the primary table.
        // The three dynamic fields are zero from fresh owner allocation.
        if(extent==dynamic_trigger_extent){
            unsigned fields[3]{};std::memcpy(fields,a+0x470,12);
            if(fields[0]||fields[1]||fields[2])return 0;
        }
        const unsigned vt=extent==dynamic_trigger_extent?dynamic_trigger_vtable:secret_trigger_vtable;
        std::memcpy(a,&vt,4);
        if(!r->initialize(r->context,a,name))return 0;
        o->networkable=a+0x160;o->completed=1;return 1;
    }
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
    if(o->spec->family==Family::MathRemap||o->spec->family==Family::MathColorBlend){
        const bool blend=o->spec->family==Family::MathColorBlend;
        const auto* r=blend?b->math_colorblend:b->math_remap;
        if(!r||!r->allocate||!r->base_construct||!r->initialize||
           !r->allocate(r->context,remap_extent,&o->actor,&o->bytes)||!o->actor||o->bytes<remap_extent)return 0;
        auto* a=static_cast<unsigned char*>(o->actor);
        // Original 2217d500 / 2217d5c0: identical extent and output triplet.
        if(!r->base_construct(r->context,a,1))return 0;
        unsigned bounds[4]{},head=0;std::memcpy(bounds,a+0x394,16);std::memcpy(&head,a+0x3b8,4);
        if(bounds[0]||bounds[1]||bounds[2]||bounds[3]||head)return 0;
        const unsigned vt=blend?colorblend_vtable:remap_vtable,zero=0,invalid=0xffffffffu;
        std::memcpy(a,&vt,4);std::memcpy(a+0x3a4,&zero,4);std::memcpy(a+0x3b0,&invalid,4);std::memcpy(a+0x3b4,&zero,4);
        if(!r->initialize(r->context,a,name))return 0;
        o->networkable=a+0x160;o->completed=1;return 1;
    }
    if(o->spec->family==Family::LogicMultiCompare){
        const auto* r=b->logic_multicompare;
        if(!r||!r->allocate||!r->base_construct||!r->initialize||
           !r->allocate(r->context,multicompare_extent,&o->actor,&o->bytes)||!o->actor||o->bytes<multicompare_extent)return 0;
        auto* a=static_cast<unsigned char*>(o->actor);
        if(!r->base_construct(r->context,a,1))return 0;
        unsigned scalar[3]{};std::memcpy(scalar,a+0x3c4,8);std::memcpy(scalar+2,a+0x3d0,4);
        if(scalar[0]||scalar[1]||scalar[2])return 0;
        for(unsigned off=0x394;off<=0x3ac;off+=0x18){
            unsigned head=0;std::memcpy(&head,a+off+0x14,4);if(head)return 0;
        }
        const unsigned zero=0,invalid=0xffffffffu,vt=multicompare_vtable;
        std::memcpy(a,&vt,4);
        for(unsigned off=0x394;off<=0x3ac;off+=0x18){
            std::memcpy(a+off,&zero,4);std::memcpy(a+off+0xc,&invalid,4);std::memcpy(a+off+0x10,&zero,4);
        }
        std::memcpy(a+0x3cc,&zero,4);
        if(!r->initialize(r->context,a,name))return 0;
        o->networkable=a+0x160;o->completed=1;return 1;
    }
    if(o->spec->family==Family::LogicAutosave){
        const auto* r=b->logic_autosave;
        if(!r||!r->allocate||!r->base_construct||!r->initialize||
           !r->allocate(r->context,autosave_extent,&o->actor,&o->bytes)||!o->actor||o->bytes<autosave_extent)return 0;
        auto* a=static_cast<unsigned char*>(o->actor);
        // Original 2217b050 base(1), primary vtable, Initialize.
        if(!r->base_construct(r->context,a,1)||a[0x394])return 0;
        const unsigned vt=autosave_vtable;std::memcpy(a,&vt,4);
        if(!r->initialize(r->context,a,name))return 0;
        o->networkable=a+0x160;o->completed=1;return 1;
    }
    if(o->spec->family==Family::LogicNavigation){
        const auto* r=b->logic_navigation;
        if(!r||!r->allocate||!r->base_construct||!r->initialize||
           !r->allocate(r->context,navigation_extent,&o->actor,&o->bytes)||!o->actor||o->bytes<navigation_extent)return 0;
        auto* a=static_cast<unsigned char*>(o->actor);
        // Original 221782e0 base(1) and secondary listener constructor.
        if(!r->base_construct(r->context,a,1))return 0;
        unsigned property=0;std::memcpy(&property,a+0x39c,4);
        if(a[0x398]||property)return 0;
        const unsigned vt=navigation_vtable,listener_base=0x223ec0dc,listener=navigation_listener_vtable;
        std::memcpy(a+0x394,&listener_base,4);std::memcpy(a,&vt,4);std::memcpy(a+0x394,&listener,4);
        if(!r->initialize(r->context,a,name))return 0;
        o->networkable=a+0x160;o->completed=1;return 1;
    }
    if(o->spec->family==Family::LogicMenuListener){
        const auto* r=b->logic_menu;
        if(!r||!r->allocate||!r->base_construct||!r->initialize||
           !r->allocate(r->context,menu_extent,&o->actor,&o->bytes)||!o->actor||o->bytes<menu_extent)return 0;
        auto* a=static_cast<unsigned char*>(o->actor);
        // Original 2217dc30 constructs both primary and secondary vtables.
        if(!r->base_construct(r->context,a,1))return 0;
        for(unsigned off=0x398;off<=0x500;off+=0x18){
            unsigned head=0;std::memcpy(&head,a+off+0x14,4);if(head)return 0;
        }
        const unsigned zero=0,invalid=0xffffffffu,vt=menu_vtable,listener_base=0x224227c4,listener=menu_listener_vtable;
        std::memcpy(a+0x394,&listener_base,4);std::memcpy(a,&vt,4);std::memcpy(a+0x394,&listener,4);
        for(unsigned off=0x398;off<=0x500;off+=0x18){
            std::memcpy(a+off,&zero,4);std::memcpy(a+off+0xc,&invalid,4);std::memcpy(a+off+0x10,&zero,4);
        }
        if(!r->initialize(r->context,a,name))return 0;
        o->networkable=a+0x160;o->completed=1;return 1;
    }
    if(o->spec->family==Family::MathCounter){
        const auto* r=b->math_counter;
        if(!r||!r->allocate||!r->base_construct||!r->initialize||
           !r->allocate(r->context,counter_extent,&o->actor,&o->bytes)||!o->actor||o->bytes<counter_extent)return 0;
        auto* a=static_cast<unsigned char*>(o->actor);
        // Original 2217d810 base(1), three output triplets.
        if(!r->base_construct(r->context,a,1))return 0;
        unsigned values[2]{};std::memcpy(values,a+0x394,8);
        if(values[0]||values[1]||a[0x39c]||a[0x39d])return 0;
        for(unsigned off=0x3a0;off<=0x3d0;off+=0x18){
            unsigned head=0;std::memcpy(&head,a+off+0x14,4);if(head)return 0;
        }
        const unsigned zero=0,invalid=0xffffffffu,vt=counter_vtable;
        std::memcpy(a,&vt,4);
        for(unsigned off=0x3a0;off<=0x3d0;off+=0x18){
            std::memcpy(a+off,&zero,4);std::memcpy(a+off+0xc,&invalid,4);std::memcpy(a+off+0x10,&zero,4);
        }
        if(!r->initialize(r->context,a,name))return 0;
        o->networkable=a+0x160;o->completed=1;return 1;
    }
    if(o->spec->family==Family::LogicCompare){
        const auto* r=b->logic_compare;
        if(!r||!r->allocate||!r->base_construct||!r->initialize||
           !r->allocate(r->context,compare_extent,&o->actor,&o->bytes)||!o->actor||o->bytes<compare_extent)return 0;
        auto* a=static_cast<unsigned char*>(o->actor);
        // Original 2217da20 base(1) and four float output triplets.
        if(!r->base_construct(r->context,a,1))return 0;
        unsigned values[2]{};std::memcpy(values,a+0x394,8);
        if(values[0]||values[1])return 0;
        for(unsigned off=0x39c;off<=0x3e4;off+=0x18){
            unsigned head=0;std::memcpy(&head,a+off+0x14,4);if(head)return 0;
        }
        const unsigned zero=0,invalid=0xffffffffu,vt=compare_vtable;
        std::memcpy(a,&vt,4);
        for(unsigned off=0x39c;off<=0x3e4;off+=0x18){
            std::memcpy(a+off,&zero,4);std::memcpy(a+off+0xc,&invalid,4);std::memcpy(a+off+0x10,&zero,4);
        }
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
int __cdecl destroy_compare(void* actor,unsigned bytes,void* networkable,const RelayFactoryBindings* b) noexcept {
    if(!networkable)return 1;
    if(!actor||bytes<compare_extent||entity_vtable(actor)!=compare_vtable||
       networkable!=static_cast<unsigned char*>(actor)+0x160||!b||!b->release_networkable)return 0;
    return b->release_networkable(b->context,networkable);
}
int __cdecl destroy_counter(void* actor,unsigned bytes,void* networkable,const RelayFactoryBindings* b) noexcept {
    if(!networkable)return 1;
    if(!actor||bytes<counter_extent||entity_vtable(actor)!=counter_vtable||
       networkable!=static_cast<unsigned char*>(actor)+0x160||!b||!b->release_networkable)return 0;
    return b->release_networkable(b->context,networkable);
}
int __cdecl destroy_menu(void* actor,unsigned bytes,void* networkable,const RelayFactoryBindings* b) noexcept {
    if(!networkable)return 1;
    if(!actor||bytes<menu_extent||entity_vtable(actor)!=menu_vtable||
       networkable!=static_cast<unsigned char*>(actor)+0x160||!b||!b->release_networkable)return 0;
    return b->release_networkable(b->context,networkable);
}
int __cdecl destroy_navigation(void* actor,unsigned bytes,void* networkable,const RelayFactoryBindings* b) noexcept {
    if(!networkable)return 1;
    if(!actor||bytes<navigation_extent||entity_vtable(actor)!=navigation_vtable||
       networkable!=static_cast<unsigned char*>(actor)+0x160||!b||!b->release_networkable)return 0;
    return b->release_networkable(b->context,networkable);
}
int __cdecl destroy_autosave(void* actor,unsigned bytes,void* networkable,const RelayFactoryBindings* b) noexcept {
    if(!networkable)return 1;
    if(!actor||bytes<autosave_extent||entity_vtable(actor)!=autosave_vtable||
       networkable!=static_cast<unsigned char*>(actor)+0x160||!b||!b->release_networkable)return 0;
    return b->release_networkable(b->context,networkable);
}
int __cdecl destroy_multicompare(void* actor,unsigned bytes,void* networkable,const RelayFactoryBindings* b) noexcept {
    if(!networkable)return 1;
    if(!actor||bytes<multicompare_extent||entity_vtable(actor)!=multicompare_vtable||
       networkable!=static_cast<unsigned char*>(actor)+0x160||!b||!b->release_networkable)return 0;
    return b->release_networkable(b->context,networkable);
}
int __cdecl destroy_remap(void* actor,unsigned bytes,void* networkable,const RelayFactoryBindings* b) noexcept {
    if(!networkable)return 1;
    if(!actor||bytes<remap_extent||entity_vtable(actor)!=remap_vtable||
       networkable!=static_cast<unsigned char*>(actor)+0x160||!b||!b->release_networkable)return 0;
    return b->release_networkable(b->context,networkable);
}
int __cdecl destroy_colorblend(void* actor,unsigned bytes,void* networkable,const RelayFactoryBindings* b) noexcept {
    if(!networkable)return 1;
    if(!actor||bytes<colorblend_extent||entity_vtable(actor)!=colorblend_vtable||
       networkable!=static_cast<unsigned char*>(actor)+0x160||!b||!b->release_networkable)return 0;
    return b->release_networkable(b->context,networkable);
}
int __cdecl destroy_campaign_trigger(void* a,unsigned n,void* networkable,const TriggerFactoryBindings* b) noexcept {
    if(!networkable)return 1;
    if(!a||n<4||!b||!b->release_networkable)return 0;
    const auto vt=entity_vtable(a);
    const unsigned extent=vt==dynamic_trigger_vtable?dynamic_trigger_extent:vt==secret_trigger_vtable?secret_trigger_extent:0;
    return extent&&n>=extent&&networkable==static_cast<unsigned char*>(a)+0x160&&b->release_networkable(b->context,networkable);
}
constexpr FactorySpec filter_specs[]{
    {"filter_base",Family::FilterBase,0x224f8ad4,0x22414800,0x3c8},
    {"filter_multi",Family::FilterMultiple,0x224f8ad8,0x2241480c,0x3f4},
    {"filter_activator_name",Family::FilterName,0x224f8adc,0x22414818,0x3cc},
    {"filter_activator_class",Family::FilterClass,0x224f8ae0,0x22414824,0x3cc}
};
constexpr unsigned filter_tables[]{base_filter_vtable,multi_filter_vtable,name_filter_vtable,class_filter_vtable};
int create_filter_factory(const char* name,const CampaignFilterFactoryBindings* b,FactoryResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!name||!b)return 0;
    unsigned i=0;for(;i<4;++i)if(equal_class(name,filter_specs[i].name))break;
    if(i==4){o->completed=1;return 1;}
    o->spec=&filter_specs[i];o->matched=1;
    const auto* owner=b->owner;const unsigned extent=o->spec->extent;
    if(!owner||!owner->allocate||!owner->base_construct||!owner->initialize||
       !owner->allocate(owner->context,extent,&o->actor,&o->bytes)||!o->actor||o->bytes<extent||
       !SinCampaignStorageReadableInternal(o->actor,extent))return 0;
    auto* a=static_cast<unsigned char*>(o->actor);
    if(!owner->base_construct(owner->context,a,1))return 0;
    // The original allocator zeroes fresh derived storage. Base filter
    // constructor sets six output words, leaving action heads untouched.
    unsigned pass_head=0,fail_head=0;
    std::memcpy(&pass_head,a+0x3ac,4);std::memcpy(&fail_head,a+0x3c4,4);
    if(pass_head||fail_head||a[0x394])return 0;
    for(unsigned off=0x3c8;off<(i==1?0x3e0u:extent);off+=4){
        unsigned value=0;std::memcpy(&value,a+off,4);if(value)return 0;
    }
    write_field(a,0,filter_tables[i]);
    for(unsigned off:{0x398u,0x3b0u}){
        write_field(a,off,0u);write_field(a,off+0xc,0xffffffffu);write_field(a,off+0x10,0u);
    }
    if(i==1)for(unsigned j=0;j<5;++j)write_field(a,0x3e0+j*4,0xffffffffu);
    if(!owner->initialize(owner->context,a,name))return 0;
    o->networkable=a+0x160;o->completed=1;return 1;
}
constexpr FactorySpec scripted_sequence_spec{"scripted_sequence",Family::ScriptedSequence,0x22505008u,0x2242ee70u,scripted_sequence_extent};
int create_scripted_sequence(const RelayFactoryBindings* owner,FactoryResult* o) noexcept {
    if(!o)return 0;*o={};o->spec=&scripted_sequence_spec;o->matched=1;
    if(!owner||!owner->allocate||!owner->base_construct||!owner->initialize||
       !owner->allocate(owner->context,scripted_sequence_extent,&o->actor,&o->bytes)||!o->actor||o->bytes<scripted_sequence_extent||
       !SinCampaignStorageReadableInternal(o->actor,scripted_sequence_extent))return 0;
    auto* a=static_cast<unsigned char*>(o->actor);
    // 221b01f0 calls CBaseEntity(0), unlike the earlier logic factories.
    if(!owner->base_construct(owner->context,a,0))return 0;
    // Fresh original allocation zeroes the entire derived extension.
    for(unsigned off=0x394;off<scripted_sequence_extent;++off)if(a[off])return 0;
    write_field(a,0,scripted_sequence_vtable);
    for(unsigned off:{0x3d0u,0x3d4u,0x3dcu})write_field(a,off,0xffffffffu);
    for(unsigned i=0;i<10;++i){
        const unsigned off=0x3e0+0x18*i;
        write_field(a,off,0u);write_field(a,off+0xc,0xffffffffu);write_field(a,off+0x10,0u);
    }
    if(!owner->initialize(owner->context,a,scripted_sequence_spec.name))return 0;
    o->networkable=a+0x160;o->completed=1;return 1;
}
bool known_map_vtable(unsigned vt) noexcept {
    if(vt==dynamic_trigger_vtable||vt==secret_trigger_vtable||campaign_filter_class(vt)||vt==scripted_sequence_vtable)return true;
    if(vt==colorblend_vtable||vt==remap_vtable||vt==multicompare_vtable||vt==autosave_vtable||vt==navigation_vtable||vt==menu_vtable||vt==counter_vtable||vt==compare_vtable||vt==case_vtable||vt==branch_vtable||vt==timer_vtable||vt==auto_vtable||vt==relay_vtable||vt==resupply_vtable||vt==antigen_vtable||vt==antigen_canister_vtable||vt==vial_vtable||vt==megahealth_vtable)return true;
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
    // CLogicNavigation::KeyValue 22177f90 handles exact navprop
    // before base normalization. Unknown values warn but are handled.
    if(vt==navigation_vtable&&equal_class(key,"navprop")){
        if(bytes<navigation_extent)return 0;
        if(equal_class(value,"Ignore")){
            write_field(actor,0x39c,1u);o->scalar=1;o->offset=0x39c;o->field_type=5;
        }else if(!b->navigation_warning||!b->navigation_warning(b->context,actor,value))return 0;
        o->handled=1;return done();
    }
    // CMathCounter::KeyValue 22179950 handles the exact startvalue
    // name before base # suffix normalization; original atoi wraps.
    if(vt==counter_vtable&&equal_class(key,"startvalue")){
        if(bytes<counter_extent)return 0;
        const char* p=value;while(*p==' '||(*p>='\t'&&*p<='\r'))++p;
        const bool negative=*p=='-';if(negative||*p=='+')++p;
        unsigned raw=0;while(*p>='0'&&*p<='9'){raw=raw*10u+unsigned(*p-'0');++p;}
        if(negative)raw=0u-raw;
        int scalar=0;std::memcpy(&scalar,&raw,4);
        write_field(actor,0x3a0,static_cast<float>(scalar));write_field(actor,0x3b0,1u);
        o->offset=0x3a0;o->field_type=1;o->scalar=o->handled=1;return done();
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
    constexpr Spec counter_keys[]{
        {"min",0x394,1},{"max",0x398,1},{"OutValue",0x3a0,11},
        {"OnHitMin",0x3b8,11},{"OnHitMax",0x3d0,11}
    };
    constexpr Spec compare_keys[]{
        {"InitialValue",0x394,1},{"CompareValue",0x398,1},
        {"OnLessThan",0x39c,11},{"OnEqualTo",0x3b4,11},
        {"OnNotEqualTo",0x3cc,11},{"OnGreaterThan",0x3e4,11}
    };
    constexpr Spec colorblend_keys[]{{"inmin",0x394,1},{"inmax",0x398,1},{"colormin",0x39c,9},{"colormax",0x3a0,9},{"OutColor",0x3a4,11}};
    constexpr Spec remap_keys[]{{"in1",0x394,1},{"in2",0x398,1},{"out1",0x39c,1},{"out2",0x3a0,1},{"OutValue",0x3a4,11}};
    constexpr Spec multicompare_keys[]{{"OnEqual",0x394,11},{"OnNotEqual",0x3ac,11},{"IntegerValue",0x3c4,5},{"ShouldComparetoValue",0x3c8,5}};
    constexpr Spec autosave_keys[]{{"NewLevelUnit",0x394,6}};
    constexpr Spec dynamic_keys[]{{"EstimatedTime",0x470,1},{"EstimatedDifficulty",0x474,5},{"EndOfGame",0x478,5}};
    // CBaseTrigger map 2250b17c, rows 0,2,6..8; derived fields
    // take priority. Remaining brush/base keys stay owner supplied.
    constexpr Spec trigger_keys[]{{"filtername",0x40c,2},{"StartDisabled",0x408,6},
        {"OnStartTouch",0x414,11},{"OnEndTouch",0x42c,11},{"OnEndTouchAll",0x444,11}};
    // 22505794 static rows 0..8: m_iszPreIdle is the MEMBER,
    // the external key is m_iszIdle. Runtime initializer 221b05d0
    // writes the ten output descriptors, not present in the disk image.
    constexpr Spec sequence_keys[]{{"m_iszIdle",0x394,2},{"m_iszPlay",0x398,2},{"m_iszPostIdle",0x39c,2},
        {"m_iszCustomMove",0x3a0,2},{"m_iszNextScript",0x3a4,2},{"m_iszEntity",0x3a8,2},
        {"m_fMoveTo",0x3ac,5},{"m_flRadius",0x3b0,1},{"m_flRepeat",0x3b4,1},
        {"OnBeginSequence",0x3e0,11},{"OnEndSequence",0x3f8,11}};
    constexpr Spec filter_base_keys[]{{"Negated",0x394,6},{"OnPass",0x398,11},{"OnFail",0x3b0,11}};
    constexpr Spec filter_multi_keys[]{{"FilterType",0x3c8,5},{"Filter01",0x3cc,2},{"Filter02",0x3d0,2},
        {"Filter03",0x3d4,2},{"Filter04",0x3d8,2},{"Filter05",0x3dc,2}};
    constexpr Spec filter_name_keys[]{{"filtername",0x3c8,2}};
    constexpr Spec filter_class_keys[]{{"filterclass",0x3c8,2}};
    constexpr Spec navigation_keys[]{{"target",0x90,2}};
    constexpr Spec menu_keys[]{
        {"resumeGamePressed",0x398,11},{"newGamePressed",0x3b0,11},{"optionsPressed",0x3c8,11},
        {"loadGamePressed",0x3e0,11},{"saveGamePressed",0x3f8,11},{"statsPressed",0x410,11},
        {"friendsPressed",0x428,11},{"quitPressed",0x440,11},
        {"resumeGameMousedOver",0x458,11},{"newGameMousedOver",0x470,11},{"optionsMousedOver",0x488,11},
        {"loadGameMousedOver",0x4a0,11},{"saveGameMousedOver",0x4b8,11},{"statsMousedOver",0x4d0,11},
        {"friendsMousedOver",0x4e8,11},{"quitMousedOver",0x500,11}
    };
    Spec numbered{};
    const Spec* spec=nullptr;
    auto match=[&](const auto& table){for(const auto& s:table)if(equal_class(name,s.key)){spec=&s;break;}};
    if(vt==antigen_vtable)match(dispenser);else if(vt==antigen_canister_vtable)match(canister);else if(vt==relay_vtable)match(relay);else if(vt==auto_vtable)match(auto_keys);else if(vt==timer_vtable)match(timer_keys);else if(vt==branch_vtable)match(branch_keys);else if(vt==compare_vtable)match(compare_keys);else if(vt==counter_vtable)match(counter_keys);else if(vt==menu_vtable)match(menu_keys);else if(vt==navigation_vtable)match(navigation_keys);else if(vt==autosave_vtable)match(autosave_keys);else if(vt==multicompare_vtable)match(multicompare_keys);else if(vt==remap_vtable)match(remap_keys);else if(vt==colorblend_vtable)match(colorblend_keys);
    else if(vt==dynamic_trigger_vtable)match(dynamic_keys);
    else if(vt==scripted_sequence_vtable){
        if(bytes<scripted_sequence_extent)return 0;
        match(sequence_keys);
        if(!spec)for(unsigned i=1;i<=8;++i){
            char output[]="OnScriptEvent00";output[14]=char('0'+i);
            if(equal_class(name,output)){numbered={nullptr,0x3f8+0x18*i,11};spec=&numbered;break;}
        }
    }
    else if(vt==multi_filter_vtable)match(filter_multi_keys);
    else if(vt==name_filter_vtable)match(filter_name_keys);
    else if(vt==class_filter_vtable)match(filter_class_keys);
    else if(vt==case_vtable){
        if(equal_class(name,"OnDefault")){numbered={nullptr,0x554,11};spec=&numbered;}
        else for(unsigned i=1;i<=16;++i){
            char key[]="Case00",out[]="OnCase00";
            key[4]=out[6]=char('0'+i/10);key[5]=out[7]=char('0'+i%10);
            if(equal_class(name,key)){numbered={nullptr,0x394+4*(i-1),2};spec=&numbered;break;}
            if(equal_class(name,out)){numbered={nullptr,0x3d4+0x18*(i-1),11};spec=&numbered;break;}
        }
    }
    if(!spec&&(vt==dynamic_trigger_vtable||vt==secret_trigger_vtable)){
        match(trigger_keys);if(spec)o->base_field=1;
    }
    if(!spec&&campaign_filter_class(vt)){match(filter_base_keys);if(spec)o->base_field=1;}
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
    }else if(spec->type==9){
        // Original 222793b1 -> 2228b4b0 -> 2228b420: bounded copy,
        // atoi at each pointer, then advance to the NEXT literal space.
        // Repeated spaces can duplicate a value; missing channels zero.
        char buffer[128]{};unsigned len=0;
        while(len<127&&value[len]){buffer[len]=value[len];++len;}
        const char* p=buffer;unsigned rgba=0;
        for(unsigned i=0;i<4;++i){
            const char* q=p;while(*q==' '||(*q>='\t'&&*q<='\r'))++q;
            const bool negative=*q=='-';if(negative||*q=='+')++q;
            unsigned raw=0;while(*q>='0'&&*q<='9'){raw=raw*10u+unsigned(*q-'0');++q;}
            if(negative)raw=0u-raw;
            rgba|=(raw&255u)<<(8*i);
            while(*p&&*p!=' ')++p;
            if(!*p)break;++p;
        }
        write_field(actor,o->offset,rgba);o->scalar=1;
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
int parse_impl(const char* s,unsigned n,const Bindings* b,Result* o,Collector collect,void* context,
    const CampaignFilterFactoryBindings* filters=nullptr,const ScriptedSequenceMapBindings* sequences=nullptr) noexcept {
    if(!o)return 0;*o={};
    if(!s||!b||(!b->create&&!b->native_factory)||(!b->key_value&&!b->native_keys)||(!collect&&!b->accept_entity))return 0;
    if((filters||sequences)&&(b->create||b->key_value||!b->native_factory||!b->native_keys||!valid_dictionary(b->native_factory->dictionary)))return 0;
    if(sequences&&!sequences->factory)return 0;
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
                if(sequences&&equal_class(classname,scripted_sequence_spec.name)){
                    if(!create_scripted_sequence(sequences->factory,&o->last_factory))return 0;
                }else if(filters){
                    if(!create_filter_factory(classname,filters,&o->last_factory))return 0;
                    if(!o->last_factory.matched&&!create_factory(classname,b->native_factory,&o->last_factory))return 0;
                }else if(!create_factory(classname,b->native_factory,&o->last_factory))return 0;
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
int __cdecl counter_spawn(void*,unsigned,CounterResult*) noexcept;
int __cdecl remap_spawn(void*,unsigned,RemapResult*) noexcept;
int __cdecl colorblend_spawn(void*,unsigned,ColorBlendResult*) noexcept;
int __cdecl menu_listen(void*,unsigned,bool,const MenuBindings*,MenuResult*) noexcept;
int __cdecl navigation_activate(void*,unsigned,const NavigationBindings*,NavigationResult*) noexcept;
int __cdecl defer_logic_remove(void*,unsigned,const DeferredRemovalBindings*,DeferredRemovalResult*) noexcept;
// InputEnable/Disable tail-jump to +28c/+290; Toggle +294 changes
// the solid trigger bit but deliberately does not write m_bDisabled.
int __cdecl trigger_control(void* a,unsigned n,const char* name,const TriggerControlBindings* b,TriggerControlResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!a||n<4||!name||!b)return 0;
    const auto vt=entity_vtable(a);
    const unsigned extent=vt==dynamic_trigger_vtable?dynamic_trigger_extent:vt==secret_trigger_vtable?secret_trigger_extent:0;
    if(!extent||n<extent)return 0;
    const bool enable=equal_class(name,"Enable"),disable=equal_class(name,"Disable"),toggle=equal_class(name,"Toggle");
    if(!enable&&!disable&&!toggle){o->completed=1;return 1;}
    o->handled=1;o->prior_flags=read_field<unsigned short>(a,0x120);
    const bool active=(o->prior_flags&8u)!=0;
    const bool next=toggle?!active:enable;
    o->requested_flags=next?o->prior_flags|8u:o->prior_flags&~8u;
    if(next!=active&&(!b->solid_flags||!b->refresh))return 0;
    if(!toggle){write_field(a,0x408,static_cast<unsigned char>(disable));o->disabled_written=1;}
    o->disabled=read_field<unsigned char>(a,0x408);
    if(next!=active){
        if(!b->solid_flags(b->context,static_cast<unsigned char*>(a)+0xfc,o->requested_flags))return 0;
        o->solid_called=1;
        if(!b->refresh(b->context,a,0))return 0;
        o->refreshed=o->changed=1;
    }
    o->completed=1;return 1;
}
bool campaign_valid(void* a,unsigned n,const CampaignTriggerBindings* b,CampaignTriggerResult* o) noexcept {
    if(!o)return false;*o={};
    if(!a||n<4||!b)return false;
    const auto vt=entity_vtable(a);o->dynamic=vt==dynamic_trigger_vtable;
    return o->dynamic?n>=dynamic_trigger_extent:vt==secret_trigger_vtable&&n>=secret_trigger_extent;
}
bool campaign_name(void* a,const CampaignTriggerBindings& b,const char*& name) noexcept {
    auto resolve=[&](unsigned token){
        name="";if(!token)return true;
        unsigned bytes=0;
        return b.string_text&&b.string_text(b.context,token,&name,&bytes)&&name&&bytes&&std::memchr(name,0,bytes<4096?bytes:4096);
    };
    if(!resolve(read_field<unsigned>(a,0xc0)))return false;
    return *name||resolve(read_field<unsigned>(a,0xc));
}
// 222b63b0 / 222b64d0. Owner event operations are explicit; original
// CreateEvent null skips every setter and still permits removal.
bool campaign_event(void* a,const char* name,int descriptor,float value,int decision,const CampaignTriggerBindings& b,CampaignTriggerResult& o,
    void* manager,int* publication,void* startup_globals=nullptr,void* startup_engine=nullptr) noexcept {
    if(manager){
        namespace fs=sin_port::fixed_service;
        const fs::NativeTriggerEvent event{o.dynamic?fs::TriggerDynamicEvent:fs::TriggerGameEvent,
            descriptor,-1,-1,decision,value,0,0,0,name};
        o.event_attempted=1;o.descriptor=descriptor;
        *publication=SinPublishCampaignEventInternal(manager,&event,b.owner_word,b.context,
            o.dynamic?static_cast<const unsigned char*>(a)+0x2bc:nullptr,startup_globals,startup_engine);
        if(*publication==fs::EventUnavailable){o.event_unavailable=1;return true;}
        if(*publication==fs::EventFired){o.event_fired=1;return true;}
        // Ordinary FireEvent false consumes the event. The original trigger
        // does not retry publication; continue its post-event lifecycle.
        return *publication==fs::EventFireReturnedFalse;
    }
    if(!b.create_event)return false;
    void* event=nullptr;o.event_attempted=1;o.descriptor=descriptor;
    if(!b.create_event(b.context,o.dynamic?"dynamic_difficulty":"game",false,&event))return false;
    if(!event){o.event_unavailable=1;return true;}
    if(!b.set_int||!b.set_string||!b.fire_event)return false;
    int owner=-1;
    if(o.dynamic&&(!b.owner_word||!b.owner_word(b.context,&owner)))return false;
    if(!b.set_int(b.context,event,"descriptor",descriptor)||
       !b.set_int(b.context,event,"owner",owner)||
       !b.set_int(b.context,event,"subject",-1)||
       !b.set_string(b.context,event,"name",name))return false;
    if(o.dynamic){
        if(!b.set_float||!b.set_int(b.context,event,"decision",decision)||
           !b.set_float(b.context,event,"value",value)||
           !b.set_float(b.context,event,"x",read_field<float>(a,0x2bc))||
           !b.set_float(b.context,event,"y",read_field<float>(a,0x2c0))||
           !b.set_float(b.context,event,"z",read_field<float>(a,0x2c4)))return false;
    }
    if(!b.fire_event(b.context,event,false))return false;
    o.event_fired=1;return true;
}
bool campaign_remove(void* a,const CampaignTriggerBindings& b,CampaignTriggerResult& o) noexcept {
    if(b.deferred){
        DeferredRemovalResult r{};
        if(!defer_logic_remove(a,removal_entity_extent(entity_vtable(a)),b.deferred,&r))return false;
        o.native_removed=1;
    }else if(!b.remove||!b.remove(b.context,a))return false;
    o.removed=1;return true;
}
int campaign_trigger_spawn_impl(void* a,unsigned n,const CampaignTriggerBindings* b,CampaignTriggerResult* o,void* manager,int* publication) noexcept {
    if(!campaign_valid(a,n,b,o)||!b->spawn_gate)return 0;
    bool suppress=false;if(!b->spawn_gate(b->context,&suppress))return 0;
    if(suppress){o->spawn_suppressed=1;if(!campaign_remove(a,*b,*o))return 0;}
    else{
        const char* name=nullptr;
        // Secret Spawn captures the resolved pointer BEFORE base Spawn.
        if(!o->dynamic&&!campaign_name(a,*b,name))return 0;
        if(!b->base_spawn||!b->base_spawn(b->context,a))return 0;o->base_spawned=1;
        if(o->dynamic){
            if(read_field<unsigned>(a,0xb4)&0x400u){
                if(!b->warning||!b->warning(b->context,"DD Trigger with don't destroy set\n"))return 0;
                o->warned=1;
            }
        }else if(read_field<unsigned char>(a,0x120)&8u){
            if(!campaign_event(a,name,11,0,0,*b,*o,manager,publication))return 0;
        }
    }
    o->completed=1;return 1;
}
int campaign_trigger_touch_impl(void* a,unsigned n,void* other,const CampaignTriggerBindings* b,CampaignTriggerResult* o,void* manager,int* publication,
    void* startup_globals=nullptr,void* startup_engine=nullptr) noexcept {
    if(!campaign_valid(a,n,b,o)||!other)return 0;
    bool player=false;
    if(startup_globals&&startup_engine){
        unsigned marker=0;if(!SinReadCampaignTouchMarkerInternal(other,&marker))return 0;
        const auto classification=campaign_player_marker(marker);
        if(classification==PlayerPredicate::Player)player=true;
        else if(classification==PlayerPredicate::Unknown){
            // Preserve the host route only for genuinely unreviewed classes.
            if(!b->is_player||!b->is_player(b->context,other,&player))return 0;
        }
    }else if(!b->is_player||!b->is_player(b->context,other,&player))return 0;
    if(!player){o->nonplayer=o->completed=1;return 1;}
    // The existing fixed-policy adapter is deliberately opt-in. Unlike
    // the original raw Touch leaf it refuses duplicate observations once
    // native deferred removal has marked the actor.
    const sin_port::fixed_service::Api* fixed=nullptr;
    static bool fixed_touch_active=false;
    struct FixedGuard {
        bool& active;bool owns=false;
        ~FixedGuard(){if(owns)active=false;}
    } guard{fixed_touch_active};
    if(o->dynamic&&b->fixed_campaign){
        if(b->difficulty_manager||!b->deferred||!b->deferred->queue||fixed_touch_active)return 0;
        if(read_field<unsigned>(a,0xb8)&1u){o->already_removed=o->completed=1;return 1;}
        using namespace sin_port::fixed_service;
        fixed=SiN_GetFixedDifficultyApi(abi_version,sizeof(sin_port::fixed_service::Api));
        State state{};if(!fixed||!fixed->get_state(&state))return 0;
        fixed_touch_active=guard.owns=true;
    }
    const char* name=nullptr;if(!campaign_name(a,*b,name))return 0;
    float value=0;int decision=0;
    if(o->dynamic){
        if(read_field<unsigned>(a,0xb8)&0x800u){if(!b->special||!b->special(b->context,a))return 0;o->special_called=1;}
        value=read_field<float>(a,0x470);decision=read_field<int>(a,0x474);
    }
    if(fixed&&!std::isfinite(value))return 0;
    if(!campaign_event(a,name,o->dynamic?0:12,value,decision,*b,*o,manager,publication,startup_globals,startup_engine))return 0;
    // EndOfGame is read after the event helper, and only its LOW BYTE.
    if(o->dynamic){
        const unsigned end=read_field<unsigned char>(a,0x478);
        if(fixed){
            // Existing fixed profile owns accounting. Observe once even
            // when CreateEvent returned null; no adaptive metric changes.
            if(!fixed->observe_trigger(decision,value,static_cast<int>(end)))return 0;
            o->fixed_observed=1;o->manager_signaled=end!=0;
        }else if(end){
            if(b->difficulty_manager){
                void* manager=nullptr;unsigned bytes=0;
                if(!b->difficulty_manager(b->context,&manager,&bytes)||!manager||bytes<0x268||
                   read_field<unsigned>(manager,0)!=0x2245cdacu)return 0;
                write_field(manager,0x11,static_cast<unsigned char>(1));o->native_manager=1;
            }else if(!b->end_game||!b->end_game(b->context))return 0;
            o->manager_signaled=1;
        }
    }
    if(!campaign_remove(a,*b,*o))return 0;
    o->completed=1;return 1;
}
int __cdecl campaign_trigger_spawn(void* a,unsigned n,const CampaignTriggerBindings* b,CampaignTriggerResult* o) noexcept {
    return campaign_trigger_spawn_impl(a,n,b,o,nullptr,nullptr);
}
int __cdecl campaign_trigger_touch(void* a,unsigned n,void* other,const CampaignTriggerBindings* b,CampaignTriggerResult* o) noexcept {
    return campaign_trigger_touch_impl(a,n,other,b,o,nullptr,nullptr);
}
int scripted_sequence_spawn(void*,unsigned,const ScriptedSequenceSpawnBindings*,ScriptedSequenceSpawnResult*) noexcept;
bool native_spawn_leaf(const SpawnBindings& b,void* a,unsigned n,bool& native,void* manager,const ScriptedSequenceSpawnBindings* sequence=nullptr) noexcept {
    const auto vt=read_field<unsigned>(a,0);
    if((vt==dynamic_trigger_vtable||vt==secret_trigger_vtable)&&b.campaign_triggers){native=true;CampaignTriggerResult r{};int publication=engine_event_not_attempted;return campaign_trigger_spawn_impl(a,n,b.campaign_triggers,&r,manager,&publication)!=0;}
    if(vt==scripted_sequence_vtable&&sequence){native=true;ScriptedSequenceSpawnResult r{};return scripted_sequence_spawn(a,n,sequence,&r)!=0;}
    if(vt==timer_vtable&&b.logic_timer){native=true;TimerResult r{};return timer_spawn(a,n,b.logic_timer,&r)!=0;}
    if(vt==counter_vtable&&b.native_math_counter){native=true;CounterResult r{};return counter_spawn(a,n,&r)!=0;}
    if(vt==remap_vtable&&b.native_math_remap){native=true;RemapResult r{};return remap_spawn(a,n,&r)!=0;}
    if(vt==colorblend_vtable&&b.native_math_colorblend){native=true;ColorBlendResult r{};return colorblend_spawn(a,n,&r)!=0;}
    if(vt==menu_vtable&&b.logic_menu){native=true;MenuResult r{};return menu_listen(a,n,false,b.logic_menu,&r)!=0;}
    native=(vt==resupply_vtable&&b.resupply)||(vt==antigen_vtable&&b.dispenser)||(vt==antigen_canister_vtable&&b.canister);
    if(!native)return b.virtual_spawn&&b.virtual_spawn(b.context,a);
    const auto* api=SiN_GetServerDynamicResupplyApi(1,sizeof(native_resupply::Api));if(!api)return false;
    if(vt==resupply_vtable){LifecycleResult r{};return api->spawn(a,n,b.resupply,&r)!=0;}
    if(vt==antigen_vtable){AntigenInteractionResult r{};return api->antigen_spawn(a,n,b.dispenser,&r)!=0;}
    CanisterResult r{};return api->canister_spawn(a,n,b.canister,&r)!=0;
}
int dispatch_spawn_impl(void* a,const SpawnBindings* b,DispatchResult* o,void* manager,const ScriptedSequenceSpawnBindings* sequence=nullptr) noexcept {
    if(!o)return 0;*o={};o->global_index=-1;
    if(!a){o->completed=1;return 1;}
    if(!b||!b->native_dispatch)return 0;
    const auto& d=*b->native_dispatch;
    unsigned n=0;if(!extent_ok(*b,a,0x37c,n)||!d.scope||!d.current_map||
        d.entry_count>sin_independent::kEntityHandleCapacity||(!d.entries&&d.entry_count))return 0;
    if(!d.scope(d.context,true))return 0;o->entered=1;
    auto finish=[&](bool ok){const bool closed=d.scope(d.context,false)!=0;o->closed=closed;o->completed=ok&&closed;return int(o->completed);};
    o->captured_handle=read_field<unsigned>(a,0x378);
    bool native=false;if(!native_spawn_leaf(*b,a,n,native,manager,sequence))return finish(false);
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
int __cdecl dispatch_spawn(void* a,const SpawnBindings* b,DispatchResult* o) noexcept {
    return dispatch_spawn_impl(a,b,o,nullptr);
}
bool spawn_actor(const SpawnBindings& b,SpawnResult& o,void* a,int& code,void* manager,const ScriptedSequenceSpawnBindings* sequence=nullptr) noexcept {
    if(b.native_dispatch){
        ++o.native_dispatches;
        if(!dispatch_spawn_impl(a,&b,&o.dispatch,manager,sequence))return false;
        code=o.dispatch.native_result;o.native_spawns+=o.dispatch.native_spawn;return true;
    }
    unsigned n=0;if(!extent_ok(b,a,4,n))return false;
    const auto vt=read_field<unsigned>(a,0);
    const bool native=((vt==dynamic_trigger_vtable||vt==secret_trigger_vtable)&&b.campaign_triggers)||(vt==colorblend_vtable&&b.native_math_colorblend)||(vt==remap_vtable&&b.native_math_remap)||(vt==menu_vtable&&b.logic_menu)||(vt==counter_vtable&&b.native_math_counter)||(vt==timer_vtable&&b.logic_timer)||(vt==resupply_vtable&&b.resupply)||(vt==antigen_vtable&&b.dispenser)||(vt==antigen_canister_vtable&&b.canister);
    if(!native)return b.spawn&&b.spawn(b.context,a,&code);
    if(!b.native_spawn_begin||!b.native_spawn_end)return false;
    bool proceed=false;if(!b.native_spawn_begin(b.context,a,&proceed))return false;
    if(proceed){
        if(vt==dynamic_trigger_vtable||vt==secret_trigger_vtable){CampaignTriggerResult r{};int publication=engine_event_not_attempted;if(!campaign_trigger_spawn_impl(a,n,b.campaign_triggers,&r,manager,&publication))return false;}
        else if(vt==menu_vtable){MenuResult r{};if(!menu_listen(a,n,false,b.logic_menu,&r))return false;}
        else if(vt==counter_vtable){CounterResult r{};if(!counter_spawn(a,n,&r))return false;}
        else if(vt==remap_vtable){RemapResult r{};if(!remap_spawn(a,n,&r))return false;}
        else if(vt==colorblend_vtable){ColorBlendResult r{};if(!colorblend_spawn(a,n,&r))return false;}
        else if(vt==timer_vtable){TimerResult r{};if(!timer_spawn(a,n,b.logic_timer,&r))return false;}
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
bool native_named_output(void*,unsigned,void*,unsigned,bool,const native_weapon_tables::ServerEntityVoidOutputBindings*,const RelayQueueBindings*,unsigned&) noexcept;
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
        unsigned queued=0;
        if(b->native_queue){
            if(!native_named_output(actor,bytes,nullptr,off,false,b->output,b->native_queue,queued))return false;
            o->native_output_queued+=queued;
        }else{
            ServerEntityVoidOutputResult r{};
            if(!fire->fire(static_cast<unsigned char*>(actor)+off,0x18,nullptr,actor,bytes,0.f,b->output,&r))return false;
            queued=r.queued;
        }
        ++o->outputs;o->queued+=queued;return true;
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
        if(b->deferred){
            DeferredRemovalResult removed{};
            if(!defer_logic_remove(actor,bytes,b->deferred,&removed))return 0;
        }else if(!b->remove||!b->remove(b->context,actor))return 0;
        o->removed=1;
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
int spawn_ordinary_impl(SpawnRow* rows,unsigned count,bool activate,const SpawnBindings* b,SpawnResult* o,void* manager,const ScriptedSequenceSpawnBindings* sequence=nullptr) noexcept {
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
            int result=0;ok=spawn_actor(*b,*o,rows[i].actor,result,manager,sequence);
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
            }else if(ok&&read_field<unsigned>(a,0)==navigation_vtable&&b->logic_navigation){
                NavigationResult r{};ok=navigation_activate(a,n,b->logic_navigation,&r)!=0;if(ok)++o->native_activations;
            }else if(ok)ok=b->activate&&b->activate(b->context,a);
            const bool closed=b->engine_scope(b->context,false)!=0;if(closed)++o->engine_closes;
            ok=ok&&closed;if(ok)++o->activated;
        }
        const bool closed=scope(name,false);if(!ok||!closed)return 0;
    }
    o->completed=1;return 1;
}
int __cdecl spawn_ordinary(SpawnRow* rows,unsigned count,bool activate,const SpawnBindings* b,SpawnResult* o) noexcept {
    return spawn_ordinary_impl(rows,count,activate,b,o,nullptr);
}
struct Pipeline {
    const PipelineBindings& b;PipelineResult& o;void* manager;const ScriptedSequenceSpawnBindings* sequence=nullptr;
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
    if(!spawn_actor(*p.b.spawn,result,a,code,p.manager,p.sequence))return false;
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
int parse_all_impl(const char* s,unsigned n,bool activate,const PipelineBindings* b,PipelineResult* o,void* manager,
    const CampaignFilterFactoryBindings* filters=nullptr,const ScriptedSequenceMapBindings* sequences=nullptr) noexcept {
    if(!o)return 0;*o={};
    if(!s||!b||!b->parse||!b->spawn||!b->classify||!b->scope)return 0;
    // LevelInit defers activation to ServerActivate. Require the complete
    // native DispatchSpawn wrapper rather than a hosted virtual spawn.
    if(sequences&&(activate||!sequences->spawn||!b->spawn->native_dispatch))return 0;
    Pipeline p{*b,*o,manager,sequences?sequences->spawn:nullptr};constexpr char outer[]="MapEntity_ParseAllEntities",templates[]="MapEntity_ParseAllEntities_SpawnTemplates";
    if(!pipeline_scope(p,outer,true))return 0;
    bool ok=parse_impl(s,n,b->parse,&o->parse,&collect_entity,&p,filters,sequences)!=0;
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
    if(ok)ok=spawn_ordinary_impl(p.rows,p.ordinary,activate,b->spawn,&o->spawn,manager,p.sequence)!=0;
    const bool closed=pipeline_scope(p,outer,false);o->completed=ok&&closed;return int(o->completed);
}
int __cdecl parse_all(const char* s,unsigned n,bool activate,const PipelineBindings* b,PipelineResult* o) noexcept {
    return parse_all_impl(s,n,activate,b,o,nullptr);
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
    if(b->native_fire){
        if(!native_named_output(actor,bytes,activator,relay_output,false,b->output,b->native_queue,o->queued))return 0;
        o->native_output_queued=o->queued;
    }else{
        const auto* fire=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
        ServerEntityVoidOutputResult fr{};
        if(!fire||!b->output||!fire->fire(a+relay_output,0x18,activator,actor,bytes,0.f,b->output,&fr))return 0;
        o->queued=fr.queued;
    }
    o->fired=1;
    unsigned flags=0;std::memcpy(&flags,a+0xb4,4);
    if(flags&1){
        if(b->deferred){DeferredRemovalResult r{};if(!defer_logic_remove(actor,bytes,b->deferred,&r))return 0;o->native_removed=1;}
        else if(!b->remove||!b->remove(b->context,actor))return 0;
        o->removed=1;return done();
    }
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
    if(!fire||!b.output)return false;
    if(b.native_queue){
        if(!native_named_output(a,n,a,off,false,b.output,b.native_queue,o.queued))return false;
        o.native_output_queued=o.queued;
    }else{
        if(!fire->fire(static_cast<unsigned char*>(a)+off,0x18,a,a,n,0.f,b.output,&r))return false;
        o.queued=r.queued;
    }
    o.output_offset=off;o.fired=1;
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
        if(!api||!b->output)return 0;
        if(b->native_queue){
            if(!native_named_output(a,n,activator,o->output_offset,false,b->output,b->native_queue,o->queued))return 0;
            o->native_output_queued=o->queued;
        }else{
            if(!api->fire(static_cast<unsigned char*>(a)+o->output_offset,0x18,activator,a,n,0.f,b->output,&result))return 0;
            o->queued=result.queued;
        }
        o->fired=1;
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
    if(b.format_variant&&(type==1||type==3||type==9||type==13)){
        if(!b.format_variant(b.context,variant,0x14,buffer,sizeof(buffer))||!std::memchr(buffer,0,sizeof(buffer)))return false;
        *text=buffer;return true;
    }
    // Original 221561d0: Q_snprintf with %g, [%g %g %g], and
    // %d %d %d %d. Finite %g uses six significant digits. Exact
    // legacy vstdlib exponent/nonfinite spelling remains owner overridable.
    if(type==1||type==3||type==9){
        char* p=buffer;char* end=buffer+sizeof(buffer)-1;
        auto literal=[&](char ch){if(p==end)return false;*p++=ch;return true;};
        auto scalar=[&](float value){
            if(!std::isfinite(value))return false;
            const auto r=std::to_chars(p,end,double(value),std::chars_format::general,6);
            if(r.ec!=std::errc{})return false;p=r.ptr;return true;
        };
        if(type==1){if(!scalar(read_field<float>(variant,0)))return false;}
        else if(type==3){
            if(!literal('['))return false;
            for(unsigned i=0;i<3;++i){if(i&&!literal(' '))return false;if(!scalar(read_field<float>(variant,i*4)))return false;}
            if(!literal(']'))return false;
        }else for(unsigned i=0;i<4;++i){
            if(i&&!literal(' '))return false;
            const auto r=std::to_chars(p,end,unsigned(read_field<unsigned char>(variant,i)));
            if(r.ec!=std::errc{})return false;p=r.ptr;
        }
        *p=0;*text=buffer;return true;
    }
    if(type==13){
        if(b.entry_count>sin_independent::kEntityHandleCapacity||(!b.entries&&b.entry_count))return false;
        const auto handle=read_field<unsigned>(variant,0xc);
        void* entity=sin_independent::EntityHandleView(b.entries,b.entry_count).Resolve(handle);
        if(!entity){*text="<<null entity>>";return true;}
        unsigned bytes=0;
        if(!b.entity_extent||!b.entity_extent(b.entity_context,entity,&bytes)||bytes<0xc4)return false;
        return case_text(b,read_field<unsigned>(entity,0xc0),text);
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
    const auto* fire=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
    ServerEntityVoidOutputResult r{};
    if(!fire||!b->output)return 0;
    auto* out=static_cast<unsigned char*>(a)+off;
    // Original OnDefault passes its stored by-value variant to 22156680.
    // Numbered matches and PickRandom instead fire a void event.
    if(b->native_queue){
        if(!native_named_output(a,n,activator,off,o->defaulted!=0,b->output,b->native_queue,o->queued))return 0;
        o->native_output_queued=o->queued;
    }else{
        if(o->defaulted&&b->output->enqueue_variant){
            if(!fire->fire_variant(out,0x18,activator,a,n,0.f,out,0x14,b->output,&r))return 0;
        }else if(!fire->fire(out,0x18,activator,a,n,0.f,b->output,&r))return 0;
        o->queued=r.queued;
    }
    o->fired=1;o->completed=1;return 1;
}
// Original 2212dc30 stores the FLOAT value and type in the output
// object, then passes its five-word variant by value to 22156680.
bool compare_fire(void* a,unsigned n,unsigned offset,void* activator,float value,const CompareBindings& b,CompareResult& o) noexcept {
    using namespace sin_port::native_weapon_tables;
    const auto* fire=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
    if(!fire||!b.output||(!b.native_queue&&!b.output->enqueue_variant))return false;
    auto* out=static_cast<unsigned char*>(a)+offset;
    write_field(out,0,value);write_field(out,0x10,1u);
    unsigned queued=0;
    if(b.native_queue){
        if(!native_named_output(a,n,activator,offset,true,b.output,b.native_queue,queued))return false;
        o.native_output_queued+=queued;
    }else{
        ServerEntityVoidOutputResult r{};
        if(!fire->fire_variant(out,0x18,activator,a,n,0.f,out,0x14,b.output,&r))return false;
        queued=r.queued;
    }
    if(!o.outputs)o.first_offset=offset;
    ++o.outputs;o.last_offset=offset;o.queued+=queued;return true;
}
int __cdecl compare_input(void* a,unsigned n,const char* name,void* activator,unsigned type,float value,const CompareBindings* b,CompareResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!a||n<compare_extent||entity_vtable(a)!=compare_vtable||!name||!b)return 0;
    o->handled=1;bool compare=false;
    const float candidate=type==1?value:0.f;
    if(equal_class(name,"SetValue")||equal_class(name,"SetValueCompare")){
        write_field(a,0x394,candidate);o->stored=1;
        compare=equal_class(name,"SetValueCompare");
    }else if(equal_class(name,"SetCompareValue")){
        write_field(a,0x398,candidate);o->stored=1;
    }else if(equal_class(name,"Compare"))compare=true;
    else o->handled=0;
    o->value=read_field<float>(a,0x394);o->threshold=read_field<float>(a,0x398);
    if(compare){
        o->compared=1;
        // x87 FUCOMPP parity: equality only (including signed zero).
        // Unordered takes not-equal then the non-greater branch.
        if(o->value==o->threshold){
            if(!compare_fire(a,n,0x3b4,activator,o->value,*b,*o))return 0;
        }else{
            if(!compare_fire(a,n,0x3cc,activator,o->value,*b,*o))return 0;
            o->threshold=read_field<float>(a,0x398);
            if(!compare_fire(a,n,o->value>o->threshold?0x3e4:0x39c,activator,o->value,*b,*o))return 0;
        }
    }
    o->completed=1;return 1;
}
// CMathRemap::Spawn 22178bc0: equality only, including signed
// zeros; unordered bounds are not normalized and descending bounds stay.
int __cdecl remap_spawn(void* a,unsigned n,RemapResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!a||n<remap_extent||entity_vtable(a)!=remap_vtable)return 0;
    if(read_field<float>(a,0x394)==read_field<float>(a,0x398)){
        write_field(a,0x394,0.f);write_field(a,0x398,1.f);o->reset_bounds=1;
    }
    o->handled=o->completed=1;return 1;
}
// CMathRemap::InputValue 2217a610. Clamp high before low,
// suppress changed/unordered input only with original spawnflag bit 0.
// Store intermediate result to float BEFORE the ordered output clamp.
int __cdecl remap_input(void* a,unsigned n,const char* name,void* activator,unsigned type,float value,const RemapBindings* b,RemapResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!a||n<remap_extent||entity_vtable(a)!=remap_vtable||!name||!b)return 0;
    if(!equal_class(name,"InValue")){o->completed=1;return 1;}
    o->handled=1;o->input=type==1?value:0.f;
    const float lo=read_field<float>(a,0x394),hi=read_field<float>(a,0x398);
    const float v=o->input>hi?hi:o->input<lo?lo:o->input;
    o->clamped_input=v;o->clamped=v!=o->input;
    if(o->clamped&&(read_field<unsigned>(a,0xb4)&1u)){o->suppressed=o->completed=1;return 1;}
    const float out1=read_field<float>(a,0x39c),out2=read_field<float>(a,0x3a0);
    // Keep x87 instruction order (subtract, multiply, divide, add).
    // Host double intermediates approximate default x87 precision;
    // exhaustive floating-point parity remains a retail-test boundary.
    float result=static_cast<float>((double(v)-lo)*(double(out2)-out1)/(double(hi)-lo)+out1);
    if(out1<out2){if(result>out2)result=out2;else if(result<out1)result=out1;}
    else {if(result>out1)result=out1;else if(result<out2)result=out2;}
    o->value=result;
    // 2212dc30/22154e40 changes the scalar/type words only.
    using namespace sin_port::native_weapon_tables;
    const auto* fire=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
    if(!fire||!b->output||(!b->native_queue&&!b->output->enqueue_variant))return 0;
    auto* out=static_cast<unsigned char*>(a)+0x3a4;
    write_field(out,0,result);write_field(out,0x10,1u);o->stored=1;
    if(b->native_queue){
        if(!native_named_output(a,n,activator,0x3a4,true,b->output,b->native_queue,o->queued))return 0;
        o->native_output_queued=o->queued;
    }else{
        ServerEntityVoidOutputResult r{};
        if(!fire->fire_variant(out,0x18,activator,a,n,0.f,out,0x14,b->output,&r))return 0;
        o->queued=r.queued;
    }
    o->completed=1;return 1;
}
// Original 22178c00 normalizes equal endpoints and then swaps reversed
// input bounds. The endpoint colors are NOT swapped.
int __cdecl colorblend_spawn(void* a,unsigned n,ColorBlendResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!a||n<colorblend_extent||entity_vtable(a)!=colorblend_vtable)return 0;
    float lo=read_field<float>(a,0x394),hi=read_field<float>(a,0x398);
    if(lo==hi){lo=0.f;hi=1.f;write_field(a,0x394,lo);write_field(a,0x398,hi);o->reset_bounds=1;}
    if(lo>hi){write_field(a,0x394,hi);write_field(a,0x398,lo);o->swapped=1;}
    o->handled=o->completed=1;return 1;
}
// Original 2217a740: high-first clamp, flag1 suppression and four
// independent unsigned-byte channels, including alpha. No final clamp.
// CRT 223a63bc truncates to integer; the handler stores AL only.
int __cdecl colorblend_input(void* a,unsigned n,const char* name,void* activator,unsigned type,float value,const ColorBlendBindings* b,ColorBlendResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!a||n<colorblend_extent||entity_vtable(a)!=colorblend_vtable||!name||!b)return 0;
    if(!equal_class(name,"InValue")){o->completed=1;return 1;}
    o->handled=1;o->input=type==1?value:0.f;
    const float lo=read_field<float>(a,0x394),hi=read_field<float>(a,0x398);
    const float v=o->input>hi?hi:o->input<lo?lo:o->input;
    o->clamped_input=v;o->clamped=v!=o->input;
    if(o->clamped&&(read_field<unsigned>(a,0xb4)&1u)){o->suppressed=o->completed=1;return 1;}
    // Double intermediates follow x87 instruction order. Exact extended
    // precision and exotic control-word parity remain unverified.
    const double numerator=double(v)-lo,denominator=double(hi)-lo;
    unsigned rgba=0;
    for(unsigned i=0;i<4;++i){
        const int from=read_field<unsigned char>(a,0x39c+i),to=read_field<unsigned char>(a,0x3a0+i);
        const double channel=double(to-from)*numerator/denominator+from;
        // Indefinite signed64 conversion has low byte zero. Avoid undefined
        // host float-to-integer conversions and preserve low-byte wrapping.
        const auto truncated=std::trunc(channel);
        const auto converted=std::isfinite(truncated)&&truncated>=-9223372036854775808.0&&truncated<9223372036854775808.0?static_cast<std::int64_t>(truncated):std::numeric_limits<std::int64_t>::min();
        rgba|=(static_cast<unsigned>(converted)&255u)<<(8*i);
    }
    o->rgba=rgba;
    using namespace sin_port::native_weapon_tables;
    const auto* fire=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
    if(!fire||!b->output||(!b->native_queue&&!b->output->enqueue_variant))return 0;
    auto* out=static_cast<unsigned char*>(a)+0x3a4;
    // Original COutputColor::Set 2217a3b0 -> 22154e40 type9.
    write_field(out,0,rgba);write_field(out,0x10,9u);o->stored=1;
    if(b->native_queue){
        if(!native_named_output(a,n,activator,0x3a4,true,b->output,b->native_queue,o->queued))return 0;
        o->native_output_queued=o->queued;
    }else{
        ServerEntityVoidOutputResult r{};
        if(!fire->fire_variant(out,0x18,activator,a,n,0.f,out,0x14,b->output,&r))return 0;
        o->queued=r.queued;
    }
    o->completed=1;return 1;
}
bool counter_valid(void* a,unsigned n,CounterResult* o) noexcept {
    if(!o)return false;*o={};
    return a&&n>=counter_extent&&entity_vtable(a)==counter_vtable;
}
bool counter_bounded(void* a) noexcept {
    return read_field<float>(a,0x394)!=0.f||read_field<float>(a,0x398)!=0.f;
}
float counter_clamp(void* a,float value,CounterResult& o) noexcept {
    const float hi=read_field<float>(a,0x398),lo=read_field<float>(a,0x394);
    if(value>hi){o.clamped=1;return hi;}
    if(value<lo){o.clamped=1;return lo;}
    return value;
}
void counter_store(void* a,float value,CounterResult& o) noexcept {
    write_field(a,0x3a0,value);write_field(a,0x3b0,1u);o.stored=1;
}
void counter_snapshot(void* a,CounterResult& o) noexcept {
    o.value=read_field<float>(a,0x3a0);o.minimum=read_field<float>(a,0x394);o.maximum=read_field<float>(a,0x398);
}
int __cdecl counter_spawn(void* a,unsigned n,CounterResult* o) noexcept {
    if(!counter_valid(a,n,o))return 0;
    const float lo=read_field<float>(a,0x394),hi=read_field<float>(a,0x398);
    if(lo>hi){write_field(a,0x394,hi);write_field(a,0x398,lo);o->swapped=1;}
    // Original 221799b0: zero/zero means unlimited, no variant setter.
    if(counter_bounded(a))counter_store(a,counter_clamp(a,read_field<float>(a,0x3a0),*o),*o);
    counter_snapshot(a,*o);o->handled=o->completed=1;return 1;
}
bool counter_emit(void* a,unsigned n,unsigned offset,void* activator,const CounterBindings& b,CounterResult& o,bool typed) noexcept {
    using namespace sin_port::native_weapon_tables;
    const auto* fire=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
    if(!fire||!b.output)return false;
    unsigned queued=0;
    if(b.native_queue){
        if(!native_named_output(a,n,activator,offset,typed,b.output,b.native_queue,queued))return false;
        o.native_output_queued+=queued;
    }else{
        auto* out=static_cast<unsigned char*>(a)+offset;ServerEntityVoidOutputResult r{};
        if(typed){
            if(!b.output->enqueue_variant||!fire->fire_variant(out,0x18,activator,a,n,0.f,out,0x14,b.output,&r))return false;
        }else if(!fire->fire(out,0x18,activator,a,n,0.f,b.output,&r))return false;
        queued=r.queued;
    }
    ++o.outputs;o.queued+=queued;return true;
}
bool counter_update(void* a,unsigned n,float candidate,void* activator,const CounterBindings& b,CounterResult& o) noexcept {
    // Original 2217aa00: boundary events before clamp and typed OutValue.
    // The min check and clamp reread live bounds after earlier outputs.
    if(counter_bounded(a)){
        if(candidate>=read_field<float>(a,0x398)){
            if(!read_field<unsigned char>(a,0x39d)){
                write_field(a,0x39d,static_cast<unsigned char>(1));o.hit_max=1;
                if(!counter_emit(a,n,0x3d0,activator,b,o,false))return false;
            }
        }else write_field(a,0x39d,static_cast<unsigned char>(0));
        if(candidate<=read_field<float>(a,0x394)){
            if(!read_field<unsigned char>(a,0x39c)){
                write_field(a,0x39c,static_cast<unsigned char>(1));o.hit_min=1;
                if(!counter_emit(a,n,0x3b8,activator,b,o,false))return false;
            }
        }else write_field(a,0x39c,static_cast<unsigned char>(0));
        candidate=counter_clamp(a,candidate,o);
    }
    counter_store(a,candidate,o);
    return counter_emit(a,n,0x3a0,activator,b,o,true);
}
int __cdecl counter_input(void* a,unsigned n,const char* name,void* activator,unsigned type,float value,const CounterBindings* b,CounterResult* o) noexcept {
    if(!counter_valid(a,n,o)||!name||!b)return 0;
    const float arg=type==1?value:0.f,current=read_field<float>(a,0x3a0);
    float candidate=current;bool update=true;o->handled=1;
    if(equal_class(name,"Add"))candidate=static_cast<float>(double(current)+arg);
    else if(equal_class(name,"Subtract"))candidate=static_cast<float>(double(current)-arg);
    else if(equal_class(name,"Multiply"))candidate=static_cast<float>(double(current)*arg);
    else if(equal_class(name,"Divide")){
        if(type!=1||arg==0.f){
            if(b->divide_warning&&!b->divide_warning(b->context,a))return 0;
            o->warned=1;candidate=read_field<float>(a,0x3a0); // original reload after warning
        }else candidate=static_cast<float>(double(current)/arg);
    }else if(equal_class(name,"SetValue"))candidate=arg;
    else if(equal_class(name,"SetValueNoFire")){
        candidate=counter_bounded(a)?counter_clamp(a,arg,*o):arg;
        counter_store(a,candidate,*o);update=false;
    }else if(equal_class(name,"SetHitMax")){
        write_field(a,0x398,arg);if(arg<read_field<float>(a,0x394))write_field(a,0x394,arg);
    }else if(equal_class(name,"SetHitMin")){
        write_field(a,0x394,arg);if(arg>read_field<float>(a,0x398))write_field(a,0x398,arg);
    }else{o->handled=0;update=false;}
    if(update&&!counter_update(a,n,candidate,activator,*b,*o))return 0;
    counter_snapshot(a,*o);o->completed=1;return 1;
}
// Original 221544d0 walks +14 head, caches row+18 before releasing
// the pool record and deliberately leaves the destroyed output unchanged.
// Derived destructors call this in reverse member order, then base.
// Independent owner data is preflighted before any destructive callback;
// a failed callback after release begins is terminal for that storage.
int __cdecl registry_unlink_logic(void*,unsigned,unsigned,const RegistryRemovalBindings*,RegistryRemovalResult*) noexcept;
int destroy_entity_members(void* a,unsigned n,unsigned flags,const LogicDestroyBindings* b,LogicDestroyResult* o,const TriggerDestroyBindings* trigger) noexcept {
    if(!o)return 0;*o={};
    if(!a||n<4||!b||(!b->base_destroy&&!b->native_base)||((flags&1u)&&!b->free_actor))return 0;
    if(b->native_base&&(!b->native_base->registry||!b->native_base->registry->registry||!b->native_base->prefix||!b->native_base->suffix))return 0;
    unsigned first=0,count=0,extent=0;
    const auto vt=entity_vtable(a);const bool menu=vt==menu_vtable;
    const bool is_trigger=vt==dynamic_trigger_vtable||vt==secret_trigger_vtable;
    if(is_trigger!=bool(trigger))return 0;
    switch(vt){
        case dynamic_trigger_vtable:first=0x414;count=3;extent=dynamic_trigger_extent;break;
        case secret_trigger_vtable:first=0x414;count=3;extent=secret_trigger_extent;break;
        case scripted_sequence_vtable:first=0x3e0;count=10;extent=scripted_sequence_extent;break; // 221b02e0 reverse order
        case relay_vtable:first=0x394;count=1;extent=relay_extent;break;
        case auto_vtable:first=0x394;count=7;extent=auto_extent;break;
        case timer_vtable:first=0x394;count=3;extent=timer_extent;break;
        case branch_vtable:first=0x398;count=2;extent=branch_extent;break;
        case case_vtable:first=0x3d4;count=17;extent=case_extent;break;
        case compare_vtable:first=0x39c;count=4;extent=compare_extent;break;
        case counter_vtable:first=0x3a0;count=3;extent=counter_extent;break;
        case menu_vtable:first=0x398;count=16;extent=menu_extent;break;
        case navigation_vtable:extent=navigation_extent;break;
        case autosave_vtable:extent=autosave_extent;break;
        case multicompare_vtable:first=0x394;count=2;extent=multicompare_extent;break;
        case remap_vtable:first=0x3a4;count=1;extent=remap_extent;break;
        case colorblend_vtable:first=0x3a4;count=1;extent=colorblend_extent;break;
        default:extent=campaign_filter_extent(vt);if(!extent)return 0;first=0x398;count=2;break;
    }
    if(n<extent||(menu&&(!b->unregister_menu||read_field<unsigned>(a,0x394)!=menu_listener_vtable))||
       (vt==navigation_vtable&&read_field<unsigned>(a,0x394)!=navigation_listener_vtable))return 0;
    constexpr unsigned max_per_output=256,max_actions=22*max_per_output;
    struct Record {unsigned token;void* row;};
    Record plan[max_actions]{};
    auto range=[](const void* p,unsigned bytes,std::uintptr_t& begin,std::uintptr_t& end){
        begin=reinterpret_cast<std::uintptr_t>(p);
        if(!p||begin>std::uintptr_t(-1)-bytes)return false;
        end=begin+bytes;return true;
    };
    std::uintptr_t actor_begin=0,actor_end=0;
    if(!range(a,n,actor_begin,actor_end))return 0;
    // CMultiInputVar destructor 22154c20 uses a special head-last
    // release order, but the same 28-byte pool as output actions.
    if(vt==multicompare_vtable){
        unsigned token=read_field<unsigned>(a,0x3cc);
        while(token){
            if(o->comparisons==max_per_output||!b->resolve||!b->release_action)return 0;
            void* row=nullptr;unsigned bytes=0;
            if(!b->resolve(b->context,token,&row,&bytes)||bytes<0x1c)return 0;
            std::uintptr_t begin=0,end=0;
            if(!range(row,0x1c,begin,end)||(begin<actor_end&&actor_begin<end))return 0;
            for(unsigned j=0;j<o->comparisons;++j){
                const auto prev=reinterpret_cast<std::uintptr_t>(plan[j].row);
                if(plan[j].token==token||(begin<prev+0x1c&&prev<end))return 0;
            }
            plan[o->comparisons++]={token,row};token=read_field<unsigned>(row,0x18);
        }
    }
    auto output_plan=[&](unsigned off,bool base)->bool{
        unsigned token=read_field<unsigned>(a,off+0x14),list_count=0;
        if(base)++o->base_outputs;else ++o->outputs;
        while(token){
            const unsigned total=o->comparisons+o->actions+o->base_actions;
            if(list_count++==max_per_output||total==max_actions||!b->resolve||!b->release_action)return false;
            void* row=nullptr;unsigned bytes=0;
            if(!b->resolve(b->context,token,&row,&bytes)||bytes<0x1c)return 0;
            std::uintptr_t begin=0,end=0;
            if(!range(row,0x1c,begin,end)||(begin<actor_end&&actor_begin<end))return 0;
            for(unsigned j=0;j<total;++j){
                const auto prev=reinterpret_cast<std::uintptr_t>(plan[j].row);
                if(plan[j].token==token||(begin<prev+0x1c&&prev<end))return false;
            }
            plan[total]={token,row};
            if(base)++o->base_actions;else ++o->actions;
            token=read_field<unsigned>(row,0x18);
        }
        return true;
    };
    for(unsigned i=count;i>0;--i)if(!output_plan(first+(i-1)*0x18,false))return 0;
    if(b->native_base&&b->native_base->native_outputs)
        for(unsigned i=5;i>0;--i)if(!output_plan(0x2dc+(i-1)*0x18,true))return 0;
    // CBaseTrigger destructor 221be440 purges its vector BEFORE
    // the three derived output members. 22044c90 external sentinel
    // -1 preserves the allocation and mirrors the pointer at +46c.
    unsigned vector_token=0;void* vector_storage=nullptr;bool vector_external=false;
    if(trigger){
        vector_token=read_field<unsigned>(a,0x45c);
        const int capacity=read_field<int>(a,0x460),size=read_field<int>(a,0x468);
        vector_external=read_field<unsigned>(a,0x464)==0xffffffffu;
        if(size<0||capacity<0||size>capacity||(capacity&&!vector_token))return 0;
        if(vector_token&&!vector_external){
            unsigned bytes=0;
            if(!trigger->resolve_vector||!trigger->free_vector||
               !trigger->resolve_vector(trigger->context,vector_token,&vector_storage,&bytes)||!bytes)return 0;
            std::uintptr_t begin=0,end=0;
            if(!range(vector_storage,bytes,begin,end)||(begin<actor_end&&actor_begin<end))return 0;
            for(unsigned j=0;j<o->comparisons+o->actions+o->base_actions;++j){
                const auto prev=reinterpret_cast<std::uintptr_t>(plan[j].row);
                if(plan[j].token==vector_token||(begin<prev+0x1c&&prev<end))return 0;
            }
        }
    }
    o->validated=1;
    if(trigger){
        write_field(a,0x468,0u);
        if(vector_external){
            write_field(a,0x46c,vector_token);o->trigger_vector_external=1;
        }else{
            if(vector_token){
                if(!trigger->free_vector(trigger->context,vector_token,vector_storage))return 0;
                write_field(a,0x45c,0u);o->trigger_vector_freed=1;
            }
            write_field(a,0x460,0u);write_field(a,0x46c,0u);
        }
        o->trigger_vector_purged=1;
    }
    // Original 2217ddd0 resets derived vtables and unregisters the
    // secondary interface before releasing any output rows.
    if(menu){
        write_field(a,0,menu_vtable);write_field(a,0x394,menu_listener_vtable);
        if(!b->unregister_menu(b->context,static_cast<unsigned char*>(a)+0x394))return 0;
        o->unregistered=1;
    }
    if(o->comparisons){
        for(unsigned i=1;i<o->comparisons;++i){
            write_field(plan[0].row,0x18,i+1<o->comparisons?plan[i+1].token:0u);
            if(!b->release_action(b->context,plan[i].token,plan[i].row))return 0;
            ++o->comparisons_released;
        }
        if(!b->release_action(b->context,plan[0].token,plan[0].row))return 0;
        ++o->comparisons_released;
    }
    for(unsigned i=o->comparisons;i<o->comparisons+o->actions;++i){
        if(!b->release_action(b->context,plan[i].token,plan[i].row))return 0;
        ++o->released;
    }
    if(menu)write_field(a,0x394,0x224227c4u);
    if(b->native_base){
        const auto& base=*b->native_base;
        // Base destructor installs its primary table before any prefix
        // calls. Owner prefix retains the original guard transitions.
        write_field(a,0,0x2240ac2cu);
        if(!base.prefix(base.context,a))return 0;o->base_prefix=1;
        RegistryRemovalResult removal{};
        if(!registry_unlink_logic(a,n,read_field<unsigned>(a,0x378),base.registry,&removal))return 0;
        o->registry_unlinked=removal.unlinked;
        // 221442c3..221442fa: five CBaseEntity output members,
        // reverse member order and forward action-chain order.
        for(unsigned i=o->comparisons+o->actions;i<o->comparisons+o->actions+o->base_actions;++i){
            if(!b->release_action(b->context,plan[i].token,plan[i].row))return 0;
            ++o->base_released;
        }
        if(!base.suffix(base.context,a))return 0;o->base_suffix=1;
        // Final retail CBaseEntity base table at 22144350.
        write_field(a,0,0x2240a088u);
    }else if(!b->base_destroy(b->context,a))return 0;
    o->base_called=1;
    if(flags&1u){if(!b->free_actor(b->context,a))return 0;o->freed=1;}
    o->completed=1;return 1;
}
int __cdecl destroy_logic(void* a,unsigned n,unsigned flags,const LogicDestroyBindings* b,LogicDestroyResult* o) noexcept {
    return destroy_entity_members(a,n,flags,b,o,nullptr);
}
int __cdecl destroy_trigger_members(void* a,unsigned n,unsigned flags,const TriggerDestroyBindings* b,LogicDestroyResult* o) noexcept {
    if(!b){if(o)*o={};return 0;}
    return destroy_entity_members(a,n,flags,b->shared,o,b);
}
bool menu_valid(void* a,unsigned n,const MenuBindings* b,MenuResult* o) noexcept {
    if(!o)return false;*o={};
    return a&&n>=menu_extent&&entity_vtable(a)==menu_vtable&&read_field<unsigned>(a,0x394)==menu_listener_vtable&&b;
}
int __cdecl menu_listen(void* a,unsigned n,bool restore,const MenuBindings* b,MenuResult* o) noexcept {
    if(!menu_valid(a,n,b,o)||!b->subscribe)return 0;
    const auto base=restore?b->base_restore:b->base_spawn;
    if(!base||!base(b->context,a))return 0;o->base_called=1;
    auto* listener=static_cast<unsigned char*>(a)+0x394;
    // 22178e30 Spawn / 22178e80 restore call AddListener in this order.
    if(!b->subscribe(b->context,listener,"gameui_selected",true))return 0;++o->subscribed;
    if(!b->subscribe(b->context,listener,"gameui_highlighted",true))return 0;++o->subscribed;
    o->completed=1;return 1;
}
bool menu_text(const char* value,unsigned bytes) noexcept {
    return value&&bytes&&std::memchr(value,0,bytes<4096?bytes:4096);
}
int __cdecl menu_event(void* a,unsigned n,void* event,const MenuBindings* b,MenuResult* o) noexcept {
    if(!menu_valid(a,n,b,o)||!event||!b->event_name)return 0;
    const char* text=nullptr;unsigned bytes=0;
    auto name=[&](){
        text=nullptr;bytes=0;++o->name_reads;
        return b->event_name(b->context,event,&text,&bytes)&&menu_text(text,bytes);
    };
    if(!name())return 0;
    if(!std::strcmp(text,"gameui_selected"))o->selected=1;
    else {
        // Original calls GetName again before checking highlighted.
        if(!name())return 0;
        if(!std::strcmp(text,"gameui_highlighted"))o->highlighted=1;
        else{o->completed=1;return 1;}
    }
    if(!b->event_string||!b->output)return 0;
    constexpr const char* buttons[]{"resume game","new game","options","load game","save game","friends","stats","quit"};
    constexpr unsigned offsets[]{0x398,0x3b0,0x3c8,0x3e0,0x3f8,0x428,0x410,0x440};
    using namespace sin_port::native_weapon_tables;
    const auto* fire=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
    if(!fire)return 0;
    for(unsigned i=0;i<8;++i){
        // Original invokes GetString(buttonname,"") for every candidate;
        // resume fires then falls through, all other matches return.
        text=nullptr;bytes=0;++o->button_reads;
        if(!b->event_string(b->context,event,"buttonname","",&text,&bytes)||!menu_text(text,bytes))return 0;
        if(std::strcmp(text,buttons[i]))continue;
        const unsigned off=offsets[i]+(o->highlighted?0xc0:0);
        unsigned queued=0;
        if(b->native_queue){
            if(!native_named_output(a,n,a,off,false,b->output,b->native_queue,queued))return 0;
            o->native_output_queued+=queued;
        }else{
            ServerEntityVoidOutputResult r{};
            if(!fire->fire(static_cast<unsigned char*>(a)+off,0x18,a,a,n,0.f,b->output,&r))return 0;
            queued=r.queued;
        }
        if(!o->outputs)o->first_offset=off;
        ++o->outputs;o->last_offset=off;o->queued+=queued;
        if(i)break;
    }
    o->completed=1;return 1;
}
bool navigation_valid(void* a,unsigned n,const NavigationBindings* b,NavigationResult* o) noexcept {
    if(!o)return false;*o={};
    if(!a||n<navigation_extent||entity_vtable(a)!=navigation_vtable||read_field<unsigned>(a,0x394)!=navigation_listener_vtable||!b)return false;
    o->on=read_field<unsigned char>(a,0x398)!=0;return true;
}
bool navigation_target(const NavigationBindings& b,void* entity,unsigned required) noexcept {
    unsigned bytes=0;return entity&&b.extent&&b.extent(b.context,entity,&bytes)&&bytes>=required;
}
bool navigation_update(void* a,const NavigationBindings& b,NavigationResult& o) noexcept {
    if(!b.find)return false;
    void* seen[4096]{};void* prev=nullptr;
    // Original 22178050 rereads target token, on byte and property
    // for every result. Only the Ignore bit changes the target field.
    for(;;){
        void* entity=nullptr;
        if(!b.find(b.context,prev,read_field<unsigned>(a,0x90),&entity))return false;
        if(!entity)return true;
        if(o.visited==4096)return false;
        for(unsigned i=0;i<o.visited;++i)if(seen[i]==entity)return false;
        seen[o.visited++]=entity;prev=entity;
        if(!(read_field<unsigned>(a,0x39c)&1u))continue;
        if(!navigation_target(b,entity,0x238))return false;
        if(read_field<unsigned char>(a,0x398)){
            if(std::numeric_limits<float>::max()>read_field<float>(entity,0x234)){
                write_field(entity,0x234,std::numeric_limits<float>::max());++o.written;
            }
        }else {write_field(entity,0x234,0.f);++o.written;}
    }
}
bool navigation_set(void* a,bool on,const NavigationBindings& b,NavigationResult& o) noexcept {
    if((read_field<unsigned char>(a,0x398)!=0)==on)return true;
    if(!b.listen||!b.find)return false;
    write_field(a,0x398,static_cast<unsigned char>(on));o.changed=1;o.on=on;
    if(!b.listen(b.context,static_cast<unsigned char*>(a)+0x394,on))return false;
    if(on)++o.registered;else ++o.unregistered;
    return navigation_update(a,b,o);
}
int __cdecl navigation_input(void* a,unsigned n,const char* name,const NavigationBindings* b,NavigationResult* o) noexcept {
    if(!navigation_valid(a,n,b,o)||!name)return 0;
    o->handled=1;
    if(equal_class(name,"TurnOn")){if(!navigation_set(a,true,*b,*o))return 0;}
    else if(equal_class(name,"TurnOff")){if(!navigation_set(a,false,*b,*o))return 0;}
    else if(equal_class(name,"Toggle")){if(!navigation_set(a,!o->on,*b,*o))return 0;}
    else o->handled=0;
    o->completed=1;return 1;
}
int __cdecl navigation_activate(void* a,unsigned n,const NavigationBindings* b,NavigationResult* o) noexcept {
    if(!navigation_valid(a,n,b,o)||!b->base_activate||!b->base_activate(b->context,a))return 0;
    o->base_called=1;
    if(read_field<unsigned>(a,0xb4)&1u){
        if(!navigation_set(a,true,*b,*o))return 0;
        const auto flags=read_field<unsigned>(a,0xb4);
        if(flags&1u){
            if(read_field<unsigned char>(a,0x1f0)){write_field(a,0x200,static_cast<unsigned char>(1));o->dirty=1;}
            else if(const auto edict=read_field<unsigned>(a,0x16c)){
                if(!b->mark_edict||!b->mark_edict(b->context,edict))return 0;o->dirty=1;
            }
            write_field(a,0xb4,flags&~1u);
        }
    }else if(read_field<unsigned char>(a,0x398)){
        // Restored enabled entity re-registers, without rescanning.
        if(!b->listen||!b->listen(b->context,static_cast<unsigned char*>(a)+0x394,true))return 0;
        ++o->registered;
    }
    o->on=read_field<unsigned char>(a,0x398)!=0;o->handled=o->completed=1;return 1;
}
int __cdecl navigation_created(void* a,unsigned n,void* entity,const NavigationBindings* b,NavigationResult* o) noexcept {
    if(!navigation_valid(a,n,b,o))return 0;o->handled=1;
    if(o->on&&(read_field<unsigned>(a,0x39c)&1u)){
        if(!navigation_target(*b,entity,0xc4))return 0;
        const auto name=read_field<unsigned>(a,0x90);
        bool match=read_field<unsigned>(entity,0xc0)==name;
        if(!match&&(!b->match||!b->match(b->context,entity,name,&match)))return 0;
        if(match){
            o->matched=1;
            if(!navigation_target(*b,entity,0x238))return 0;
            if(std::numeric_limits<float>::max()>read_field<float>(entity,0x234)){
                write_field(entity,0x234,std::numeric_limits<float>::max());++o->written;
            }
        }
    }
    o->completed=1;return 1;
}
int __cdecl navigation_remove(void* a,unsigned n,const NavigationBindings* b,NavigationResult* o) noexcept {
    if(!navigation_valid(a,n,b,o)||!b->base_remove)return 0;
    if(o->on){
        if(!b->listen||!b->listen(b->context,static_cast<unsigned char*>(a)+0x394,false))return 0;
        ++o->unregistered;
    }
    // Original UpdateOnRemove leaves on byte and existing target deadlines.
    if(!b->base_remove(b->context,a))return 0;
    o->base_called=o->handled=o->completed=1;return 1;
}
// Original 22178dd0: force new level unit first, then acquire
// CDynamicDifficulty singleton and read +6; command is queued, not
// executed synchronously. No SaveDangerous or player health condition.
int __cdecl autosave_input(void* a,unsigned n,const char* name,const AutosaveBindings* b,AutosaveResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!a||n<autosave_extent||entity_vtable(a)!=autosave_vtable||!name||!b)return 0;
    if(!equal_class(name,"Save")){o->completed=1;return 1;}
    o->handled=1;
    if(!b->difficulty_gate)return 0;
    if(read_field<unsigned char>(a,0x394)){
        if(!b->new_level_unit||!b->new_level_unit(b->context))return 0;
        o->new_level_unit=1;
    }
    if(!b->difficulty_gate(b->context,&o->gate))return 0;o->gate_read=1;
    if(o->gate)o->suppressed=1;
    else {
        if(!b->server_command||!b->server_command(b->context,"autosave\n"))return 0;
        o->command_queued=1;
    }
    o->completed=1;return 1;
}
// Inherited CBaseEntity::InputKill 2213cdc0. The virtual attachment
// effects and deferred removal owner are explicit; no immediate free.
template<class BindingsT,class ResultT>
int base_entity_kill(void* a,unsigned n,unsigned extent,unsigned vt,const sin_independent::EntityHandleEntry* entries,unsigned count,const BindingsT* b,ResultT* o,const DeferredRemovalBindings* deferred=nullptr) noexcept {
    if(!o)return 0;*o={};
    if(!a||n<extent||entity_vtable(a)!=vt||!b||(!b->remove&&!deferred)||
       count>sin_independent::kEntityHandleCapacity||(!entries&&count))return 0;
    o->handled=1;
    auto* attached=sin_independent::EntityHandleView(entries,count).Resolve(read_field<unsigned>(a,0x20c));
    if(attached){
        if(!b->attachment_notice||!b->clear_attachment||!b->attachment_notice(b->context,attached,a))return 0;
        o->attachment_notified=1;
        if(!b->clear_attachment(b->context,a))return 0;o->attachment_cleared=1;
    }
    if(deferred){
        DeferredRemovalResult result{};
        if(!defer_logic_remove(a,n,deferred,&result))return 0;
    }else if(!b->remove(b->context,a))return 0;
    o->remove_requested=o->completed=1;return 1;
}
int __cdecl autosave_kill(void* a,unsigned n,const sin_independent::EntityHandleEntry* entries,unsigned count,const AutosaveBindings* b,AutosaveResult* o) noexcept {
    return base_entity_kill(a,n,autosave_extent,autosave_vtable,entries,count,b,o,b?b->deferred:nullptr);
}
int __cdecl multicompare_kill(void* a,unsigned n,const sin_independent::EntityHandleEntry* entries,unsigned count,const EntityKillBindings* b,EntityKillResult* o) noexcept {
    return base_entity_kill(a,n,multicompare_extent,multicompare_vtable,entries,count,b,o,b?b->deferred:nullptr);
}
int __cdecl logic_kill(void* a,unsigned n,unsigned vt,const sin_independent::EntityHandleEntry* entries,unsigned count,const EntityKillBindings* b,EntityKillResult* o) noexcept {
    if(!o)return 0;*o={};
    const auto extent=logic_kill_extent(vt);
    if(!extent)return 0;
    return base_entity_kill(a,n,extent,vt,entries,count,b,o,b?b->deferred:nullptr);
}
int __cdecl trigger_kill(void* a,unsigned n,unsigned vt,const sin_independent::EntityHandleEntry* entries,unsigned count,const EntityKillBindings* b,EntityKillResult* o) noexcept {
    if(!o)return 0;*o={};
    const unsigned extent=vt==dynamic_trigger_vtable?dynamic_trigger_extent:vt==secret_trigger_vtable?secret_trigger_extent:0;
    if(!extent)return 0;
    return base_entity_kill(a,n,extent,vt,entries,count,b,o,b?b->deferred:nullptr);
}
bool deferred_valid(const DeferredRemovalQueue* q) noexcept {
    return q&&q->capacity<=sin_independent::kEntityHandleCapacity&&q->count<=q->capacity&&
        q->reserved<=q->capacity-q->count&&(!q->capacity||q->slots);
}
bool listener_storage(const DeletionListenerBindings* b) noexcept {
    return b&&b->count&&b->capacity<=4096&&*b->count<=b->capacity&&
        (!b->capacity||b->slots)&&b->entry_count<=sin_independent::kEntityHandleCapacity&&(!b->entry_count||b->entries);
}
// 22162d10: resolve the original handle before reading listener count,
// then read each live pointer slot in reverse. All known logic primary
// +14 return self (22001ae0). Nonlogic types remain outside this leaf.
int __cdecl notify_deleting(unsigned handle,const DeletionListenerBindings* b,DeletionListenerResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!listener_storage(b))return 0;
    auto* a=sin_independent::EntityHandleView(b->entries,b->entry_count).Resolve(handle);
    if(!a){o->stale=o->completed=1;return 1;}
    const unsigned count=*b->count;
    if(count&&!b->deleted)return 0;
    for(unsigned i=count;i>0;){
        --i;
        // The original takes the starting count and reads live slots,
        // not a copied list. The owner guarantees those slots remain valid.
        void* listener=b->slots[i];if(!listener||!b->deleted(b->context,listener,a))return 0;
        ++o->notified;
    }
    o->completed=1;return 1;
}
// 2214471f..22144787. With EFL_KILLME already set, 221639f0
// returns immediately; all supported logic +268 slots are RET.
bool removal_metadata(void* a,const RemovalMetadataBindings& b,DeferredRemovalResult& o) noexcept {
    if(read_field<unsigned>(a,0x16c)){
        const unsigned prior=read_field<unsigned>(a,0xbc);
        write_field(a,0xbc,prior|0x04000000u);o.edict_flag_added=(prior&0x04000000u)==0;
    }
    const unsigned global=read_field<unsigned>(a,0x10);
    if(global){
        const char* text=nullptr;unsigned bytes=0;
        if(!b.string_text||!b.string_text(b.context,global,&text,&bytes)||!text||!bytes||
            !std::memchr(text,0,bytes<4096?bytes:4096))return false;
        using namespace sin_port::fixed_service;
        const auto* api=SiN_GetGlobalStateApi(global_state_version,sizeof(GlobalStateApi));
        if(!api)return false;
        const int index=api->find(text);o.global_checked=1;
        // Original setter is void and ignores missing or gate-blocked
        // entries. The independent service reports refusal instead.
        if(index>=0&&api->set_state(index,2))o.global_dead=1;else o.global_noop=1;
    }
    if(read_field<unsigned>(a,0x218)){
        if(!b.cleanup_224||!b.cleanup_224(b.context,a))return false;
        o.cleanup_224=1;
    }
    // Read after +224, as the original does. Clear bit 0 at +78,
    // preserving remaining flags and original dirty-mark routing.
    const unsigned flags=read_field<unsigned>(a,0x78);
    if(flags&1u){
        if(read_field<unsigned char>(a,0x1f0)){
            write_field(a,0x200,static_cast<unsigned char>(1));o.dirty_marked=1;
        }else if(const unsigned token=read_field<unsigned>(a,0x16c)){
            void* edict=nullptr;unsigned bytes=0;
            if(!b.edict||!b.edict(b.context,token,&edict,&bytes)||!edict||bytes<4)return false;
            write_field(edict,0,read_field<unsigned>(edict,0)|1u);o.dirty_marked=1;
        }
        write_field(a,0x78,flags&~1u);o.flags_cleared=1;
    }
    o.metadata=1;return true;
}
bool hierarchy_storage(const RemovalHierarchyBindings* b) noexcept {
    return b&&b->before_collect&&b->extent&&b->count&&b->capacity<=4096&&
        *b->count<=b->capacity&&(!b->capacity||b->handles);
}
// 22174690 -> 221745b0. Collect descendant pointers in preorder;
// 221447e6 consumes that snapshot in reverse. No recursive deletion
// while collecting. Bound and reject cycles/aliasing independently.
bool removal_hierarchy(void* a,const RemovalHierarchyBindings& b,const DeletionListenerBindings& l,
                       const DeferredRemovalBindings& deferred,DeferredRemovalResult& o) noexcept {
    if(!hierarchy_storage(&b)||!b.before_collect(b.context,a))return false;
    const sin_independent::EntityHandleView registry(l.entries,l.entry_count);
    constexpr unsigned max_children=256;
    struct Child {void* actor;unsigned bytes;};
    Child children[max_children]{};
    auto collect=[&](auto&& self,unsigned handle)->bool{
        for(auto* child=registry.Resolve(handle);child;child=registry.Resolve(handle)){
            if(child==a||o.children_collected==max_children)return false;
            for(unsigned i=0;i<o.children_collected;++i)if(children[i].actor==child)return false;
            unsigned bytes=0;
            if(!b.extent(b.context,child,&bytes)||bytes<0xfc)return false;
            children[o.children_collected++]={child,bytes};
            if(!self(self,read_field<unsigned>(child,0xf4)))return false;
            handle=read_field<unsigned>(child,0xf8);
        }
        return true;
    };
    if(!collect(collect,read_field<unsigned>(a,0xf4)))return false;
    // Preflight owner dispatch availability for the snapshot.
    for(unsigned i=0;i<o.children_collected;++i){
        const auto extent=removal_entity_extent(entity_vtable(children[i].actor));
        if(extent?children[i].bytes<extent:!b.foreign_remove)return false;
    }
    if(o.children_collected&&(!b.warning||!b.warning(b.context,a,o.children_collected)))return false;
    for(unsigned i=o.children_collected;i>0;--i){
        const auto& child=children[i-1];
        if(removal_entity_extent(entity_vtable(child.actor))){
            DeferredRemovalResult r{};
            if(!defer_logic_remove(child.actor,child.bytes,&deferred,&r))return false;
            ++o.children_native;
        }else{
            if(!b.foreign_remove(b.context,child.actor))return false;
            ++o.children_foreign;
        }
        ++o.children_requested;
    }
    // 220a21f0 compares RESOLVED pointers (including null), not raw
    // serial words. Remove only first equal row and shift its suffix.
    if(!hierarchy_storage(&b))return false;
    const auto* target=registry.Resolve(read_field<unsigned>(a,0x378));
    for(unsigned i=0;i<*b.count;++i)if(registry.Resolve(b.handles[i])==target){
        std::memmove(b.handles+i,b.handles+i+1,(*b.count-i-1)*sizeof(unsigned));
        --*b.count;o.handle_vector_removed=1;o.handle_vector_index=i;break;
    }
    o.hierarchy=1;return true;
}
bool removal_update_native(void* a,unsigned n,unsigned vt,const NativeRemovalUpdateBindings& b,DeferredRemovalQueue& q,DeferredRemovalResult& o,const DeferredRemovalBindings& deferred) noexcept {
    if(!listener_storage(b.listeners)||!b.tail)return false;
    const auto& l=*b.listeners;
    // CLogicNavigation::UpdateOnRemove unregisters its listener first,
    // preserving the on byte. Remove first matching pointer (22163160).
    if(vt==navigation_vtable){
        if(n<navigation_extent||read_field<unsigned>(a,0x394)!=navigation_listener_vtable)return false;
        if(read_field<unsigned char>(a,0x398)){
            void* listener=static_cast<unsigned char*>(a)+0x394;
            for(unsigned i=0;i<*l.count;++i)if(l.slots[i]==listener){
                std::memmove(l.slots+i,l.slots+i+1,(*l.count-i-1)*sizeof(void*));
                --*l.count;o.unregistered=1;break;
            }
        }
    }
    q.chained_update=true; // base +180 is a RET for all 13 known classes
    DeletionListenerResult notified{};
    const bool ok=notify_deleting(read_field<unsigned>(a,0x378),&l,&notified)!=0;
    o.notified=notified.notified;o.stale_notification=notified.stale;
    if(!ok||(b.metadata&&!removal_metadata(a,*b.metadata,o))||
        (b.hierarchy&&!removal_hierarchy(a,*b.hierarchy,l,deferred,o))||!b.tail(b.context,a))return false;
    o.native_update=1;return true;
}
// 22286bf0 -> 22286b90 -> 221636d0. The embedded networkable's
// authenticated flags and base-entity getters use its +8 backpointer.
int scripted_sequence_cancel(void*,unsigned,const ScriptedSequenceCancelBindings*,ScriptedSequenceCancelResult*) noexcept;
int defer_remove_impl(void* a,unsigned n,const DeferredRemovalBindings* b,DeferredRemovalResult* o,
    const ScriptedSequenceCancelBindings* sequence=nullptr,ScriptedSequenceCancelResult* cancelled=nullptr) noexcept {
    if(cancelled)*cancelled={};
    if(!o)return 0;*o={};
    if(!a){o->completed=1;return 1;}
    if(n<4)return 0;
    const auto vt=entity_vtable(a);
    const bool is_sequence=vt==scripted_sequence_vtable;
    if(is_sequence&&(!sequence||!cancelled||!sequence->control||!sequence->control->extent))return 0;
    if(sequence&&!is_sequence)return 0;
    const auto extent=is_sequence?scripted_sequence_extent:removal_entity_extent(vt);
    if(!extent||n<extent||read_field<unsigned>(a,0x160)!=0x22454f00u||
       read_field<unsigned>(a,0x168)!=static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(a)))return 0;
    const auto flags=read_field<unsigned>(a,0xb8);
    if(flags&1u){o->already_marked=o->completed=1;return 1;}
    if(!b||!deferred_valid(b->queue)||(!b->update&&!b->native_update))return 0;
    if(b->native_update&&(!listener_storage(b->native_update->listeners)||!b->native_update->tail||
       (b->native_update->hierarchy&&(!b->native_update->metadata||!hierarchy_storage(b->native_update->hierarchy)))))return 0;
    auto& q=*b->queue;
    // Independent capacity reservation: a reentrant update can append
    // other entities without consuming this request's slot. The original
    // CUtlVector grows dynamically; bounded storage refuses before mark.
    if(q.count+q.reserved==q.capacity)return 0;
    ++q.reserved;
    write_field(a,0xb8,flags|1u);o->marked=1;q.chained_update=false;
    // Retail +17c 221af950 calls 221af840 before tail-jumping to
    // CBaseEntity::UpdateOnRemove. The owner sees only the base leaf.
    bool updated=!is_sequence||scripted_sequence_cancel(a,n,sequence,cancelled)!=0;
    if(updated)updated=b->native_update?removal_update_native(a,n,vt,*b->native_update,q,*o,*b):
        b->update(b->context,a,vt==navigation_vtable?0x22177f50u:0x221446f0u)!=0;
    --q.reserved;
    if(!updated)return 0; // original virtual is void; do not roll back side effects
    o->updated=1;write_field(a,0xc0,0u);o->name_cleared=1;
    // 221636d0 checks the CURRENT handle against -1, not serial lookup.
    if(read_field<unsigned>(a,0x378)==0xffffffffu){o->invalid_handle=o->completed=1;return 1;}
    if(!deferred_valid(&q)||q.count+q.reserved>=q.capacity)return 0;
    q.slots[q.count++]=static_cast<unsigned char*>(a)+0x160;
    o->queued=o->completed=1;o->remaining=q.count;return 1;
}
int __cdecl defer_logic_remove(void* a,unsigned n,const DeferredRemovalBindings* b,DeferredRemovalResult* o) noexcept {
    return defer_remove_impl(a,n,b,o);
}
// 22162aa0: pre-cleanup before handle guard, forward traversal of the
// LIVE vector count. Release can append new rows during the same pass.
int __cdecl cleanup_deferred(const DeferredRemovalBindings* b,DeferredRemovalResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!b||!deferred_valid(b->queue)||!b->release)return 0;
    auto& q=*b->queue;
    if(q.cleanup_active||q.reserved||q.handle_access_disabled)return 0;
    if(b->scope&&!b->scope(b->context,true))return 0;
    o->entered=1;q.cleanup_active=true;
    bool ok=true;
    if(b->pre_cleanup){ok=b->pre_cleanup(b->context)!=0;o->pre_called=1;}
    unsigned cursor=0;
    if(ok){
        q.handle_access_disabled=true;
        while(cursor<q.count){
            if(!deferred_valid(&q)||!q.slots[cursor]||!b->release(b->context,q.slots[cursor])){ok=false;break;}
            ++cursor;++o->released; // never touch a released object again
        }
        q.handle_access_disabled=false;
    }
    // Normal retail completion only sets count=0. On an independent
    // owner failure retain the unconsumed suffix, not freed pointers.
    if(cursor<=q.count&&deferred_valid(&q)){
        if(cursor&&cursor<q.count)std::memmove(q.slots,q.slots+cursor,(q.count-cursor)*sizeof(void*));
        q.count-=cursor;
    }else ok=false;
    q.cleanup_active=false;o->remaining=q.count;
    const bool closed=!b->scope||b->scope(b->context,false)!=0;o->closed=closed;
    o->completed=ok&&closed;return int(o->completed);
}
// 22162b30 drops the logical vector size and returns the old count.
// This is NOT a destructor, and the storage itself remains owner-owned.
int __cdecl discard_deferred(DeferredRemovalQueue* q,DeferredRemovalResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!deferred_valid(q)||q->cleanup_active||q->reserved||q->handle_access_disabled)return 0;
    o->discarded=q->count;q->count=0;o->completed=1;return 1;
}
// Validate both owner chains without dereferencing foreign link pointers.
// Detached retail rows link to themselves. A bounded independent view
// need not contain the original full 4096 slots.
bool registry_chain(const RemovalRegistry& r,const RegistryChain& chain,bool free_chain,bool (&seen)[sin_independent::kEntityHandleCapacity]) noexcept {
    auto index=[&](const sin_independent::EntityHandleEntry* p,unsigned& i){
        const auto first=reinterpret_cast<std::uintptr_t>(r.entries),at=reinterpret_cast<std::uintptr_t>(p);
        if(at<first)return false;const auto delta=at-first;
        if(delta%sizeof(*p)||delta/sizeof(*p)>=r.count)return false;
        i=static_cast<unsigned>(delta/sizeof(*p));return true;
    };
    const sin_independent::EntityHandleEntry* prev=nullptr;
    auto* row=chain.head;
    while(row){
        unsigned i=0;if(!index(row,i)||seen[i]||row->previous!=prev)return false;
        if(free_chain?(i<2048||row->entity!=nullptr):row->entity==nullptr)return false;
        seen[i]=true;prev=row;row=row->next;
    }
    return prev==chain.tail;
}
void registry_detach(sin_independent::EntityHandleEntry& e,RegistryChain& chain) noexcept {
    if(e.previous==&e)return;
    if(e.previous)e.previous->next=e.next;else chain.head=e.next;
    if(e.next)e.next->previous=e.previous;else chain.tail=e.previous;
    e.previous=e.next=&e;
}
void registry_append(sin_independent::EntityHandleEntry& e,RegistryChain& chain) noexcept {
    e.previous=chain.tail;e.next=nullptr;
    if(chain.tail)chain.tail->next=&e;else chain.head=&e;
    chain.tail=&e;
}
// 22164360 / global list callback 22162280. Deliberately separate from
// UpdateOnRemove listener notification (22162d10); this occurs later.
int __cdecl registry_unlink_logic(void* a,unsigned n,unsigned handle,const RegistryRemovalBindings* b,RegistryRemovalResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!a||n<0x37c||!b||!b->registry||handle==0xffffffffu)return 0;
    auto& r=*b->registry;const unsigned vt=entity_vtable(a),extent=removal_entity_extent(vt);
    // Base destructor installs its own table before calling 22164520.
    if((!extent&&vt!=0x2240ac2cu)||(extent&&n<extent)||!r.entries||r.count>sin_independent::kEntityHandleCapacity||r.unlink_active)return 0;
    const unsigned i=handle&0xfffu;o->index=i;
    if(i>=r.count)return 0;
    auto& e=r.entries[i];
    if(!e.entity){o->empty=o->completed=1;return 1;}
    // Retail wrapper masks the index only. Independent owner refuses a
    // stale handle instead of accidentally unlinking its replacement.
    if(e.entity!=a||e.serial!=(handle>>12)||read_field<unsigned>(a,0x378)!=handle)return 0;
    bool seen[sin_independent::kEntityHandleCapacity]{};
    if(!registry_chain(r,r.active,false,seen)||!seen[i]||!registry_chain(r,r.free_nonnetworked,true,seen)||!r.entity_count)return 0;
    const unsigned token=read_field<unsigned>(a,0x16c);
    void* edict=nullptr;unsigned bytes=0;
    if(token&&(!r.edict_count||!b->edict||!b->edict(b->context,token,&edict,&bytes)||!edict||bytes<8))return 0;
    r.unlink_active=true;
    o->old_serial=e.serial;o->old_handle=handle;
    // SetRefEHandle 2213dfc0 writes the invalid handle first, and
    // mirrors its low ten serial bits to the edict at +4.
    write_field(a,0x378,0xffffffffu);o->handle_cleared=1;
    if(edict){write_field(edict,4,0x3ffu);o->edict_cleared=1;--r.edict_count;}
    --r.entity_count;o->counts_changed=1;
    e.entity=nullptr;e.serial=(e.serial+1u)&0x7fffu;o->new_serial=e.serial;
    registry_detach(e,r.active);o->unlinked=1;
    if(i>=2048){registry_append(e,r.free_nonnetworked);o->returned_to_free=1;}
    r.unlink_active=false;o->completed=1;return 1;
}
// Original variant_t::Convert(5), 22155e60. FIELD_INTEGER,
 // FLOAT (x87 truncating conversion), STRING (atoi); other types reject.
bool multi_integer(unsigned (&value)[5],const MultiCompareBindings& b,bool& accepted) noexcept {
    accepted=true;
    if(value[4]==5)return true;
    if(value[4]==1){
        float scalar=0;std::memcpy(&scalar,value,4);
        const double truncated=std::trunc(double(scalar));
        const int i=(!std::isfinite(truncated)||truncated<double(INT_MIN)||truncated>double(INT_MAX))?INT_MIN:static_cast<int>(truncated);
        std::memcpy(value,&i,4);value[4]=5;return true;
    }
    if(value[4]==2){
        const char* text="";unsigned bytes=1;
        if(value[0]&&(!b.resolve_text||!b.resolve_text(b.context,value[0],&text,&bytes)||!text||!bytes||!std::memchr(text,0,bytes<4096?bytes:4096)))return false;
        while(*text==' '||(*text>='\t'&&*text<='\r'))++text;
        const bool negative=*text=='-';if(negative||*text=='+')++text;
        unsigned raw=0;while(*text>='0'&&*text<='9'){raw=raw*10u+unsigned(*text-'0');++text;}
        value[0]=negative?0u-raw:raw;value[4]=5;return true;
    }
    accepted=false;return true;
}
bool multi_row(const MultiCompareBindings& b,unsigned token,void** row) noexcept {
    unsigned bytes=0;return b.resolve&&b.resolve(b.context,token,row,&bytes)&&*row&&bytes>=0x1c;
}
bool schedule_multicompare(void* a,void* activator,unsigned id,const RelayQueueBindings& b) noexcept {
    if(!valid_relay_queue(b)||!b.now||!b.handle||!b.intern)return false;
    const sin_independent::EntityHandleView registry(b.entries,b.entry_count);
    unsigned self=0xffffffffu,source=0xffffffffu,input=0;
    if(!b.handle(b.context,a,&self)||registry.Resolve(self)!=a)return false;
    if(activator&&(!b.handle(b.context,activator,&source)||registry.Resolve(source)!=activator))return false;
    if(!b.intern(b.context,"ForceRecompare",&input)||!input)return false;
    const float now=*b.now;if(!std::isfinite(now))return false;
    using namespace sin_port::native_weapon_tables;
    const auto* api=SiN_GetServerEntityNamedEventQueueApi(1,sizeof(ServerEntityNamedEventQueueApi));
    unsigned variant[5]{0,0,0,0xffffffffu,0};
    ServerEntityNamedEventQueueResult q{};
    return api&&api->enqueue_direct(b.root,b.root_bytes,b.root_token,now,self,input,source,self,variant,sizeof(variant),0.f,id,b.queue,&q);
}
// Route original COutputEvent (22156900 -> 22156680) to the live
// named event queue without an owner enqueue callback. The owner still
// resolves action records, handles, and supplies release/debug effects.
bool native_named_output(void* a,unsigned n,void* activator,unsigned off,bool typed,const native_weapon_tables::ServerEntityVoidOutputBindings* owner,const RelayQueueBindings* q,unsigned& queued) noexcept {
    using namespace sin_port::native_weapon_tables;
    const auto* fire=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
    if(!fire||!owner)return false;
    auto* out=static_cast<unsigned char*>(a)+off;
    ServerEntityVoidOutputResult r{};
    if(!q||!valid_relay_queue(*q)||!q->now||!q->handle||!std::isfinite(*q->now))return false;
    const sin_independent::EntityHandleView registry(q->entries,q->entry_count);
    struct Route {
        const ServerEntityVoidOutputBindings* owner;
        const RelayQueueBindings* queue;
        void* actor;
        void* activator;
        unsigned self=0xffffffffu,source=0xffffffffu;
    } ctx{owner,q,a,activator};
    if(!q->handle(q->context,a,&ctx.self)||registry.Resolve(ctx.self)!=a)return false;
    if(activator&&(!q->handle(q->context,activator,&ctx.source)||registry.Resolve(ctx.source)!=activator))return false;
    ServerEntityVoidOutputBindings bound{};
    bound.context=&ctx;
    bound.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
        const auto& s=*static_cast<Route*>(p);return s.owner->resolve&&s.owner->resolve(s.owner->context,token,row,bytes);
    };
    bound.release=[](void* p,unsigned token,void* row) noexcept ->int{
        const auto& s=*static_cast<Route*>(p);return s.owner->release&&s.owner->release(s.owner->context,token,row);
    };
    bound.debug_action=[](void* p,void* caller,const ServerEntityVoidOutputAction* action) noexcept ->int{
        const auto& s=*static_cast<Route*>(p);return s.owner->debug_action&&s.owner->debug_action(s.owner->context,caller,action);
    };
    bound.enqueue=[](void* p,void* activator,void* caller,const ServerEntityVoidOutputAction* action) noexcept ->int{
        const auto& s=*static_cast<Route*>(p);const auto& q=*s.queue;
        if(activator!=s.activator||caller!=s.actor)return 0;
        const auto* api=SiN_GetServerEntityNamedEventQueueApi(1,sizeof(ServerEntityNamedEventQueueApi));
        ServerEntityNamedEventQueueResult r{};
        return api&&api->enqueue(q.root,q.root_bytes,q.root_token,*q.now,s.source,s.self,action,q.queue,&r);
    };
    bound.enqueue_variant=[](void* p,void* activator,void* caller,const ServerEntityVoidOutputAction* action,const void* variant,unsigned bytes) noexcept ->int{
        const auto& s=*static_cast<Route*>(p);const auto& q=*s.queue;
        if(activator!=s.activator||caller!=s.actor)return 0;
        const auto* api=SiN_GetServerEntityNamedEventQueueApi(1,sizeof(ServerEntityNamedEventQueueApi));
        ServerEntityNamedEventQueueResult r{};
        return api&&api->enqueue_variant(q.root,q.root_bytes,q.root_token,*q.now,s.source,s.self,action,variant,bytes,q.queue,&r);
    };
    if(typed){
        if(!fire->fire_variant(out,0x18,activator,a,n,0.f,out,0x14,&bound,&r))return false;
    }else if(!fire->fire(out,0x18,activator,a,n,0.f,&bound,&r))return false;
    queued=r.queued;return true;
}
bool multicompare_fire(void* a,unsigned n,void* activator,unsigned off,const MultiCompareBindings& b,MultiCompareResult& o) noexcept {
    if(b.native_outputs){
        if(!native_named_output(a,n,activator,off,false,b.output,b.native_queue,o.queued))return false;
        o.native_output_queued=o.queued;return true;
    }
    using namespace sin_port::native_weapon_tables;
    const auto* fire=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
    ServerEntityVoidOutputResult r{};
    if(!fire||!b.output||!fire->fire(static_cast<unsigned char*>(a)+off,0x18,activator,a,n,0.f,b.output,&r))return false;
    o.queued=r.queued;return true;
}
int __cdecl multicompare_input(void* a,unsigned n,const char* name,void* activator,const void* variant,unsigned vn,unsigned id,const MultiCompareBindings* b,MultiCompareResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!a||n<multicompare_extent||entity_vtable(a)!=multicompare_vtable||!name||!b)return 0;
    o->handled=1;
    const bool input=equal_class(name,"InputValue"),compare=equal_class(name,"CompareValues");
    if(!input&&!compare){o->handled=0;o->completed=1;return 1;}
    unsigned payload[5]{};
    if(input){
        if(!variant||vn<0x14)return 0;
        std::memcpy(payload,variant,0x14);bool accepted=false;
        if(!multi_integer(payload,*b,accepted))return 0;
        if(!accepted){o->rejected=o->completed=1;return 1;}
        o->converted=1;
    }
    // Preflight owner list with bounded token and address uniqueness.
    unsigned tokens[256]{};void* rows[256]{};
    unsigned token=read_field<unsigned>(a,0x3cc);
    while(token){
        if(o->visited==256)return 0;
        void* row=nullptr;if(!multi_row(*b,token,&row))return 0;
        const auto start=reinterpret_cast<std::uintptr_t>(row),actor=reinterpret_cast<std::uintptr_t>(a);
        if(start>std::uintptr_t(-1)-0x1c||actor>std::uintptr_t(-1)-n||(start<actor+n&&actor<start+0x1c))return 0;
        for(unsigned i=0;i<o->visited;++i){
            const auto prior=reinterpret_cast<std::uintptr_t>(rows[i]);
            if(tokens[i]==token||(start<prior+0x1c&&prior<start+0x1c))return 0;
        }
        tokens[o->visited]=token;rows[o->visited++]=row;
        token=read_field<unsigned>(row,0x18);
    }
    if(input){
        void* found=nullptr;
        for(unsigned i=0;i<o->visited;++i)if(read_field<unsigned>(rows[i],0x14)==id){found=rows[i];break;}
        if(found){std::memcpy(found,payload,0x14);o->replaced=1;}
        else {
            if(o->visited==256||!b->allocate)return 0;
            void* row=nullptr;unsigned bytes=0,new_token=0;
            if(!b->allocate(b->context,&new_token,&row,&bytes)||!new_token||!row||bytes<0x1c)return 0;
            const auto start=reinterpret_cast<std::uintptr_t>(row),actor=reinterpret_cast<std::uintptr_t>(a);
            if(start>std::uintptr_t(-1)-0x1c||(start<actor+n&&actor<start+0x1c))return 0;
            for(unsigned i=0;i<o->visited;++i){
                const auto prior=reinterpret_cast<std::uintptr_t>(rows[i]);
                if(tokens[i]==new_token||(start<prior+0x1c&&prior<start+0x1c))return 0;
            }
            std::memcpy(row,payload,0x14);write_field(row,0x14,id);write_field(row,0x18,read_field<unsigned>(a,0x3cc));
            write_field(a,0x3cc,new_token);o->inserted=1;
        }
        if(!read_field<unsigned>(a,0x3d0)){
            if(b->native_queue){if(!schedule_multicompare(a,activator,id,*b->native_queue))return 0;o->native_scheduled=1;}
            else if(!b->schedule||!b->schedule(b->context,a,activator,a,"ForceRecompare",0.f,id))return 0;
            write_field(a,0x3d0,1u);o->scheduled=1;
        }
    }else{
        write_field(a,0x3d0,0u);
        auto integer=[](void* row){return read_field<unsigned>(row,0x10)==5?read_field<int>(row,0):0;};
        int reference=read_field<int>(a,0x3c4);
        if(!read_field<unsigned>(a,0x3c8)&&o->visited)reference=integer(rows[0]);
        bool equal=true;for(unsigned i=0;i<o->visited;++i)if(integer(rows[i])!=reference){equal=false;break;}
        o->compared=1;o->equal=equal;o->output_offset=equal?0x394:0x3ac;
        if(!multicompare_fire(a,n,activator,o->output_offset,*b,*o))return 0;
    }
    o->completed=1;return 1;
}
// 221ce3e0 resolves the first named entity, casts it to CBaseFilter,
// then calls the base Activate. 2216b3e0 calls base first and resolves
// all five child names without a cast. Empty names preserve handles.
int campaign_filter_activate(void* a,unsigned n,const CampaignFilterActivationBindings* b,CampaignFilterActivationResult* o) noexcept {
    if(!o)return 0;*o={};
    unsigned vt=0;if(!b||!b->base_activate||n<4||!SinReadCampaignTouchMarkerInternal(a,&vt))return 0;
    const bool trigger=vt==dynamic_trigger_vtable||vt==secret_trigger_vtable;
    const bool multi=vt==multi_filter_vtable;
    const unsigned extent=vt==dynamic_trigger_vtable?dynamic_trigger_extent:vt==secret_trigger_vtable?secret_trigger_extent:
        multi?0x3f4u:campaign_filter_class(vt)?0x395u:0;
    if(!extent||n<extent||!SinCampaignStorageReadableInternal(a,extent))return 0;
    o->handled=1;
    static thread_local bool active=false;if(active)return 0;
    struct Guard{bool& flag;~Guard(){flag=false;}} guard{active};active=true;
    auto base=[&](){o->base_called=1;return b->base_activate(b->context,a)!=0;};
    auto bind=[&](unsigned name_offset,unsigned handle_offset,bool cast){
        const unsigned name=read_field<unsigned>(a,name_offset);
        if(!name){++o->preserved;return true;}
        const auto* search=b->search;
        using namespace sin_port::native_weapon_tables;
        const auto* api=SiN_GetServerEntitySearchApi(1,sizeof(ServerEntitySearchApi));
        ServerEntitySearchResult found{};
        ++o->lookups;
        if(!search||!api||!api->named(nullptr,name,nullptr,nullptr,search,&found))return false;
        void* entity=found.entity;
        unsigned handle=0xffffffffu;
        if(!entity)++o->missing;
        else{
            unsigned marker=0,bytes=0;
            if(!search->extent||!search->extent(search->context,entity,&bytes)||bytes<4||
               !SinReadCampaignTouchMarkerInternal(entity,&marker))return false;
            if(cast&&!campaign_filter_class(marker)){
                void* subobject=nullptr;
                if(campaign_player_marker(marker)==PlayerPredicate::Unknown){
                    if(!b->foreign_cast||!b->foreign_cast(b->context,entity,&subobject))return false;
                }
                entity=subobject;
                if(!entity)++o->wrong_class;
                else if(!search->extent(search->context,entity,&bytes)||bytes<4||
                    !SinReadCampaignTouchMarkerInternal(entity,&marker))return false;
            }
            if(entity){
                if(campaign_filter_class(marker)||campaign_player_marker(marker)!=PlayerPredicate::Unknown){
                    if(!SinReadCampaignHandleInternal(entity,bytes,&handle))return false;
                }else if(!search->foreign_handle||!search->foreign_handle(search->context,entity,&handle))return false;
                // Independent storage must agree with the live registry.
                if(sin_independent::EntityHandleView(search->entities.entries,search->entities.count).Resolve(handle)!=entity)return false;
                ++o->resolved;
            }
        }
        write_field(a,handle_offset,handle);++o->written;return true;
    };
    if(trigger){
        if(!bind(0x40c,0x410,true)||!base())return 0;
    }else{
        if(!base())return 0;
        if(multi)for(unsigned i=0;i<5;++i)if(!bind(0x3cc+i*4,0x3e0+i*4,false))return 0;
    }
    o->completed=1;return 1;
}
// CBaseFilter wrapper 2216b2a0 and RTTI-backed +28c leaves.
// Recursion guards are independent safeguards against malformed/cyclic
// owner records; retail assumes valid acyclic map filters.
int campaign_filter_entity(void* filter,void* other,const CampaignCustomFilterBindings* b,CampaignCustomFilterResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!b||!b->extent||b->entry_count>sin_independent::kEntityHandleCapacity||
       (!b->entries&&b->entry_count))return 0;
    static thread_local bool active=false;if(active)return 0;
    struct Guard{bool& flag;~Guard(){flag=false;}} guard{active};active=true;
    const sin_independent::EntityHandleView registry(b->entries,b->entry_count);
    unsigned other_bytes=0,other_vt=0;
    if(other&&(!b->extent(b->context,other,&other_bytes)||other_bytes<4||!SinReadCampaignTouchMarkerInternal(other,&other_vt)))return 0;
    auto read_other=[&](unsigned need){return other&&other_bytes>=need&&SinCampaignStorageReadableInternal(other,need);};
    auto text=[&](unsigned token,const char*& s){
        s="";
        if(!token)return true;
        unsigned bytes=0;
        return b->string_text&&b->string_text(b->context,token,&s,&bytes)&&s&&bytes&&bytes<=4096&&
            SinCampaignStorageReadableInternal(s,bytes)&&std::memchr(s,0,bytes);
    };
    auto match=[](const char* value,const char* pattern){
        auto fold=[](unsigned char ch){return ch>='A'&&ch<='Z'?ch+32:ch;};
        while(*value&&*pattern&&fold(*value)==fold(*pattern)){++value;++pattern;}
        return (!*value&&!*pattern)||*pattern=='*';
    };
    auto is_player=[&](bool& yes){
        ++o->player_reads;const auto p=campaign_player_marker(other_vt);
        if(p==PlayerPredicate::Unknown)return b->foreign_player&&b->foreign_player(b->context,other,&yes);
        yes=p==PlayerPredicate::Player;return true;
    };
    auto vehicle=[&](bool& yes){
        ++o->vehicle_reads;
        if(other_vt==0x224aec3cu||other_vt==0x2245222cu){
            if(!read_other(0x9b0))return false;
            yes=registry.Resolve(read_field<unsigned>(other,0x9ac))!=nullptr;return true;
        }
        return b->foreign_vehicle&&b->foreign_vehicle(b->context,other,&yes);
    };
    void* ancestors[32]{};
    auto evaluate=[&](auto&& self,void* node,unsigned depth,bool& accepted)->bool{
        if(!node||depth==32||o->visited==512)return false;
        for(unsigned i=0;i<depth;++i)if(ancestors[i]==node)return false;
        ancestors[depth]=node;++o->visited;
        unsigned bytes=0,vt=0;
        if(!b->extent(b->context,node,&bytes)||bytes<4||!SinReadCampaignTouchMarkerInternal(node,&vt))return false;
        const unsigned need=vt==base_filter_vtable?0x395u:
            vt==name_filter_vtable||vt==class_filter_vtable?0x3ccu:vt==multi_filter_vtable?0x3f4u:0;
        if(!need){
            ++o->foreign_filters;
            return b->foreign_filter&&b->foreign_filter(b->context,node,other,&accepted);
        }
        if(bytes<need||!SinCampaignStorageReadableInternal(node,need))return false;
        ++o->native_filters;
        bool raw=false;
        if(vt==base_filter_vtable)raw=true;
        else if(vt==multi_filter_vtable){
            const bool any=read_field<unsigned>(node,0x3c8)!=0;
            raw=!any;
            for(unsigned i=0;i<5;++i){
                auto* child=registry.Resolve(read_field<unsigned>(node,0x3e0+4*i));
                if(!child){++o->stale;continue;}
                bool pass=false;if(!self(self,child,depth+1,pass))return false;
                if(pass==any){raw=any;++o->short_circuits;break;}
            }
        }else{
            // The original name/class predicates dereference activator;
            // refuse null here, but base and empty multi are null-safe.
            if(!other)return false;
            const unsigned token=read_field<unsigned>(node,0x3c8);
            const char* pattern=nullptr;
            // Name filter checks !player before pointer/token equality.
            if(vt==name_filter_vtable){
                if(!text(token,pattern))return false;
                if(equal_class(pattern,"!player")){
                    bool player=false;if(!is_player(player))return false;
                    if(player){
                        // Original makes a second IsPlayer virtual call
                        // before reading the vehicle handle.
                        if(!is_player(player)||!player)return false;
                        bool present=false;if(!vehicle(present))return false;
                        raw=!present;
                    }
                }else{
                    if(!read_other(0xc4))return false;
                    const unsigned own=read_field<unsigned>(other,0xc0);
                    if(own&&own==token)raw=true;
                    else{const char* value=nullptr;if(!text(own,value))return false;raw=match(value,pattern);}
                }
            }else{
                if(!read_other(0x10))return false;
                const unsigned own=read_field<unsigned>(other,0xc);
                if(own&&own==token)raw=true;
                else{
                    const char* value=nullptr;if(!text(token,pattern)||!text(own,value))return false;
                    raw=match(value,pattern);
                }
            }
        }
        // Negation read AFTER child/raw predicate, as original.
        if(read_field<unsigned char>(node,0x394)){raw=!raw;++o->negated;}
        accepted=raw;return true;
    };
    bool accepted=false;
    if(!evaluate(evaluate,filter,0,accepted))return 0;
    o->accepted=accepted;o->completed=1;return 1;
}
// Authenticated primary +f4 = 22044210 (true), +574 = 2203e690.
bool scripted_npc_marker(unsigned vt) noexcept {
    switch(vt){
    case 0x2248df04u:case 0x224954a4u:case 0x2249aaa4u:case 0x224a416cu:
    case 0x2248c514u:case 0x2248e83cu:case 0x2249964cu:case 0x224a4a4cu:return true;
    default:return false;
    }
}
// Shared 2203e690, mode differs for named and classname acquisition.
bool sequence_permission(void* target,void* a,int mode,const ScriptedSequenceControlBindings& b,
                         int& permission,bool& linked_blocked) noexcept {
    permission=0;linked_blocked=false;
    const sin_independent::EntityHandleView registry(b.entries,b.entry_count);
    int value=1;
    if(void* current=registry.Resolve(read_field<unsigned>(target,0x970))){
        unsigned n=0,vt=0;
        if(!b.extent||!b.extent(b.context,current,&n)||n<scripted_sequence_extent||
           !SinCampaignStorageReadableInternal(current,scripted_sequence_extent)||
           !SinReadCampaignTouchMarkerInternal(current,&vt)||vt!=scripted_sequence_vtable)return false;
        if(void* next=registry.Resolve(read_field<unsigned>(current,0x3d4))){
            if(!b.extent(b.context,next,&n)||n<0xb8||!SinCampaignStorageReadableInternal(next,0xb8))return false;
            const unsigned reason=read_field<unsigned>(current,0x3a4)?1u:(read_field<unsigned>(next,0xb4)&0x200u)?2u:0u;
            if(reason){
                if(!b.linked_warning||!b.linked_warning(b.context,next,reason))return false;
                linked_blocked=true;return true;
            }
        }
        value=2;
    }
    const int activity=read_field<int>(target,0x850);
    const bool force=(read_field<unsigned char>(a,0xb4)&0x40)!=0;
    if(read_field<int>(target,0x8c)==0&&(force||activity==0||activity==1||read_field<int>(target,0x858)==1||(activity==2&&mode>=1)))permission=value;
    return true;
}
// 221ae040 search and 221ae250 handle commit. Radius search iterates
// original list order, not nearest-distance order. Position refresh is
// the pre-existing engine boundary, not a new physics implementation.
int scripted_sequence_acquire(void* a,unsigned n,const ScriptedSequenceAcquireBindings* b,ScriptedSequenceAcquireResult* o) noexcept {
    if(!o)return 0;*o={};o->handle=0xffffffffu;
    unsigned vt=0;
    if(!b||!b->control||!b->search||n<scripted_sequence_extent||
       !SinReadCampaignTouchMarkerInternal(a,&vt)||vt!=scripted_sequence_vtable||
       !SinCampaignStorageReadableInternal(a,scripted_sequence_extent))return 0;
    const auto& c=*b->control;
    if(!c.extent||c.entry_count>sin_independent::kEntityHandleCapacity||(!c.entries&&c.entry_count))return 0;
    static thread_local bool active=false;if(active)return 0;
    struct Guard{bool& flag;~Guard(){flag=false;}} guard{active};active=true;
    const sin_independent::EntityHandleView registry(c.entries,c.entry_count);
    using namespace sin_port::native_weapon_tables;
    const auto* api=SiN_GetServerEntitySearchApi(1,sizeof(ServerEntitySearchApi));
    if(!api)return 0;
    auto search=*b->search;search.entities.entries=c.entries;search.entities.count=c.entry_count;
    auto readable=[&](void* p,unsigned need,unsigned& bytes){return p&&c.extent(c.context,p,&bytes)&&bytes>=need&&SinCampaignStorageReadableInternal(p,need);};
    auto refresh=[&](void* p){
        if(!(read_field<unsigned>(p,0xb8)&0x800u))return true;
        ++o->refreshes;return b->refresh_origin&&b->refresh_origin(b->context,p)!=0;
    };
    auto find=[&](bool named,void* previous,void*& actor){
        actor=nullptr;
        if(!refresh(a))return false;
        const unsigned name=read_field<unsigned>(a,0x3a8);
        const float radius=read_field<float>(a,0x3b0),r2=static_cast<float>(double(radius)*radius);
        if(!std::isfinite(r2))return false;
        if(!name)return true;
        for(unsigned i=0;i<4096;++i){
            ServerEntitySearchResult found{};++o->searches;
            const int ok=named?api->named(previous,name,nullptr,nullptr,&search,&found):api->classname(previous,name,&search,&found);
            if(!ok)return false;
            if(!found.entity)return true;
            if(found.entity==previous)return false;previous=found.entity;
            if(r2==0.f){actor=previous;return true;}
            unsigned bytes=0;
            if(!readable(previous,0x2c8,bytes))return false;
            if(read_field<unsigned>(previous,0x16c)){
                if(!refresh(previous))return false;
                float d[3]{};
                for(unsigned j=0;j<3;++j)d[j]=static_cast<float>(double(read_field<float>(previous,0x2bc+4*j))-read_field<float>(a,0x2bc+4*j));
                const double sq=double(d[2])*d[2]+double(d[1])*d[1]+double(d[0])*d[0];
                if(sq<double(r2)){actor=previous;return true;}
            }
            ++o->radius_skips;
        }
        return false;
    };
    void* candidate=nullptr;void* fallback=nullptr;
    auto* cached=registry.Resolve(read_field<unsigned>(a,0x3dc));
    bool named=true;
    if(!find(true,cached,candidate))return 0;
    if(!candidate){named=false;if(!find(false,registry.Resolve(read_field<unsigned>(a,0x3dc)),candidate))return 0;}
    o->named_path=named;o->class_path=!named;
    for(unsigned i=0;candidate;++i){
        if(i==4096)return 0;++o->candidates;
        unsigned bytes=0,marker=0;
        if(!readable(candidate,4,bytes)||!SinReadCampaignTouchMarkerInternal(candidate,&marker))return 0;
        const bool known=scripted_npc_marker(marker);bool npc=known;
        if(!known&&campaign_player_marker(marker)==PlayerPredicate::Unknown&&!campaign_filter_class(marker)&&marker!=scripted_sequence_vtable){
            if(!c.foreign_is_npc||!c.foreign_is_npc(c.context,candidate,&npc))return 0;
        }
        if(!npc)++o->non_npcs;
        else{
            if(!readable(candidate,0x974,bytes))return 0;
            int permission=0;bool blocked=false;
            const bool force=(read_field<unsigned char>(a,0xb4)&0x40)!=0;
            if(known){
                if(!sequence_permission(candidate,a,named?1:0,c,permission,blocked))return 0;
            }else if(!c.foreign_permission||!c.foreign_permission(c.context,candidate,force,named?1:0,&permission))return 0;
            if(permission==1){o->preferred=1;fallback=candidate;break;}
            if(permission==2){++o->fallback_candidates;fallback=candidate;}
            else{
                ++o->denied;
                if(!(read_field<unsigned>(a,0xb4)&0x800u)&&(!b->denied_warning||!b->denied_warning(b->context,read_field<unsigned>(a,0x3a8))))return 0;
            }
        }
        if(!find(named,candidate,candidate))return 0;
    }
    unsigned handle=0xffffffffu;
    if(fallback){
        unsigned bytes=0,marker=0;
        if(!readable(fallback,4,bytes)||!SinReadCampaignTouchMarkerInternal(fallback,&marker))return 0;
        if(scripted_npc_marker(marker)){
            if(!SinReadCampaignHandleInternal(fallback,bytes,&handle))return 0;
        }else if(!search.foreign_handle||!search.foreign_handle(search.context,fallback,&handle))return 0;
        if(registry.Resolve(handle)!=fallback)return 0;
        o->found=1;o->actor=fallback;
    }
    if(read_field<unsigned>(a,0xb4)&0x400u){write_field(a,0x3dc,handle);o->cached=1;}
    write_field(a,0x3d0,handle);o->handle=handle;o->completed=1;return 1;
}
// 221ae370: named scripts count matching sequence entities on self;
// unnamed scripts write count=1 and stamp curtime immediately.
bool sequence_sync(void* a,const ScriptedSequencePossessionBindings& b,ScriptedSequencePossessionResult& o) noexcept {
    const unsigned name=read_field<unsigned>(a,0xc0);
    if(!name){
        if(!b.now||!std::isfinite(*b.now))return false;
        write_field(a,0x3b8,1u);write_field(a,0x3bc,*b.now);++o.group_updates;return true;
    }
    using namespace sin_port::native_weapon_tables;
    const auto* api=SiN_GetServerEntitySearchApi(1,sizeof(ServerEntitySearchApi));
    if(!api||!b.search)return false;
    auto search=*b.search;search.entities.entries=b.entries;search.entities.count=b.entry_count;
    void* previous=nullptr;
    for(unsigned i=0;i<4096;++i){
        ServerEntitySearchResult found{};
        const unsigned live_name=read_field<unsigned>(a,0xc0);
        if(!api->named(previous,live_name,nullptr,nullptr,&search,&found))return false;
        if(!found.entity)return true;
        if(found.entity==previous)return false;previous=found.entity;
        unsigned n=0,vt=0;
        if(!b.extent||!b.extent(b.context,previous,&n)||n<0x10||!SinCampaignStorageReadableInternal(previous,0x10)||
            !SinReadCampaignTouchMarkerInternal(previous,&vt))return false;
        // Exact original classname or owner-resolved case-insensitive
        // name; refuse foreign entities pretending to be this layout.
        const unsigned token=read_field<unsigned>(previous,0xc);
        bool match=token==0x2242ee1cu;
        if(!match&&token){
            const char* text=nullptr;unsigned bytes=0;
            if(!search.string_text||!search.string_text(search.context,token,&text,&bytes)||!text||!bytes||bytes>4096||
                !SinCampaignStorageReadableInternal(text,bytes)||!std::memchr(text,0,bytes))return false;
            match=equal_class(text,"scripted_sequence");
        }
        if(match){
            if(vt!=scripted_sequence_vtable||n<scripted_sequence_extent)return false;
            ++o.group_matches;
            write_field(a,0x3b8,read_field<unsigned>(a,0x3b8)+1u);++o.group_updates;
        }
    }
    return false;
}
// Original 221ae8f0. Registry identity checks prevent stale/foreign
// actor layout use. A live prior script receives successor only.
int scripted_sequence_possession(void* a,unsigned n,const ScriptedSequencePossessionBindings* b,ScriptedSequencePossessionResult* o) noexcept {
    if(!o)return 0;*o={};
    unsigned vt=0;
    if(!b||!b->extent||n<scripted_sequence_extent||!SinReadCampaignTouchMarkerInternal(a,&vt)||vt!=scripted_sequence_vtable||
       !SinCampaignStorageReadableInternal(a,scripted_sequence_extent)||b->entry_count>sin_independent::kEntityHandleCapacity||
       (!b->entries&&b->entry_count))return 0;
    static thread_local bool active=false;if(active)return 0;
    struct Guard{bool& flag;~Guard(){flag=false;}} guard{active};active=true;
    const sin_independent::EntityHandleView registry(b->entries,b->entry_count);
    auto done=[&](){o->completed=1;return 1;};
    auto readable=[&](void* p,unsigned need,unsigned& bytes,unsigned& marker){
        return p&&b->extent(b->context,p,&bytes)&&bytes>=need&&SinCampaignStorageReadableInternal(p,need)&&SinReadCampaignTouchMarkerInternal(p,&marker);
    };
    o->npc_handle=read_field<unsigned>(a,0x3d0);o->script_handle=read_field<unsigned>(a,0x378);
    void* npc=registry.Resolve(o->npc_handle);
    if(!npc){o->missing=1;return done();}
    unsigned bytes=0,marker=0;
    if(!readable(npc,4,bytes,marker))return 0;
    if(!scripted_npc_marker(marker)){
        if(campaign_player_marker(marker)==PlayerPredicate::Unknown&&!campaign_filter_class(marker)&&marker!=scripted_sequence_vtable)return 0;
        o->non_npc=1;return done();
    }
    if(!readable(npc,0x974,bytes,marker)||registry.Resolve(o->script_handle)!=a)return 0;
    auto current=[&](){return registry.Resolve(read_field<unsigned>(npc,0x970));};
    auto seq=[&](void* p){unsigned ext=0,m=0;return readable(p,scripted_sequence_extent,ext,m)&&m==scripted_sequence_vtable;};
    if(void* old=current()){
        if(!seq(old))return 0;
        if(void* next=registry.Resolve(read_field<unsigned>(old,0x3d4))){
            if(!seq(next)||!b->replaced)return 0;
            // Original compares resolved identities, not raw handles.
            if(registry.Resolve(read_field<unsigned>(old,0x3d4))!=current()){
                write_field(next,0x3d0,0xffffffffu);o->displaced=1;
            }
            if(!b->replaced(b->context,a,next))return 0;
        }
        old=current();if(!seq(old))return 0;
        write_field(old,0x3d4,read_field<unsigned>(a,0x378));o->queued=1;return done();
    }
    const unsigned mode=read_field<unsigned>(a,0x3ac);o->move_mode=mode;
    if(!b->add_flags||(mode==4&&(!b->move4_prepare||!b->move4_reset||!b->move4_angles)))return 0;
    if(!read_field<unsigned>(a,0x3a4))write_field(a,0x3d4,0xffffffffu);
    write_field(npc,0x90c,read_field<unsigned>(a,0x378));write_field(npc,0x864,0u);write_field(npc,0x85c,0u);
    write_field(npc,0x970,read_field<unsigned>(a,0x378));
    write_field(npc,0x8a8,read_field<unsigned>(a,0x378));o->claimed=1;
    write_field(a,0x3c4,read_field<unsigned>(npc,0x78));
    ++o->flags_calls;if(!b->add_flags(b->context,npc,read_field<unsigned>(a,0x78)))return 0;
    write_field(a,0x3c8,read_field<unsigned>(npc,0xbc));
    if(mode==4){
        if(!b->move4_prepare(b->context,npc,static_cast<unsigned char*>(a)+0x360,read_field<unsigned>(a,0x370))||
            !b->move4_reset(b->context,npc))return 0;
        ++o->flags_calls;if(!b->add_flags(b->context,npc,8))return 0;
        float angles[3]{};std::memcpy(angles,static_cast<unsigned char*>(npc)+0x36c,sizeof(angles));
        angles[1]=read_field<float>(a,0x370);
        if(!b->move4_angles(b->context,npc,angles))return 0;
        write_field(npc,0x96c,1u);
    }else if(mode==0||mode==5)write_field(npc,0x96c,1u);
    else if(mode>=1&&mode<=3)write_field(npc,0x96c,mode+3);
    o->state=read_field<int>(npc,0x96c);
    if(!sequence_sync(a,*b,*o))return 0;
    if(read_field<int>(npc,0x858)!=4)write_field(npc,0x858,4);
    return done();
}
// 221af840 handles one actor. The named-group loop belongs to
// acquisition failure at 221afc00, not the CancelSequence input itself.
int scripted_sequence_cancel(void* a,unsigned n,const ScriptedSequenceCancelBindings* b,ScriptedSequenceCancelResult* o) noexcept {
    if(!o)return 0;*o={};
    if(!a||!b||!b->control||!b->control->extent||n<0x10||!SinCampaignStorageReadableInternal(a,0x10))return 0;
    const auto& c=*b->control;
    if(c.entry_count>sin_independent::kEntityHandleCapacity||(!c.entries&&c.entry_count))return 0;
    bool match=read_field<unsigned>(a,0xc)==0x2242ee1cu;
    if(!match&&read_field<unsigned>(a,0xc)){
        const char* text=nullptr;unsigned size=0;
        if(!b->search||!b->search->string_text||!b->search->string_text(b->search->context,read_field<unsigned>(a,0xc),&text,&size)||
           !text||!size||size>4096||!SinCampaignStorageReadableInternal(text,size)||!std::memchr(text,0,size))return 0;
        match=equal_class(text,"scripted_sequence");
    }
    if(!match){o->completed=1;return 1;}
    unsigned vt=0;
    if(n<scripted_sequence_extent||!SinReadCampaignTouchMarkerInternal(a,&vt)||vt!=scripted_sequence_vtable||
       !SinCampaignStorageReadableInternal(a,scripted_sequence_extent))return 0;
    o->matched=1;
    const sin_independent::EntityHandleView registry(c.entries,c.entry_count);
    void* npc=registry.Resolve(read_field<unsigned>(a,0x3d0));
    if(npc){
        unsigned bytes=0,marker=0;
        if(!c.extent(c.context,npc,&bytes)||bytes<4||!SinReadCampaignTouchMarkerInternal(npc,&marker))return 0;
        if(scripted_npc_marker(marker)){
            if(bytes<0x974||!SinCampaignStorageReadableInternal(npc,0x974))return 0;
            if(read_field<int>(npc,0x850)==4){
                if(!b->cleanup_script_state||!b->cancel_schedule)return 0;
                write_field(npc,0x96c,3u);o->scripted_state=1;
                unsigned flags=0;
                if(void* current=registry.Resolve(read_field<unsigned>(npc,0x970))){
                    unsigned cb=0,cv=0;
                    if(!c.extent(c.context,current,&cb)||cb<scripted_sequence_extent||!SinCampaignStorageReadableInternal(current,scripted_sequence_extent)||
                       !SinReadCampaignTouchMarkerInternal(current,&cv)||cv!=scripted_sequence_vtable)return 0;
                    flags=read_field<unsigned>(current,0x3c8);
                }
                if(!b->cleanup_script_state(b->context,npc))return 0;o->cleanup=1;
                if(!b->cancel_schedule(b->context,a,npc,flags))return 0;
            }else{
                if(!b->restore_flags)return 0;
                write_field(a,0x3d0,0xffffffffu);
                if(!b->restore_flags(b->context,npc,read_field<unsigned>(a,0x3c4)))return 0;o->restored_flags=1;
                write_field(npc,0x970,0xffffffffu);write_field(npc,0x8a8,0xffffffffu);write_field(npc,0x90c,0xffffffffu);
                if(read_field<int>(npc,0x858)!=1)write_field(npc,0x858,1);
                o->cleared=1;
            }
        }else if(campaign_player_marker(marker)!=PlayerPredicate::Unknown||campaign_filter_class(marker)||marker==scripted_sequence_vtable)o->non_npc=1;
        else return 0;
    }else o->missing=1;
    write_field(a,0x3b8,0u);o->count_reset=o->completed=1;return 1;
}
int scripted_sequence_think(void* a,unsigned n,const ScriptedSequenceThinkBindings* b,ScriptedSequenceThinkResult* o) noexcept {
    if(!o)return 0;*o={};
    unsigned vt=0;
    if(!b||!b->acquire||!b->acquire->control||!b->possession||!b->cancel||
       n<scripted_sequence_extent||!SinReadCampaignTouchMarkerInternal(a,&vt)||vt!=scripted_sequence_vtable||
       !SinCampaignStorageReadableInternal(a,scripted_sequence_extent)||read_field<unsigned>(a,8)!=0x221afc90u)return 0;
    const auto& c=*b->acquire->control;
    if(!c.now||!c.interval||!c.cadence||b->cancel->control!=&c||b->possession->entries!=c.entries||
       b->possession->entry_count!=c.entry_count||b->possession->now!=c.now||
       c.entry_count>sin_independent::kEntityHandleCapacity||(!c.entries&&c.entry_count))return 0;
    static thread_local bool active=false;if(active)return 0;
    struct Guard{bool& flag;~Guard(){flag=false;}} guard{active};active=true;
    auto report=[&](unsigned stage,void* npc,unsigned token){
        if(!b->report||!b->report(b->context,a,stage,npc,token))return false;
        ++o->reported;return true;
    };
    if(b->rules_enabled&&!*b->rules_enabled)o->rules_blocked=1;
    else{
        if(!scripted_sequence_acquire(a,n,b->acquire,&o->acquire))return 0;
        if(o->acquire.found){
            o->acquired=1;
            if(!scripted_sequence_possession(a,n,b->possession,&o->possession))return 0;
            const sin_independent::EntityHandleView registry(c.entries,c.entry_count);
            void* npc=registry.Resolve(read_field<unsigned>(a,0x3d0));
            if(!npc||!report(1,npc,read_field<unsigned>(a,0x3a8)))return 0;
            o->possessed=o->possession.claimed||o->possession.queued;o->completed=1;return 1;
        }
        // 221afc00 first reports animation, then cancels one unnamed
        // script or each live same-name entity. Re-read name per lookup.
        if(!report(2,nullptr,read_field<unsigned>(a,0x398)))return 0;
        auto cancel=[&](void* target,unsigned bytes){
            ScriptedSequenceCancelResult result{};
            if(!scripted_sequence_cancel(target,bytes,b->cancel,&result))return false;
            o->cancelled+=result.matched;return true;
        };
        if(!read_field<unsigned>(a,0xc0)){if(!cancel(a,n))return 0;}
        else{
            using namespace sin_port::native_weapon_tables;
            const auto* api=SiN_GetServerEntitySearchApi(1,sizeof(ServerEntitySearchApi));
            if(!api||!b->cancel->search||!c.extent)return 0;
            auto search=*b->cancel->search;search.entities.entries=c.entries;search.entities.count=c.entry_count;
            void* previous=nullptr;
            for(unsigned i=0;;++i){
                if(i==4096)return 0;
                const unsigned name=read_field<unsigned>(a,0xc0);if(!name)break;
                ServerEntitySearchResult found{};
                if(!api->named(previous,name,nullptr,nullptr,&search,&found))return 0;
                if(!found.entity)break;if(found.entity==previous)return 0;previous=found.entity;
                unsigned bytes=0;if(!c.extent(c.context,previous,&bytes)||!cancel(previous,bytes))return 0;
            }
        }
        if(!report(3,nullptr,read_field<unsigned>(a,0x3a8)))return 0;
    }
    // Original 221afd47: use live time after all earlier calls.
    o->retry_time=static_cast<float>(double(*c.now)+1.);
    using namespace sin_port::native_weapon_tables;
    const auto* api=SiN_GetServerNpcOuterCadenceApi(1,sizeof(ServerNpcOuterCadenceApi));
    ServerNpcOuterCadenceResult cadence{};
    if(!api||!api->set_default(a,n,o->retry_time,*c.interval,scripted_sequence_vtable,c.cadence,&cadence))return 0;
    o->due_tick=cadence.tick;o->retry_scheduled=o->completed=1;return 1;
}
// Original Spawn 221b0be0: configure collision through owner, install
// acquisition think on unnamed or auto-start scripts, then reset latches.
int scripted_sequence_spawn(void* a,unsigned n,const ScriptedSequenceSpawnBindings* b,ScriptedSequenceSpawnResult* o) noexcept {
    if(!o)return 0;*o={};
    unsigned vt=0;
    if(!b||!b->control||!b->solid_type||n<scripted_sequence_extent||
       !SinReadCampaignTouchMarkerInternal(a,&vt)||vt!=scripted_sequence_vtable||
       !SinCampaignStorageReadableInternal(a,scripted_sequence_extent))return 0;
    const auto& c=*b->control;
    if(!b->solid_type(b->context,static_cast<unsigned char*>(a)+0xfc,0))return 0;o->solid_called=1;
    if(!read_field<unsigned>(a,0xc0)||(read_field<unsigned>(a,0xb4)&0x10u)){
        if(!c.now||!c.interval||!c.cadence||!std::isfinite(*c.now)||!std::isfinite(*c.interval)||*c.interval<=0)return 0;
        write_field(a,0x3cd,static_cast<unsigned char>(0));write_field(a,0x3d8,static_cast<unsigned char>(1));
        write_field(a,8,0x221afc90u);o->think_installed=1;
        using namespace sin_port::native_weapon_tables;
        const auto* api=SiN_GetServerNpcOuterCadenceApi(1,sizeof(ServerNpcOuterCadenceApi));
        ServerNpcOuterCadenceResult cadence{};
        if(!api||!api->set_default(a,n,static_cast<float>(double(*c.now)+1.),*c.interval,vt,c.cadence,&cadence))return 0;
        o->scheduled=1;o->due_tick=cadence.tick;
        if(read_field<unsigned>(a,0xc0))write_field(a,0x3c0,static_cast<unsigned char>(1));
    }
    const auto flag=static_cast<unsigned char>((read_field<unsigned>(a,0xb4)&0x20u)==0);
    write_field(a,0x3cc,flag);write_field(a,0x3cd,static_cast<unsigned char>(0));write_field(a,0x3bc,0u);
    write_field(a,0x3d4,0xffffffffu);write_field(a,0x3dc,0xffffffffu);
    o->waiting=read_field<unsigned char>(a,0x3c0);o->interruptible=flag;o->completed=1;return 1;
}
// Original Activate 221ae430: base first, name-or-class presence scan,
// then exact first named successor (no RTTI cast in the original).
int scripted_sequence_activate(void* a,unsigned n,const ScriptedSequenceActivateBindings* b,ScriptedSequenceActivateResult* o) noexcept {
    if(!o)return 0;*o={};o->next_handle=0xffffffffu;
    unsigned vt=0;
    if(!b||!b->control||!b->search||!b->base_activate||n<scripted_sequence_extent||
       !SinReadCampaignTouchMarkerInternal(a,&vt)||vt!=scripted_sequence_vtable||
       !SinCampaignStorageReadableInternal(a,scripted_sequence_extent))return 0;
    const auto& c=*b->control;
    if(!c.extent||c.entry_count>sin_independent::kEntityHandleCapacity||(!c.entries&&c.entry_count))return 0;
    if(!b->base_activate(b->context,a))return 0;o->base_called=1;
    const sin_independent::EntityHandleView registry(c.entries,c.entry_count);
    using namespace sin_port::native_weapon_tables;
    const auto* api=SiN_GetServerEntitySearchApi(1,sizeof(ServerEntitySearchApi));
    if(!api)return 0;
    auto search=*b->search;search.entities.entries=c.entries;search.entities.count=c.entry_count;
    void* previous=nullptr;
    for(unsigned i=0;;++i){
        if(i==4096)return 0;
        const unsigned name=read_field<unsigned>(a,0x3a8);
        if(!name)break;
        ServerEntitySearchResult found{};++o->searches;
        if(!api->named(previous,name,nullptr,nullptr,&search,&found))return 0;
        if(!found.entity){++o->searches;if(!api->classname(previous,name,&search,&found))return 0;}
        if(!found.entity)break;if(found.entity==previous)return 0;previous=found.entity;++o->visited;
        unsigned bytes=0,marker=0;
        if(!c.extent(c.context,previous,&bytes)||bytes<4||!SinReadCampaignTouchMarkerInternal(previous,&marker))return 0;
        bool npc=scripted_npc_marker(marker);
        if(!npc&&campaign_player_marker(marker)==PlayerPredicate::Unknown&&!campaign_filter_class(marker)&&marker!=scripted_sequence_vtable){
            if(!c.foreign_is_npc||!c.foreign_is_npc(c.context,previous,&npc))return 0;
        }
        if(npc){o->npc_found=1;break;}
    }
    const unsigned name=read_field<unsigned>(a,0x3a4);
    if(name){
        ServerEntitySearchResult found{};++o->searches;
        if(!api->named(nullptr,name,nullptr,nullptr,&search,&found))return 0;
        if(found.entity){
            unsigned bytes=0,marker=0;
            if(!c.extent(c.context,found.entity,&bytes)||bytes<4||!SinReadCampaignTouchMarkerInternal(found.entity,&marker))return 0;
            if(scripted_npc_marker(marker)||marker==scripted_sequence_vtable||campaign_filter_class(marker)||campaign_player_marker(marker)!=PlayerPredicate::Unknown){
                if(!SinReadCampaignHandleInternal(found.entity,bytes,&o->next_handle))return 0;
            }else if(!search.foreign_handle||!search.foreign_handle(search.context,found.entity,&o->next_handle))return 0;
            o->next_found=registry.Resolve(o->next_handle)!=nullptr;
        }
    }
    write_field(a,0x3d4,o->next_handle);
    if(!registry.Resolve(o->next_handle)){write_field(a,0x3a4,0u);o->next_name_cleared=1;}
    o->completed=1;return 1;
}
// No-edict 2221c35f -> 22222c30 -> 22222b90 default context.
int sequence_removal_think(void* a,unsigned n,const ScriptedSequenceRemovalThinkBindings* b,ScriptedSequenceRemovalThinkResult* o) noexcept {
    if(!o)return 0;*o={};
    unsigned vt=0;
    if(!b||!b->deferred||!b->cancel||!b->cancel->control||n<scripted_sequence_extent||
       !SinReadCampaignTouchMarkerInternal(a,&vt)||vt!=scripted_sequence_vtable||
       !SinCampaignStorageReadableInternal(a,scripted_sequence_extent))return 0;
    if(read_field<int>(a,0x9c)>0){
        if(!b->positive_health||!b->warning||!b->positive_health(b->context,a))return 0;o->positive_health=1;
        write_field(a,0x9c,0);o->health_cleared=1;
        if(!b->warning(b->context,a))return 0;o->warning_called=1;
    }
    if(!defer_remove_impl(a,n,b->deferred,&o->removal,b->cancel,&o->cancelled))return 0;
    o->completed=1;return 1;
}
int sequence_primary(void* a,unsigned n,int tick,const ScriptedSequencePrimaryBindings* b,ScriptedSequencePrimaryResult* o,
    const ScriptedSequenceRemovalThinkBindings* removal=nullptr,ScriptedSequenceRemovalThinkResult* removed=nullptr) noexcept {
    if(removed)*removed={};
    if(removal&&(!removed||!b||!b->think||removal->cancel!=b->think->cancel||!removal->deferred))return 0;
    if(!o)return 0;*o={};o->due_tick=o->next_tick=o->last_tick=-1;
    unsigned vt=0;
    if(!b||!b->think||!b->think->acquire||!b->think->acquire->control||n<scripted_sequence_extent||
       !SinReadCampaignTouchMarkerInternal(a,&vt)||vt!=scripted_sequence_vtable||
       !SinCampaignStorageReadableInternal(a,scripted_sequence_extent)||read_field<int>(a,0x50)>0)return 0;
    const auto& c=*b->think->acquire->control;
    if(!c.now||!c.interval||!c.cadence||!std::isfinite(*c.now)||!std::isfinite(*c.interval)||*c.interval<=0||
       c.entry_count>sin_independent::kEntityHandleCapacity||(!c.entries&&c.entry_count)||
       sin_independent::EntityHandleView(c.entries,c.entry_count).Resolve(read_field<unsigned>(a,0x378))!=a)return 0;
    auto idle=[&](){o->next_tick=read_field<int>(a,0x74);o->native_result=o->completed=1;return 1;};
    if(read_field<unsigned>(a,0xb8)&0x400000u){o->no_think=1;return idle();}
    o->due_tick=read_field<int>(a,0x74);
    if(o->due_tick<=0||o->due_tick>tick){o->not_due=1;return idle();}
    const unsigned token=read_field<unsigned>(a,8);
    if(token&&token!=0x221afc90u&&!(removal&&token==0x221c0d40u))return 0;
    if(!b->record||!b->scope)return 0;
    const float interval=*c.interval;
    const double scheduled=double(o->due_tick)*interval;
    o->recorded_time=scheduled>double(*c.now)?*c.now:static_cast<float>(scheduled);
    if(!b->record(b->context,a,tick,o->recorded_time,read_field<int>(a,0x74)))return 0;o->recorded=1;
    using namespace sin_port::native_weapon_tables;
    const auto* api=SiN_GetServerNpcOuterCadenceApi(1,sizeof(ServerNpcOuterCadenceApi));
    ServerNpcOuterCadenceResult cadence{};
    if(!api||!api->set_default(a,n,-1.f,interval,vt,c.cadence,&cadence)||
       !api->set_default(a,n,-1.f,interval,vt,c.cadence,&cadence))return 0;
    if(!b->scope(b->context,a,true))return 0;o->entered=1;
    // 22002ca0 re-reads the token after the default-clear callbacks.
    const unsigned live=read_field<unsigned>(a,8);
    bool ok=live==0;
    if(live==0x221afc90u){o->invoked=1;ok=scripted_sequence_think(a,n,b->think,&o->behavior)!=0;}
    else if(live==0x221c0d40u&&removal){o->invoked=1;ok=sequence_removal_think(a,n,removal,removed)!=0;}
    const bool closed=b->scope(b->context,a,false)!=0;o->closed=closed;
    if(!ok||!closed)return 0;
    o->last_time=*c.now;
    const double last=std::trunc(double(o->last_time)/interval+.5);
    if(!std::isfinite(last)||last<INT_MIN||last>INT_MAX)return 0;
    o->last_tick=static_cast<int>(last);write_field(a,0x34,o->last_tick);
    o->next_tick=read_field<int>(a,0x74);o->native_result=(read_field<unsigned>(a,0xb8)&1u)==0;
    o->completed=1;return 1;
}
// Original 221b0430/221b0500/221af970 input wrappers. Native
// handle/state/permission leaves; possession/cancel group remain owners.
int scripted_sequence_control(void* a,unsigned n,ScriptedSequenceInput input,
    const ScriptedSequenceControlBindings* b,ScriptedSequenceControlResult* o,
    const ScriptedSequenceNativeControlBindings* native=nullptr) noexcept {
    if(!o)return 0;*o={};
    if(native&&(!b||native->control!=b||!native->possession||!native->cancel||native->cancel->control!=b||
        native->possession->entries!=b->entries||native->possession->entry_count!=b->entry_count||
        native->possession->now!=b->now))return 0;
    unsigned marker=0;
    if(!b||n<scripted_sequence_extent||!SinReadCampaignTouchMarkerInternal(a,&marker)||marker!=scripted_sequence_vtable||
       !SinCampaignStorageReadableInternal(a,scripted_sequence_extent))return 0;
    if(input!=ScriptedSequenceInput::MoveToPosition&&input!=ScriptedSequenceInput::BeginSequence&&input!=ScriptedSequenceInput::CancelSequence)return 0;
    if(read_field<unsigned char>(a,0x3d9)){o->suppressed=o->completed=1;return 1;}
    static thread_local bool active=false;if(active)return 0;
    struct Guard{bool& flag;~Guard(){flag=false;}} guard{active};active=true;
    auto done=[&](){o->completed=1;return 1;};
    if(input==ScriptedSequenceInput::CancelSequence){
        if(!b->cancel_warning||(!native&&!b->cancel_group)||!b->cancel_warning(b->context,read_field<unsigned>(a,0x398)))return 0;
        if(read_field<unsigned char>(a,0x3d8)){
            // 2214d6f0 with null context writes only default think pointer.
            write_field(a,8,0u);write_field(a,0x3d8,static_cast<unsigned char>(0));o->think_cleared=1;
        }
        if(native){
            ScriptedSequenceCancelResult result{};
            if(!scripted_sequence_cancel(a,n,native->cancel,&result)||!result.matched)return 0;
        }else if(!b->cancel_group(b->context,a))return 0;
        o->cancelled=1;return done();
    }
    const bool move=input==ScriptedSequenceInput::MoveToPosition;
    // BeginSequence clears waiting before lookup, even on denial.
    if(!move)write_field(a,0x3c0,static_cast<unsigned char>(0));
    if(b->entry_count>sin_independent::kEntityHandleCapacity||(!b->entries&&b->entry_count))return 0;
    const sin_independent::EntityHandleView registry(b->entries,b->entry_count);
    auto readable=[&](void* p,unsigned need,unsigned& extent){
        return b->extent&&b->extent(b->context,p,&extent)&&extent>=need&&SinCampaignStorageReadableInternal(p,need);
    };
    void* target=registry.Resolve(read_field<unsigned>(a,0x3d0));bool npc=false,known=false;unsigned bytes=0;
    if(target){
        o->target_resolved=1;unsigned vt=0;
        if(!readable(target,4,bytes)||!SinReadCampaignTouchMarkerInternal(target,&vt))return 0;
        known=scripted_npc_marker(vt);npc=known;
        if(!known&&campaign_player_marker(vt)==PlayerPredicate::Unknown&&!campaign_filter_class(vt)&&vt!=scripted_sequence_vtable){
            if(!b->foreign_is_npc||!b->foreign_is_npc(b->context,target,&npc))return 0;
            o->foreign_npc=1;
        }
    }
    if(npc){
        o->npc=1;if(!readable(target,0x974,bytes))return 0;
        const int state=read_field<int>(target,0x96c);
        if(state!=0&&state!=2){o->state_blocked=1;return done();}
        const bool force=(read_field<unsigned char>(a,0xb4)&0x40)!=0;
        if(known){
            bool blocked=false;
            if(!sequence_permission(target,a,1,*b,o->permission,blocked))return 0;
            if(blocked){o->linked_blocked=o->permission_blocked=1;return done();}
        }else if(!b->foreign_permission||!b->foreign_permission(b->context,target,force,1,&o->permission))return 0;
        if(!o->permission){o->permission_blocked=1;return done();}
        if(native){
            ScriptedSequencePossessionResult result{};
            if(!scripted_sequence_possession(a,n,native->possession,&result)||(!result.claimed&&!result.queued))return 0;
        }else if(!b->possess||!b->possess(b->context,a))return 0;
        o->possessed=1;
    }else{
        // Missing/stale/non-NPC -> install acquisition think then
        // schedule at live curtime, not an arbitrary frame delay.
        if(!b->now||!b->interval||!b->cadence||!std::isfinite(*b->now)||!std::isfinite(*b->interval)||*b->interval<=0)return 0;
        const auto* api=SiN_GetServerNpcOuterCadenceApi(1,sizeof(native_weapon_tables::ServerNpcOuterCadenceApi));
        if(!api)return 0;
        write_field(a,0x3cd,static_cast<unsigned char>(0));write_field(a,0x3d8,static_cast<unsigned char>(1));write_field(a,8,0x221afc90u);o->think_installed=1;
        native_weapon_tables::ServerNpcOuterCadenceResult cadence{};
        if(!api->set_default(a,n,*b->now,*b->interval,scripted_sequence_vtable,b->cadence,&cadence))return 0;
        o->tick_scheduled=1;o->due_tick=cadence.tick;
    }
    // MoveToPosition writes waiting only AFTER successful possession
    // or scheduling. The denial exits do not write it.
    if(move)write_field(a,0x3c0,static_cast<unsigned char>(1));
    return done();
}
// 221adec0: index 1..8 only, caller and activator are both self.
int scripted_sequence_event(void* actor,unsigned bytes,int index,const ScriptedSequenceEventBindings* b,ScriptedSequenceEventResult* o) noexcept {
    if(!o)return 0;*o={};
    unsigned vt=0;
    if(bytes<scripted_sequence_extent||!SinReadCampaignTouchMarkerInternal(actor,&vt)||vt!=scripted_sequence_vtable||
       !SinCampaignStorageReadableInternal(actor,scripted_sequence_extent))return 0;
    if(index<1||index>8){o->completed=1;return 1;}
    if(!b||!b->output)return 0;
    const unsigned off=0x3f8+0x18*static_cast<unsigned>(index);o->offset=off;
    static thread_local bool active=false;if(active)return 0;
    struct Guard{bool& flag;~Guard(){flag=false;}} guard{active};active=true;
    using namespace sin_port::native_weapon_tables;
    if(b->native_queue){
        if(!native_named_output(actor,bytes,actor,off,false,b->output,b->native_queue,o->queued))return 0;
        o->native_queued=o->queued;
    }else{
        const auto* fire=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
        ServerEntityVoidOutputResult r{};
        if(!fire||!fire->fire(static_cast<unsigned char*>(actor)+off,0x18,actor,actor,bytes,0.f,b->output,&r))return 0;
        o->queued=r.queued;
    }
    o->output_called=o->completed=1;return 1;
}
// 221aebc0 sets the sequence-start latch even for an empty token.
bool animation_valid(void* a,unsigned n,void* npc,const ScriptedSequenceAnimationBindings* b,unsigned& bytes) noexcept {
    unsigned vt=0,nv=0;
    return b&&b->control&&b->control->extent&&n>=scripted_sequence_extent&&
        SinReadCampaignTouchMarkerInternal(a,&vt)&&vt==scripted_sequence_vtable&&SinCampaignStorageReadableInternal(a,scripted_sequence_extent)&&
        npc&&b->control->extent(b->control->context,npc,&bytes)&&bytes>=0x974&&
        SinReadCampaignTouchMarkerInternal(npc,&nv)&&scripted_npc_marker(nv)&&SinCampaignStorageReadableInternal(npc,bytes);
}
int sequence_finish(void*,unsigned,void*,const ScriptedSequenceFinishBindings*,ScriptedSequenceFinishResult*) noexcept;
bool animation_done(void*,unsigned,void*,unsigned,const ScriptedSequenceAnimationBindings&,ScriptedSequenceAnimationResult&,
    const ScriptedSequenceFinishBindings* =nullptr,ScriptedSequenceFinishResult* =nullptr) noexcept;
bool animation_start(void* a,unsigned n,void* npc,unsigned bytes,unsigned token,bool complete_empty,
    const ScriptedSequenceAnimationBindings& b,ScriptedSequenceAnimationResult& o,
    const ScriptedSequenceFinishBindings* finish=nullptr,ScriptedSequenceFinishResult* terminal=nullptr) noexcept {
    write_field(a,0x3cd,static_cast<unsigned char>(1));
    if(!token&&complete_empty){
        o.empty_completed=1;
        if(!animation_done(a,n,npc,bytes,b,o,finish,terminal))return false;
        o.started=0; // original AL false, even if post-idle started
        return true;
    }
    const unsigned name=token?token:0x223d47a8u; // original empty string
    if(!b.lookup||!b.activity||!b.assignment)return false;
    if(!b.lookup(b.context,npc,name,&o.sequence))return false;o.lookup_called=1;
    if(o.sequence==-1){
        if(!b.missing_sequence||!b.missing_sequence(b.context,npc,name))return false;
        o.missing=1;o.sequence=0;
    }
    if(o.sequence<0||!b.activity(b.context,npc,o.sequence,&o.activity))return false;o.activity_called=1;
    if(o.activity==-1){o.activity=1;o.activity_fallback=1;}
    // Original 220417d0 receives activity, sequence, activity, activity.
    // Reuse the native direct assignment, including sequence reset and
    // event registration, without modifying pending selection fields.
    using namespace sin_port::native_weapon_tables;
    const auto* api=SiN_GetServerNpcActivityAssignmentApi(1,sizeof(ServerNpcActivityAssignmentApi));
    const ServerNpcActivityAssignmentInput in{o.activity,o.sequence,o.activity,o.activity};
    ServerNpcActivityAssignmentResult result{};
    if(!api||!api->assign(npc,bytes,&in,b.assignment,&result)||!result.assigned)return false;
    o.assigned=o.started=1;return true;
}
// 221ae2c0: no successor + nonempty post-idle -> NPC script state 2
// and native StartSequence; otherwise call the remaining finish leaf.
// Both branches fire OnEndSequence with null activator and self caller.
bool animation_done(void* a,unsigned n,void* npc,unsigned bytes,const ScriptedSequenceAnimationBindings& b,
    ScriptedSequenceAnimationResult& o,const ScriptedSequenceFinishBindings* finish,ScriptedSequenceFinishResult* terminal) noexcept {
    const auto& c=*b.control;
    if(c.entry_count>sin_independent::kEntityHandleCapacity||(!c.entries&&c.entry_count)||!b.events||!b.events->output)return false;
    const sin_independent::EntityHandleView registry(c.entries,c.entry_count);
    const unsigned post=read_field<unsigned>(a,0x39c);
    if(post&&!registry.Resolve(read_field<unsigned>(a,0x3d4))){
        write_field(npc,0x96c,2u);o.post_idle=1;
        if(!animation_start(a,n,npc,bytes,post,false,b,o))return false;
        o.post_idle_started=o.started;
    }else{
        if(finish){
            if(!terminal||finish->animation!=&b||!sequence_finish(a,n,npc,finish,terminal))return false;
        }else if(!b.finish||!b.finish(b.context,a,npc))return false;
        o.finish_called=1;
    }
    if(b.events->native_queue){
        if(!native_named_output(a,n,nullptr,0x3f8,false,b.events->output,b.events->native_queue,o.queued))return false;
        o.native_queued=o.queued;
    }else{
        using namespace sin_port::native_weapon_tables;
        const auto* fire=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
        ServerEntityVoidOutputResult r{};
        if(!fire||!fire->fire(static_cast<unsigned char*>(a)+0x3f8,0x18,nullptr,a,n,0.f,b.events->output,&r))return false;
        o.queued=r.queued;
    }
    o.output_called=1;return true;
}
int run_sequence_animation(void* a,unsigned n,void* npc,unsigned token,bool complete_empty,bool done,
    const ScriptedSequenceAnimationBindings* b,ScriptedSequenceAnimationResult* o,
    const ScriptedSequenceFinishBindings* finish=nullptr,ScriptedSequenceFinishResult* terminal=nullptr) noexcept {
    if(terminal){*terminal={};terminal->due_tick=-1;terminal->successor_handle=0xffffffffu;terminal->animation.sequence=terminal->animation.activity=-1;}
    if(!o)return 0;*o={};o->sequence=o->activity=-1;
    if(finish&&(!terminal||finish->animation!=b))return 0;
    unsigned bytes=0;if(!animation_valid(a,n,npc,b,bytes))return 0;
    static thread_local bool active=false;if(active)return 0;
    struct Guard{bool& flag;~Guard(){flag=false;}} guard{active};active=true;
    const bool ok=done?animation_done(a,n,npc,bytes,*b,*o,finish,terminal):animation_start(a,n,npc,bytes,token,complete_empty,*b,*o,finish,terminal);
    o->completed=ok;return int(ok);
}
// 221afa70 terminal completion, with 221af490 schedule reset
// reconstructed below. Successor resolution is intentionally AFTER the
// owner cleanup and schedule calls, which can mutate live handles.
bool sequence_finish_impl(void* a,unsigned n,void* npc,unsigned bytes,
    const ScriptedSequenceFinishBindings& b,ScriptedSequenceFinishResult& o) noexcept {
    const auto& animation=*b.animation;const auto& c=*animation.control;
    if(c.entry_count>sin_independent::kEntityHandleCapacity||(!c.entries&&c.entry_count))return false;
    const sin_independent::EntityHandleView registry(c.entries,c.entry_count);
    const unsigned post=read_field<unsigned>(a,0x39c);
    if(post&&(read_field<unsigned>(a,0xb4)&0x100u)&&!registry.Resolve(read_field<unsigned>(a,0x3d4))){
        if(read_field<unsigned char>(npc,0xa9)&0x10u){
            void* current=registry.Resolve(read_field<unsigned>(npc,0x970));
            unsigned size=0,vt=0;
            if(!current||!c.extent(c.context,current,&size)||size<scripted_sequence_extent||
               !SinReadCampaignTouchMarkerInternal(current,&vt)||vt!=scripted_sequence_vtable||
               !SinCampaignStorageReadableInternal(current,scripted_sequence_extent)||!b.post_idle_debug)return false;
            const unsigned token=read_field<unsigned>(current,0x39c);
            if(!b.post_idle_debug(b.context,npc,token?token:0x223d47a8u))return false;
            o.debugged=1;
        }
        write_field(npc,0x96c,2u);
        if(!animation_start(a,n,npc,bytes,read_field<unsigned>(a,0x39c),false,animation,o.animation))return false;
        o.animation.completed=o.looped=1;return true;
    }
    if(!b.cleanup_script_state||!b.current_schedule||!b.clear_schedule)return false;
    if(!(read_field<unsigned>(a,0xb4)&4u)){
        if(!c.now||!c.interval||!c.cadence||!std::isfinite(*c.now)||!std::isfinite(*c.interval)||*c.interval<=0)return false;
        using namespace sin_port::native_weapon_tables;
        const auto* api=SiN_GetServerNpcOuterCadenceApi(1,sizeof(ServerNpcOuterCadenceApi));
        if(!api)return false;
        write_field(a,8,0x221c0d40u); // unnamed default context 2214d6f0
        o.removal_time=static_cast<float>(double(*c.now)+double(0.1f));
        ServerNpcOuterCadenceResult cadence{};
        if(!api->set_default(a,n,o.removal_time,*c.interval,scripted_sequence_vtable,c.cadence,&cadence))return false;
        o.removal_scheduled=1;o.due_tick=cadence.tick;
        write_field(a,0x3d8,static_cast<unsigned char>(0));write_field(a,0x3d9,static_cast<unsigned char>(1));
    }
    if(!b.cleanup_script_state(b.context,npc))return false;o.cleanup_called=1;
    // 221af490: preserve ideal state 7 or 1. Read the schedule twice
    // on the present branch, exactly as the original does.
    const unsigned flags=read_field<unsigned>(a,0x3c8);
    const int ideal=read_field<int>(npc,0x858);
    if(ideal!=7&&ideal!=1){write_field(npc,0x858,1);o.ideal_changed=1;}
    bool present=false;int type=0;
    if(!b.current_schedule(b.context,npc,&present,&type))return false;++o.schedule_reads;
    if(present){
        if(!b.current_schedule(b.context,npc,&present,&type)||!present)return false;++o.schedule_reads;
        if(type==0x6c&&!(flags&0x400u)){
            bool motion=false;
            if(!b.sequence_has_motion||!b.sequence_has_motion(b.context,npc,read_field<int>(npc,0x3ec),&motion))return false;
            o.motion_checked=1;
            if(motion){
                if(!b.add_flags||!b.add_flags(b.context,npc,0x400u))return false;o.flags_added=1;
            }
        }
    }
    if(!b.clear_schedule(b.context,npc))return false;o.schedule_cleared=1;
    o.successor_handle=read_field<unsigned>(a,0x3d4);
    void* next=registry.Resolve(o.successor_handle);
    if(!next)return true;
    o.successor_found=1;
    if(next==a&&!(read_field<unsigned>(a,0xb4)&4u)){o.self_skipped=1;return true;}
    unsigned extent=0,vt=0,handle=0;
    if(!b.possession||b.possession->entries!=c.entries||b.possession->entry_count!=c.entry_count||
       !c.extent(c.context,next,&extent)||extent<scripted_sequence_extent||
       !SinReadCampaignTouchMarkerInternal(next,&vt)||vt!=scripted_sequence_vtable||
       !SinCampaignStorageReadableInternal(next,scripted_sequence_extent)||
       !SinReadCampaignHandleInternal(npc,bytes,&handle)||registry.Resolve(handle)!=npc)return false;
    // Original cleanup clears current script before this transfer.
    // Refuse an incomplete owner cleanup instead of silently requeueing.
    if(registry.Resolve(read_field<unsigned>(npc,0x970)))return false;
    write_field(next,0x3d0,handle);
    if(!scripted_sequence_possession(next,extent,b.possession,&o.possession)||!o.possession.claimed)return false;
    o.transferred=1;return true;
}
int sequence_finish(void* a,unsigned n,void* npc,const ScriptedSequenceFinishBindings* b,ScriptedSequenceFinishResult* o) noexcept {
    if(!o)return 0;*o={};o->due_tick=-1;o->successor_handle=0xffffffffu;o->animation.sequence=o->animation.activity=-1;
    unsigned bytes=0;
    if(!b||!animation_valid(a,n,npc,b->animation,bytes))return 0;
    static thread_local bool active=false;if(active)return 0;
    struct Guard{bool& flag;~Guard(){flag=false;}} guard{active};active=true;
    o->completed=sequence_finish_impl(a,n,npc,bytes,*b,*o);return o->completed;
}
// 2203d0d0: networked playback rate setter, with same-value no-op.
bool scripted_rate(void* npc,float rate,const ScriptedNpcTaskBindings& b,ScriptedNpcTaskResult& o) noexcept {
    if(read_field<float>(npc,0x3c0)==rate)return true;
    if(read_field<unsigned char>(npc,0x1f0))write_field(npc,0x200,static_cast<unsigned char>(1));
    else if(const unsigned token=read_field<unsigned>(npc,0x16c)){
        void* p=nullptr;unsigned n=0;
        if(!b.rate_storage||!b.rate_storage(b.context,token,&p,&n)||n<4||!SinCampaignStorageReadableInternal(p,4))return false;
        write_field(p,0,read_field<unsigned>(p,0)|1u);
    }
    write_field(npc,0x3c0,rate);o.rate_changed=1;return true;
}
int run_scripted_npc_task(void* npc,unsigned bytes,int task,const ScriptedNpcTaskBindings* b,ScriptedNpcTaskResult* o) noexcept {
    if(!o)return 0;*o={};o->task=task;
    if(!b||!b->finish||!b->finish->animation||!b->finish->animation->control||
       !b->lifecycle||(task!=106&&task!=108&&task!=109&&task!=110)||bytes<0x9f4)return 0;
    const auto& finish=*b->finish;const auto& anim=*finish.animation;const auto& c=*anim.control;
    unsigned marker=0;
    if(!c.extent||!c.now||!std::isfinite(*c.now)||c.entry_count>sin_independent::kEntityHandleCapacity||(!c.entries&&c.entry_count)||
       !SinReadCampaignTouchMarkerInternal(npc,&marker)||!scripted_npc_marker(marker)||!SinCampaignStorageReadableInternal(npc,bytes))return 0;
    static thread_local bool active=false;if(active)return 0;
    struct Guard{bool& flag;~Guard(){flag=false;}} guard{active};active=true;
    const sin_independent::EntityHandleView registry(c.entries,c.entry_count);
    using namespace sin_port::native_weapon_tables;
    const auto* life=SiN_GetServerNpcTaskLifecycleApi(1,sizeof(ServerNpcTaskLifecycleApi));
    if(!life)return 0;
    auto complete=[&](){
        ServerNpcTaskLifecycleResult result{};
        if(!life->complete(npc,bytes,*c.now,false,b->lifecycle,&result))return false;
        o->task_completed=result.completed;o->status=result.status;return true;
    };
    auto resolved=[&](void*& seq,unsigned& n){
        seq=registry.Resolve(read_field<unsigned>(npc,0x970));n=0;
        if(!seq)return true;
        unsigned vt=0;
        return c.extent(c.context,seq,&n)&&n>=scripted_sequence_extent&&
            SinReadCampaignTouchMarkerInternal(seq,&vt)&&vt==scripted_sequence_vtable&&SinCampaignStorageReadableInternal(seq,scripted_sequence_extent);
    };
    auto done=[&](){o->completed=1;return 1;};
    auto wait=[&](){o->waiting=1;return done();};
    auto warn=[&](){o->missing=1;if(!b->warning||!b->warning(b->context,npc,task))return false;o->warned=1;return true;};
    void* seq=nullptr;unsigned n=0;
    if(task==108){
        // Original calls aim update even when the animation is unfinished.
        if(!b->update_aim||!b->update_aim(b->context,npc,0,0))return 0;o->aim_called=1;
        if(!read_field<unsigned char>(npc,0x3e0))return wait();
        // Original dereferences the script here; refuse stale/foreign storage.
        if(!resolved(seq,n)||!seq)return 0;
        if(!(read_field<unsigned>(seq,0x3c8)&0x400u)){
            bool motion=false;
            if(!finish.sequence_has_motion||!finish.sequence_has_motion(finish.context,npc,read_field<int>(npc,0x3ec),&motion))return 0;
            o->motion_checked=1;
            if(motion){
                if(!b->remove_flags||!b->remove_flags(b->context,npc,0x400u))return 0;o->flags_removed=1;
            }
        }
        if(!resolved(seq,n))return 0;
        if(seq&&!run_sequence_animation(seq,n,npc,0,false,true,&anim,&o->animation,&finish,&o->terminal))return 0;
        return complete()?done():0;
    }
    if(!resolved(seq,n))return 0;
    if(!seq){
        if(!warn())return 0;
        if(task==106)return complete()?done():0;
        if(!b->task_fail||!b->task_fail(b->context,npc,5))return 0;o->task_failed=1;return done();
    }
    if(task==106){
        if(read_field<unsigned>(seq,0x3b8))return wait();
        if(!complete()||!resolved(seq,n)||!seq||!anim.events||!anim.events->output)return 0;
        // 221adf70 OnBeginSequence, self caller and activator.
        if(anim.events->native_queue){
            if(!native_named_output(seq,n,seq,0x3e0,false,anim.events->output,anim.events->native_queue,o->begin_queued))return 0;
            o->begin_native_queued=o->begin_queued;
        }else{
            const auto* fire=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
            ServerEntityVoidOutputResult r{};
            if(!fire||!fire->fire(static_cast<unsigned char*>(seq)+0x3e0,0x18,seq,seq,n,0.f,anim.events->output,&r))return 0;
            o->begin_queued=r.queued;
        }
        o->begin_output=1;
        if(!resolved(seq,n)||!seq||
           !run_sequence_animation(seq,n,npc,read_field<unsigned>(seq,0x398),true,false,&anim,&o->animation,&finish,&o->terminal))return 0;
        if(!resolved(seq,n))return 0;
        if(!seq){
            if(!finish.clear_schedule||!finish.clear_schedule(finish.context,npc))return 0;o->schedule_cleared=1;
        }
        return scripted_rate(npc,1.f,*b,*o)?done():0;
    }
    if(task==109){
        if(!read_field<unsigned char>(npc,0x3e0)&&!registry.Resolve(read_field<unsigned>(seq,0x3d4)))return wait();
        // No TaskComplete on the original terminal branch.
        return sequence_finish(seq,n,npc,&finish,&o->terminal)?done():0;
    }
    if(read_field<unsigned char>(seq,0x3c0))return wait();
    if(!finish.possession||finish.possession->entries!=c.entries||finish.possession->entry_count!=c.entry_count||finish.possession->now!=c.now)return 0;
    ScriptedSequencePossessionResult sync{};
    if(!sequence_sync(seq,*finish.possession,sync))return 0;
    o->synchronized=1;o->group_updates=sync.group_updates;
    return complete()?done():0;
}
// Original StartTask cases 106, 109, 110. 110's missing sequence
// dereference is not synthesized; the independent owner must supply it.
int start_scripted_npc_task(void* npc,unsigned bytes,int task,const ScriptedNpcStartTaskBindings* b,ScriptedNpcStartTaskResult* o) noexcept {
    if(!o)return 0;*o={};o->task=task;
    if(!b||!b->task||!b->task->finish||!b->task->finish->animation||!b->task->finish->animation->control||
       (task!=106&&task!=109&&task!=110)||bytes<0x974)return 0;
    const auto& route=*b->task;const auto& anim=*route.finish->animation;const auto& c=*anim.control;
    unsigned marker=0;
    if(!SinReadCampaignTouchMarkerInternal(npc,&marker)||!scripted_npc_marker(marker)||!SinCampaignStorageReadableInternal(npc,bytes))return 0;
    static thread_local bool active=false;if(active)return 0;
    struct Guard{bool& flag;~Guard(){flag=false;}} guard{active};active=true;
    auto done=[&](){o->completed=1;return 1;};
    if(task==109){write_field(npc,0x96c,2u);o->state_written=1;return done();}
    if(!c.extent||c.entry_count>sin_independent::kEntityHandleCapacity||(!c.entries&&c.entry_count))return 0;
    const sin_independent::EntityHandleView registry(c.entries,c.entry_count);
    auto resolved=[&](void*& seq,unsigned& n){
        seq=registry.Resolve(read_field<unsigned>(npc,0x970));n=0;
        if(!seq)return true;
        unsigned vt=0;
        return c.extent(c.context,seq,&n)&&n>=scripted_sequence_extent&&
            SinReadCampaignTouchMarkerInternal(seq,&vt)&&vt==scripted_sequence_vtable&&SinCampaignStorageReadableInternal(seq,scripted_sequence_extent);
    };
    void* seq=nullptr;unsigned n=0;if(!resolved(seq,n))return 0;
    if(!seq){
        o->missing=1;
        if(task!=106||!route.warning||!route.task_fail||!route.warning(route.context,npc,task))return 0;
        o->warned=1;if(!route.task_fail(route.context,npc,5))return 0;o->task_failed=1;return done();
    }
    if(task==106)return done(); // valid handle: no TaskComplete
    const unsigned idle=read_field<unsigned>(seq,0x394);
    if(idle){
        if(!run_sequence_animation(seq,n,npc,idle,false,false,&anim,&o->animation))return 0;o->pre_idle=1;
        if(!resolved(seq,n)||!seq)return 0;
        const unsigned live_idle=read_field<unsigned>(seq,0x394),play=read_field<unsigned>(seq,0x398);
        bool same=false;
        if(!b->equal_names||!b->equal_names(b->context,live_idle?live_idle:0x223d47a8u,play?play:0x223d47a8u,&same))return 0;
        o->compared=1;o->same_name=same;
        if(same){ScriptedNpcTaskResult rate{};if(!scripted_rate(npc,0.f,route,rate))return 0;o->rate_changed=rate.rate_changed;}
    }else if(read_field<int>(npc,0x96c)!=6){
        const int pending=read_field<int>(npc,0x880);
        if(pending==6||pending==7||pending==10||pending==11){
            if(!b->request_activity||!b->request_activity(b->context,npc,1))return 0;o->requested_idle=1;
        }
    }
    return done();
}
// 2216b300 InputTestActivator: predicate (including inherited negation),
// exactly one output, original activator and self caller, zero extra delay.
int campaign_filter_test_activator(void* a,unsigned n,void* activator,const CampaignFilterInputBindings* b,CampaignFilterInputResult* o) noexcept {
    if(!o)return 0;*o={};
    unsigned marker=0;
    if(!b||!b->predicate||!b->output||n<4||!SinReadCampaignTouchMarkerInternal(a,&marker)||!campaign_filter_class(marker))return 0;
    const unsigned extent=marker==base_filter_vtable?0x3c8u:marker==multi_filter_vtable?0x3f4u:0x3ccu;
    if(n<extent||!SinCampaignStorageReadableInternal(a,extent))return 0;
    static thread_local bool active=false;if(active)return 0;
    struct Guard{bool& flag;~Guard(){flag=false;}} guard{active};active=true;
    if(!campaign_filter_entity(a,activator,b->predicate,&o->predicate))return 0;
    const unsigned off=o->predicate.accepted?0x398:0x3b0;
    using namespace sin_port::native_weapon_tables;
    if(b->native_queue){
        if(!native_named_output(a,n,activator,off,false,b->output,b->native_queue,o->queued))return 0;
        o->native_queued=o->queued;
    }else{
        const auto* fire=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
        ServerEntityVoidOutputResult fired{};
        if(!fire||!fire->fire(static_cast<unsigned char*>(a)+off,0x18,activator,a,n,0.f,b->output,&fired))return 0;
        o->queued=fired.queued;
    }
    if(o->predicate.accepted)o->pass_output=1;else o->fail_output=1;
    o->completed=1;return 1;
}
// 221ce520: broad eligibility OR, followed by ordered restrictive
// conditions and the optional full-serial custom filter lookup.
int campaign_filter(const void* a,unsigned n,void* other,unsigned other_bytes,
    const CampaignFilterBindings* b,CampaignFilterResult* o) noexcept {
    if(!o)return 0;*o={};o->filter_handle=0xffffffffu;
    unsigned marker=0,vt=0;
    if(!b||b->entry_count>sin_independent::kEntityHandleCapacity||(!b->entries&&b->entry_count)||
       n<4||!SinReadCampaignTouchMarkerInternal(a,&marker)||other_bytes<4||!SinReadCampaignTouchMarkerInternal(other,&vt))return 0;
    const unsigned extent=marker==dynamic_trigger_vtable?dynamic_trigger_extent:marker==secret_trigger_vtable?secret_trigger_extent:0;
    if(!extent||n<extent||!SinCampaignStorageReadableInternal(a,extent))return 0;
    auto done=[&](bool accepted){o->accepted=accepted;o->completed=1;return 1;};
    auto readable=[&](unsigned need){return other_bytes>=need&&SinCampaignStorageReadableInternal(other,need);};
    const unsigned flags=read_field<unsigned>(a,0xb4);
    if(flags&64u)o->eligibility=64;
    else if(flags&1u){
        if(!readable(0xbd))return 0;
        if(read_field<unsigned char>(other,0xbc)&0x80u)o->eligibility=1;
    }
    if(!o->eligibility&&(flags&2u)){
        if(!readable(0xbe))return 0;
        if(read_field<unsigned char>(other,0xbd)&0x20u)o->eligibility=2;
    }
    if(!o->eligibility&&(flags&4u)){
        const char* name=nullptr;unsigned bytes=0;
        if(!b->classname||!b->classname(b->context,other,&name,&bytes))return 0;
        if(name){
            if(!bytes||bytes>4096||!SinCampaignStorageReadableInternal(name,bytes)||!std::memchr(name,0,bytes))return 0;
            if(equal_class(name,"func_pushable"))o->eligibility=4;
        }
    }
    if(!o->eligibility&&(read_field<unsigned>(a,0xb4)&8u)){
        if(!readable(0x208))return 0;
        if(read_field<unsigned>(other,0x204)==6)o->eligibility=8;
    }
    if(!o->eligibility)return done(false);
    // Always perform IsPlayer after broad eligibility, before restrictions.
    bool player=false;o->player_read=1;
    const auto classification=campaign_player_marker(vt);
    if(classification==PlayerPredicate::Player)player=true;
    else if(classification==PlayerPredicate::Unknown&&(!b->is_player||!b->is_player(b->context,other,&player)))return 0;
    o->player=player;
    if((read_field<unsigned>(a,0xb4)&16u)&&!player){
        bool condition=false;o->npc_read=1;
        // All 18 reviewed primary layouts have the same false +f4 leaf.
        // Foreign actors need the owner's complete +f4/+53c condition.
        if(classification==PlayerPredicate::Unknown&&(!b->npc_condition||!b->npc_condition(b->context,other,&condition)))return 0;
        if(!condition)return done(false);
    }
    const unsigned restrictions=read_field<unsigned>(a,0xb4);
    const sin_independent::EntityHandleView registry(b->entries,b->entry_count);
    auto vehicle=[&](bool& present){
        ++o->vehicle_reads;
        if(vt==0x224aec3cu||vt==0x2245222cu){
            if(!readable(0x9b0))return false;
            present=registry.Resolve(read_field<unsigned>(other,0x9ac))!=nullptr;return true;
        }
        return b->player_vehicle&&b->player_vehicle(b->context,other,&present);
    };
    if(player&&(restrictions&32u)){bool present=false;if(!vehicle(present))return 0;if(!present)return done(false);}
    if(player&&(restrictions&512u)){bool present=false;if(!vehicle(present))return 0;if(present)return done(false);}
    o->filter_handle=read_field<unsigned>(a,0x410);
    if(auto* filter=registry.Resolve(o->filter_handle)){
        bool accepted=false;o->custom_called=1;
        if(!b->custom_filter||!b->custom_filter(b->context,filter,other,&accepted))return 0;
        return done(accepted);
    }
    return done(true);
}
// 221d0df0: filter, handle, append to +45c vector, then
// COutputEvent::FireOutput(+414, other, this, 0). Preserve duplicate rows.
int __cdecl campaign_start_output(void* a,unsigned n,void* other,unsigned other_bytes,
    const CampaignStartTouchBindings* b,CampaignStartTouchResult* o,
    const CampaignFilterBindings* native_filter=nullptr,CampaignFilterResult* filter_result=nullptr) noexcept {
    if(!o)return 0;*o={};if(filter_result)*filter_result={};
    unsigned marker=0;
    if(!b||(!native_filter&&!b->filter)||n<4||!SinReadCampaignTouchMarkerInternal(a,&marker))return 0;
    const unsigned extent=marker==dynamic_trigger_vtable?dynamic_trigger_extent:marker==secret_trigger_vtable?secret_trigger_extent:0;
    if(!extent||n<extent)return 0;
    static thread_local bool active=false;if(active)return 0;
    struct Guard{bool& flag;~Guard(){flag=false;}} guard{active};active=true;
    bool accepted=false;o->filter_called=1;
    if(native_filter){
        if(!filter_result||!campaign_filter(a,n,other,other_bytes,native_filter,filter_result))return 0;
        accepted=filter_result->accepted!=0;
    }else if(!b->filter(b->context,a,other,&accepted))return 0;
    if(!accepted){o->completed=1;return 1;}o->accepted=1;
    unsigned handle=0xffffffffu;
    if(other){
        unsigned vt=0;
        if(other_bytes<4||!SinReadCampaignTouchMarkerInternal(other,&vt))return 0;
        if(campaign_player_marker(vt)!=PlayerPredicate::Unknown){
            if(!SinReadCampaignHandleInternal(other,other_bytes,&handle))return 0;
        }else if(!b->foreign_handle||!b->foreign_handle(b->context,other,&handle))return 0;
    }
    o->handle=handle;
    const int size=read_field<int>(a,0x468);int capacity=read_field<int>(a,0x460);
    unsigned token=read_field<unsigned>(a,0x45c);
    // Signed original count/capacity and bounded independent storage.
    if(size<0||capacity<0||size>capacity||capacity>4096||(capacity&&!token)||size==4096||!b->output)return 0;
    o->prior_count=o->count=static_cast<unsigned>(size);
    if(size==capacity){
        if(read_field<unsigned>(a,0x464)==0xffffffffu||!b->grow_vector||
           !b->grow_vector(b->context,a,static_cast<unsigned>(size+1)))return 0;
        o->grew=1;
        token=read_field<unsigned>(a,0x45c);capacity=read_field<int>(a,0x460);
        if(read_field<int>(a,0x468)!=size||capacity<=size||capacity>4096||!token)return 0;
    }
    void* vector=nullptr;unsigned bytes=0;
    if(!b->resolve_vector||!b->resolve_vector(b->context,token,&vector,&bytes)||
       !vector||bytes<static_cast<unsigned>(capacity)*4u)return 0;
    const auto vb=reinterpret_cast<std::uintptr_t>(vector),ab=reinterpret_cast<std::uintptr_t>(a);
    if(vb+bytes<vb||ab+n<ab||(vb<ab+n&&ab<vb+bytes))return 0;
    write_field(a,0x468,size+1);write_field(a,0x46c,token);
    write_field(vector,static_cast<unsigned>(size)*4u,handle);
    o->appended=1;o->count=static_cast<unsigned>(size+1);
    using namespace sin_port::native_weapon_tables;
    if(b->native_queue){
        if(!native_named_output(a,n,other,0x414,false,b->output,b->native_queue,o->queued))return 0;
        o->native_queued=o->queued;
    }else{
        const auto* fire=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
        ServerEntityVoidOutputResult fired{};
        if(!fire||!fire->fire(static_cast<unsigned char*>(a)+0x414,0x18,other,a,n,0.f,b->output,&fired))return 0;
        o->queued=fired.queued;
    }
    o->output_fired=o->completed=1;return 1;
}
// 221d3590: find by resolved identity twice, remove one row,
// fire OnEndTouch, then prune stale handles in reverse and fire
// OnEndTouchAll only when no live contact remains.
int __cdecl campaign_end_output(void* a,unsigned n,void* other,unsigned other_bytes,
    const CampaignEndTouchBindings* b,CampaignEndTouchResult* o) noexcept {
    if(!o)return 0;*o={};
    unsigned marker=0;
    if(!b||b->entry_count>sin_independent::kEntityHandleCapacity||(!b->entries&&b->entry_count)||
       !SinReadCampaignTouchMarkerInternal(a,&marker))return 0;
    const unsigned extent=marker==dynamic_trigger_vtable?dynamic_trigger_extent:marker==secret_trigger_vtable?secret_trigger_extent:0;
    if(!extent||n<extent)return 0;
    static thread_local bool active=false;if(active)return 0;
    struct Guard{bool& flag;~Guard(){flag=false;}} guard{active};active=true;
    const sin_independent::EntityHandleView registry(b->entries,b->entry_count);
    auto handle=[&](unsigned& h){
        ++o->handle_reads;h=0xffffffffu;
        if(!other)return true;
        unsigned vt=0;if(other_bytes<4||!SinReadCampaignTouchMarkerInternal(other,&vt))return false;
        if(campaign_player_marker(vt)!=PlayerPredicate::Unknown)return SinReadCampaignHandleInternal(other,other_bytes,&h);
        return b->foreign_handle&&b->foreign_handle(b->context,other,&h);
    };
    void* vector=nullptr;int size=0;
    auto load=[&](){
        vector=nullptr;size=read_field<int>(a,0x468);const int cap=read_field<int>(a,0x460);
        const unsigned token=read_field<unsigned>(a,0x45c);
        if(size<0||cap<0||size>cap||cap>4096||(cap&&!token))return false;
        o->count=static_cast<unsigned>(size);
        if(!size)return true;
        unsigned bytes=0;
        if(!b->resolve_vector||!b->resolve_vector(b->context,token,&vector,&bytes)||
           !vector||bytes<static_cast<unsigned>(cap)*4u)return false;
        const auto vb=reinterpret_cast<std::uintptr_t>(vector),ab=reinterpret_cast<std::uintptr_t>(a);
        return vb+bytes>=vb&&ab+n>=ab&&!(vb<ab+n&&ab<vb+bytes);
    };
    auto find=[&](unsigned h){
        auto* target=registry.Resolve(h);
        for(int i=0;i<size;++i)
            if(registry.Resolve(read_field<unsigned>(vector,static_cast<unsigned>(i)*4u))==target)return i;
        return -1;
    };
    auto erase=[&](int i){
        auto* row=static_cast<unsigned char*>(vector)+static_cast<unsigned>(i)*4u;
        if(size-i>1)std::memmove(row,row+4,static_cast<unsigned>(size-i-1)*4u);
        --size;write_field(a,0x468,size);o->count=static_cast<unsigned>(size);
    };
    unsigned h=0;
    if(!handle(h)||!load())return 0;
    if(find(h)<0){o->completed=1;return 1;}
    o->matched=1;
    if(!b->output||!handle(h)||!load())return 0;
    const int second=find(h);
    if(second>=0){erase(second);o->removed=1;}
    auto fire_output=[&](unsigned off){
        unsigned queued=0;
        using namespace sin_port::native_weapon_tables;
        if(b->native_queue){
            if(!native_named_output(a,n,other,off,false,b->output,b->native_queue,queued))return false;
            o->native_queued+=queued;
        }else{
            const auto* fire=SiN_GetServerEntityVoidOutputApi(1,sizeof(ServerEntityVoidOutputApi));
            ServerEntityVoidOutputResult fired{};
            if(!fire||!fire->fire(static_cast<unsigned char*>(a)+off,0x18,other,a,n,0.f,b->output,&fired))return false;
            queued=fired.queued;
        }
        o->queued+=queued;return true;
    };
    if(!fire_output(0x42c))return 0;o->end_output=1;
    // Original takes a fresh count after the output; do not cache the
    // old allocation across owner callbacks.
    if(!load())return 0;
    for(int i=size-1;i>=0;--i){
        if(registry.Resolve(read_field<unsigned>(vector,static_cast<unsigned>(i)*4u)))++o->live_remaining;
        else{erase(i);++o->stale_removed;}
    }
    if(!o->live_remaining){if(!fire_output(0x444))return 0;o->all_output=1;}
    o->completed=1;return 1;
}
const sin_port::native_map::Api api{1,sizeof(sin_port::native_map::Api),&token,&slice,&lookup,&parse,&register_factories,&find_factory,&create_factory,&spawn_ordinary,&dispatch_spawn,&key_value,&parse_all,&relay_input,&register_relay,&destroy_relay,&auto_activate,&auto_think,&register_auto,&destroy_auto,&auto_primary_activate,&auto_primary_run,&timer_spawn,&timer_input,&register_timer,&destroy_timer,&timer_primary_run,&branch_input,&register_branch,&destroy_branch,&case_input,&register_case,&destroy_case,&compare_input,&register_compare,&destroy_compare,&counter_spawn,&counter_input,&register_counter,&destroy_counter,&destroy_logic,&menu_listen,&menu_event,&register_menu,&destroy_menu,&navigation_input,&navigation_activate,&navigation_created,&navigation_remove,&register_navigation,&destroy_navigation,&register_autosave,&destroy_autosave,&autosave_input,&autosave_kill,&multicompare_input,&register_multicompare,&destroy_multicompare,&multicompare_kill,&remap_spawn,&remap_input,&register_remap,&destroy_remap,&colorblend_spawn,&colorblend_input,&register_colorblend,&destroy_colorblend,&logic_kill,&defer_logic_remove,&cleanup_deferred,&discard_deferred,&registry_unlink_logic,&notify_deleting,&register_campaign_triggers,&destroy_campaign_trigger,&campaign_trigger_spawn,&campaign_trigger_touch,&destroy_trigger_members,&trigger_control,&trigger_kill};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerMapEntityApi=_SiN_GetServerMapEntityApi")
extern "C" __declspec(dllexport) const sin_port::native_map::Api* __cdecl SiN_GetServerMapEntityApi(std::uint32_t v,std::uint32_t n) noexcept {
    using namespace sin_port::native_map;return v==1&&n==sizeof(sin_port::native_map::Api)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_SpawnCampaignTriggerWithEngineEvents=_SiN_SpawnCampaignTriggerWithEngineEvents")
extern "C" int __cdecl SiN_SpawnCampaignTriggerWithEngineEvents(void* a,unsigned n,
    const sin_port::native_map::CampaignTriggerBindings* b,void* manager,
    sin_port::native_map::CampaignTriggerResult* o,int* publication) noexcept {
    if(publication)*publication=sin_port::native_map::engine_event_not_attempted;
    if(o)*o={};
    if(!manager||!publication)return 0;
    return sin_port::native_map::campaign_trigger_spawn_impl(a,n,b,o,manager,publication);
}
#pragma comment(linker,"/EXPORT:SiN_TouchCampaignTriggerWithEngineEvents=_SiN_TouchCampaignTriggerWithEngineEvents")
extern "C" int __cdecl SiN_TouchCampaignTriggerWithEngineEvents(void* a,unsigned n,void* other,
    const sin_port::native_map::CampaignTriggerBindings* b,void* manager,
    sin_port::native_map::CampaignTriggerResult* o,int* publication) noexcept {
    if(publication)*publication=sin_port::native_map::engine_event_not_attempted;
    if(o)*o={};
    if(!manager||!publication)return 0;
    return sin_port::native_map::campaign_trigger_touch_impl(a,n,other,b,o,manager,publication);
}

int __cdecl SinParseMapWithEventsInternal(const char* text,unsigned bytes,bool activate,
    const sin_port::native_map::PipelineBindings* b,sin_port::native_map::PipelineResult* o,void* manager) noexcept {
    return manager&&sin_port::native_map::parse_all_impl(text,bytes,activate,b,o,manager);
}

int __cdecl SinTouchCampaignTriggerFromStartupInternal(void* a,unsigned n,void* other,
    const sin_port::native_map::CampaignTriggerBindings* b,void* manager,
    sin_port::native_map::CampaignTriggerResult* o,int* publication,void* globals,void* engine) noexcept {
    if(!manager||!publication||!globals||!engine)return 0;
    return sin_port::native_map::campaign_trigger_touch_impl(a,n,other,b,o,manager,publication,globals,engine);
}

#pragma comment(linker,"/EXPORT:SiN_ClassifyCampaignTouchPlayer=_SiN_ClassifyCampaignTouchPlayer")
extern "C" sin_port::native_map::PlayerPredicate __cdecl SiN_ClassifyCampaignTouchPlayer(const void* actor,unsigned bytes) noexcept {
    using namespace sin_port::native_map;
    unsigned marker=0;
    if(bytes<4||!SinReadCampaignTouchMarkerInternal(actor,&marker))return PlayerPredicate::Invalid;
    return campaign_player_marker(marker);
}

#pragma comment(linker,"/EXPORT:SiN_StartCampaignTriggerOutput=_SiN_StartCampaignTriggerOutput")
extern "C" int __cdecl SiN_StartCampaignTriggerOutput(void* a,unsigned n,void* other,unsigned other_bytes,
    const sin_port::native_map::CampaignStartTouchBindings* b,sin_port::native_map::CampaignStartTouchResult* o) noexcept {
    return sin_port::native_map::campaign_start_output(a,n,other,other_bytes,b,o);
}

#pragma comment(linker,"/EXPORT:SiN_EndCampaignTriggerOutput=_SiN_EndCampaignTriggerOutput")
extern "C" int __cdecl SiN_EndCampaignTriggerOutput(void* a,unsigned n,void* other,unsigned other_bytes,
    const sin_port::native_map::CampaignEndTouchBindings* b,sin_port::native_map::CampaignEndTouchResult* o) noexcept {
    return sin_port::native_map::campaign_end_output(a,n,other,other_bytes,b,o);
}

#pragma comment(linker,"/EXPORT:SiN_TestCampaignTriggerFilter=_SiN_TestCampaignTriggerFilter")
extern "C" int __cdecl SiN_TestCampaignTriggerFilter(const void* a,unsigned n,void* other,unsigned other_bytes,
    const sin_port::native_map::CampaignFilterBindings* b,sin_port::native_map::CampaignFilterResult* o) noexcept {
    return sin_port::native_map::campaign_filter(a,n,other,other_bytes,b,o);
}
#pragma comment(linker,"/EXPORT:SiN_StartCampaignTriggerOutputWithFilter=_SiN_StartCampaignTriggerOutputWithFilter")
extern "C" int __cdecl SiN_StartCampaignTriggerOutputWithFilter(void* a,unsigned n,void* other,unsigned other_bytes,
    const sin_port::native_map::CampaignStartTouchBindings* b,sin_port::native_map::CampaignStartTouchResult* o,
    const sin_port::native_map::CampaignFilterBindings* filter,sin_port::native_map::CampaignFilterResult* fr) noexcept {
    if(!filter||!fr){if(o)*o={};return 0;}
    return sin_port::native_map::campaign_start_output(a,n,other,other_bytes,b,o,filter,fr);
}

#pragma comment(linker,"/EXPORT:SiN_TestCampaignFilterEntity=_SiN_TestCampaignFilterEntity")
extern "C" int __cdecl SiN_TestCampaignFilterEntity(void* filter,void* other,
    const sin_port::native_map::CampaignCustomFilterBindings* b,sin_port::native_map::CampaignCustomFilterResult* o) noexcept {
    return sin_port::native_map::campaign_filter_entity(filter,other,b,o);
}
#pragma comment(linker,"/EXPORT:SiN_CampaignFilterEntityCallback=_SiN_CampaignFilterEntityCallback")
extern "C" int __cdecl SiN_CampaignFilterEntityCallback(void* context,void* filter,void* other,bool* accepted) noexcept {
    if(!accepted)return 0;*accepted=false;
    sin_port::native_map::CampaignCustomFilterResult result{};
    if(!sin_port::native_map::campaign_filter_entity(filter,other,
        static_cast<const sin_port::native_map::CampaignCustomFilterBindings*>(context),&result))return 0;
    *accepted=result.accepted!=0;return 1;
}

#pragma comment(linker,"/EXPORT:SiN_ActivateCampaignFilter=_SiN_ActivateCampaignFilter")
extern "C" int __cdecl SiN_ActivateCampaignFilter(void* actor,unsigned bytes,
    const sin_port::native_map::CampaignFilterActivationBindings* b,sin_port::native_map::CampaignFilterActivationResult* o) noexcept {
    return sin_port::native_map::campaign_filter_activate(actor,bytes,b,o);
}

#pragma comment(linker,"/EXPORT:SiN_CreateCampaignFilterEntity=_SiN_CreateCampaignFilterEntity")
extern "C" int __cdecl SiN_CreateCampaignFilterEntity(const char* name,const sin_port::native_map::CampaignFilterFactoryBindings* b,
    sin_port::native_map::FactoryResult* o) noexcept {
    return sin_port::native_map::create_filter_factory(name,b,o);
}
#pragma comment(linker,"/EXPORT:SiN_CreateCampaignFilterForMap=_SiN_CreateCampaignFilterForMap")
extern "C" int __cdecl SiN_CreateCampaignFilterForMap(void* context,const char* name,void** actor) noexcept {
    if(!actor)return 0;*actor=nullptr;
    auto* b=static_cast<const sin_port::native_map::CampaignFilterFactoryBindings*>(context);
    sin_port::native_map::FactoryResult r{};
    if(!sin_port::native_map::create_filter_factory(name,b,&r))return 0;
    if(r.matched){*actor=r.actor;return 1;}
    return b->fallback_create&&b->fallback_create(b->fallback_context,name,actor);
}

#pragma comment(linker,"/EXPORT:SiN_CampaignFilterTestActivator=_SiN_CampaignFilterTestActivator")
extern "C" int __cdecl SiN_CampaignFilterTestActivator(void* actor,unsigned bytes,void* activator,
    const sin_port::native_map::CampaignFilterInputBindings* b,sin_port::native_map::CampaignFilterInputResult* o) noexcept {
    return sin_port::native_map::campaign_filter_test_activator(actor,bytes,activator,b,o);
}

#pragma comment(linker,"/EXPORT:SiN_DestroyCampaignFilterEntity=_SiN_DestroyCampaignFilterEntity")
extern "C" int __cdecl SiN_DestroyCampaignFilterEntity(void* actor,unsigned bytes,void* networkable,
    const sin_port::native_map::RelayFactoryBindings* owner) noexcept {
    using namespace sin_port::native_map;
    if(!networkable)return 1;
    unsigned marker=0;
    if(!owner||!owner->release_networkable||bytes<4||!SinReadCampaignTouchMarkerInternal(actor,&marker))return 0;
    const unsigned extent=campaign_filter_extent(marker);
    if(!extent||bytes<extent||!SinCampaignStorageReadableInternal(actor,extent)||
        networkable!=static_cast<unsigned char*>(actor)+0x160)return 0;
    // Factory Destroy calls networkable +18; no actor read afterwards.
    return owner->release_networkable(owner->context,networkable);
}

int __cdecl SinParseMapWithFiltersInternal(const char* text,unsigned bytes,bool activate,
    const sin_port::native_map::PipelineBindings* b,sin_port::native_map::PipelineResult* o,void* manager,
    const sin_port::native_map::CampaignFilterFactoryBindings* filters) noexcept {
    return manager&&filters&&sin_port::native_map::parse_all_impl(text,bytes,activate,b,o,manager,filters);
}

#pragma comment(linker,"/EXPORT:SiN_CreateScriptedSequence=_SiN_CreateScriptedSequence")
extern "C" int __cdecl SiN_CreateScriptedSequence(const sin_port::native_map::RelayFactoryBindings* owner,sin_port::native_map::FactoryResult* o) noexcept {
    return sin_port::native_map::create_scripted_sequence(owner,o);
}
#pragma comment(linker,"/EXPORT:SiN_FireScriptedSequenceEvent=_SiN_FireScriptedSequenceEvent")
extern "C" int __cdecl SiN_FireScriptedSequenceEvent(void* a,unsigned n,int index,const sin_port::native_map::ScriptedSequenceEventBindings* b,
    sin_port::native_map::ScriptedSequenceEventResult* o) noexcept {
    return sin_port::native_map::scripted_sequence_event(a,n,index,b,o);
}

#pragma comment(linker,"/EXPORT:SiN_ControlScriptedSequence=_SiN_ControlScriptedSequence")
extern "C" int __cdecl SiN_ControlScriptedSequence(void* actor,unsigned bytes,sin_port::native_map::ScriptedSequenceInput input,
    const sin_port::native_map::ScriptedSequenceControlBindings* b,sin_port::native_map::ScriptedSequenceControlResult* o) noexcept {
    return sin_port::native_map::scripted_sequence_control(actor,bytes,input,b,o);
}

#pragma comment(linker,"/EXPORT:SiN_PossessScriptedSequence=_SiN_PossessScriptedSequence")
extern "C" int __cdecl SiN_PossessScriptedSequence(void* actor,unsigned bytes,const sin_port::native_map::ScriptedSequencePossessionBindings* b,
    sin_port::native_map::ScriptedSequencePossessionResult* o) noexcept {
    return sin_port::native_map::scripted_sequence_possession(actor,bytes,b,o);
}
#pragma comment(linker,"/EXPORT:SiN_PossessScriptedSequenceCallback=_SiN_PossessScriptedSequenceCallback")
extern "C" int __cdecl SiN_PossessScriptedSequenceCallback(void* context,void* actor) noexcept {
    const auto* b=static_cast<const sin_port::native_map::ScriptedSequencePossessionBindings*>(context);
    unsigned bytes=0;sin_port::native_map::ScriptedSequencePossessionResult o{};
    return b&&b->extent&&b->extent(b->context,actor,&bytes)&&sin_port::native_map::scripted_sequence_possession(actor,bytes,b,&o);
}

#pragma comment(linker,"/EXPORT:SiN_AcquireScriptedSequenceNpc=_SiN_AcquireScriptedSequenceNpc")
extern "C" int __cdecl SiN_AcquireScriptedSequenceNpc(void* actor,unsigned bytes,const sin_port::native_map::ScriptedSequenceAcquireBindings* b,
    sin_port::native_map::ScriptedSequenceAcquireResult* o) noexcept {
    return sin_port::native_map::scripted_sequence_acquire(actor,bytes,b,o);
}

#pragma comment(linker,"/EXPORT:SiN_CancelScriptedSequenceNpc=_SiN_CancelScriptedSequenceNpc")
extern "C" int __cdecl SiN_CancelScriptedSequenceNpc(void* actor,unsigned bytes,const sin_port::native_map::ScriptedSequenceCancelBindings* b,
    sin_port::native_map::ScriptedSequenceCancelResult* o) noexcept {
    return sin_port::native_map::scripted_sequence_cancel(actor,bytes,b,o);
}
#pragma comment(linker,"/EXPORT:SiN_RunScriptedSequenceAcquisitionThink=_SiN_RunScriptedSequenceAcquisitionThink")
extern "C" int __cdecl SiN_RunScriptedSequenceAcquisitionThink(void* actor,unsigned bytes,const sin_port::native_map::ScriptedSequenceThinkBindings* b,
    sin_port::native_map::ScriptedSequenceThinkResult* o) noexcept {
    return sin_port::native_map::scripted_sequence_think(actor,bytes,b,o);
}

#pragma comment(linker,"/EXPORT:SiN_ControlScriptedSequenceNative=_SiN_ControlScriptedSequenceNative")
extern "C" int __cdecl SiN_ControlScriptedSequenceNative(void* actor,unsigned bytes,sin_port::native_map::ScriptedSequenceInput input,
    const sin_port::native_map::ScriptedSequenceNativeControlBindings* b,sin_port::native_map::ScriptedSequenceControlResult* o) noexcept {
    if(o)*o={};
    return b&&sin_port::native_map::scripted_sequence_control(actor,bytes,input,b->control,o,b);
}

#pragma comment(linker,"/EXPORT:SiN_RunScriptedSequencePrimaryThink=_SiN_RunScriptedSequencePrimaryThink")
extern "C" int __cdecl SiN_RunScriptedSequencePrimaryThink(void* actor,unsigned bytes,int tick,const sin_port::native_map::ScriptedSequencePrimaryBindings* b,
    sin_port::native_map::ScriptedSequencePrimaryResult* o) noexcept {
    return sin_port::native_map::sequence_primary(actor,bytes,tick,b,o);
}

#pragma comment(linker,"/EXPORT:SiN_SpawnScriptedSequence=_SiN_SpawnScriptedSequence")
extern "C" int __cdecl SiN_SpawnScriptedSequence(void* actor,unsigned bytes,const sin_port::native_map::ScriptedSequenceSpawnBindings* b,
    sin_port::native_map::ScriptedSequenceSpawnResult* o) noexcept {
    return sin_port::native_map::scripted_sequence_spawn(actor,bytes,b,o);
}
#pragma comment(linker,"/EXPORT:SiN_ActivateScriptedSequence=_SiN_ActivateScriptedSequence")
extern "C" int __cdecl SiN_ActivateScriptedSequence(void* actor,unsigned bytes,const sin_port::native_map::ScriptedSequenceActivateBindings* b,
    sin_port::native_map::ScriptedSequenceActivateResult* o) noexcept {
    return sin_port::native_map::scripted_sequence_activate(actor,bytes,b,o);
}

int __cdecl SinParseMapWithSequencesInternal(const char* text,unsigned bytes,
    const sin_port::native_map::PipelineBindings* b,sin_port::native_map::PipelineResult* o,void* manager,
    const sin_port::native_map::ScriptedSequenceMapBindings* sequences) noexcept {
    return sequences&&sin_port::native_map::parse_all_impl(text,bytes,false,b,o,manager,sequences->filters,sequences);
}

#pragma comment(linker,"/EXPORT:SiN_DestroyScriptedSequenceEntity=_SiN_DestroyScriptedSequenceEntity")
extern "C" int __cdecl SiN_DestroyScriptedSequenceEntity(void* actor,unsigned bytes,void* networkable,
    const sin_port::native_map::RelayFactoryBindings* owner) noexcept {
    using namespace sin_port::native_map;
    if(!networkable)return 1;
    unsigned marker=0;
    if(!owner||!owner->release_networkable||bytes<scripted_sequence_extent||
       !SinReadCampaignTouchMarkerInternal(actor,&marker)||marker!=scripted_sequence_vtable||
       !SinCampaignStorageReadableInternal(actor,scripted_sequence_extent)||
       networkable!=static_cast<unsigned char*>(actor)+0x160)return 0;
    return owner->release_networkable(owner->context,networkable);
}
#pragma comment(linker,"/EXPORT:SiN_DeferScriptedSequenceRemoval=_SiN_DeferScriptedSequenceRemoval")
extern "C" int __cdecl SiN_DeferScriptedSequenceRemoval(void* actor,unsigned bytes,
    const sin_port::native_map::DeferredRemovalBindings* b,sin_port::native_map::DeferredRemovalResult* o,
    const sin_port::native_map::ScriptedSequenceCancelBindings* cancel,sin_port::native_map::ScriptedSequenceCancelResult* cancelled) noexcept {
    if(o)*o={};if(cancelled)*cancelled={};
    if(!cancel||!cancelled)return 0;
    return sin_port::native_map::defer_remove_impl(actor,bytes,b,o,cancel,cancelled);
}

#pragma comment(linker,"/EXPORT:SiN_StartScriptedSequenceAnimation=_SiN_StartScriptedSequenceAnimation")
extern "C" int __cdecl SiN_StartScriptedSequenceAnimation(void* a,unsigned n,void* npc,unsigned token,bool complete_empty,
    const sin_port::native_map::ScriptedSequenceAnimationBindings* b,sin_port::native_map::ScriptedSequenceAnimationResult* o) noexcept {
    return sin_port::native_map::run_sequence_animation(a,n,npc,token,complete_empty,false,b,o);
}
#pragma comment(linker,"/EXPORT:SiN_CompleteScriptedSequencePlay=_SiN_CompleteScriptedSequencePlay")
extern "C" int __cdecl SiN_CompleteScriptedSequencePlay(void* a,unsigned n,void* npc,
    const sin_port::native_map::ScriptedSequenceAnimationBindings* b,sin_port::native_map::ScriptedSequenceAnimationResult* o) noexcept {
    return sin_port::native_map::run_sequence_animation(a,n,npc,0,false,true,b,o);
}

#pragma comment(linker,"/EXPORT:SiN_FinishScriptedSequence=_SiN_FinishScriptedSequence")
extern "C" int __cdecl SiN_FinishScriptedSequence(void* a,unsigned n,void* npc,
    const sin_port::native_map::ScriptedSequenceFinishBindings* b,sin_port::native_map::ScriptedSequenceFinishResult* o) noexcept {
    return sin_port::native_map::sequence_finish(a,n,npc,b,o);
}

#pragma comment(linker,"/EXPORT:SiN_StartScriptedSequenceAnimationNative=_SiN_StartScriptedSequenceAnimationNative")
extern "C" int __cdecl SiN_StartScriptedSequenceAnimationNative(void* a,unsigned n,void* npc,unsigned token,bool complete_empty,
    const sin_port::native_map::ScriptedSequenceFinishBindings* b,sin_port::native_map::ScriptedSequenceAnimationResult* o,
    sin_port::native_map::ScriptedSequenceFinishResult* terminal) noexcept {
    if(!b||!terminal){if(o)*o={};if(terminal)*terminal={};return 0;}
    return sin_port::native_map::run_sequence_animation(a,n,npc,token,complete_empty,false,b->animation,o,b,terminal);
}
#pragma comment(linker,"/EXPORT:SiN_CompleteScriptedSequencePlayNative=_SiN_CompleteScriptedSequencePlayNative")
extern "C" int __cdecl SiN_CompleteScriptedSequencePlayNative(void* a,unsigned n,void* npc,
    const sin_port::native_map::ScriptedSequenceFinishBindings* b,sin_port::native_map::ScriptedSequenceAnimationResult* o,
    sin_port::native_map::ScriptedSequenceFinishResult* terminal) noexcept {
    if(!b||!terminal){if(o)*o={};if(terminal)*terminal={};return 0;}
    return sin_port::native_map::run_sequence_animation(a,n,npc,0,false,true,b->animation,o,b,terminal);
}

#pragma comment(linker,"/EXPORT:SiN_RunScriptedSequenceRemovalThink=_SiN_RunScriptedSequenceRemovalThink")
extern "C" int __cdecl SiN_RunScriptedSequenceRemovalThink(void* a,unsigned n,
    const sin_port::native_map::ScriptedSequenceRemovalThinkBindings* b,sin_port::native_map::ScriptedSequenceRemovalThinkResult* o) noexcept {
    return sin_port::native_map::sequence_removal_think(a,n,b,o);
}
#pragma comment(linker,"/EXPORT:SiN_RunScriptedSequencePrimaryWithRemoval=_SiN_RunScriptedSequencePrimaryWithRemoval")
extern "C" int __cdecl SiN_RunScriptedSequencePrimaryWithRemoval(void* a,unsigned n,int tick,
    const sin_port::native_map::ScriptedSequencePrimaryBindings* b,sin_port::native_map::ScriptedSequencePrimaryResult* o,
    const sin_port::native_map::ScriptedSequenceRemovalThinkBindings* removal,sin_port::native_map::ScriptedSequenceRemovalThinkResult* removed) noexcept {
    if(!removal||!removed){if(o)*o={};if(removed)*removed={};return 0;}
    return sin_port::native_map::sequence_primary(a,n,tick,b,o,removal,removed);
}

#pragma comment(linker,"/EXPORT:SiN_RunScriptedNpcTask=_SiN_RunScriptedNpcTask")
extern "C" int __cdecl SiN_RunScriptedNpcTask(void* a,unsigned n,int task,
    const sin_port::native_map::ScriptedNpcTaskBindings* b,sin_port::native_map::ScriptedNpcTaskResult* o) noexcept {
    return sin_port::native_map::run_scripted_npc_task(a,n,task,b,o);
}

#pragma comment(linker,"/EXPORT:SiN_StartScriptedNpcTask=_SiN_StartScriptedNpcTask")
extern "C" int __cdecl SiN_StartScriptedNpcTask(void* a,unsigned n,int task,
    const sin_port::native_map::ScriptedNpcStartTaskBindings* b,sin_port::native_map::ScriptedNpcStartTaskResult* o) noexcept {
    return sin_port::native_map::start_scripted_npc_task(a,n,task,b,o);
}
