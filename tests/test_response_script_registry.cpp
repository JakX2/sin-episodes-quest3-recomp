#include "../pc/independent_slot0/response_script_registry.h"
#include <cassert>
using namespace sin_port::response_script;
struct Fixture {
    Registry r;unsigned draws=0;
    static Fixture& self(void* p){return *static_cast<Fixture*>(p);}
    rr::Scoring scoring(){
        rr::Scoring s{};s.context=this;
        s.criterion=[](void* p,unsigned i,rr::Bytes* out) noexcept{return self(p).r.row(0x60,rr::criterion_stride,rr::criterion_bytes,i,*out);};
        s.rule=[](void* p,unsigned i,rr::Bytes* out) noexcept{return self(p).r.row(0xb8,rr::rule_stride,rr::rule_bytes,i,*out);};
        s.storage=[](void* p,std::uint32_t w,rr::Bytes* out) noexcept{return self(p).r.storage(w,*out);};
        s.string=[](void* p,std::uint32_t w,const char** out) noexcept{return self(p).r.string(w,*out);};
        s.lookup=[](void*,const char* key,const char** out,float* w) noexcept{
            *w=1;*out="";
            if(rr::ascii_equal(key,"concept")){*out="TLK_IDLE";*w=5;}
            else if(rr::ascii_equal(key,"npcstate"))*out="[state::idle]";
            return true;};
        s.draw=[](void* p,int,int,int* out) noexcept{++self(p).draws;*out=0;return true;};
        s.enumeration={&r,Registry::enum_lookup};return s;
    }
};
int main(){
    Parser p;
    assert(p.parse("root",R"(enumeration state { idle 1 }
criterion concept concept TLK_IDLE required
criterion idle npcstate "[state::idle]" required
response fallback speak generic
response specific
{
 sequential
 scene "scenes/example.vcd" delay 3
 speak backup
}
rule general
{
 criteria concept
 response fallback
}
rule best
{
 criteria concept idle
 response specific
 matchonce
}
)"));
    Fixture f;assert(f.r.build(p.db));auto s=f.scoring();
    int ties[2]{},pick=-1;assert(rr::best_rule(s,2,ties,2,pick)&&pick==1&&!f.draws);
    rr::Bytes rule{},group{},entries{},groups{};
    assert(f.r.row(0xb8,rr::rule_stride,rr::rule_bytes,1,rule)&&rule.p[0x28]);
    assert(f.r.storage(rule.get<unsigned>(0x14),groups)&&groups.get<unsigned>(0)==1);
    assert(f.r.row(8,rr::group_stride,rr::group_bytes,1,group)&&group.p[0x3a]);
    assert(f.r.storage(group.get<unsigned>(0),entries));
    assert(group.get<float>(0x1c)==3.f&&group.get<unsigned>(0x14)==1);
    int selected=-1;
    assert(rr::choose_group(const_cast<unsigned char*>(group.p),group.n,const_cast<unsigned char*>(entries.p),entries.n,{},nullptr,0,selected)&&selected==0);
    const char* value=nullptr;
    assert(f.r.string(entries.get<unsigned>(4),value)&&!std::strcmp(value,"scenes/example.vcd"));
    assert(rr::choose_group(const_cast<unsigned char*>(group.p),group.n,const_cast<unsigned char*>(entries.p),entries.n,{},nullptr,0,selected)&&selected==1);
    const_cast<unsigned char*>(rule.p)[0x29]=0;
    assert(rr::best_rule(s,2,ties,2,pick)&&pick==0);
    p.db.rules[0].groups[0]=999;
    assert(!f.r.build(p.db)&&f.r.group_names.size()==2);
}
