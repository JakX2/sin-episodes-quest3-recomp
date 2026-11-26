#include "native_server_map_entity.h"
#include "server_fixed_difficulty_service.h"
extern "C" const sin_port::fixed_service::GlobalStateApi* __cdecl SiN_GetGlobalStateApi(std::uint32_t,std::uint32_t) noexcept;
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <climits>
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
template<class T>void write_field(void* a,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(a)+off,&v,sizeof(v));}
unsigned entity_vtable(void* a) noexcept {unsigned vt=0;std::memcpy(&vt,a,4);return vt;}
bool known_map_vtable(unsigned vt) noexcept {
    if(vt==resupply_vtable||vt==antigen_vtable||vt==antigen_canister_vtable||vt==vial_vtable||vt==megahealth_vtable)return true;
    for(const auto& item:ammo_pickups)if(vt==item.vtable)return true;return false;
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
    const Spec* spec=nullptr;
    auto match=[&](const auto& table){for(const auto& s:table)if(equal_class(name,s.key)){spec=&s;break;}};
    if(vt==antigen_vtable)match(dispenser);else if(vt==antigen_canister_vtable)match(canister);
    if(!spec){match(common);if(spec)o->base_field=1;}
    if(!spec)return foreign(name);
    o->offset=spec->offset;o->field_type=spec->type;
    if(bytes<o->offset+4)return 0;
    if(spec->type==2){
        unsigned token=0;if(!b->intern||!b->intern(b->context,value,&token))return 0;
        write_field(actor,o->offset,token);o->interned=1;
    }else if(spec->type==1){
        // Original atof -> float, accepts numeric prefix and empty->0.
        const float scalar=static_cast<float>(std::strtod(value,nullptr));
        write_field(actor,o->offset,scalar);o->scalar=1;
    }else if(spec->type==5){
        // MSVCRT atoi overflow is not a portable C++ contract. Bound the
        // conversion rather than relying on host undefined behavior.
        errno=0;const long long scalar=std::strtoll(value,nullptr,10);
        if(errno==ERANGE||scalar<INT_MIN||scalar>INT_MAX)return 0;
        write_field(actor,o->offset,static_cast<int>(scalar));o->scalar=1;
    }else{
        if(!b->output||!b->output(b->context,actor,static_cast<unsigned char*>(actor)+o->offset,value))return 0;
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
bool native_spawn_leaf(const SpawnBindings& b,void* a,unsigned n,bool& native) noexcept {
    const auto vt=read_field<unsigned>(a,0);
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
const sin_port::native_map::Api api{1,sizeof(sin_port::native_map::Api),&token,&slice,&lookup,&parse,&register_factories,&find_factory,&create_factory,&spawn_ordinary,&dispatch_spawn,&key_value,&parse_all};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerMapEntityApi=_SiN_GetServerMapEntityApi")
extern "C" __declspec(dllexport) const sin_port::native_map::Api* __cdecl SiN_GetServerMapEntityApi(std::uint32_t v,std::uint32_t n) noexcept {
    using namespace sin_port::native_map;return v==1&&n==sizeof(sin_port::native_map::Api)?&api:nullptr;
}
