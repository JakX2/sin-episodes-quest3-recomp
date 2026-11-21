#include "sin_compat/original_grunt_action_translation_phase584.hpp"
#include <cassert>
#include <cmath>
#include <string>
#include <vector>
using namespace sin_compat::phase584;
int main() {
    {
        int iterations=0;std::vector<std::string> log;
        TranslationHost h;
        h.virtual_33c=[&](int v,bool& f){f=false;log.push_back("33c:"+std::to_string(v));return v;};
        h.virtual_338=[&](int v){++iterations;return v;};
        h.original_220eb8a0_available=[](){return true;};
        h.original_220eebd0_lookup=[&](int v){log.push_back("lookup:"+std::to_string(v));return v==7?42:-1;};
        h.original_2202e600_check=[](int){return false;};
        assert(ResolveRequest(7,h)==7);assert(iterations==1);assert(log.back()=="lookup:7");
        // Native 15 bypasses the availability test and schedule lookup.
        int checks=0;h.original_220eb8a0_available=[&](){++checks;return true;};
        assert(ResolveRequest(15,h)==15);assert(checks==0);
    }
    {
        TranslationHost h;
        int count=0;h.virtual_33c=[&](int v,bool& f){f=false;return v==6?6:v;};
        h.virtual_338=[&](int v){++count;return v;};
        h.original_220eb8a0_available=[](){return false;};
        h.original_220eebd0_lookup=[](int){return -1;};
        h.original_2202e600_check=[](int){return false;};
        assert(ResolveRequest(10,h)==6);assert(ResolveRequest(6,h)==10);
        assert(ResolveRequest(5,h)==5);assert(count==3);
    }
    {
        std::vector<int> looked;LookupHost h;
        h.original_22139a10_entity=[](){return std::optional<int>{};};
        h.entity_virtual_24=[](int){assert(false);return -1;};
        h.entity_virtual_28=[](int){assert(false);return 0;};
        h.original_220eb1c0_lookup=[&](int m,int q,int current){looked={m,q,current};return 55;};
        assert(LookupAction(8,19,h)==55);assert((looked==std::vector<int>{0,19,8}));
        h.original_22139a10_entity=[](){return std::optional<int>{17};};
        h.entity_virtual_24=[](int e){assert(e==17);return 3;};
        h.entity_virtual_28=[](int e){assert(e==17);return 99;};
        assert(LookupAction(10,21,h)==55);assert((looked==std::vector<int>{99,21,10}));
        h.entity_virtual_24=[](int){return 2;};
        h.entity_virtual_28=[](int){assert(false);return 0;};
        assert(LookupAction(-1,21,h)==55);assert(looked[0]==0);
    }
    {
        TransitionState s{10,-1,4,false};std::vector<std::string> l;
        TransitionHost h;
        h.virtual_4e0=[&](int v){l.push_back("allows"+std::to_string(v));return v==10;};
        h.original_220405c0_reset=[&](int v){assert(v==0);l.push_back("reset");};
        h.original_220f2300_set=[&](int v){l.push_back("set"+std::to_string(v));};
        h.original_220eede0_float=[&](int v){l.push_back("float");return float(v)*0.5f;};
        h.original_222a5470_use=[&](int context,float x){assert(context==77 && std::fabs(x-3.f)<0.01f);l.push_back("use");};
        h.virtual_630_with_out_context=[&](int id){assert(s.translated_87c==id && (id==33||id==4));l.push_back("v630");return 42;};
        h.original_22140bd0=[&](int id){assert(id==42);l.push_back("dispatch");};
        h.virtual_4e4=[&](int v){assert(v==11 && s.current_request_878==10);l.push_back("changed");};
        h.nav_virtual_4c=[&](){assert(s.current_request_878==11);l.push_back("nav");};
        ApplyTransition(s,11,6,33,77,h);
        assert((l==std::vector<std::string>{"allows10","allows11","reset","set6","float","use","v630","dispatch","changed","nav"}));
        assert(s.current_request_878==11 && s.translated_87c==33);
        l.clear();s.selected_action_3ec=6;s.selected_action_active_3e1=true;
        ApplyTransition(s,11,6,4,77,h);assert(l.front()=="set6");
    }
    {
        TransitionState s{3,-1,-1,false};int checks=0,clear=0;
        TransitionHost h;
        h.virtual_4e0=[&](int){++checks;return false;};
        h.original_220405c0_reset=[](int){assert(false);};
        h.original_220f2300_set=[&](int id){assert(id==0);++clear;};
        h.original_220eede0_float=[](int){assert(false);return 0.f;};
        h.original_222a5470_use=[](int,float){assert(false);};
        h.virtual_630_with_out_context=[](int){return 1;};
        h.original_22140bd0=[](int){};
        h.virtual_4e4=[](int){};
        h.nav_virtual_4c=[](){};
        ApplyTransition(s,4,-1,-1,0,h);assert(checks==0 && clear==1 && s.current_request_878==4);
    }
}