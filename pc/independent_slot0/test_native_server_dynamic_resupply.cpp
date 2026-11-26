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
    FreeLibrary(s);FreeLibrary(c);return 0;
}
