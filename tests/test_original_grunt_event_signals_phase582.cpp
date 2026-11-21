#include "sin_compat/original_grunt_event_signals_phase582.hpp"
#include <cassert>
#include <cmath>
#include <string>
#include <vector>
using namespace sin_compat::original_grunt_event_signals_phase582;
struct Signals : SignalHooks {
    std::vector<std::string> calls;
    bool canRoute=true;
    void debugSignal(int32_t code) override { calls.push_back("debug:"+std::to_string(code)); }
    void ownerVirtual4DC(int32_t arg) override { calls.push_back("virtual:"+std::to_string(arg)); }
    bool canRouteAction() override {calls.push_back("query");return canRoute;}
    void routeAction(int32_t code) override {calls.push_back("route:"+std::to_string(code));}
};
struct Events : EventHooks {
    std::vector<std::string> calls;
    float out5to10=6.5f;
    float out0toHalf=0.3f;
    float out2to5=3.0f;
    int32_t outInt=45;
    float lastAngle=-1.0f;
    float lastShaping=-1.0f;
    int32_t randomInt(int32_t lo,int32_t hi) override { calls.push_back("int:"+std::to_string(lo)+":"+std::to_string(hi)); return outInt; }
    float randomFloat(float lo,float hi) override {
        calls.push_back("float:"+std::to_string(lo)+":"+std::to_string(hi));
        if (lo==0.1f && hi==0.5f) return out0toHalf;
        if (lo==2.0f && hi==5.0f) return out2to5;
        if (lo==5.0f && hi==10.0f) return out5to10;
        assert(false && "unexpected original RNG bounds");
        return 0.0f;
    }
    void eventCall(uint32_t va,int32_t first,int32_t second) override {
        calls.push_back("call:"+std::to_string(va)+":"+std::to_string(first)+":"+std::to_string(second));
    }
    void originalDirectionalCall(float angle,float shape,float scale) override {
        calls.push_back("vector");lastAngle=angle;lastShaping=shape; assert(scale==220.0f);
    }
};
int main() {
    {
        State s;Signals h; s.raw_0xA9=0x10;
        numericSignal(s,2,true,h);
        assert(h.calls.empty() && s.raw_0x880==-1);
        numericSignal(s,0,true,h);
        assert((h.calls==std::vector<std::string>{"debug:0","virtual:0"}));
        assert(s.raw_0x880==-1);
    }
    {
        State s;Signals h; s.raw_0xA9=0x10;
        numericSignal(s,0x9A,true,h);
        assert(s.raw_0x880==0x9A);
        assert((h.calls==std::vector<std::string>{"debug:154"}));
        numericSignal(s,0x19,true,h);
        assert(s.raw_0x880==0x19);
        assert((h.calls==std::vector<std::string>{"debug:154","debug:25","query","route:25"}));
    }
    {
        State s;Signals h;h.canRoute=false;
        numericSignal(s,0x1E,false,h);
        assert(s.raw_0x880==0x1E);
        assert((h.calls==std::vector<std::string>{"query"}));
    }
    {
        assert(std::fabs(quantizeDegreesHost(45.0f)-45.0f)<0.01f);
        assert(std::fabs(quantizeDegreesHost(-90.0f)-270.0f)<0.02f);
        assert(std::fabs(quantizeDegreesHost(360.0f))<0.01f);
        bool threw=false;try{(void)quantizeDegreesHost(std::numeric_limits<float>::infinity());}
        catch(const std::domain_error&){threw=true;}assert(threw);
        threw=false;try{(void)quantizeDegreesHost(100000000.0f);}
        catch(const std::out_of_range&){threw=true;}assert(threw);
    }
    {
        State s;Events e;
        onEvent92(s,15.0f,e);
        assert(s.raw_0xC20==21.5f);
        assert((e.calls==std::vector<std::string>{
            "call:571787056:1024:0","call:570630688:0:0",
            "call:570622896:4:0","call:570622864:1:0",
            "call:571749904:3:0","float:5.000000:10.000000"}));
    }
    {
        State s;Events e;Signals h;e.outInt=45;
        onEvent8D(s,20.0f,e,h,false);
        assert(s.raw_0x880==0x19 && s.raw_0xC3C==0);
        assert(s.raw_0xC38==23.0f);
        assert(std::fabs(e.lastAngle-45.0f)<0.01f && e.lastShaping==0.3f);
        assert((h.calls==std::vector<std::string>{"query","route:25"}));
        assert((e.calls==std::vector<std::string>{"int:-180:180","float:0.100000:0.500000","vector","float:2.000000:5.000000"}));
    }
    return 0;
}