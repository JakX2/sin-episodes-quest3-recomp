#include "sin_compat/original_grunt_signal_route_phase583.hpp"
#include <cassert>
#include <string>
#include <vector>
using namespace sin_compat::original_grunt_signal_route_phase583;
struct Host : Hooks {
    std::vector<std::string> calls;
    bool available=true;
    int32_t translateAction(int32_t code,int32_t context) override {
        calls.push_back("translate:"+std::to_string(code)+":"+std::to_string(context));
        return code;
    }
    int32_t lookupSchedule(int32_t mapped) override {
        calls.push_back("lookup:"+std::to_string(mapped));
        if (mapped==10) return -1;
        if (mapped==6) return 26;
        if (mapped==4) return -1;
        return mapped+10;
    }
    int32_t special15() override {calls.push_back("special15");return 77;}
    void onUnmapped(int32_t code) override {calls.push_back("unmapped:"+std::to_string(code));}
    bool isActionRoutingAvailable() override {calls.push_back("available");return available;}
    void onTransition(int32_t request,int32_t schedule,int32_t mapped,int32_t context) override {
        calls.push_back("transition:"+std::to_string(request)+":"+
                std::to_string(schedule)+":"+std::to_string(mapped)+":"+std::to_string(context));
    }
};
int main(){
    {
        State s;Host h;s.raw_0x88C_context=11;
        mapAction(s,10,h);
        assert(s.raw_0x888_mapped==6 && s.raw_0x884_schedule==26);
        assert((h.calls==std::vector<std::string>{"translate:10:11","lookup:10","unmapped:10","lookup:6"}));
    }
    {
        State s;Host h;mapAction(s,15,h);
        assert(s.raw_0x888_mapped==15 && s.raw_0x884_schedule==77);
        assert((h.calls==std::vector<std::string>{"translate:15:0","special15"}));
    }
    {
        State s;Host h;mapAction(s,4,h);
        assert(s.raw_0x884_schedule==0 && s.raw_0x888_mapped==4);
        assert((h.calls==std::vector<std::string>{"translate:4:0","lookup:4","unmapped:4"}));
    }
    {
        State s;Host h;h.available=false;applyRoutedAction(s,3,h);
        assert(s.raw_0x880_request==-1 && h.calls==std::vector<std::string>{"available"});
    }
    {
        State s;Host h;s.raw_0x878_current=3;applyRoutedAction(s,3,h);
        assert(s.raw_0x880_request==-1 && h.calls==std::vector<std::string>{"available"});
    }
    {
        State s;Host h;s.raw_0x878_current=2;applyRoutedAction(s,25,h);
        assert(s.raw_0x880_request==-1 && h.calls==std::vector<std::string>{"available"});
    }
    {
        State s;Host h;s.raw_0x878_current=2;s.raw_0x880_request=0x9A;s.raw_0x88C_context=42;
        applyRoutedAction(s,25,h);
        assert(s.raw_0x880_request==25 && s.raw_0x884_schedule==35 && s.raw_0x888_mapped==25);
        assert((h.calls==std::vector<std::string>{"available","translate:25:42","lookup:25","transition:25:35:25:42"}));
    }
    {
        State s;Host h;s.raw_0x878_current=2;applyRoutedAction(s,0,h);
        assert(s.raw_0x880_request==0 && s.raw_0x884_schedule==10);
        assert((h.calls==std::vector<std::string>{"available","translate:0:0","lookup:0","transition:0:10:0:0"}));
    }
    return 0;
}