#include "native_server_map_entity.h"
#include <cstring>
#include <cstdlib>
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
bool spawn_actor(const SpawnBindings& b,SpawnResult& o,void* a,int& code) noexcept {
    unsigned n=0;if(!extent_ok(b,a,4,n))return false;
    const auto vt=read_field<unsigned>(a,0);
    const bool native=(vt==resupply_vtable&&b.resupply)||(vt==antigen_vtable&&b.dispenser)||(vt==antigen_canister_vtable&&b.canister);
    if(!native)return b.spawn&&b.spawn(b.context,a,&code);
    if(!b.native_spawn_begin||!b.native_spawn_end)return false;
    bool proceed=false;if(!b.native_spawn_begin(b.context,a,&proceed))return false;
    if(proceed){
        const auto* api=SiN_GetServerDynamicResupplyApi(1,sizeof(native_resupply::Api));if(!api)return false;
        if(vt==resupply_vtable){LifecycleResult r{};if(!api->spawn(a,n,b.resupply,&r))return false;}
        else if(vt==antigen_vtable){AntigenInteractionResult r{};if(!api->antigen_spawn(a,n,b.dispenser,&r))return false;}
        else {CanisterResult r{};if(!api->canister_spawn(a,n,b.canister,&r))return false;}
        ++o.native_spawns;
    }
    return b.native_spawn_end(b.context,a,&code);
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
            }else if(ok)ok=b->activate&&b->activate(b->context,a);
            const bool closed=b->engine_scope(b->context,false)!=0;if(closed)++o->engine_closes;
            ok=ok&&closed;if(ok)++o->activated;
        }
        const bool closed=scope(name,false);if(!ok||!closed)return 0;
    }
    o->completed=1;return 1;
}
const sin_port::native_map::Api api{1,sizeof(sin_port::native_map::Api),&token,&slice,&lookup,&parse,&register_factories,&find_factory,&create_factory,&spawn_ordinary};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerMapEntityApi=_SiN_GetServerMapEntityApi")
extern "C" __declspec(dllexport) const sin_port::native_map::Api* __cdecl SiN_GetServerMapEntityApi(std::uint32_t v,std::uint32_t n) noexcept {
    using namespace sin_port::native_map;return v==1&&n==sizeof(sin_port::native_map::Api)?&api:nullptr;
}
