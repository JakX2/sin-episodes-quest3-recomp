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
    assert(timer_ai(0x22420090,2,303)&&tair.converted&&tair.native_handler&&word(timer,0x3e0)==0x3e800000u);
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
    nw::ServerEntityVoidOutputBindings df{};df.context=&dq;
    df.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
        auto& f=*static_cast<DecimalQueue*>(p);if(token!=static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[0])))return 0;
        *row=f.pool->rows[0];*bytes=0x1c;return 1;
    };
    df.enqueue=[](void* p,void* a,void* c,const nw::ServerEntityVoidOutputAction* action) noexcept ->int{
        auto& f=*static_cast<DecimalQueue*>(p);assert(a==f.actor&&c==f.actor&&action->parameter==304&&action->variant_type==2);
        nw::ServerEntityNamedEventQueueResult out{};auto& q=*f.queue;
        return q.api->enqueue(q.root,sizeof(q.root),99,q.now,0x3000u,0x3000u,action,&q.queue,&out);
    };
    df.release=[](void* p,unsigned,void* row) noexcept ->int{
        auto& f=*static_cast<DecimalQueue*>(p);assert(row==f.pool->rows[0]);++f.pool->released;return 1;
    };
    tes.find_named=[](void* p,void* prev,unsigned name,void* a,void* c,void** target) noexcept ->int{
        auto& f=*static_cast<ConnectedOutput*>(p);assert(f.locked&&name==311&&a==f.actor&&c==f.actor);*target=prev?nullptr:f.actor;return 1;
    };
    nw::ServerEntityVoidOutputResult decimal_fire{};
    assert(fire_api->fire(timer+0x394,0x18,timer,timer,sizeof(timer),0.f,&df,&decimal_fire)&&decimal_fire.queued==1&&decimal_fire.removed==1&&decimal_pool.released==1);
    assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&tes,&es)&&es.native_inputs==1&&word(timer,0x3e0)==0x41480000u&&word(timer,0x74)==820&&co.released==1);
    FreeLibrary(server);FreeLibrary(client);
}
