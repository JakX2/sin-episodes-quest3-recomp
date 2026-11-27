#include "server_gameents_transmit_entry_core.h"
#include <array>
#include <cstdio>
#include <string>

class CBaseEntity { public: unsigned id=0; };
using namespace sin_independent;
static void Put32(unsigned char* p,std::uint32_t v) noexcept {
    for(unsigned i=0;i<4;++i)p[i]=static_cast<unsigned char>(v>>(8*i));
}
struct Fixture {
    std::array<unsigned char,40*20> records{},fallback{};
    std::array<unsigned char,0x2010> info{};
    std::array<CBaseEntity,40> entities{};
    std::array<std::uint32_t,2> selected{},always{};
    std::string calls;
    unsigned queries=0,recipient_calls=0,bitset_calls=0,entity_calls=0;
    unsigned should_calls=0,area_calls=0,pvs_calls=0,set_calls=0;
    bool valid=true,hltv=false,missing_recipient=false,missing_fallback=false;
    bool fail_table=false,fail_recipient=false,fail_bits=false,fail_entity=false;
    bool short_table=false,short_bits=false,propagate=false,change_recipient=false;
    Fixture(){
        for(unsigned i=0;i<40;++i){
            entities[i].id=i;
            Record(i,kTransmitDontSend,1000+i);
            Put32(fallback.data()+i*20,kTransmitDontSend);
        }
        // Unresolvable tokens must not be resolved on direct/skip paths.
        Record(1,kTransmitDontSend|kTransmitAlways,99999);
        Record(2,kTransmitFullCheck,99999);
        selected[0]|=1u<<2;
        Record(3,kTransmitAlways,99999);
        Record(4,kTransmitFullCheck,1004);
        Record(5,kTransmitPvsCheck,1005);
        Record(6,kTransmitPvsCheck,1006);
        Record(7,kTransmitPvsCheck,1007);
        Record(8,kTransmitPvsCheck,0);
        Record(33,kTransmitAlways,99999);
        Put32(info.data(),91);
        Put32(info.data()+0x2008,201);
        Put32(info.data()+0x200c,202);
    }
    void Record(unsigned i,std::uint32_t flags,std::uint32_t token) noexcept {
        Put32(records.data()+i*20,flags);
        Put32(records.data()+i*20+0xc,token);
        // Poison otherwise-unused dwords to catch accidental field selection.
        Put32(records.data()+i*20+4,0x12345678);
        Put32(records.data()+i*20+8,0x87654321);
        Put32(records.data()+i*20+16,0xffffffff);
    }
};
static bool Table(void* p,int arg,GameEntsTransmitTable* out) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='T';++f.queries;
    if(arg!=0)f.valid=false;
    if(f.fail_table)return false;
    if(f.queries==1)*out={f.records.data(),f.records.size()-(f.short_table?1:0),90};
    else *out={f.fallback.data(),f.fallback.size(),f.missing_fallback?0u:92u};
    return true;
}
static bool Recipient(void* p,std::uint32_t token,TransmitRecipient* out) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='R';++f.recipient_calls;
    if(token!=(f.queries==1?91u:92u))f.valid=false;
    if(f.fail_recipient)return false;
    *out={!f.missing_recipient,f.hltv,17};return true;
}
static bool Bits(void* p,std::uint32_t token,TransmitBits* out) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='B';++f.bitset_calls;
    if(f.fail_bits)return false;
    if(token==201)*out={f.selected.data(),f.short_bits?1u:f.selected.size()};
    else if(token==202)*out={f.always.data(),f.always.size()};
    else {f.valid=false;return false;}
    return true;
}
static bool Entity(void* p,std::uint32_t token,CBaseEntity** out) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='E';++f.entity_calls;
    if(token<1000||token>=1040){f.valid=false;return false;}
    if(f.fail_entity)return false;
    *out=&f.entities[token-1000];return true;
}
static std::uint32_t Should(CBaseEntity* e,TransmitContext& ctx,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='S';++f.should_calls;
    if(e->id!=4)f.valid=false;
    if(f.change_recipient){ctx.recipient.skyboxArea=999;ctx.recipient.hltv=!ctx.recipient.hltv;}
    return kTransmitAlways;
}
static int Area(CBaseEntity* e,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='A';++f.area_calls;
    return e->id==5?17:99;
}
static bool Pvs(CBaseEntity* e,TransmitContext&,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='P';++f.pvs_calls;
    return e->id==7;
}
static void Set(CBaseEntity* e,std::size_t i,TransmitContext& ctx,bool force,void* p) noexcept {
    auto& f=*static_cast<Fixture*>(p);f.calls+='X';++f.set_calls;
    if(e->id!=i||ctx.selected.Test(i)||force!=(i==4||i==5))f.valid=false;
    ctx.selected.Set(i);
    if(f.propagate&&i==4)ctx.selected.Set(5);
}
static GameEntsTransmitEntryHooks Hooks(Fixture& f) noexcept {
    return {&f,&Table,&Recipient,&Bits,&Entity,{&Should,&Area,&Pvs,&Set,&f}};
}
static int Fail(int n,const char* why) {
    std::fprintf(stderr,"FAIL GameEnts CheckTransmit entry %d: %s\n",n,why);return n;
}
int main() {
    const std::uint16_t ids[]{1,2,3,4,5,6,7,8,33,3,4,7};
    constexpr auto count=sizeof(ids)/sizeof(*ids);
    GameEntsTransmitEntryResult out{};
    auto run=[&](Fixture& f,int n=static_cast<int>(count)){
        return CheckTransmitEntry(f.info.data(),f.info.size(),ids,count,n,Hooks(f),out);
    };
    Fixture direct;
    if(run(direct)!=TransmitResult::Completed||!direct.valid||
       direct.calls!="TRBESXEAXEAPEAPX"||
       out.table_calls!=1||out.recipient_calls!=1||out.bitset_calls!=1||out.entity_calls!=4||
       out.fallback||direct.selected[0]!=((1u<<2)|(1u<<3)|(1u<<4)|(1u<<5)|(1u<<7))||
       direct.selected[1]!=2u||direct.always[0]||direct.always[1])
        return Fail(1,"original entry stride/flags, lazy entity lookup, PVS, repeated indices");
    Fixture fallback;
    Put32(fallback.info.data(),0);
    if(run(fallback)!=TransmitResult::Completed||!fallback.valid||
       fallback.calls!="TTRBESXEAXEAPEAPX"||!out.fallback||out.table_calls!=2||
       fallback.selected!=direct.selected)
        return Fail(2,"fallback edict comes from second query, candidates remain first table");
    Fixture special;special.hltv=true;
    if(run(special)!=TransmitResult::Completed||!special.valid||
       special.calls!="TRBBESXEAXEAXEAX"||out.bitset_calls!=2||special.pvs_calls||
       !out.policy_context.selected.Test(6)||special.always[0]!=(1u<<3)||special.always[1]!=2u)
        return Fail(3,"special recipient always bits and visibility bypass");
    Fixture snapshot;snapshot.change_recipient=true;snapshot.propagate=true;
    if(run(snapshot)!=TransmitResult::Completed||!snapshot.valid||
       snapshot.calls!="TRBESXEAPEAPX"||snapshot.area_calls!=2||snapshot.pvs_calls!=2||
       !out.policy_context.selected.Test(5)||out.policy_context.selected.Test(6))
        return Fail(4,"recipient snapshot and earlier selection propagation");
    Fixture empty;
    if(run(empty,0)!=TransmitResult::Completed||empty.calls!="TR"||
       out.bitset_calls||out.entity_calls)
        return Fail(5,"zero count after recipient snapshot");
    Fixture negative;
    if(CheckTransmitEntry(negative.info.data(),4,nullptr,0,-1,Hooks(negative),out)
       !=TransmitResult::Completed||negative.calls!="TR")
        return Fail(6,"signed negative count does not access bitsets or candidates");
    Fixture missing;Put32(missing.info.data(),0);missing.missing_fallback=true;
    if(run(missing)!=TransmitResult::MissingRecipient||missing.calls!="TT"||
       out.recipient_calls||out.bitset_calls)
        return Fail(7,"null fallback recipient");
    Fixture no_actor;no_actor.missing_recipient=true;
    if(run(no_actor)!=TransmitResult::MissingRecipient||no_actor.calls!="TR")
        return Fail(8,"unresolved recipient actor");
    Fixture short_table;short_table.short_table=true;
    if(run(short_table)!=TransmitResult::InvalidInput||short_table.calls!="TR")
        return Fail(9,"truncated 20-byte record");
    Fixture short_bits;short_bits.short_bits=true;
    const auto before=short_bits.selected;
    if(run(short_bits)!=TransmitResult::InvalidInput||short_bits.calls!="TRB"||
       short_bits.selected!=before||out.entity_calls)
        return Fail(10,"whole-index preflight prevents partial selection");
    Fixture excess;
    if(run(excess,static_cast<int>(count+1))!=TransmitResult::InvalidInput||excess.calls!="TR")
        return Fail(11,"signed count larger than supplied index span");
    Fixture failed;failed.fail_entity=true;
    if(run(failed)!=TransmitResult::HostFailure||failed.calls!="TRBE"||
       !out.policy_context.selected.Test(3)||out.policy_context.selected.Test(4))
        return Fail(12,"owner failure preserves already performed direct selection");
    Fixture failed_bits;failed_bits.fail_bits=true;
    if(run(failed_bits)!=TransmitResult::HostFailure||failed_bits.calls!="TRB"||
       out.entity_calls)
        return Fail(13,"bitset provider failure");
    Fixture failed_table;failed_table.fail_table=true;
    if(run(failed_table)!=TransmitResult::HostFailure||failed_table.calls!="T")
        return Fail(14,"table provider failure");
    Fixture failed_recipient;failed_recipient.fail_recipient=true;
    if(run(failed_recipient)!=TransmitResult::HostFailure||failed_recipient.calls!="TR")
        return Fail(15,"recipient provider failure");
    Fixture short_info;
    if(CheckTransmitEntry(short_info.info.data(),0x200b,ids,count,1,Hooks(short_info),out)
       !=TransmitResult::InvalidInput||short_info.calls!="TR")
        return Fail(16,"short info buffer rejected before bitset read");
    std::puts("PASS GameEnts original CheckTransmit entry: two table queries, bounded PE32 records, recipient snapshot, shared policy and owner failures");
    return 0;
}
