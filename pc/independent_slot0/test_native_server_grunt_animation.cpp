#include "native_server_grunt_animation.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <vector>
#include <cstdio>
#include <limits>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char actor[0xfcc]{},peer[0xc94]{},weapon[0x500]{};
    void* peers[2]{actor,peer};
    std::vector<int> trace;
    bool has_weapon=true;
    const ServerGruntAnimationApi* api{};
    ServerGruntAnimationBindings b{};
    ServerGruntAnimationResult last{};
    unsigned delivered=0;
    template<class T> void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T> T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto g=reinterpret_cast<GetServerGruntAnimationApi>(GetProcAddress(server,server_grunt_animation_export));
    auto ge=reinterpret_cast<GetServerAnimationEventsApi>(GetProcAddress(server,server_animation_events_export));
    assert(g&&ge&&!GetProcAddress(client,server_grunt_animation_export)&&!g(0,sizeof(ServerGruntAnimationApi))&&!g(1,0));
    F f;f.api=g(1,sizeof(ServerGruntAnimationApi));auto* events=ge(1,sizeof(ServerAnimationEventsApi));assert(f.api&&events);
    f.b.context=&f;f.b.begin_altfire=1101;f.b.altfire=1102;
    f.b.debug=[](void* p,const char* s) noexcept {assert(!std::strcmp(s,"BLOCKING!\n"));F::self(p).trace.push_back(14);};
    f.b.weapon=[](void* p,void* a,std::uint32_t handle,void** w) noexcept -> int {auto& f=F::self(p);assert(a==f.actor&&handle==0x1234);f.trace.push_back(100);*w=f.has_weapon?f.weapon:nullptr;return 1;};
    f.b.weapon_mode=[](void* p,void* w,int a,int b) noexcept {auto& f=F::self(p);assert(w==f.weapon&&a==9&&b==0);f.trace.push_back(101);};
    f.b.weapon_value=[](void* p,void* w,unsigned s) noexcept -> int {auto& f=F::self(p);assert(w==f.weapon&&(s==0x404||s==0x408));f.trace.push_back(s);return s==0x404?12:34;};
    f.b.weapon_store=[](void* p,void* w,unsigned off,int v) noexcept {auto& f=F::self(p);assert(w==f.weapon);f.put(w,off,v);f.trace.push_back(off);};
    f.b.clear_condition=[](void* p,void* a,int c) noexcept {auto& f=F::self(p);assert(a==f.actor);f.trace.push_back(c);};
    f.b.weapon_event=[](void* p,void* w,int ev,void* a) noexcept {auto& f=F::self(p);assert(w==f.weapon&&a==f.actor&&ev==0xc1f&&f.get<float>(a,0xc90)==5.f);f.trace.push_back(ev);};
    f.b.peers=[](void* p,void* a,std::uint32_t squad,void*** peers,unsigned* count) noexcept -> int {auto& f=F::self(p);assert(a==f.actor&&squad==0x8888);*peers=f.peers;*count=2;return 1;};
    f.b.activity=[](void* p,void* a) noexcept -> int {auto& f=F::self(p);assert(a==f.actor);f.trace.push_back(200);return 44;};
    f.b.activity_request=[](void* p,void* a,int act,int x,int y) noexcept {auto& f=F::self(p);assert(a==f.actor&&act==44&&x==0&&y==2);f.trace.push_back(201);};
    f.b.parent=[](void* p,void* a,const ServerAnimationEvent* e) noexcept {auto& f=F::self(p);assert(a==f.actor);f.trace.push_back(e->event+10000);};
    f.put(f.actor,0x78c,0x1234u);f.put(f.actor,0x9c4,0x8888u);f.put(f.actor,0xc90,5.f);f.put(f.peer,0xc90,20.f);f.put(f.actor,0xc38,3u);f.put(f.actor,0xfc8,99);
    // Real reconstructed base animation iterator feeds the Grunt handler.
    ServerStudioEvent records[4]{{.1f,0x14,0,"",0},{.2f,2,0,"",0},{.3f,1102,0x400,"",0},{.4f,8,0,"",0}};
    ServerAnimationEventSequence seq{records,4,0,0};
    struct C {F* f;ServerAnimationEventSequence* seq;} c{&f,&seq};
    ServerAnimationEventBindings sb{&c,
        [](void* p,void* a,int id,ServerAnimationEventSequence* out) noexcept -> int {auto& c=*static_cast<C*>(p);assert(a==c.f->actor&&id==7);*out=*c.seq;return 1;},
        [](void*,void*,int,double* r) noexcept -> int {*r=1.;return 1;},
        [](void*,std::uint32_t,void**,unsigned*) noexcept -> int {return 0;},
        [](void* p,void* a,int layer,const ServerAnimationEvent* e) noexcept {auto& f=*static_cast<C*>(p)->f;assert(layer==-1&&a==f.actor&&e->source==f.actor);assert(f.api->handle(a,sizeof(f.actor),e,8,&f.b,&f.last)==1);++f.delivered;}
    };
    f.put(f.actor,0x3c0,1.f);f.put(f.actor,0x3ec,7);f.put(f.actor,0x3e8,.5f);
    ServerAnimationEventResult er{};
    assert(events->dispatch(f.actor,sizeof(f.actor),f.actor,8,&sb,&er)&&er.emitted==4&&f.delivered==4);
    assert((f.trace==std::vector<int>{14,100,101,0x404,0x4ec,0x408,0x4f0,3,4,5,100,0xc1f}));
    assert(f.get<std::uint8_t>(f.actor,0xc35)==1&&f.get<int>(f.actor,0xfc8)==8&&f.get<unsigned>(f.actor,0xc38)==2);
    assert(f.get<float>(f.actor,0xc90)==18.f&&f.get<float>(f.peer,0xc90)==20.f&&f.get<int>(f.weapon,0x4ec)==12&&f.get<int>(f.weapon,0x4f0)==34);
    auto run=[&](int id){ServerAnimationEvent e{id,"",0,8,0,f.actor};return f.api->handle(f.actor,sizeof(f.actor),&e,8,&f.b,&f.last);};
    f.trace.clear();assert(run(1101)==1&&f.last.route==GruntAnimBeginAlt&&f.trace.empty()&&f.get<int>(f.actor,0xfc8)==8);
    assert(run(3)==0&&f.last.route==GruntAnimUnrecovered&&f.trace.empty()&&f.get<int>(f.actor,0xfc8)==8);
    assert(run(10)==1&&f.last.route==GruntAnimActivity&&(f.trace==std::vector<int>{200,201})&&f.get<int>(f.actor,0xfc8)==10);
    f.trace.clear();assert(run(5)==1&&f.last.route==GruntAnimParent&&(f.trace==std::vector<int>{10005})&&f.get<int>(f.actor,0xfc8)==10);
    f.trace.clear();f.has_weapon=false;
    assert(run(2)==1&&(f.trace==std::vector<int>{100,3,4,5})&&f.get<int>(f.actor,0xfc8)==2);
    f.trace.clear();assert(run(1102)==-1&&f.get<unsigned>(f.actor,0xc38)==2);

    auto gr=reinterpret_cast<GetServerGruntRegistrationApi>(GetProcAddress(server,server_grunt_registration_export));
    auto ga=reinterpret_cast<GetServerActivityDictionaryApi>(GetProcAddress(server,server_activity_dictionary_export));
    auto gd=reinterpret_cast<GetServerEventDictionaryApi>(GetProcAddress(server,server_event_dictionary_export));
    assert(gr&&ga&&gd&&!GetProcAddress(client,server_grunt_registration_export)&&!gr(0,sizeof(ServerGruntRegistrationApi))&&!gr(1,0));
    auto* registration=gr(1,sizeof(ServerGruntRegistrationApi));auto* activity=ga(1,sizeof(ServerActivityDictionaryApi));auto* dict=gd(1,sizeof(ServerEventDictionaryApi));assert(registration&&activity&&dict);
    ServerEventDictionaryEntry ar[16]{},ev[32]{};
    ServerActivityDictionary activities{{ar,16,0,4,0},-1};ServerEventDictionary events_dict{ev,32,0,7,0};
    assert(activity->shared(&activities,"ACT_HL2MP_JUMP_SLAM",920)&&dict->seed(&events_dict));
    int filler=-1;assert(activity->private_id(&activities,"PRIVATE_FIRST",&filler)&&filler==921);
    assert(dict->private_id(&events_dict,"PRIVATE_EVENT_FIRST",&filler)&&filler==24);
    ServerGruntRegistration ids{};
    struct RegistrationTrace {std::vector<int> ids;std::vector<bool> kinds;} rt;
    ServerGruntRegistrationBindings rb{&rt,[](void* p,bool event,const char* name,int id) noexcept {
        auto& t=*static_cast<RegistrationTrace*>(p);assert(name&&*name);t.ids.push_back(id);t.kinds.push_back(event);
    }};
    assert(registration->jetpack(&events_dict,&rb,&ids)&&ids.events_ready&&!ids.activities_ready&&ids.events[0]==25&&ids.events[1]==26);
    assert((rt.ids==std::vector<int>{25,26}));rt.ids.clear();rt.kinds.clear();
    assert(registration->grunt(&activities,&events_dict,&rb,&ids)&&ids.activities_ready&&ids.events_ready);
    assert((rt.ids==std::vector<int>{922,923,924,925,926,927,25,26}));
    for(unsigned i=0;i<8;++i)assert(rt.kinds[i]==(i>=6));
    assert(events_dict.count==27&&activities.names.count==8);
    struct ActivityFixture {F* f;bool launcher=false,identity=false;unsigned weapon_calls=0,identity_calls=0,delegate_calls=0,parent_calls=0;} af{&f};
    ServerGruntActivityBindings ab{&af,
        [](void* p,void* a,const char* name,int owner,bool* found) noexcept -> int {
            auto& x=*static_cast<ActivityFixture*>(p);assert(a==x.f->actor&&!std::strcmp(name,"weapon_grenadelauncher")&&owner==0);
            ++x.weapon_calls;*found=x.launcher;return 1;},
        [](void* p,void* a,std::uint32_t c,bool* equal) noexcept -> int {
            auto& x=*static_cast<ActivityFixture*>(p);assert(a==x.f->actor&&c==0x111);++x.identity_calls;*equal=x.identity;return 1;},
        [](void* p,std::uint32_t c,int input,int* out) noexcept -> int {
            auto& x=*static_cast<ActivityFixture*>(p);assert(c==0x222);++x.delegate_calls;*out=input+1000;return 1;},
        [](void* p,void* a,int input,int* out) noexcept -> int {
            auto& x=*static_cast<ActivityFixture*>(p);assert(a==x.f->actor);++x.parent_calls;*out=input;return 1;}
    };
    f.put(f.actor,0xdbc,0x111u);f.put(f.actor,0xc18,0u);
    int mapped=-1;
    assert(registration->translate(f.actor,sizeof(f.actor),17,&ids,&ab,&mapped)&&mapped==922);
    af.launcher=true;assert(registration->translate(f.actor,sizeof(f.actor),17,&ids,&ab,&mapped)&&mapped==923&&af.weapon_calls==2&&!af.identity_calls);
    f.put(f.actor,0xc64,std::uint8_t{0});f.put(f.actor,0xca0,std::uint8_t{1});
    assert(registration->translate(f.actor,sizeof(f.actor),16,&ids,&ab,&mapped)&&mapped==199);
    assert(registration->translate(f.actor,sizeof(f.actor),66,&ids,&ab,&mapped)&&mapped==200);
    f.put(f.actor,0xca0,std::uint8_t{0});af.identity=true;
    assert(registration->translate(f.actor,sizeof(f.actor),6,&ids,&ab,&mapped)&&mapped==7&&f.get<std::uint8_t>(f.actor,0xc64)==1);
    f.put(f.actor,0xc18,0x222u);
    assert(registration->translate(f.actor,sizeof(f.actor),10,&ids,&ab,&mapped)&&mapped==1011&&af.delegate_calls==1);
    // Studio registration resolves the actual runtime event IDs, then native iterator -> Grunt handler.
    alignas(4) unsigned char studio[0x180]{};
    f.put(studio,0x18,2);f.put(studio,0x1c,0x40);
    ServerStudioEvent named[2]{{.1f,-1,0x400,"",0xc0},{.2f,-1,0x400,"",0xa0}};
    std::memcpy(studio+0x40,named,sizeof(named));
    std::strcpy(reinterpret_cast<char*>(studio+0x100),"COMBINE_AE_BEGIN_ALTFIRE");
    std::strcpy(reinterpret_cast<char*>(studio+0x130),"COMBINE_AE_ALTFIRE");
    ServerStudioEventRegistrationResult rr{};
    assert(dict->apply(&events_dict,studio,sizeof(studio),0,&rr)&&rr.existing==2&&!rr.created);
    std::memcpy(named,studio+0x40,sizeof(named));
    ServerAnimationEventSequence named_seq{named,2,0,0};
    f.has_weapon=true;f.trace.clear();f.put(f.actor,0xc90,5.f);f.put(f.actor,0xc38,3u);
    ServerAnimationEvent delivered{};
    assert(events->next(&named_seq,0,.5f,0,8,&delivered)==1&&delivered.event==25);
    assert(registration->handle(f.actor,sizeof(f.actor),&delivered,8,&ids,&f.b,&f.last)==1&&f.last.route==GruntAnimBeginAlt);
    assert(events->next(&named_seq,0,.5f,1,8,&delivered)==2&&delivered.event==26);
    assert(registration->handle(f.actor,sizeof(f.actor),&delivered,8,&ids,&f.b,&f.last)==1&&f.last.route==GruntAnimAlt&&f.get<unsigned>(f.actor,0xc38)==2);
    assert((f.trace==std::vector<int>{100,0xc1f}));
    assert(dict->clear(&events_dict)&&registration->handle(f.actor,sizeof(f.actor),&delivered,8,&ids,&f.b,&f.last)==-1);
    assert(dict->seed(&events_dict)&&registration->jetpack(&events_dict,nullptr,&ids)&&ids.events[0]==27&&ids.events[1]==28);
    assert(activity->clear(&activities)&&!registration->translate(f.actor,sizeof(f.actor),17,&ids,&ab,&mapped));

    auto gj=reinterpret_cast<GetServerJetpackActivityApi>(GetProcAddress(server,server_jetpack_activity_export));
    assert(gj&&!GetProcAddress(client,server_jetpack_activity_export)&&!gj(0,sizeof(ServerJetpackActivityApi))&&!gj(1,0));
    auto* jet=gj(1,sizeof(ServerJetpackActivityApi));assert(jet);
    struct J {
        alignas(4) unsigned char actor[0x1028]{},attachment[0xc24]{};
        unsigned resolutions=0,delegations=0;
        bool live=true;
    } jf;
    ServerJetpackActivityBindings jb{};
    jb.context=&jf;jb.grunt.context=&jf;
    jb.attachment=[](void* p,std::uint32_t h,void** e,unsigned* extent) noexcept -> int {
        auto& j=*static_cast<J*>(p);assert(h==0x1234);++j.resolutions;
        if(!j.live)return 0;*e=j.attachment;*extent=sizeof(j.attachment);return 1;
    };
    jb.grunt.component_identity=[](void* p,void* a,std::uint32_t c,bool* identity) noexcept -> int {
        auto& j=*static_cast<J*>(p);assert(a==j.actor&&c==0x111);*identity=false;return 1;
    };
    jb.grunt.parent_translate=[](void* p,void* a,int i,int* out) noexcept -> int {
        auto& j=*static_cast<J*>(p);assert(a==j.actor);++j.delegations;*out=i;return 1;
    };
    f.put(jf.actor,0x1024,0x1234u);f.put(jf.actor,0xdbc,0x111u);
    auto vector=[&](float x,float y,float z){f.put(jf.attachment,0xc18,x);f.put(jf.attachment,0xc1c,y);f.put(jf.attachment,0xc20,z);};
    ServerJetpackActivityResult jr{};
    auto jet_run=[&](int incoming){return jet->translate(jf.actor,sizeof(jf.actor),incoming,nullptr,&jb,&jr);};
    vector(30,40,0);
    for(int incoming:{1,7,16})assert(jet_run(incoming)&&jr.activity==208&&jr.magnitude==50.f&&!jr.delegated);
    vector(50,.001f,0);assert(jet_run(1)&&jr.activity==208&&jr.magnitude==50.f);
    vector(50,.02f,0);assert(jet_run(1)&&jr.activity==213&&jr.magnitude>50.f);
    vector(0,0,51);assert(jet_run(16)&&jr.activity==213);
    assert(jet_run(66)&&jr.activity==218&&!jr.delegated);
    assert(jet_run(6)&&jr.activity==6&&jr.delegated&&jf.delegations==1&&f.get<std::uint8_t>(jf.actor,0xc64)==1);
    assert(jf.resolutions==8);
    jf.live=false;assert(!jet_run(66)&&!jr.delegated&&jf.resolutions==9);
    jf.live=true;f.put(jf.actor,0x1024,0xffffffffu);assert(!jet_run(6)&&jf.resolutions==9);
    f.put(jf.actor,0x1024,0x1234u);vector(std::numeric_limits<float>::quiet_NaN(),0,0);
    assert(!jet_run(66)&&jf.resolutions==10&&!jr.delegated);
    FreeLibrary(server);FreeLibrary(client);
    std::puts("PASS native Grunt studio event routing and server event handoff");
}
