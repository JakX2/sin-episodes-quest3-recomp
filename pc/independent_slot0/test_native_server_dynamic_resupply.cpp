#include "native_server_dynamic_resupply.h"
#include "native_client_vgui_button.h"
#include "native_server_npc_active_weapon_step.h"
#include "server_native_cvar_registration.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <limits>
#include <cstring>
using namespace sin_port::native_resupply;
struct Fixture {
    bool owned[18]{};int ammo[18]{};int maxima[18]{};
    unsigned ownership_reads{},reserve_reads{},scale_reads{},max_reads{};
    float scale=1.f;bool fail_scale=false;
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto get=reinterpret_cast<GetApi>(GetProcAddress(s,export_name));
    assert(get&&!GetProcAddress(c,export_name)&&!get(0,sizeof(Api))&&!get(1,0));
    const auto* api=get(1,sizeof(Api));assert(api);
    Fixture f{};Bindings b{};b.context=&f;
    b.has_weapon=[](void* p,int i,bool* yes) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(i>0&&i<18);*yes=f.owned[i];++f.ownership_reads;return 1;};
    b.reserve=[](void* p,int i,int* reserve) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(i>0&&i<18);*reserve=f.ammo[i];++f.reserve_reads;return 1;};
    b.scale=[](void* p,float* scale) noexcept ->int{auto& f=*static_cast<Fixture*>(p);*scale=f.scale;++f.scale_reads;return !f.fail_scale;};
    float desired[]{.5f,.5f,.5f,.5f,.5f,.5f};AmmoRow rows[count]{};Result o{};
    assert(api->ammo_ratios(desired,count,rows,count,&b,&o)&&o.rows==6&&o.index_native==6&&!o.max_native&&!o.owned&&o.scale_reads==6);
    for(auto& row:rows)assert(row.desired==.5f&&row.current==1.f&&row.delta==0.f);
    constexpr int ids[]{1,2,3,6,10,9},capacities[]{80,330,30,3,10,8};
    for(unsigned i=0;i<count;++i){f.owned[ids[i]]=true;f.ammo[ids[i]]=1;f.maxima[ids[i]]=capacities[i];rows[i].nearby=1;}
    f.scale=1.5f;
    assert(api->ammo_ratios(desired,count,rows,count,&b,&o)&&o.rows==6&&o.index_native==6&&o.max_native==12&&o.owned==6&&o.reserve_reads==6);
    for(unsigned i=0;i<count;++i){
        const float current=float(1+ammo_types[i].units)/float(capacities[i]);
        const float delta=.75f-current;
        assert(rows[i].desired==.75f&&rows[i].current==current&&rows[i].delta==(delta<0?0:delta)&&rows[i].nearby==1);
    }
    b.max_carry=[](void* p,int i,int* value) noexcept ->int{auto& f=*static_cast<Fixture*>(p);*value=f.maxima[i];++f.max_reads;return 1;};
    f.maxima[1]=-2;f.maxima[2]=0;f.ammo[2]=0;rows[1].nearby=0;
    f.scale=4.f;
    assert(api->ammo_ratios(desired,count,rows,count,&b,&o)&&o.rows==6&&!o.max_native&&o.reserve_reads==5);
    assert(rows[0].current==-504.5f&&rows[0].delta==1.f&&std::isnan(rows[1].current)&&std::isnan(rows[1].delta));
    f.maxima[1]=80;f.maxima[2]=330;
    // FST of desired must not become the input to subtraction.
    desired[0]=std::nextafter(1.f,2.f);f.scale=std::nextafter(desired[0],2.f);
    f.ammo[1]=80;rows[0].nearby=0;
    assert(api->ammo_ratios(desired,count,rows,count,&b,&o));
    const double product=double(desired[0])*double(f.scale);
    assert(rows[0].desired==float(product)&&rows[0].delta==float(product-1.)&&rows[0].delta!=rows[0].desired-1.f);
    assert(!api->ammo_ratios(desired,count-1,rows,count,&b,&o)&&!o.rows);
    assert(!api->ammo_ratios(desired,count,rows,count-1,&b,&o)&&!o.rows);
    f.fail_scale=true;assert(!api->ammo_ratios(desired,count,rows,count,&b,&o)&&!o.rows);
    // Health uses the live health-vial amount and the same desired scale.
    struct HealthFixture{float vial=10.f,scale=1.f;unsigned calls=0;} hf;
    HealthBindings hb{&hf,
        [](void* p,float* v) noexcept ->int{auto& q=*static_cast<HealthFixture*>(p);assert(q.calls++%2==0);*v=q.vial;return 1;},
        [](void* p,float* v) noexcept ->int{auto& q=*static_cast<HealthFixture*>(p);assert(q.calls++%2==1);*v=q.scale;return 1;}};
    HealthResult hr{};AmmoRow health_row{};health_row.nearby=2;
    assert(api->health_ratio(30,100,.75f,&health_row,&hb,&hr)&&hr.completed&&hr.vial_reads==1&&hr.scale_reads==1);
    assert(health_row.current==.5f&&health_row.desired==.75f&&health_row.delta==.25f&&health_row.nearby==2);
    hf.scale=2.f;assert(api->health_ratio(0,100,1.f,&health_row,&hb,&hr)&&health_row.delta==1.f);
    hf.scale=1.f;assert(api->health_ratio(100,100,.5f,&health_row,&hb,&hr)&&health_row.delta==0.f);
    health_row.nearby=0;assert(api->health_ratio(0,0,.5f,&health_row,&hb,&hr)&&std::isnan(health_row.current)&&std::isnan(health_row.delta));
    int selected=99;AmmoRow deficits[count]{};
    deficits[0].delta=std::numeric_limits<float>::quiet_NaN();deficits[1].delta=-1.f;
    assert(api->greatest_deficit(deficits,count,&selected)&&selected==-1);
    deficits[1].delta=.5f;deficits[4].delta=.5f;assert(api->greatest_deficit(deficits,count,&selected)&&selected==1);
    deficits[5].delta=.75f;assert(api->greatest_deficit(deficits,count,&selected)&&selected==5);
    assert(!api->greatest_deficit(deficits,count+1,&selected)&&selected==-1);
    // Fallback: flag 4, fixed .5 weights, inclusive threshold, NaN skip.
    struct FallbackFixture{bool owned[18]{};float draw=0,hi=0;unsigned draws=0;} ff;
    FallbackBindings fb{};fb.context=&ff;
    fb.has_weapon=[](void* p,int i,bool* yes) noexcept ->int{auto& q=*static_cast<FallbackFixture*>(p);assert(i>0&&i<18);*yes=q.owned[i];return 1;};
    fb.random_float=[](void* p,float lo,float hi,float* v) noexcept ->int{auto& q=*static_cast<FallbackFixture*>(p);assert(lo==0);q.hi=hi;*v=q.draw;++q.draws;return 1;};
    FallbackResult fr{};float wants[count]{.5f,.5f,0.f,.5f,.5f,.5f};
    assert(api->fallback(0,nullptr,0,nullptr,&fr)&&!fr.enabled&&fr.selected==-1&&!ff.draws);
    ff.draw=1.f;assert(api->fallback(4,wants,count,&fb,&fr)&&fr.enabled&&fr.forced_magnum&&fr.selected==0&&fr.total==1.f&&ff.hi==1.f&&!std::strcmp(fr.classname,"item_ammo_magnum"));
    ff.owned[2]=ff.owned[3]=ff.owned[10]=true;
    ff.draw=.5f;assert(api->fallback(4,wants,count,&fb,&fr)&&fr.eligible==2&&fr.index_reads==6&&fr.ownership_reads==6&&fr.cumulative[1]==.5f&&fr.cumulative[2]==-1.f&&fr.cumulative[4]==1.f&&fr.selected==1);
    ff.draw=std::nextafter(.5f,1.f);assert(api->fallback(4,wants,count,&fb,&fr)&&fr.selected==4);
    ff.draw=std::numeric_limits<float>::quiet_NaN();assert(api->fallback(4,wants,count,&fb,&fr)&&fr.random_called&&fr.selected==-1&&!fr.classname);
    // Negative and unordered desired ratios are not exactly zero.
    wants[2]=std::numeric_limits<float>::quiet_NaN();wants[4]=-1.f;ff.draw=1.f;
    assert(api->fallback(4,wants,count,&fb,&fr)&&fr.eligible==3&&fr.total==1.5f&&fr.selected==2);
    // Visible entity enumeration is hosted, counting and debug order are native.
    struct NearbyFixture {
        struct Item{float center[3];const char* name;};
        Item actor{{0,0,0},"item_dynamic_resupply"};
        Item items[7]{
            {{1024,0,0},"ITEM_AMMO_MAGNUM"}, // inclusive boundary
            {{1025,0,0},"item_ammo_ar"},     // excluded by distance
            {{0,5,0},"item_ammo_ar"},
            {{0,0,10},"item_healthvial"},
            {{0,0,0},"item_ammo_flak_extra"}, // no prefix match
            {{0,0,0},nullptr},
            {{0,0,0},"item_ammo_duag"}
        };
        unsigned visits=0,centers=0,classes=0,lines=0,reports=0;
        bool stuck=false;
    } nf;
    NearbyBindings nb{};nb.context=&nf;
    nb.next_visible=[](void* p,void* a,void* prev,void** e) noexcept ->int{
        auto& q=*static_cast<NearbyFixture*>(p);assert(a==&q.actor);++q.visits;
        if(q.stuck&&prev){*e=prev;return 1;}
        unsigned i=0;if(prev){while(i<7&&prev!=&q.items[i])++i;assert(i<7);++i;}
        *e=i<7?&q.items[i]:nullptr;return 1;
    };
    nb.center=[](void* p,void* e,float* v) noexcept ->int{
        auto& q=*static_cast<NearbyFixture*>(p);++q.centers;
        std::memcpy(v,static_cast<NearbyFixture::Item*>(e)->center,12);return 1;
    };
    nb.classname=[](void* p,void* e,const char** n) noexcept ->int{
        auto& q=*static_cast<NearbyFixture*>(p);++q.classes;*n=static_cast<NearbyFixture::Item*>(e)->name;return 1;
    };
    nb.debug_line=[](void* p,const float* a,const float*,int r,int g,int b,bool no_depth,float t) noexcept ->int{
        auto& q=*static_cast<NearbyFixture*>(p);assert(a[0]==0&&a[1]==0&&a[2]==0&&r==0&&g==255&&b==0&&no_depth&&t==20.f);++q.lines;return 1;
    };
    nb.debug_count=[](void* p,const char* n,int value) noexcept ->int{
        auto& q=*static_cast<NearbyFixture*>(p);assert(n&&value>=0);++q.reports;return 1;
    };
    NearbyResult nr{};for(auto& row:rows)row={.25f,.5f,.75f,99};
    assert(api->nearby(&nf.actor,false,2,rows,count,&nb,&nr)&&nr.completed&&nr.visited==7&&nr.within==6&&nr.matched==3&&nr.center_reads==20&&nr.class_reads==6&&nr.lines==3&&nr.reports==6);
    assert(nf.visits==8&&nf.centers==20&&nf.classes==6&&nf.lines==3&&nf.reports==6);
    for(unsigned i=0;i<count;++i)assert(rows[i].desired==.25f&&rows[i].current==.5f&&rows[i].delta==.75f&&rows[i].nearby==((i==0||i==1||i==5)?1:0));
    health_row.nearby=99;
    assert(api->nearby(&nf.actor,true,1,&health_row,1,&nb,&nr)&&nr.completed&&nr.matched==1&&nr.reports==1&&!nr.lines&&health_row.nearby==1);
    // Unordered distance passes original FCOMP gate.
    nf.items[1].center[0]=std::numeric_limits<float>::quiet_NaN();
    assert(api->nearby(&nf.actor,false,0,rows,count,&nb,&nr)&&nr.within==7&&nr.matched==4&&rows[1].nearby==2&&!nr.lines&&!nr.reports);
    assert(!api->nearby(&nf.actor,false,0,rows,count-1,&nb,&nr)&&!nr.visited);
    nf.stuck=true;assert(!api->nearby(&nf.actor,false,0,rows,count,&nb,&nr)&&nr.visited==1&&!nr.completed);
    // Complete evaluation through the independently loaded server DLL.
    struct EvalFixture{
        alignas(4) unsigned char actor[0x3b0]{},master[0x3b0]{},player[0x78c]{},weapons[2][0x4ec]{};
        int items[2]{};
        unsigned creations=0,finalizes=0,positions=0,angles=0,owners=0,
            linear=0,angular=0,bounds=0,moves=0,draws=0,removes=0,origin=0,motion=0;
        bool fallback_only=false,null_factory=false;
        static void put(void* p,unsigned off,const void* v,unsigned n){std::memcpy(static_cast<unsigned char*>(p)+off,v,n);}
        static void u(void* p,unsigned off,unsigned v){put(p,off,&v,4);}
        static void f(void* p,unsigned off,float v){put(p,off,&v,4);}
        static unsigned rd(void* p,unsigned off){unsigned v;std::memcpy(&v,static_cast<unsigned char*>(p)+off,4);return v;}
    } ef;
    EvaluateBindings eb{};eb.nearby.context=&ef;
    eb.nearby.next_visible=[](void* p,void* a,void* prev,void** e) noexcept ->int{auto& q=*static_cast<EvalFixture*>(p);assert(a==q.actor&&!prev);*e=nullptr;return 1;};
    eb.nearby.center=[](void*,void*,float*) noexcept ->int{assert(false);return 0;};
    eb.nearby.classname=[](void*,void*,const char**) noexcept ->int{assert(false);return 0;};
    eb.health=hb;hf.scale=1.f;
    f.fail_scale=false;f.scale=1.f;for(auto& v:f.owned)v=false;
    f.owned[1]=true;f.ammo[1]=1;f.maxima[1]=80;eb.ammo=b;
    ff.owned[1]=true;ff.draw=.5f;eb.fallback=fb;
    auto& sp=eb.spawn;sp.context=&ef;
    sp.refresh_origin=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<EvalFixture*>(p);assert(a==q.actor);++q.origin;EvalFixture::u(a,0xb8,EvalFixture::rd(a,0xb8)&~0x800u);return 1;};
    sp.refresh_motion=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<EvalFixture*>(p);assert(a==q.actor);++q.motion;EvalFixture::u(a,0xb8,EvalFixture::rd(a,0xb8)&~0x1000u);return 1;};
    sp.create=[](void* p,const char* name,int index,void** item) noexcept ->int{
        auto& q=*static_cast<EvalFixture*>(p);assert(index==-1&&q.creations<2);
        assert(!std::strcmp(name,q.fallback_only||q.creations?"item_ammo_magnum":"item_healthvial"));
        *item=q.null_factory?nullptr:&q.items[q.creations];++q.creations;return 1;
    };
    sp.position=[](void* p,void* e,const float* v) noexcept ->int{
        auto& q=*static_cast<EvalFixture*>(p);assert(e==&q.items[q.positions]);
        if(q.positions==0)assert(v[0]==10&&v[1]==20&&v[2]==30);
        else assert(v[0]==11&&v[1]==18&&v[2]==46);
        ++q.positions;return 1;
    };
    sp.orientation=[](void* p,void*,const float* v) noexcept ->int{auto& q=*static_cast<EvalFixture*>(p);assert(v[0]==1&&v[1]==2&&v[2]==3);++q.angles;return 1;};
    sp.owner=[](void* p,void*,void* a) noexcept ->int{auto& q=*static_cast<EvalFixture*>(p);assert(a==q.actor);++q.owners;return 1;};
    sp.finalize=[](void* p,void* e) noexcept ->int{auto& q=*static_cast<EvalFixture*>(p);assert((e==nullptr)==q.null_factory);++q.finalizes;return 1;};
    sp.linear=[](void* p,void*,const float* v) noexcept ->int{auto& q=*static_cast<EvalFixture*>(p);assert(v[0]==5&&v[1]==6&&v[2]==7);++q.linear;return 1;};
    sp.angular=[](void* p,void*,const float* v) noexcept ->int{auto& q=*static_cast<EvalFixture*>(p);assert(v[0]==8&&v[1]==9&&v[2]==10);++q.angular;return 1;};
    sp.world_bounds=[](void* p,void*,float* mins,float* maxs) noexcept ->int{auto& q=*static_cast<EvalFixture*>(p);mins[2]=q.bounds?46.f:28.f;maxs[2]=q.bounds?50.f:38.f;++q.bounds;return 1;};
    sp.move_origin=[](void* p,void*,const float* v) noexcept ->int{auto& q=*static_cast<EvalFixture*>(p);assert(v[0]==10&&v[1]==20&&v[2]==32);++q.moves;return 1;};
    sp.random_float=[](void* p,float lo,float hi,float* v) noexcept ->int{auto& q=*static_cast<EvalFixture*>(p);assert(lo==-6&&hi==6);*v=(q.draws++%2)?-2.f:1.f;return 1;};
    sp.remove=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<EvalFixture*>(p);assert(a==q.actor&&!EvalFixture::rd(a,8));++q.removes;return 1;};
    auto seed=[&](){
        ef=EvalFixture{};EvalFixture::u(ef.actor,8,123);EvalFixture::u(ef.actor,0xb8,0x1800);
        for(unsigned i=0;i<3;++i){
            EvalFixture::f(ef.actor,0x2bc+i*4,float(10+10*i));
            EvalFixture::f(ef.actor,0x354+i*4,float(1+i));
            EvalFixture::f(ef.actor,0x250+i*4,float(5+i));EvalFixture::f(ef.actor,0x25c+i*4,float(8+i));
        }
        EvalFixture::f(ef.actor,0x394,.75f);EvalFixture::f(ef.actor,0x398,.5f);
        EvalFixture::u(ef.player,0x98,100);EvalFixture::u(ef.player,0x9c,25);f.ammo[1]=1;
    };
    EvaluateResult er{};seed();
    assert(api->evaluate(ef.actor,sizeof(ef.actor),ef.player,sizeof(ef.player),0,&eb,&er)&&er.completed&&er.selected_health==0&&er.selected_ammo==0&&er.normal_spawns==2&&er.factories==2&&er.finalizes==2&&er.corrections==1&&er.randoms==4&&er.origin_refreshes==1&&er.motion_refreshes==1&&er.think_cleared&&er.removed);
    assert(ef.creations==2&&ef.positions==2&&ef.angles==2&&ef.owners==2&&ef.linear==2&&ef.angular==2&&ef.bounds==2&&ef.moves==1&&ef.removes==1);
    assert(er.next_position[0]==12&&er.next_position[1]==16&&er.next_position[2]==56);
    // Full player fallback has no velocity, bounds, correction or jitter.
    seed();ef.fallback_only=true;EvalFixture::u(ef.actor,0xb4,4);EvalFixture::u(ef.player,0x9c,100);f.ammo[1]=80;
    assert(api->evaluate(ef.actor,sizeof(ef.actor),ef.player,sizeof(ef.player),0,&eb,&er)&&er.completed&&er.selected_health==-1&&er.selected_ammo==-1&&er.fallback_spawns==1&&er.fallback.selected==0&&!er.normal_spawns&&!er.randoms&&!ef.linear&&!ef.bounds&&ef.removes==1);
    // Self master is an early return, without clearing think or enumerating.
    sin_independent::EntityHandleEntry entry{};entry.serial=3;entry.entity=reinterpret_cast<CBaseEntity*>(ef.actor);std::uint32_t handle=0x3000;
    eb.entries=&entry;eb.entry_count=1;eb.master_handle=&handle;
    seed();assert(api->evaluate(ef.actor,sizeof(ef.actor),ef.player,sizeof(ef.player),0,&eb,&er)&&er.master_skipped&&er.completed&&!er.think_cleared&&!ef.creations&&EvalFixture::rd(ef.actor,8)==123);
    // Master profile selected only when flag 1 and live serial resolves.
    entry.entity=reinterpret_cast<CBaseEntity*>(ef.master);eb.extent_context=&ef;
    eb.extent=[](void* p,void* a,unsigned* n) noexcept ->int{auto& q=*static_cast<EvalFixture*>(p);assert(a==q.master);*n=sizeof(q.master);return 1;};
    seed();EvalFixture::u(ef.actor,0xb4,1);
    assert(api->evaluate(ef.actor,sizeof(ef.actor),ef.player,sizeof(ef.player),0,&eb,&er)&&er.master_profile&&er.completed&&!er.factories&&er.removed);
    // Stale master returns to local profile; null normal factory finalizes
    // but fails closed before original's unsafe item dereference.
    seed();EvalFixture::u(ef.actor,0xb4,1);handle=0x4000;ef.null_factory=true;
    assert(!api->evaluate(ef.actor,sizeof(ef.actor),ef.player,sizeof(ef.player),0,&eb,&er)&&!er.master_profile&&er.null_factory==1&&er.factories==1&&er.finalizes==1&&!er.removed&&!er.think_cleared);
    // Original 222a44d0 walks 48 serial-validated handles and compares
    // +44c then +450, both direct getters for authenticated weapon classes.
    seed();EvalFixture::u(ef.player,0,0x224aec3cu);
    for(unsigned i=0;i<48;++i)EvalFixture::u(ef.player,0x6cc+4*i,0xffffffffu);
    sin_independent::EntityHandleEntry inv_entries[2]{};
    for(unsigned i=0;i<2;++i){
        inv_entries[i].serial=7;inv_entries[i].entity=reinterpret_cast<CBaseEntity*>(ef.weapons[i]);
        EvalFixture::u(ef.weapons[i],0,i?0x2247a1e4u:0x2247875cu);
    }
    EvalFixture::u(ef.player,0x6cc,0x8001u); // stale
    EvalFixture::u(ef.player,0x6cc+4*4,0x7000u);
    EvalFixture::u(ef.player,0x6cc+4*47,0x7001u);
    EvalFixture::u(ef.weapons[0],0x4e4,1);EvalFixture::u(ef.weapons[0],0x4e8,6);
    EvalFixture::u(ef.weapons[1],0x4e4,3);EvalFixture::u(ef.weapons[1],0x4e8,9);
    InventoryView iv{ef.player,sizeof(ef.player),inv_entries,2,&ef,
        [](void* p,void* w,unsigned* n) noexcept ->int{
            auto& q=*static_cast<EvalFixture*>(p);assert(w==q.weapons[0]||w==q.weapons[1]);*n=sizeof(q.weapons[0]);return 1;
        }};
    OwnershipResult own{};
    assert(api->owns(&iv,1,&own)&&own.found&&own.slot==4&&own.slots==5&&own.weapons==1&&own.primary_reads==1&&!own.secondary_reads);
    assert(api->owns(&iv,6,&own)&&own.found&&own.slot==4&&own.secondary_reads==1);
    assert(api->owns(&iv,9,&own)&&own.found&&own.slot==47&&own.slots==48&&own.weapons==2&&own.primary_reads==2&&own.secondary_reads==2);
    assert(api->owns(&iv,10,&own)&&!own.found&&own.slot==-1&&own.slots==48&&own.weapons==2);
    EvalFixture::u(ef.weapons[0],0,0xdeadbeefu);
    assert(!api->owns(&iv,1,&own)&&own.weapons==1&&!own.primary_reads);
    EvalFixture::u(ef.weapons[0],0,0x2247875cu);
    // All-in-native player reads within full evaluation, with shared
    // serial registry and original metadata. No ownership/reserve services.
    eb.entries=inv_entries;eb.entry_count=2;eb.master_handle=nullptr;eb.extent_context=&ef;eb.extent=iv.extent;
    eb.native_player_inventory=true;eb.ammo.has_weapon=nullptr;eb.ammo.reserve=nullptr;eb.ammo.max_carry=nullptr;
    eb.fallback.has_weapon=nullptr;
    EvalFixture::u(ef.player,0x650,1);
    assert(api->evaluate(ef.actor,sizeof(ef.actor),ef.player,sizeof(ef.player),0,&eb,&er)&&er.completed&&er.normal_spawns==2&&er.ammo.ownership_native==6&&er.ammo.reserve_native==4&&er.ammo.owned==4&&er.ammo.max_native==8&&er.ammo_rows[0].current==1.f/80.f);
    // Directly exercise native fallback ownership using the same inventory.
    FallbackBindings native_fb=fb;native_fb.has_weapon=nullptr;native_fb.native_inventory=&iv;
    float native_wants[count]{0,0,0,.5f,0,0};ff.draw=.5f;
    assert(api->fallback(4,native_wants,count,&native_fb,&fr)&&fr.ownership_native==6&&fr.eligible==1&&fr.selected==3);
    // Original resupply lifecycle, independent of original engine pointers.
    struct LifeFixture{
        alignas(4) unsigned char actor[0x3b0]{},other[0x3b0]{};
        unsigned base=0,marks=0,precache=0,draws=0,clocks=0,schedules=0,activations=0,handles=0,warnings=0,removes=0;
        float now=.5f,scheduled=0;bool defer=false,existing=false,has_edict=true;
        static unsigned rd(const void* a,unsigned off){unsigned v;std::memcpy(&v,static_cast<const unsigned char*>(a)+off,4);return v;}
        static void wr(void* a,unsigned off,unsigned v){std::memcpy(static_cast<unsigned char*>(a)+off,&v,4);}
    } lf;
    LifecycleBindings lb{};lb.context=&lf;
    lb.base_construct=[](void* p,void* a,int n) noexcept ->int{
        auto& q=*static_cast<LifeFixture*>(p);assert(a==q.actor&&n==0);++q.base;
        LifeFixture::wr(a,0xb4,q.existing?1u:8u);LifeFixture::wr(a,0x1f0,q.defer?1u:0u);
        LifeFixture::wr(a,0x16c,q.has_edict?0x1234u:0u);return 1;
    };
    lb.mark_edict=[](void* p,unsigned token,unsigned bits) noexcept ->int{auto& q=*static_cast<LifeFixture*>(p);assert(token==0x1234u&&bits==1);++q.marks;return 1;};
    lb.precache=[](void* p,const char* name,int flag) noexcept ->int{auto& q=*static_cast<LifeFixture*>(p);unsigned i=q.precache++%7;assert(flag==0&&!std::strcmp(name,i?ammo_types[i-1].classname:health_classname));return 1;};
    lb.random_float=[](void* p,float lo,float hi,float* v) noexcept ->int{auto& q=*static_cast<LifeFixture*>(p);assert(lo==.2f&&hi==.4f);++q.draws;*v=.25f;return 1;};
    lb.now=[](void* p,float* v) noexcept ->int{auto& q=*static_cast<LifeFixture*>(p);++q.clocks;*v=q.now;return 1;};
    lb.schedule=[](void* p,void* a,float when) noexcept ->int{auto& q=*static_cast<LifeFixture*>(p);assert(a==q.actor&&LifeFixture::rd(a,8)==0);++q.schedules;q.scheduled=when;return 1;};
    lb.base_activate=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<LifeFixture*>(p);assert(a==q.actor);++q.activations;return 1;};
    lb.entity_handle=[](void* p,void* a,unsigned* handle) noexcept ->int{auto& q=*static_cast<LifeFixture*>(p);assert(a==q.actor);++q.handles;*handle=0x3000u;return 1;};
    lb.warning=[](void* p,const char* msg) noexcept ->int{auto& q=*static_cast<LifeFixture*>(p);assert(std::strstr(msg,"no item_dynamic_resupply master exists."));++q.warnings;return 1;};
    lb.remove=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<LifeFixture*>(p);assert(a==q.actor);++q.removes;return 1;};
    sin_independent::EntityHandleEntry master_entries[2]{};
    master_entries[0].serial=3;master_entries[0].entity=reinterpret_cast<CBaseEntity*>(lf.actor);
    master_entries[1].serial=4;master_entries[1].entity=reinterpret_cast<CBaseEntity*>(lf.other);
    unsigned master_handle=0xffffffffu;lb.entries=master_entries;lb.entry_count=2;lb.master_handle=&master_handle;
    LifecycleResult lr{};
    assert(api->construct(lf.actor,sizeof(lf.actor),&lb,&lr)&&lr.completed&&lr.base_called&&lr.flag_added&&lr.edict_marked&&lr.defaults_written==7&&LifeFixture::rd(lf.actor,0)==resupply_vtable&&LifeFixture::rd(lf.actor,0xb4)==9);
    for(unsigned i=0;i<7;++i)assert(LifeFixture::rd(lf.actor,0x394+4*i)==0x3f800000u);
    lf.defer=true;assert(api->construct(lf.actor,sizeof(lf.actor),&lb,&lr)&&lr.deferred_dirty&&!lr.edict_marked&&(LifeFixture::rd(lf.actor,0x200)&255)==1);
    lf.existing=true;assert(api->construct(lf.actor,sizeof(lf.actor),&lb,&lr)&&!lr.flag_added&&!lr.deferred_dirty&&!lr.edict_marked);
    lf.existing=false;lf.defer=false;lf.has_edict=false;assert(api->construct(lf.actor,sizeof(lf.actor),&lb,&lr)&&lr.flag_added&&!lr.edict_marked);
    assert(api->spawn(lf.actor,sizeof(lf.actor),&lb,&lr)&&lr.completed&&lr.precached==7&&lr.random_called&&lr.scheduled&&lr.think_installed&&lr.next_time==.75f&&LifeFixture::rd(lf.actor,8)==check_pvs_think);
    LifeFixture::wr(lf.actor,0xb4,3);LifeFixture::wr(lf.actor,8,0xabcdefu);
    assert(api->spawn(lf.actor,sizeof(lf.actor),&lb,&lr)&&lr.manual&&lr.precached==7&&!lr.random_called&&!lr.scheduled&&LifeFixture::rd(lf.actor,8)==0xabcdefu);
    assert(api->activate(lf.actor,sizeof(lf.actor),&lb,&lr)&&lr.master_assigned&&master_handle==0x3000u&&!lr.warning_called);
    master_handle=0x4001u;assert(api->activate(lf.actor,sizeof(lf.actor),&lb,&lr)&&lr.master_retained&&!lr.master_assigned&&!lr.warning_called);
    master_handle=0x5001u;lf.now=1.f;assert(api->activate(lf.actor,sizeof(lf.actor),&lb,&lr)&&lr.master_assigned&&!lr.warning_called);
    LifeFixture::wr(lf.actor,0xb4,1);master_handle=0xffffffffu;lf.now=.5f;
    assert(api->activate(lf.actor,sizeof(lf.actor),&lb,&lr)&&!lr.master_assigned&&lr.warning_called);
    lf.now=1.f;assert(api->activate(lf.actor,sizeof(lf.actor),&lb,&lr)&&!lr.warning_called);
    master_handle=0x4001u;
    assert(api->become_master(lf.actor,sizeof(lf.actor),&lb,&lr)&&lr.master_assigned&&lr.think_cleared&&master_handle==0x3000u&&!LifeFixture::rd(lf.actor,8));
    LifeFixture::wr(lf.actor,8,123);
    assert(api->kill(lf.actor,sizeof(lf.actor),&lb,&lr)&&lr.removed&&!lr.think_cleared&&LifeFixture::rd(lf.actor,8)==123&&master_handle==0x3000u);
    assert(!api->spawn(lf.actor,sizeof(lf.actor)-1,&lb,&lr)&&!lr.completed);
    // Scheduled and explicit input entry points execute the same evaluator.
    struct DispatchFixture{
        EvalFixture* e;unsigned char edict[0x10]{};
        bool present=false,base_present=true,fail_pvs=false;
        unsigned bytes=0x10,pvs=0,clients_reads=0,slots=0,unknowns=0,extents=0,schedules=0;
        int clients=1;float now=10.f,scheduled=0.f;
    } df{&ef};
    DispatchBindings db{};db.context=&df;db.evaluation=&eb;
    db.pvs_edict=[](void* p,unsigned token,const void** v,unsigned* n) noexcept ->int{
        auto& q=*static_cast<DispatchFixture*>(p);assert(token==0x4321u);++q.pvs;
        *v=q.present?q.edict:nullptr;*n=q.bytes;return !q.fail_pvs;
    };
    db.max_clients=[](void* p,int* n) noexcept ->int{auto& q=*static_cast<DispatchFixture*>(p);*n=q.clients;++q.clients_reads;return 1;};
    db.edict=[](void* p,int i,const void** v,unsigned* n) noexcept ->int{auto& q=*static_cast<DispatchFixture*>(p);assert(i==1);++q.slots;*v=q.present?q.edict:nullptr;*n=q.bytes;return 1;};
    db.base_entity=[](void* p,unsigned token,void** v) noexcept ->int{auto& q=*static_cast<DispatchFixture*>(p);assert(token==0x1234u);++q.unknowns;*v=q.base_present?q.e->player:nullptr;return 1;};
    db.extent=[](void* p,void* e,unsigned* n) noexcept ->int{auto& q=*static_cast<DispatchFixture*>(p);assert(e==q.e->player);++q.extents;*n=sizeof(q.e->player);return 1;};
    db.now=[](void* p,float* t) noexcept ->int{*t=static_cast<DispatchFixture*>(p)->now;return 1;};
    db.schedule=[](void* p,void* a,float t) noexcept ->int{auto& q=*static_cast<DispatchFixture*>(p);assert(a==q.e->actor&&EvalFixture::rd(a,8)==check_pvs_think);++q.schedules;q.scheduled=t;return 1;};
    eb.entries=nullptr;eb.entry_count=0;eb.master_handle=nullptr;eb.native_player_inventory=false;eb.ammo=b;eb.fallback=fb;
    auto dispatch_seed=[&](){seed();EvalFixture::u(ef.actor,0,resupply_vtable);EvalFixture::u(ef.actor,0x16c,0x4321u);EvalFixture::u(ef.actor,8,check_pvs_think);};
    DispatchResult dr{};dispatch_seed();
    assert(api->check_pvs(ef.actor,sizeof(ef.actor),0,&db,&dr)&&dr.completed&&dr.retry&&dr.next_time==11.f&&!dr.evaluated&&!ef.creations);
    df.present=true;assert(api->check_pvs(ef.actor,sizeof(ef.actor),0,&db,&dr)&&dr.edict_found&&dr.retry&&!dr.unknown_called);
    EvalFixture::u(df.edict,0xc,0x1234);df.base_present=false;
    assert(api->check_pvs(ef.actor,sizeof(ef.actor),0,&db,&dr)&&dr.unknown_called&&!dr.player_found&&dr.retry);
    df.base_present=true;df.bytes=0xc;
    assert(!api->check_pvs(ef.actor,sizeof(ef.actor),0,&db,&dr)&&dr.edict_found&&!dr.retry&&!dr.evaluated);
    df.bytes=0x10;df.fail_pvs=true;assert(!api->check_pvs(ef.actor,sizeof(ef.actor),0,&db,&dr)&&!dr.retry&&!dr.evaluated);
    df.fail_pvs=false;
    assert(api->check_pvs(ef.actor,sizeof(ef.actor),0,&db,&dr)&&dr.completed&&dr.player_found&&dr.extent_read&&dr.evaluated&&dr.evaluation.normal_spawns==2&&dr.evaluation.removed&&!dr.retry&&df.schedules==3);
    dispatch_seed();df.clients=2;
    assert(!api->calculate_type(ef.actor,sizeof(ef.actor),0,&db,&dr)&&dr.local_called&&!dr.slot_checked&&dr.evaluated&&!dr.retry&&!ef.creations);
    df.clients=1;EvalFixture::u(df.edict,0,2);
    assert(!api->calculate_type(ef.actor,sizeof(ef.actor),0,&db,&dr)&&dr.slot_checked&&dr.edict_free&&!dr.unknown_called&&!dr.retry);
    EvalFixture::u(df.edict,0,0);
    assert(api->calculate_type(ef.actor,sizeof(ef.actor),0,&db,&dr)&&dr.completed&&dr.local_called&&dr.slot_checked&&dr.evaluation.normal_spawns==2&&dr.evaluation.removed&&!dr.pvs_called&&!dr.retry);
    // Self-master exits before touching a missing player.
    dispatch_seed();entry.entity=reinterpret_cast<CBaseEntity*>(ef.actor);handle=0x3000;
    eb.entries=&entry;eb.entry_count=1;eb.master_handle=&handle;df.clients=0;
    assert(api->calculate_type(ef.actor,sizeof(ef.actor),0,&db,&dr)&&dr.completed&&dr.evaluation.master_skipped&&!dr.player_found&&!dr.extent_read&&!ef.creations&&EvalFixture::rd(ef.actor,8)==check_pvs_think);
    // Original campaign output action -> native queue -> targetname ->
    // resupply datamap -> CalculateType -> native evaluation.
    using namespace sin_port::native_weapon_tables;
    const auto get_input=reinterpret_cast<GetServerEntityAcceptInputApi>(GetProcAddress(s,server_entity_accept_input_export));
    const auto get_queue=reinterpret_cast<GetServerEntityNamedEventQueueApi>(GetProcAddress(s,server_entity_named_event_queue_export));
    const auto get_service=reinterpret_cast<GetServerEntityEventServiceApi>(GetProcAddress(s,server_entity_event_service_export));
    const auto get_output=reinterpret_cast<GetServerEntityVoidOutputApi>(GetProcAddress(s,server_entity_void_output_export));
    assert(get_input&&get_queue&&get_service&&get_output);
    const auto* input_api=get_input(1,sizeof(ServerEntityAcceptInputApi));
    const auto* queue_api=get_queue(1,sizeof(ServerEntityNamedEventQueueApi));
    const auto* service_api=get_service(1,sizeof(ServerEntityEventServiceApi));
    const auto* output_api=get_output(1,sizeof(ServerEntityVoidOutputApi));
    assert(input_api&&queue_api&&service_api&&output_api);
    struct CampaignFixture{
        EvalFixture* e;
        const ServerEntityNamedEventQueueApi* queue;
        const ServerEntityNamedEventQueueBindings* bindings;
        alignas(4) unsigned char root[0x38]{},event[0x38]{},action[0x1c]{},output[0x18]{},edict[4]{},parent[0x10]{};
        unsigned allocated=0,released=0,actions=0,locks=0;
        float now=20.f;
    } cf{&ef,queue_api,nullptr};
    auto store=[](void* p,unsigned token,void** a,unsigned* n) noexcept ->int{
        auto& q=*static_cast<CampaignFixture*>(p);
        switch(token){case 0x7200:*a=q.event;*n=sizeof(q.event);return q.allocated!=0;
        case 0x7100:*a=q.action;*n=sizeof(q.action);return 1;
        case 0x4321:*a=q.edict;*n=sizeof(q.edict);return 1;
        case 0x224f0350:*a=q.parent;*n=sizeof(q.parent);return 1;default:return 0;}
    };
    auto strings=[](void*,unsigned token,const char** str,unsigned* n) noexcept ->int{
        switch(token){case 501:*str="supply_trigger";break;case 601:*str="cAlCuLaTeTyPe";break;
        case 602:*str="bEcOmEmAsTeR";break;case 603:*str="kIlL";break;case 604:*str="unrecognized";break;
        default:return 0;}*n=unsigned(std::strlen(*str)+1);return 1;
    };
    ServerEntityNamedEventQueueBindings qb{};qb.context=&cf;qb.resolve=store;
    qb.allocate=[](void* p,unsigned* token,void** a,unsigned* n) noexcept ->int{
        auto& q=*static_cast<CampaignFixture*>(p);assert(!q.allocated);++q.allocated;*token=0x7200;*a=q.event;*n=sizeof(q.event);return 1;
    };
    qb.string_nonempty=[](void*,unsigned,bool* yes) noexcept ->int{*yes=true;return 1;};cf.bindings=&qb;
    ServerEntityVoidOutputBindings ob{};ob.context=&cf;ob.resolve=store;
    ob.enqueue=[](void* p,void* a,void* caller,const ServerEntityVoidOutputAction* action) noexcept ->int{
        auto& q=*static_cast<CampaignFixture*>(p);assert(a==q.e->actor&&caller==a);
        ServerEntityNamedEventQueueResult result{};
        return q.queue->enqueue(q.root,sizeof(q.root),0x7000u,q.now,0x3000u,0x3000u,action,q.bindings,&result);
    };
    ob.release=[](void* p,unsigned token,void* row) noexcept ->int{auto& q=*static_cast<CampaignFixture*>(p);assert(token==0x7100&&row==q.action);++q.actions;return 1;};
    ServerEntitySearchBindings sb{};sb.context=&cf;sb.head=&entry;sb.string_text=strings;
    sb.extent=[](void* p,void* a,unsigned* n) noexcept ->int{auto& q=*static_cast<CampaignFixture*>(p);assert(a==q.e->actor);*n=sizeof(q.e->actor);return 1;};
    ServerNpcActivePostAnimationBindings storage{};storage.context=&cf;storage.resolve=store;
    ServerEntityResupplyInputBindings rb{};rb.dispatch=&db;rb.dispatch_result=&dr;
    ServerEntityAcceptInputBindings ib{};ib.context=&cf;ib.storage=&storage;ib.string_text=strings;ib.resupply=&rb;
    ServerEntityEventServiceBindings es{};es.context=&cf;es.resolve=store;es.native_search=&sb;es.native_input=&ib;es.live_now=&cf.now;
    es.entities.entries=&entry;es.entities.count=1;
    es.lock=[](void* p,bool enter) noexcept ->int{auto& q=*static_cast<CampaignFixture*>(p);assert(q.locks==unsigned(!enter));q.locks=enter?1:0;return 1;};
    es.release=[](void* p,unsigned token,void* row) noexcept ->int{auto& q=*static_cast<CampaignFixture*>(p);assert(token==0x7200&&row==q.event);++q.released;return 1;};
    dispatch_seed();entry.entity=reinterpret_cast<CBaseEntity*>(ef.actor);entry.next=nullptr;entry.serial=3;
    EvalFixture::u(ef.actor,0x378,0x3000);EvalFixture::u(ef.actor,0xc0,501);
    // Deliberately absent nested registry; outer AcceptInput supplies it.
    eb.entries=nullptr;eb.entry_count=0;eb.master_handle=nullptr;df.clients=1;df.present=true;
    EvalFixture::u(cf.action,0,501);EvalFixture::u(cf.action,4,601);EvalFixture::f(cf.action,0xc,.25f);
    EvalFixture::u(cf.action,0x10,1);EvalFixture::u(cf.action,0x14,92);EvalFixture::u(cf.output,0x14,0x7100);
    ServerEntityVoidOutputResult out_action{};ServerEntityEventServiceResult out_service{};int steps=0;
    assert(output_api->fire(cf.output,sizeof(cf.output),ef.actor,ef.actor,sizeof(ef.actor),0.f,&ob,&out_action)&&out_action.queued==1&&out_action.removed==1&&cf.actions==1);
    assert(service_api->service(cf.root,sizeof(cf.root),0x7000,-99.f,false,&steps,&es,&out_service)&&out_service.future_stopped&&!ef.creations);
    cf.now=20.25f;
    assert(service_api->service(cf.root,sizeof(cf.root),0x7000,-99.f,false,&steps,&es,&out_service)&&out_service.completed&&out_service.events==1&&out_service.named==1&&out_service.native_inputs==1&&!out_service.foreign_inputs&&!out_service.rejected_inputs&&out_service.native_named_searches==2);
    assert(dr.completed&&dr.evaluation.normal_spawns==2&&dr.evaluation.removed&&cf.released==1&&!cf.locks&&!EvalFixture::rd(cf.root,0x30)&&EvalFixture::rd(cf.edict,0)==1);
    // Explicit void input conversion, master binding and inherited unknown.
    LifecycleBindings campaign_life{};campaign_life.context=&ef;campaign_life.master_handle=&handle;
    campaign_life.entity_handle=[](void* p,void* a,unsigned* v) noexcept ->int{auto& q=*static_cast<EvalFixture*>(p);assert(a==q.actor);*v=EvalFixture::rd(a,0x378);return 1;};
    campaign_life.remove=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<EvalFixture*>(p);assert(a==q.actor);++q.removes;return 1;};
    rb.lifecycle=&campaign_life;rb.lifecycle_result=&lr;ib.entities=es.entities;
    dispatch_seed();EvalFixture::u(ef.actor,0x378,0x3000);handle=0xffffffffu;
    unsigned char variant[0x14]{};EvalFixture::u(variant,0x10,2);
    ServerEntityAcceptInputResult input_result{};
    assert(input_api->run(ef.actor,sizeof(ef.actor),resupply_vtable,602,nullptr,nullptr,variant,sizeof(variant),0,&ib,&input_result)&&input_result.accepted&&input_result.resupply_handler&&input_result.converted&&input_result.row==3&&handle==0x3000u&&lr.think_cleared);
    df.clients=0;
    assert(input_api->run(ef.actor,sizeof(ef.actor),resupply_vtable,601,nullptr,nullptr,variant,sizeof(variant),0,&ib,&input_result)&&input_result.accepted&&input_result.row==2&&dr.evaluation.master_skipped&&!dr.player_found);
    assert(input_api->run(ef.actor,sizeof(ef.actor),resupply_vtable,603,nullptr,nullptr,variant,sizeof(variant),0,&ib,&input_result)&&input_result.accepted&&input_result.row==1&&lr.removed&&ef.removes==1);
    assert(input_api->run(ef.actor,sizeof(ef.actor),resupply_vtable,604,nullptr,nullptr,variant,sizeof(variant),0,&ib,&input_result)&&input_result.completed&&!input_result.accepted&&input_result.reason==EntityInputUnknown&&input_result.maps==2);
    // Frame scheduling without the old high-level time scheduling callback.
    const auto get_frame=reinterpret_cast<GetServerResupplyPrimaryThinkApi>(GetProcAddress(s,server_resupply_primary_think_export));
    assert(get_frame&&!GetProcAddress(c,server_resupply_primary_think_export)&&!get_frame(0,sizeof(ServerResupplyPrimaryThinkApi)));
    const auto* frame=get_frame(1,sizeof(ServerResupplyPrimaryThinkApi));assert(frame);
    struct FrameFixture {
        EvalFixture* e;
        float now=10.f,delay=.25f;
        unsigned precached=0,random=0,record=0,enters=0,leaves=0,notices=0;
        bool clear_on_enter=false,unknown_on_enter=false,fail_enter=false;
        int observed_tick=0,observed_due=0;
        float observed_time=0;
    } frf{&ef};
    LifecycleBindings fl{};fl.context=&frf;
    fl.precache=[](void* p,const char*,int flag) noexcept ->int{auto& q=*static_cast<FrameFixture*>(p);assert(flag==0);++q.precached;return 1;};
    fl.now=[](void* p,float* v) noexcept ->int{*v=static_cast<FrameFixture*>(p)->now;return 1;};
    fl.random_float=[](void* p,float lo,float hi,float* v) noexcept ->int{auto& q=*static_cast<FrameFixture*>(p);assert(lo==.2f&&hi==.4f);*v=q.delay;++q.random;return 1;};
    ServerResupplyPrimaryThinkBindings pb{};pb.context=&frf;pb.entities=es.entities;pb.lifecycle=&fl;pb.dispatch=&db;
    pb.cadence.context=&frf;
    pb.cadence.notify_think_flags=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<FrameFixture*>(p);assert(a==q.e->actor);++q.notices;return 1;};
    pb.clock=fl.now;
    pb.record=[](void* p,void* a,int tick,float time,int due) noexcept ->int{auto& q=*static_cast<FrameFixture*>(p);assert(a==q.e->actor);++q.record;q.observed_tick=tick;q.observed_due=due;q.observed_time=time;return 1;};
    pb.scope=[](void* p,void* a,bool enter) noexcept ->int{
        auto& q=*static_cast<FrameFixture*>(p);assert(a==q.e->actor);
        if(enter){++q.enters;assert(EvalFixture::rd(a,0x74)==0xffffffffu);
            if(q.clear_on_enter)EvalFixture::u(a,8,0);
            if(q.unknown_on_enter)EvalFixture::u(a,8,0xdeadbeefu);
            return !q.fail_enter;
        }++q.leaves;return 1;
    };
    ServerResupplyPrimaryThinkResult pr{};
    auto frame_seed=[&](){dispatch_seed();EvalFixture::u(ef.actor,8,0);EvalFixture::u(ef.actor,0x74,0xffffffffu);EvalFixture::u(ef.actor,0xb8,0x400000u);};
    frame_seed();df.present=false;frf.now=10.f;df.now=10.f;
    assert(frame->spawn(ef.actor,sizeof(ef.actor),0x3000,.125f,&pb,&pr)&&pr.completed&&pr.spawn.precached==7&&pr.rescheduled&&pr.reschedule.tick==82&&pr.reschedule.flags_changed&&EvalFixture::rd(ef.actor,8)==check_pvs_think);
    assert(!(EvalFixture::rd(ef.actor,0xb8)&0x400000u)&&frf.notices==1);
    assert(frame->run(ef.actor,sizeof(ef.actor),0x3000,81,.125f,&pb,&pr)&&pr.not_due&&!pr.invoked);
    // At deadline, clear the tick before callback and retry via native ticks.
    frf.now=df.now=10.25f;
    assert(frame->run(ef.actor,sizeof(ef.actor),0x3000,82,.125f,&pb,&pr)&&pr.completed&&pr.invoked&&pr.dispatch.retry&&pr.rescheduled&&pr.reschedule.tick==90&&pr.last_tick==82&&pr.clear.flags_changed&&pr.closed);
    assert(frf.record==1&&frf.observed_due==82&&frf.observed_tick==82&&frf.observed_time==10.25f&&frf.notices==3&&frf.enters==1&&frf.leaves==1);
    // Once visible, use the same evaluator and leave no pending tick.
    df.present=true;df.base_present=true;frf.now=df.now=11.25f;
    assert(frame->run(ef.actor,sizeof(ef.actor),0x3000,90,.125f,&pb,&pr)&&pr.completed&&pr.dispatch.evaluation.normal_spawns==2&&pr.dispatch.evaluation.removed&&!pr.rescheduled&&pr.last_tick==90&&pr.closed);
    assert(EvalFixture::rd(ef.actor,8)==0&&EvalFixture::rd(ef.actor,0x74)==0xffffffffu&&EvalFixture::rd(ef.actor,0x34)==90&&frf.notices==4);
    assert(frame->run(ef.actor,sizeof(ef.actor),0x3000,91,.125f,&pb,&pr)&&pr.no_think&&!pr.invoked);
    assert(!frame->run(ef.actor,sizeof(ef.actor),0x4000,91,.125f,&pb,&pr)&&!pr.completed);
    frame_seed();EvalFixture::u(ef.actor,0xb4,2);
    assert(frame->spawn(ef.actor,sizeof(ef.actor),0x3000,.125f,&pb,&pr)&&pr.spawn.manual&&!pr.rescheduled);
    // A cleared think pointer with a due tick is a retail no-op.
    EvalFixture::u(ef.actor,0xb8,0);EvalFixture::u(ef.actor,0x74,90);
    assert(frame->run(ef.actor,sizeof(ef.actor),0x3000,90,.125f,&pb,&pr)&&pr.completed&&pr.null_token&&!pr.invoked&&pr.closed);
    // Reload identity after engine scope entry, never execute unknown PE VA.
    frame_seed();EvalFixture::u(ef.actor,0xb8,0);EvalFixture::u(ef.actor,0x74,90);EvalFixture::u(ef.actor,8,check_pvs_think);frf.unknown_on_enter=true;
    assert(!frame->run(ef.actor,sizeof(ef.actor),0x3000,90,.125f,&pb,&pr)&&pr.entered&&pr.closed&&!pr.invoked&&pr.token==0xdeadbeefu);
    // VEngineCvar002 const lookup (slot 3), root values rather than aliases.
    struct ResupplyCvars final:sin_independent::console_abi::EngineCvar {
        using Variable=sin_independent::console_abi::Variable;
        using CommandBase=sin_independent::console_abi::CommandBase;
        mutable Variable roots[7],aliases[7];
        mutable unsigned lookups[16]{},calls=0,modifier_reads=0;
        bool missing=false,change_each=false,wrong_root=false;
        ResupplyCvars(){
            for(unsigned i=0;i<7;++i){
                roots[i].CreateVariable(convars[i].name,convars[i].initial,int(convars[i].flags),"",false,0,false,0,nullptr);
                aliases[i].CreateVariable(convars[i].name,"0",int(convars[i].flags),"",false,0,false,0,nullptr);
                aliases[i].parent=&roots[i];
            }
        }
        void __thiscall RegisterConCommandBase(CommandBase*) noexcept override{assert(false);}
        const char* __thiscall GetCommandLineValue(const char*) noexcept override{assert(false);return nullptr;}
        Variable* __thiscall FindMutableVar(const char*) noexcept override{assert(false);return nullptr;}
        const Variable* __thiscall FindConstVar(const char* name) const noexcept override{
            unsigned i=0;while(i<7&&std::strcmp(name,convars[i].name))++i;
            assert(i<7&&calls<16);lookups[calls++]=i;
            if(missing&&i==0)return nullptr;
            if(wrong_root&&i==1)return &aliases[0];
            if(change_each&&i==0)roots[0].SetFloat(.125f*float(++modifier_reads));
            return &aliases[i];
        }
        CommandBase* __thiscall GetCommands() noexcept override{assert(false);return nullptr;}
    } cv;
    HealthBindings live_health{};live_health.engine_cvar=&cv;AmmoRow live_row{};live_row.nearby=2;
    assert(api->health_ratio(20,100,1.f,&live_row,&live_health,&hr)&&live_row.current==.7f&&live_row.desired==1.f&&hr.vial_reads==1&&hr.scale_reads==1);
    assert(cv.calls==2&&cv.lookups[0]==1&&cv.lookups[1]==0);
    cv.roots[1].SetFloat(10.f);cv.roots[0].SetFloat(.5f);cv.calls=0;
    assert(api->health_ratio(20,100,1.f,&live_row,&live_health,&hr)&&live_row.current==.4f&&live_row.desired==.5f);
    cv.wrong_root=true;cv.calls=0;assert(!api->health_ratio(20,100,1.f,&live_row,&live_health,&hr)&&!hr.vial_reads);
    cv.wrong_root=false;cv.calls=0;cv.missing=true;
    assert(!api->health_ratio(20,100,1.f,&live_row,&live_health,&hr)&&hr.vial_reads==1&&!hr.scale_reads);
    cv.missing=false;cv.calls=0;cv.change_each=true;
    auto live_ammo=b;live_ammo.scale=nullptr;live_ammo.engine_cvar=&cv;
    for(auto& row:rows)row={};
    float wants_live[count]{1,1,1,1,1,1};
    assert(api->ammo_ratios(wants_live,count,rows,count,&live_ammo,&o)&&o.rows==6&&o.scale_reads==6&&cv.calls==6);
    for(unsigned i=0;i<count;++i)assert(cv.lookups[i]==0&&rows[i].desired==.125f*float(i+1));
    // Complete evaluation reads debug once, healthkit once and modifier
    // seven times in original order. No hosted high-level scale/vial callbacks.
    cv.change_each=false;cv.calls=0;cv.roots[0].SetFloat(1.f);cv.roots[1].SetFloat(25.f);
    dispatch_seed();eb.master_handle=nullptr;eb.entries=nullptr;eb.entry_count=0;
    eb.engine_cvar=&cv;eb.health=live_health;eb.ammo=live_ammo;
    assert(api->evaluate(ef.actor,sizeof(ef.actor),ef.player,sizeof(ef.player),2,&eb,&er)&&er.completed&&er.normal_spawns==2&&er.debug_native&&er.debug_value==0);
    assert(cv.calls==9&&cv.lookups[0]==2&&cv.lookups[1]==1);for(unsigned i=2;i<9;++i)assert(cv.lookups[i]==0);
    // No cvar lookup or player access if the actor is the current master.
    cv.calls=0;handle=0x3000;eb.master_handle=&handle;eb.entries=&entry;eb.entry_count=1;
    assert(api->evaluate(ef.actor,sizeof(ef.actor),nullptr,0,0,&eb,&er)&&er.master_skipped&&!cv.calls);
    // Original factory allocation -> native constructor -> base initialize.
    struct ResupplyFactoryFixture {
        EvalFixture* e;
        unsigned allocs=0,ctors=0,inits=0,releases=0,bytes=resupply_extent;
        bool null_alloc=false,fail_init=false;
        static unsigned word(void* p,unsigned off){return EvalFixture::rd(p,off);}
    } rf{&ef};
    LifecycleBindings factory_life{};factory_life.context=&rf;
    factory_life.base_construct=[](void* p,void* a,int n) noexcept ->int{
        auto& q=*static_cast<ResupplyFactoryFixture*>(p);assert(a==q.e->actor&&n==0&&q.allocs>q.ctors);
        ++q.ctors;std::memset(a,0,resupply_extent);EvalFixture::u(a,0x74,0xffffffffu);
        EvalFixture::u(a,0xb8,0x400000u);return 1;
    };
    FactoryBindings fac{};fac.context=&rf;fac.lifecycle=&factory_life;
    fac.allocate=[](void* p,unsigned n,void** a,unsigned* bytes) noexcept ->int{
        auto& q=*static_cast<ResupplyFactoryFixture*>(p);assert(n==resupply_extent);++q.allocs;
        *a=q.null_alloc?nullptr:q.e->actor;*bytes=q.bytes;return 1;
    };
    fac.initialize=[](void* p,void* a,const char* name) noexcept ->int{
        auto& q=*static_cast<ResupplyFactoryFixture*>(p);
        assert(a==q.e->actor&&!std::strcmp(name,resupply_classname)&&q.ctors>q.inits);
        assert(ResupplyFactoryFixture::word(a,0)==resupply_vtable&&ResupplyFactoryFixture::word(a,0xb4)==1);
        for(unsigned i=0;i<7;++i)assert(ResupplyFactoryFixture::word(a,0x394+4*i)==0x3f800000u);
        ++q.inits;EvalFixture::u(a,0x378,0x3000);EvalFixture::u(a,0x16c,0x4321);return !q.fail_init;
    };
    fac.release_networkable=[](void* p,void* a) noexcept ->int{
        auto& q=*static_cast<ResupplyFactoryFixture*>(p);assert(a==q.e->actor+resupply_networkable_offset);
        ++q.releases;std::memset(q.e->actor,0,resupply_extent);return 1;
    };
    FactoryResult fact{};
    assert(!api->factory_create("item_healthvial",&fac,&fact)&&!fact.allocation_called);
    rf.null_alloc=true;assert(!api->factory_create(resupply_classname,&fac,&fact)&&fact.allocation_called&&!fact.allocated&&!fact.actor&&!fact.networkable);
    rf.null_alloc=false;rf.bytes=resupply_extent-1;
    assert(!api->factory_create(resupply_classname,&fac,&fact)&&fact.allocated&&!fact.constructed&&!fact.networkable&&fact.actor==ef.actor);
    rf.bytes=resupply_extent;rf.fail_init=true;
    assert(!api->factory_create(resupply_classname,&fac,&fact)&&fact.constructed&&!fact.initialized&&!fact.networkable&&fact.actor==ef.actor);
    rf.fail_init=false;
    assert(api->factory_create(resupply_classname,&fac,&fact)&&fact.completed&&fact.constructed&&fact.initialized&&fact.actor==ef.actor&&fact.networkable==ef.actor+0x160);
    assert(rf.allocs==4&&rf.ctors==2&&rf.inits==2);
    // Factory-created entity enters the existing tick scheduler, not
    // an isolated test of allocation. Manual map spawnflags then activate.
    entry.entity=reinterpret_cast<CBaseEntity*>(ef.actor);
    handle=0xffffffffu;fl.master_handle=&handle;fl.entries=&entry;fl.entry_count=1;
    fl.base_activate=[](void*,void*) noexcept ->int{return 1;};
    EvalFixture::u(ef.actor,0xb4,3);
    assert(frame->spawn(ef.actor,sizeof(ef.actor),0x3000,.125f,&pb,&pr)&&pr.spawn.manual&&!pr.rescheduled);
    assert(api->activate(ef.actor,sizeof(ef.actor),&fl,&lr)&&lr.master_assigned&&lr.native_handle&&handle==0x3000u);
    EvalFixture::u(ef.actor,8,check_pvs_think);
    assert(api->become_master(ef.actor,sizeof(ef.actor),&fl,&lr)&&lr.native_handle&&lr.think_cleared&&handle==0x3000u);
    assert(api->factory_destroy(nullptr,0,nullptr,nullptr,&fact)&&fact.completed&&!fact.released);
    assert(!api->factory_destroy(ef.actor,sizeof(ef.actor),ef.actor+0x15c,&fac,&fact)&&!rf.releases);
    assert(api->factory_destroy(ef.actor,sizeof(ef.actor),ef.actor+0x160,&fac,&fact)&&fact.completed&&fact.released&&rf.releases==1&&!EvalFixture::rd(ef.actor,0));
    // Six real pickup constructors and common MyTouch, with rule scaling.
    struct PickupFixture {
        alignas(4) unsigned char item[0x4d8]{};
        int player=0,index=0,requested=0,granted=1,rule=6;
        float multiplier=.75f;
        unsigned ctors=0,scales=0,gives=0,rules=0,removes=0,warnings=0;
    } pf;
    AmmoPickupBindings ap{};ap.context=&pf;
    ap.base_construct=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<PickupFixture*>(p);assert(a==q.item);++q.ctors;return 1;};
    ap.multiplier=[](void* p,int i,float* v) noexcept ->int{auto& q=*static_cast<PickupFixture*>(p);q.index=i;*v=q.multiplier;++q.scales;return 1;};
    ap.give=[](void* p,void* player,int n,int i,bool silent,int* granted) noexcept ->int{
        auto& q=*static_cast<PickupFixture*>(p);assert(player==&q.player&&i==q.index&&!silent);q.requested=n;*granted=q.granted;++q.gives;return 1;
    };
    ap.respawn_rule=[](void* p,void* a,int* v) noexcept ->int{auto& q=*static_cast<PickupFixture*>(p);assert(a==q.item);*v=q.rule;++q.rules;return 1;};
    ap.remove=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<PickupFixture*>(p);assert(a==q.item);++q.removes;return 1;};
    ap.unknown_ammo=[](void* p,const char* n) noexcept ->int{auto& q=*static_cast<PickupFixture*>(p);assert(!std::strcmp(n,"MAG"));++q.warnings;return 1;};
    AmmoPickupResult ar{};constexpr int actual[]{10,30,10,1,1,1},scaled[]{7,22,7,1,1,1};
    for(unsigned i=0;i<count;++i){
        assert(api->ammo_pickup_construct(pf.item,sizeof(pf.item),i,&ap,&ar)&&ar.completed&&ar.constructed&&ar.units==actual[i]);
        assert(EvalFixture::rd(pf.item,0)==ammo_pickups[i].vtable&&EvalFixture::rd(pf.item,0x494)==ammo_pickups[i].secondary&&EvalFixture::rd(pf.item,0x4cc)==ammo_pickups[i].model_token&&EvalFixture::rd(pf.item,0x4d0)==ammo_pickups[i].ammo_token&&EvalFixture::rd(pf.item,0x4d4)==unsigned(actual[i]));
        assert(api->ammo_pickup_touch(pf.item,sizeof(pf.item),&pf.player,&ap,&ar)&&ar.completed&&ar.accepted&&ar.index==ids[i]&&ar.requested==scaled[i]&&ar.granted==1&&ar.rule==6&&ar.removed);
    }
    assert(ammo_types[5].units==5&&ammo_pickups[5].units==1&&pf.ctors==6&&pf.gives==6&&pf.removes==6);
    assert(api->ammo_pickup_construct(pf.item,sizeof(pf.item),0,&ap,&ar));
    pf.multiplier=-2.f;pf.rule=5;
    assert(api->ammo_pickup_touch(pf.item,sizeof(pf.item),&pf.player,&ap,&ar)&&ar.requested==1&&ar.accepted&&!ar.removed);
    pf.granted=0;
    assert(api->ammo_pickup_touch(pf.item,sizeof(pf.item),&pf.player,&ap,&ar)&&ar.completed&&!ar.accepted&&!ar.rule_called);
    pf.multiplier=std::numeric_limits<float>::quiet_NaN();
    assert(api->ammo_pickup_touch(pf.item,sizeof(pf.item),&pf.player,&ap,&ar)&&ar.requested==std::numeric_limits<int>::min());
    ap.index=[](void*,const char*,int* i) noexcept ->int{*i=-1;return 1;};
    assert(api->ammo_pickup_touch(pf.item,sizeof(pf.item),&pf.player,&ap,&ar)&&ar.completed&&ar.unknown&&!ar.give_called&&pf.warnings==1);
    EvalFixture::u(pf.item,0x4d0,0xdeadbeef);
    assert(!api->ammo_pickup_touch(pf.item,sizeof(pf.item),&pf.player,&ap,&ar)&&!ar.index_read);
    // Actual vial reads sk_healthvial, unlike nearby healthkit estimate.
    struct VialFixture {
        alignas(4) unsigned char item[0x4cc]{};
        int player=0,healed=1,rule=0;float value=0;
        unsigned stage=0,ctors=0,token=0,respawns=0,removes=0;
    } vf;
    VialPickupBindings vb{};vb.context=&vf;vb.engine_cvar=&cv;
    vb.base_construct=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<VialFixture*>(p);assert(a==q.item);++q.ctors;return 1;};
    vb.heal=[](void* p,void* a,float amount,unsigned flags,int* result) noexcept ->int{
        auto& q=*static_cast<VialFixture*>(p);assert(a==&q.player&&!flags&&q.stage++==0);q.value=amount;*result=q.healed;return 1;
    };
    vb.item_pickup=[](void* p,void* a,unsigned token) noexcept ->int{
        auto& q=*static_cast<VialFixture*>(p);assert(a==&q.player&&q.stage++==1);q.token=token;return 1;
    };
    vb.sound=[](void* p,void* a,unsigned edict,const char* s) noexcept ->int{
        auto& q=*static_cast<VialFixture*>(p);assert(a==&q.player&&edict==0x4321&&q.stage++==2&&!std::strcmp(s,"HealthVial.Touch"));return 1;
    };
    vb.respawn_rule=[](void* p,void* a,int* rule) noexcept ->int{auto& q=*static_cast<VialFixture*>(p);assert(a==q.item&&q.stage++==3);*rule=q.rule;return 1;};
    vb.respawn=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<VialFixture*>(p);assert(a==q.item&&q.stage++==4);++q.respawns;return 1;};
    vb.remove=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<VialFixture*>(p);assert(a==q.item&&q.stage++==4);++q.removes;return 1;};
    VialPickupResult vr{};
    assert(api->vial_construct(vf.item,sizeof(vf.item),&vb,&vr)&&vr.constructed&&EvalFixture::rd(vf.item,0)==vial_vtable&&EvalFixture::rd(vf.item,0x494)==vial_secondary);
    EvalFixture::u(vf.item,0x16c,0x4321);EvalFixture::u(vf.item,0xc,0x22459e28);
    cv.calls=0;
    assert(api->vial_touch(vf.item,sizeof(vf.item),&vf.player,&vb,&vr)&&vr.completed&&vr.accepted&&vr.removed&&vr.message&&vr.sound&&vr.value==10.f&&vf.stage==5&&vf.token==0x22459e28);
    assert(cv.calls==1&&cv.lookups[0]==3&&vf.removes==1);
    vf.stage=0;vf.rule=6;cv.roots[3].SetFloat(12.5f);cv.calls=0;EvalFixture::u(vf.item,0xc,0);
    assert(api->vial_touch(vf.item,sizeof(vf.item),&vf.player,&vb,&vr)&&vr.respawned&&!vr.removed&&vr.value==12.5f&&vf.token==0&&vf.respawns==1);
    vf.stage=0;vf.healed=0;cv.calls=0;
    assert(api->vial_touch(vf.item,sizeof(vf.item),&vf.player,&vb,&vr)&&vr.completed&&!vr.accepted&&!vr.message&&!vr.sound&&!vr.rule_called&&vf.stage==1);
    vf.stage=0;cv.aliases[3].parent=&cv.roots[1];cv.calls=0;
    assert(!api->vial_touch(vf.item,sizeof(vf.item),&vf.player,&vb,&vr)&&!vr.heal_called&&!vf.stage);
    cv.aliases[3].parent=&cv.roots[3];
    // Full default ItemTouch on actual ammo item, including native output.
    struct TouchFixture {
        PickupFixture* ammo;
        alignas(4) unsigned char player[0xbc]{},vehicle[4]{},other[4]{},action[0x1c]{};
        unsigned permission=0,notices=0,rules=0,respawns=0,removes=0,outputs=0,releases=0,vehicles=0,passengers=0;
        int rule=5;bool allow=true,force=false,has_passenger=true;
    } tf{&pf};
    ItemTouchBindings tb{};tb.context=&tf;
    tb.can_pickup=[](void* p,void* pl,void* a,bool* yes) noexcept ->int{auto& q=*static_cast<TouchFixture*>(p);assert(pl==q.player&&a==q.ammo->item);++q.permission;*yes=q.allow;return 1;};
    tb.picked_up=[](void* p,void* pl,void* a) noexcept ->int{auto& q=*static_cast<TouchFixture*>(p);assert(pl==q.player&&a==q.ammo->item&&!EvalFixture::rd(a,8)&&!EvalFixture::rd(a,0x7c));++q.notices;return 1;};
    tb.respawn_rule=[](void* p,void*,int* rule) noexcept ->int{auto& q=*static_cast<TouchFixture*>(p);++q.rules;*rule=q.rule;return 1;};
    tb.respawn=[](void* p,void*) noexcept ->int{++static_cast<TouchFixture*>(p)->respawns;return 1;};
    tb.remove=[](void* p,void*) noexcept ->int{++static_cast<TouchFixture*>(p)->removes;return 1;};
    tb.force_failed_pickup=[](void* p,bool* yes) noexcept ->int{*yes=static_cast<TouchFixture*>(p)->force;return 1;};
    tb.vehicle=[](void* p,void* a,void** v) noexcept ->int{auto& q=*static_cast<TouchFixture*>(p);assert(a==q.other);++q.vehicles;*v=q.vehicle;return 1;};
    tb.passenger=[](void* p,void* v,int seat,void** pl) noexcept ->int{auto& q=*static_cast<TouchFixture*>(p);assert(v==q.vehicle&&seat==0);++q.passengers;*pl=q.has_passenger?q.player:nullptr;return 1;};
    tb.extent=[](void* p,void* pl,unsigned* n) noexcept ->int{auto& q=*static_cast<TouchFixture*>(p);assert(pl==q.player);*n=sizeof(q.player);return 1;};
    auto touch_ammo=ap;touch_ammo.context=&tf;touch_ammo.index=nullptr;
    touch_ammo.multiplier=[](void*,int,float* x) noexcept ->int{*x=1.f;return 1;};
    touch_ammo.give=[](void* p,void* pl,int n,int i,bool silent,int* result) noexcept ->int{auto& q=*static_cast<TouchFixture*>(p);assert(pl==q.player&&n==10&&i==1&&!silent);*result=q.ammo->granted;return 1;};
    touch_ammo.respawn_rule=[](void* p,void*,int* rule) noexcept ->int{*rule=static_cast<TouchFixture*>(p)->ammo->rule;return 1;};
    touch_ammo.remove=tb.remove;tb.ammo=&touch_ammo;
    ServerEntityVoidOutputBindings touch_output{};touch_output.context=&tf;
    touch_output.resolve=[](void* p,unsigned token,void** a,unsigned* n) noexcept ->int{auto& q=*static_cast<TouchFixture*>(p);assert(token==0x7777);*a=q.action;*n=sizeof(q.action);return 1;};
    touch_output.enqueue=[](void* p,void* pl,void* a,const ServerEntityVoidOutputAction* v) noexcept ->int{auto& q=*static_cast<TouchFixture*>(p);assert(pl==q.player&&a==q.ammo->item&&v->target==501&&v->input==601&&v->delay==.25f&&EvalFixture::rd(a,0x7c)==item_touch_token);++q.outputs;return 1;};
    touch_output.release=[](void* p,unsigned token,void* a) noexcept ->int{auto& q=*static_cast<TouchFixture*>(p);assert(token==0x7777&&a==q.action);++q.releases;return 1;};tb.output=&touch_output;
    assert(api->ammo_pickup_construct(pf.item,sizeof(pf.item),0,&ap,&ar));
    auto touch_seed=[&](){EvalFixture::u(pf.item,8,123);EvalFixture::u(pf.item,0x7c,item_touch_token);};
    touch_seed();EvalFixture::u(tf.player,0,0x224aec3c);pf.granted=10;pf.rule=5;
    EvalFixture::u(tf.action,0,501);EvalFixture::u(tf.action,4,601);EvalFixture::f(tf.action,0xc,.25f);EvalFixture::u(tf.action,0x10,1);
    EvalFixture::u(pf.item,0x49c+0x14,0x7777);
    ItemTouchResult tr{};
    assert(api->item_touch(pf.item,sizeof(pf.item),tf.player,sizeof(tf.player),&tb,&tr)&&tr.completed&&tr.player_native&&tr.accepted&&tr.output_actions==1&&tr.touch_cleared&&tr.think_cleared&&tr.picked_up&&tr.respawned&&!tr.removed);
    assert(tf.outputs==1&&tf.releases==1&&tf.notices==1&&tf.respawns==1&&!EvalFixture::rd(pf.item,0x4b0));
    touch_seed();tf.rule=6;pf.rule=6;
    assert(api->item_touch(pf.item,sizeof(pf.item),tf.other,sizeof(tf.other),&tb,&tr)&&tr.completed&&tr.vehicle_reads==2&&tr.passenger_read&&tr.ammo.removed&&tr.removed&&tf.removes==2);
    touch_seed();tf.has_passenger=false;
    assert(api->item_touch(pf.item,sizeof(pf.item),tf.other,sizeof(tf.other),&tb,&tr)&&tr.completed&&tr.passenger_read&&!tr.subclass_called);
    tf.has_passenger=true;EvalFixture::u(tf.player,0xb8,0x20000000);
    assert(api->item_touch(pf.item,sizeof(pf.item),tf.player,sizeof(tf.player),&tb,&tr)&&tr.blocked&&!tr.permission_read);
    EvalFixture::u(tf.player,0xb8,0);tf.allow=false;
    assert(api->item_touch(pf.item,sizeof(pf.item),tf.player,sizeof(tf.player),&tb,&tr)&&tr.permission_read&&!tr.subclass_called);
    tf.allow=true;pf.granted=0;
    assert(api->item_touch(pf.item,sizeof(pf.item),tf.player,sizeof(tf.player),&tb,&tr)&&tr.completed&&tr.force_read&&!tr.accepted&&!tr.removed&&EvalFixture::rd(pf.item,0x7c)==item_touch_token);
    tf.force=true;
    assert(api->item_touch(pf.item,sizeof(pf.item),tf.player,sizeof(tf.player),&tb,&tr)&&tr.completed&&tr.removed&&!tr.touch_cleared&&!tr.output_called);
    // Direct reserve transfer in CSE1Player, without hosted grant or
    // rule scaling. No collision or rendering callbacks involved.
    struct GrantFixture {
        alignas(4) unsigned char player[0x790]{},weapon[0x4ec]{};
        unsigned sounds=0,denied=0,auto_reads=0,tests=0,choices=0,switches=0,marks=0;
        bool auto_on=false,clip=false;
        char sound[64]{};
    } gf;
    sin_independent::EntityHandleEntry ge{};ge.serial=8;ge.entity=reinterpret_cast<CBaseEntity*>(gf.weapon);
    PlayerAmmoBindings gb{};gb.context=&gf;gb.inventory={gf.player,sizeof(gf.player),&ge,1,&gf,
        [](void* p,void* w,unsigned* n) noexcept ->int{auto& q=*static_cast<GrantFixture*>(p);assert(w==q.weapon);*n=sizeof(q.weapon);return 1;}};
    gb.sound=[](void* p,void* a,const char* n) noexcept ->int{auto& q=*static_cast<GrantFixture*>(p);assert(a==q.player);++q.sounds;const auto len=std::strlen(n)+1;assert(len<=sizeof(q.sound));std::memcpy(q.sound,n,len);return 1;};
    gb.denied_message=[](void* p,void* a,int i) noexcept ->int{auto& q=*static_cast<GrantFixture*>(p);assert(a==q.player&&i==1);++q.denied;return 1;};
    gb.auto_switch_enabled=[](void* p,bool* yes) noexcept ->int{auto& q=*static_cast<GrantFixture*>(p);++q.auto_reads;*yes=q.auto_on;return 1;};
    gb.weapon_test=[](void* p,void* w,unsigned off,bool* yes) noexcept ->int{auto& q=*static_cast<GrantFixture*>(p);assert(w==q.weapon);++q.tests;*yes=off==0x438||(off==0x330&&q.clip);return 1;};
    gb.next_best=[](void* p,void* a,void* current,void** best) noexcept ->int{auto& q=*static_cast<GrantFixture*>(p);assert(a==q.player&&!current);++q.choices;*best=q.weapon;return 1;};
    gb.switch_weapon=[](void* p,void* a,void* best,int n) noexcept ->int{auto& q=*static_cast<GrantFixture*>(p);assert(a==q.player&&best==q.weapon&&!n);++q.switches;return 1;};
    gb.mark_edict=[](void* p,unsigned token,unsigned mask) noexcept ->int{auto& q=*static_cast<GrantFixture*>(p);assert(token==0x9911&&mask==1);++q.marks;return 1;};
    EvalFixture::u(gf.player,0,0x224aec3c);EvalFixture::u(gf.player,0x16c,0x9911);
    for(unsigned i=0;i<48;++i)EvalFixture::u(gf.player,0x6cc+4*i,0xffffffffu);
    EvalFixture::u(gf.player,0x78c,0xffffffffu);
    EvalFixture::u(gf.weapon,0,0x2247875c);EvalFixture::u(gf.weapon,0x4e4,1);
    EvalFixture::u(gf.player,0x650,75);PlayerAmmoResult gr{};
    assert(api->player_give_ammo(gf.player,10,1,false,&gb,&gr)&&gr.completed&&gr.sin_override&&gr.has_ammo&&gr.max_reads==4&&gr.granted==5&&gr.before==75&&gr.after==80&&gr.dirty&&gr.sound&&gf.marks==1);
    assert(!std::strcmp(gf.sound,"BaseCombatCharacter.AmmoPickup"));
    assert(api->player_give_ammo(gf.player,10,1,false,&gb,&gr)&&gr.completed&&!gr.granted&&gr.denied&&gf.denied==1&&!gr.dirty);
    EvalFixture::u(gf.player,0x650,0);EvalFixture::u(gf.player,0x6cc,0x8000);gf.auto_on=true;
    assert(api->player_give_ammo(gf.player,10,1,true,&gb,&gr)&&gr.granted==10&&!gr.has_ammo&&gr.weapon_tests==3&&gr.next_best_calls==2&&gr.switched&&gf.switches==1&&!gr.sound);
    EvalFixture::u(gf.player,0x650,0);gf.clip=true;
    assert(api->player_give_ammo(gf.player,10,1,true,&gb,&gr)&&gr.granted==10&&gr.has_ammo&&gr.weapon_tests==2&&!gr.next_best_calls);
    EvalFixture::u(gf.player,0x1f0,1);gf.auto_on=false;gf.clip=false;
    EvalFixture::u(gf.player,0x664,1);
    assert(api->player_give_ammo(gf.player,1,6,false,&gb,&gr)&&gr.granted==1&&gr.sound&&gr.dirty&&!std::strcmp(gf.sound,"Player.PickupSecondary")&&(EvalFixture::rd(gf.player,0x200)&255)==1);
    // Whole ItemTouch uses the live metric2 and direct player grant;
    // native rules: allow, no-op notice, rule6 in both branches.
    float metric2=.5f;
    assert(api->ammo_pickup_construct(pf.item,sizeof(pf.item),0,&ap,&ar));
    touch_seed();EvalFixture::u(gf.player,0x650,77);
    AmmoPickupBindings native_ap{};native_ap.context=&tf;native_ap.live_metric2=&metric2;native_ap.native_singleplayer_rules=true;native_ap.native_grant=&gb;native_ap.remove=tb.remove;
    ItemTouchBindings native_tb{};native_tb.context=&tf;native_tb.native_singleplayer_rules=true;native_tb.ammo=&native_ap;native_tb.remove=tb.remove;
    assert(api->item_touch(pf.item,sizeof(pf.item),gf.player,sizeof(gf.player),&native_tb,&tr)&&tr.completed&&tr.accepted&&tr.ammo.requested==5&&tr.ammo.granted==3&&tr.ammo.native_grant.after==80&&tr.ammo.rule==6&&tr.rule==6&&tr.ammo.removed&&tr.removed&&tr.picked_up);
    assert(EvalFixture::rd(gf.player,0x650)==80&&!EvalFixture::rd(pf.item,8)&&!EvalFixture::rd(pf.item,0x7c));
    // Both authenticated player tables share the original heal override.
    struct HealFixture {
        alignas(4) unsigned char player[0x12a4]{};
        unsigned events=0,fades=0;int type=0,hp=0,delta=0;unsigned flags=0;
    } hf2;
    PlayerHealthBindings ph{};ph.context=&hf2;ph.player=hf2.player;ph.bytes=sizeof(hf2.player);
    ph.health_event=[](void* p,void* a,int type,int hp,int delta,unsigned flags) noexcept ->int{
        auto& q=*static_cast<HealFixture*>(p);assert(a==q.player&&EvalFixture::rd(a,0x9c)==unsigned(hp));++q.events;q.type=type;q.hp=hp;q.delta=delta;q.flags=flags;return 1;
    };
    ph.fade=[](void* p,void* a,const unsigned char* rgba,float duration,float hold,int flags) noexcept ->int{
        auto& q=*static_cast<HealFixture*>(p);assert(a==q.player&&rgba[0]==0&&rgba[1]==0&&rgba[2]==128&&rgba[3]==64&&duration==1.f&&hold==.5f&&flags==1);++q.fades;return 1;
    };
    EvalFixture::u(hf2.player,0,0x224aec3c);EvalFixture::u(hf2.player,0x16c,0x4321);EvalFixture::u(hf2.player,0x94,2);
    EvalFixture::u(hf2.player,0x98,100);EvalFixture::u(hf2.player,0x9c,95);EvalFixture::u(hf2.player,0x9d0,0xffffffffu);
    PlayerHealthResult phr{};
    assert(api->player_take_health(hf2.player,10.75f,0,&ph,&phr)&&phr.completed&&phr.accepted&&phr.converted==10&&phr.raw_health==105&&phr.raw_delta==10&&phr.event_type==2&&phr.after==100&&phr.delta==5&&hf2.events==1&&hf2.fades==1);
    assert(!phr.maximum_reset&&EvalFixture::rd(hf2.player,0x9c)==100);
    assert(api->player_take_health(hf2.player,10,0,&ph,&phr)&&phr.completed&&!phr.accepted&&!phr.event_called&&hf2.events==1);
    EvalFixture::u(hf2.player,0x98,150);EvalFixture::u(hf2.player,0x9c,110);
    assert(api->player_take_health(hf2.player,10,0x4002,&ph,&phr)&&phr.completed&&phr.maximum_reset&&!phr.accepted&&phr.maximum_after==100&&EvalFixture::rd(hf2.player,0x9d0)==0xffffbffdu);
    EvalFixture::u(hf2.player,0x98,150);EvalFixture::u(hf2.player,0x12a1,1);
    assert(api->player_take_health(hf2.player,10,0,&ph,&phr)&&phr.accepted&&phr.after==120&&phr.overheal_flag_cleared&&!phr.maximum_reset);
    EvalFixture::u(hf2.player,0x98,100);EvalFixture::u(hf2.player,0x9c,50);
    assert(api->player_take_health(hf2.player,.75f,0,&ph,&phr)&&phr.completed&&!phr.accepted&&phr.event_called&&phr.raw_delta==0&&!phr.fade_called);
    EvalFixture::u(hf2.player,0x94,1);
    assert(api->player_take_health(hf2.player,10,0,&ph,&phr)&&phr.completed&&!phr.event_called&&!phr.accepted);
    EvalFixture::u(hf2.player,0x94,2);EvalFixture::u(hf2.player,0x16c,0);
    assert(api->player_take_health(hf2.player,10,0,&ph,&phr)&&phr.completed&&!phr.event_called);
    EvalFixture::u(hf2.player,0x16c,0x4321);EvalFixture::u(hf2.player,0,0x2245222c);
    assert(api->player_take_health(hf2.player,25,0,&ph,&phr)&&phr.accepted&&phr.after==75&&phr.event_type==3);
    // Actual health vial, no hosted heal function.
    struct NativeVialFixture {HealFixture* h;unsigned messages=0,sounds=0,respawns=0,removes=0;} nvf{&hf2};
    VialPickupBindings nvb{};nvb.context=&nvf;nvb.native_heal=&ph;nvb.engine_cvar=&cv;nvb.native_singleplayer_rules=true;
    nvb.item_pickup=[](void* p,void* a,unsigned token) noexcept ->int{auto& q=*static_cast<NativeVialFixture*>(p);assert(a==q.h->player&&token==0);++q.messages;return 1;};
    nvb.sound=[](void* p,void* a,unsigned,const char* n) noexcept ->int{auto& q=*static_cast<NativeVialFixture*>(p);assert(a==q.h->player&&!std::strcmp(n,"HealthVial.Touch"));++q.sounds;return 1;};
    nvb.respawn=[](void* p,void*) noexcept ->int{++static_cast<NativeVialFixture*>(p)->respawns;return 1;};
    ItemTouchBindings nvt{};nvt.context=&nvf;nvt.native_singleplayer_rules=true;nvt.vial=&nvb;
    nvt.remove=[](void* p,void*) noexcept ->int{++static_cast<NativeVialFixture*>(p)->removes;return 1;};
    cv.roots[3].SetFloat(10.f);cv.calls=0;
    EvalFixture::u(vf.item,8,123);EvalFixture::u(vf.item,0x7c,item_touch_token);
    assert(api->item_touch(vf.item,sizeof(vf.item),hf2.player,sizeof(hf2.player),&nvt,&tr)&&tr.completed&&tr.accepted&&tr.vial.native_heal.after==85&&tr.vial.respawned&&tr.removed&&nvf.messages==1&&nvf.sounds==1&&nvf.respawns==1&&nvf.removes==1);
    // All negative health-event mask precedence, including unusual
    // negative cvar values, is recovered rather than guessed.
    constexpr unsigned dmg[]{2,0x91,4,0x1200008,0x30000,0x100000,0x100,0x20,0x10000040,0x4000,0x400000,0,0x102};
    constexpr int cats[]{8,7,13,9,11,15,10,16,18,17,12,6,8};
    for(unsigned i=0;i<sizeof(dmg)/sizeof(dmg[0]);++i){
        EvalFixture::u(hf2.player,0x9c,50);
        assert(api->player_take_health(hf2.player,-2.75f,dmg[i],&ph,&phr)&&phr.completed&&phr.accepted&&phr.after==48&&phr.raw_delta==-2&&phr.event_type==cats[i]&&phr.fade_called);
    }
    // One continuous evaluation -> actual pickup factories -> player
    // consumption -> reevaluation, with one live player and inventory.
    struct LoopFixture{
        alignas(4) unsigned char pickups[2][0x4d8]{};
        unsigned allocated=0,initialized=0,constructed=0,finalized=0,removed=0,release=0;
        unsigned messages=0,sounds=0,respawns=0;bool undersize=false;
        void* actor=nullptr;void* player=nullptr;
    } loop;
    loop.actor=ef.actor;loop.player=hf2.player;
    PickupFactoryBindings pfb{};pfb.context=&loop;
    pfb.allocate=[](void* p,unsigned need,void** a,unsigned* bytes) noexcept ->int{
        auto& q=*static_cast<LoopFixture*>(p);assert(q.allocated<2&&(need==0x4cc||need==0x4d8));*a=q.pickups[q.allocated++];*bytes=q.undersize?need-1:need;return 1;
    };
    pfb.initialize=[](void* p,void* a,const char* name) noexcept ->int{
        auto& q=*static_cast<LoopFixture*>(p);assert(q.initialized<2&&a==q.pickups[q.initialized]&&q.constructed>q.initialized);
        assert(!std::strcmp(name,q.initialized?"item_ammo_magnum":"item_healthvial"));
        ++q.initialized;EvalFixture::u(a,0x7c,item_touch_token);EvalFixture::u(a,0x16c,0x4321);return 1;
    };
    pfb.release_networkable=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<LoopFixture*>(p);assert(a==q.pickups[0]+0x160||a==q.pickups[1]+0x160);++q.release;return 1;};
    AmmoPickupBindings ac{};ac.context=&loop;
    ac.base_construct=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<LoopFixture*>(p);assert(a==q.pickups[q.constructed]);++q.constructed;return 1;};
    VialPickupBindings vc{};vc.context=&loop;vc.base_construct=ac.base_construct;
    pfb.ammo=&ac;pfb.vial=&vc;
    PickupFactoryResult pfr{};
    assert(!api->pickup_factory_create("weapon_magnum",&pfb,&pfr)&&!pfr.allocation_called);
    loop.undersize=true;
    assert(!api->pickup_factory_create(health_classname,&pfb,&pfr)&&pfr.allocated&&!pfr.constructed&&!pfr.networkable&&pfr.actor==loop.pickups[0]);
    loop.allocated=0;loop.undersize=false;
    seed();EvalFixture::u(ef.actor,0,resupply_vtable);EvalFixture::u(ef.actor,0xb8,0);
    EvalFixture::u(hf2.player,0,0x224aec3c);EvalFixture::u(hf2.player,0x94,2);
    EvalFixture::u(hf2.player,0x98,100);EvalFixture::u(hf2.player,0x9c,25);EvalFixture::u(hf2.player,0x650,1);
    for(unsigned i=0;i<48;++i)EvalFixture::u(hf2.player,0x6cc+4*i,0xffffffffu);
    EvalFixture::u(hf2.player,0x6cc,0x8000);EvalFixture::u(hf2.player,0x78c,0xffffffffu);
    EvalFixture::u(gf.weapon,0x4e4,1);EvalFixture::u(gf.weapon,0x4e8,0xffffffffu);
    EvaluateBindings continuous{};continuous.entries=&ge;continuous.entry_count=1;
    continuous.extent_context=&gf;continuous.extent=gb.inventory.extent;continuous.native_player_inventory=true;
    continuous.engine_cvar=&cv;continuous.nearby=eb.nearby;
    continuous.spawn.context=&loop;continuous.spawn.native_factory=&pfb;
    continuous.spawn.position=[](void*,void*,const float*) noexcept ->int{return 1;};
    continuous.spawn.orientation=continuous.spawn.position;
    continuous.spawn.owner=[](void* p,void*,void* owner) noexcept ->int{assert(owner==static_cast<LoopFixture*>(p)->actor);return 1;};
    continuous.spawn.finalize=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<LoopFixture*>(p);assert(a==q.pickups[q.finalized]&&q.initialized>q.finalized);++q.finalized;return 1;};
    continuous.spawn.linear=continuous.spawn.position;continuous.spawn.angular=continuous.spawn.position;
    continuous.spawn.world_bounds=[](void* p,void* a,float* min,float* max) noexcept ->int{auto& q=*static_cast<LoopFixture*>(p);assert(a==q.pickups[0]||a==q.pickups[1]);min[2]=a==q.pickups[0]?30.f:46.f;max[2]=min[2]+10.f;return 1;};
    continuous.spawn.random_float=[](void*,float lo,float hi,float* v) noexcept ->int{assert(lo==-6&&hi==6);*v=0;return 1;};
    continuous.spawn.remove=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<LoopFixture*>(p);assert(a==q.actor&&!EvalFixture::rd(a,8));++q.removed;return 1;};
    cv.calls=0;cv.roots[0].SetFloat(1.f);cv.roots[1].SetFloat(25.f);cv.roots[3].SetFloat(10.f);
    assert(api->evaluate(ef.actor,sizeof(ef.actor),hf2.player,sizeof(hf2.player),0,&continuous,&er)&&er.completed&&er.native_factory_calls==2&&er.normal_spawns==2&&er.ammo.ownership_native==6&&er.ammo.reserve_native==1&&er.removed);
    assert(loop.allocated==2&&loop.constructed==2&&loop.initialized==2&&loop.finalized==2&&loop.removed==1&&er.health_row.current==.25f&&er.ammo_rows[0].current==1.f/80.f);
    PlayerAmmoBindings loop_grant=gb;loop_grant.inventory.player=hf2.player;loop_grant.inventory.player_bytes=sizeof(hf2.player);
    loop_grant.context=&loop;
    loop_grant.sound=[](void* p,void* a,const char* n) noexcept ->int{auto& q=*static_cast<LoopFixture*>(p);assert(a==q.player&&!std::strcmp(n,"BaseCombatCharacter.AmmoPickup"));++q.sounds;return 1;};
    loop_grant.auto_switch_enabled=[](void*,bool* yes) noexcept ->int{*yes=false;return 1;};
    loop_grant.mark_edict=[](void*,unsigned,unsigned) noexcept ->int{return 1;};
    ac.native_grant=&loop_grant;ac.live_metric2=&metric2;ac.native_singleplayer_rules=true;
    ac.remove=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<LoopFixture*>(p);assert(a==q.pickups[0]||a==q.pickups[1]);++q.removed;return 1;};
    vc.native_heal=&ph;vc.engine_cvar=&cv;vc.native_singleplayer_rules=true;
    vc.item_pickup=[](void* p,void* a,unsigned) noexcept ->int{auto& q=*static_cast<LoopFixture*>(p);assert(a==q.player);++q.messages;return 1;};
    vc.sound=[](void* p,void* a,unsigned,const char* n) noexcept ->int{auto& q=*static_cast<LoopFixture*>(p);assert(a==q.player&&!std::strcmp(n,"HealthVial.Touch"));++q.sounds;return 1;};
    vc.respawn=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<LoopFixture*>(p);assert(a==q.pickups[0]);++q.respawns;return 1;};
    ItemTouchBindings ct{};ct.context=&loop;ct.native_singleplayer_rules=true;ct.ammo=&ac;ct.vial=&vc;ct.remove=ac.remove;
    assert(api->item_touch(loop.pickups[0],0x4cc,hf2.player,sizeof(hf2.player),&ct,&tr)&&tr.completed&&tr.vial.native_heal.after==35&&tr.vial.respawned&&tr.removed);
    assert(api->item_touch(loop.pickups[1],0x4d8,hf2.player,sizeof(hf2.player),&ct,&tr)&&tr.completed&&tr.ammo.native_grant.after==6&&tr.ammo.requested==5&&tr.ammo.granted==5&&tr.removed);
    assert(EvalFixture::rd(hf2.player,0x9c)==35&&EvalFixture::rd(hf2.player,0x650)==6&&loop.messages==1&&loop.sounds==2&&loop.respawns==1&&loop.removed==4);
    assert(api->pickup_factory_destroy(loop.pickups[0],0x4cc,loop.pickups[0]+0x160,&pfb,&pfr)&&pfr.released&&pfr.type==count);
    assert(api->pickup_factory_destroy(loop.pickups[1],0x4d8,loop.pickups[1]+0x160,&pfb,&pfr)&&pfr.released&&pfr.type==0&&loop.release==2);
    assert(!api->pickup_factory_destroy(loop.pickups[1],0x4d8,loop.pickups[1]+0x15c,&pfb,&pfr));
    // Re-evaluate same player after collection with demand below new stock.
    EvalFixture::u(ef.actor,8,123);EvalFixture::f(ef.actor,0x394,.3f);EvalFixture::f(ef.actor,0x398,.05f);cv.calls=0;
    assert(api->evaluate(ef.actor,sizeof(ef.actor),hf2.player,sizeof(hf2.player),0,&continuous,&er)&&er.completed&&!er.factories&&er.selected_health==-1&&er.selected_ammo==-1&&er.health_row.current==.35f&&er.ammo_rows[0].current==6.f/80.f&&loop.allocated==2);
    // Original dispenser, live health mutation, actual fractional dose.
    struct AntigenFixture {
        alignas(4) unsigned char actor[antigen_extent]{},steam[4]{};
        void* player=nullptr;
        float now=1.f,last_schedule=0,pose=0;
        unsigned start=0,stop=0,empty=0,stop_sound=0,creates=0,configs=0,removes=0,schedules=0,advances=0;
    } af;
    af.player=hf2.player;
    sin_independent::EntityHandleEntry ae[2]{};
    ae[0].serial=10;ae[0].entity=reinterpret_cast<CBaseEntity*>(hf2.player);
    ae[1].serial=11;ae[1].entity=reinterpret_cast<CBaseEntity*>(af.steam);
    AntigenBindings ab{};ab.context=&af;ab.entries=ae;ab.entry_count=2;ab.health=&ph;
    ab.base_construct=[](void*,void*) noexcept ->int{return 1;};
    ab.extent=[](void* p,void* a,unsigned* n) noexcept ->int{assert(a==static_cast<AntigenFixture*>(p)->player);*n=0x12a4;return 1;};
    ab.clock=[](void* p,float* v) noexcept ->int{*v=static_cast<AntigenFixture*>(p)->now;return 1;};
    ab.use_distance=[](void* p,void* a,void* pl,float distance,bool* yes) noexcept ->int{auto& q=*static_cast<AntigenFixture*>(p);assert(a==q.actor&&pl==q.player&&distance==80);*yes=true;return 1;};
    ab.sound=[](void* p,void* a,const char* name,bool play) noexcept ->int{auto& q=*static_cast<AntigenFixture*>(p);assert(a==q.actor);
        if(!std::strcmp(name,"dispenser.use_loop")){if(play)++q.start;else ++q.stop;}
        else if(!std::strcmp(name,"dispenser.stop")){assert(play);++q.stop_sound;}
        else {assert(play&&!std::strcmp(name,"dispenser.empty"));++q.empty;}return 1;};
    ab.create_steam=[](void* p,void*,unsigned* token) noexcept ->int{++static_cast<AntigenFixture*>(p)->creates;*token=0xb001;return 1;};
    ab.configure_steam=[](void* p,void* a,void* e) noexcept ->int{auto& q=*static_cast<AntigenFixture*>(p);assert(a==q.actor&&e==q.steam);++q.configs;return 1;};
    ab.remove=[](void* p,void* e) noexcept ->int{auto& q=*static_cast<AntigenFixture*>(p);assert(e==q.steam);++q.removes;return 1;};
    ab.schedule=[](void* p,void* a,float time) noexcept ->int{auto& q=*static_cast<AntigenFixture*>(p);assert(a==q.actor);q.last_schedule=time;++q.schedules;return 1;};
    ab.pose=[](void* p,void* a,const char* name,float v) noexcept ->int{auto& q=*static_cast<AntigenFixture*>(p);assert(a==q.actor&&!std::strcmp(name,"percentFull"));q.pose=v;return 1;};
    ab.advance=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<AntigenFixture*>(p);assert(a==q.actor);++q.advances;return 1;};
    AntigenResult agr{};
    assert(api->antigen_construct(af.actor,sizeof(af.actor),&ab,&agr)&&agr.constructed&&agr.remaining==100&&EvalFixture::rd(af.actor,0x4ac)==0xffffffffu);
    assert(EvalFixture::rd(af.actor,0x4c4)==0xffffffffu&&EvalFixture::rd(af.actor,0x4dc)==0xffffffffu);
    EvalFixture::u(hf2.player,0x378,0xa000);EvalFixture::u(hf2.player,0x98,100);EvalFixture::u(hf2.player,0x9c,35);
    EvalFixture::u(hf2.player,0xb8,0);EvalFixture::u(hf2.player,0x94,2);
    for(unsigned i=0;i<3;++i)EvalFixture::f(hf2.player,0x2bc+i*4,0);
    assert(api->antigen_use(af.actor,sizeof(af.actor),hf2.player,sizeof(hf2.player),&ab,&agr)&&agr.started&&agr.steam_created&&agr.steam_configured&&agr.clock_reads==2&&agr.next_time==1.f);
    assert(EvalFixture::rd(af.actor,8)==antigen_think_token&&EvalFixture::rd(af.actor,0x4ac)==0xa000);
    af.now=1.125f;
    assert(api->antigen_think(af.actor,sizeof(af.actor),&ab,&agr)&&agr.completed&&agr.scheduled&&!agr.healed&&std::abs(agr.accumulator-.7f)<.00001f);
    af.now=1.25f;
    assert(api->antigen_think(af.actor,sizeof(af.actor),&ab,&agr)&&agr.completed&&agr.healed&&agr.integer_dose==1&&agr.health.after==36&&agr.health.event_type==4&&hf2.flags==0x400000u&&std::abs(agr.remaining-98.6f)<.0001f);
    assert(af.creates==1&&af.configs==1&&af.schedules==3&&af.advances==2);
    EvalFixture::f(hf2.player,0x2bc,80.f);
    assert(api->antigen_think(af.actor,sizeof(af.actor),&ab,&agr)&&agr.stopped&&agr.distance_squared==6400&&agr.steam_removed&&agr.loop_stopped&&agr.stop_sound&&!agr.scheduled);
    assert(EvalFixture::rd(af.actor,0x4ac)==0xffffffffu&&EvalFixture::rd(af.actor,0x4b0)==0xffffffffu&&EvalFixture::rd(af.actor,8)==antigen_think_token);
    // Depletion before a whole dose still schedules one more think.
    EvalFixture::f(hf2.player,0x2bc,0.f);EvalFixture::f(af.actor,0x49c,.1f);EvalFixture::f(af.actor,0x4a4,0.f);
    assert(api->antigen_use(af.actor,sizeof(af.actor),hf2.player,sizeof(hf2.player),&ab,&agr)&&agr.started);
    af.now=1.375f;
    assert(api->antigen_think(af.actor,sizeof(af.actor),&ab,&agr)&&agr.scheduled&&agr.remaining==0&&!agr.healed);
    af.now=1.5f;
    assert(api->antigen_think(af.actor,sizeof(af.actor),&ab,&agr)&&agr.stopped&&agr.integer_dose==1&&!agr.healed);
    assert(api->antigen_use(af.actor,sizeof(af.actor),hf2.player,sizeof(hf2.player),&ab,&agr)&&agr.empty&&!agr.started&&af.empty==1);
    // Invalid serial stops without accessing stale user/steam objects.
    EvalFixture::u(af.actor,0x4ac,0xc000);EvalFixture::u(af.actor,0x4b0,0xc001);
    assert(api->antigen_think(af.actor,sizeof(af.actor),&ab,&agr)&&agr.stopped&&!agr.steam_removed&&!agr.loop_stopped&&EvalFixture::rd(af.actor,0x4b0)==0xc001);
    // Eject a live canister, retain user/dose, then reject rolling touch
    // and reinsert through the authentic CPropContainer flag/amount fields.
    struct ExchangeFixture {
        AntigenFixture* a;
        alignas(4) unsigned char canister[0x6a0]{};
        unsigned creates=0,spawns=0,groups=0,inserted=0,ejected=0,removed=0,messages=0,sounds=0;
        int last_group=-1;float fraction=0;
    } ex{&af};
    AntigenExchangeBindings xb{};xb.control=ab;xb.context=&ex;
    xb.control.context=&ex;
    xb.control.clock=[](void* p,float* v) noexcept ->int{*v=static_cast<ExchangeFixture*>(p)->a->now;return 1;};
    xb.control.sound=[](void* p,void* a,const char* name,bool play) noexcept ->int{
        auto& q=*static_cast<ExchangeFixture*>(p);assert(a==q.a->actor);
        assert((play&&(!std::strcmp(name,"dispenser.load")||!std::strcmp(name,"dispenser.eject")||!std::strcmp(name,"dispenser.stop")))||(!play&&!std::strcmp(name,"dispenser.use_loop")));++q.sounds;return 1;
    };
    xb.control.remove=[](void* p,void* c) noexcept ->int{auto& q=*static_cast<ExchangeFixture*>(p);assert(c==q.canister||c==q.a->steam);++q.removed;return 1;};
    xb.control.pose=[](void* p,void* a,const char* n,float v) noexcept ->int{auto& q=*static_cast<ExchangeFixture*>(p);assert(a==q.a->actor&&!std::strcmp(n,"percentFull"));q.a->pose=v;return 1;};
    xb.control.advance=[](void*,void*) noexcept ->int{return 1;};
    xb.create=[](void* p,const char* n,int i,void** c,unsigned* bytes) noexcept ->int{auto& q=*static_cast<ExchangeFixture*>(p);assert(!std::strcmp(n,"prop_antigen_explosive_barrel")&&i==-1);++q.creates;*c=q.canister;*bytes=sizeof(q.canister);EvalFixture::u(*c,0,antigen_canister_vtable);EvalFixture::u(*c,0x62c,9);return 1;};
    xb.position=[](void* p,void* c,const float* v) noexcept ->int{auto& q=*static_cast<ExchangeFixture*>(p);assert(c==q.canister);for(unsigned i=0;i<3;++i)EvalFixture::f(c,0x2bc+4*i,v[i]);return 1;};
    xb.orientation=[](void*,void*,const float*) noexcept ->int{return 1;};
    xb.spawn=[](void* p,void* c) noexcept ->int{auto& q=*static_cast<ExchangeFixture*>(p);assert(c==q.canister);++q.spawns;return 1;};
    xb.container_pose=[](void* p,void* c,const char* n,float v) noexcept ->int{auto& q=*static_cast<ExchangeFixture*>(p);assert(c==q.canister&&!std::strcmp(n,"percentFull"));q.fraction=v;return 1;};
    xb.local_player=[](void* p,void** pl) noexcept ->int{*pl=static_cast<ExchangeFixture*>(p)->a->player;return 1;};
    xb.eject_notice=[](void* p,void* pl,void* c,const float* pos) noexcept ->int{auto& q=*static_cast<ExchangeFixture*>(p);assert(pl==q.a->player&&c==q.canister&&pos[0]==0);++q.ejected;return 1;};
    xb.insert_notice=[](void* p,void* pl,void* c) noexcept ->int{auto& q=*static_cast<ExchangeFixture*>(p);assert(pl==q.a->player&&c==q.canister);++q.inserted;return 1;};
    xb.bodygroup=[](void* p,void* a,int i,int v) noexcept ->int{auto& q=*static_cast<ExchangeFixture*>(p);assert(a==q.a->actor&&i==1);++q.groups;q.last_group=v;return 1;};
    xb.entity_message=[](void* p,void* a,int v) noexcept ->int{auto& q=*static_cast<ExchangeFixture*>(p);assert(a==q.a->actor&&v==1);++q.messages;return 1;};
    assert(api->antigen_construct(af.actor,sizeof(af.actor),&ab,&agr));
    EvalFixture::f(af.actor,0x49c,55.f);EvalFixture::f(af.actor,0x4a4,.4f);
    EvalFixture::u(af.actor,0x4ac,0xa000);EvalFixture::u(af.actor,0x4b0,0xb001);EvalFixture::u(af.actor,8,antigen_think_token);
    af.now=10.f;AntigenExchangeResult xr{};
    assert(api->antigen_eject(af.actor,sizeof(af.actor),&xb,&xr)&&xr.completed&&xr.created&&xr.spawned&&xr.transferred&&xr.amount==55.f&&xr.control.steam_removed&&xr.message&&xr.think_cleared&&xr.output_called&&xr.reject_time==10.f);
    assert(ex.creates==1&&ex.ejected==1&&ex.removed==1&&ex.last_group==0&&std::abs(ex.fraction-.55f)<.00001f);
    assert(EvalFixture::rd(af.actor,0x4ac)==0xa000&&EvalFixture::rd(af.actor,0x4a4)==0x3ecccccdu&&EvalFixture::rd(af.actor,0x4a0)==0x41200000u);
    assert(api->antigen_eject(af.actor,sizeof(af.actor),&xb,&xr)&&xr.completed&&!xr.created);
    af.now=10.5f;
    assert(api->antigen_insert(af.actor,sizeof(af.actor),ex.canister,sizeof(ex.canister),&xb,&xr)&&xr.completed&&xr.cast_native&&xr.rejected&&xr.reject_time==10.5f);
    af.now=11.f;
    assert(api->antigen_insert(af.actor,sizeof(af.actor),ex.canister,sizeof(ex.canister),&xb,&xr)&&xr.rejected&&xr.reject_time==11.f);
    af.now=11.625f;
    assert(api->antigen_insert(af.actor,sizeof(af.actor),ex.canister,sizeof(ex.canister),&xb,&xr)&&xr.completed&&xr.transferred&&xr.amount==55.f&&xr.output_called&&xr.removed&&ex.inserted==1&&ex.last_group==1&&std::abs(af.pose-.55f)<.00001f);
    assert(api->antigen_insert(af.actor,sizeof(af.actor),ex.canister,sizeof(ex.canister),&xb,&xr)&&xr.completed&&!xr.eligible);
    // Real Use virtual press/release event and spawn/animation admission.
    struct InteractionFixture {
        AntigenFixture* a;
        unsigned events=0,setup=0,groups=0,listeners=0,screen_creates=0,screen_configs=0;
        int descriptor=0,group=-1;unsigned char screen[4]{};
    } inf{&af};
    sin_independent::EntityHandleEntry interaction_entries[3]{ae[0],ae[1],{}};
    interaction_entries[2].serial=12;interaction_entries[2].entity=reinterpret_cast<CBaseEntity*>(inf.screen);
    AntigenInteractionBindings antigen_interaction{};antigen_interaction.context=&inf;antigen_interaction.control=ab;
    antigen_interaction.control.entries=interaction_entries;antigen_interaction.control.entry_count=3;
    antigen_interaction.use_event=[](void* p,int descriptor,void* pl,void* a,int sub) noexcept ->int{
        auto& q=*static_cast<InteractionFixture*>(p);assert(pl==q.a->player&&a==q.a->actor&&sub==0);++q.events;q.descriptor=descriptor;return 1;
    };
    antigen_interaction.spawn_setup=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<InteractionFixture*>(p);assert(a==q.a->actor);++q.setup;return 1;};
    antigen_interaction.bodygroup=[](void* p,void* a,int i,int v) noexcept ->int{auto& q=*static_cast<InteractionFixture*>(p);assert(a==q.a->actor&&i==1);++q.groups;q.group=v;return 1;};
    antigen_interaction.button_listener=[](void* p,void* a,const char* n) noexcept ->int{auto& q=*static_cast<InteractionFixture*>(p);assert(a==q.a->actor+0x494&&!std::strcmp(n,"button_press"));++q.listeners;return 1;};
    antigen_interaction.create_screen=[](void* p,void* a,unsigned* h) noexcept ->int{auto& q=*static_cast<InteractionFixture*>(p);assert(a==q.a->actor);++q.screen_creates;*h=0xc002;return 1;};
    antigen_interaction.configure_screen=[](void* p,void* a,void* s) noexcept ->int{auto& q=*static_cast<InteractionFixture*>(p);assert(a==q.a->actor&&s==q.screen);++q.screen_configs;return 1;};
    assert(api->antigen_construct(af.actor,sizeof(af.actor),&ab,&agr));
    AntigenInteractionResult ir{};af.now=20.f;
    EvalFixture::f(af.actor,0x49c,200.f);
    assert(api->antigen_spawn(af.actor,sizeof(af.actor),&antigen_interaction,&ir)&&ir.completed&&ir.setup&&ir.control.remaining==100.f&&ir.control.loaded&&ir.touch_installed&&ir.animation_installed&&ir.listener);
    assert(inf.group==1&&inf.listeners==1&&EvalFixture::rd(af.actor,8)==antigen_animation_token&&EvalFixture::rd(af.actor,0x7c)==antigen_touch_token);
    assert(api->antigen_animation_think(af.actor,sizeof(af.actor),&antigen_interaction,&ir)&&ir.completed&&ir.screen_created&&ir.screen_configured&&ir.control.scheduled);
    assert(api->antigen_animation_think(af.actor,sizeof(af.actor),&antigen_interaction,&ir)&&ir.completed&&ir.screen_reused&&inf.screen_creates==1&&inf.screen_configs==1);
    assert(api->antigen_use_event(af.actor,sizeof(af.actor),hf2.player,sizeof(hf2.player),2,&antigen_interaction,&ir)&&ir.completed&&!ir.event_called&&!inf.events);
    assert(api->antigen_use_event(af.actor,sizeof(af.actor),hf2.player,sizeof(hf2.player),1,&antigen_interaction,&ir)&&ir.completed&&ir.press&&ir.event_descriptor==2&&ir.control.started);
    assert(api->antigen_use_event(af.actor,sizeof(af.actor),hf2.player,sizeof(hf2.player),0,&antigen_interaction,&ir)&&ir.completed&&ir.release&&ir.user_invalidated&&inf.descriptor==3&&inf.events==2);
    assert(EvalFixture::rd(af.actor,0x4b0)==0xb001&&EvalFixture::rd(af.actor,8)==antigen_think_token);
    assert(api->antigen_think(af.actor,sizeof(af.actor),&ab,&agr)&&agr.stopped&&agr.steam_removed);
    EvalFixture::f(af.actor,0x49c,-10.f);
    assert(api->antigen_spawn(af.actor,sizeof(af.actor),&antigen_interaction,&ir)&&ir.completed&&!ir.control.loaded&&ir.control.remaining==0.f&&inf.group==0);
    // Native frame schedule from use and screen event to healing/ejection.
    struct AntigenFrameFixture {
        AntigenFixture* a;
        alignas(4) unsigned char screen[0x170]{};
        unsigned records=0,enters=0,leaves=0,flags=0;
        bool unknown=false;
    } aff{&af};
    const auto get_antigen_frame=reinterpret_cast<GetServerAntigenPrimaryThinkApi>(GetProcAddress(s,server_antigen_primary_think_export));
    assert(get_antigen_frame&&!GetProcAddress(c,server_antigen_primary_think_export));
    const auto* antigen_frame=get_antigen_frame(1,sizeof(ServerAntigenPrimaryThinkApi));assert(antigen_frame);
    sin_independent::EntityHandleEntry af_entries[4]{ae[0],ae[1],{},{}};
    af_entries[2].serial=12;af_entries[2].entity=reinterpret_cast<CBaseEntity*>(aff.screen);
    af_entries[3].serial=13;af_entries[3].entity=reinterpret_cast<CBaseEntity*>(af.actor);
    auto frame_interaction=antigen_interaction;
    frame_interaction.control.extent=[](void* p,void* e,unsigned* bytes) noexcept ->int{
        auto& q=*static_cast<AntigenFixture*>(p);*bytes=e==q.player?0x12a4:0x170;return 1;
    };
    frame_interaction.screen_index=[](void*,unsigned token,int* i) noexcept ->int{assert(token==0x9182);*i=123;return 1;};
    frame_interaction.control.schedule=nullptr;
    ServerAntigenPrimaryThinkBindings afb{};afb.context=&aff;afb.entities.entries=af_entries;afb.entities.count=4;
    afb.interaction=&frame_interaction;afb.exchange=&xb;afb.cadence.context=&aff;
    afb.cadence.notify_think_flags=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<AntigenFrameFixture*>(p);assert(a==q.a->actor);++q.flags;return 1;};
    afb.clock=[](void* p,float* v) noexcept ->int{*v=static_cast<AntigenFrameFixture*>(p)->a->now;return 1;};
    afb.record=[](void* p,void* a,int,float,int due) noexcept ->int{auto& q=*static_cast<AntigenFrameFixture*>(p);assert(a==q.a->actor&&EvalFixture::rd(a,0x74)==unsigned(due));++q.records;return 1;};
    afb.scope=[](void* p,void* a,bool enter) noexcept ->int{auto& q=*static_cast<AntigenFrameFixture*>(p);assert(a==q.a->actor);
        if(enter){++q.enters;assert(EvalFixture::rd(a,0x74)==0xffffffffu);if(q.unknown)EvalFixture::u(a,8,0xdeadcafe);}
        else ++q.leaves;return 1;
    };
    assert(api->antigen_construct(af.actor,sizeof(af.actor),&ab,&agr));
    EvalFixture::u(af.actor,0x74,0xffffffffu);EvalFixture::u(af.actor,0xb8,0x400000u);
    EvalFixture::u(af.actor,0x4b4,0xc002);EvalFixture::u(aff.screen,0x16c,0x9182);
    EvalFixture::u(hf2.player,0x9c,40);af.now=30.f;
    ServerAntigenPrimaryThinkResult afr{};
    assert(antigen_frame->spawn(af.actor,sizeof(af.actor),0xd003,.125f,&afb,&afr)&&afr.completed&&afr.rescheduled&&afr.reschedule.tick==241&&afr.interaction.listener);
    assert(antigen_frame->run(af.actor,sizeof(af.actor),0xd003,240,.125f,&afb,&afr)&&afr.not_due);
    af.now=30.125f;
    assert(antigen_frame->run(af.actor,sizeof(af.actor),0xd003,241,.125f,&afb,&afr)&&afr.completed&&afr.invoked&&afr.token==antigen_animation_token&&afr.interaction.screen_reused&&afr.reschedule.tick==242&&afr.last_tick==241);
    assert(antigen_frame->use(af.actor,sizeof(af.actor),0xd003,.125f,hf2.player,sizeof(hf2.player),1,&afb,&afr)&&afr.interaction.control.started&&afr.reschedule.tick==241);
    af.now=30.25f;
    assert(antigen_frame->run(af.actor,sizeof(af.actor),0xd003,242,.125f,&afb,&afr)&&afr.completed&&afr.token==antigen_think_token&&!afr.healing.healed&&afr.reschedule.tick==243);
    af.now=30.375f;
    assert(antigen_frame->run(af.actor,sizeof(af.actor),0xd003,243,.125f,&afb,&afr)&&afr.healing.healed&&afr.healing.health.after==41&&afr.reschedule.tick==244);
    assert(antigen_frame->use(af.actor,sizeof(af.actor),0xd003,.125f,hf2.player,sizeof(hf2.player),0,&afb,&afr)&&afr.interaction.user_invalidated&&!afr.rescheduled);
    af.now=30.5f;
    assert(antigen_frame->run(af.actor,sizeof(af.actor),0xd003,244,.125f,&afb,&afr)&&afr.healing.stopped&&afr.healing.steam_removed&&!afr.rescheduled&&afr.closed);
    assert(antigen_frame->run(af.actor,sizeof(af.actor),0xd003,245,.125f,&afb,&afr)&&afr.no_think);
    assert(antigen_frame->button(af.actor,sizeof(af.actor),0xd003,.125f,"button_press",122,"eject",&afb,&afr)&&afr.interaction.button_checked&&!afr.interaction.button_matched&&!afr.rescheduled);
    assert(antigen_frame->button(af.actor,sizeof(af.actor),0xd003,.125f,"button_press",123,"Eject",&afb,&afr)&&!afr.interaction.button_matched);
    assert(antigen_frame->button(af.actor,sizeof(af.actor),0xd003,.125f,"button_press",123,"eject",&afb,&afr)&&afr.interaction.eject_installed&&afr.reschedule.tick==244);
    assert(antigen_frame->run(af.actor,sizeof(af.actor),0xd003,244,.125f,&afb,&afr)&&afr.completed&&afr.token==antigen_eject_token&&afr.exchange.created&&afr.exchange.think_cleared&&!afr.rescheduled&&afr.closed);
    assert(!antigen_frame->run(af.actor,sizeof(af.actor),0xe003,244,.125f,&afb,&afr));
    assert(aff.records==5&&aff.enters==5&&aff.leaves==5);
    // Factory aliases and canister-specific constructor/amount/idle paths.
    struct AntigenFactoryFixture {
        ExchangeFixture* ex;
        alignas(4) unsigned char dispenser[antigen_extent]{};
        unsigned allocations=0,initializes=0,constructs=0,releases=0,spawns=0,poses=0,advances=0,schedules=0;
        bool short_allocation=false;
        float fraction=0,next=0;
    } affactory{&ex};
    sin_port::native_resupply::CanisterBindings canister_bind{};
    canister_bind.context=&affactory;
    canister_bind.base_construct=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<AntigenFactoryFixture*>(p);assert(a==q.ex->canister);++q.constructs;return 1;};
    canister_bind.spawn_setup=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<AntigenFactoryFixture*>(p);assert(a==q.ex->canister);++q.spawns;return 1;};
    canister_bind.pose=[](void* p,void* a,const char* name,float fraction) noexcept ->int{auto& q=*static_cast<AntigenFactoryFixture*>(p);assert(a==q.ex->canister&&!std::strcmp(name,"percentFull"));++q.poses;q.fraction=fraction;return 1;};
    canister_bind.advance=[](void* p,void*) noexcept ->int{++static_cast<AntigenFactoryFixture*>(p)->advances;return 1;};
    canister_bind.clock=[](void* p,float* time) noexcept ->int{*time=static_cast<AntigenFactoryFixture*>(p)->ex->a->now;return 1;};
    canister_bind.random_float=[](void*,float lo,float hi,float* draw) noexcept ->int{assert(lo==.5f&&hi==1.5f);*draw=.75f;return 1;};
    canister_bind.schedule=[](void* p,void* a,float time) noexcept ->int{auto& q=*static_cast<AntigenFactoryFixture*>(p);assert(a==q.ex->canister);++q.schedules;q.next=time;return 1;};
    AntigenFactoryBindings antigen_factory{};
    antigen_factory.context=&affactory;antigen_factory.dispenser=&ab;antigen_factory.canister=&canister_bind;
    antigen_factory.allocate=[](void* p,unsigned extent,void** a,unsigned* bytes) noexcept ->int{
        auto& q=*static_cast<AntigenFactoryFixture*>(p);++q.allocations;assert(extent==antigen_extent||extent==antigen_canister_extent);
        *a=extent==antigen_extent?q.dispenser:q.ex->canister;std::memset(*a,0,extent);*bytes=q.short_allocation?extent-1:extent;return 1;
    };
    antigen_factory.initialize=[](void* p,void* a,const char* name) noexcept ->int{
        auto& q=*static_cast<AntigenFactoryFixture*>(p);++q.initializes;
        if(a==q.dispenser){assert(!std::strcmp(name,"item_antigen_dispenser")&&EvalFixture::rd(a,0)==antigen_vtable);}
        else{assert(a==q.ex->canister&&(!std::strcmp(name,"prop_antigen_canister")||!std::strcmp(name,"prop_antigen_explosive_barrel"))&&EvalFixture::rd(a,0)==antigen_canister_vtable);}
        return 1;
    };
    antigen_factory.release_networkable=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<AntigenFactoryFixture*>(p);assert(a==q.dispenser+0x160||a==q.ex->canister+0x160);++q.releases;return 1;};
    AntigenFactoryResult afres{};
    assert(!api->antigen_factory_create("prop_dynamic",&antigen_factory,&afres)&&!afres.allocation_called);
    affactory.short_allocation=true;
    assert(!api->antigen_factory_create("prop_antigen_canister",&antigen_factory,&afres)&&afres.allocated&&!afres.constructed);
    affactory.short_allocation=false;
    assert(api->antigen_factory_create("item_antigen_dispenser",&antigen_factory,&afres)&&afres.completed&&afres.type==0&&afres.dispenser.constructed&&afres.networkable==affactory.dispenser+0x160);
    assert(api->antigen_factory_destroy(affactory.dispenser,sizeof(affactory.dispenser),afres.networkable,&antigen_factory,&afres)&&afres.released);
    for(unsigned alias=1;alias<=2;++alias){
        assert(api->antigen_factory_create(antigen_factories[alias].classname,&antigen_factory,&afres)&&afres.completed&&afres.type==alias&&afres.canister.constructed);
        assert(EvalFixture::rd(ex.canister,0x62c)==9&&EvalFixture::rd(ex.canister,0x630)==0x2246a8f4&&EvalFixture::rd(ex.canister,0x678)==0xffffffffu&&EvalFixture::rd(ex.canister,0x69c)==0xffffffffu);
        assert(EvalFixture::rd(ex.canister,0x494)==0x2246f01c&&EvalFixture::rd(ex.canister,0x498)==0x2246eff4);
        assert(api->antigen_factory_destroy(ex.canister,sizeof(ex.canister),ex.canister+0x160,&antigen_factory,&afres)&&afres.released);
    }
    CanisterResult canres{};af.now=40.f;
    assert(api->canister_set_amount(ex.canister,sizeof(ex.canister),150.f,&canister_bind,&canres)&&canres.amount_written&&canres.fraction>1.f);
    assert(api->canister_spawn(ex.canister,sizeof(ex.canister),&canister_bind,&canres)&&canres.completed&&canres.base_spawn&&canres.think_installed&&canres.next_time==40.25f);
    af.now=40.25f;
    assert(api->canister_idle(ex.canister,sizeof(ex.canister),&canister_bind,&canres)&&canres.advanced&&canres.random_called&&canres.next_time==42.f);
    // Ejection now uses real factory and native canister spawn/amount,
    // with no hosted create, spawn, or amount-pose adapter.
    auto actual_exchange=xb;actual_exchange.create=nullptr;actual_exchange.spawn=nullptr;actual_exchange.container_pose=nullptr;
    actual_exchange.native_factory=&antigen_factory;actual_exchange.native_canister=&canister_bind;
    assert(api->antigen_construct(af.actor,sizeof(af.actor),&ab,&agr));
    EvalFixture::f(af.actor,0x49c,42.f);af.now=50.f;
    assert(api->antigen_eject(af.actor,sizeof(af.actor),&actual_exchange,&xr)&&xr.completed&&xr.factory.initialized&&xr.factory.type==1&&xr.canister_spawn.think_installed&&xr.canister_amount.amount_written&&xr.amount==42.f);
    assert(EvalFixture::rd(ex.canister,8)==antigen_canister_idle&&std::abs(affactory.fraction-.42f)<.00001f&&affactory.next==50.25f);
    af.now=51.f;
    assert(api->antigen_insert(af.actor,sizeof(af.actor),ex.canister,sizeof(ex.canister),&actual_exchange,&xr)&&xr.completed&&xr.cast_native&&xr.transferred&&xr.amount==42.f);
    // Real client panel OnCommand -> event fields -> server native
    // listener -> scheduled ejection -> native canister construction.
    namespace vgui_button=sin_port::native_vgui_button;
    const auto get_button=reinterpret_cast<vgui_button::GetApi>(GetProcAddress(c,vgui_button::export_name));
    assert(get_button&&!GetProcAddress(s,vgui_button::export_name)&&!get_button(0,sizeof(vgui_button::Api)));
    const auto* client_button=get_button(1,sizeof(vgui_button::Api));assert(client_button);
    struct ClientButtonFixture {
        alignas(4) unsigned char panel[0xdc]{},screen[0x54]{};
        const ServerAntigenPrimaryThinkApi* frame;
        ServerAntigenPrimaryThinkBindings* server;
        AntigenFixture* antigen;
        ServerAntigenPrimaryThinkResult server_result{};
        unsigned creates=0,sets=0,fires=0,discards=0;
        bool missing=false,fail_string=false;
        int index=0;char event[32]{},command[32]{};
    } cbf{};
    cbf.frame=antigen_frame;cbf.server=&afb;cbf.antigen=&af;
    sin_independent::EntityHandleEntry client_entries[1]{};
    client_entries[0].serial=14;client_entries[0].entity=reinterpret_cast<CBaseEntity*>(cbf.screen);
    EvalFixture::u(cbf.panel,0,vgui_button::panel_vtable);EvalFixture::u(cbf.panel,0xd8,0xe000);
    EvalFixture::u(cbf.screen,0,vgui_button::screen_vtable);EvalFixture::u(cbf.screen,8,vgui_button::screen_networkable_vtable);EvalFixture::u(cbf.screen,0x50,123);
    vgui_button::Bindings cbb{};cbb.context=&cbf;cbb.entries=client_entries;cbb.entry_count=1;
    cbb.extent=[](void* p,void* e,unsigned* bytes) noexcept ->int{auto& q=*static_cast<ClientButtonFixture*>(p);assert(e==q.screen);*bytes=sizeof(q.screen);return 1;};
    cbb.create_event=[](void* p,const char* n,bool force,void** out) noexcept ->int{
        auto& q=*static_cast<ClientButtonFixture*>(p);assert(force&&!std::strcmp(n,"button_press"));++q.creates;
        std::memcpy(q.event,n,std::strlen(n)+1);*out=q.missing?nullptr:&q;return 1;
    };
    cbb.set_int=[](void* p,void* e,const char* n,int value) noexcept ->int{
        auto& q=*static_cast<ClientButtonFixture*>(p);assert(e==&q&&!std::strcmp(n,"entindex"));q.index=value;++q.sets;return 1;
    };
    cbb.set_string=[](void* p,void* e,const char* n,const char* value) noexcept ->int{
        auto& q=*static_cast<ClientButtonFixture*>(p);assert(e==&q&&!std::strcmp(n,"command"));if(q.fail_string)return 0;
        const auto len=std::strlen(value)+1;assert(len<=sizeof(q.command));std::memcpy(q.command,value,len);++q.sets;return 1;
    };
    cbb.fire_event=[](void* p,void* e,bool dont_broadcast) noexcept ->int{
        auto& q=*static_cast<ClientButtonFixture*>(p);assert(e==&q&&!dont_broadcast);++q.fires;
        return q.frame->button(q.antigen->actor,sizeof(q.antigen->actor),0xd003,.125f,q.event,q.index,q.command,q.server,&q.server_result);
    };
    cbb.discard_event=[](void* p,void* e) noexcept ->int{auto& q=*static_cast<ClientButtonFixture*>(p);assert(e==&q);++q.discards;return 1;};
    vgui_button::Result cbr{};
    afb.exchange=&actual_exchange;EvalFixture::u(af.actor,0x4b4,0xc002);
    af.now=60.f;
    cbf.missing=true;EvalFixture::u(cbf.panel,0xd8,0xf000);
    assert(client_button->command(cbf.panel,sizeof(cbf.panel),"eject",&cbb,&cbr)&&cbr.completed&&cbr.create_called&&!cbr.created&&cbr.base_noop&&!cbf.fires);
    cbf.missing=false;
    assert(!client_button->command(cbf.panel,sizeof(cbf.panel),"eject",&cbb,&cbr)&&cbr.created&&!cbr.resolved&&cbr.discarded&&cbf.discards==1);
    EvalFixture::u(cbf.panel,0xd8,0xe000);cbf.fail_string=true;
    assert(!client_button->command(cbf.panel,sizeof(cbf.panel),"eject",&cbb,&cbr)&&cbr.index_native&&cbr.int_set&&!cbr.string_set&&cbr.discarded&&cbf.discards==2);
    cbf.fail_string=false;EvalFixture::u(cbf.screen,0x50,124);
    assert(client_button->command(cbf.panel,sizeof(cbf.panel),"eject",&cbb,&cbr)&&cbr.fired&&cbr.index==124&&cbf.server_result.interaction.button_checked&&!cbf.server_result.interaction.button_matched);
    EvalFixture::u(cbf.screen,0x50,123);
    assert(client_button->command(cbf.panel,sizeof(cbf.panel),"eject",&cbb,&cbr)&&cbr.completed&&cbr.fired&&cbr.index_native&&cbr.index==123&&cbf.server_result.interaction.eject_installed&&cbf.server_result.reschedule.tick==480);
    assert(antigen_frame->run(af.actor,sizeof(af.actor),0xd003,480,.125f,&afb,&afr)&&afr.completed&&afr.exchange.factory.initialized&&afr.exchange.canister_amount.amount_written&&afr.exchange.amount==42.f&&afr.exchange.think_cleared);
    // Damage the actual canister created by the client-driven ejection.
    struct DamageFixture {
        ExchangeFixture* ex;void* player;
        unsigned effects=0,events=0,clouds=0,fragments=0,removes=0,schedules=0,stops=0;
        alignas(4) unsigned char damage[0x48]{};
    } adf{&ex,hf2.player};
    AntigenDamageBindings adb{};adb.context=&adf;adb.entries=af_entries;adb.entry_count=4;
    adb.damage_effect=[](void* p,void* a,const void* d) noexcept ->int{auto& q=*static_cast<DamageFixture*>(p);assert(a==q.ex->canister&&d==q.damage);++q.effects;return 1;};
    adb.handle=[](void* p,void* a,unsigned* h) noexcept ->int{assert(a==static_cast<DamageFixture*>(p)->player);*h=0xa000;return 1;};
    adb.break_event=[](void* p,int descriptor,void* attacker,void* a) noexcept ->int{auto& q=*static_cast<DamageFixture*>(p);assert(descriptor==10&&attacker==q.player&&a==q.ex->canister);++q.events;return 1;};
    adb.cloud=[](void* p,void* a,const float* pos,int type,float amount,float radius,float x,float y) noexcept ->int{auto& q=*static_cast<DamageFixture*>(p);assert(a==q.ex->canister&&type==1&&std::abs(amount-4.2f)<.00001f&&radius==150&&x==-1&&y==-1&&pos[0]==0);++q.clouds;return 1;};
    adb.schedule=[](void* p,void* a,float time) noexcept ->int{auto& q=*static_cast<DamageFixture*>(p);assert(a==q.ex->canister&&time==-1&&!EvalFixture::rd(a,8));++q.schedules;return 1;};
    adb.remove=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<DamageFixture*>(p);assert(a==q.ex->canister||a==q.ex->a->steam);++q.removes;return 1;};
    adb.fragment=[](void* p,void* a,const char* name,int count,float x,float y,float z) noexcept ->int{
        auto& q=*static_cast<DamageFixture*>(p);constexpr const char* models[]{"models/items/antigen/antigen_break01.mdl","models/items/antigen/antigen_break02.mdl","models/items/antigen/antigen_break03.mdl","models/items/antigen/antigen_break04.mdl"};
        assert(a==q.ex->canister&&q.removes&&q.fragments<4&&!std::strcmp(name,models[q.fragments])&&count==1&&x==200&&y==-200&&z==25);++q.fragments;return 1;
    };
    adb.stop_sound=[](void* p,unsigned token) noexcept ->int{assert(token==99);++static_cast<DamageFixture*>(p)->stops;return 1;};
    AntigenDamageResult adr{};
    EvalFixture::u(adf.damage,0x28,0xa000);EvalFixture::u(adf.damage,0x38,0x100);
    assert(api->antigen_canister_damage(ex.canister,sizeof(ex.canister),adf.damage,sizeof(adf.damage),&adb,&adr)&&adr.completed&&adr.native_result==1&&!adr.damage_admitted);
    assert(EvalFixture::rd(ex.canister,0x638)==1u);
    EvalFixture::u(adf.damage,0x38,2);
    assert(api->antigen_canister_damage(ex.canister,sizeof(ex.canister),adf.damage,sizeof(adf.damage),&adb,&adr)&&adr.completed&&adr.damage_admitted&&adr.damage_effect&&adr.attacker_handle==0xa000&&adr.broken&&adr.break_event&&adr.output_called&&adr.clouds==1&&adr.removed&&adr.fragments==4);
    assert(adf.effects==1&&adf.events==1&&adf.clouds==1&&adf.fragments==4&&adf.removes==1&&adf.schedules==1&&EvalFixture::rd(ex.canister,0x62c)==8);
    // Explicit leak termination on restored/non-default flag state:
    // count is signed integer, stale steam handles remain unchanged.
    EvalFixture::u(ex.canister,0x62c,0x10b);EvalFixture::u(ex.canister,0x638,1);EvalFixture::u(ex.canister,0x640,99);EvalFixture::u(ex.canister,0x668,0xb001);
    EvalFixture::f(ex.canister,0x63c,10);EvalFixture::f(ex.canister,0x628,42);
    assert(api->antigen_canister_stop_leak(ex.canister,sizeof(ex.canister),&adb,&adr)&&adr.completed&&adr.leak_stopped&&adr.steam_removed&&adr.sound_stopped&&adr.leak_decremented&&adr.leak_count==0&&adr.depleted);
    assert(EvalFixture::rd(ex.canister,0x62c)==9&&EvalFixture::rd(ex.canister,0x628)==0&&EvalFixture::rd(ex.canister,0x668)==0xffffffffu);
    EvalFixture::u(ex.canister,0x62c,0x109);EvalFixture::u(ex.canister,0x638,2);EvalFixture::u(ex.canister,0x668,0xdead0001);
    assert(api->antigen_canister_stop_leak(ex.canister,sizeof(ex.canister),&adb,&adr)&&adr.completed&&adr.leak_count==1&&!adr.steam_removed&&!adr.depleted&&EvalFixture::rd(ex.canister,0x668)==0xdead0001u);
    assert(api->antigen_canister_stop_leak(ex.canister,sizeof(ex.canister),&adb,&adr)&&adr.completed&&!adr.leak_stopped);
    // Native gas creation from canister break, original lifetime/pulse
    // behavior with existing engine radius service (not a fake instant heal).
    struct GasFixture {
        alignas(4) unsigned char gas[antigen_gas_extent]{};
        float now=70.f,last_time=0,amount=0;
        unsigned allocs=0,particles=0,particle_removes=0,starts=0,stops=0,loops=0,pulses=0,removes=0,schedules=0;
        bool null_factory=false;
    } agf;
    AntigenGasBindings agb{};agb.context=&agf;
    agb.allocate=[](void* p,unsigned n,void** a,unsigned* size) noexcept ->int{auto& q=*static_cast<GasFixture*>(p);assert(n==antigen_gas_extent);++q.allocs;std::memset(q.gas,0,n);*a=q.null_factory?nullptr:q.gas;*size=n;return 1;};
    agb.base_construct=[](void*,void*) noexcept ->int{return 1;};
    agb.initialize=[](void*,void*,const char* name) noexcept ->int{assert(!std::strcmp(name,"env_poison_gas"));return 1;};
    agb.position=[](void*,void* a,const float* pos) noexcept ->int{for(unsigned i=0;i<3;++i)EvalFixture::f(a,0x2bc+4*i,pos[i]);return 1;};
    agb.spawn_setup=[](void*,void*) noexcept ->int{return 1;};
    agb.clock=[](void* p,float* now) noexcept ->int{*now=static_cast<GasFixture*>(p)->now;return 1;};
    agb.random_float=[](void*,float lo,float hi,float* out) noexcept ->int{assert(lo==0&&hi==.5f);*out=.125f;return 1;};
    agb.schedule=[](void* p,void* a,float time) noexcept ->int{auto& q=*static_cast<GasFixture*>(p);assert(a==q.gas);q.last_time=time;++q.schedules;return 1;};
    agb.particle=[](void* p,void* a,unsigned* token) noexcept ->int{auto& q=*static_cast<GasFixture*>(p);assert(a==q.gas);++q.particles;*token=0x9876;return 1;};
    agb.remove_particle=[](void* p,unsigned token) noexcept ->int{assert(token==0x9876);++static_cast<GasFixture*>(p)->particle_removes;return 1;};
    agb.sound=[](void* p,void* a,const char* n,bool play) noexcept ->int{
        auto& q=*static_cast<GasFixture*>(p);assert(a==q.gas);
        if(!std::strcmp(n,"gas.antigen_spawn")){assert(play);++q.starts;}
        else{assert(!std::strcmp(n,"gas.antigen_loop"));if(play)++q.loops;else ++q.stops;}return 1;
    };
    agb.radius_effect=[](void* p,void* a,const float* pos,float amount,float radius,unsigned flags) noexcept ->int{
        auto& q=*static_cast<GasFixture*>(p);assert(a==q.gas&&pos[0]==0&&radius==150&&flags==0x400000u);++q.pulses;q.amount=amount;return 1;
    };
    agb.remove=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<GasFixture*>(p);assert(a==q.gas);++q.removes;return 1;};
    AntigenGasResult agras{};
    float gas_origin[3]{};
    agf.null_factory=true;
    assert(api->antigen_gas_create(gas_origin,1,150,-1,-1,&agb,&agras)&&agras.completed&&!agras.allocated);
    agf.null_factory=false;
    assert(api->antigen_gas_create(gas_origin,1,150,-1,99,&agb,&agras)&&agras.completed&&agras.rate_reset&&agras.rate==5&&agras.particle_life==15&&agras.remaining==1&&agras.next_time==70.125f);
    assert(agf.starts==1&&agf.particles==1&&EvalFixture::rd(agf.gas,8)==antigen_gas_think_token);
    agf.now=70.125f;
    assert(api->antigen_gas_think(agf.gas,sizeof(agf.gas),&agb,&agras)&&agras.completed&&!agras.radius_called&&agras.next_time==70.375f);
    agf.now=70.375f;
    assert(api->antigen_gas_think(agf.gas,sizeof(agf.gas),&agb,&agras)&&!agras.elapsed_admitted&&!agras.radius_called);
    agf.now=70.625f;
    assert(api->antigen_gas_think(agf.gas,sizeof(agf.gas),&agb,&agras)&&agras.completed&&agras.radius_called&&agras.amount==2.5f&&agras.remaining==.5f&&agras.particle_removed&&agras.loop_stopped&&agras.schedules==3&&agras.next_time==70.875f);
    agf.now=71.125f;
    assert(api->antigen_gas_think(agf.gas,sizeof(agf.gas),&agb,&agras)&&agras.completed&&agras.expired&&agras.removed&&agras.amount==2.5f&&agras.remaining==0);
    assert(agf.pulses==2&&agf.particle_removes==1&&agf.stops==1&&agf.removes==1);
    // Drive native gas factory from the actual canister break path.
    assert(api->antigen_factory_create("prop_antigen_canister",&antigen_factory,&afres));
    EvalFixture::f(ex.canister,0x628,42.f);EvalFixture::u(ex.canister,0x69c,0xa000);
    adf.fragments=0;adb.cloud=nullptr;adb.native_gas=&agb;agf.now=80.f;
    assert(api->antigen_canister_break(ex.canister,sizeof(ex.canister),&adb,&adr)&&adr.completed&&adr.broken&&adr.gas.completed&&adr.gas.started&&std::abs(adr.gas.remaining-4.2f)<.00001f&&adr.gas.radius==150&&adr.gas.rate==5);
    assert(api->antigen_gas_stop(agf.gas,sizeof(agf.gas),&agb,&agras)&&agras.completed&&agras.particle_removed&&agras.next_time==-1);
    // MegaHealth uses a separate cvar and max-health setter before the
    // normal native healing path; failed heal does not roll back max.
    struct MegaFixture {
        alignas(4) unsigned char item[0x4cc]{};
        void* player;
        unsigned allocs=0,constructs=0,initializes=0,messages=0,sounds=0,respawns=0,removes=0,releases=0;
    } mgf{};mgf.player=hf2.player;
    VialPickupBindings mgb{};mgb.context=&mgf;mgb.engine_cvar=&cv;mgb.native_heal=&ph;mgb.native_singleplayer_rules=true;
    mgb.base_construct=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<MegaFixture*>(p);assert(a==q.item);++q.constructs;return 1;};
    mgb.item_pickup=[](void* p,void* a,unsigned token) noexcept ->int{auto& q=*static_cast<MegaFixture*>(p);assert(a==q.player&&token==0);++q.messages;return 1;};
    mgb.sound=[](void* p,void* a,unsigned,const char* name) noexcept ->int{auto& q=*static_cast<MegaFixture*>(p);assert(a==q.player&&!std::strcmp(name,"HealthVial.Touch"));++q.sounds;return 1;};
    mgb.respawn=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<MegaFixture*>(p);assert(a==q.item);++q.respawns;return 1;};
    mgb.remove=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<MegaFixture*>(p);assert(a==q.item);++q.removes;return 1;};
    PickupFactoryBindings mgfactory{};mgfactory.context=&mgf;mgfactory.mega=&mgb;
    mgfactory.allocate=[](void* p,unsigned n,void** a,unsigned* bytes) noexcept ->int{auto& q=*static_cast<MegaFixture*>(p);assert(n==sizeof(q.item));++q.allocs;std::memset(q.item,0,n);*a=q.item;*bytes=n;return 1;};
    mgfactory.initialize=[](void* p,void* a,const char* name) noexcept ->int{auto& q=*static_cast<MegaFixture*>(p);assert(a==q.item&&!std::strcmp(name,"item_megahealth"));++q.initializes;return 1;};
    mgfactory.release_networkable=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<MegaFixture*>(p);assert(a==q.item+0x160);++q.releases;return 1;};
    assert(api->pickup_factory_create(megahealth_classname,&mgfactory,&pfr)&&pfr.completed&&pfr.type==count+1&&pfr.mega.constructed&&pfr.networkable==mgf.item+0x160);
    assert(EvalFixture::rd(mgf.item,0)==megahealth_vtable&&EvalFixture::rd(mgf.item,0x494)==megahealth_secondary);
    EvalFixture::u(hf2.player,0x98,100);EvalFixture::u(hf2.player,0x9c,100);EvalFixture::u(hf2.player,0x94,2);EvalFixture::u(hf2.player,0x12a1,0);
    cv.calls=0;cv.roots[4].SetFloat(100.f);
    ItemTouchBindings mgt{};mgt.context=&mgf;mgt.mega=&mgb;mgt.native_singleplayer_rules=true;mgt.remove=mgb.remove;
    EvalFixture::u(mgf.item,0x7c,item_touch_token);
    assert(api->item_touch(mgf.item,sizeof(mgf.item),hf2.player,sizeof(hf2.player),&mgt,&tr)&&tr.completed&&tr.accepted&&tr.mega.mega_max_set&&tr.mega.mega_units==100&&tr.mega.mega_max_before==100&&tr.mega.mega_max_after==200&&tr.mega.native_heal.after==200&&tr.mega.native_heal.overheal_flag_cleared&&tr.removed);
    assert(cv.calls==1&&cv.lookups[0]==4&&mgf.messages==1&&mgf.sounds==1&&mgf.respawns==1&&mgf.removes==1);
    // A later ordinary heal resets max to100 but does not clamp existing HP.
    assert(api->player_take_health(hf2.player,10,0,&ph,&phr)&&phr.completed&&phr.maximum_reset&&!phr.accepted&&phr.after==200&&phr.maximum_after==100);
    EvalFixture::u(hf2.player,0x98,100);EvalFixture::u(hf2.player,0x9c,90);EvalFixture::u(hf2.player,0x94,1);
    cv.calls=0;cv.roots[4].SetFloat(20.75f);
    VialPickupResult mhr{};
    assert(api->mega_touch(mgf.item,sizeof(mgf.item),hf2.player,&mgb,&mhr)&&mhr.completed&&mhr.mega_units==20&&mhr.mega_max_after==120&&!mhr.accepted&&!mhr.message&&EvalFixture::rd(hf2.player,0x98)==120);
    EvalFixture::u(hf2.player,0x94,2);cv.calls=0;cv.roots[4].SetFloat(250.f);
    assert(api->mega_touch(mgf.item,sizeof(mgf.item),hf2.player,&mgb,&mhr)&&mhr.completed&&mhr.mega_max_before==120&&mhr.mega_max_after==200&&mhr.native_heal.raw_health==340&&mhr.native_heal.after==200);
    assert(api->pickup_factory_destroy(mgf.item,sizeof(mgf.item),mgf.item+0x160,&mgfactory,&pfr)&&pfr.released&&pfr.type==count+1&&mgf.releases==1);
    // Gas radius dispatch: original water gate, fixed damage (no falloff),
    // strict trace target hit, nonstrict obstruction attenuation.
    struct RadiusFixture {
        GasFixture* gas;void* player;
        alignas(4) unsigned char blocker[0x29c]{},world[0x29c]{};
        unsigned mode=0,contents=0,direct=0,attacks=0,after=0,force=0,clear=0,apply=0;
        float damage=0,mass=100.f;
        bool wet=false,strict=true;
    } arf{&agf,hf2.player};
    sin_independent::EntityHandleEntry radius_entries[2]{};
    radius_entries[0].serial=15;radius_entries[0].entity=reinterpret_cast<CBaseEntity*>(agf.gas);
    radius_entries[1].serial=10;radius_entries[1].entity=reinterpret_cast<CBaseEntity*>(hf2.player);
    AntigenRadiusBindings arb{};arb.context=&arf;arb.entries=radius_entries;arb.entry_count=2;
    arb.contents=[](void* p,const float* pos,unsigned* v) noexcept ->int{auto& q=*static_cast<RadiusFixture*>(p);++q.contents;assert(pos[2]==0||pos[2]==24);*v=q.wet?0x20:0;return 1;};
    arb.enumerate=[](void* p,const float* source,float radius,void** items,unsigned cap,unsigned* n) noexcept ->int{auto& q=*static_cast<RadiusFixture*>(p);assert(source[2]==1&&radius==150&&cap==512);items[0]=q.gas->gas;items[1]=q.player;*n=2;return 1;};
    arb.extent=[](void* p,void* e,unsigned* n) noexcept ->int{auto& q=*static_cast<RadiusFixture*>(p);*n=e==q.gas->gas?antigen_gas_extent:e==q.player?0x12a4:0x29c;return 1;};
    arb.body_target=[](void*,void*,const float*,float* body) noexcept ->int{body[0]=25;body[1]=0;body[2]=1;return 1;};
    arb.strict_trace=[](void* p,bool* yes) noexcept ->int{*yes=static_cast<RadiusFixture*>(p)->strict;return 1;};
    arb.world=[](void* p,void** e) noexcept ->int{*e=static_cast<RadiusFixture*>(p)->world;return 1;};
    arb.trace=[](void* p,const float* source,const float* body,unsigned mask,void* gas,void* target,AntigenRadiusTrace* out) noexcept ->int{
        auto& q=*static_cast<RadiusFixture*>(p);assert(gas==q.gas->gas&&source[2]==1&&body[0]==25);
        for(unsigned i=0;i<3;++i){out->start[i]=source[i];out->end[i]=body[i];}
        if(mask==1){assert(!target);out->fraction=1;return 1;}
        assert(mask==0x46004003u&&target==q.player);
        out->fraction=q.mode? .5f:1.f;out->hit=q.mode==1?q.player:q.mode==2?q.blocker:q.mode==3?q.world:nullptr;return 1;
    };
    arb.allow_corner=[](void*,void*,bool* yes) noexcept ->int{*yes=true;return 1;};
    arb.corner_trace=[](void*,void*,void*,const float*,const float*,AntigenRadiusTrace* tr) noexcept ->int{tr->fraction=1;tr->hit=nullptr;return 1;};
    arb.mass=[](void* p,void* e,float* mass) noexcept ->int{auto& q=*static_cast<RadiusFixture*>(p);assert(e==q.blocker);*mass=q.mass;return 1;};
    arb.prepare_force=[](void* p,void* gas,void* info,const float*,const float* direction,float scale) noexcept ->int{auto& q=*static_cast<RadiusFixture*>(p);assert(gas==q.gas->gas&&scale==1&&direction[0]==1&&EvalFixture::rd(info,0x24)==0xf000&&EvalFixture::rd(info,0x28)==0xf000&&EvalFixture::rd(info,0x38)==0x400000);++q.force;return 1;};
    arb.take_damage=[](void* p,void* e,const void* info) noexcept ->int{auto& q=*static_cast<RadiusFixture*>(p);assert(e==q.player);std::memcpy(&q.damage,static_cast<const unsigned char*>(info)+0x2c,4);++q.direct;return 1;};
    arb.clear_multi=[](void* p) noexcept ->int{++static_cast<RadiusFixture*>(p)->clear;return 1;};
    arb.apply_multi=[](void* p) noexcept ->int{++static_cast<RadiusFixture*>(p)->apply;return 1;};
    arb.trace_attack=[](void* p,void* e,const void*,const float*,const AntigenRadiusTrace* tr) noexcept ->int{auto& q=*static_cast<RadiusFixture*>(p);assert(e==q.player&&tr->hit==e);++q.attacks;return 1;};
    arb.after_effect=[](void* p,void* e,const void*,const float*,const AntigenRadiusTrace*,const float*) noexcept ->int{auto& q=*static_cast<RadiusFixture*>(p);assert(e==q.player);++q.after;return 1;};
    AntigenRadiusResult arr{};
    EvalFixture::u(agf.gas,0x378,0xf000);EvalFixture::u(agf.gas,0x94,0);
    EvalFixture::u(hf2.player,0x94,2);EvalFixture::u(hf2.player,0x298,0);
    assert(api->antigen_radius(agf.gas,sizeof(agf.gas),gas_origin,5.f,150.f,&arb,&arr)&&arr.completed&&arr.visited==2&&arr.ignored==1&&arr.direct_attacks==1&&arr.last_amount==5&&arf.damage==5);
    arf.mode=1;
    assert(api->antigen_radius(agf.gas,sizeof(agf.gas),gas_origin,5.f,150.f,&arb,&arr)&&arr.trace_attacks==1&&arf.attacks==1&&arf.clear==1&&arf.apply==1);
    arf.mode=2;
    assert(api->antigen_radius(agf.gas,sizeof(agf.gas),gas_origin,5.f,150.f,&arb,&arr)&&arr.strict_rejected==1&&!arr.direct_attacks);
    arf.strict=false;EvalFixture::u(arf.blocker,0x20c,0xffffffffu);
    assert(api->antigen_radius(agf.gas,sizeof(agf.gas),gas_origin,5.f,150.f,&arb,&arr)&&arr.obstruction_traces==1&&arr.direct_attacks==1&&arr.last_amount==3.75f);
    EvalFixture::u(arf.blocker,0x218,1);arf.mass=350.f;
    // 350 * stored float(1/350) is slightly below 1. Use 351 for reject.
    arf.mass=351.f;
    assert(api->antigen_radius(agf.gas,sizeof(agf.gas),gas_origin,5.f,150.f,&arb,&arr)&&arr.mass_reads==1&&arr.obstruction_rejected==1);
    arf.mode=3;
    assert(api->antigen_radius(agf.gas,sizeof(agf.gas),gas_origin,5.f,150.f,&arb,&arr)&&arr.corner_calls==1&&arr.direct_attacks==1);
    arf.wet=true;
    assert(api->antigen_radius(agf.gas,sizeof(agf.gas),gas_origin,5.f,150.f,&arb,&arr)&&arr.wet&&arr.contents_reads==2&&arr.water_rejected==1&&!arr.traces);
    arf.wet=false;EvalFixture::u(hf2.player,0x298,3);
    assert(api->antigen_radius(agf.gas,sizeof(agf.gas),gas_origin,5.f,150.f,&arb,&arr)&&arr.water_rejected==1);
    // Native gas think drives this same dispatcher instead of hosted radius.
    auto native_gas=agb;native_gas.radius_effect=nullptr;native_gas.native_radius=&arb;
    af.now=90.f;agf.now=90.f;
    assert(api->antigen_gas_create(gas_origin,2,150,-1,-1,&native_gas,&agras));
    EvalFixture::u(agf.gas,0x378,0xf000);EvalFixture::u(hf2.player,0x298,0);
    arf.mode=0;arf.strict=true;agf.now=90.125f;
    assert(api->antigen_gas_think(agf.gas,sizeof(agf.gas),&native_gas,&agras)&&!agras.radius_called);
    agf.now=90.625f;
    assert(api->antigen_gas_think(agf.gas,sizeof(agf.gas),&native_gas,&agras)&&agras.radius_result.completed&&agras.radius_result.direct_attacks==1&&agras.radius_result.last_amount==2.5f);
    // Exact antigen admission in the SiN player +d8 virtual: a pure
    // 0x400000 damage-info packet heals and rejects normal damage.
    arb.native_player_health=&ph;arb.take_damage=nullptr;arb.trace_attack=nullptr;
    EvalFixture::u(hf2.player,0,0x224aec3c);EvalFixture::u(hf2.player,0x8c,0);
    EvalFixture::u(hf2.player,0x94,2);EvalFixture::u(hf2.player,0x98,100);EvalFixture::u(hf2.player,0x9c,40);
    EvalFixture::u(hf2.player,0x12a1,0);EvalFixture::u(hf2.player,0x9d0,0xffffffffu);
    arf.mode=0;arf.strict=true;
    assert(api->antigen_radius(agf.gas,sizeof(agf.gas),gas_origin,5,150,&arb,&arr)&&arr.completed&&arr.native_admissions==1&&arr.native_heals==1&&arr.native_rejected==1&&!arr.direct_attacks&&arr.player.healing.after==45&&hf2.flags==0x400000u);
    assert(EvalFixture::rd(hf2.player,0x9d0)==0xffbfffffu);
    // Trace path invokes the same admission, skipping the virtual damage
    // leaf but preserving clear/apply multidamage and after-effect ordering.
    unsigned before_clear=arf.clear,before_apply=arf.apply;arf.mode=1;
    assert(api->antigen_radius(agf.gas,sizeof(agf.gas),gas_origin,2.75f,150,&arb,&arr)&&arr.native_heals==1&&arr.player.healing.after==47&&!arr.trace_attacks&&arf.clear==before_clear+1&&arf.apply==before_apply+1);
    EvalFixture::u(hf2.player,0x8c,1);arf.mode=0;
    assert(api->antigen_radius(agf.gas,sizeof(agf.gas),gas_origin,5,150,&arb,&arr)&&arr.native_admissions==1&&arr.player.dead&&!arr.native_heals&&EvalFixture::rd(hf2.player,0x9c)==47);
    EvalFixture::u(hf2.player,0x8c,0);EvalFixture::u(hf2.player,0,0x2245222c);
    assert(api->antigen_radius(agf.gas,sizeof(agf.gas),gas_origin,5,150,&arb,&arr)&&arr.player.base_player&&arr.native_rejected==1&&!arr.native_heals);
    EvalFixture::u(hf2.player,0,0x224aec3c);EvalFixture::u(hf2.player,0x9c,100);
    assert(api->antigen_radius(agf.gas,sizeof(agf.gas),gas_origin,5,150,&arb,&arr)&&arr.native_heals==1&&!arr.player.healing.accepted&&arr.native_rejected==1);
    // Do not accept the same flag mixed with unrelated damage or a stale
    // gas identity. Such paths must go through the original host boundary.
    unsigned char gate_info[0x48]{};
    EvalFixture::u(gate_info,0x24,0xf000);EvalFixture::u(gate_info,0x28,0xf000);EvalFixture::u(gate_info,0x38,0x400002);
    AntigenPlayerAdmissionResult apar{};
    assert(!api->antigen_player_admission(hf2.player,sizeof(hf2.player),gate_info,sizeof(gate_info),&arb,&apar));
    EvalFixture::u(gate_info,0x38,0x400000);EvalFixture::u(gate_info,0x28,0xe000);
    assert(!api->antigen_player_admission(hf2.player,sizeof(hf2.player),gate_info,sizeof(gate_info),&arb,&apar));
    // A real gas-think pulse changes HP without hosted damage or heal.
    EvalFixture::u(hf2.player,0x9c,50);agf.now=100.f;
    assert(api->antigen_gas_create(gas_origin,1,150,-1,-1,&native_gas,&agras));
    EvalFixture::u(agf.gas,0x378,0xf000);agf.now=100.125f;
    assert(api->antigen_gas_think(agf.gas,sizeof(agf.gas),&native_gas,&agras)&&!agras.radius_called);
    agf.now=100.625f;
    assert(api->antigen_gas_think(agf.gas,sizeof(agf.gas),&native_gas,&agras)&&agras.radius_result.native_heals==1&&agras.radius_result.player.healing.after==52&&agras.amount==2.5f);
    agf.now=101.125f;
    assert(api->antigen_gas_think(agf.gas,sizeof(agf.gas),&native_gas,&agras)&&agras.radius_result.native_heals==1&&agras.radius_result.player.healing.after==54&&agras.expired&&agras.removed);
    // Live strict trace and robust corner cvars; rules +b4 does not
    // admit player corner retries even when robust explosions is enabled.
    arb.strict_trace=nullptr;arb.allow_corner=nullptr;arb.engine_cvar=&cv;arb.native_singleplayer_corner=true;
    cv.calls=0;cv.roots[5].SetFloat(1.f);cv.roots[6].SetFloat(1.f);arf.mode=3;
    assert(api->antigen_radius(agf.gas,sizeof(agf.gas),gas_origin,5,150,&arb,&arr)&&arr.strict_cvar_reads==1&&arr.strict_rejected==1&&!arr.native_heals&&cv.calls==1&&cv.lookups[0]==5);
    cv.calls=0;cv.roots[5].SetFloat(0.f);
    assert(api->antigen_radius(agf.gas,sizeof(agf.gas),gas_origin,5,150,&arb,&arr)&&arr.strict_cvar_reads==1&&arr.robust_cvar_reads==1&&arr.native_corner_checks==1&&arr.corner_rejected==1&&!arr.corner_calls&&cv.calls==2&&cv.lookups[1]==6);
    // The same unnamed-context scheduler now owns gas and canister due
    // ticks; no per-think hosted schedule callback is required.
    struct AuxFrameFixture {
        GasFixture* gas;ExchangeFixture* exchange;
        unsigned flags=0,records=0,enters=0,leaves=0;
    } auxf{&agf,&ex};
    sin_independent::EntityHandleEntry aux_entries[3]{radius_entries[0],radius_entries[1],{}};
    aux_entries[2].serial=16;aux_entries[2].entity=reinterpret_cast<CBaseEntity*>(ex.canister);
    ServerAntigenPrimaryThinkBindings auxb{};auxb.context=&auxf;auxb.entities.entries=aux_entries;auxb.entities.count=3;auxb.cadence.context=&auxf;
    auxb.cadence.notify_think_flags=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<AuxFrameFixture*>(p);assert(a==q.gas->gas||a==q.exchange->canister);++q.flags;return 1;};
    auxb.clock=[](void* p,float* time) noexcept ->int{*time=static_cast<AuxFrameFixture*>(p)->gas->now;return 1;};
    auxb.record=[](void* p,void* a,int,float,int due) noexcept ->int{auto& q=*static_cast<AuxFrameFixture*>(p);assert((a==q.gas->gas||a==q.exchange->canister)&&EvalFixture::rd(a,0x74)==unsigned(due));++q.records;return 1;};
    auxb.scope=[](void* p,void* a,bool enter) noexcept ->int{auto& q=*static_cast<AuxFrameFixture*>(p);assert(a==q.gas->gas||a==q.exchange->canister);
        if(enter){assert(EvalFixture::rd(a,0x74)==0xffffffffu);++q.enters;}else ++q.leaves;return 1;};
    auto aux_gas=native_gas;aux_gas.schedule=nullptr;aux_gas.native_radius=&arb;
    auto aux_canister=canister_bind;aux_canister.schedule=nullptr;
    auxb.gas=&aux_gas;auxb.canister=&aux_canister;
    EvalFixture::u(hf2.player,0x9c,60);arf.mode=0;cv.calls=0;agf.now=110.f;
    assert(api->antigen_gas_create(gas_origin,1,150,-1,-1,&native_gas,&agras));
    EvalFixture::u(agf.gas,0x378,0xf000);EvalFixture::u(agf.gas,0x74,0xffffffffu);EvalFixture::u(agf.gas,0xb8,0x400000u);
    assert(antigen_frame->schedule_aux(agf.gas,sizeof(agf.gas),0xf000,.125f,agras.next_time,&auxb,&afr)&&afr.completed&&afr.reschedule.tick==881);
    assert(antigen_frame->run(agf.gas,sizeof(agf.gas),0xf000,880,.125f,&auxb,&afr)&&afr.not_due);
    agf.now=110.125f;
    assert(antigen_frame->run(agf.gas,sizeof(agf.gas),0xf000,881,.125f,&auxb,&afr)&&afr.completed&&afr.token==antigen_gas_think_token&&!afr.gas.radius_called&&afr.reschedule.tick==883);
    agf.now=110.375f;
    assert(antigen_frame->run(agf.gas,sizeof(agf.gas),0xf000,883,.125f,&auxb,&afr)&&!afr.gas.radius_called&&afr.reschedule.tick==885);
    agf.now=110.625f;
    assert(antigen_frame->run(agf.gas,sizeof(agf.gas),0xf000,885,.125f,&auxb,&afr)&&afr.gas.radius_result.native_heals==1&&afr.gas.radius_result.player.healing.after==62&&afr.reschedule.tick==887);
    agf.now=111.125f;
    assert(antigen_frame->run(agf.gas,sizeof(agf.gas),0xf000,889,.125f,&auxb,&afr)&&afr.gas.expired&&afr.gas.removed&&afr.gas.radius_result.player.healing.after==64&&!afr.rescheduled&&afr.last_tick==889);
    assert(antigen_frame->run(agf.gas,sizeof(agf.gas),0xf000,890,.125f,&auxb,&afr)&&afr.no_think);
    assert(!antigen_frame->run(agf.gas,sizeof(agf.gas),0xe000,890,.125f,&auxb,&afr));
    assert(api->antigen_factory_create("prop_antigen_canister",&antigen_factory,&afres));
    EvalFixture::u(ex.canister,0x378,0x10002);EvalFixture::u(ex.canister,0xb8,0x400000u);EvalFixture::u(ex.canister,0x74,0xffffffffu);
    af.now=agf.now=112.f;
    assert(api->canister_spawn(ex.canister,sizeof(ex.canister),&canister_bind,&canres));
    assert(antigen_frame->schedule_aux(ex.canister,sizeof(ex.canister),0x10002,.125f,canres.next_time,&auxb,&afr)&&afr.reschedule.tick==898);
    af.now=agf.now=112.25f;
    assert(antigen_frame->run(ex.canister,sizeof(ex.canister),0x10002,898,.125f,&auxb,&afr)&&afr.completed&&afr.token==antigen_canister_idle&&afr.canister.advanced&&afr.reschedule.tick==912&&afr.last_tick==898);
    assert(auxf.records==5&&auxf.enters==5&&auxf.leaves==5);
    FreeLibrary(s);FreeLibrary(c);return 0;
}
