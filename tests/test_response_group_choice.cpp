#include "../pc/independent_slot0/response_group_choice.h"
#include <cassert>
using namespace sin_port::response_rule;
struct Fixture {
    unsigned char group[group_bytes]{},rows[3*response_row_bytes]{};
    unsigned calls=0,draws=0,reject_mask=0;float draw_fraction=0;
    int scratch[6]{},selected=-2;
    static Fixture& self(void* p){return *static_cast<Fixture*>(p);}
    unsigned char* row(unsigned i){return rows+i*response_row_bytes;}
    Fixture(){
        group_put(group,0xc,3);group[0x3c]=1;group_put(group,0x34,1u);
        for(unsigned i=0;i<3;++i){group_put(row(i),0,1u);group_put(row(i),4,i);group_put(row(i),8,1.f);}
    }
    GroupChoice bindings(bool filtering=true){
        GroupChoice h{};h.context=this;
        if(filtering)h.filter=[](void* p,unsigned type,std::uint32_t id,unsigned* yes) noexcept{
            auto& f=self(p);assert(type==1&&id<3);++f.calls;*yes=!(f.reject_mask&(1u<<id));return true;};
        h.random=[](void* p,float lo,float hi,float* v) noexcept{
            auto& f=self(p);assert(lo==0);++f.draws;*v=hi*f.draw_fraction;return true;};
        return h;
    }
    int run(bool filter=true){
        assert(choose_group(group,sizeof(group),rows,sizeof(rows),bindings(filter),scratch,6,selected));return selected;
    }
};
int main(){
    {
        Fixture f;f.group[0x3a]=1;
        assert(f.run(false)==0&&f.run(false)==1&&f.run(false)==2&&f.run(false)==0&&f.run(false)==0);
        f.group[0x3b]=1;group_put(f.group,0x40,3u);
        assert(f.run()==-1&&!f.group[0x3c]);
        assert(reset_group(f.group,sizeof(f.group),f.rows,sizeof(f.rows))&&f.group[0x3c]&&f.run()==0);
    }
    {
        Fixture f;f.group[0x3a]=1;f.reject_mask=7;
        assert(f.run()==-1&&f.calls==4);
    }
    {
        Fixture f;f.draw_fraction=1;
        assert(f.run(false)==0&&f.draws==3); // equality never replaces the earlier choice
        f.draw_fraction=0;assert(f.run(false)==2);
        f.reject_mask=4;assert(f.run()==1);
        for(unsigned i=0;i<3;++i)group_put(f.row(i),8,0.f);
        f.reject_mask=0;const auto draws=f.draws;assert(f.run()==2&&f.draws==draws);
    }
    {
        Fixture f;f.group[0x30]=1;f.row(0)[0x10]=1;f.row(2)[0x11]=1;
        assert(f.run(false)==0&&!f.draws);
        assert(f.run(false)==1&&f.draws==1);
        assert(f.run(false)==2&&f.draws==1);
        assert(f.run(false)==0&&group_get<unsigned>(f.group,0x34)==2);
    }
    {
        Fixture f;f.group[0x30]=1;f.reject_mask=2;
        assert(f.run()==2&&f.calls==3&&group_get<unsigned>(f.row(1),0xc)==0);
        f.reject_mask=7;
        assert(f.run()==-1&&group_get<unsigned>(f.group,0x34)==2);
        for(unsigned i=0;i<3;++i)assert(group_get<unsigned>(f.row(i),0xc)==2);
    }
    {
        Fixture f;f.group[0x30]=1;f.group[0x3b]=1;
        for(unsigned i=0;i<3;++i)group_put(f.row(i),0xc,1u);
        assert(f.run(false)==-1&&!f.group[0x3c]&&group_get<unsigned>(f.group,0x34)==2);
        assert(reset_group(f.group,sizeof(f.group),f.rows,sizeof(f.rows)));
        assert(f.run(false)==2);
    }
}
