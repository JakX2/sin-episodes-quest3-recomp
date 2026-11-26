#include "native_server_dynamic_resupply.h"
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
    assert(api->activate(lf.actor,sizeof(lf.actor),&lb,&lr)&&lr.master_assigned&&master_handle==0x3000u&&lr.warning_called);
    master_handle=0x4001u;assert(api->activate(lf.actor,sizeof(lf.actor),&lb,&lr)&&lr.master_retained&&!lr.master_assigned&&lr.warning_called);
    master_handle=0x5001u;lf.now=1.f;assert(api->activate(lf.actor,sizeof(lf.actor),&lb,&lr)&&lr.master_assigned&&!lr.warning_called);
    LifeFixture::wr(lf.actor,0xb4,1);master_handle=0xffffffffu;lf.now=.5f;
    assert(api->activate(lf.actor,sizeof(lf.actor),&lb,&lr)&&!lr.master_assigned&&!lr.warning_called);
    master_handle=0x4001u;
    assert(api->become_master(lf.actor,sizeof(lf.actor),&lb,&lr)&&lr.master_assigned&&lr.think_cleared&&master_handle==0x3000u&&!LifeFixture::rd(lf.actor,8));
    LifeFixture::wr(lf.actor,8,123);
    assert(api->kill(lf.actor,sizeof(lf.actor),&lb,&lr)&&lr.removed&&!lr.think_cleared&&LifeFixture::rd(lf.actor,8)==123&&master_handle==0x3000u);
    assert(!api->spawn(lf.actor,sizeof(lf.actor)-1,&lb,&lr)&&!lr.completed);
    FreeLibrary(s);FreeLibrary(c);return 0;
}
