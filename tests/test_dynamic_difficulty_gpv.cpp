#include "sin_compat/dynamic_difficulty_gpv.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
using namespace sin_compat;
int main() {
    GameplayDifficultyVariables g;
    assert(g.current_challenge({"A"}).status == GameplayDifficultyStatus::InvalidTable);
    assert(g.interpolate(0) == GameplayDifficultyStatus::InvalidTable);
    // Completely synthetic variable names and values.
    std::vector<GameplayVariableDefinition> def={
        {"A", 5,0,10,0,10}, {"B",1.0f,1.5f,0.5f,1.5f,0.5f},
        {"C",100,0,10,0,10}, {"D",-1,0,1,0,1}};
    std::string error;
    assert(g.configure(def,&error) && error.empty());
    assert(g.count()==4);
    assert(g.find("A") && g.find("A")->current==5);
    assert(!g.find("a")); // Case-sensitive original name lookup.
    auto r=g.current_challenge({"A","B"});
    assert(r && r.rating_percent==50.0f && r.recognized_entries==2);
    r=g.current_challenge({"C","D"});
    assert(r && r.rating_percent==50.0f); // clamp 1.0 and 0.0
    r=g.current_challenge({"A","MISSING"});
    assert(r && r.rating_percent==25.0f && r.entries_visited==2 && r.recognized_entries==1);
    r=g.current_challenge({"MISSING"});
    assert(r && r.rating_percent==0.0f && r.recognized_entries==0);
    r=g.current_challenge({});
    assert(r && r.rating_percent==0.0f && r.entries_visited==0);
    r=g.current_challenge({"A","A","B"});
    assert(r && r.rating_percent==50.0f);
    assert(g.interpolate(0.0f)==GameplayDifficultyStatus::Ok);
    assert(g.find("A")->current==0 && g.find("B")->current==1.5f);
    assert(g.interpolate(1.0f)==GameplayDifficultyStatus::Ok);
    assert(g.find("A")->current==10 && g.find("B")->current==0.5f);
    assert(g.interpolate(0.5f)==GameplayDifficultyStatus::Ok);
    assert(g.find("A")->current==5 && g.find("B")->current==1.0f);
    r=g.current_challenge({"A","B"});
    assert(r && r.rating_percent==50.0f);
    assert(g.interpolate(2.0f)==GameplayDifficultyStatus::Ok);
    assert(g.find("A")->current==20 && g.find("B")->current==-0.5f);
    assert(g.current_challenge({"A","B"}).rating_percent==100.0f);
    assert(g.interpolate(std::numeric_limits<float>::quiet_NaN())==GameplayDifficultyStatus::InvalidSlider);
    assert(g.interpolate(std::numeric_limits<float>::infinity())==GameplayDifficultyStatus::InvalidSlider);
    assert(g.find("A")->current==20);
    assert(!g.configure({{"A",0,0,1,1,1}},&error) && error.find("zero-range")!=std::string::npos);
    assert(g.count()==0);
    assert(!g.configure({{"A",0,0,1,0,1},{"A",0,0,1,0,1}},&error));
    assert(g.configure({{"E",0,0,1,0,1}},&error));
    assert(g.find("E"));
    assert(!g.configure({{"E",NAN,0,1,0,1}},&error));
    assert(!g.configure({},&error));
    return 0;
}
