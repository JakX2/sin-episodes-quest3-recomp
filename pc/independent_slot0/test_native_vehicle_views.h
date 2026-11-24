#pragma once
#include "native_vehicle_views.h"
#include <cassert>
#include <cstring>
#include <initializer_list>
namespace sin_port::native_weapon_tables::test {
struct VehicleViewFixture {
    alignas(4) unsigned char vehicle[0x900]{},player[0x1230]{},views[400]{};
    HologramControl v{vehicle,sizeof(vehicle)},p{player,sizeof(player)};
    unsigned events[32]{},n=0;
    bool cast=true,found=true;
    int last_attachment=-1;
    void put(void* p,unsigned off,unsigned word){std::memcpy(static_cast<unsigned char*>(p)+off,&word,4);}
    unsigned word(void* p,unsigned off){unsigned w=0;std::memcpy(&w,static_cast<unsigned char*>(p)+off,4);return w;}
    void push(unsigned e){assert(v.busy&&n<32);events[n++]=e;}
    void expect(std::initializer_list<unsigned> e){assert(e.size()==n&&!std::memcmp(events,e.begin(),n*4));n=0;}
    void view(int i,int parent,int seat,int attachment,unsigned char ret,unsigned char allow){
        auto* r=views+i*100;put(r,0x40,parent);put(r,0x44,seat);put(r,0x48,attachment);r[0x54]=ret;r[0x55]=allow;
    }
};
inline void check_vehicle_views(const VehicleViewsApi& api){
    VehicleViewFixture f;
    f.put(f.vehicle,0x7fc,static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.views)));f.put(f.vehicle,0x808,4);
    f.view(0,-1,0,10,0,1);f.view(1,0,0,11,1,3);f.view(2,1,1,12,1,2);f.view(3,-1,1,13,0,7);
    f.put(f.player,0x9b0,0xffffffffu);f.put(f.player,0x16c,0x1000);
    VehicleViewBindings b{};b.context=&f;
    b.cast_player=[](void* c,void* a,HologramControl** out) noexcept -> int{auto& f=*static_cast<VehicleViewFixture*>(c);assert(a==f.player);f.push(1);*out=f.cast?&f.p:nullptr;return 1;};
    b.dirty_edict=[](void* c,void* p,std::uint32_t edict) noexcept -> int{auto& f=*static_cast<VehicleViewFixture*>(c);assert(p==f.player&&f.p.busy&&edict==0x1000);f.push(2);return 1;};
    b.output=[](void* c,void* v,void* p,unsigned off,int index) noexcept -> int{auto& f=*static_cast<VehicleViewFixture*>(c);assert(v==f.vehicle&&p==f.player&&f.p.busy&&off==0x7e4&&f.word(p,0x9b0)==static_cast<unsigned>(index));f.push(3);return 1;};
    b.attachment=[](void* c,void* v,int index,float pos[3],unsigned* found) noexcept -> int{auto& f=*static_cast<VehicleViewFixture*>(c);assert(v==f.vehicle&&f.p.busy);f.push(4);f.last_attachment=index;pos[0]=1;pos[1]=2;pos[2]=3;*found=f.found;return 1;};
    b.set_position=[](void* c,void* p,const float pos[3]) noexcept -> int{auto& f=*static_cast<VehicleViewFixture*>(c);assert(p==f.player&&f.p.busy&&pos[0]==1&&pos[1]==2&&pos[2]==3);f.push(5);return 1;};
    assert(api.seat_default(&f.v,&f.p,&b,1)==1&&f.word(f.player,0x9b0)==3&&f.player[0x9b4]==7);f.expect({2});
    assert(api.seat_default(&f.v,&f.p,&b,9)==1);f.expect({});
    f.player[0x1f0]=1;
    assert(api.set_view(&f.v,&b,f.player,5,2)==1&&f.word(f.player,0x9b0)==2&&f.player[0x9b4]==2&&f.player[0x200]==1&&f.last_attachment==12);f.expect({1,3,4,5});
    f.player[0x1f0]=0;
    assert(api.return_view(&f.v,&f.p,&b)==1&&f.word(f.player,0x9b0)==0&&!f.player[0x9b4]&&f.last_attachment==10);f.expect({4,2,3,5});
    assert(api.return_view(&f.v,&f.p,&b)==1);f.expect({});
    float pos[]={1,2,3};
    assert(api.directional(&f.v,&f.p,&b,1,pos)==1&&f.word(f.player,0x9b0)==1&&f.player[0x9b4]==3);f.expect({2,3,5});
    assert(api.directional(&f.v,&f.p,&b,1,pos)==1);f.expect({});
    f.found=false;assert(api.return_view(&f.v,&f.p,&b)==1&&f.word(f.player,0x9b0)==1);f.expect({4});
    assert(api.set_view(&f.v,&b,f.player,2,999)==1&&f.word(f.player,0x9b0)==0&&f.player[0x9b4]==1);f.expect({1,2,3,4});
    assert(api.set_view(&f.v,&b,f.player,5,0)==1);f.expect({1,3,4});
    assert(api.set_view(&f.v,&b,f.player,5,-1)==1);f.expect({1});
    f.cast=false;assert(api.set_view(&f.v,&b,f.player,5,3)==1);f.expect({1});
    f.cast=true;f.found=true;f.view(1,2,0,11,1,3);f.view(2,1,1,12,1,2);
    assert(api.directional(&f.v,&f.p,&b,1,pos)==1);f.expect({2,3,5});
    assert(api.return_view(&f.v,&f.p,&b)==-1&&f.v.blocked&&f.p.blocked&&!f.v.busy&&!f.p.busy);f.expect({});
}
}
