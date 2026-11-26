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
    struct RelayFixture {ActionPool* pool;void* actor;void* activator;unsigned queued=0,released=0,removed=0,canceled=0,refires=0;float delay=0;} rf{&pool,relay,co.actor};
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
    auto input=[&](const char* name){return api->relay_input(relay,sizeof(relay),name,co.actor,&rb,&rr);};
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
    FreeLibrary(server);FreeLibrary(client);
}
