#include "native_server_map_entity.h"
#include "native_server_npc_active_weapon_step.h"
#include "server_fixed_difficulty_service.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
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
    // The real native constructors are reached from parsed entity lumps.
    namespace nr=sin_port::native_resupply;
    FactoryDictionary dictionary{};assert(api->register_factories(&dictionary)&&dictionary.count==12&&!api->register_factories(&dictionary));
    assert(api->find_factory(&dictionary,"ITEM_ANTIGEN_DISPENSER")->family==Family::Antigen);
    assert(api->find_factory(&dictionary,"prop_antigen_canister")->original_vtable==api->find_factory(&dictionary,"prop_antigen_explosive_barrel")->original_vtable);
    assert(!api->find_factory(&dictionary,"not_registered"));
    struct NativeFixture {
        alignas(4) unsigned char storage[12][0x6a0]{};
        unsigned alloc=0,init=0,accept=0,keys=0,clear=0,foreign=0,missing=0;
    } nf;
    auto allocate=[](void* p,unsigned n,void** a,unsigned* bytes) noexcept ->int{auto& f=*static_cast<NativeFixture*>(p);assert(n<=0x6a0&&f.alloc<12);*a=f.storage[f.alloc++];*bytes=n;return 1;};
    auto initialize=[](void* p,void* a,const char* cls) noexcept ->int{auto& f=*static_cast<NativeFixture*>(p);assert(a==f.storage[f.init]&&cls);++f.init;return 1;};
    auto base=[](void*,void*) noexcept ->int{return 1;};
    nr::LifecycleBindings lifecycle{};lifecycle.base_construct=[](void*,void*,int arg) noexcept ->int{assert(!arg);return 1;};
    nr::FactoryBindings resupply{};resupply.context=&nf;resupply.allocate=allocate;resupply.initialize=initialize;resupply.lifecycle=&lifecycle;
    nr::AmmoPickupBindings ammo{};ammo.base_construct=base;
    nr::VialPickupBindings vial{};vial.base_construct=base;
    nr::PickupFactoryBindings pickups{};pickups.context=&nf;pickups.allocate=allocate;pickups.initialize=initialize;pickups.ammo=&ammo;pickups.vial=&vial;pickups.mega=&vial;
    nr::AntigenBindings dispenser{};dispenser.base_construct=base;
    nr::CanisterBindings canister{};canister.base_construct=base;
    nr::AntigenFactoryBindings antigen{};antigen.context=&nf;antigen.allocate=allocate;antigen.initialize=initialize;antigen.dispenser=&dispenser;antigen.canister=&canister;
    FactoryBindings factories{};factories.context=&nf;factories.dictionary=&dictionary;factories.resupply=&resupply;factories.pickup=&pickups;factories.antigen=&antigen;
    factories.foreign_create=[](void* p,const char* name,void** a,unsigned* bytes) noexcept ->int{assert(!std::strcmp(name,"unknown"));++static_cast<NativeFixture*>(p)->foreign;*a=nullptr;*bytes=0;return 1;};
    Bindings nb{};nb.context=&nf;nb.native_factory=&factories;
    nb.clear_factory_global=[](void* p) noexcept ->int{++static_cast<NativeFixture*>(p)->clear;return 1;};
    nb.key_value=[](void* p,void*,const char*,const char*) noexcept ->int{++static_cast<NativeFixture*>(p)->keys;return 1;};
    nb.accept_entity=[](void* p,void* a,const char*,unsigned,const EntitySlice*) noexcept ->int{auto& f=*static_cast<NativeFixture*>(p);assert(a==f.storage[f.accept]);++f.accept;return 1;};
    nb.missing=[](void* p,const char* cls) noexcept ->int{assert(!std::strcmp(cls,"unknown"));++static_cast<NativeFixture*>(p)->missing;return 1;};
    std::string map;
    for(unsigned i=0;i<dictionary.count;++i)map+="{ classname "+std::string(dictionary.slots[i]->name)+" }\n";
    map+="{ classname unknown }";
    assert(api->parse(map.c_str(),static_cast<unsigned>(map.size()),&nb,&r)&&r.completed&&r.native_creates==12&&r.foreign_creates==1&&nf.alloc==12&&nf.init==12&&nf.accept==12&&nf.keys==12&&nf.clear==13&&nf.missing==1);
    auto word=[](const void* a,unsigned offset){unsigned v=0;std::memcpy(&v,static_cast<const unsigned char*>(a)+offset,4);return v;};
    assert(word(nf.storage[0],0)==nr::resupply_vtable&&word(nf.storage[0],0x394)==0x3f800000);
    for(unsigned i=0;i<6;++i)assert(word(nf.storage[i+1],0)==nr::ammo_pickups[i].vtable&&word(nf.storage[i+1],0x4d4)==unsigned(nr::ammo_pickups[i].units));
    assert(word(nf.storage[7],0)==nr::vial_vtable&&word(nf.storage[8],0)==nr::megahealth_vtable&&word(nf.storage[9],0)==nr::antigen_vtable&&word(nf.storage[10],0)==nr::antigen_canister_vtable&&word(nf.storage[11],0)==nr::antigen_canister_vtable);
    factories.pickup=nullptr;FactoryResult fr{};
    assert(!api->create_factory("item_megahealth",&factories,&fr)&&fr.matched&&!fr.completed&&nf.foreign==1);
    // Parent-depth order, descending class priority, reverse links,
    // removal of marked later rows before deferred cleanup.
    struct SpawnFixture {
        alignas(4) unsigned char actors[5][0x170]{};
        const char* classes[5]{"logic_relay","func_wall","scripted_sequence","logic_timer","prop_physics"};
        std::string order;unsigned cleanup=0;
        unsigned index(void* a)const noexcept {for(unsigned i=0;i<5;++i)if(a==actors[i])return i;assert(false);return 99;}
    } sf;
    auto put=[](void* a,unsigned off,unsigned v){std::memcpy(static_cast<unsigned char*>(a)+off,&v,4);};
    put(sf.actors[0],0x14,11);put(sf.actors[3],0x14,12);put(sf.actors[1],0x16c,1);put(sf.actors[0],0x16c,1);
    SpawnRow rows[]{{sf.actors[3],0,0},{sf.actors[2],0,0},{sf.actors[0],0,0},{sf.actors[4],0,0},{sf.actors[1],0,0}};
    SpawnBindings sp{};sp.context=&sf;
    sp.extent=[](void*,void*,unsigned* n) noexcept ->int{*n=0x170;return 1;};
    sp.classname=[](void* p,void* a,const char** n) noexcept ->int{auto& f=*static_cast<SpawnFixture*>(p);*n=f.classes[f.index(a)];return 1;};
    sp.find_parent=[](void* p,unsigned name,void** a) noexcept ->int{auto& f=*static_cast<SpawnFixture*>(p);assert(name==11||name==12);*a=f.actors[name==11?1:0];return 1;};
    sp.set_parent=[](void* p,void* a,void* parent,int index) noexcept ->int{auto& f=*static_cast<SpawnFixture*>(p);assert(index==-1);const auto i=f.index(a);assert((i==3&&parent==f.actors[0])||(i==0&&parent==f.actors[1]));f.order+=char('a'+i);return 1;};
    sp.scope=[](void*,const char*,bool) noexcept ->int{return 1;};
    sp.engine_scope=[](void*,bool) noexcept ->int{return 1;};
    sp.debug_enabled=[](void*,bool* yes) noexcept ->int{*yes=false;return 1;};
    sp.spawn=[](void* p,void* a,int* code) noexcept ->int{
        auto& f=*static_cast<SpawnFixture*>(p);const auto i=f.index(a);f.order+=char('0'+i);*code=0;
        if(i==0){*code=-1;unsigned flags=1;std::memcpy(f.actors[3]+0xb8,&flags,4);}return 1;
    };
    sp.cleanup=[](void* p) noexcept ->int{auto& f=*static_cast<SpawnFixture*>(p);++f.cleanup;f.order+='C';return 1;};
    sp.activate=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<SpawnFixture*>(p);f.order+=char('A'+f.index(a));return 1;};
    SpawnResult so{};
    assert(api->spawn_ordinary(rows,5,true,&sp,&so)&&so.completed&&so.ranked==5&&so.parents==2&&so.spawned==3&&so.failed==1&&so.pruned==1&&so.cleanups==1&&so.activated==3&&so.scopes==so.closes&&so.engine_scopes==3&&so.engine_closes==3);
    assert(sf.order=="da1240CBCE"&&sf.cleanup==1);
    // Cyclic parent dependency is refused before spawning.
    put(sf.actors[1],0x14,12);SpawnRow cyclic[]{ {sf.actors[0],0,0} };
    assert(!api->spawn_ordinary(cyclic,1,false,&sp,&so)&&!so.spawned);
    // Real native global state service and original post-Spawn admission.
    using namespace sin_port::fixed_service;
    const auto global_get=reinterpret_cast<GetGlobalStateApi>(GetProcAddress(server,global_state_export_name));assert(global_get);
    const auto* globals=global_get(global_state_version,sizeof(GlobalStateApi));assert(globals&&globals->clear());
    struct DispatchFixture {
        alignas(4) unsigned char actor[0x3b0]{};
        sin_independent::EntityHandleEntry entries[1]{};
        unsigned listeners[2]{},count=2,mode=0,spawns=0,removes=0,dormant=0,enters=0,closes=0,precache=0;
        void* list[2]{&listeners[0],&listeners[1]};
        const char* map="SE1_docks01";std::string order;
    } df;
    put(df.actor,0x378,0x3000u);put(df.actor,0x10,21);
    df.entries[0].serial=3;df.entries[0].entity=reinterpret_cast<CBaseEntity*>(df.actor);
    DispatchBindings db{};db.context=&df;db.entries=df.entries;db.entry_count=1;db.current_map=&df.map;
    db.resolve_string=[](void*,unsigned token,const char** s) noexcept ->int{assert(token==21);*s="campaign_item";return 1;};
    db.scope=[](void* p,bool enter) noexcept ->int{auto& f=*static_cast<DispatchFixture*>(p);if(enter)++f.enters;else ++f.closes;return 1;};
    db.remove=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<DispatchFixture*>(p);assert(a==f.actor);++f.removes;return 1;};
    db.dormant=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<DispatchFixture*>(p);assert(a==f.actor);++f.dormant;return 1;};
    db.listeners=df.list;db.listener_count=&df.count;db.listener_capacity=2;
    db.notify=[](void* p,void* listener,void* a) noexcept ->int{auto& f=*static_cast<DispatchFixture*>(p);assert(a==f.actor);f.order+=listener==&f.listeners[1]?'1':'0';return 1;};
    SpawnBindings ds{};ds.context=&df;ds.native_dispatch=&db;
    ds.extent=[](void* p,void* a,unsigned* n) noexcept ->int{auto& f=*static_cast<DispatchFixture*>(p);assert(a==f.actor);*n=sizeof(f.actor);return 1;};
    ds.virtual_spawn=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<DispatchFixture*>(p);assert(a==f.actor&&f.enters==f.closes+1);++f.spawns;
        if(f.mode==1)f.entries[0].serial=4;
        if(f.mode==2){unsigned flags=1;std::memcpy(f.actor+0xb8,&flags,4);}return 1;};
    DispatchResult dr{};
    assert(api->dispatch_spawn(nullptr,&ds,&dr)&&dr.completed&&!dr.entered);
    assert(api->dispatch_spawn(df.actor,&ds,&dr)&&dr.completed&&dr.global_added&&dr.global_state==1&&dr.notified==2&&df.order=="10");
    int gi=globals->find("campaign_item");assert(gi>=0);
    df.map="SE1_DOCKS01";df.order.clear();
    assert(api->dispatch_spawn(df.actor,&ds,&dr)&&!dr.dormant&&!dr.global_added&&df.order=="10");
    df.map="SE1_docks02";df.order.clear();
    assert(api->dispatch_spawn(df.actor,&ds,&dr)&&dr.dormant&&df.dormant==1&&df.order=="10");
    assert(globals->set_state(gi,2));df.order.clear();
    assert(api->dispatch_spawn(df.actor,&ds,&dr)&&dr.global_dead&&dr.removed&&dr.native_result==-1&&df.removes==1&&df.order.empty());
    df.mode=1;
    assert(api->dispatch_spawn(df.actor,&ds,&dr)&&dr.stale&&dr.native_result==-1&&!dr.global_lookup);
    df.entries[0].serial=3;df.mode=2;
    assert(api->dispatch_spawn(df.actor,&ds,&dr)&&dr.marked&&dr.native_result==-1&&!dr.global_lookup);
    df.mode=0;put(df.actor,0xb8,0);put(df.actor,0x10,0);put(df.actor,0,nr::resupply_vtable);put(df.actor,0xb4,2);
    nr::LifecycleBindings dlife{};dlife.context=&df;
    dlife.precache=[](void* p,const char*,int) noexcept ->int{++static_cast<DispatchFixture*>(p)->precache;return 1;};
    ds.resupply=&dlife;ds.virtual_spawn=nullptr;
    assert(api->dispatch_spawn(df.actor,&ds,&dr)&&dr.native_spawn&&df.precache==7&&dr.notified==2&&df.enters==df.closes);
    assert(globals->clear());
    // Native KeyValue mutates actual authenticated map entity storage.
    struct KeyFixture {unsigned interns=0,outputs=0,foreign=0,warnings=0;void* output=nullptr;std::string value,key;} kf;
    KeyBindings kb{};kb.context=&kf;
    kb.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=0x6a0;return 1;};
    kb.intern=[](void* p,const char* value,unsigned* token) noexcept ->int{auto& f=*static_cast<KeyFixture*>(p);f.value=value;*token=++f.interns+100;return 1;};
    kb.output=[](void* p,void*,void* output,const char* value) noexcept ->int{auto& f=*static_cast<KeyFixture*>(p);f.output=output;f.value=value;++f.outputs;return 1;};
    kb.foreign=[](void* p,void*,const char* key,const char* value,bool* handled) noexcept ->int{auto& f=*static_cast<KeyFixture*>(p);f.key=key;f.value=value;++f.foreign;*handled=false;return 1;};
    kb.resupply_bounds_warning=[](void* p,void*) noexcept ->int{++static_cast<KeyFixture*>(p)->warnings;return 1;};
    KeyResult kr{};
    auto* disp=nf.storage[9];auto* barrel=nf.storage[10];auto* supply=nf.storage[0];
    assert(api->key_value(disp,"aNtIgEn#2","42.5xyz",&kb,&kr)&&kr.handled&&kr.offset==0x49c&&kr.field_type==1&&word(disp,0x49c)==0x422a0000);
    assert(api->key_value(barrel,"Percent","-4",&kb,&kr)&&kr.handled&&word(barrel,0x628)==0xc0800000);
    assert(api->key_value(barrel,"ContainerFlags","  +9tail",&kb,&kr)&&kr.handled&&word(barrel,0x62c)==9);
    assert(api->key_value(barrel,"ContainerFlags","2147483648",&kb,&kr)==0&&word(barrel,0x62c)==9);
    assert(api->key_value(disp,"OnCanisterInserted#1","relay,Trigger,,0,-1",&kb,&kr)&&kr.output&&kf.output==disp+0x4b8);
    assert(api->key_value(disp,"OnCanisterEjected","relay,Trigger,,1,1",&kb,&kr)&&kr.output&&kf.output==disp+0x4d0);
    assert(api->key_value(barrel,"OnExplode","relay,Trigger,,0,1",&kb,&kr)&&kr.output&&kf.output==barrel+0x66c);
    assert(api->key_value(supply,"globalname","campaign_supply",&kb,&kr)&&kr.interned&&word(supply,0x10)==101);
    assert(api->key_value(supply,"parentname","crate",&kb,&kr)&&kr.interned&&word(supply,0x14)==102);
    assert(api->key_value(supply,"targetname","supply_1",&kb,&kr)&&kr.interned&&word(supply,0xc0)==103);
    assert(api->key_value(supply,"spawnflags","3",&kb,&kr)&&kr.scalar&&word(supply,0xb4)==3);
    assert(api->key_value(supply,"m_flDesiredHealth","0.1",&kb,&kr)&&!kr.handled&&kr.foreign&&word(supply,0x394)==0x3f800000);
    assert(api->key_value(supply,"mins","0 0 0",&kb,&kr)&&kr.ignored_bounds&&kf.warnings==1);
    assert(api->key_value(supply,"mins#1","0 0 0",&kb,&kr)&&kr.foreign&&kf.key=="mins"&&kf.warnings==1);
    // Real parser can now use native keys rather than the test key sink.
    NativeFixture parsed{};resupply.context=&parsed;pickups.context=&parsed;antigen.context=&parsed;factories.context=&parsed;factories.pickup=&pickups;
    nb.context=&parsed;nb.key_value=nullptr;nb.native_keys=&kb;
    const char gameplay[]="{ classname item_antigen_dispenser Antigen 33 targetname station OnCanisterInserted \"relay,Trigger,,0,-1\" } { classname prop_antigen_canister Percent 77 ContainerFlags 9 } { classname item_dynamic_resupply spawnflags 3 globalname persistent }";
    assert(api->parse(gameplay,sizeof(gameplay),&nb,&r)&&r.completed&&r.native_creates==3&&parsed.alloc==3&&parsed.accept==3);
    assert(word(parsed.storage[0],0x49c)==0x42040000&&word(parsed.storage[1],0x628)==0x429a0000&&word(parsed.storage[1],0x62c)==9&&word(parsed.storage[2],0xb4)==3&&word(parsed.storage[2],0x10)!=0);
    assert(kf.output==parsed.storage[0]+0x4b8);
    // Real output field, real PE32 action layout, shared action IDs.
    struct ActionPool {alignas(4) unsigned char rows[8][0x1c]{};unsigned used=0,id=40,released=0;bool fail=false;} pool;
    OutputActionBindings output{};output.context=&pool;output.next_id=&pool.id;
    output.allocate=[](void* p,unsigned bytes,void** row) noexcept ->int{
        auto& f=*static_cast<ActionPool*>(p);assert(bytes==0x1c);
        if(f.fail||f.used==8)return 0;*row=f.rows[f.used++];return 1;
    };
    output.release=[](void* p,void*) noexcept ->int{++static_cast<ActionPool*>(p)->released;return 1;};
    kb.native_output=&output;
    kb.intern=[](void*,const char* value,unsigned* token) noexcept ->int{
        if(!std::strcmp(value,"relay"))*token=201;
        else if(!std::strcmp(value,"Trigger"))*token=202;
        else if(!std::strcmp(value,"Use"))*token=203;
        else if(!std::strcmp(value,"param"))*token=204;
        else return 0;return 1;
    };
    put(disp,0x4b8+0x14,0);put(barrel,0x66c+0x14,0);
    auto ptrword=[](const void* a){return static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(a));};
    assert(api->key_value(disp,"OnCanisterInserted","relay,Trigger,param,1.5,2",&kb,&kr)&&kr.output);
    assert(word(pool.rows[0],0)==201&&word(pool.rows[0],4)==202&&word(pool.rows[0],8)==204&&word(pool.rows[0],0xc)==0x3fc00000&&word(pool.rows[0],0x10)==2&&word(pool.rows[0],0x14)==41&&!word(pool.rows[0],0x18));
    assert(api->key_value(disp,"OnCanisterInserted#2","relay,,,0,0",&kb,&kr)&&kr.output);
    assert(word(disp,0x4b8+0x14)==ptrword(pool.rows[1])&&word(pool.rows[1],4)==203&&!word(pool.rows[1],8)&&word(pool.rows[1],0x10)==0xffffffffu&&word(pool.rows[1],0x14)==42&&word(pool.rows[1],0x18)==ptrword(pool.rows[0]));
    assert(api->key_value(barrel,"OnExplode","relay,Trigger",&kb,&kr)&&word(barrel,0x66c+0x14)==ptrword(pool.rows[2])&&word(pool.rows[2],0x10)==0xffffffffu&&word(pool.rows[2],0x14)==43);
    assert(!api->key_value(disp,"OnCanisterInserted","relay,Trigger,,0,2147483648",&kb,&kr)&&pool.used==3);
    std::string too_long(256,'x');assert(!api->key_value(disp,"OnCanisterInserted",too_long.c_str(),&kb,&kr)&&pool.used==3);
    pool.fail=true;assert(!api->key_value(disp,"OnCanisterInserted","relay,Trigger",&kb,&kr)&&pool.id==43);pool.fail=false;
    assert(!api->key_value(disp,"OnCanisterInserted","unknown,Trigger",&kb,&kr)&&pool.released==1&&word(disp,0x4b8+0x14)==ptrword(pool.rows[1]));
    // Actual map tokenizer and native constructor, two output keys in source order.
    NativeFixture outparsed{};resupply.context=&outparsed;pickups.context=&outparsed;antigen.context=&outparsed;factories.context=&outparsed;
    nb.context=&outparsed;
    // All non-output classname tokens retain an owner-provided intern result.
    kb.intern=[](void*,const char* value,unsigned* token) noexcept ->int{
        *token=!std::strcmp(value,"relay")?201u:!std::strcmp(value,"Trigger")?202u:!std::strcmp(value,"Use")?203u:!std::strcmp(value,"param")?204u:300u;return 1;
    };
    const char outmap[]="{ classname item_antigen_dispenser OnCanisterInserted \"relay,Trigger,param,0.5,1\" OnCanisterInserted#2 \"relay,,,2,-1\" }";
    assert(api->parse(outmap,sizeof(outmap),&nb,&r)&&r.completed&&r.native_creates==1);
    assert(word(outparsed.storage[0],0x4b8+0x14)==ptrword(pool.rows[5])&&word(pool.rows[5],0x18)==ptrword(pool.rows[4])&&word(pool.rows[5],4)==203&&word(pool.rows[5],0xc)==0x40000000&&word(pool.rows[4],0x10)==1);
    // Use the very same map-parsed action rows through native firing,
    // queue construction and deadline service; no synthetic row reseeding.
    namespace nw=sin_port::native_weapon_tables;
    const auto fire_get=reinterpret_cast<nw::GetServerEntityVoidOutputApi>(GetProcAddress(server,nw::server_entity_void_output_export));
    const auto queue_get=reinterpret_cast<nw::GetServerEntityNamedEventQueueApi>(GetProcAddress(server,nw::server_entity_named_event_queue_export));
    const auto service_get=reinterpret_cast<nw::GetServerEntityEventServiceApi>(GetProcAddress(server,nw::server_entity_event_service_export));
    assert(fire_get&&queue_get&&service_get);
    const auto* fire_api=fire_get(1,sizeof(nw::ServerEntityVoidOutputApi));
    const auto* queue_api=queue_get(1,sizeof(nw::ServerEntityNamedEventQueueApi));
    const auto* service_api=service_get(1,sizeof(nw::ServerEntityEventServiceApi));
    assert(fire_api&&queue_api&&service_api);
    struct ConnectedOutput {
        ActionPool* actions;
        const nw::ServerEntityNamedEventQueueApi* api;
        void* actor;
        alignas(4) unsigned char root[0x38]{},rows[4][0x38]{},target[4]{};
        unsigned alloc=0,delivered=0,released=0;
        unsigned ids[3]{},inputs[3]{},types[3]{},params[3]{};
        float now=10.f;bool locked=false;
        nw::ServerEntityNamedEventQueueBindings queue{};
    } co{&pool,queue_api,outparsed.storage[0]};
    co.queue.context=&co;
    co.queue.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
        auto& f=*static_cast<ConnectedOutput*>(p);if(!token||token>f.alloc)return 0;*row=f.rows[token-1];*bytes=0x38;return 1;
    };
    co.queue.allocate=[](void* p,unsigned* token,void** row,unsigned* bytes) noexcept ->int{
        auto& f=*static_cast<ConnectedOutput*>(p);if(f.alloc==4)return 0;*row=f.rows[f.alloc++];*token=f.alloc;*bytes=0x38;return 1;
    };
    co.queue.string_nonempty=[](void*,unsigned token,bool* yes) noexcept ->int{*yes=token>=201&&token<=204;return 1;};
    nw::ServerEntityVoidOutputBindings fire_bind{};fire_bind.context=&co;
    fire_bind.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
        auto& f=*static_cast<ConnectedOutput*>(p);
        for(unsigned i=4;i<6;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.actions->rows[i]))){
            *row=f.actions->rows[i];*bytes=0x1c;return 1;
        }return 0;
    };
    fire_bind.enqueue=[](void* p,void* activator,void* caller,const nw::ServerEntityVoidOutputAction* a) noexcept ->int{
        auto& f=*static_cast<ConnectedOutput*>(p);assert(activator==f.actor&&caller==f.actor);
        nw::ServerEntityNamedEventQueueResult qr{};
        return f.api->enqueue(f.root,sizeof(f.root),99,f.now,0x3000u,0x3000u,a,&f.queue,&qr);
    };
    fire_bind.release=[](void* p,unsigned token,void* row) noexcept ->int{
        auto& f=*static_cast<ConnectedOutput*>(p);assert(row==f.actions->rows[4]&&token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(row)));
        ++f.actions->released;std::memset(row,0xcc,0x1c);return 1;
    };
    nw::ServerEntityVoidOutputResult connected_fire_result{};
    put(outparsed.storage[0],0xa8,0);
    auto* outfield=outparsed.storage[0]+0x4b8;
    const unsigned finite_id=word(pool.rows[4],0x14),unlimited_id=word(pool.rows[5],0x14);
    assert(fire_api->fire(outfield,0x18,co.actor,co.actor,0x6a0,.25f,&fire_bind,&connected_fire_result)&&connected_fire_result.queued==2&&connected_fire_result.parameter_overrides==1&&connected_fire_result.removed==1);
    assert(pool.released==2&&word(outfield,0x14)==ptrword(pool.rows[5])&&!word(pool.rows[5],0x18)&&word(pool.rows[5],0x10)==0xffffffffu);
    // Unlimited row is visited first, but finite parameter row has an earlier deadline.
    assert(co.alloc==2&&word(co.root,0x30)==2&&word(co.rows[1],0x30)==1&&word(co.rows[0],0x34)==2);
    assert(word(co.rows[0],0)==0x41440000&&word(co.rows[1],0)==0x41280000);
    sin_independent::EntityHandleEntry entry{};entry.entity=reinterpret_cast<CBaseEntity*>(co.actor);entry.serial=3;
    nw::ServerEntityEventServiceBindings sb{};sb.context=&co;sb.entities.entries=&entry;sb.entities.count=1;sb.resolve=co.queue.resolve;
    sb.lock=[](void* p,bool enter) noexcept ->int{auto& f=*static_cast<ConnectedOutput*>(p);assert(f.locked!=enter);f.locked=enter;return 1;};
    sb.find_named=[](void* p,void* prev,unsigned name,void* activator,void* caller,void** entity) noexcept ->int{
        auto& f=*static_cast<ConnectedOutput*>(p);assert(f.locked&&name==201&&activator==f.actor&&caller==f.actor);*entity=prev?nullptr:f.target;return 1;
    };
    sb.accept=[](void* p,void* target,unsigned input,void* activator,void* caller,const void* variant,unsigned id) noexcept ->int{
        auto& f=*static_cast<ConnectedOutput*>(p);assert(f.locked&&target==f.target&&activator==f.actor&&caller==f.actor&&f.delivered<3);
        const unsigned i=f.delivered++;f.ids[i]=id;f.inputs[i]=input;
        std::memcpy(&f.params[i],variant,4);std::memcpy(&f.types[i],static_cast<const unsigned char*>(variant)+0x10,4);return 1;
    };
    sb.release=[](void* p,unsigned token,void* row) noexcept ->int{
        auto& f=*static_cast<ConnectedOutput*>(p);assert(f.locked&&token&&token<=f.alloc&&row==f.rows[token-1]);++f.released;return 1;
    };
    nw::ServerEntityEventServiceResult es{};int budget=0;
    auto service_at=[&](float now){return service_api->service(co.root,sizeof(co.root),99,now,false,&budget,&sb,&es);};
    assert(service_at(10.f)&&es.future_stopped&&!es.events);
    assert(service_at(10.5f)&&es.events==1&&es.future_stopped&&co.ids[0]==finite_id&&co.inputs[0]==202&&co.params[0]==204&&co.types[0]==2);
    assert(service_at(12.25f)&&es.events==1&&co.ids[1]==unlimited_id&&co.inputs[1]==203&&!co.params[1]&&!co.types[1]&&!word(co.root,0x30));
    co.now=20.f;
    assert(fire_api->fire(outfield,0x18,co.actor,co.actor,0x6a0,.25f,&fire_bind,&connected_fire_result)&&connected_fire_result.queued==1&&!connected_fire_result.removed);
    assert(service_at(22.25f)&&es.events==1&&co.ids[2]==unlimited_id&&co.delivered==3&&co.released==3&&!co.locked&&!word(co.root,0x30));
    // CLogicRelay original input leaves, with the same native output parser.
    alignas(4) unsigned char relay[relay_extent]{};
    put(relay,0,relay_vtable);relay[0x3ad]=1;
    assert(api->key_value(relay,"StartDisabled","-2",&kb,&kr)&&kr.handled&&relay[0x3ac]==1&&relay[0x3ad]==1);
    assert(api->key_value(relay,"m_bWaitForRefire","0",&kb,&kr)&&kr.foreign&&!kr.handled&&relay[0x3ad]==1);
    assert(api->key_value(relay,"StartDisabled","0",&kb,&kr)&&!relay[0x3ac]&&relay[0x3ad]);
    assert(api->key_value(relay,"OnTrigger","relay,Trigger,,10,1",&kb,&kr));
    assert(api->key_value(relay,"OnTrigger#2","relay,Trigger,,2,-1",&kb,&kr));
    struct RelayFixture {ActionPool* pool;void* actor;void* activator;unsigned queued=0,released=0,removed=0,canceled=0,refires=0;float delay=0;ConnectedOutput* connected=nullptr;} rf{&pool,relay,co.actor};
    nw::ServerEntityVoidOutputBindings ro{};ro.context=&rf;
    ro.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
        auto& f=*static_cast<RelayFixture*>(p);
        for(unsigned i=6;i<8;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[i]))){*row=f.pool->rows[i];*bytes=0x1c;return 1;}return 0;
    };
    ro.enqueue=[](void* p,void* a,void* c,const nw::ServerEntityVoidOutputAction*) noexcept ->int{
        auto& f=*static_cast<RelayFixture*>(p);assert(a==f.activator&&c==f.actor);++f.queued;return 1;
    };
    ro.release=[](void* p,unsigned,void* row) noexcept ->int{auto& f=*static_cast<RelayFixture*>(p);assert(row==f.pool->rows[6]);++f.released;return 1;};
    RelayBindings rb{};rb.context=&rf;rb.output=&ro;
    rb.remove=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<RelayFixture*>(p);assert(a==f.actor);++f.removed;return 1;};
    rb.cancel=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<RelayFixture*>(p);assert(a==f.actor);++f.canceled;return 1;};
    rb.refire=[](void* p,void* a,float d) noexcept ->int{auto& f=*static_cast<RelayFixture*>(p);assert(a==f.actor);++f.refires;f.delay=d;return 1;};
    RelayResult rr{};
    auto input=[&](const char* name){return api->relay_input(relay,sizeof(relay),name,rf.activator,&rb,&rr);};
    assert(input("Trigger")&&rr.suppressed&&!rf.queued);
    assert(input("EnableRefire")&&!relay[0x3ad]);
    assert(input("Disable")&&relay[0x3ac]);
    assert(input("Trigger")&&rr.suppressed);
    assert(input("Toggle")&&!relay[0x3ac]);
    assert(input("tRiGgEr")&&rr.fired&&rr.queued==2&&rr.actions==1&&rr.maximum_delay==2.f&&rr.refire_delay==static_cast<float>(2.001)&&rf.delay==rr.refire_delay&&rf.released==1&&relay[0x3ad]);
    assert(input("Trigger")&&rr.suppressed&&rf.queued==2);
    assert(input("CancelPending")&&rr.canceled&&rf.canceled==1&&!relay[0x3ad]);
    put(relay,0xb4,2);assert(input("Trigger")&&rr.fired&&!rr.refire&&!relay[0x3ad]&&rf.queued==3);
    put(relay,0xb4,3);assert(input("Trigger")&&rr.removed&&rf.removed==1&&rf.queued==4&&rf.refires==1);
    assert(input("Enable")&&!relay[0x3ac]);assert(input("Unknown")&&!rr.handled);
    // Native direct refire scheduling and original caller cancellation.
    // Reuse actual named/direct queue API with an independent fresh root.
    std::memset(co.root,0,sizeof(co.root));std::memset(co.rows,0,sizeof(co.rows));co.alloc=0;co.released=0;co.now=30.f;co.actor=relay;
    co.queue.string_nonempty=[](void*,unsigned token,bool* yes) noexcept ->int{*yes=token>=201&&token<=205;return 1;};
    sin_independent::EntityHandleEntry relay_entries[2]{};
    relay_entries[0].entity=reinterpret_cast<CBaseEntity*>(relay);relay_entries[0].serial=3;
    relay_entries[1].entity=reinterpret_cast<CBaseEntity*>(rf.activator);relay_entries[1].serial=7;
    RelayQueueBindings rq{};rq.context=&co;rq.root=co.root;rq.root_bytes=sizeof(co.root);rq.root_token=99;rq.queue=&co.queue;
    rq.entries=relay_entries;rq.entry_count=2;rq.now=&co.now;
    rq.handle=[](void* p,void* a,unsigned* h) noexcept ->int{assert(a==static_cast<ConnectedOutput*>(p)->actor);*h=0x3000u;return 1;};
    rq.intern=[](void*,const char* text,unsigned* token) noexcept ->int{assert(!std::strcmp(text,"EnableRefire"));*token=205;return 1;};
    rq.release=[](void* p,unsigned token,void* row) noexcept ->int{auto& f=*static_cast<ConnectedOutput*>(p);assert(token&&token<=f.alloc&&row==f.rows[token-1]);++f.released;std::memset(row,0xcc,0x38);return 1;};
    rb.native_queue=&rq;
    put(relay,0xb4,0);relay[0x3ad]=0;
    assert(input("Trigger")&&rr.native_refire&&rr.refire&&relay[0x3ad]&&co.alloc==1&&rf.refires==1);
    assert(word(co.rows[0],4)==0&&word(co.rows[0],8)==205&&word(co.rows[0],0xc)==0x3000&&word(co.rows[0],0x10)==0x3000&&word(co.rows[0],0x14)==0&&word(co.rows[0],0x18)==0x3000&&word(co.rows[0],0x28)==0xffffffffu&&word(co.rows[0],0x2c)==0);
    float deadline=0;std::memcpy(&deadline,co.rows[0],4);assert(deadline==static_cast<float>(30.0+rr.refire_delay));
    nw::ServerEntityVoidOutputAction cancel_action{201,202,0,0,123,1.f};
    nw::ServerEntityNamedEventQueueResult cancel_qr{};
    auto queue_caller=[&](unsigned caller){return queue_api->enqueue(co.root,sizeof(co.root),99,co.now,0x3000,caller,&cancel_action,&co.queue,&cancel_qr);};
    assert(queue_caller(0x7001)&&queue_caller(0x2000)&&queue_caller(0x3000));
    // Queue order: other caller, stale caller, matching named caller, refire.
    assert(word(co.root,0x30)==2&&word(co.rows[1],0x30)==3&&word(co.rows[2],0x30)==4&&word(co.rows[3],0x30)==1);
    unsigned protected_node=4;rq.protected_node=&protected_node;
    assert(!input("CancelPending")&&rr.protected_match&&!rr.cancel_removed&&relay[0x3ad]&&co.released==0);
    protected_node=0;
    assert(input("CancelPending")&&rr.native_cancel&&rr.cancel_visited==4&&rr.cancel_removed==2&&!relay[0x3ad]&&co.released==2);
    assert(word(co.root,0x30)==2&&word(co.rows[1],0x30)==3&&word(co.rows[2],0x34)==2&&!word(co.rows[2],0x30)&&rf.canceled==1);
    // Real event-service AcceptInput walks CLogicRelay's authenticated
    // datamap and executes the recovered input leaf, not a relay callback.
    std::memset(co.root,0,sizeof(co.root));std::memset(co.rows,0,sizeof(co.rows));co.alloc=0;co.released=0;co.delivered=0;co.now=40.f;
    co.queue.string_nonempty=[](void*,unsigned token,bool* yes) noexcept ->int{*yes=token>=201&&token<=206;return 1;};
    rf.connected=&co;
    ro.enqueue=[](void* p,void* a,void* caller,const nw::ServerEntityVoidOutputAction* action) noexcept ->int{
        auto& f=*static_cast<RelayFixture*>(p);assert(a==f.activator&&caller==f.actor);
        auto& q=*f.connected;nw::ServerEntityNamedEventQueueResult out{};
        return q.api->enqueue(q.root,sizeof(q.root),99,q.now,0x7001,0x3000,action,&q.queue,&out);
    };
    nw::ServerNpcActivePostAnimationBindings relay_storage{};
    nw::ServerEntityAcceptInputBindings ai{};ai.storage=&relay_storage;ai.relay=&rb;
    ai.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
        *text=token==202?"Trigger":token==205?"EnableRefire":nullptr;if(!*text)return 0;
        *bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
    };
    nw::ServerEntitySearchBindings relay_search{};relay_search.context=&co;
    relay_search.extent=[](void* p,void* actor,unsigned* bytes) noexcept ->int{auto& f=*static_cast<ConnectedOutput*>(p);*bytes=actor==f.actor?relay_extent:actor==f.target?4u:0u;return *bytes!=0;};
    auto relay_service=sb;relay_service.entities.entries=relay_entries;relay_service.entities.count=2;relay_service.native_search=&relay_search;relay_service.native_input=&ai;relay_service.live_now=&co.now;
    relay_service.find_named=[](void* p,void* prev,unsigned name,void*,void*,void** target) noexcept ->int{
        auto& f=*static_cast<ConnectedOutput*>(p);assert(f.locked&&(name==201||name==206));
        *target=prev?nullptr:(name==206?f.actor:f.target);return 1;
    };
    relay_service.accept=[](void* p,void* target,unsigned input,void* activator,void* caller,const void* variant,unsigned) noexcept ->int{
        auto& f=*static_cast<ConnectedOutput*>(p);assert(f.locked&&target==f.target&&input==202&&caller==f.actor&&activator&&*static_cast<const unsigned char*>(variant)==0);
        ++f.delivered;return 1;
    };
    // String parameter to void input is converted by existing AcceptInput.
    const nw::ServerEntityVoidOutputAction relay_trigger{206,202,204,2,987,0.f};
    assert(queue_api->enqueue(co.root,sizeof(co.root),99,co.now,0x7001,0x7001,&relay_trigger,&co.queue,&cancel_qr));
    relay[0x1f0]=1;relay[0x200]=0;
    auto run_relay_service=[&](float now){co.now=now;return service_api->service(co.root,sizeof(co.root),99,now,false,&budget,&relay_service,&es);};
    assert(run_relay_service(40.f)&&es.events==1&&es.native_inputs==1&&es.future_stopped&&relay[0x3ad]&&relay[0x200]&&co.alloc==3&&co.delivered==0);
    assert(word(co.root,0x30)==2&&word(co.rows[1],0x30)==3&&word(co.rows[2],0x18)==0x3000);
    assert(run_relay_service(42.f)&&es.events==1&&es.foreign_inputs==1&&es.future_stopped&&co.delivered==1&&relay[0x3ad]);
    assert(run_relay_service(42.01f)&&es.events==1&&es.native_inputs==1&&es.direct==1&&!relay[0x3ad]&&!word(co.root,0x30)&&co.released==3);
    // Opt-in campaign factory registration and actual native map creation.
    assert(dictionary.count==12&&!api->find_factory(&dictionary,"logic_relay"));
    assert(api->register_relay(&dictionary)&&dictionary.count==13&&!api->register_relay(&dictionary));
    const auto* relay_spec=api->find_factory(&dictionary,"LOGIC_RELAY");
    assert(relay_spec&&relay_spec->family==Family::Relay&&relay_spec->original_object==0x224fda88&&relay_spec->original_vtable==0x224228bc&&relay_spec->extent==relay_extent);
    assert(!api->create_factory("logic_relay",&factories,&fr)&&fr.matched&&!fr.actor);
    struct RelayFactoryFixture {
        alignas(4) unsigned char storage[relay_extent]{};
        unsigned alloc=0,base=0,init=0,released=0,cleared=0,accepted=0;
        bool short_alloc=false;
    } rff;
    RelayFactoryBindings rfb{};rfb.context=&rff;
    rfb.allocate=[](void* p,unsigned n,void** a,unsigned* bytes) noexcept ->int{
        auto& f=*static_cast<RelayFactoryFixture*>(p);assert(n==relay_extent);
        ++f.alloc;*a=f.storage;*bytes=f.short_alloc?n-1:n;return 1;
    };
    rfb.base_construct=[](void* p,void* a,int arg) noexcept ->int{
        auto& f=*static_cast<RelayFactoryFixture*>(p);assert(a==f.storage&&arg==1);
        ++f.base;unsigned untouched=0x12345678;std::memcpy(f.storage+0x398,&untouched,4);return 1;
    };
    rfb.initialize=[](void* p,void* a,const char* cls) noexcept ->int{
        auto& f=*static_cast<RelayFactoryFixture*>(p);assert(a==f.storage&&!std::strcmp(cls,"logic_relay")&&f.base==1);
        unsigned vt=0,invalid=0;std::memcpy(&vt,f.storage,4);std::memcpy(&invalid,f.storage+0x3a0,4);
        assert(vt==relay_vtable&&invalid==0xffffffffu);++f.init;return 1;
    };
    rfb.release_networkable=[](void* p,void* a) noexcept ->int{
        auto& f=*static_cast<RelayFactoryFixture*>(p);assert(a==f.storage+0x160);++f.released;return 1;
    };
    factories.relay=&rfb;
    rff.short_alloc=true;assert(!api->create_factory("logic_relay",&factories,&fr)&&fr.matched&&fr.actor==rff.storage&&!fr.networkable&&!rff.base);
    rff.short_alloc=false;
    ActionPool relay_pool{};OutputActionBindings rpo=output;rpo.context=&relay_pool;rpo.next_id=&relay_pool.id;
    auto relay_keys=kb;relay_keys.native_output=&rpo;
    relay_keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=relay_extent;return 1;};
    Bindings relay_parse{};relay_parse.context=&rff;relay_parse.native_factory=&factories;relay_parse.native_keys=&relay_keys;
    relay_parse.clear_factory_global=[](void* p) noexcept ->int{++static_cast<RelayFactoryFixture*>(p)->cleared;return 1;};
    relay_parse.accept_entity=[](void* p,void* a,const char* text,unsigned bytes,const EntitySlice* slice) noexcept ->int{
        auto& f=*static_cast<RelayFactoryFixture*>(p);assert(a==f.storage&&text&&slice&&slice->begin<slice->end&&slice->end<=bytes);
        constexpr char prefix[]="{ classname logic_relay";
        assert(slice->end-slice->begin>=sizeof(prefix)-1&&!std::memcmp(text+slice->begin,prefix,sizeof(prefix)-1));++f.accepted;return 1;
    };
    const char relay_map_text[]="{ classname logic_relay StartDisabled 1 targetname station OnTrigger \"relay,Trigger,,2,-1\" }";
    assert(api->parse(relay_map_text,sizeof(relay_map_text),&relay_parse,&r)&&r.native_creates==1&&r.created==1&&r.last_factory.networkable==rff.storage+0x160);
    assert(rff.alloc==2&&rff.base==1&&rff.init==1&&rff.cleared==1&&rff.accepted==1&&rff.storage[0x3ac]==1&&!rff.storage[0x3ad]);
    assert(word(rff.storage,0x394)==0&&word(rff.storage,0x398)==0x12345678&&word(rff.storage,0x3a0)==0xffffffffu&&word(rff.storage,0x3a8)==ptrword(relay_pool.rows[0]));
    assert(api->destroy_relay(nullptr,0,nullptr,nullptr)&&!api->destroy_relay(rff.storage,relay_extent,rff.storage+0x164,&rfb)&&!rff.released);
    assert(api->destroy_relay(rff.storage,relay_extent,rff.storage+0x160,&rfb)&&rff.released==1);
    // CLogicAuto actual datamap outputs and virtual activation/think leaves.
    alignas(4) unsigned char auto_actor[auto_extent]{};
    put(auto_actor,0,auto_vtable);
    ActionPool auto_pool{};OutputActionBindings apo=output;apo.context=&auto_pool;apo.next_id=&auto_pool.id;
    auto ak=kb;ak.native_output=&apo;
    ak.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=auto_extent;return 1;};
    constexpr const char* auto_names[]{"OnMapSpawn","OnViolenceLow","OnViolenceHigh","OnNewGame","OnLoadGame","OnMapTransition","OnBackgroundMap"};
    constexpr unsigned auto_offsets[]{0x394,0x3ac,0x3c4,0x3dc,0x3f4,0x40c,0x424};
    for(unsigned i=0;i<7;++i){
        assert(api->key_value(auto_actor,auto_names[i],"relay,Trigger,,0,-1",&ak,&kr)&&kr.output&&kr.offset==auto_offsets[i]);
        assert(word(auto_actor,auto_offsets[i]+0x14)==ptrword(auto_pool.rows[i]));
    }
    assert(api->key_value(auto_actor,"globalstate","campaign_auto_gate",&ak,&kr)&&kr.interned&&word(auto_actor,0x43c)==300);
    struct AutoFixture {
        ActionPool* pool;void* actor;float now=10.f,next=0;int load=0;bool low=false;
        unsigned base=0,schedules=0,removes=0,violence=0;std::string order;
    } af{&auto_pool,auto_actor};
    nw::ServerEntityVoidOutputBindings ao{};ao.context=&af;
    ao.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
        auto& f=*static_cast<AutoFixture*>(p);
        for(unsigned i=0;i<7;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[i]))){*row=f.pool->rows[i];*bytes=0x1c;return 1;}return 0;
    };
    ao.enqueue=[](void* p,void* a,void* c,const nw::ServerEntityVoidOutputAction* act) noexcept ->int{
        auto& f=*static_cast<AutoFixture*>(p);assert(!a&&c==f.actor&&act->action_id>=41&&act->action_id<=47);
        f.order+=char('0'+act->action_id-41);return 1;
    };
    AutoBindings ab{};ab.context=&af;ab.output=&ao;ab.now=&af.now;ab.load_type=&af.load;
    ab.base_activate=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<AutoFixture*>(p);assert(a==f.actor);++f.base;f.now=12.f;return 1;};
    ab.schedule=[](void* p,void* a,float time) noexcept ->int{auto& f=*static_cast<AutoFixture*>(p);assert(a==f.actor&&f.base);f.next=time;++f.schedules;return 1;};
    ab.resolve_string=[](void*,unsigned token,const char** text) noexcept ->int{assert(token==300);*text="campaign_auto_gate";return 1;};
    ab.low_violence=[](void* p,bool* low) noexcept ->int{auto& f=*static_cast<AutoFixture*>(p);++f.violence;*low=f.low;return 1;};
    ab.remove=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<AutoFixture*>(p);assert(a==f.actor);++f.removes;f.order+='R';return 1;};
    AutoResult ar{};
    assert(api->auto_activate(auto_actor,sizeof(auto_actor),&ab,&ar)&&ar.base_called&&ar.scheduled&&ar.next_think==static_cast<float>(12.2)&&af.next==ar.next_think);
    assert(globals->clear());
    assert(api->auto_think(auto_actor,sizeof(auto_actor),&ab,&ar)&&ar.blocked&&ar.global_checked&&!ar.outputs&&!af.violence);
    const int gate=globals->add("campaign_auto_gate","se1_intro01",0);assert(gate>=0);
    assert(api->auto_think(auto_actor,sizeof(auto_actor),&ab,&ar)&&ar.blocked&&!ar.outputs);
    assert(globals->set_state(gate,2)&&api->auto_think(auto_actor,sizeof(auto_actor),&ab,&ar)&&ar.blocked);
    assert(globals->set_state(gate,1));
    for(int load=0;load<4;++load){
        af.load=load;af.low=(load&1)!=0;af.order.clear();
        assert(api->auto_think(auto_actor,sizeof(auto_actor),&ab,&ar)&&ar.completed&&!ar.blocked&&ar.outputs==3&&ar.queued==3&&ar.load_output==auto_offsets[load+3]);
        std::string expected;expected+=char('3'+load);expected+='0';expected+=af.low?'1':'2';assert(af.order==expected);
    }
    put(auto_actor,0x43c,0);put(auto_actor,0xb4,1);af.load=99;af.low=true;af.order.clear();
    assert(api->auto_think(auto_actor,sizeof(auto_actor),&ab,&ar)&&!ar.global_checked&&!ar.load_output&&ar.outputs==2&&ar.removed&&af.order=="01R"&&af.removes==1);
    assert(globals->clear());
    // Real logic_auto factory, independent optional registration order,
    // map parsing, ordinary activation and scheduled think leaf.
    FactoryDictionary auto_first{};assert(api->register_factories(&auto_first)&&api->register_auto(&auto_first)&&auto_first.count==13);
    assert(api->find_factory(&auto_first,"logic_auto")&&!api->find_factory(&auto_first,"logic_relay"));
    assert(api->register_relay(&auto_first)&&auto_first.count==14&&!api->register_auto(&auto_first));
    assert(api->register_auto(&dictionary)&&dictionary.count==14&&!api->register_auto(&dictionary));
    const auto* autospec=api->find_factory(&dictionary,"LOGIC_AUTO");
    assert(autospec&&autospec->family==Family::LogicAuto&&autospec->original_object==0x224fb288&&autospec->original_vtable==0x2241efcc&&autospec->extent==auto_extent);
    assert(!api->create_factory("logic_auto",&factories,&fr)&&fr.matched&&!fr.actor);
    struct AutoFactoryFixture {
        alignas(4) unsigned char storage[auto_extent]{};
        unsigned alloc=0,base=0,init=0,released=0,cleared=0,spawned=0;
        bool short_alloc=false;
    } aff;
    RelayFactoryBindings afb{};afb.context=&aff;
    afb.allocate=[](void* p,unsigned n,void** a,unsigned* bytes) noexcept ->int{
        auto& f=*static_cast<AutoFactoryFixture*>(p);assert(n==auto_extent);++f.alloc;*a=f.storage;*bytes=f.short_alloc?n-1:n;return 1;
    };
    afb.base_construct=[](void* p,void* a,int arg) noexcept ->int{
        auto& f=*static_cast<AutoFactoryFixture*>(p);assert(a==f.storage&&arg==0);++f.base;
        const unsigned untouched=0x12345678;std::memcpy(f.storage+0x398,&untouched,4);return 1;
    };
    afb.initialize=[](void* p,void* a,const char* cls) noexcept ->int{
        auto& f=*static_cast<AutoFactoryFixture*>(p);assert(a==f.storage&&!std::strcmp(cls,"logic_auto")&&f.base==1);
        unsigned vt=0;std::memcpy(&vt,f.storage,4);assert(vt==auto_vtable);++f.init;return 1;
    };
    afb.release_networkable=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<AutoFactoryFixture*>(p);assert(a==f.storage+0x160);++f.released;return 1;};
    factories.logic_auto=&afb;
    aff.short_alloc=true;assert(!api->create_factory("logic_auto",&factories,&fr)&&fr.matched&&fr.actor==aff.storage&&!fr.networkable&&!aff.base);
    aff.short_alloc=false;
    ActionPool auto_map_pool{};OutputActionBindings ampo=output;ampo.context=&auto_map_pool;ampo.next_id=&auto_map_pool.id;
    auto amk=ak;amk.native_output=&ampo;
    Bindings amp{};amp.context=&aff;amp.native_factory=&factories;amp.native_keys=&amk;
    amp.clear_factory_global=[](void* p) noexcept ->int{++static_cast<AutoFactoryFixture*>(p)->cleared;return 1;};
    af.actor=aff.storage;af.pool=&auto_map_pool;af.load=0;af.low=false;af.order.clear();
    SpawnBindings ams{};ams.context=&aff;ams.logic_auto=&ab;
    ams.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=auto_extent;return 1;};
    ams.classname=[](void*,void*,const char** cls) noexcept ->int{*cls="logic_auto";return 1;};
    ams.scope=[](void*,const char*,bool) noexcept ->int{return 1;};
    ams.debug_enabled=[](void*,bool* enabled) noexcept ->int{*enabled=false;return 1;};
    ams.engine_scope=[](void*,bool) noexcept ->int{return 1;};
    ams.spawn=[](void* p,void* a,int* code) noexcept ->int{auto& f=*static_cast<AutoFactoryFixture*>(p);assert(a==f.storage);++f.spawned;*code=0;return 1;};
    PipelineBindings ampipe{};ampipe.context=&aff;ampipe.parse=&amp;ampipe.spawn=&ams;
    ampipe.scope=[](void*,const char*,bool) noexcept ->int{return 1;};
    ampipe.classify=[](void*,void*,Category* cat) noexcept ->int{*cat=Category::Ordinary;return 1;};
    const char auto_map_text[]="{ classname logic_auto OnMapSpawn \"relay,Trigger,,0,-1\" OnNewGame \"relay,Trigger,,0,-1\" }";
    PipelineResult amr{};
    assert(api->parse_all(auto_map_text,sizeof(auto_map_text),true,&ampipe,&amr)&&amr.parse.native_creates==1&&amr.spawn.native_activations==1&&amr.spawn.spawned==1);
    assert(aff.alloc==2&&aff.base==1&&aff.init==1&&aff.cleared==1&&aff.spawned==1&&af.base==2&&af.schedules==2);
    for(unsigned off=0x394;off<=0x424;off+=0x18)assert(word(aff.storage,off)==0&&word(aff.storage,off+0xc)==0xffffffffu&&word(aff.storage,off+0x10)==0);
    assert(word(aff.storage,0x398)==0x12345678&&word(aff.storage,0x3a8)==ptrword(auto_map_pool.rows[0])&&word(aff.storage,0x3f0)==ptrword(auto_map_pool.rows[1]));
    assert(api->auto_think(aff.storage,sizeof(aff.storage),&ab,&ar)&&ar.outputs==3&&ar.queued==2&&af.order=="10");
    assert(api->destroy_auto(nullptr,0,nullptr,nullptr)&&!api->destroy_auto(aff.storage,auto_extent,aff.storage+0x164,&afb)&&!aff.released);
    assert(api->destroy_auto(aff.storage,auto_extent,aff.storage+0x160,&afb)&&aff.released==1);
    // Full outer map parser: immediate special branches, delayed point
    // template pass, captured ordinary row removal, then ordinary spawn.
    struct PipelineFixture {
        alignas(4) unsigned char actors[7][0x170]{};
        unsigned next=0,scopes=0,closes=0;std::string order;
        unsigned index(void* a)const noexcept {for(unsigned i=0;i<7;++i)if(a==actors[i])return i;assert(false);return 99;}
    } pf;
    const char outer_map[]="{ classname worldspawn } { classname node } { classname light } { classname template_entity } { classname ordinary } { classname point_template } { classname ordinary2 }";
    put(pf.actors[0],0x14,99);
    sin_port::native_map::Bindings pb{};pb.context=&pf;
    pb.create=[](void* p,const char*,void** a) noexcept ->int{auto& f=*static_cast<PipelineFixture*>(p);assert(f.next<7);*a=f.actors[f.next++];return 1;};
    pb.clear_factory_global=[](void*) noexcept ->int{return 1;};
    pb.key_value=[](void*,void*,const char*,const char*) noexcept ->int{return 1;};
    SpawnBindings ps{};ps.context=&pf;
    ps.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=0x170;return 1;};
    ps.classname=[](void*,void*,const char** cls) noexcept ->int{*cls="logic_relay";return 1;};
    ps.scope=[](void*,const char*,bool) noexcept ->int{return 1;};
    ps.debug_enabled=[](void*,bool* value) noexcept ->int{*value=false;return 1;};
    ps.spawn=[](void* p,void* a,int* code) noexcept ->int{auto& f=*static_cast<PipelineFixture*>(p);f.order+=char('0'+f.index(a));*code=0;return 1;};
    ps.engine_scope=[](void*,bool) noexcept ->int{return 1;};
    ps.activate=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<PipelineFixture*>(p);f.order+=char('A'+f.index(a));return 1;};
    PipelineBindings pipe{};pipe.context=&pf;pipe.parse=&pb;pipe.spawn=&ps;
    pipe.scope=[](void* p,const char*,bool enter) noexcept ->int{auto& f=*static_cast<PipelineFixture*>(p);if(enter)++f.scopes;else ++f.closes;return 1;};
    pipe.classify=[](void* p,void* a,Category* cat) noexcept ->int{auto& f=*static_cast<PipelineFixture*>(p);constexpr Category cats[]{Category::World,Category::Node,Category::Light,Category::TemplateEntity,Category::Ordinary,Category::PointTemplate,Category::Ordinary};*cat=cats[f.index(a)];return 1;};
    pipe.spawn_node=[](void* p,void* a,const char*,unsigned,const EntitySlice*,int* code) noexcept ->int{auto& f=*static_cast<PipelineFixture*>(p);assert(a==f.actors[1]);f.order+='N';*code=0;return 1;};
    pipe.capture_template=[](void* p,void* a,const char* s,unsigned n,const EntitySlice* e) noexcept ->int{auto& f=*static_cast<PipelineFixture*>(p);assert(a==f.actors[3]&&e->end<=n&&s[e->begin]=='{');f.order+='T';return 1;};
    pipe.precache=[](void* p,void*) noexcept ->int{static_cast<PipelineFixture*>(p)->order+='P';return 1;};
    pipe.remove=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<PipelineFixture*>(p);f.order+=char('a'+f.index(a));return 1;};
    pipe.cleanup=[](void* p) noexcept ->int{static_cast<PipelineFixture*>(p)->order+='C';return 1;};
    pipe.template_prepare=[](void* p,void* a,int* count) noexcept ->int{auto& f=*static_cast<PipelineFixture*>(p);assert(a==f.actors[5]&&f.next==7);f.order+='Q';*count=1;return 1;};
    pipe.template_member=[](void* p,void*,int i,void** a) noexcept ->int{assert(!i);*a=static_cast<PipelineFixture*>(p)->actors[4];return 1;};
    pipe.template_capture_member=[](void* p,void*,void* member,const char* s,unsigned,const EntitySlice* e,bool* remove) noexcept ->int{auto& f=*static_cast<PipelineFixture*>(p);assert(member==f.actors[4]&&!std::strncmp(s+e->begin,"{ classname ordinary }",22));f.order+='R';*remove=true;return 1;};
    pipe.template_finish=[](void* p,void*) noexcept ->int{static_cast<PipelineFixture*>(p)->order+='F';return 1;};
    PipelineResult pr{};
    assert(api->parse_all(outer_map,sizeof(outer_map),true,&pipe,&pr)&&pr.completed&&pr.parse.created==7&&pr.worlds==1&&pr.nodes==1&&pr.lights==1&&pr.template_entities==1&&pr.point_templates==1&&pr.template_members==1&&pr.captured==1&&pr.ordinary==2&&pr.spawn.spawned==1&&pr.spawn.activated==1&&pr.removed==2&&pr.cleanups==2&&pr.scopes==pr.closes);
    assert(word(pf.actors[0],0x14)==0&&pf.order=="0N2TPdC5QReCF6G");
    // Original CLogicTimer field types, fixed/random scheduling, alternating
    // output ordering and input reschedule semantics.
    alignas(4) unsigned char timer[timer_extent]{};
    put(timer,0,timer_vtable);
    ActionPool timer_pool{};OutputActionBindings tpo=output;tpo.context=&timer_pool;tpo.next_id=&timer_pool.id;
    auto tk=kb;tk.native_output=&tpo;
    tk.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=timer_extent;return 1;};
    assert(api->key_value(timer,"StartDisabled","-2",&tk,&kr)&&kr.field_type==5&&word(timer,0x3dc)==0xfffffffeu);
    assert(api->key_value(timer,"UseRandomTime","3",&tk,&kr)&&word(timer,0x3e8)==3);
    assert(api->key_value(timer,"LowerRandomBound","4",&tk,&kr)&&word(timer,0x3ec)==0x40800000);
    assert(api->key_value(timer,"UpperRandomBound","2",&tk,&kr)&&word(timer,0x3f0)==0x40000000);
    assert(api->key_value(timer,"RefireTime","0",&tk,&kr)&&!word(timer,0x3e0));
    constexpr const char* timer_names[]{"OnTimer","OnTimerHigh","OnTimerLow"};
    for(unsigned i=0;i<3;++i)assert(api->key_value(timer,timer_names[i],"relay,Trigger,,0,-1",&tk,&kr)&&kr.offset==0x394+i*0x18);
    struct TimerFixture {void* actor;ActionPool* pool;float now=10.f,next=0;unsigned schedules=0,randoms=0;std::string order;} tf{timer,&timer_pool};
    nw::ServerEntityVoidOutputBindings to{};to.context=&tf;
    to.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
        auto& f=*static_cast<TimerFixture*>(p);for(unsigned i=0;i<3;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[i]))){*row=f.pool->rows[i];*bytes=0x1c;return 1;}return 0;
    };
    to.enqueue=[](void* p,void* a,void* c,const nw::ServerEntityVoidOutputAction* act) noexcept ->int{
        auto& f=*static_cast<TimerFixture*>(p);assert(a==f.actor&&c==f.actor&&act->action_id>=41&&act->action_id<=43);f.order+=char('0'+act->action_id-41);return 1;
    };
    TimerBindings tb{};tb.context=&tf;tb.output=&to;tb.now=&tf.now;
    tb.random=[](void* p,float lower,float upper,float* value) noexcept ->int{auto& f=*static_cast<TimerFixture*>(p);assert(lower==4.f&&upper==2.f);++f.randoms;*value=1.5f;return 1;};
    tb.schedule=[](void* p,void* a,float time) noexcept ->int{auto& f=*static_cast<TimerFixture*>(p);assert(a==f.actor);f.next=time;++f.schedules;return 1;};
    TimerResult tr{};
    auto ti=[&](const char* name,unsigned type=0,float value=0.f){return api->timer_input(timer,sizeof(timer),name,type,value,&tb,&tr);};
    assert(api->timer_spawn(timer,sizeof(timer),&tb,&tr)&&tr.scheduled&&tf.next==-1.f&&!tf.randoms&&word(timer,0x3dc)==1);
    assert(ti("FireTimer")&&tr.suppressed&&!tr.fired);
    assert(ti("Enable")&&tr.randomized&&tf.next==11.5f&&tf.randoms==1&&!word(timer,0x3dc));
    assert(ti("FireTimer")&&tr.fired&&tr.output_offset==0x394&&tf.order=="0"&&tf.randoms==2);
    put(timer,0xb4,1);assert(ti("FireTimer")&&tr.output_offset==0x3c4&&timer[0x3e4]==1&&tf.order=="02");
    assert(ti("FireTimer")&&tr.output_offset==0x3ac&&!timer[0x3e4]&&tf.order=="021");
    assert(ti("Toggle")&&tf.next==-1.f&&word(timer,0x3dc)==1);
    put(timer,0x3e8,0);assert(ti("RefireTime",1,2.f)&&tr.changed&&tr.suppressed&&!tr.scheduled&&word(timer,0x3e0)==0x40000000);
    assert(ti("Toggle")&&tf.next==12.f);
    const auto schedules=tf.schedules;
    assert(ti("RefireTime",1,2.f)&&!tr.changed&&!tr.scheduled&&tf.schedules==schedules);
    assert(ti("RefireTime",1,-1.f)&&tr.clamped&&tr.changed&&tf.next==static_cast<float>(10.0+double(.01f)));
    assert(ti("RefireTime",5,100.f)&&tr.clamped&&!tr.changed);
    assert(ti("Disable")&&tf.next==-1.f);put(timer,0x3dc,0);put(timer,0x3e0,0);
    assert(api->timer_spawn(timer,sizeof(timer),&tb,&tr)&&tr.clamped&&tr.scheduled&&tf.next==static_cast<float>(10.0+double(.01f)));
    assert(ti("Unknown")&&!tr.handled);
    // Optional timer factory and map-to-native-spawn route.
    FactoryDictionary timer_dictionary{};
    assert(api->register_factories(&timer_dictionary)&&api->register_timer(&timer_dictionary)&&timer_dictionary.count==13);
    assert(api->register_auto(&timer_dictionary)&&api->register_relay(&timer_dictionary)&&timer_dictionary.count==15&&!api->register_timer(&timer_dictionary));
    const auto* tspec=api->find_factory(&timer_dictionary,"LOGIC_TIMER");
    assert(tspec&&tspec->family==Family::LogicTimer&&tspec->original_object==0x224fb604&&tspec->original_vtable==0x2241f590&&tspec->extent==timer_extent);
    struct TimerFactoryFixture {
        alignas(4) unsigned char storage[timer_extent]{};
        unsigned alloc=0,base=0,init=0,released=0,cleared=0,spawn_begin=0,spawn_end=0;
        bool short_alloc=false;
    } tff;
    RelayFactoryBindings tfb{};tfb.context=&tff;
    tfb.allocate=[](void* p,unsigned n,void** a,unsigned* bytes) noexcept ->int{
        auto& f=*static_cast<TimerFactoryFixture*>(p);assert(n==timer_extent);++f.alloc;*a=f.storage;*bytes=f.short_alloc?n-1:n;return 1;
    };
    tfb.base_construct=[](void* p,void* a,int arg) noexcept ->int{
        auto& f=*static_cast<TimerFactoryFixture*>(p);assert(a==f.storage&&arg==1);++f.base;
        unsigned keep=0x12345678;std::memcpy(f.storage+0x398,&keep,4);return 1;
    };
    tfb.initialize=[](void* p,void* a,const char* cls) noexcept ->int{
        auto& f=*static_cast<TimerFactoryFixture*>(p);assert(a==f.storage&&f.base==1&&!std::strcmp(cls,"logic_timer"));
        unsigned vt=0;std::memcpy(&vt,a,4);assert(vt==timer_vtable);++f.init;return 1;
    };
    tfb.release_networkable=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<TimerFactoryFixture*>(p);assert(a==f.storage+0x160);++f.released;return 1;};
    FactoryBindings timer_factories{};timer_factories.dictionary=&timer_dictionary;
    FactoryResult tfr{};
    assert(!api->create_factory("logic_timer",&timer_factories,&tfr)&&tfr.matched&&!tfr.actor);
    timer_factories.logic_timer=&tfb;tff.short_alloc=true;
    assert(!api->create_factory("logic_timer",&timer_factories,&tfr)&&tfr.matched&&!tfr.networkable&&!tff.base);tff.short_alloc=false;
    ActionPool timer_map_pool{};OutputActionBindings tmpo=output;tmpo.context=&timer_map_pool;tmpo.next_id=&timer_map_pool.id;
    auto tmk=tk;tmk.native_output=&tmpo;
    Bindings tmp{};tmp.context=&tff;tmp.native_factory=&timer_factories;tmp.native_keys=&tmk;
    tmp.clear_factory_global=[](void* p) noexcept ->int{++static_cast<TimerFactoryFixture*>(p)->cleared;return 1;};
    TimerFixture tmf{tff.storage,&timer_map_pool};auto tmb=tb;tmb.context=&tmf;
    auto tmo=to;tmo.context=&tmf;tmb.output=&tmo;tmb.now=&tmf.now;
    SpawnBindings tms{};tms.context=&tff;tms.logic_timer=&tmb;
    tms.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=timer_extent;return 1;};
    tms.classname=[](void*,void*,const char** cls) noexcept ->int{*cls="logic_timer";return 1;};
    tms.scope=[](void*,const char*,bool) noexcept ->int{return 1;};
    tms.debug_enabled=[](void*,bool* enabled) noexcept ->int{*enabled=false;return 1;};
    tms.native_spawn_begin=[](void* p,void* a,bool* proceed) noexcept ->int{auto& f=*static_cast<TimerFactoryFixture*>(p);assert(a==f.storage);++f.spawn_begin;*proceed=true;return 1;};
    tms.native_spawn_end=[](void* p,void* a,int* code) noexcept ->int{auto& f=*static_cast<TimerFactoryFixture*>(p);assert(a==f.storage);++f.spawn_end;*code=0;return 1;};
    PipelineBindings tmpipe{};tmpipe.context=&tff;tmpipe.parse=&tmp;tmpipe.spawn=&tms;
    tmpipe.scope=[](void*,const char*,bool) noexcept ->int{return 1;};
    tmpipe.classify=[](void*,void*,Category* cat) noexcept ->int{*cat=Category::Ordinary;return 1;};
    const char timer_map[]="{ classname logic_timer RefireTime 2 OnTimer \"relay,Trigger,,0,-1\" }";
    PipelineResult tmpr{};
    assert(api->parse_all(timer_map,sizeof(timer_map),false,&tmpipe,&tmpr)&&tmpr.parse.native_creates==1&&tmpr.spawn.native_spawns==1&&tmpr.spawn.spawned==1);
    assert(tff.alloc==2&&tff.base==1&&tff.init==1&&tff.cleared==1&&tff.spawn_begin==1&&tff.spawn_end==1&&tmf.schedules==1&&tmf.next==12.f);
    for(unsigned off=0x394;off<=0x3c4;off+=0x18)assert(word(tff.storage,off)==0&&word(tff.storage,off+0xc)==0xffffffffu&&word(tff.storage,off+0x10)==0);
    assert(word(tff.storage,0x398)==0x12345678&&word(tff.storage,0x3a8)==ptrword(timer_map_pool.rows[0]));
    assert(api->timer_input(tff.storage,sizeof(tff.storage),"FireTimer",0,0,&tmb,&tr)&&tr.fired&&tmf.order=="0");
    assert(api->destroy_timer(nullptr,0,nullptr,nullptr)&&!api->destroy_timer(tff.storage,timer_extent,tff.storage+0x164,&tfb)&&!tff.released);
    assert(api->destroy_timer(tff.storage,timer_extent,tff.storage+0x160,&tfb)&&tff.released==1);
    // Generic AcceptInput must distinguish timer handler and direct-field
    // descriptors, and use the original variant conversion / dirty path.
    const auto timer_accept_get=reinterpret_cast<nw::GetServerEntityAcceptInputApi>(GetProcAddress(server,nw::server_entity_accept_input_export));
    assert(timer_accept_get);const auto* timer_accept=timer_accept_get(1,sizeof(nw::ServerEntityAcceptInputApi));assert(timer_accept);
    nw::ServerNpcActivePostAnimationBindings tstorage{};
    nw::ServerEntityAcceptInputBindings tai{};tai.storage=&tstorage;tai.timer=&tb;
    tai.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
        switch(token){
            case 301:case 0x22420054:*text="FireTimer";break;
            case 302:*text="-3";break;
            case 303:*text="  +2.5e-1 seconds";break;
            case 304:*text="1.25D+1 trailing";break;
            case 305:*text="garbage";break;
            case 306:*text="-0.0";break;
            case 307:*text="0x1p+2";break;
            case 308:*text="3.5e+";break;
            case 309:*text="1e9999";break;
            case 310:*text="-1e-9999";break;
            case 0x22420090:*text="RefireTime";break;
            case 0x223edd00:*text="Enable";break;
            case 0x223edce8:*text="Disable";break;
            case 0x223d4914:*text="Toggle";break;
            case 0x22420030:*text="UseRandomTime";break;
            case 0x22420004:*text="LowerRandomBound";break;
            case 0x2241ffd8:*text="UpperRandomBound";break;
            default:return 0;
        }
        *bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
    };
    nw::ServerEntityAcceptInputResult tair{};
    unsigned tv[5]{};
    auto timer_ai=[&](unsigned token,unsigned type,unsigned value){
        tv[0]=value;tv[4]=type;timer[0x1f0]=1;timer[0x200]=0;
        return timer_accept->run(timer,sizeof(timer),timer_vtable,token,nullptr,nullptr,tv,sizeof(tv),0,&tai,&tair);
    };
    const auto before_inputs=tf.schedules;
    assert(timer_ai(0x22420030,2,302)&&tair.accepted&&tair.converted&&tair.written==4&&tair.row==8&&word(timer,0x3e8)==0xfffffffdu&&timer[0x200]&&tf.schedules==before_inputs);
    assert(timer_ai(0x22420004,5,4)&&tair.written==4&&tair.converted&&word(timer,0x3ec)==0x40800000);
    assert(timer_ai(0x2241ffd8,1,0x40000000)&&tair.written==4&&word(timer,0x3f0)==0x40000000&&tf.schedules==before_inputs);
    assert(timer_ai(0x223edce8,0,0)&&tair.native_handler&&word(timer,0x3dc)==1&&tf.next==-1.f);
    assert(timer_ai(0x22420090,5,3)&&tair.converted&&tair.native_handler&&word(timer,0x3e0)==0x40400000&&word(timer,0x3dc)==1);
    assert(timer_ai(0x223edd00,0,0)&&tair.native_handler&&tf.next==11.5f);
    assert(timer_ai(301,2,302)&&tair.converted&&tair.native_handler&&tair.row==4&&timer[0x200]);
    // Actual queue delivery to the timer with a live event-service clock.
    std::memset(co.root,0,sizeof(co.root));std::memset(co.rows,0,sizeof(co.rows));co.alloc=0;co.released=0;co.now=55.f;co.actor=timer;
    co.queue.string_nonempty=[](void*,unsigned,bool* yes) noexcept ->int{*yes=true;return 1;};
    sin_independent::EntityHandleEntry te{};te.entity=reinterpret_cast<CBaseEntity*>(timer);te.serial=3;
    auto tes=sb;tes.entities.entries=&te;tes.entities.count=1;tes.native_input=&tai;tes.live_now=&co.now;
    nw::ServerEntitySearchBindings tsearch{};tsearch.context=&co;
    tsearch.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{*bytes=a==static_cast<ConnectedOutput*>(p)->actor?timer_extent:0;return *bytes!=0;};
    tes.native_search=&tsearch;
    unsigned void_variant[5]{0,0,0,0xffffffffu,0};
    assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x3000,0x223edd00,0x3000,0x3000,void_variant,sizeof(void_variant),0.f,0,&co.queue,&cancel_qr));
    assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&tes,&es)&&es.native_inputs==1&&es.direct==1&&tf.next==56.5f&&co.released==1);
    // Native recurring timer: schedule, due admission, default clear,
    // reschedule, input mutations and serial rejection.
    put(timer,0x378,0x3000u);put(timer,0x74,-1);put(timer,0xb8,0x400000u);
    put(timer,0x3dc,0);put(timer,0x3e8,0);put(timer,0x3e0,0x3e800000u); // .25
    put(timer,0xb4,1);timer[0x3e4]=0;put(timer,8,0xdeadbeefu);
    tf.now=60.f;tf.order.clear();
    struct TimerPrimaryFixture {void* actor;unsigned notifications=0,records=0,enters=0,leaves=0;float at=0;int tick=0,due=0;} tpf{timer};
    nw::ServerNpcOuterCadenceBindings tcad{};tcad.context=&tpf;
    tcad.notify_think_flags=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<TimerPrimaryFixture*>(p);assert(a==f.actor);++f.notifications;return 1;};
    float tick_interval=.125f;
    tb.cadence=&tcad;tb.entries=&te;tb.entry_count=1;tb.tick_interval=&tick_interval;
    TimerPrimaryBindings tpb{};tpb.behavior=&tb;tpb.context=&tpf;
    tpb.record=[](void* p,void* a,int tick,float time,int due) noexcept ->int{auto& f=*static_cast<TimerPrimaryFixture*>(p);assert(a==f.actor);++f.records;f.at=time;f.tick=tick;f.due=due;return 1;};
    tpb.scope=[](void* p,void* a,bool enter) noexcept ->int{auto& f=*static_cast<TimerPrimaryFixture*>(p);assert(a==f.actor);if(enter)++f.enters;else ++f.leaves;return 1;};
    TimerPrimaryResult tpr{};
    const auto hosted_count=tf.schedules;
    assert(api->timer_spawn(timer,sizeof(timer),&tb,&tr)&&tr.scheduled&&word(timer,0x74)==482&&tpf.notifications==1&&tf.schedules==hosted_count);
    assert(api->timer_primary_run(timer,sizeof(timer),481,&tpb,&tpr)&&tpr.not_due&&tpr.next_tick==482&&!tpf.records);
    tf.now=60.25f;
    assert(api->timer_primary_run(timer,sizeof(timer),482,&tpb,&tpr)&&tpr.invoked&&tpr.behavior.fired&&tpr.behavior.output_offset==0x3c4&&tpr.next_tick==484&&tpr.last_tick==482&&tf.order=="2"&&tpf.notifications==3&&tpf.records==1&&tpf.at==60.25f);
    assert(api->timer_primary_run(timer,sizeof(timer),482,&tpb,&tpr)&&tpr.not_due&&tf.order=="2");
    tf.now=60.5f;
    assert(api->timer_primary_run(timer,sizeof(timer),484,&tpb,&tpr)&&tpr.invoked&&tpr.next_tick==486&&tf.order=="21"&&tpf.records==2&&tpf.enters==2&&tpf.leaves==2);
    assert(timer_ai(0x22420090,1,0x3f800000)&&tair.native_handler&&word(timer,0x74)==492);
    assert(timer_ai(0x223edce8,0,0)&&word(timer,0x74)==0xffffffffu);
    assert(api->timer_primary_run(timer,sizeof(timer),492,&tpb,&tpr)&&tpr.no_think&&tpf.records==2);
    // Event queue owns registry and live clock; the timer native cadence
    // must use both rather than the old hosted schedule callback.
    co.now=70.f;
    assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x3000,0x223edd00,0x3000,0x3000,void_variant,sizeof(void_variant),0.f,0,&co.queue,&cancel_qr));
    assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&tes,&es)&&es.native_inputs==1&&word(timer,0x74)==568&&tf.schedules==hosted_count);
    te.serial=4;
    assert(!api->timer_primary_run(timer,sizeof(timer),568,&tpb,&tpr)&&!tpr.completed);
    assert(!timer_ai(0x22420030,5,1)&&!word(timer,0x3e8));
    // Real map parameter strings reach the FLOAT input without a hosted
    // converter. Preserve decimal prefix, old D exponent and null token.
    te.serial=3;tf.now=80.f;
    assert(timer_ai(0x223edce8,0,0)&&word(timer,0x74)==0xffffffffu);
    const int decimal_status=timer_ai(0x22420090,2,303);
    std::fprintf(stderr,"decimal input status=%d completed=%u accepted=%u maps=%u row=%u handler=%08x expected=%u actual=%u converted=%u native=%u reason=%u state=%08x disabled=%08x\n",
        decimal_status,tair.completed,tair.accepted,tair.maps,tair.row,tair.handler,tair.expected_type,tair.actual_type,tair.converted,tair.native_handler,tair.reason,word(timer,0x3e0),word(timer,0x3dc));
    assert(decimal_status&&tair.converted&&tair.native_handler&&word(timer,0x3e0)==0x3e800000u);
    assert(timer_ai(0x22420004,2,304)&&tair.converted&&tair.written==4&&word(timer,0x3ec)==0x41480000u);
    assert(timer_ai(0x22420004,2,305)&&tair.written==4&&!word(timer,0x3ec));
    assert(timer_ai(0x22420004,2,306)&&word(timer,0x3ec)==0x80000000u);
    assert(timer_ai(0x22420004,2,307)&&!word(timer,0x3ec));
    assert(timer_ai(0x22420004,2,308)&&word(timer,0x3ec)==0x40600000u);
    assert(timer_ai(0x22420004,2,309)&&word(timer,0x3ec)==0x7f800000u);
    assert(timer_ai(0x22420004,2,310)&&word(timer,0x3ec)==0x80000000u);
    assert(timer_ai(0x22420004,2,0)&&!word(timer,0x3ec));
    // Queue named action with parameter override (FIELD_STRING) from an
    // actual map output row; native service converts and re-arms timer.
    std::memset(co.root,0,sizeof(co.root));std::memset(co.rows,0,sizeof(co.rows));co.alloc=0;co.released=0;co.now=90.f;
    assert(timer_ai(0x223edd00,0,0)&&word(timer,0x74)==642);
    ActionPool decimal_pool{};OutputActionBindings dpo=output;dpo.context=&decimal_pool;dpo.next_id=&decimal_pool.id;
    auto dk=tk;dk.native_output=&dpo;
    dk.intern=[](void*,const char* text,unsigned* token) noexcept ->int{
        if(!std::strcmp(text,"timer"))*token=311;
        else if(!std::strcmp(text,"RefireTime"))*token=0x22420090;
        else if(!std::strcmp(text,"1.25D+1 trailing"))*token=304;
        else return 0;return 1;
    };
    // Empty prior OnTimer head before attaching this independent row.
    put(timer,0x3a8,0u);
    assert(api->key_value(timer,"OnTimer","timer,RefireTime,1.25D+1 trailing,0,1",&dk,&kr)&&kr.output);
    struct DecimalQueue {ActionPool* pool;ConnectedOutput* queue;void* actor;} dq{&decimal_pool,&co,timer};
    nw::ServerEntityVoidOutputBindings decimal_fire_bind{};decimal_fire_bind.context=&dq;
    decimal_fire_bind.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
        auto& f=*static_cast<DecimalQueue*>(p);if(token!=static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[0])))return 0;
        *row=f.pool->rows[0];*bytes=0x1c;return 1;
    };
    decimal_fire_bind.enqueue=[](void* p,void* a,void* c,const nw::ServerEntityVoidOutputAction* action) noexcept ->int{
        auto& f=*static_cast<DecimalQueue*>(p);assert(a==f.actor&&c==f.actor&&action->parameter==304&&action->variant_type==2);
        nw::ServerEntityNamedEventQueueResult out{};auto& q=*f.queue;
        return q.api->enqueue(q.root,sizeof(q.root),99,q.now,0x3000u,0x3000u,action,&q.queue,&out);
    };
    decimal_fire_bind.release=[](void* p,unsigned,void* row) noexcept ->int{
        auto& f=*static_cast<DecimalQueue*>(p);assert(row==f.pool->rows[0]);++f.pool->released;return 1;
    };
    tes.find_named=[](void* p,void* prev,unsigned name,void* a,void* c,void** target) noexcept ->int{
        auto& f=*static_cast<ConnectedOutput*>(p);assert(f.locked&&name==311&&a==f.actor&&c==f.actor);*target=prev?nullptr:f.actor;return 1;
    };
    nw::ServerEntityVoidOutputResult decimal_fire{};
    assert(fire_api->fire(timer+0x394,0x18,timer,timer,sizeof(timer),0.f,&decimal_fire_bind,&decimal_fire)&&decimal_fire.queued==1&&decimal_fire.removed==1&&decimal_pool.released==1);
    assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&tes,&es)&&es.native_inputs==1&&word(timer,0x3e0)==0x41480000u&&word(timer,0x74)==820&&co.released==1);
    {
        // Original branch: silent setters, two outputs, incoming activator.
        alignas(4) unsigned char actor[branch_extent]{};
        put(actor,0,branch_vtable);
        ActionPool bp{};OutputActionBindings bo=output;bo.context=&bp;bo.next_id=&bp.id;
        auto bk=kb;bk.native_output=&bo;
        bk.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=branch_extent;return 1;};
        assert(api->key_value(actor,"InitialValue","-2",&bk,&kr)&&kr.field_type==6&&actor[0x394]==1);
        assert(api->key_value(actor,"SetValue","0",&bk,&kr)&&!actor[0x394]);
        assert(api->key_value(actor,"OnTrue","relay,Trigger,,0,-1",&bk,&kr)&&kr.offset==0x398);
        assert(api->key_value(actor,"OnFalse","relay,Trigger,,0,-1",&bk,&kr)&&kr.offset==0x3b0);
        struct BranchFixture {void* actor;void* activator;ActionPool* pool;std::string order;} bf{actor,timer,&bp};
        nw::ServerEntityVoidOutputBindings fire{};fire.context=&bf;
        fire.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<BranchFixture*>(p);
            for(unsigned i=0;i<2;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[i]))){*row=f.pool->rows[i];*bytes=0x1c;return 1;}return 0;
        };
        fire.enqueue=[](void* p,void* a,void* c,const nw::ServerEntityVoidOutputAction* action) noexcept ->int{
            auto& f=*static_cast<BranchFixture*>(p);assert(a==f.activator&&c==f.actor&&action->action_id>=41&&action->action_id<=42);
            f.order+=char('0'+action->action_id-41);return 1;
        };
        BranchBindings bb{&fire};BranchResult br{};
        auto bi=[&](const char* name,unsigned type=0,unsigned char val=0){return api->branch_input(actor,sizeof(actor),name,timer,type,val,&bb,&br);};
        assert(bi("Test")&&br.fired&&br.output_offset==0x3b0&&bf.order=="1");
        assert(bi("SetValue",6,1)&&br.changed&&!br.fired&&bf.order=="1");
        assert(bi("Test")&&br.output_offset==0x398&&bf.order=="10");
        assert(bi("Toggle")&&br.changed&&!br.value&&!br.fired);
        assert(bi("ToggleTest")&&br.value&&br.fired&&bf.order=="100");
        assert(bi("SetValueTest",6,7)&&br.value==7&&actor[0x394]==7&&bf.order=="1000");
        assert(bi("SetValueTest",5,1)&&!br.value&&bf.order=="10001");
        assert(bi("Unknown")&&!br.handled);
        assert(!api->branch_input(actor,branch_extent-1,"Test",timer,0,0,&bb,&br));
    }
    {
        FactoryDictionary dict{};
        assert(api->register_factories(&dict)&&api->register_branch(&dict)&&api->register_timer(&dict)&&api->register_relay(&dict)&&api->register_auto(&dict)&&dict.count==16);
        assert(!api->register_branch(&dict));
        const auto* spec=api->find_factory(&dict,"LOGIC_BRANCH");
        assert(spec&&spec->family==Family::LogicBranch&&spec->original_object==0x224fb628&&spec->original_vtable==0x2241f5fc&&spec->extent==branch_extent);
        struct FactoryFixture {alignas(4) unsigned char storage[branch_extent]{};unsigned allocations=0,base=0,init=0,released=0,cleared=0,accepted=0;bool short_alloc=false;} ff;
        RelayFactoryBindings binding{};binding.context=&ff;
        binding.allocate=[](void* p,unsigned n,void** a,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<FactoryFixture*>(p);assert(n==branch_extent);++f.allocations;*a=f.storage;*bytes=f.short_alloc?n-1:n;return 1;
        };
        binding.base_construct=[](void* p,void* a,int arg) noexcept ->int{
            auto& f=*static_cast<FactoryFixture*>(p);assert(a==f.storage&&arg==1);++f.base;
            unsigned marker=0x12345678;std::memcpy(f.storage+0x39c,&marker,4);return 1;
        };
        binding.initialize=[](void* p,void* a,const char* cls) noexcept ->int{
            auto& f=*static_cast<FactoryFixture*>(p);assert(a==f.storage&&f.base==1&&!std::strcmp(cls,"logic_branch"));++f.init;return 1;
        };
        binding.release_networkable=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<FactoryFixture*>(p);assert(a==f.storage+0x160);++f.released;return 1;
        };
        FactoryBindings factories{};factories.dictionary=&dict;
        FactoryResult result{};
        assert(!api->create_factory("logic_branch",&factories,&result)&&result.matched&&!result.actor);
        factories.logic_branch=&binding;ff.short_alloc=true;
        assert(!api->create_factory("logic_branch",&factories,&result)&&result.matched&&!result.networkable&&!ff.base);
        ff.short_alloc=false;
        ActionPool pool{};OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
        auto keys=kb;keys.native_output=&actions;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=branch_extent;return 1;};
        Bindings parse{};parse.context=&ff;parse.native_factory=&factories;parse.native_keys=&keys;
        parse.clear_factory_global=[](void* p) noexcept ->int{++static_cast<FactoryFixture*>(p)->cleared;return 1;};
        parse.accept_entity=[](void* p,void* a,const char* text,unsigned bytes,const EntitySlice* slice) noexcept ->int{
            auto& f=*static_cast<FactoryFixture*>(p);assert(a==f.storage&&text&&slice&&slice->begin<slice->end&&slice->end<=bytes);++f.accepted;return 1;
        };
        const char map[]="{ classname logic_branch InitialValue 1 OnTrue \"relay,Trigger,,0,-1\" OnFalse \"relay,Trigger,,1,1\" }";
        Result parsed{};
        assert(api->parse(map,sizeof(map),&parse,&parsed)&&parsed.native_creates==1&&parsed.created==1&&parsed.last_factory.networkable==ff.storage+0x160);
        assert(ff.allocations==2&&ff.base==1&&ff.init==1&&ff.cleared==1&&ff.accepted==1&&ff.storage[0x394]==1);
        assert(word(ff.storage,0)==branch_vtable&&word(ff.storage,0x39c)==0x12345678&&word(ff.storage,0x3ac)==ptrword(pool.rows[0])&&word(ff.storage,0x3c4)==ptrword(pool.rows[1]));
        for(unsigned off=0x398;off<=0x3b0;off+=0x18)assert(word(ff.storage,off)==0&&word(ff.storage,off+0xc)==0xffffffffu&&word(ff.storage,off+0x10)==0);
        assert(api->destroy_branch(nullptr,0,nullptr,nullptr)&&!api->destroy_branch(ff.storage,branch_extent,ff.storage+0x164,&binding)&&!ff.released);
        assert(api->destroy_branch(ff.storage,branch_extent,ff.storage+0x160,&binding)&&ff.released==1);
    }
    {
        // Original branch datamap through AcceptInput, then actual queue
        // delivery with resolved activator and caller.
        alignas(4) unsigned char actor[branch_extent]{};
        put(actor,0,branch_vtable);put(actor,0x378,0x4001u);actor[0x1f0]=1;
        ActionPool pool{};OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
        auto keys=kb;keys.native_output=&actions;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=branch_extent;return 1;};
        assert(api->key_value(actor,"OnTrue","relay,Trigger,,0,-1",&keys,&kr));
        assert(api->key_value(actor,"OnFalse","relay,Trigger,,0,-1",&keys,&kr));
        struct BranchFixture {void* actor;void* activator;ActionPool* pool;std::string order;} f{actor,timer,&pool};
        nw::ServerEntityVoidOutputBindings fire{};fire.context=&f;
        fire.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<BranchFixture*>(p);
            for(unsigned i=0;i<2;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[i]))){*row=f.pool->rows[i];*bytes=0x1c;return 1;}return 0;
        };
        fire.enqueue=[](void* p,void* a,void* c,const nw::ServerEntityVoidOutputAction* action) noexcept ->int{
            auto& f=*static_cast<BranchFixture*>(p);assert(a==f.activator&&c==f.actor&&action->action_id>=41&&action->action_id<=42);f.order+=char('0'+action->action_id-41);return 1;
        };
        BranchBindings behavior{&fire};
        nw::ServerEntityAcceptInputBindings input{};input.storage=&tstorage;input.branch=&behavior;
        input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            switch(token){
                case 0x224202e4:*text="SetValue";break;
                case 0x2242080c:*text="SetValueTest";break;
                case 0x223d4914:*text="Toggle";break;
                case 0x224207f0:*text="ToggleTest";break;
                case 0x224207dc:*text="Test";break;
                case 301:*text="-1";break;
                case 302:*text="0";break;
                default:return 0;
            }
            *bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        nw::ServerEntityAcceptInputResult result{};
        auto accept=[&](unsigned token,unsigned type,unsigned value){
            unsigned variant[5]{value,0,0,0xffffffffu,type};actor[0x200]=0;
            return timer_accept->run(actor,sizeof(actor),branch_vtable,token,timer,timer,variant,sizeof(variant),17,&input,&result);
        };
        assert(accept(0x224202e4,2,301)&&result.row==1&&result.written==1&&result.converted&&result.dirty&&actor[0x394]==1&&actor[0x200]&&f.order.empty());
        assert(accept(0x224207dc,0,0)&&result.native_handler&&f.order=="0");
        assert(accept(0x223d4914,0,0)&&result.native_handler&&!actor[0x394]&&f.order=="0");
        assert(accept(0x2242080c,2,302)&&result.converted&&result.native_handler&&f.order=="01");
        assert(accept(0x224207f0,0,0)&&result.native_handler&&actor[0x394]&&f.order=="010");
        std::memset(co.root,0,sizeof(co.root));std::memset(co.rows,0,sizeof(co.rows));co.alloc=0;co.released=0;co.now=100.f;
        sin_independent::EntityHandleEntry entries[2]{};
        entries[0].serial=3;entries[0].entity=reinterpret_cast<CBaseEntity*>(timer);
        entries[1].serial=4;entries[1].entity=reinterpret_cast<CBaseEntity*>(actor);
        co.actor=actor;
        auto bindings=tes;bindings.entities.entries=entries;bindings.entities.count=2;bindings.native_input=&input;
        nw::ServerEntitySearchBindings search{};search.context=&co;
        search.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{*bytes=a==static_cast<ConnectedOutput*>(p)->actor?branch_extent:0;return *bytes!=0;};
        bindings.native_search=&search;
        unsigned variant[5]{0,0,0,0xffffffffu,5};
        assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x4001,0x2242080c,0x3000,0x3000,variant,sizeof(variant),0.f,17,&co.queue,&cancel_qr));
        assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&bindings,&es)&&es.native_inputs==1&&es.direct==1&&co.released==1&&!actor[0x394]&&f.order=="0101");
    }
    {
        alignas(4) unsigned char actor[case_extent]{};
        put(actor,0,case_vtable);
        ActionPool pool{};OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
        auto keys=kb;keys.native_output=&actions;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=case_extent;return 1;};
        keys.intern=[](void*,const char* text,unsigned* token) noexcept ->int{
            const char* values[]{"Alpha","alpha","16","relay","Trigger","other"};
            for(unsigned i=0;i<6;++i)if(!std::strcmp(text,values[i])){*token=201+i;return 1;}return 0;
        };
        assert(api->key_value(actor,"Case01","Alpha",&keys,&kr)&&kr.offset==0x394);
        assert(api->key_value(actor,"case02","alpha",&keys,&kr)&&kr.offset==0x398);
        assert(api->key_value(actor,"Case16","16",&keys,&kr)&&kr.offset==0x3d0);
        assert(api->key_value(actor,"OnCase02","relay,Trigger,,0,-1",&keys,&kr)&&kr.offset==0x3ec);
        assert(api->key_value(actor,"OnCase05","relay,Trigger,,0,-1",&keys,&kr)&&kr.offset==0x434);
        assert(api->key_value(actor,"OnCase16","relay,Trigger,,0,-1",&keys,&kr)&&kr.offset==0x53c);
        assert(api->key_value(actor,"OnDefault","relay,Trigger,,0,-1",&keys,&kr)&&kr.offset==0x554);
        struct CaseFixture {void* actor;void* activator;ActionPool* pool;std::string order;int draw=0;unsigned randoms=0,warnings=0;} f{actor,timer,&pool};
        nw::ServerEntityVoidOutputBindings fire{};fire.context=&f;
        fire.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<CaseFixture*>(p);
            for(unsigned i=0;i<4;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[i]))){*row=f.pool->rows[i];*bytes=0x1c;return 1;}return 0;
        };
        fire.enqueue=[](void* p,void* a,void* c,const nw::ServerEntityVoidOutputAction* action) noexcept ->int{
            auto& f=*static_cast<CaseFixture*>(p);assert(a==f.activator&&c==f.actor&&action->action_id>=41&&action->action_id<=44);f.order+=char('0'+action->action_id-41);return 1;
        };
        CaseBindings behavior{};behavior.context=&f;behavior.output=&fire;
        behavior.resolve_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            const char* values[]{"Alpha","alpha","16","relay","Trigger","other"};
            if(token<201||token>206)return 0;*text=values[token-201];*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        behavior.random_int=[](void* p,int lo,int hi,int* value) noexcept ->int{
            auto& f=*static_cast<CaseFixture*>(p);assert(lo==0&&hi==2);++f.randoms;*value=f.draw;return 1;
        };
        behavior.empty_warning=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<CaseFixture*>(p);assert(a==f.actor);++f.warnings;return 1;};
        CaseResult result{};
        unsigned variant[5]{202,0,0,0xffffffffu,2};
        assert(api->case_input(actor,sizeof(actor),"InValue",timer,variant,sizeof(variant),&behavior,&result)&&result.selected==1&&result.matched&&result.fired&&!result.queued&&f.order.empty());
        variant[0]=16;variant[4]=5;
        assert(api->case_input(actor,sizeof(actor),"InValue",timer,variant,sizeof(variant),&behavior,&result)&&result.selected==16&&result.queued==1&&f.order=="2");
        variant[0]=206;variant[4]=2;
        put(actor,0x554,0x1234u);put(actor,0x558,0x5678u);put(actor,0x564,2u);
        assert(api->case_input(actor,sizeof(actor),"InValue",timer,variant,sizeof(variant),&behavior,&result)&&result.defaulted&&result.queued==1&&f.order=="23"&&!word(actor,0x554)&&word(actor,0x558)==0x5678u&&!word(actor,0x564));
        f.draw=1;
        assert(api->case_input(actor,sizeof(actor),"PickRandom",timer,nullptr,0,&behavior,&result)&&result.eligible==3&&result.visited==3&&result.selected==5&&result.queued==1&&f.order=="231");
        f.draw=0;
        assert(api->case_input(actor,sizeof(actor),"PickRandom",timer,nullptr,0,&behavior,&result)&&result.selected==2&&f.order=="2310");
        f.draw=3;
        assert(!api->case_input(actor,sizeof(actor),"PickRandom",timer,nullptr,0,&behavior,&result)&&!result.fired);
        // Actual native input projection. FIELD_INPUT (20) preserves
        // incoming payload/type, rather than coercing it to a string.
        put(actor,0x378,0x4001u);actor[0x1f0]=1;
        nw::ServerEntityAcceptInputBindings input{};input.storage=&tstorage;input.logic_case=&behavior;
        input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            *text=token==0x2242014c?"InValue":token==0x224203b8?"PickRandom":token==401?"invalue":nullptr;
            if(!*text)return 0;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        nw::ServerEntityAcceptInputResult accepted{};
        auto accept_case=[&](unsigned input_token,unsigned type,unsigned value){
            unsigned v[5]{value,0,0,0xffffffffu,type};actor[0x200]=0;
            return timer_accept->run(actor,sizeof(actor),case_vtable,input_token,timer,timer,v,sizeof(v),0,&input,&accepted);
        };
        assert(accept_case(401,5,16)&&accepted.row==16&&accepted.expected_type==20&&accepted.actual_type==5&&accepted.converted&&accepted.native_handler&&actor[0x200]&&f.order=="23102");
        f.draw=1;
        assert(accept_case(0x224203b8,2,206)&&accepted.row==17&&accepted.actual_type==0&&accepted.native_handler&&f.order=="231021");
        std::memset(co.root,0,sizeof(co.root));std::memset(co.rows,0,sizeof(co.rows));co.alloc=0;co.released=0;co.now=110.f;co.actor=actor;
        sin_independent::EntityHandleEntry entries[2]{};
        entries[0].serial=3;entries[0].entity=reinterpret_cast<CBaseEntity*>(timer);
        entries[1].serial=4;entries[1].entity=reinterpret_cast<CBaseEntity*>(actor);
        auto bindings=tes;bindings.entities.entries=entries;bindings.entities.count=2;bindings.native_input=&input;
        nw::ServerEntitySearchBindings search{};search.context=&co;
        search.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{*bytes=a==static_cast<ConnectedOutput*>(p)->actor?case_extent:0;return *bytes!=0;};
        bindings.native_search=&search;
        unsigned incoming[5]{206,0,0,0xffffffffu,2};
        assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x4001,0x2242014c,0x3000,0x3000,incoming,sizeof(incoming),0.f,11,&co.queue,&cancel_qr));
        assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&bindings,&es)&&es.native_inputs==1&&es.direct==1&&co.released==1&&f.order=="2310213");
        for(unsigned i=0;i<16;++i)put(actor,0x3d4+0x18*i+0x14,0u);
        assert(api->case_input(actor,sizeof(actor),"PickRandom",timer,nullptr,0,&behavior,&result)&&result.warned&&f.warnings==1&&!result.fired&&f.randoms==4);
        assert(api->case_input(actor,sizeof(actor),"Unknown",timer,nullptr,0,&behavior,&result)&&!result.handled);
    }
    {
        FactoryDictionary dict{};
        assert(api->register_factories(&dict)&&api->register_case(&dict)&&api->register_branch(&dict)&&api->register_auto(&dict)&&api->register_timer(&dict)&&api->register_relay(&dict)&&dict.count==17);
        assert(!api->register_case(&dict));
        const auto* spec=api->find_factory(&dict,"LOGIC_CASE");
        assert(spec&&spec->family==Family::LogicCase&&spec->original_object==0x224fb620&&spec->original_vtable==0x2241f5e4&&spec->extent==case_extent);
        struct CaseFactoryFixture {alignas(4) unsigned char storage[case_extent]{};unsigned alloc=0,base=0,init=0,released=0,cleared=0,accepted=0;bool short_alloc=false;} f;
        RelayFactoryBindings owner{};owner.context=&f;
        owner.allocate=[](void* p,unsigned n,void** a,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<CaseFactoryFixture*>(p);assert(n==case_extent);++f.alloc;*a=f.storage;*bytes=f.short_alloc?n-1:n;return 1;
        };
        owner.base_construct=[](void* p,void* a,int arg) noexcept ->int{
            auto& f=*static_cast<CaseFactoryFixture*>(p);assert(a==f.storage&&arg==1);++f.base;
            unsigned marker=0x12345678;std::memcpy(f.storage+0x3d8,&marker,4);return 1;
        };
        owner.initialize=[](void* p,void* a,const char* cls) noexcept ->int{
            auto& f=*static_cast<CaseFactoryFixture*>(p);assert(a==f.storage&&f.base==1&&!std::strcmp(cls,"logic_case"));++f.init;return 1;
        };
        owner.release_networkable=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<CaseFactoryFixture*>(p);assert(a==f.storage+0x160);++f.released;return 1;
        };
        FactoryBindings factories{};factories.dictionary=&dict;
        FactoryResult result{};
        assert(!api->create_factory("logic_case",&factories,&result)&&result.matched&&!result.actor);
        factories.logic_case=&owner;f.short_alloc=true;
        assert(!api->create_factory("logic_case",&factories,&result)&&result.matched&&!result.networkable&&!f.base);f.short_alloc=false;
        ActionPool pool{};OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
        auto keys=kb;keys.native_output=&actions;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=case_extent;return 1;};
        keys.intern=[](void*,const char* text,unsigned* token) noexcept ->int{
            const char* values[]{"logic_case","Alpha","Omega","relay","Trigger"};
            for(unsigned i=0;i<5;++i)if(!std::strcmp(text,values[i])){*token=201+i;return 1;}return 0;
        };
        Bindings parse{};parse.context=&f;parse.native_factory=&factories;parse.native_keys=&keys;
        parse.clear_factory_global=[](void* p) noexcept ->int{++static_cast<CaseFactoryFixture*>(p)->cleared;return 1;};
        parse.accept_entity=[](void* p,void* a,const char* text,unsigned bytes,const EntitySlice* slice) noexcept ->int{
            auto& f=*static_cast<CaseFactoryFixture*>(p);assert(a==f.storage&&text&&slice&&slice->begin<slice->end&&slice->end<=bytes);++f.accepted;return 1;
        };
        const char map[]="{ classname logic_case Case01 Alpha Case16 Omega OnCase01 \"relay,Trigger,,0,-1\" OnCase16 \"relay,Trigger,,1,1\" OnDefault \"relay,Trigger,,2,-1\" }";
        Result parsed{};
        assert(api->parse(map,sizeof(map),&parse,&parsed)&&parsed.native_creates==1&&parsed.created==1&&parsed.last_factory.networkable==f.storage+0x160);
        assert(f.alloc==2&&f.base==1&&f.init==1&&f.cleared==1&&f.accepted==1&&word(f.storage,0)==case_vtable);
        assert(word(f.storage,0x394)==202&&word(f.storage,0x3d0)==203&&word(f.storage,0x3d8)==0x12345678);
        for(unsigned i=1;i<15;++i)assert(!word(f.storage,0x394+4*i));
        for(unsigned off=0x3d4;off<=0x554;off+=0x18)assert(word(f.storage,off)==0&&word(f.storage,off+0xc)==0xffffffffu&&word(f.storage,off+0x10)==0);
        assert(word(f.storage,0x3e8)==ptrword(pool.rows[0])&&word(f.storage,0x550)==ptrword(pool.rows[1])&&word(f.storage,0x568)==ptrword(pool.rows[2]));
        assert(api->destroy_case(nullptr,0,nullptr,nullptr)&&!api->destroy_case(f.storage,case_extent,f.storage+0x164,&owner)&&!f.released);
        assert(api->destroy_case(f.storage,case_extent,f.storage+0x160,&owner)&&f.released==1);
    }
    {
        alignas(4) unsigned char actor[case_extent]{};
        put(actor,0,case_vtable);put(actor,0x378,0x4001u);
        ActionPool pool{};OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
        auto keys=kb;keys.native_output=&actions;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=case_extent;return 1;};
        keys.intern=[](void*,const char* text,unsigned* token) noexcept ->int{
            const char* names[]{"sink","Use","override","unmatched"};
            for(unsigned i=0;i<4;++i)if(!std::strcmp(text,names[i])){*token=201+i;return 1;}return 0;
        };
        assert(api->key_value(actor,"OnDefault","sink,Use,,0.25,1",&keys,&kr));
        assert(api->key_value(actor,"OnDefault","sink,Use,override,0.5,-1",&keys,&kr));
        put(actor,0x554,0x11111111u);put(actor,0x558,0x22222222u);put(actor,0x55c,0x33333333u);put(actor,0x560,0x44444444u);put(actor,0x564,5u);
        std::memset(co.root,0,sizeof(co.root));std::memset(co.rows,0,sizeof(co.rows));co.alloc=0;co.released=0;co.now=120.f;
        struct StoredFixture {ActionPool* pool;ConnectedOutput* queue;void* actor;void* activator;unsigned raw=0,overrides=0,delivered=0;} f{&pool,&co,actor,timer};
        nw::ServerEntityVoidOutputBindings fire{};fire.context=&f;
        fire.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<StoredFixture*>(p);for(unsigned i=0;i<2;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[i]))){*row=f.pool->rows[i];*bytes=0x1c;return 1;}return 0;
        };
        fire.enqueue=[](void* p,void* a,void* c,const nw::ServerEntityVoidOutputAction* action) noexcept ->int{
            auto& f=*static_cast<StoredFixture*>(p);assert(a==f.activator&&c==f.actor&&action->parameter==203&&action->variant_type==2&&action->delay==.5f);++f.overrides;
            auto& q=*f.queue;nw::ServerEntityNamedEventQueueResult result{};
            return q.api->enqueue(q.root,sizeof(q.root),99,q.now,0x3000u,0x4001u,action,&q.queue,&result);
        };
        fire.enqueue_variant=[](void* p,void* a,void* c,const nw::ServerEntityVoidOutputAction* action,const void* variant,unsigned bytes) noexcept ->int{
            auto& f=*static_cast<StoredFixture*>(p);assert(a==f.activator&&c==f.actor&&!action->parameter&&!action->variant_type&&action->delay==.25f&&bytes==0x14);++f.raw;
            auto& q=*f.queue;nw::ServerEntityNamedEventQueueResult result{};
            return q.api->enqueue_variant(q.root,sizeof(q.root),99,q.now,0x3000u,0x4001u,action,variant,bytes,&q.queue,&result);
        };
        fire.release=[](void* p,unsigned,void* row) noexcept ->int{auto& f=*static_cast<StoredFixture*>(p);assert(row==f.pool->rows[0]);++f.pool->released;return 1;};
        CaseBindings behavior{};behavior.context=&f;behavior.output=&fire;
        behavior.resolve_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{if(token!=204)return 0;*text="unmatched";*bytes=10;return 1;};
        unsigned incoming[5]{204,0,0,0xffffffffu,2};CaseResult result{};
        assert(api->case_input(actor,sizeof(actor),"InValue",timer,incoming,sizeof(incoming),&behavior,&result)&&result.defaulted&&result.queued==2&&f.raw==1&&f.overrides==1&&pool.released==1);
        assert(co.alloc==2&&word(co.root,0x30)==2&&word(co.rows[1],0x30)==1);
        const unsigned expected[5]{0,0x22222222u,0x33333333u,0x44444444u,0};
        assert(!std::memcmp(co.rows[1]+0x1c,expected,sizeof(expected))&&word(co.rows[1],0x18)==0xffffffffu);
        const unsigned override_payload[5]{203,0,0,0xffffffffu,2};
        assert(!std::memcmp(co.rows[0]+0x1c,override_payload,sizeof(override_payload)));
        assert(word(actor,0x568)==ptrword(pool.rows[1])&&word(pool.rows[1],0x18)==0);
        sin_independent::EntityHandleEntry entries[2]{};
        entries[0].serial=3;entries[0].entity=reinterpret_cast<CBaseEntity*>(timer);
        entries[1].serial=4;entries[1].entity=reinterpret_cast<CBaseEntity*>(actor);
        nw::ServerEntityEventServiceBindings delivery{};delivery.context=&f;delivery.entities.entries=entries;delivery.entities.count=2;
        delivery.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{auto& q=*static_cast<StoredFixture*>(p)->queue;return q.queue.resolve(q.queue.context,token,row,bytes);};
        delivery.lock=[](void* p,bool enter) noexcept ->int{auto& q=*static_cast<StoredFixture*>(p)->queue;assert(q.locked!=enter);q.locked=enter;return 1;};
        delivery.find_named=[](void* p,void* prev,unsigned name,void* a,void* c,void** target) noexcept ->int{auto& f=*static_cast<StoredFixture*>(p);assert(f.queue->locked&&name==201&&a==f.activator&&c==f.actor);*target=prev?nullptr:f.queue->target;return 1;};
        delivery.accept=[](void* p,void* target,unsigned input,void* a,void* c,const void* variant,unsigned) noexcept ->int{
            auto& f=*static_cast<StoredFixture*>(p);assert(f.queue->locked&&target==f.queue->target&&input==202&&a==f.activator&&c==f.actor);
            const unsigned first[5]{0,0x22222222u,0x33333333u,0x44444444u,0},second[5]{203,0,0,0xffffffffu,2};
            assert(f.delivered<2&&!std::memcmp(variant,f.delivered?second:first,0x14));++f.delivered;return 1;
        };
        delivery.release=[](void* p,unsigned token,void* row) noexcept ->int{auto& q=*static_cast<StoredFixture*>(p)->queue;assert(q.locked&&token&&token<=q.alloc&&row==q.rows[token-1]);++q.released;return 1;};
        assert(service_api->service(co.root,sizeof(co.root),99,120.25f,false,&budget,&delivery,&es)&&es.events==1&&es.future_stopped&&f.delivered==1);
        assert(service_api->service(co.root,sizeof(co.root),99,120.5f,false,&budget,&delivery,&es)&&es.events==1&&f.delivered==2&&co.released==2&&!word(co.root,0x30));
    }
    {
        alignas(4) unsigned char actor[case_extent]{},named[0xc4]{};
        put(actor,0,case_vtable);put(named,0xc0,205u);
        // No actions are needed to prove exact comparison/selection.
        for(unsigned i=0;i<8;++i)put(actor,0x394+4*i,201+i);
        const auto text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            const char* values[]{"1.25","[1.25 -2 0]","1 2 3 255","-0","entity_name","<<null entity>>","","No conversion to string"};
            if(token<201||token>208)return 0;*text=values[token-201];*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        sin_independent::EntityHandleEntry entries[1]{};
        entries[0].serial=5;entries[0].entity=reinterpret_cast<CBaseEntity*>(named);
        nw::ServerEntityVoidOutputBindings fire{};
        CaseBindings behavior{};behavior.output=&fire;behavior.resolve_text=text;behavior.entries=entries;behavior.entry_count=1;behavior.entity_context=named;
        behavior.entity_extent=[](void* p,void* entity,unsigned* bytes) noexcept ->int{if(p!=entity)return 0;*bytes=0xc4;return 1;};
        CaseResult result{};
        unsigned v[5]{0x3fa00000u,0,0,0xffffffffu,1};
        auto select=[&](unsigned expected){
            return api->case_input(actor,sizeof(actor),"InValue",nullptr,v,sizeof(v),&behavior,&result)&&result.matched&&result.selected==expected&&result.fired&&!result.queued;
        };
        assert(select(1));
        v[1]=0xc0000000u;v[2]=0;v[4]=3;assert(select(2));
        v[0]=0xff030201u;v[4]=9;assert(select(3));
        v[0]=0x80000000u;v[4]=1;assert(select(4));
        v[3]=0x5000u;v[4]=13;assert(select(5));
        v[3]=0x4000u;assert(select(6));
        v[3]=0xffffffffu;assert(select(6));
        v[3]=0x5000u;put(named,0xc0,0u);assert(select(7));put(named,0xc0,205u);
        v[4]=12;assert(select(8));
        v[4]=1;v[0]=0x7f800000u;assert(!api->case_input(actor,sizeof(actor),"InValue",nullptr,v,sizeof(v),&behavior,&result));
        behavior.format_variant=[](void*,const void*,unsigned,char* out,unsigned bytes) noexcept ->int{if(bytes<5)return 0;std::memcpy(out,"1.25",5);return 1;};
        assert(select(1));behavior.format_variant=nullptr;
        // Generic input supplies its authoritative serial registry and extent.
        nw::ServerEntityAcceptInputBindings input{};input.storage=&tstorage;input.logic_case=&behavior;
        input.entities.entries=entries;input.entities.count=1;
        input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{if(token!=0x2242014c)return 0;*text="InValue";*bytes=8;return 1;};
        nw::ServerEntitySearchBindings search{};search.context=named;search.extent=behavior.entity_extent;input.search=&search;
        behavior.entries=nullptr;behavior.entry_count=0;behavior.entity_extent=nullptr;
        v[4]=13;v[3]=0x5000u;
        nw::ServerEntityAcceptInputResult accepted{};
        assert(timer_accept->run(actor,sizeof(actor),case_vtable,0x2242014c,nullptr,nullptr,v,sizeof(v),0,&input,&accepted)&&accepted.native_handler&&accepted.actual_type==13);
    }
    {
        alignas(4) unsigned char actor[compare_extent]{};
        put(actor,0,compare_vtable);
        ActionPool pool{};OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
        auto keys=kb;keys.native_output=&actions;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=compare_extent;return 1;};
        assert(api->key_value(actor,"InitialValue","1.5",&keys,&kr)&&kr.offset==0x394&&word(actor,0x394)==0x3fc00000);
        assert(api->key_value(actor,"CompareValue","2",&keys,&kr)&&kr.offset==0x398&&word(actor,0x398)==0x40000000);
        const char* names[]{"OnLessThan","OnEqualTo","OnNotEqualTo","OnGreaterThan"};
        for(unsigned i=0;i<4;++i){
            assert(api->key_value(actor,names[i],"relay,Trigger,,0,-1",&keys,&kr)&&kr.offset==0x39c+i*0x18);
            put(actor,0x39c+i*0x18+4,0x11223344u);put(actor,0x39c+i*0x18+0xc,0x55667788u);
        }
        struct CompareFixture {void* actor;void* activator;ActionPool* pool;std::string order;unsigned value=0;bool mutate=false;} f{actor,timer,&pool};
        nw::ServerEntityVoidOutputBindings fire{};fire.context=&f;
        fire.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<CompareFixture*>(p);
            for(unsigned i=0;i<4;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[i]))){*row=f.pool->rows[i];*bytes=0x1c;return 1;}return 0;
        };
        fire.enqueue_variant=[](void* p,void* a,void* c,const nw::ServerEntityVoidOutputAction* action,const void* variant,unsigned bytes) noexcept ->int{
            auto& f=*static_cast<CompareFixture*>(p);assert(a==f.activator&&c==f.actor&&bytes==0x14&&action->variant_type==1&&!action->parameter&&action->action_id>=41&&action->action_id<=44);
            unsigned payload[5]{};std::memcpy(payload,variant,0x14);
            assert(payload[0]==f.value&&payload[1]==0x11223344u&&payload[3]==0x55667788u&&payload[4]==1);
            f.order+=char('0'+action->action_id-41);
            if(f.mutate&&action->action_id==43){const unsigned threshold=0x40800000u;std::memcpy(static_cast<unsigned char*>(f.actor)+0x398,&threshold,4);}
            return 1;
        };
        CompareBindings behavior{&fire};CompareResult result{};
        auto ci=[&](const char* name,unsigned type=0,float value=0.f){return api->compare_input(actor,sizeof(actor),name,timer,type,value,&behavior,&result);};
        f.value=0x3fc00000u;
        assert(ci("Compare")&&result.compared&&result.outputs==2&&result.queued==2&&result.first_offset==0x3cc&&result.last_offset==0x39c&&f.order=="20");
        assert(ci("SetValue",1,2.f)&&result.stored&&!result.compared&&f.order=="20");
        f.value=0x40000000u;
        assert(ci("Compare")&&result.outputs==1&&result.last_offset==0x3b4&&f.order=="201");
        f.value=0x40400000u;
        assert(ci("SetValueCompare",1,3.f)&&result.stored&&result.outputs==2&&result.last_offset==0x3e4&&f.order=="20123");
        f.mutate=true;
        assert(ci("Compare")&&result.last_offset==0x39c&&result.threshold==4.f&&f.order=="2012320");f.mutate=false;
        assert(ci("SetCompareValue",5,9.f)&&result.stored&&!word(actor,0x398)&&!result.outputs);
        f.value=0x80000000u;assert(ci("SetValueCompare",1,-0.f)&&result.outputs==1&&result.last_offset==0x3b4);
        f.value=0x7fc00000u;float nan=0;std::memcpy(&nan,&f.value,4);
        assert(ci("SetValueCompare",1,nan)&&result.outputs==2&&result.first_offset==0x3cc&&result.last_offset==0x39c);
        f.value=0;assert(ci("SetValueCompare",2,123.f)&&result.outputs==1&&!word(actor,0x394));
        assert(ci("Unknown")&&!result.handled);
        assert(!api->compare_input(actor,compare_extent-1,"Compare",timer,0,0,&behavior,&result));
    }
    {
        FactoryDictionary dict{};
        assert(api->register_factories(&dict)&&api->register_compare(&dict)&&api->register_case(&dict)&&api->register_branch(&dict)&&api->register_auto(&dict)&&api->register_timer(&dict)&&api->register_relay(&dict)&&dict.count==18);
        assert(!api->register_compare(&dict));
        const auto* spec=api->find_factory(&dict,"LOGIC_COMPARE");
        assert(spec&&spec->family==Family::LogicCompare&&spec->original_object==0x224fb624&&spec->original_vtable==0x2241f5f0&&spec->extent==compare_extent);
        struct CompareFactoryFixture {alignas(4) unsigned char storage[compare_extent]{};unsigned alloc=0,base=0,init=0,released=0,cleared=0,accepted=0;bool short_alloc=false;} f;
        RelayFactoryBindings owner{};owner.context=&f;
        owner.allocate=[](void* p,unsigned n,void** a,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<CompareFactoryFixture*>(p);assert(n==compare_extent);++f.alloc;*a=f.storage;*bytes=f.short_alloc?n-1:n;return 1;
        };
        owner.base_construct=[](void* p,void* a,int arg) noexcept ->int{
            auto& f=*static_cast<CompareFactoryFixture*>(p);assert(a==f.storage&&arg==1);++f.base;
            unsigned marker=0x12345678;std::memcpy(f.storage+0x3a0,&marker,4);return 1;
        };
        owner.initialize=[](void* p,void* a,const char* cls) noexcept ->int{
            auto& f=*static_cast<CompareFactoryFixture*>(p);assert(a==f.storage&&f.base==1&&!std::strcmp(cls,"logic_compare"));++f.init;return 1;
        };
        owner.release_networkable=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<CompareFactoryFixture*>(p);assert(a==f.storage+0x160);++f.released;return 1;
        };
        FactoryBindings factories{};factories.dictionary=&dict;FactoryResult result{};
        assert(!api->create_factory("logic_compare",&factories,&result)&&result.matched&&!result.actor);
        factories.logic_compare=&owner;f.short_alloc=true;
        assert(!api->create_factory("logic_compare",&factories,&result)&&result.matched&&!result.networkable&&!f.base);f.short_alloc=false;
        ActionPool pool{};OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
        auto keys=kb;keys.native_output=&actions;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=compare_extent;return 1;};
        keys.intern=[](void*,const char* text,unsigned* token) noexcept ->int{
            const char* values[]{"logic_compare","relay","Trigger"};
            for(unsigned i=0;i<3;++i)if(!std::strcmp(text,values[i])){*token=201+i;return 1;}return 0;
        };
        Bindings parse{};parse.context=&f;parse.native_factory=&factories;parse.native_keys=&keys;
        parse.clear_factory_global=[](void* p) noexcept ->int{++static_cast<CompareFactoryFixture*>(p)->cleared;return 1;};
        parse.accept_entity=[](void* p,void* a,const char* text,unsigned bytes,const EntitySlice* slice) noexcept ->int{
            auto& f=*static_cast<CompareFactoryFixture*>(p);assert(a==f.storage&&text&&slice&&slice->begin<slice->end&&slice->end<=bytes);++f.accepted;return 1;
        };
        const char map[]="{ classname logic_compare InitialValue 1.5 CompareValue 2 OnLessThan \"relay,Trigger,,0,-1\" OnEqualTo \"relay,Trigger,,0,-1\" OnNotEqualTo \"relay,Trigger,,0,-1\" OnGreaterThan \"relay,Trigger,,0,-1\" }";
        Result parsed{};
        assert(api->parse(map,sizeof(map),&parse,&parsed)&&parsed.native_creates==1&&parsed.created==1&&parsed.last_factory.networkable==f.storage+0x160);
        assert(f.alloc==2&&f.base==1&&f.init==1&&f.cleared==1&&f.accepted==1&&word(f.storage,0)==compare_vtable);
        assert(word(f.storage,0x394)==0x3fc00000&&word(f.storage,0x398)==0x40000000&&word(f.storage,0x3a0)==0x12345678);
        for(unsigned i=0;i<4;++i){
            const unsigned off=0x39c+i*0x18;
            assert(word(f.storage,off)==0&&word(f.storage,off+0xc)==0xffffffffu&&word(f.storage,off+0x10)==0&&word(f.storage,off+0x14)==ptrword(pool.rows[i]));
        }
        assert(api->destroy_compare(nullptr,0,nullptr,nullptr)&&!api->destroy_compare(f.storage,compare_extent,f.storage+0x164,&owner)&&!f.released);
        assert(api->destroy_compare(f.storage,compare_extent,f.storage+0x160,&owner)&&f.released==1);
    }
    {
        // Real named float output chain from a queued string input.
        alignas(4) unsigned char actor[compare_extent]{};
        put(actor,0,compare_vtable);put(actor,0x378,0x4001u);actor[0x1f0]=1;
        put(actor,0x398,0x3f800000u); // threshold 1
        put(timer,0x3dc,0);put(timer,0x3e8,0);put(timer,0x3e0,0x3f800000u);
        put(timer,0x74,-1);put(timer,0xb8,0x400000u);te.serial=3;
        ActionPool pool{};OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
        auto keys=kb;keys.native_output=&actions;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=compare_extent;return 1;};
        keys.intern=[](void*,const char* text,unsigned* token) noexcept ->int{
            if(!std::strcmp(text,"timer"))*token=201;
            else if(!std::strcmp(text,"RefireTime"))*token=0x22420090;
            else if(!std::strcmp(text,"Disable"))*token=0x223edce8;
            else return 0;return 1;
        };
        assert(api->key_value(actor,"OnNotEqualTo","timer,RefireTime,,0,1",&keys,&kr));
        assert(api->key_value(actor,"OnGreaterThan","timer,Disable,,0.25,1",&keys,&kr));
        std::memset(co.root,0,sizeof(co.root));std::memset(co.rows,0,sizeof(co.rows));co.alloc=0;co.released=0;co.now=130.f;
        co.queue.string_nonempty=[](void*,unsigned,bool* yes) noexcept ->int{*yes=true;return 1;};
        struct CompareQueueFixture {ConnectedOutput* queue;ActionPool* pool;void* actor;void* timer;unsigned fired=0;} f{&co,&pool,actor,timer};
        nw::ServerEntityVoidOutputBindings fire{};fire.context=&f;
        fire.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<CompareQueueFixture*>(p);
            for(unsigned i=0;i<2;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[i]))){*row=f.pool->rows[i];*bytes=0x1c;return 1;}return 0;
        };
        fire.enqueue_variant=[](void* p,void* a,void* c,const nw::ServerEntityVoidOutputAction* action,const void* variant,unsigned bytes) noexcept ->int{
            auto& f=*static_cast<CompareQueueFixture*>(p);assert(a==f.timer&&c==f.actor&&bytes==0x14&&action->variant_type==1);
            unsigned value=0;std::memcpy(&value,variant,4);assert(value==0x40200000u);++f.fired;
            auto& q=*f.queue;nw::ServerEntityNamedEventQueueResult out{};
            return q.api->enqueue_variant(q.root,sizeof(q.root),99,q.now,0x3000u,0x4001u,action,variant,bytes,&q.queue,&out);
        };
        fire.release=[](void* p,unsigned,void*) noexcept ->int{++static_cast<CompareQueueFixture*>(p)->pool->released;return 1;};
        CompareBindings behavior{&fire};
        nw::ServerEntityAcceptInputBindings input{};input.storage=&tstorage;input.logic_compare=&behavior;input.timer=&tb;
        input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            switch(token){
                case 0x224202e4:*text="SetValue";break;
                case 0x22420704:*text="SetValueCompare";break;
                case 0x224206dc:*text="SetCompareValue";break;
                case 0x224206c4:*text="Compare";break;
                case 0x22420090:*text="RefireTime";break;
                case 0x22420054:*text="FireTimer";break;
                case 0x223edd00:*text="Enable";break;
                case 0x223edce8:*text="Disable";break;
                case 0x223d4914:*text="Toggle";break;
                case 401:*text="2.5 seconds";break;
                default:return 0;
            }
            *bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        sin_independent::EntityHandleEntry entries[2]{};
        entries[0].serial=3;entries[0].entity=reinterpret_cast<CBaseEntity*>(timer);
        entries[1].serial=4;entries[1].entity=reinterpret_cast<CBaseEntity*>(actor);
        struct CompareDelivery {CompareQueueFixture* f;} ctx{&f};
        nw::ServerEntityEventServiceBindings delivery{};delivery.context=&ctx;delivery.entities.entries=entries;delivery.entities.count=2;delivery.native_input=&input;delivery.live_now=&co.now;
        delivery.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{auto& q=*static_cast<CompareDelivery*>(p)->f->queue;return q.queue.resolve(q.queue.context,token,row,bytes);};
        delivery.lock=[](void* p,bool enter) noexcept ->int{auto& q=*static_cast<CompareDelivery*>(p)->f->queue;assert(q.locked!=enter);q.locked=enter;return 1;};
        delivery.find_named=[](void* p,void* prev,unsigned name,void* a,void* c,void** target) noexcept ->int{auto& f=*static_cast<CompareDelivery*>(p)->f;assert(f.queue->locked&&name==201&&a==f.timer&&c==f.actor);*target=prev?nullptr:f.timer;return 1;};
        delivery.release=[](void* p,unsigned token,void* row) noexcept ->int{auto& q=*static_cast<CompareDelivery*>(p)->f->queue;assert(q.locked&&token&&token<=q.alloc&&row==q.rows[token-1]);++q.released;return 1;};
        nw::ServerEntitySearchBindings search{};search.context=&f;
        search.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{auto& f=*static_cast<CompareQueueFixture*>(p);*bytes=a==f.actor?compare_extent:a==f.timer?timer_extent:0;return *bytes!=0;};
        delivery.native_search=&search;
        unsigned incoming[5]{401,0,0,0xffffffffu,2};
        assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x4001,0x22420704,0x3000,0x3000,incoming,sizeof(incoming),0.f,19,&co.queue,&cancel_qr));
        assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&delivery,&es)&&es.events==2&&es.native_inputs==2&&es.future_stopped&&f.fired==2&&pool.released==2);
        assert(word(actor,0x394)==0x40200000u&&actor[0x200]&&word(timer,0x3e0)==0x40200000u&&word(timer,0x74)==1060&&co.released==2);
        assert(word(co.rows[1],0x1c)==0x40200000u&&word(co.rows[1],0x2c)==1&&word(co.rows[2],0x1c)==0x40200000u&&word(co.rows[2],0x2c)==1);
        co.now=130.25f;
        assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&delivery,&es)&&es.events==1&&es.native_inputs==1&&word(timer,0x3dc)==1&&word(timer,0x74)==0xffffffffu&&co.released==3&&!word(co.root,0x30));
    }
    {
        alignas(4) unsigned char actor[counter_extent]{};
        put(actor,0,counter_vtable);
        ActionPool pool{};OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
        auto keys=kb;keys.native_output=&actions;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=counter_extent;return 1;};
        assert(api->key_value(actor,"startvalue","7.9",&keys,&kr)&&kr.offset==0x3a0&&word(actor,0x3a0)==0x40e00000&&word(actor,0x3b0)==1);
        assert(api->key_value(actor,"min","10",&keys,&kr)&&api->key_value(actor,"max","2",&keys,&kr));
        assert(api->key_value(actor,"OutValue","relay,Trigger,,0,-1",&keys,&kr)&&kr.offset==0x3a0);
        assert(api->key_value(actor,"OnHitMin","relay,Trigger,,0,-1",&keys,&kr)&&kr.offset==0x3b8);
        assert(api->key_value(actor,"OnHitMax","relay,Trigger,,0,-1",&keys,&kr)&&kr.offset==0x3d0);
        CounterResult result{};
        assert(api->counter_spawn(actor,sizeof(actor),&result)&&result.swapped&&result.stored&&result.minimum==2.f&&result.maximum==10.f&&result.value==7.f);
        struct CounterFixture {void* actor;void* activator;ActionPool* pool;std::string order;float value=0;unsigned warnings=0;} f{actor,timer,&pool};
        nw::ServerEntityVoidOutputBindings fire{};fire.context=&f;
        fire.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<CounterFixture*>(p);
            for(unsigned i=0;i<3;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[i]))){*row=f.pool->rows[i];*bytes=0x1c;return 1;}return 0;
        };
        fire.enqueue=[](void* p,void* a,void* c,const nw::ServerEntityVoidOutputAction* action) noexcept ->int{
            auto& f=*static_cast<CounterFixture*>(p);assert(a==f.activator&&c==f.actor&&!action->variant_type&&(action->action_id==42||action->action_id==43));
            f.order+=char('0'+action->action_id-41);return 1;
        };
        fire.enqueue_variant=[](void* p,void* a,void* c,const nw::ServerEntityVoidOutputAction* action,const void* variant,unsigned bytes) noexcept ->int{
            auto& f=*static_cast<CounterFixture*>(p);assert(a==f.activator&&c==f.actor&&bytes==0x14&&action->variant_type==1&&action->action_id==41);
            float value=0;std::memcpy(&value,variant,4);assert(value==f.value);f.order+='0';return 1;
        };
        CounterBindings behavior{};behavior.context=&f;behavior.output=&fire;
        behavior.divide_warning=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<CounterFixture*>(p);assert(a==f.actor);++f.warnings;return 1;};
        auto ci=[&](const char* name,float value,unsigned type=1){return api->counter_input(actor,sizeof(actor),name,timer,type,value,&behavior,&result);};
        f.value=10;assert(ci("Add",5)&&result.hit_max&&result.clamped&&result.outputs==2&&f.order=="20");
        assert(ci("Add",1)&&!result.hit_max&&result.outputs==1&&f.order=="200");
        f.value=5;assert(ci("Divide",2)&&result.value==5&&!actor[0x39d]&&f.order=="2000");
        f.value=2;assert(ci("Subtract",10)&&result.hit_min&&result.clamped&&f.order=="200010");
        assert(ci("Divide",0)&&result.warned&&f.warnings==1&&!result.hit_min&&f.order=="2000100");
        assert(ci("SetValueNoFire",6)&&result.value==6&&result.stored&&!result.outputs&&actor[0x39c]);
        f.value=9;assert(ci("SetHitMin",9)&&result.value==9&&result.minimum==9&&result.maximum==10&&!result.hit_min&&f.order=="20001000");
        f.value=8;assert(ci("SetHitMax",8)&&result.value==8&&result.minimum==8&&result.maximum==8&&result.hit_max&&f.order=="2000100020");
        // Both limits zero disables boundary checking without clearing latches.
        put(actor,0x394,0);put(actor,0x398,0);
        f.value=-16;assert(ci("Multiply",-2)&&result.value==-16&&!result.clamped&&result.outputs==1);
        assert(ci("SetValueNoFire",123,5)&&!word(actor,0x3a0)&&!result.outputs);
        assert(ci("Unknown",0)&&!result.handled);
        assert(!api->counter_spawn(actor,counter_extent-1,&result));
    }
    {
        FactoryDictionary dict{};
        assert(api->register_factories(&dict)&&api->register_counter(&dict)&&api->register_compare(&dict)&&api->register_case(&dict)&&api->register_branch(&dict)&&api->register_auto(&dict)&&api->register_timer(&dict)&&api->register_relay(&dict)&&dict.count==19);
        assert(!api->register_counter(&dict));
        const auto* spec=api->find_factory(&dict,"MATH_COUNTER");
        assert(spec&&spec->family==Family::MathCounter&&spec->original_object==0x224fb61c&&spec->original_vtable==0x2241f5d8&&spec->extent==counter_extent);
        struct CounterFactoryFixture {alignas(4) unsigned char storage[counter_extent]{};unsigned alloc=0,base=0,init=0,released=0,cleared=0,begin=0,end=0;bool short_alloc=false;} f;
        RelayFactoryBindings owner{};owner.context=&f;
        owner.allocate=[](void* p,unsigned n,void** a,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<CounterFactoryFixture*>(p);assert(n==counter_extent);++f.alloc;*a=f.storage;*bytes=f.short_alloc?n-1:n;return 1;
        };
        owner.base_construct=[](void* p,void* a,int arg) noexcept ->int{
            auto& f=*static_cast<CounterFactoryFixture*>(p);assert(a==f.storage&&arg==1);++f.base;
            unsigned marker=0x12345678;std::memcpy(f.storage+0x3a4,&marker,4);return 1;
        };
        owner.initialize=[](void* p,void* a,const char* cls) noexcept ->int{
            auto& f=*static_cast<CounterFactoryFixture*>(p);assert(a==f.storage&&f.base==1&&!std::strcmp(cls,"math_counter"));++f.init;return 1;
        };
        owner.release_networkable=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<CounterFactoryFixture*>(p);assert(a==f.storage+0x160);++f.released;return 1;
        };
        FactoryBindings factories{};factories.dictionary=&dict;FactoryResult result{};
        assert(!api->create_factory("math_counter",&factories,&result)&&result.matched&&!result.actor);
        factories.math_counter=&owner;f.short_alloc=true;
        assert(!api->create_factory("math_counter",&factories,&result)&&result.matched&&!result.networkable&&!f.base);f.short_alloc=false;
        ActionPool pool{};OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
        auto keys=kb;keys.native_output=&actions;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=counter_extent;return 1;};
        keys.intern=[](void*,const char* text,unsigned* token) noexcept ->int{
            const char* values[]{"math_counter","relay","Trigger"};
            for(unsigned i=0;i<3;++i)if(!std::strcmp(text,values[i])){*token=201+i;return 1;}return 0;
        };
        Bindings parse{};parse.context=&f;parse.native_factory=&factories;parse.native_keys=&keys;
        parse.clear_factory_global=[](void* p) noexcept ->int{++static_cast<CounterFactoryFixture*>(p)->cleared;return 1;};
        SpawnBindings spawn{};spawn.context=&f;spawn.native_math_counter=true;
        spawn.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=counter_extent;return 1;};
        spawn.classname=[](void*,void*,const char** cls) noexcept ->int{*cls="math_counter";return 1;};
        spawn.scope=[](void*,const char*,bool) noexcept ->int{return 1;};
        spawn.debug_enabled=[](void*,bool* enabled) noexcept ->int{*enabled=false;return 1;};
        spawn.native_spawn_begin=[](void* p,void* a,bool* proceed) noexcept ->int{auto& f=*static_cast<CounterFactoryFixture*>(p);assert(a==f.storage);++f.begin;*proceed=true;return 1;};
        spawn.native_spawn_end=[](void* p,void* a,int* code) noexcept ->int{auto& f=*static_cast<CounterFactoryFixture*>(p);assert(a==f.storage);++f.end;*code=0;return 1;};
        PipelineBindings pipe{};pipe.context=&f;pipe.parse=&parse;pipe.spawn=&spawn;
        pipe.scope=[](void*,const char*,bool) noexcept ->int{return 1;};
        pipe.classify=[](void*,void*,Category* cat) noexcept ->int{*cat=Category::Ordinary;return 1;};
        const char map[]="{ classname math_counter startvalue 12.5 min 10 max 2 OutValue \"relay,Trigger,,0,-1\" OnHitMin \"relay,Trigger,,0,-1\" OnHitMax \"relay,Trigger,,0,-1\" }";
        PipelineResult parsed{};
        assert(api->parse_all(map,sizeof(map),false,&pipe,&parsed)&&parsed.parse.native_creates==1&&parsed.spawn.native_spawns==1&&parsed.spawn.spawned==1);
        assert(f.alloc==2&&f.base==1&&f.init==1&&f.cleared==1&&f.begin==1&&f.end==1&&word(f.storage,0)==counter_vtable);
        assert(word(f.storage,0x394)==0x40000000u&&word(f.storage,0x398)==0x41200000u&&word(f.storage,0x3a0)==0x41200000u&&word(f.storage,0x3b0)==1&&word(f.storage,0x3a4)==0x12345678u&&!f.storage[0x39c]&&!f.storage[0x39d]);
        for(unsigned i=0;i<3;++i){
            const unsigned off=0x3a0+i*0x18;
            assert(word(f.storage,off+0xc)==0xffffffffu&&word(f.storage,off+0x14)==ptrword(pool.rows[i]));
            if(i)assert(!word(f.storage,off)&&!word(f.storage,off+0x10));
        }
        assert(api->destroy_counter(nullptr,0,nullptr,nullptr)&&!api->destroy_counter(f.storage,counter_extent,f.storage+0x164,&owner)&&!f.released);
        assert(api->destroy_counter(f.storage,counter_extent,f.storage+0x160,&owner)&&f.released==1);
    }
    {
        alignas(4) unsigned char counter[counter_extent]{},comparison[compare_extent]{};
        put(counter,0,counter_vtable);put(counter,0x378,0x4001u);counter[0x1f0]=1;
        put(comparison,0,compare_vtable);put(comparison,0x378,0x5002u);comparison[0x1f0]=1;
        put(comparison,0x398,0x40a00000u);
        ActionPool pool{};OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
        struct Chain {ConnectedOutput* queue;ActionPool* pool;void* counter;void* comparison;void* activator;unsigned fired=0,delivered=0;} f{&co,&pool,counter,comparison,timer};
        auto keys=kb;keys.context=&f;keys.native_output=&actions;
        keys.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{auto& f=*static_cast<Chain*>(p);*bytes=a==f.counter?counter_extent:a==f.comparison?compare_extent:0;return *bytes!=0;};
        keys.intern=[](void*,const char* text,unsigned* token) noexcept ->int{
            if(!std::strcmp(text,"comparison"))*token=201;
            else if(!std::strcmp(text,"SetValueCompare"))*token=0x22420704;
            else if(!std::strcmp(text,"sink"))*token=202;
            else if(!std::strcmp(text,"Use"))*token=203;
            else return 0;return 1;
        };
        assert(api->key_value(counter,"startvalue","3",&keys,&kr));
        assert(api->key_value(counter,"OutValue","comparison,SetValueCompare,,0,1",&keys,&kr));
        assert(api->key_value(comparison,"OnEqualTo","sink,Use,,0,1",&keys,&kr));
        std::memset(co.root,0,sizeof(co.root));std::memset(co.rows,0,sizeof(co.rows));co.alloc=0;co.released=0;co.now=140.f;
        co.queue.string_nonempty=[](void*,unsigned,bool* yes) noexcept ->int{*yes=true;return 1;};
        nw::ServerEntityVoidOutputBindings fire{};fire.context=&f;
        fire.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Chain*>(p);for(unsigned i=0;i<2;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[i]))){*row=f.pool->rows[i];*bytes=0x1c;return 1;}return 0;
        };
        fire.enqueue_variant=[](void* p,void* a,void* c,const nw::ServerEntityVoidOutputAction* action,const void* variant,unsigned bytes) noexcept ->int{
            auto& f=*static_cast<Chain*>(p);assert(a==f.activator&&(c==f.counter||c==f.comparison)&&bytes==0x14&&action->variant_type==1);
            unsigned value=0;std::memcpy(&value,variant,4);assert(value==0x40a00000u&&action->action_id==41+f.fired);++f.fired;
            auto& q=*f.queue;nw::ServerEntityNamedEventQueueResult out{};
            return q.api->enqueue_variant(q.root,sizeof(q.root),99,q.now,0x3000u,c==f.counter?0x4001u:0x5002u,action,variant,bytes,&q.queue,&out);
        };
        fire.release=[](void* p,unsigned,void*) noexcept ->int{++static_cast<Chain*>(p)->pool->released;return 1;};
        CounterBindings count_behavior{};count_behavior.output=&fire;CompareBindings compare_behavior{&fire};
        nw::ServerEntityAcceptInputBindings input{};input.storage=&tstorage;input.math_counter=&count_behavior;input.logic_compare=&compare_behavior;
        input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            switch(token){
                case 0x22420330:*text="Add";break;
                case 0x2242031c:*text="Divide";break;
                case 0x22420300:*text="Multiply";break;
                case 0x224202e4:*text="SetValue";break;
                case 0x224202c0:*text="SetValueNoFire";break;
                case 0x224202a4:*text="Subtract";break;
                case 0x22420288:*text="SetHitMax";break;
                case 0x2242026c:*text="SetHitMin";break;
                case 0x22420704:*text="SetValueCompare";break;
                case 401:*text="2.0";break;
                default:return 0;
            }
            *bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        // Exercise the late datamap row and no-fire setter before queue.
        nw::ServerEntityAcceptInputResult accepted{};
        unsigned nofire[5]{3,0,0,0xffffffffu,5};
        assert(timer_accept->run(counter,sizeof(counter),counter_vtable,0x224202c0,timer,timer,nofire,sizeof(nofire),0,&input,&accepted)&&accepted.row==8&&accepted.native_handler&&accepted.converted&&word(counter,0x3a0)==0x40400000u&&!f.fired);
        sin_independent::EntityHandleEntry entries[3]{};
        entries[0].serial=3;entries[0].entity=reinterpret_cast<CBaseEntity*>(timer);
        entries[1].serial=4;entries[1].entity=reinterpret_cast<CBaseEntity*>(counter);
        entries[2].serial=5;entries[2].entity=reinterpret_cast<CBaseEntity*>(comparison);
        nw::ServerEntitySearchBindings search{};search.context=&f;
        search.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{auto& f=*static_cast<Chain*>(p);*bytes=a==f.counter?counter_extent:a==f.comparison?compare_extent:a==f.queue->target?4u:0;return *bytes!=0;};
        nw::ServerEntityEventServiceBindings delivery{};delivery.context=&f;delivery.entities.entries=entries;delivery.entities.count=3;delivery.native_input=&input;delivery.native_search=&search;delivery.live_now=&co.now;
        delivery.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{auto& q=*static_cast<Chain*>(p)->queue;return q.queue.resolve(q.queue.context,token,row,bytes);};
        delivery.lock=[](void* p,bool enter) noexcept ->int{auto& q=*static_cast<Chain*>(p)->queue;assert(q.locked!=enter);q.locked=enter;return 1;};
        delivery.find_named=[](void* p,void* prev,unsigned name,void* a,void* c,void** target) noexcept ->int{
            auto& f=*static_cast<Chain*>(p);assert(f.queue->locked&&a==f.activator&&(name==201||name==202)&&c==(name==201?f.counter:f.comparison));
            *target=prev?nullptr:(name==201?f.comparison:f.queue->target);return 1;
        };
        delivery.accept=[](void* p,void* target,unsigned input,void* a,void* c,const void* variant,unsigned id) noexcept ->int{
            auto& f=*static_cast<Chain*>(p);assert(f.queue->locked&&target==f.queue->target&&input==203&&a==f.activator&&c==f.comparison&&id==42);
            unsigned v[5]{};std::memcpy(v,variant,0x14);assert(v[0]==0x40a00000u&&v[4]==1);++f.delivered;return 1;
        };
        delivery.release=[](void* p,unsigned token,void* row) noexcept ->int{auto& q=*static_cast<Chain*>(p)->queue;assert(q.locked&&token&&token<=q.alloc&&row==q.rows[token-1]);++q.released;return 1;};
        unsigned incoming[5]{401,0,0,0xffffffffu,2};
        assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x4001,0x22420330,0x3000,0x3000,incoming,sizeof(incoming),0.f,19,&co.queue,&cancel_qr));
        assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&delivery,&es)&&es.events==3&&es.native_inputs==2&&es.foreign_inputs==1&&f.fired==2&&f.delivered==1&&pool.released==2);
        assert(word(counter,0x3a0)==0x40a00000u&&word(comparison,0x394)==0x40a00000u&&comparison[0x200]&&co.released==3&&!word(co.root,0x30));
    }
    {
        // Seven authenticated derived destructors, all with two pool
        // records per output and reverse member / forward list order.
        struct Layout {unsigned vt,extent,first,count;};
        constexpr Layout layouts[]{
            {relay_vtable,relay_extent,0x394,1},{auto_vtable,auto_extent,0x394,7},
            {timer_vtable,timer_extent,0x394,3},{branch_vtable,branch_extent,0x398,2},
            {case_vtable,case_extent,0x3d4,17},{compare_vtable,compare_extent,0x39c,4},
            {counter_vtable,counter_extent,0x3a0,3}
        };
        struct DestroyFixture {
            alignas(4) unsigned char actor[case_extent]{},rows[34][0x1c]{};
            unsigned order[34]{},released=0,base=0,freed=0,count=0;
            unsigned alias_token=0,fail_token=0;
        } f;
        LogicDestroyBindings owner{};owner.context=&f;
        owner.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<DestroyFixture*>(p);if(!token||token>f.count*2)return 0;
            *row=token==f.alias_token?f.rows[0]:f.rows[token-1];*bytes=0x1c;return 1;
        };
        owner.release_action=[](void* p,unsigned token,void* row) noexcept ->int{
            auto& f=*static_cast<DestroyFixture*>(p);assert(!f.base&&!f.freed&&f.released<f.count*2&&row==f.rows[token-1]);
            if(token==f.fail_token)return 0;
            f.order[f.released++]=token;std::memset(row,0xcc,0x1c);return 1;
        };
        owner.base_destroy=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<DestroyFixture*>(p);assert(a==f.actor&&f.released==f.count*2&&!f.base&&!f.freed);++f.base;return 1;
        };
        owner.free_actor=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<DestroyFixture*>(p);assert(a==f.actor&&f.base==1&&!f.freed);++f.freed;return 1;
        };
        auto setup=[&](const Layout& l){
            f={};f.count=l.count;put(f.actor,0,l.vt);
            for(unsigned i=0;i<l.count;++i){
                put(f.actor,l.first+i*0x18+0x14,i*2+1);
                put(f.rows[i*2],0x18,i*2+2);
            }
        };
        LogicDestroyResult result{};
        for(const auto& l:layouts){
            setup(l);
            assert(api->destroy_logic(f.actor,l.extent,0,&owner,&result)&&result.validated&&result.completed&&result.outputs==l.count&&result.actions==l.count*2&&result.released==l.count*2&&result.base_called&&!result.freed);
            for(unsigned i=0;i<l.count;++i){
                assert(f.order[i*2]==(l.count-i)*2-1&&f.order[i*2+1]==(l.count-i)*2);
                // Original destructor does not clear the now-dead head.
                assert(word(f.actor,l.first+i*0x18+0x14)==i*2+1);
            }
            setup(l);
            assert(api->destroy_logic(f.actor,l.extent,3,&owner,&result)&&result.freed&&f.freed==1);
        }
        const auto& l=layouts[4];
        setup(l);assert(!api->destroy_logic(f.actor,l.extent-1,1,&owner,&result)&&!f.released&&!f.base);
        setup(l);put(f.rows[33],0x18,33); // cycle in last output
        assert(!api->destroy_logic(f.actor,l.extent,1,&owner,&result)&&!result.validated&&!f.released&&!f.base);
        setup(l);put(f.actor,l.first+0x18+0x14,1); // shared action token
        assert(!api->destroy_logic(f.actor,l.extent,1,&owner,&result)&&!result.validated&&!f.released);
        setup(l);f.alias_token=3; // distinct token resolves same record
        assert(!api->destroy_logic(f.actor,l.extent,1,&owner,&result)&&!result.validated&&!f.released);
        setup(l);auto missing=owner;missing.free_actor=nullptr;
        assert(!api->destroy_logic(f.actor,l.extent,1,&missing,&result)&&!result.validated&&!f.released);
        setup(l);f.fail_token=34;
        assert(!api->destroy_logic(f.actor,l.extent,1,&owner,&result)&&result.validated&&result.released==1&&!result.base_called&&!result.freed);
        setup(l);f.count=0;for(unsigned i=0;i<l.count;++i)put(f.actor,l.first+i*0x18+0x14,0);
        auto empty=owner;empty.resolve=nullptr;empty.release_action=nullptr;
        assert(api->destroy_logic(f.actor,l.extent,1,&empty,&result)&&result.outputs==17&&!result.actions&&result.base_called&&result.freed);
    }
    {
        alignas(4) unsigned char actor[menu_extent]{};
        put(actor,0,menu_vtable);put(actor,0x394,menu_listener_vtable);
        struct MenuFixture {void* actor;ActionPool* pool;const char* name;const char* button;unsigned names=0,buttons=0,queued=0,spawned=0,restored=0,subscriptions=0;bool mutate=false;} f{actor,nullptr,"gameui_selected","resume game"};
        nw::ServerEntityVoidOutputBindings fire{};fire.context=&f;
        fire.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<MenuFixture*>(p);
            for(unsigned i=0;i<2;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[i]))){*row=f.pool->rows[i];*bytes=0x1c;return 1;}return 0;
        };
        fire.enqueue=[](void* p,void* a,void* c,const nw::ServerEntityVoidOutputAction* action) noexcept ->int{
            auto& f=*static_cast<MenuFixture*>(p);assert(a==f.actor&&c==f.actor&&!action->variant_type);++f.queued;
            if(f.mutate&&f.queued==1)f.button="new game";return 1;
        };
        MenuBindings behavior{};behavior.context=&f;behavior.output=&fire;
        behavior.event_name=[](void* p,void* e,const char** text,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<MenuFixture*>(p);assert(e==&f);++f.names;*text=f.name;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        behavior.event_string=[](void* p,void* e,const char* key,const char* fallback,const char** text,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<MenuFixture*>(p);assert(e==&f&&!std::strcmp(key,"buttonname")&&!*fallback);++f.buttons;*text=f.button;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        behavior.base_spawn=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<MenuFixture*>(p);assert(a==f.actor);++f.spawned;return 1;};
        behavior.base_restore=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<MenuFixture*>(p);assert(a==f.actor);++f.restored;return 1;};
        behavior.subscribe=[](void* p,void* a,const char* name,bool server_side) noexcept ->int{
            auto& f=*static_cast<MenuFixture*>(p);assert(a==static_cast<unsigned char*>(f.actor)+0x394&&server_side&&f.spawned+f.restored==f.subscriptions/2+1);
            assert(!std::strcmp(name,f.subscriptions%2?"gameui_highlighted":"gameui_selected"));++f.subscriptions;return 1;
        };
        MenuResult result{};
        assert(api->menu_listen(actor,sizeof(actor),false,&behavior,&result)&&result.base_called&&result.subscribed==2&&f.spawned==1);
        assert(api->menu_listen(actor,sizeof(actor),true,&behavior,&result)&&result.base_called&&result.subscribed==2&&f.restored==1&&f.subscriptions==4);
        constexpr const char* button_names[]{"resume game","new game","options","load game","save game","friends","stats","quit"};
        constexpr const char* press_keys[]{"resumeGamePressed","newGamePressed","optionsPressed","loadGamePressed","saveGamePressed","friendsPressed","statsPressed","quitPressed"};
        constexpr const char* hover_keys[]{"resumeGameMousedOver","newGameMousedOver","optionsMousedOver","loadGameMousedOver","saveGameMousedOver","friendsMousedOver","statsMousedOver","quitMousedOver"};
        constexpr unsigned offsets[]{0x398,0x3b0,0x3c8,0x3e0,0x3f8,0x428,0x410,0x440};
        for(unsigned hover=0;hover<2;++hover)for(unsigned i=0;i<8;++i){
            std::memset(actor+0x398,0,sizeof(actor)-0x398);
            ActionPool pool{};f.pool=&pool;OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
            auto keys=kb;keys.native_output=&actions;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=menu_extent;return 1;};
            const unsigned off=offsets[i]+hover*0xc0;
            assert(api->key_value(actor,hover?hover_keys[i]:press_keys[i],"relay,Trigger,,0,-1",&keys,&kr)&&kr.offset==off&&kr.field_type==11);
            f.name=hover?"gameui_highlighted":"gameui_selected";f.button=button_names[i];f.names=f.buttons=f.queued=0;
            assert(api->menu_event(actor,sizeof(actor),&f,&behavior,&result)&&result.completed&&result.outputs==1&&result.queued==1&&result.first_offset==off&&result.last_offset==off);
            assert(result.name_reads==1+hover&&result.button_reads==(i?i+1:8)&&f.names==result.name_reads&&f.buttons==result.button_reads&&f.queued==1);
        }
        // Resume falls through; a changed event string can match next button.
        std::memset(actor+0x398,0,sizeof(actor)-0x398);
        ActionPool pool{};f.pool=&pool;OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
        auto keys=kb;keys.native_output=&actions;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=menu_extent;return 1;};
        assert(api->key_value(actor,"resumeGamePressed","relay,Trigger,,0,-1",&keys,&kr)&&api->key_value(actor,"newGamePressed","relay,Trigger,,0,-1",&keys,&kr));
        f.name="gameui_selected";f.button="resume game";f.queued=0;f.mutate=true;
        assert(api->menu_event(actor,sizeof(actor),&f,&behavior,&result)&&result.outputs==2&&result.queued==2&&result.first_offset==0x398&&result.last_offset==0x3b0&&result.button_reads==2);
        f.mutate=false;f.name="GameUI_selected";assert(api->menu_event(actor,sizeof(actor),&f,&behavior,&result)&&!result.outputs&&result.name_reads==2&&!result.button_reads);
        f.name="gameui_selected";f.button="Resume Game";assert(api->menu_event(actor,sizeof(actor),&f,&behavior,&result)&&!result.outputs&&result.button_reads==8);
        put(actor,0x394,0);assert(!api->menu_event(actor,sizeof(actor),&f,&behavior,&result));
    }
    {
        FactoryDictionary dict{};
        assert(api->register_factories(&dict)&&api->register_menu(&dict)&&api->register_counter(&dict)&&api->register_compare(&dict)&&api->register_case(&dict)&&api->register_branch(&dict)&&api->register_auto(&dict)&&api->register_timer(&dict)&&api->register_relay(&dict)&&dict.count==20);
        assert(!api->register_menu(&dict));
        const auto* spec=api->find_factory(&dict,"LOGIC_MENULISTENER");
        assert(spec&&spec->family==Family::LogicMenuListener&&spec->original_object==0x224fb634&&spec->original_vtable==0x2241f620&&spec->extent==menu_extent);
        struct MenuOwner {
            alignas(4) unsigned char storage[menu_extent]{};
            ActionPool* pool=nullptr;const sin_port::native_map::Api* api=nullptr;
            LogicDestroyBindings teardown{};LogicDestroyResult destroyed{};
            unsigned alloc=0,base=0,init=0,cleared=0,begin=0,end=0,spawned=0,restored=0,subscribed=0,unregistered=0,released=0,base_dead=0,freed=0;
        } f;f.api=api;
        RelayFactoryBindings owner{};owner.context=&f;
        owner.allocate=[](void* p,unsigned n,void** a,unsigned* bytes) noexcept ->int{auto& f=*static_cast<MenuOwner*>(p);assert(n==menu_extent);++f.alloc;*a=f.storage;*bytes=n;return 1;};
        owner.base_construct=[](void* p,void* a,int arg) noexcept ->int{auto& f=*static_cast<MenuOwner*>(p);assert(a==f.storage&&arg==1);++f.base;return 1;};
        owner.initialize=[](void* p,void* a,const char* cls) noexcept ->int{auto& f=*static_cast<MenuOwner*>(p);assert(a==f.storage&&f.base==1&&!std::strcmp(cls,"logic_menulistener"));++f.init;return 1;};
        owner.release_networkable=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<MenuOwner*>(p);assert(a==f.storage+0x160);
            return f.api->destroy_logic(f.storage,sizeof(f.storage),1,&f.teardown,&f.destroyed);
        };
        FactoryBindings factories{};factories.dictionary=&dict;factories.logic_menu=&owner;
        ActionPool pool{};f.pool=&pool;OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
        auto keys=kb;keys.native_output=&actions;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=menu_extent;return 1;};
        keys.intern=[](void*,const char* text,unsigned* token) noexcept ->int{
            const char* values[]{"logic_menulistener","relay","Trigger"};
            for(unsigned i=0;i<3;++i)if(!std::strcmp(text,values[i])){*token=201+i;return 1;}return 0;
        };
        Bindings parse{};parse.context=&f;parse.native_factory=&factories;parse.native_keys=&keys;
        parse.clear_factory_global=[](void* p) noexcept ->int{++static_cast<MenuOwner*>(p)->cleared;return 1;};
        MenuBindings behavior{};behavior.context=&f;
        behavior.base_spawn=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<MenuOwner*>(p);assert(a==f.storage);++f.spawned;return 1;};
        behavior.base_restore=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<MenuOwner*>(p);assert(a==f.storage);++f.restored;return 1;};
        behavior.subscribe=[](void* p,void* a,const char* name,bool server_side) noexcept ->int{
            auto& f=*static_cast<MenuOwner*>(p);assert(a==f.storage+0x394&&server_side&&f.spawned+f.restored==f.subscribed/2+1&&!std::strcmp(name,f.subscribed%2?"gameui_highlighted":"gameui_selected"));++f.subscribed;return 1;
        };
        SpawnBindings spawn{};spawn.context=&f;spawn.logic_menu=&behavior;
        spawn.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=menu_extent;return 1;};
        spawn.classname=[](void*,void*,const char** cls) noexcept ->int{*cls="logic_menulistener";return 1;};
        spawn.scope=[](void*,const char*,bool) noexcept ->int{return 1;};
        spawn.debug_enabled=[](void*,bool* enabled) noexcept ->int{*enabled=false;return 1;};
        spawn.native_spawn_begin=[](void* p,void* a,bool* proceed) noexcept ->int{auto& f=*static_cast<MenuOwner*>(p);assert(a==f.storage);++f.begin;*proceed=true;return 1;};
        spawn.native_spawn_end=[](void* p,void* a,int* code) noexcept ->int{auto& f=*static_cast<MenuOwner*>(p);assert(a==f.storage);++f.end;*code=0;return 1;};
        PipelineBindings pipe{};pipe.context=&f;pipe.parse=&parse;pipe.spawn=&spawn;
        pipe.scope=[](void*,const char*,bool) noexcept ->int{return 1;};
        pipe.classify=[](void*,void*,Category* cat) noexcept ->int{*cat=Category::Ordinary;return 1;};
        const char map[]="{ classname logic_menulistener newGamePressed \"relay,Trigger,,0,-1\" quitMousedOver \"relay,Trigger,,0,-1\" }";
        PipelineResult parsed{};
        assert(api->parse_all(map,sizeof(map),false,&pipe,&parsed)&&parsed.parse.native_creates==1&&parsed.spawn.native_spawns==1&&parsed.spawn.spawned==1);
        assert(f.alloc==1&&f.base==1&&f.init==1&&f.cleared==1&&f.begin==1&&f.end==1&&f.spawned==1&&f.subscribed==2);
        assert(word(f.storage,0)==menu_vtable&&word(f.storage,0x394)==menu_listener_vtable&&word(f.storage,0x3c4)==ptrword(pool.rows[0])&&word(f.storage,0x514)==ptrword(pool.rows[1]));
        for(unsigned off=0x398;off<=0x500;off+=0x18)assert(!word(f.storage,off)&&word(f.storage,off+0xc)==0xffffffffu&&!word(f.storage,off+0x10));
        MenuResult listened{};assert(api->menu_listen(f.storage,sizeof(f.storage),true,&behavior,&listened)&&f.restored==1&&f.subscribed==4);
        auto& d=f.teardown;d.context=&f;
        d.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<MenuOwner*>(p);for(unsigned i=0;i<2;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[i]))){*row=f.pool->rows[i];*bytes=0x1c;return 1;}return 0;
        };
        d.unregister_menu=[](void* p,void* listener) noexcept ->int{
            auto& f=*static_cast<MenuOwner*>(p);assert(listener==f.storage+0x394&&f.subscribed==4&&!f.released&&!f.unregistered);++f.unregistered;return 1;
        };
        d.release_action=[](void* p,unsigned,void* row) noexcept ->int{
            auto& f=*static_cast<MenuOwner*>(p);assert(f.unregistered==1&&f.released<2&&row==f.pool->rows[1-f.released]&&!f.base_dead);++f.released;return 1;
        };
        d.base_destroy=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<MenuOwner*>(p);unsigned vt=0;std::memcpy(&vt,f.storage+0x394,4);assert(a==f.storage&&vt==0x224227c4u&&f.released==2);++f.base_dead;return 1;
        };
        d.free_actor=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<MenuOwner*>(p);assert(a==f.storage&&f.base_dead==1);++f.freed;return 1;};
        auto invalid=d;invalid.unregister_menu=nullptr;
        assert(!api->destroy_logic(f.storage,sizeof(f.storage),1,&invalid,&f.destroyed)&&!f.released&&!f.unregistered);
        assert(api->destroy_menu(f.storage,sizeof(f.storage),f.storage+0x160,&owner)&&f.destroyed.completed&&f.destroyed.unregistered&&f.destroyed.outputs==16&&f.destroyed.released==2&&f.destroyed.freed&&f.freed==1);
    }
    {
        alignas(4) unsigned char actor[navigation_extent]{},target[3][0x238]{};
        put(actor,0,navigation_vtable);put(actor,0x394,navigation_listener_vtable);
        struct NavigationFixture {void* actor;unsigned char (*targets)[0x238];unsigned warnings=0,registered=0,unregistered=0,finds=0,matches=0,activated=0,removed=0,edict=0;bool cycle=false;} f{actor,target};
        auto keys=kb;keys.context=&f;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=navigation_extent;return 1;};
        keys.intern=[](void*,const char* text,unsigned* token) noexcept ->int{if(std::strcmp(text,"victim"))return 0;*token=201;return 1;};
        keys.navigation_warning=[](void* p,void* a,const char* value) noexcept ->int{auto& f=*static_cast<NavigationFixture*>(p);assert(a==f.actor&&!std::strcmp(value,"unknown"));++f.warnings;return 1;};
        assert(api->key_value(actor,"navprop","IgNoRe",&keys,&kr)&&kr.handled&&word(actor,0x39c)==1);
        assert(api->key_value(actor,"navprop","unknown",&keys,&kr)&&kr.handled&&f.warnings==1&&word(actor,0x39c)==1);
        assert(api->key_value(actor,"target","victim",&keys,&kr)&&kr.offset==0x90&&word(actor,0x90)==201);
        put(target[0],0xc0,201);put(target[1],0xc0,202);put(target[2],0xc0,203);
        put(target[1],0x234,0x7f800000u); // already greater than FLT_MAX
        NavigationBindings b{};b.context=&f;
        b.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{auto& f=*static_cast<NavigationFixture*>(p);*bytes=(a==f.targets[0]||a==f.targets[1]||a==f.targets[2])?0x238:0;return *bytes!=0;};
        b.find=[](void* p,void* prev,unsigned token,void** found) noexcept ->int{auto& f=*static_cast<NavigationFixture*>(p);assert(token==201);++f.finds;*found=!prev?f.targets[0]:f.cycle?f.targets[0]:prev==f.targets[0]?f.targets[1]:nullptr;return 1;};
        b.listen=[](void* p,void* listener,bool on) noexcept ->int{auto& f=*static_cast<NavigationFixture*>(p);assert(listener==static_cast<unsigned char*>(f.actor)+0x394);if(on)++f.registered;else ++f.unregistered;return 1;};
        b.match=[](void* p,void* a,unsigned token,bool* matched) noexcept ->int{auto& f=*static_cast<NavigationFixture*>(p);assert(token==201);++f.matches;*matched=a==f.targets[1];return 1;};
        b.base_activate=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<NavigationFixture*>(p);assert(a==f.actor);++f.activated;return 1;};
        b.base_remove=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<NavigationFixture*>(p);assert(a==f.actor);++f.removed;return 1;};
        b.mark_edict=[](void* p,unsigned token) noexcept ->int{auto& f=*static_cast<NavigationFixture*>(p);assert(token==77);++f.edict;return 1;};
        NavigationResult result{};
        put(actor,0xb4,5);actor[0x1f0]=1;
        assert(api->navigation_activate(actor,sizeof(actor),&b,&result)&&result.base_called&&result.registered==1&&result.visited==2&&result.written==1&&result.dirty&&actor[0x200]&&word(actor,0xb4)==4);
        assert(word(target[0],0x234)==0x7f7fffffu&&word(target[1],0x234)==0x7f800000u);
        assert(api->navigation_input(actor,sizeof(actor),"TurnOn",&b,&result)&&!result.changed&&!result.visited&&f.registered==1);
        put(target[0],0x234,0);assert(api->navigation_created(actor,sizeof(actor),target[0],&b,&result)&&result.matched&&result.written==1&&!f.matches);
        put(target[1],0x234,0);assert(api->navigation_created(actor,sizeof(actor),target[1],&b,&result)&&result.matched&&result.written==1&&f.matches==1);
        assert(api->navigation_created(actor,sizeof(actor),target[2],&b,&result)&&!result.matched&&!result.written&&f.matches==2);
        assert(api->navigation_input(actor,sizeof(actor),"Toggle",&b,&result)&&!result.on&&result.unregistered==1&&result.written==2&&!word(target[0],0x234)&&!word(target[1],0x234));
        assert(api->navigation_created(actor,sizeof(actor),target[0],&b,&result)&&!result.matched);
        actor[0x1f0]=0;put(actor,0x16c,77);put(actor,0xb4,1);
        assert(api->navigation_activate(actor,sizeof(actor),&b,&result)&&result.dirty&&f.edict==1&&result.registered==1);
        unsigned searches=f.finds;
        assert(api->navigation_activate(actor,sizeof(actor),&b,&result)&&result.registered==1&&!result.visited&&f.finds==searches);
        assert(api->navigation_remove(actor,sizeof(actor),&b,&result)&&result.unregistered==1&&result.base_called&&actor[0x398]&&word(target[0],0x234)==0x7f7fffffu);
        assert(api->navigation_input(actor,sizeof(actor),"TurnOff",&b,&result)&&!result.on&&result.written==2);
        put(actor,0x39c,0);assert(api->navigation_input(actor,sizeof(actor),"TurnOn",&b,&result)&&result.visited==2&&!result.written);
        assert(api->navigation_input(actor,sizeof(actor),"TurnOff",&b,&result));
        f.cycle=true;assert(!api->navigation_input(actor,sizeof(actor),"TurnOn",&b,&result)&&result.visited==1&&!result.completed);
        assert(!api->navigation_activate(actor,navigation_extent-1,&b,&result));
    }
    {
        FactoryDictionary dict{};
        assert(api->register_factories(&dict)&&api->register_navigation(&dict)&&api->register_menu(&dict)&&api->register_counter(&dict)&&api->register_compare(&dict)&&api->register_case(&dict)&&api->register_branch(&dict)&&api->register_auto(&dict)&&api->register_timer(&dict)&&api->register_relay(&dict)&&dict.count==21);
        assert(!api->register_navigation(&dict));
        const auto* spec=api->find_factory(&dict,"LOGIC_NAVIGATION");
        assert(spec&&spec->family==Family::LogicNavigation&&spec->original_object==0x224fb0e0&&spec->original_vtable==0x2241ece4&&spec->extent==navigation_extent);
        struct NavigationOwner {
            alignas(4) unsigned char storage[navigation_extent]{},target[0x238]{};
            const sin_port::native_map::Api* api=nullptr;
            LogicDestroyBindings teardown{};LogicDestroyResult destroyed{};
            unsigned alloc=0,base=0,init=0,cleared=0,spawned=0,activated=0,registered=0,unregistered=0,removed=0,base_dead=0,freed=0;
        } f;f.api=api;
        RelayFactoryBindings owner{};owner.context=&f;
        owner.allocate=[](void* p,unsigned n,void** a,unsigned* bytes) noexcept ->int{auto& f=*static_cast<NavigationOwner*>(p);assert(n==navigation_extent);++f.alloc;*a=f.storage;*bytes=n;return 1;};
        owner.base_construct=[](void* p,void* a,int arg) noexcept ->int{auto& f=*static_cast<NavigationOwner*>(p);assert(a==f.storage&&arg==1);++f.base;return 1;};
        owner.initialize=[](void* p,void* a,const char* cls) noexcept ->int{auto& f=*static_cast<NavigationOwner*>(p);assert(a==f.storage&&f.base==1&&!std::strcmp(cls,"logic_navigation"));++f.init;return 1;};
        owner.release_networkable=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<NavigationOwner*>(p);assert(a==f.storage+0x160&&f.removed==1);
            return f.api->destroy_logic(f.storage,sizeof(f.storage),1,&f.teardown,&f.destroyed);
        };
        FactoryBindings factories{};factories.dictionary=&dict;factories.logic_navigation=&owner;
        auto keys=kb;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=navigation_extent;return 1;};
        keys.intern=[](void*,const char* text,unsigned* token) noexcept ->int{
            if(!std::strcmp(text,"logic_navigation"))*token=201;
            else if(!std::strcmp(text,"victim"))*token=202;
            else return 0;return 1;
        };
        Bindings parse{};parse.context=&f;parse.native_factory=&factories;parse.native_keys=&keys;
        parse.clear_factory_global=[](void* p) noexcept ->int{++static_cast<NavigationOwner*>(p)->cleared;return 1;};
        NavigationBindings behavior{};behavior.context=&f;
        behavior.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{auto& f=*static_cast<NavigationOwner*>(p);*bytes=a==f.target?0x238:0;return *bytes!=0;};
        behavior.find=[](void* p,void* prev,unsigned name,void** found) noexcept ->int{auto& f=*static_cast<NavigationOwner*>(p);assert(name==202&&(!prev||prev==f.target));*found=prev?nullptr:f.target;return 1;};
        behavior.listen=[](void* p,void* listener,bool on) noexcept ->int{auto& f=*static_cast<NavigationOwner*>(p);assert(listener==f.storage+0x394);if(on)++f.registered;else ++f.unregistered;return 1;};
        behavior.base_activate=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<NavigationOwner*>(p);assert(a==f.storage&&f.spawned==1);++f.activated;return 1;};
        behavior.base_remove=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<NavigationOwner*>(p);assert(a==f.storage&&f.unregistered==1);++f.removed;return 1;};
        SpawnBindings spawn{};spawn.context=&f;spawn.logic_navigation=&behavior;
        spawn.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=navigation_extent;return 1;};
        spawn.classname=[](void*,void*,const char** cls) noexcept ->int{*cls="logic_navigation";return 1;};
        spawn.scope=[](void*,const char*,bool) noexcept ->int{return 1;};
        spawn.engine_scope=[](void*,bool) noexcept ->int{return 1;};
        spawn.debug_enabled=[](void*,bool* enabled) noexcept ->int{*enabled=false;return 1;};
        spawn.spawn=[](void* p,void* a,int* code) noexcept ->int{auto& f=*static_cast<NavigationOwner*>(p);assert(a==f.storage);++f.spawned;*code=0;return 1;};
        PipelineBindings pipe{};pipe.context=&f;pipe.parse=&parse;pipe.spawn=&spawn;
        pipe.scope=[](void*,const char*,bool) noexcept ->int{return 1;};
        pipe.classify=[](void*,void*,Category* cat) noexcept ->int{*cat=Category::Ordinary;return 1;};
        const char map[]="{ classname logic_navigation target victim navprop Ignore spawnflags 1 }";
        PipelineResult parsed{};
        assert(api->parse_all(map,sizeof(map),true,&pipe,&parsed)&&parsed.parse.native_creates==1&&parsed.spawn.native_activations==1&&parsed.spawn.spawned==1&&parsed.spawn.activated==1);
        assert(f.alloc==1&&f.base==1&&f.init==1&&f.cleared==1&&f.activated==1&&f.registered==1);
        assert(word(f.storage,0)==navigation_vtable&&word(f.storage,0x394)==navigation_listener_vtable&&word(f.storage,0x39c)==1&&word(f.storage,0x90)==202&&!word(f.storage,0xb4)&&f.storage[0x398]&&word(f.target,0x234)==0x7f7fffffu);
        NavigationResult removed{};
        assert(api->navigation_remove(f.storage,sizeof(f.storage),&behavior,&removed)&&removed.unregistered==1&&removed.base_called&&word(f.target,0x234)==0x7f7fffffu);
        auto& d=f.teardown;d.context=&f;
        d.base_destroy=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<NavigationOwner*>(p);assert(a==f.storage&&f.removed==1);++f.base_dead;return 1;};
        d.free_actor=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<NavigationOwner*>(p);assert(a==f.storage&&f.base_dead==1);++f.freed;return 1;};
        assert(!api->destroy_navigation(f.storage,navigation_extent,f.storage+0x164,&owner));
        assert(api->destroy_navigation(f.storage,navigation_extent,f.storage+0x160,&owner)&&f.destroyed.completed&&!f.destroyed.outputs&&f.destroyed.base_called&&f.destroyed.freed&&f.freed==1);
    }
    {
        alignas(4) unsigned char actor[navigation_extent]{},target[0x238]{};
        put(actor,0,navigation_vtable);put(actor,0x394,navigation_listener_vtable);
        put(actor,0x378,0x4001u);actor[0x1f0]=1;put(actor,0x90,201);put(actor,0x39c,1);put(target,0xc0,201);
        struct NavigationQueue {ConnectedOutput* queue;void* actor;void* target;unsigned on=0,off=0,searches=0;} f{&co,actor,target};
        NavigationBindings behavior{};behavior.context=&f;
        behavior.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{auto& f=*static_cast<NavigationQueue*>(p);*bytes=a==f.target?0x238:0;return *bytes!=0;};
        behavior.find=[](void* p,void* prev,unsigned token,void** found) noexcept ->int{
            auto& f=*static_cast<NavigationQueue*>(p);assert(token==201&&(!prev||prev==f.target));++f.searches;*found=prev?nullptr:f.target;return 1;
        };
        behavior.listen=[](void* p,void* listener,bool on) noexcept ->int{
            auto& f=*static_cast<NavigationQueue*>(p);assert(listener==static_cast<unsigned char*>(f.actor)+0x394);
            if(on)++f.on;else ++f.off;return 1;
        };
        nw::ServerEntityAcceptInputBindings input{};input.storage=&tstorage;input.logic_navigation=&behavior;
        input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            *text=token==0x223d4940?"TurnOn":token==0x223d4928?"TurnOff":token==0x223d4914?"Toggle":token==401?"turnon":nullptr;
            if(!*text)return 0;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        unsigned v[5]{5,0,0,0xffffffffu,5};nw::ServerEntityAcceptInputResult accepted{};
        assert(timer_accept->run(actor,sizeof(actor),navigation_vtable,401,timer,timer,v,sizeof(v),0,&input,&accepted)&&accepted.row==2&&accepted.converted&&accepted.actual_type==0&&accepted.native_handler&&actor[0x200]&&f.on==1&&f.searches==2&&word(target,0x234)==0x7f7fffffu);
        assert(timer_accept->run(actor,sizeof(actor),navigation_vtable,0x223d4940,timer,timer,v,sizeof(v),0,&input,&accepted)&&f.on==1&&f.searches==2);
        std::memset(co.root,0,sizeof(co.root));std::memset(co.rows,0,sizeof(co.rows));co.alloc=0;co.released=0;co.now=150.f;
        co.queue.string_nonempty=[](void*,unsigned,bool* yes) noexcept ->int{*yes=true;return 1;};
        sin_independent::EntityHandleEntry entries[2]{};
        entries[0].serial=3;entries[0].entity=reinterpret_cast<CBaseEntity*>(timer);
        entries[1].serial=4;entries[1].entity=reinterpret_cast<CBaseEntity*>(actor);
        nw::ServerEntitySearchBindings search{};search.context=&f;
        search.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{*bytes=a==static_cast<NavigationQueue*>(p)->actor?navigation_extent:0;return *bytes!=0;};
        nw::ServerEntityEventServiceBindings delivery{};delivery.context=&f;delivery.entities.entries=entries;delivery.entities.count=2;delivery.native_input=&input;delivery.native_search=&search;delivery.live_now=&co.now;
        delivery.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{auto& q=*static_cast<NavigationQueue*>(p)->queue;return q.queue.resolve(q.queue.context,token,row,bytes);};
        delivery.lock=[](void* p,bool enter) noexcept ->int{auto& q=*static_cast<NavigationQueue*>(p)->queue;assert(q.locked!=enter);q.locked=enter;return 1;};
        delivery.release=[](void* p,unsigned token,void* row) noexcept ->int{auto& q=*static_cast<NavigationQueue*>(p)->queue;assert(q.locked&&token&&token<=q.alloc&&row==q.rows[token-1]);++q.released;return 1;};
        assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x4001,0x223d4914,0x3000,0x3000,v,sizeof(v),0.f,19,&co.queue,&cancel_qr));
        assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x4001,0x223d4940,0x3000,0x3000,v,sizeof(v),.25f,20,&co.queue,&cancel_qr));
        assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x4001,0x223d4928,0x3000,0x3000,v,sizeof(v),.5f,21,&co.queue,&cancel_qr));
        assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&delivery,&es)&&es.events==1&&es.native_inputs==1&&es.future_stopped&&f.off==1&&!actor[0x398]&&!word(target,0x234));
        co.now=150.25f;
        assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&delivery,&es)&&es.events==1&&es.native_inputs==1&&f.on==2&&actor[0x398]&&word(target,0x234)==0x7f7fffffu);
        co.now=150.5f;
        assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&delivery,&es)&&es.events==1&&es.native_inputs==1&&f.off==2&&f.searches==8&&!word(target,0x234)&&co.released==3&&!word(co.root,0x30));
        put(actor,0x394,0);
        assert(!timer_accept->run(actor,sizeof(actor),navigation_vtable,0x223d4940,timer,timer,v,sizeof(v),0,&input,&accepted)&&!accepted.completed);
    }
    {
        FactoryDictionary dict{};
        assert(api->register_factories(&dict)&&api->register_autosave(&dict)&&api->register_navigation(&dict)&&api->register_menu(&dict)&&api->register_counter(&dict)&&api->register_compare(&dict)&&api->register_case(&dict)&&api->register_branch(&dict)&&api->register_auto(&dict)&&api->register_timer(&dict)&&api->register_relay(&dict)&&dict.count==22);
        assert(!api->register_autosave(&dict));
        const auto* spec=api->find_factory(&dict,"LOGIC_AUTOSAVE");
        assert(spec&&spec->family==Family::LogicAutosave&&spec->original_object==0x224fb62c&&spec->original_vtable==0x2241f608&&spec->extent==autosave_extent);
        struct AutosaveOwner {
            alignas(4) unsigned char storage[autosave_extent]{};
            unsigned char gate=0;
            unsigned alloc=0,base=0,init=0,cleared=0,accepted=0,released=0;
            bool mutate=false;std::string calls;
        } f;
        RelayFactoryBindings owner{};owner.context=&f;
        owner.allocate=[](void* p,unsigned n,void** a,unsigned* bytes) noexcept ->int{auto& f=*static_cast<AutosaveOwner*>(p);assert(n==autosave_extent);++f.alloc;*a=f.storage;*bytes=n;return 1;};
        owner.base_construct=[](void* p,void* a,int arg) noexcept ->int{auto& f=*static_cast<AutosaveOwner*>(p);assert(a==f.storage&&arg==1);++f.base;return 1;};
        owner.initialize=[](void* p,void* a,const char* cls) noexcept ->int{auto& f=*static_cast<AutosaveOwner*>(p);assert(a==f.storage&&f.base==1&&!std::strcmp(cls,"logic_autosave"));++f.init;return 1;};
        owner.release_networkable=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<AutosaveOwner*>(p);assert(a==f.storage+0x160);++f.released;return 1;};
        FactoryBindings factories{};factories.dictionary=&dict;factories.logic_autosave=&owner;
        auto keys=kb;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=autosave_extent;return 1;};
        keys.intern=[](void*,const char* text,unsigned* token) noexcept ->int{if(std::strcmp(text,"logic_autosave"))return 0;*token=201;return 1;};
        Bindings parse{};parse.context=&f;parse.native_factory=&factories;parse.native_keys=&keys;
        parse.clear_factory_global=[](void* p) noexcept ->int{++static_cast<AutosaveOwner*>(p)->cleared;return 1;};
        parse.accept_entity=[](void* p,void* a,const char*,unsigned,const EntitySlice*) noexcept ->int{auto& f=*static_cast<AutosaveOwner*>(p);assert(a==f.storage);++f.accepted;return 1;};
        const char map[]="{ classname logic_autosave NewLevelUnit 1 }";
        Result parsed{};
        assert(api->parse(map,sizeof(map),&parse,&parsed)&&parsed.native_creates==1&&parsed.last_factory.networkable==f.storage+0x160);
        assert(f.alloc==1&&f.base==1&&f.init==1&&f.cleared==1&&f.accepted==1&&word(f.storage,0)==autosave_vtable&&f.storage[0x394]);
        AutosaveBindings behavior{};behavior.context=&f;
        behavior.new_level_unit=[](void* p) noexcept ->int{auto& f=*static_cast<AutosaveOwner*>(p);f.calls+='N';if(f.mutate)f.gate=7;return 1;};
        behavior.difficulty_gate=[](void* p,unsigned char* gate) noexcept ->int{auto& f=*static_cast<AutosaveOwner*>(p);f.calls+='G';*gate=f.gate;return 1;};
        behavior.server_command=[](void* p,const char* cmd) noexcept ->int{auto& f=*static_cast<AutosaveOwner*>(p);assert(!std::strcmp(cmd,"autosave\n"));f.calls+='S';return 1;};
        AutosaveResult result{};
        assert(api->autosave_input(f.storage,sizeof(f.storage),"Save",&behavior,&result)&&result.new_level_unit&&result.gate_read&&result.command_queued&&f.calls=="NGS");
        f.mutate=true;
        assert(api->autosave_input(f.storage,sizeof(f.storage),"Save",&behavior,&result)&&result.suppressed&&result.gate==7&&!result.command_queued&&f.calls=="NGSNG");
        assert(api->key_value(f.storage,"NewLevelUnit","0",&keys,&kr)&&!f.storage[0x394]);
        assert(api->autosave_input(f.storage,sizeof(f.storage),"Save",&behavior,&result)&&!result.new_level_unit&&result.suppressed&&f.calls=="NGSNGG");
        f.gate=0;assert(api->autosave_input(f.storage,sizeof(f.storage),"save",&behavior,&result)&&result.command_queued&&f.calls=="NGSNGGGS");
        assert(api->autosave_input(f.storage,sizeof(f.storage),"SaveDangerous",&behavior,&result)&&!result.handled);
        assert(!api->autosave_input(f.storage,autosave_extent-1,"Save",&behavior,&result));
        assert(api->destroy_autosave(f.storage,autosave_extent,f.storage+0x160,&owner)&&f.released==1);
    }
    {
        alignas(4) unsigned char actor[multicompare_extent]{};
        put(actor,0,multicompare_vtable);
        ActionPool pool{};OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
        auto keys=kb;keys.native_output=&actions;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=multicompare_extent;return 1;};
        assert(api->key_value(actor,"IntegerValue","5",&keys,&kr)&&kr.offset==0x3c4);
        assert(api->key_value(actor,"ShouldComparetoValue","0",&keys,&kr)&&kr.offset==0x3c8);
        assert(api->key_value(actor,"OnEqual","relay,Trigger,,0,-1",&keys,&kr)&&kr.offset==0x394);
        assert(api->key_value(actor,"OnNotEqual","relay,Trigger,,0,-1",&keys,&kr)&&kr.offset==0x3ac);
        struct MultiFixture {void* actor;void* activator;ActionPool* pool;alignas(4) unsigned char rows[4][0x1c]{};unsigned alloc=0,scheduled=0,last_id=0;std::string order;} f{actor,timer,&pool};
        MultiCompareBindings behavior{};behavior.context=&f;
        behavior.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<MultiFixture*>(p);if(!token||token>f.alloc)return 0;*row=f.rows[token-1];*bytes=0x1c;return 1;
        };
        behavior.allocate=[](void* p,unsigned* token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<MultiFixture*>(p);if(f.alloc==4)return 0;*row=f.rows[f.alloc];*token=++f.alloc;*bytes=0x1c;return 1;
        };
        behavior.resolve_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            if(token!=201)return 0;*text="5 trailing";*bytes=11;return 1;
        };
        behavior.schedule=[](void* p,void* target,void* a,void* c,const char* input,float delay,unsigned id) noexcept ->int{
            auto& f=*static_cast<MultiFixture*>(p);assert(target==f.actor&&a==f.activator&&c==f.actor&&!std::strcmp(input,"ForceRecompare")&&!delay);
            ++f.scheduled;f.last_id=id;return 1;
        };
        nw::ServerEntityVoidOutputBindings fire{};fire.context=&f;
        fire.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<MultiFixture*>(p);for(unsigned i=0;i<2;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[i]))){*row=f.pool->rows[i];*bytes=0x1c;return 1;}return 0;
        };
        fire.enqueue=[](void* p,void* a,void* c,const nw::ServerEntityVoidOutputAction* action) noexcept ->int{
            auto& f=*static_cast<MultiFixture*>(p);assert(a==f.activator&&c==f.actor&&!action->variant_type);f.order+=char('0'+action->action_id-41);return 1;
        };
        behavior.output=&fire;MultiCompareResult result{};
        unsigned v[5]{5,0x11223344,0x55667788,0xffffffffu,5};
        auto mi=[&](const char* name,unsigned id=0){return api->multicompare_input(actor,sizeof(actor),name,timer,v,sizeof(v),id,&behavior,&result);};
        assert(mi("CompareValues")&&result.equal&&f.order=="0");
        assert(mi("InputValue",17)&&result.inserted&&result.scheduled&&f.last_id==17&&word(actor,0x3d0)==1&&word(actor,0x3cc)==1);
        assert(!std::memcmp(f.rows[0],v,0x14)&&word(f.rows[0],0x14)==17&&!word(f.rows[0],0x18));
        v[0]=0x40b00000u;v[4]=1;
        assert(mi("InputValue",18)&&result.inserted&&!result.scheduled&&word(f.rows[1],0)==5&&word(f.rows[1],0x10)==5&&word(f.rows[1],0x18)==1&&word(actor,0x3cc)==2);
        v[0]=201;v[4]=2;
        assert(mi("InputValue",17)&&result.replaced&&!result.scheduled&&f.alloc==2&&word(f.rows[0],0)==5);
        assert(mi("ForceRecompare")&&!result.handled&&word(actor,0x3d0)==1);
        assert(mi("CompareValues")&&result.equal&&result.visited==2&&!word(actor,0x3d0)&&f.order=="00");
        v[0]=6;v[4]=5;
        assert(mi("InputValue",18)&&result.replaced&&result.scheduled&&f.last_id==18);
        assert(mi("CompareValues")&&!result.equal&&f.order=="001");
        assert(api->key_value(actor,"ShouldComparetoValue","1",&keys,&kr));
        v[0]=6;assert(mi("InputValue",17)&&result.replaced);
        assert(mi("CompareValues")&&!result.equal&&f.order=="0011");
        assert(api->key_value(actor,"IntegerValue","6",&keys,&kr));
        assert(mi("CompareValues")&&result.equal&&f.order=="00110");
        v[4]=6;assert(mi("InputValue",19)&&result.rejected&&f.alloc==2&&!word(actor,0x3d0));
        put(f.rows[0],0x18,2);assert(!mi("CompareValues")&&!result.completed);
    }
    {
        FactoryDictionary dict{};
        assert(api->register_factories(&dict)&&api->register_multicompare(&dict)&&api->register_autosave(&dict)&&api->register_navigation(&dict)&&api->register_menu(&dict)&&api->register_counter(&dict)&&api->register_compare(&dict)&&api->register_case(&dict)&&api->register_branch(&dict)&&api->register_auto(&dict)&&api->register_timer(&dict)&&api->register_relay(&dict)&&dict.count==23);
        assert(!api->register_multicompare(&dict));
        const auto* spec=api->find_factory(&dict,"LOGIC_MULTICOMPARE");
        assert(spec&&spec->family==Family::LogicMultiCompare&&spec->original_object==0x224fb600&&spec->original_vtable==0x2241f584&&spec->extent==multicompare_extent);
        struct MultiOwner {alignas(4) unsigned char storage[multicompare_extent]{},rows[5][0x1c]{};unsigned alloc=0,base=0,init=0,cleared=0,accepted=0,released=0,base_dead=0,freed=0;unsigned order[5]{};const sin_port::native_map::Api* api=nullptr;LogicDestroyBindings teardown{};LogicDestroyResult destroyed{};} f;
        f.api=api;
        RelayFactoryBindings owner{};owner.context=&f;
        owner.allocate=[](void* p,unsigned n,void** a,unsigned* bytes) noexcept ->int{auto& f=*static_cast<MultiOwner*>(p);assert(n==multicompare_extent);++f.alloc;*a=f.storage;*bytes=n;return 1;};
        owner.base_construct=[](void* p,void* a,int arg) noexcept ->int{auto& f=*static_cast<MultiOwner*>(p);assert(a==f.storage&&arg==1);++f.base;return 1;};
        owner.initialize=[](void* p,void* a,const char* cls) noexcept ->int{auto& f=*static_cast<MultiOwner*>(p);assert(a==f.storage&&f.base==1&&!std::strcmp(cls,"logic_multicompare"));++f.init;return 1;};
        owner.release_networkable=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<MultiOwner*>(p);assert(a==f.storage+0x160);return f.api->destroy_logic(f.storage,sizeof(f.storage),1,&f.teardown,&f.destroyed);};
        FactoryBindings factories{};factories.dictionary=&dict;factories.logic_multicompare=&owner;
        auto keys=kb;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=multicompare_extent;return 1;};
        keys.intern=[](void*,const char* text,unsigned* token) noexcept ->int{if(std::strcmp(text,"logic_multicompare"))return 0;*token=201;return 1;};
        Bindings parse{};parse.context=&f;parse.native_factory=&factories;parse.native_keys=&keys;
        parse.clear_factory_global=[](void* p) noexcept ->int{++static_cast<MultiOwner*>(p)->cleared;return 1;};
        parse.accept_entity=[](void* p,void* a,const char*,unsigned,const EntitySlice*) noexcept ->int{auto& f=*static_cast<MultiOwner*>(p);assert(a==f.storage);++f.accepted;return 1;};
        const char map[]="{ classname logic_multicompare IntegerValue 7 ShouldComparetoValue 1 }";
        Result parsed{};
        assert(api->parse(map,sizeof(map),&parse,&parsed)&&parsed.native_creates==1&&parsed.last_factory.networkable==f.storage+0x160);
        assert(f.alloc==1&&f.base==1&&f.init==1&&f.cleared==1&&f.accepted==1&&word(f.storage,0)==multicompare_vtable&&word(f.storage,0x3c4)==7&&word(f.storage,0x3c8)==1&&!word(f.storage,0x3cc)&&!word(f.storage,0x3d0));
        for(unsigned off=0x394;off<=0x3ac;off+=0x18)assert(!word(f.storage,off)&&word(f.storage,off+0xc)==0xffffffffu&&!word(f.storage,off+0x10)&&!word(f.storage,off+0x14));
        // Original pool destruction: successors of comparison head,
        // comparison head, OnNotEqual list, OnEqual list, base, free.
        put(f.storage,0x3cc,3);put(f.rows[2],0x18,2);put(f.rows[1],0x18,1);
        put(f.storage,0x3c0,4);put(f.storage,0x3a8,5);
        auto& d=f.teardown;d.context=&f;
        d.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{auto& f=*static_cast<MultiOwner*>(p);if(!token||token>5)return 0;*row=f.rows[token-1];*bytes=0x1c;return 1;};
        d.release_action=[](void* p,unsigned token,void* row) noexcept ->int{
            auto& f=*static_cast<MultiOwner*>(p);assert(token&&token<=5&&row==f.rows[token-1]&&f.released<5);
            unsigned next=0;std::memcpy(&next,f.rows[2]+0x18,4);
            if(token==2)assert(next==1);if(token==1||token==3)assert(!next);
            f.order[f.released++]=token;return 1;
        };
        d.base_destroy=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<MultiOwner*>(p);assert(a==f.storage&&f.released==5);++f.base_dead;return 1;};
        d.free_actor=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<MultiOwner*>(p);assert(a==f.storage&&f.base_dead==1);++f.freed;return 1;};
        // Cross-list alias must refuse before ANY pool release.
        put(f.storage,0x3a8,2);
        assert(!api->destroy_multicompare(f.storage,sizeof(f.storage),f.storage+0x160,&owner)&&!f.released&&!f.destroyed.validated);
        put(f.storage,0x3a8,5);
        assert(api->destroy_multicompare(f.storage,sizeof(f.storage),f.storage+0x160,&owner)&&f.destroyed.completed&&f.destroyed.comparisons==3&&f.destroyed.comparisons_released==3&&f.destroyed.outputs==2&&f.destroyed.released==2&&f.freed==1);
        const unsigned expected[]{2,1,3,4,5};assert(!std::memcmp(f.order,expected,sizeof(expected)));
        assert(word(f.storage,0x3cc)==3&&word(f.storage,0x3c0)==4&&word(f.storage,0x3a8)==5);
    }
    {
        alignas(4) unsigned char actor[multicompare_extent]{};
        put(actor,0,multicompare_vtable);put(actor,0x378,0x4001u);actor[0x1f0]=1;
        ActionPool pool{};OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
        auto keys=kb;keys.native_output=&actions;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=multicompare_extent;return 1;};
        assert(api->key_value(actor,"OnEqual","relay,Trigger,,0,-1",&keys,&kr));
        struct MultiQueue {ConnectedOutput* queue;ActionPool* pool;void* actor;void* activator;alignas(4) unsigned char records[2][0x1c]{},base[0x10]{};unsigned alloc=0,fired=0,rejected=0,kill_order=0,removes=0;} f{&co,&pool,actor,timer};
        std::memset(co.root,0,sizeof(co.root));std::memset(co.rows,0,sizeof(co.rows));co.alloc=0;co.released=0;co.now=160.f;
        co.queue.string_nonempty=[](void*,unsigned,bool* yes) noexcept ->int{*yes=true;return 1;};
        MultiCompareBindings behavior{};behavior.context=&f;
        behavior.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{auto& f=*static_cast<MultiQueue*>(p);if(!token||token>f.alloc)return 0;*row=f.records[token-1];*bytes=0x1c;return 1;};
        behavior.allocate=[](void* p,unsigned* token,void** row,unsigned* bytes) noexcept ->int{auto& f=*static_cast<MultiQueue*>(p);if(f.alloc==2)return 0;*row=f.records[f.alloc];*token=++f.alloc;*bytes=0x1c;return 1;};
        behavior.resolve_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{if(token!=401)return 0;*text="5 units";*bytes=8;return 1;};
        nw::ServerEntityVoidOutputBindings fire{};fire.context=&f;
        fire.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{auto& f=*static_cast<MultiQueue*>(p);if(token!=static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[0])))return 0;*row=f.pool->rows[0];*bytes=0x1c;return 1;};
        fire.enqueue=[](void* p,void* a,void* c,const nw::ServerEntityVoidOutputAction* action) noexcept ->int{auto& f=*static_cast<MultiQueue*>(p);assert(a==f.activator&&c==f.actor&&action->action_id==41&&!action->variant_type);++f.fired;return 1;};
        behavior.output=&fire;
        RelayQueueBindings direct{};direct.context=&f;direct.root=co.root;direct.root_bytes=sizeof(co.root);direct.root_token=99;direct.queue=&co.queue;
        direct.handle=[](void* p,void* a,unsigned* handle) noexcept ->int{auto& f=*static_cast<MultiQueue*>(p);if(a==f.actor)*handle=0x4001u;else if(a==f.activator)*handle=0x3000u;else return 0;return 1;};
        direct.intern=[](void*,const char* text,unsigned* token) noexcept ->int{assert(!std::strcmp(text,"ForceRecompare"));*token=0x2241f628u;return 1;};
        behavior.native_queue=&direct; // service supplies authoritative live time and handles.
        EntityKillBindings kill{};kill.context=&f;
        kill.attachment_notice=[](void* p,void* attached,void* actor) noexcept ->int{
            auto& f=*static_cast<MultiQueue*>(p);assert(attached==f.activator&&actor==f.actor&&!f.kill_order);f.kill_order=1;return 1;
        };
        kill.clear_attachment=[](void* p,void* actor) noexcept ->int{
            auto& f=*static_cast<MultiQueue*>(p);assert(actor==f.actor&&f.kill_order==1);const unsigned invalid=0xffffffffu;std::memcpy(static_cast<unsigned char*>(actor)+0x20c,&invalid,4);f.kill_order=2;return 1;
        };
        kill.remove=[](void* p,void* actor) noexcept ->int{
            auto& f=*static_cast<MultiQueue*>(p);assert(actor==f.actor&&f.kill_order==2);++f.removes;return 1;
        };
        behavior.kill=&kill;
        nw::ServerNpcActivePostAnimationBindings storage{};storage.context=&f;
        storage.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            // Empty hosted inherited-map fixture: it does not invent an alias.
            auto& f=*static_cast<MultiQueue*>(p);if(token!=0x224f0350u)return 0;*row=f.base;*bytes=sizeof(f.base);return 1;
        };
        nw::ServerEntityAcceptInputBindings input{};input.storage=&storage;input.context=&f;input.logic_multicompare=&behavior;
        input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            *text=token==0x2241ffb8u?"InputValue":token==0x2241ff14u?"CompareValues":token==0x2241f628u?"ForceRecompare":token==0x2240b624u?"Kill":token==402?"kIlL":nullptr;
            if(!*text)return 0;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        input.diagnostic=[](void* p,void* a,void* c,unsigned token,unsigned,const void*) noexcept ->int{
            auto& f=*static_cast<MultiQueue*>(p);assert(a==f.actor&&c==f.actor&&token==0x2241f628u);++f.rejected;return 1;
        };
        sin_independent::EntityHandleEntry entries[2]{};
        entries[0].serial=3;entries[0].entity=reinterpret_cast<CBaseEntity*>(timer);
        entries[1].serial=4;entries[1].entity=reinterpret_cast<CBaseEntity*>(actor);
        nw::ServerEntitySearchBindings search{};search.context=&f;
        search.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{*bytes=a==static_cast<MultiQueue*>(p)->actor?multicompare_extent:0;return *bytes!=0;};
        nw::ServerEntityEventServiceBindings delivery{};delivery.context=&f;delivery.entities.entries=entries;delivery.entities.count=2;delivery.native_input=&input;delivery.native_search=&search;delivery.live_now=&co.now;
        delivery.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{auto& q=*static_cast<MultiQueue*>(p)->queue;return q.queue.resolve(q.queue.context,token,row,bytes);};
        delivery.lock=[](void* p,bool enter) noexcept ->int{auto& q=*static_cast<MultiQueue*>(p)->queue;assert(q.locked!=enter);q.locked=enter;return 1;};
        delivery.release=[](void* p,unsigned token,void* row) noexcept ->int{auto& q=*static_cast<MultiQueue*>(p)->queue;assert(q.locked&&token&&token<=q.alloc&&row==q.rows[token-1]);++q.released;return 1;};
        unsigned a[5]{401,0,0,0xffffffffu,2},b[5]{0x40b00000u,0,0,0xffffffffu,1},v[5]{0,0,0,0xffffffffu,0};
        assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x4001,0x2241ffb8u,0x3000,0x3000,a,sizeof(a),0.f,17,&co.queue,&cancel_qr));
        assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x4001,0x2241ffb8u,0x3000,0x3000,b,sizeof(b),0.f,18,&co.queue,&cancel_qr));
        assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x4001,0x2241ff14u,0x3000,0x3000,v,sizeof(v),.25f,19,&co.queue,&cancel_qr));
        assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&delivery,&es)&&es.events==3&&es.native_inputs==3&&es.rejected_inputs==1&&es.future_stopped&&f.rejected==1&&f.alloc==2&&co.alloc==4&&co.released==3&&!f.fired&&word(actor,0x3d0)==1);
        assert(word(co.rows[3],8)==0x2241f628u&&word(co.rows[3],0x18)==0x4001u&&word(co.rows[3],0xc)==0x3000u&&word(co.rows[3],0x10)==0x4001u&&word(co.rows[3],0x14)==17&&!word(co.rows[3],0x2c));
        assert(word(f.records[0],0)==5&&word(f.records[1],0)==5&&word(f.records[0],0x14)==17&&word(f.records[1],0x14)==18);
        co.now=160.25f;
        assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&delivery,&es)&&es.events==1&&es.native_inputs==1&&!es.rejected_inputs&&f.fired==1&&co.released==4&&!word(co.root,0x30)&&!word(actor,0x3d0));
        // A fresh queue follows the comparison. Kill requests removal, not
        // immediate destruction: comparison/output owner pools remain live.
        std::memset(co.root,0,sizeof(co.root));std::memset(co.rows,0,sizeof(co.rows));co.alloc=0;co.released=0;
        put(actor,0x20c,0x3000u);
        EntityKillResult krill{};
        assert(!api->multicompare_kill(actor,multicompare_extent-1,entries,2,&kill,&krill)&&!krill.completed&&!f.kill_order);
        assert(!api->multicompare_kill(timer,timer_extent,entries,2,&kill,&krill)&&!krill.completed&&!f.kill_order);
        assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x4001,0x2240b624u,0x3000,0x3000,v,sizeof(v),0.f,20,&co.queue,&cancel_qr));
        assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x4001,402,0x3000,0x3000,v,sizeof(v),.25f,21,&co.queue,&cancel_qr));
        assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&delivery,&es)&&es.events==1&&es.native_inputs==1&&!es.rejected_inputs&&es.future_stopped&&f.kill_order==2&&f.removes==1&&co.released==1);
        assert(word(actor,0x20c)==0xffffffffu&&word(actor,0x3cc)==2&&f.alloc==2&&word(actor,0x3a8)==ptrword(pool.rows[0]));
        // Stale serial must not notify or clear the unrelated attachment.
        put(actor,0x20c,0x2000u);co.now+=.25f;
        assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&delivery,&es)&&es.events==1&&es.native_inputs==1&&!es.rejected_inputs&&f.removes==2&&co.released==2&&!word(co.root,0x30));
        assert(word(actor,0x20c)==0x2000u&&word(actor,0x3cc)==2&&f.kill_order==2);
    }
    {
        // Original InputValue -> explicit CompareValues -> named relay
        // delivery. No owner output enqueue; both output branches use the
        // active service queue. ForceRecompare remains an unknown name.
        alignas(4) unsigned char actor[multicompare_extent]{},target[relay_extent]{};
        put(actor,0,multicompare_vtable);put(actor,0x378,0x4001u);actor[0x1f0]=1;actor[0xa8]=0x10;
        put(target,0,relay_vtable);put(target,0x378,0x3000u);target[0x1f0]=1;
        ActionPool pool{};OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
        auto keys=kb;keys.native_output=&actions;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=multicompare_extent;return 1;};
        keys.intern=[](void*,const char* text,unsigned* token) noexcept ->int{
            if(!std::strcmp(text,"target"))*token=201;
            else if(!std::strcmp(text,"Disable"))*token=0x223edce8;
            else if(!std::strcmp(text,"Enable"))*token=0x223edd00;
            else return 0;return 1;
        };
        assert(api->key_value(actor,"IntegerValue","5",&keys,&kr));
        assert(api->key_value(actor,"ShouldComparetoValue","1",&keys,&kr));
        assert(api->key_value(actor,"OnEqual","target,Disable,,0,1",&keys,&kr));
        assert(api->key_value(actor,"OnNotEqual","target,Enable,,0,1",&keys,&kr));
        struct Chain {ConnectedOutput* queue;ActionPool* pool;void* actor;void* target;alignas(4) unsigned char row[0x1c]{},base[0x10]{};unsigned alloc=0,debug=0,rejected=0;} f{&co,&pool,actor,target};
        MultiCompareBindings behavior{};behavior.context=&f;behavior.native_outputs=true;
        behavior.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Chain*>(p);if(token!=1||!f.alloc)return 0;*row=f.row;*bytes=0x1c;return 1;
        };
        behavior.allocate=[](void* p,unsigned* token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Chain*>(p);if(f.alloc)return 0;++f.alloc;*token=1;*row=f.row;*bytes=0x1c;return 1;
        };
        behavior.resolve_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            *text=token==401?" +4294967301 trailing":token==402?" -4294967290 trailing":nullptr;
            if(!*text)return 0;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        nw::ServerEntityVoidOutputBindings fire{};fire.context=&f;
        fire.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Chain*>(p);for(unsigned i=0;i<2;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[i]))){*row=f.pool->rows[i];*bytes=0x1c;return 1;}return 0;
        };
        fire.debug_action=[](void* p,void* caller,const nw::ServerEntityVoidOutputAction* action) noexcept ->int{
            auto& f=*static_cast<Chain*>(p);assert(caller==f.actor&&action->action_id==41+f.debug&&action->target==201&&!action->variant_type&&!action->delay);++f.debug;return 1;
        };
        fire.release=[](void* p,unsigned token,void* row) noexcept ->int{
            auto& f=*static_cast<Chain*>(p);unsigned count=0;std::memcpy(&count,static_cast<unsigned char*>(row)+0x10,4);assert(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(row))&&row==f.pool->rows[f.pool->released]&&!count);++f.pool->released;return 1;
        };
        behavior.output=&fire; // intentionally no enqueue callback
        RelayQueueBindings direct{};direct.context=&f;direct.root=co.root;direct.root_bytes=sizeof(co.root);direct.root_token=99;direct.queue=&co.queue;
        direct.handle=[](void* p,void* actor,unsigned* handle) noexcept ->int{
            auto& f=*static_cast<Chain*>(p);if(actor==f.actor)*handle=0x4001u;else if(actor==f.target)*handle=0x3000u;else return 0;return 1;
        };
        direct.intern=[](void*,const char* text,unsigned* token) noexcept ->int{if(std::strcmp(text,"ForceRecompare"))return 0;*token=0x2241f628u;return 1;};
        behavior.native_queue=&direct;
        RelayBindings relay_behavior{};
        nw::ServerNpcActivePostAnimationBindings storage{};storage.context=&f;
        storage.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Chain*>(p);if(token!=0x224f0350u)return 0;*row=f.base;*bytes=sizeof(f.base);return 1;
        };
        nw::ServerEntityAcceptInputBindings input{};input.context=&f;input.storage=&storage;input.logic_multicompare=&behavior;input.relay=&relay_behavior;
        input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            switch(token){
                case 0x2241ffb8u:*text="InputValue";break;
                case 0x2241ff14u:*text="CompareValues";break;
                case 0x2241f628u:*text="ForceRecompare";break;
                case 0x223edce8u:*text="Disable";break;
                case 0x223edd00u:*text="Enable";break;
                default:return 0;
            }
            *bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        input.diagnostic=[](void* p,void* actor,void* caller,unsigned name,unsigned,const void*) noexcept ->int{
            auto& f=*static_cast<Chain*>(p);assert(actor==f.actor&&caller==f.actor&&name==0x2241f628u);++f.rejected;return 1;
        };
        sin_independent::EntityHandleEntry entries[2]{};
        entries[0].serial=3;entries[0].entity=reinterpret_cast<CBaseEntity*>(target);
        entries[1].serial=4;entries[1].entity=reinterpret_cast<CBaseEntity*>(actor);
        nw::ServerEntitySearchBindings search{};search.context=&f;
        search.extent=[](void* p,void* actor,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Chain*>(p);*bytes=actor==f.actor?multicompare_extent:actor==f.target?relay_extent:0;return *bytes!=0;
        };
        nw::ServerEntityEventServiceBindings delivery{};delivery.context=&f;delivery.entities.entries=entries;delivery.entities.count=2;delivery.native_input=&input;delivery.native_search=&search;delivery.live_now=&co.now;
        delivery.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{auto& q=*static_cast<Chain*>(p)->queue;return q.queue.resolve(q.queue.context,token,row,bytes);};
        delivery.lock=[](void* p,bool enter) noexcept ->int{auto& q=*static_cast<Chain*>(p)->queue;assert(q.locked!=enter);q.locked=enter;return 1;};
        delivery.find_named=[](void* p,void* prev,unsigned name,void* a,void* c,void** out) noexcept ->int{
            auto& f=*static_cast<Chain*>(p);assert(f.queue->locked&&name==201&&a==f.target&&c==f.actor);*out=prev?nullptr:f.target;return 1;
        };
        delivery.release=[](void* p,unsigned token,void* row) noexcept ->int{auto& q=*static_cast<Chain*>(p)->queue;assert(q.locked&&token&&token<=q.alloc&&row==q.rows[token-1]);++q.released;return 1;};
        for(unsigned i=0;i<2;++i){
            std::memset(co.root,0,sizeof(co.root));std::memset(co.rows,0,sizeof(co.rows));co.alloc=0;co.released=0;co.now=170.f+i;
            co.queue.string_nonempty=[](void*,unsigned,bool* yes) noexcept ->int{*yes=true;return 1;};
            unsigned incoming[5]{401+i,0,0,0xffffffffu,2},v[5]{0,0,0,0xffffffffu,0};
            assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x4001,0x2241ffb8u,0x3000,0x3000,incoming,sizeof(incoming),0.f,17,&co.queue,&cancel_qr));
            assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x4001,0x2241ff14u,0x3000,0x3000,v,sizeof(v),.25f,18,&co.queue,&cancel_qr));
            assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&delivery,&es)&&es.events==2&&es.native_inputs==2&&es.rejected_inputs==1&&es.future_stopped&&f.rejected==i+1&&f.alloc==1&&word(f.row,0)==5+i&&word(f.row,0x14)==17);
            co.now+=.25f;
            assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&delivery,&es)&&es.events==2&&es.native_inputs==2&&!es.rejected_inputs&&f.debug==i+1&&pool.released==i+1&&co.released==4&&!word(co.root,0x30));
            assert(target[0x3ac]==(i?0:1)&&!word(actor,0x3d0)&&!word(actor,i?0x3c0:0x3a8));
            assert(word(co.rows[3],4)==201&&word(co.rows[3],8)==(i?0x223edd00u:0x223edce8u)&&word(co.rows[3],0xc)==0x3000u&&word(co.rows[3],0x10)==0x4001u&&word(co.rows[3],0x14)==41+i&&!word(co.rows[3],0x2c));
        }
    }
    FreeLibrary(server);FreeLibrary(client);
}
