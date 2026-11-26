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
    FreeLibrary(server);FreeLibrary(client);
}
