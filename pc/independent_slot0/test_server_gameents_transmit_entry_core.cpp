#include "server_gameents_transmit_entry_core.h"
#include <array>
#include <cstdio>
#include <string>
#include <cstdint>

class CBaseEntity { public: unsigned id=0; };
using namespace sin_independent;
struct F {
    std::array<CBaseEntity,8> entities{};
    std::array<TransmitEntry,8> entries{};
    std::array<std::uint32_t,1> selected{},always{};
    std::array<unsigned,8> should{},areas{},pvs{},set{},force{};
    std::string calls;
    bool valid=true,hltv=false,fail_table=false,fail_recipient=false,
         fail_bits=false,short_bits=false,change_recipient=false;
    int recipient_cookie=123;
    F() {
        for(unsigned i=0;i<entries.size();++i) {
            entities[i].id=i;
            entries[i]={kTransmitDontSend,&entities[i]};
        }
        entries[1]={kTransmitAlways,nullptr};
        entries[2]={kTransmitFullCheck,&entities[2]};
        entries[3]={kTransmitPvsCheck,&entities[3]};
        entries[4]={kTransmitPvsCheck,&entities[4]};
        entries[5]={kTransmitFullCheck,&entities[5]};
    }
    void Reset() {
        selected.fill(0);always.fill(0);
        should.fill(0);areas.fill(0);pvs.fill(0);set.fill(0);force.fill(0);
        calls.clear();
        valid=true;
    }
};
static bool Table(void* p,const TransmitEntry** out,std::size_t* size) noexcept {
    auto& f=*static_cast<F*>(p);f.calls+='T';
    if(f.fail_table)return false;
    *out=f.entries.data();*size=f.entries.size();return true;
}
static bool Recipient(void* p,const void* info,TransmitRecipient* out) noexcept {
    auto& f=*static_cast<F*>(p);f.calls+='R';
    if(f.fail_recipient)return false;
    if(info!=&f.recipient_cookie){f.valid=false;return false;}
    *out={f.valid,f.hltv,7};
    return true;
}
static bool Bitsets(void* p,const void* info,TransmitBits* selected,TransmitBits* always) noexcept {
    auto& f=*static_cast<F*>(p);f.calls+='B';
    if(info!=&f.recipient_cookie){f.valid=false;return false;}
    if(f.fail_bits)return false;
    *selected={f.selected.data(),f.short_bits?0u:f.selected.size()};
    *always={f.always.data(),f.short_bits?0u:f.always.size()};
    return true;
}
static std::uint32_t Should(CBaseEntity* e,TransmitContext& ctx,void* p) noexcept {
    auto& f=*static_cast<F*>(p);f.calls+='W';++f.should[e->id];
    if(f.change_recipient && e->id==2) {
        ctx.recipient.skyboxArea=999;
        ctx.recipient.hltv=!ctx.recipient.hltv;
    }
    return e->id==2?kTransmitAlways:kTransmitDontSend;
}
static int Area(CBaseEntity* e,void* p) noexcept {
    auto& f=*static_cast<F*>(p);f.calls+='A';++f.areas[e->id];
    return e->id==3?7:9;
}
static bool Pvs(CBaseEntity* e,TransmitContext&,void* p) noexcept {
    auto& f=*static_cast<F*>(p);f.calls+='V';++f.pvs[e->id];
    return e->id==4;
}
static void Set(CBaseEntity* e,std::size_t id,TransmitContext& ctx,bool forced,void* p) noexcept {
    auto& f=*static_cast<F*>(p);f.calls+='S';
    if(e->id!=id||ctx.selected.Test(id))f.valid=false;
    ++f.set[e->id];
    if(forced)++f.force[e->id];
    ctx.selected.Set(id);
}
static GameEntsTransmitEntryHooks Hooks(F& f) noexcept {
    return {&f,&Table,&Recipient,&Bitsets,
            {&Should,&Area,&Pvs,&Set,&f}};
}
static int fail(int n,const char* why) {
    std::fprintf(stderr,"FAIL original GameEnts transmit entry %d: %s\n",n,why);
    return n;
}
int main() {
    F f;GameEntsTransmitEntryResult out{};
    const std::uint16_t candidates[]{0,1,2,3,4,5,1,2};
    auto run=[&](const std::uint16_t* list,int count) {
        return CheckTransmitEntry(&f.recipient_cookie,list,count,
            count>0?static_cast<std::size_t>(count):0u,Hooks(f),out);
    };
    if(run(candidates,8)!=GameEntsTransmitEntryStatus::Completed ||
       out.table_calls!=1||out.recipient_calls!=1||out.bitset_calls!=1||
       out.candidates!=8||out.policy_calls!=1||
       out.snapshot.skyboxArea!=7||out.snapshot.hltv||!f.valid ||
       f.calls.substr(0,3)!="TRB"||
       f.selected[0]!=((1u<<1)|(1u<<2)|(1u<<3)|(1u<<4))||
       f.always[0]||f.should[2]!=1||f.should[5]!=1||
       f.areas[3]!=1||f.areas[4]!=1||
       f.pvs[4]!=1||f.set[2]!=1||f.set[3]!=1||f.set[4]!=1||
       f.force[2]!=1||f.force[3]!=1||f.force[4]!=0)
        return fail(1,"real-policy composition with non-HLTV recipient");
    f.Reset();f.hltv=true;
    if(run(candidates,8)!=GameEntsTransmitEntryStatus::Completed ||
       !f.valid||f.always[0]!=(1u<<1)||f.pvs[4]||
       f.set[4]!=1||f.selected[0]!=(0x1eu)||
       !out.snapshot.hltv)
        return fail(2,"HLTV direct-always bit and area mismatch");
    f.Reset();f.hltv=false;f.change_recipient=true;
    if(run(candidates,8)!=GameEntsTransmitEntryStatus::Completed ||
       !f.valid||f.pvs[4]!=1||f.set[3]!=1||f.force[3]!=1||
       f.always[0]!=0||out.snapshot.skyboxArea!=7)
        return fail(3,"recipient snapshot independent of policy callback mutation");
    f.change_recipient=false;f.Reset();
    if(run(nullptr,0)!=GameEntsTransmitEntryStatus::Completed ||
       !out.empty||out.policy_calls||out.bitset_calls||f.calls!="TR"||
       f.selected[0])
        return fail(4,"zero count skips bitsets and policy");
    f.Reset();
    if(run(nullptr,-1)!=GameEntsTransmitEntryStatus::Completed||
       !out.empty||f.calls!="TR")
        return fail(5,"negative original signed count skips loop");
    f.Reset();f.fail_table=true;
    if(run(candidates,8)!=GameEntsTransmitEntryStatus::HostFailure ||
       f.calls!="T"||out.recipient_calls)
        return fail(6,"owner table query failure before recipient");
    f.fail_table=false;f.Reset();f.fail_recipient=true;
    if(run(candidates,8)!=GameEntsTransmitEntryStatus::HostFailure ||
       f.calls!="TR"||out.bitset_calls)
        return fail(7,"recipient resolver failure before bitset");
    f.fail_recipient=false;f.Reset();f.valid=false;
    if(run(candidates,8)!=GameEntsTransmitEntryStatus::NoRecipient ||
       f.calls!="TR"||out.bitset_calls)
        return fail(8,"missing recipient exits without candidate iteration");
    f.Reset();f.fail_bits=true;
    if(run(candidates,8)!=GameEntsTransmitEntryStatus::HostFailure ||
       f.calls!="TRB"||out.policy_calls||f.selected[0])
        return fail(9,"bitset provider failure stops candidate loop");
    f.fail_bits=false;f.Reset();f.short_bits=true;
    if(run(candidates,8)!=GameEntsTransmitEntryStatus::InvalidInput ||
       !out.policy_calls||out.policy_status!=TransmitResult::InvalidInput ||
       f.selected[0]||f.always[0]||f.calls!="TRB")
        return fail(10,"preflight rejects short selected/always bitsets");
    f.short_bits=false;f.Reset();
    const std::uint16_t bad[]{1,8,2};
    if(run(bad,3)!=GameEntsTransmitEntryStatus::InvalidInput ||
       !out.policy_calls||f.selected[0]||f.always[0]||
       f.calls!="TRB")
        return fail(11,"out-of-range index rejects before first mutation");
    f.Reset();
    if(run(nullptr,1)!=GameEntsTransmitEntryStatus::InvalidInput ||
       out.bitset_calls||f.calls!="TR")
        return fail(12,"missing nonzero work list");
    f.Reset();
    // A caller can supply a larger count than the actual array; reject
    // this before any index dereference, bitset access or engine callback.
    if(CheckTransmitEntry(&f.recipient_cookie,candidates,4,3u,Hooks(f),out)
          !=GameEntsTransmitEntryStatus::InvalidInput ||
       out.bitset_calls||f.calls!="TR"||
       f.selected[0]||f.always[0])
        return fail(14,"undersized physical candidate index buffer");
    auto missing=Hooks(f);missing.table=nullptr;
    f.Reset();
    if(CheckTransmitEntry(&f.recipient_cookie,candidates,1,1u,missing,out)
          !=GameEntsTransmitEntryStatus::InvalidInput||!f.calls.empty())
        return fail(13,"missing global manager callback");
    std::puts("PASS ServerGameEnts CheckTransmit entry: table, recipient, bounded bitsets, policy integration and failure order");
    return 0;
}
