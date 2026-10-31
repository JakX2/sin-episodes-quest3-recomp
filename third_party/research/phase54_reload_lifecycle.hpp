#pragma once
// Phase 54 clean-room behavioral scaffolding. NOT Ritual source or full engine logic.
#include <cstdint>
#include <vector>

namespace sin54 {
enum class Weapon { Magnum, AssaultRifle, ScatterGun };
enum class Event { ZoomExit, ReloadCleanup, ResetReloadStage, RequestHolster,
                   ProcessReloadFrame, InsertOneRound, DeductReserveOne,
                   RequestNextReloadStage, ReloadInterrupted, NoAction };
struct State {
    Weapon weapon{Weapon::Magnum};
    bool mode508{false};  // all three examined constructor paths start at zero
    bool reloadActive{false};
    bool zoomActive{false};
    int reloadStage{0};
    int clip{0},capacity{0},reserve{0};
};
inline State constructed(Weapon weapon) {
    State s{};s.weapon=weapon;s.mode508=false;return s;
}
// This only represents the evidenced *ordering*. The real cleanup method also
// adjusts attack timing, activity states and conditional stage bookkeeping.
inline std::vector<Event> holster(State& s) {
    std::vector<Event> out{Event::ZoomExit};
    s.zoomActive=false;
    if(s.reloadActive) {out.push_back(Event::ReloadCleanup);s.reloadActive=false;}
    out.push_back(Event::ResetReloadStage);s.reloadStage=0;
    out.push_back(Event::RequestHolster);
    return out;
}
// Live zoom eligibility includes more checks. In Phase 54, active reload is
// only one known blocker. A zoom command does not by itself transfer ammo.
inline bool canZoomWhenOtherGatesPass(const State& s) {return !s.reloadActive;}
inline void zoomExit(State& s) {s.zoomActive=false;}
// Abstract shared frame dispatch: returns whether it passes the clip gate.
// Actual slot 221 contains further owner, timing and state checks.
inline std::vector<Event> frameDispatch(bool clipGatePassed) {
    if(clipGatePassed)return {Event::ProcessReloadFrame};
    return {};
}
struct SingleRoundResult { bool interrupted{false};bool inserted{false}; };
// Narrow model of the +0x508 enabled branch AFTER owner/time/active guards
// pass. Aborting with attack buttons requires at least one loaded round.
inline SingleRoundResult enabledModeTick(State& s,uint32_t buttons,
                                         bool ownerValid,bool deadlinePassed) {
    if(!s.mode508||!s.reloadActive||!ownerValid||!deadlinePassed)return {};
    if((buttons&0x801u)!=0 && s.clip>0){s.reloadActive=false;return {true,false};}
    if(s.clip<s.capacity&&s.reserve>0){++s.clip;--s.reserve;return {false,true};}
    return {};
}
}
