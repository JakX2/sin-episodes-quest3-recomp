#include "../pc/independent_slot0/response_rule_scoring.h"
#include <cassert>
#include <array>
#include <limits>
using namespace sin_port::response_rule;
struct Fixture {
    unsigned char criteria[8][criterion_bytes]{},rules[6][rule_bytes]{};
    std::uint32_t vectors[8][4]{};
    unsigned lookups=0,draws=0;int pick=1;
    bool null_actual=false;
    static Fixture& self(void* p){return *static_cast<Fixture*>(p);}
    template<class T> static void put(unsigned char* p,unsigned off,T v){std::memcpy(p+off,&v,sizeof(v));}
    void leaf(unsigned i,unsigned name,const char* expr,float w,bool req){
        auto* c=criteria[i];put(c,0,name);put(c,8,w);c[0xc]=req;
        assert(compile(expr,{},c+0x10,matcher_bytes));
    }
    void composite(unsigned i,unsigned v,unsigned count,float w,bool req){
        auto* c=criteria[i];put(c,8,w);c[0xc]=req;put(c,0x124,v+1);put(c,0x130,count);
    }
    void rule(unsigned i,unsigned v,unsigned count,bool enabled=true){
        put(rules[i],0,v+1);put(rules[i],0xc,count);rules[i][0x29]=enabled;
    }
    Scoring bindings(){
        Scoring s{};s.context=this;
        s.criterion=[](void* p,unsigned i,Bytes* b) noexcept{if(i>=8)return false;*b={self(p).criteria[i],criterion_bytes};return true;};
        s.rule=[](void* p,unsigned i,Bytes* b) noexcept{if(i>=6)return false;*b={self(p).rules[i],rule_bytes};return true;};
        s.storage=[](void* p,std::uint32_t word,Bytes* b) noexcept{if(!word||word>8)return false;*b={reinterpret_cast<unsigned char*>(self(p).vectors[word-1]),16};return true;};
        s.string=[](void*,std::uint32_t word,const char** out) noexcept{switch(word){case 1:*out="concept";break;case 2:*out="enemy";break;case 3:*out="missing";break;default:return false;}return true;};
        s.lookup=[](void* p,const char* key,const char** actual,float* weight) noexcept{
            auto& f=self(p);++f.lookups;*weight=1;
            if(!std::strcmp(key,"concept")){*actual="TLK_IDLE";*weight=5;}
            else if(!std::strcmp(key,"enemy")){*actual=f.null_actual?nullptr:"none";}
            else *actual="";
            return true;};
        s.draw=[](void* p,int lo,int hi,int* out) noexcept{auto& f=self(p);assert(lo==0&&hi==1);++f.draws;*out=f.pick;return true;};
        return s;
    }
};
int main(){
    Fixture f;auto s=f.bindings();
    f.leaf(0,1,"TLK_IDLE",2,true);f.leaf(1,2,"npc_sintekgrunt",7,true);
    f.leaf(2,3,"!absent",3,false);f.leaf(3,2,"none",-10,false);
    f.vectors[0][0]=0;f.vectors[0][1]=1;f.composite(4,0,2,0.5f,true);
    f.vectors[1][0]=0;f.vectors[1][1]=3;f.composite(5,1,2,1,true);
    Score x{};assert(score_criterion(s,0,x)&&x.value==10&&!x.required_failed);
    assert(score_criterion(s,1,x)&&x.value==0&&x.required_failed);
    assert(score_criterion(s,2,x)&&x.value==3&&!x.required_failed);
    assert(score_criterion(s,4,x)&&x.value==5&&!x.required_failed);
    assert(score_criterion(s,5,x)&&x.value==0&&x.required_failed);
    f.null_actual=true;assert(score_criterion(s,1,x)&&!x.required_failed&&x.value==0);f.null_actual=false;
    f.vectors[2][0]=0;f.vectors[2][1]=1;f.rule(0,2,2);
    f.vectors[3][0]=4;f.rule(1,3,1);f.rule(2,3,1);
    f.vectors[4][0]=0;f.rule(3,4,1,false);
    f.vectors[5][0]=5;f.rule(4,5,1);
    f.vectors[6][0]=2;f.rule(5,6,1);
    float v=99;assert(score_rule(s,0,v)&&v==0);
    assert(score_rule(s,3,v)&&v==0);
    assert(score_rule(s,4,v)&&v==0);
    int scratch[6]{},best=99;
    assert(best_rule(s,6,scratch,6,best)&&best==2&&f.draws==1);
    f.pick=0;assert(best_rule(s,6,scratch,6,best)&&best==1&&f.draws==2);
    f.rules[3][0x29]=1;assert(best_rule(s,6,scratch,6,best)&&best==3&&f.draws==2);
    assert(best_rule(s,1,scratch,6,best)&&best==-1);
    f.leaf(6,2,"none",0.001f,false);f.vectors[7][0]=6;f.rule(0,7,1);
    assert(best_rule(s,1,scratch,6,best)&&best==0);
    f.leaf(6,2,"none",std::numeric_limits<float>::quiet_NaN(),false);
    assert(best_rule(s,1,scratch,6,best)&&best==-1);
    f.vectors[7][0]=7;f.composite(7,7,1,1,false);
    assert(!score_criterion(s,7,x));
    assert(!best_rule(s,6,scratch,5,best));
}
