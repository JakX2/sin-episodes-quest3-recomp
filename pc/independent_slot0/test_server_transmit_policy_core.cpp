#include "server_transmit_policy_core.h"
#include <array>
#include <cstdio>
#include <iterator>
#include <initializer_list>

class CBaseEntity { public: unsigned id=0; };
using namespace sin_independent;

struct Fixture {
    std::array<CBaseEntity,40> entities{};
    std::array<TransmitEntry,40> entries{};
    std::array<std::uint32_t,40> decisions{};
    std::array<int,40> areas{};
    std::array<bool,40> visible{};
    std::array<unsigned,40> should{}, area{}, pvs{}, sets{}, force{};
    std::array<std::uint32_t,2> selected{}, always{};
    TransmitContext ctx{};
    bool changeRecipient=false;
    bool propagate=false;
    bool ok=true;
    explicit Fixture(bool hltv) {
        for(std::size_t i=0;i<entries.size();++i) {
            entities[i].id=static_cast<unsigned>(i);
            entries[i]={kTransmitDontSend,&entities[i]};
            decisions[i]=kTransmitDontSend;
        }
        ctx={{true,hltv,17},{selected.data(),selected.size()},
             {always.data(),always.size()}};
        entries[1].flags=kTransmitDontSend|kTransmitAlways;
        entries[2].flags=kTransmitFullCheck;
        ctx.selected.Set(2);
        entries[3]={kTransmitAlways,nullptr};
        entries[4].flags=kTransmitFullCheck;
        decisions[4]=kTransmitAlways;
        entries[5].flags=kTransmitFullCheck;
        decisions[5]=kTransmitPvsCheck;
        areas[5]=17;
        entries[6].flags=kTransmitPvsCheck;
        entries[7].flags=kTransmitPvsCheck;
        visible[7]=true;
        entries[8].flags=kTransmitFullCheck;
        entries[9]={kTransmitPvsCheck,nullptr};
        entries[33]={kTransmitAlways,nullptr};
        entries[34].flags=kTransmitFullCheck;
        decisions[34]=kTransmitFullCheck;
        entries[35].flags=kTransmitPvsCheck;
        areas[35]=17;
        entries[36]={kTransmitAlways|kTransmitPvsCheck,nullptr};
        entries[37].flags=0x1000; // unrelated state flag is ignored
    }
};
static std::uint32_t Should(CBaseEntity* e,TransmitContext& c,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    ++f.should[e->id];
    if(f.changeRecipient && e->id==4) {
        // Original decision loop snapshots these before any entity callback.
        c.recipient.skyboxArea=999;
        c.recipient.hltv=!c.recipient.hltv;
    }
    return f.decisions[e->id];
}
static int Area(CBaseEntity* e,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p); ++f.area[e->id]; return f.areas[e->id];
}
static bool Pvs(CBaseEntity* e,TransmitContext&,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p); ++f.pvs[e->id]; return f.visible[e->id];
}
static void Set(CBaseEntity* e,std::size_t index,TransmitContext& c,bool force,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);
    if(e->id!=index || c.selected.Test(index))f.ok=false;
    ++f.sets[e->id]; if(force)++f.force[e->id];
    c.selected.Set(index);
    if(force && f.propagate && index==4)c.selected.Set(5);
}
static TransmitHooks Hooks(Fixture& f) {return {&Should,&Area,&Pvs,&Set,&f};}
static int fail(int n,const char* msg) {
    std::fprintf(stderr,"FAIL transmit policy %d: %s\n",n,msg);return n;
}
int main() {
    const std::uint16_t list[]={1,2,3,4,5,6,7,8,9,33,34,35,36,37,4,33,7};
    for(bool hltv:{false,true}) {
        Fixture f(hltv);
        if(CheckTransmit(f.entries.data(),f.entries.size(),list,std::size(list),
                         f.ctx,Hooks(f))!=TransmitResult::Completed || !f.ok)
            return fail(1,"normal input");
        for(unsigned i=0;i<40;++i) {
            const bool expected=i==2||i==3||i==4||i==5||i==7||i==33||i==35||i==36||(hltv&&i==6);
            if(f.ctx.selected.Test(i)!=expected)
                return fail(2,"selected bits");
            // Only direct ALWAYS paths set the always bit directly.
            if(f.ctx.always.Test(i)!=(hltv&&(i==3||i==33||i==36)))
                return fail(3,"always bits");
            const unsigned should=i==4||i==5||i==8||i==34||i==37;
            const unsigned area=i==5||i==6||i==7||i==35;
            const unsigned pvs=!hltv&&(i==6||i==7);
            const unsigned sets=i==4||i==5||i==7||i==35||(hltv&&i==6);
            const unsigned force=i==4||i==5||i==35;
            if(f.should[i]!=should||f.area[i]!=area||f.pvs[i]!=pvs||
               f.sets[i]!=sets||f.force[i]!=force)
                return fail(4,"exact callback dispatch");
        }
    }
    Fixture snapshot(false);
    snapshot.changeRecipient=true;
    if(CheckTransmit(snapshot.entries.data(),snapshot.entries.size(),list,std::size(list),
                     snapshot.ctx,Hooks(snapshot))!=TransmitResult::Completed ||
       !snapshot.sets[5] || !snapshot.force[5] || snapshot.ctx.selected.Test(6) ||
       !snapshot.pvs[6] || !snapshot.pvs[7])
        return fail(5,"recipient snapshot");
    Fixture propagated(false);
    propagated.propagate=true;
    if(CheckTransmit(propagated.entries.data(),propagated.entries.size(),list,std::size(list),
                     propagated.ctx,Hooks(propagated))!=TransmitResult::Completed ||
       !propagated.ctx.selected.Test(5) || propagated.should[5] || propagated.sets[5])
        return fail(6,"subsequent candidate observes propagation");

    Fixture malformed(true);
    const std::uint16_t bad[]={3,40};
    auto before=malformed.selected;
    if(CheckTransmit(malformed.entries.data(),malformed.entries.size(),bad,2,
          malformed.ctx,Hooks(malformed))!=TransmitResult::InvalidInput ||
       malformed.selected!=before || malformed.always[0] || malformed.always[1])
        return fail(7,"preflight before mutation");
    malformed.ctx.always.wordCount=1;
    if(CheckTransmit(malformed.entries.data(),malformed.entries.size(),list,std::size(list),
          malformed.ctx,Hooks(malformed))!=TransmitResult::InvalidInput ||
       malformed.selected!=before)
        return fail(8,"short HLTV bitset");
    malformed.ctx.recipient.valid=false;
    if(CheckTransmit(malformed.entries.data(),malformed.entries.size(),list,std::size(list),
          malformed.ctx,Hooks(malformed))!=TransmitResult::MissingRecipient)
        return fail(9,"missing recipient");
    malformed.ctx.recipient.valid=true;
    if(CheckTransmit(nullptr,0,nullptr,0,malformed.ctx,{})!=TransmitResult::Completed)
        return fail(10,"empty work");
    std::puts("PASS transmit policy: direct flags, ShouldTransmit, skybox/PVS/HLTV, duplicates, propagation and bounded bitsets");
    return 0;
}
