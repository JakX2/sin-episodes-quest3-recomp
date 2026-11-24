#pragma once
#include "native_weapon_datatables.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
struct Fixture {
    alignas(4) unsigned char bytes[0x4f4]{};
    n::OwnerObservation owner{};
};
inline void* __cdecl local(const s::SendProp*,const void* structure,const void* data,s::Recipients* out,int){
    const auto* f=static_cast<const Fixture*>(structure);
    if(!f||!out)return nullptr;
    for(auto& word:out->words)word=0;
    if(f->owner.resolved&&f->owner.player_eligible&&f->owner.client_slot>=1&&f->owner.client_slot<=256){
        const auto i=f->owner.client_slot-1;out->words[i/32]=1u<<(i%32);
    }
    return const_cast<void*>(data);
}
// The test host callback resolves an actual fixture observation instead
// of pretending the original engine's global EHANDLE table exists.
inline void __cdecl owner(const s::SendProp*,const void* structure,const void* data,s::DVariant* out,int,int){
    const auto* f=static_cast<const Fixture*>(structure);
    std::uint32_t value=0;std::memcpy(&value,data,4);
    assert(value==f->owner.native_handle);
    out->words[0]=f->owner.resolved?((value&0x7ffu)|(((value>>12)&0x3ffu)<<11)):n::wire_invalid_handle;
}
inline void check(const SendApi& sa,const RecvApi& ra,const n::Api& contract,const n::Values& values){
    s::SendProp dummy;
    s::SendTable sb;assert(sb.Construct(&dummy,1,"DT_BaseAnimating"));
    RecvProp rdummy;RecvTable rb{&rdummy,1,nullptr,"DT_BaseAnimating"};
    SendView sv{};RecvView rv{};
    SendBindings bindings{&sb,&local,&local,&owner};
    RecvBindings rec{&rb};
    auto bad=bindings;bad.local_active=nullptr;
    assert(!sa.create(&bad,&sv)&&!sv.handle);
    bad=bindings;bad.base_animating=nullptr;
    assert(!sa.create(&bad,&sv)&&!sv.handle);
    assert(!ra.create(nullptr,&rv)&&!rv.handle);
    assert(sa.create(&bindings,&sv)&&ra.create(&rec,&rv));
    assert(!sa.create(&bindings,&sv)&&!ra.create(&rec,&rv));
    const unsigned parent[]={BaseCombat,BaseHL,MachineGun,BaseHL,SelectFire,BaseHL};
    for(unsigned i=0;i<TableCount;++i){
        const auto& a=*sv.tables[i];const auto& b=*rv.tables[i];
        assert(std::strcmp(a.name,table_names[i])==0&&std::strcmp(b.name,a.name)==0);
        assert(a.count==table_counts[i]&&b.count==a.count);
        if(i>=BaseHL){
            assert(a.props[0].table==sv.tables[parent[i-BaseHL]]&&b.props[0].table==rv.tables[parent[i-BaseHL]]);
            assert(a.props[0].flags==0x200&&a.props[0].offset==0&&b.props[0].offset==0);
        }
    }
    assert(sv.tables[BaseCombat]->props[0].table==&sb&&rv.tables[BaseCombat]->props[0].table==&rb);
    assert(sv.tables[BaseCombat]->props[1].table==sv.tables[LocalWeapon]&&
        sv.tables[BaseCombat]->props[2].table==sv.tables[LocalActive]);
    assert(rv.tables[BaseCombat]->props[1].table==rv.tables[LocalWeapon]&&
        rv.tables[BaseCombat]->props[2].table==rv.tables[LocalActive]);
    Fixture f;f.owner={values.words[n::Owner],1,1,7};
    alignas(4) unsigned char client[0x7e8]{};
    for(unsigned i=0;i<n::field_count;++i){
        n::FieldInfo info{};assert(contract.field(i,&info));
        auto& send=sv.tables[info.group]->props[info.slot];auto& recv=rv.tables[info.group]->props[info.slot];
        assert(std::strcmp(send.name,info.name)==0&&std::strcmp(recv.name,info.name)==0);
        assert(send.offset==static_cast<int>(info.server_offset)&&recv.offset==static_cast<int>(info.client_offset));
        assert(send.flags==static_cast<int>(info.flags)&&send.bits==static_cast<int>(info.bits)&&recv.flags==0);
        assert(send.elements==1&&recv.elements==1&&send.scalarProxy&&recv.proxy);
        std::memcpy(f.bytes+send.offset,&values.words[i],4);
        s::DVariant variant{};
        send.scalarProxy(&send,&f,f.bytes+send.offset,&variant,0,1);
        // Raw proxy result BEFORE bitstream truncation; all valid test
        // inputs decode correctly without a fake Source wire packet.
        if(i==n::Clip2)assert(variant.words[0]==0);
        RecvData received{&recv,variant,0,1};
        recv.proxy(&received,client,client+recv.offset);
        std::uint32_t result=0;std::memcpy(&result,client+recv.offset,4);
        assert(result==(i==n::Owner?((0x345u<<12)|37u):values.words[i]));
    }
    s::Recipients mask{};
    auto& localProp=sv.tables[BaseCombat]->props[1];
    assert(localProp.tableProxy(&localProp,&f,&f,&mask,1)==&f&&mask.words[0]==64);
    f.owner.player_eligible=0;
    assert(localProp.tableProxy(&localProp,&f,&f,&mask,1)==&f&&mask.words[0]==0);
    void* dest=nullptr;
    auto& receiveBase=rv.tables[BaseCombat]->props[0];
    receiveBase.table_proxy(&receiveBase,&dest,client,1);assert(dest==client);
    sv.tables[BaseCombat]->initialized=true;
    rv.tables[BaseCombat]->decoder=&f;
    assert(!sa.destroy(&sv)&&!ra.destroy(&rv));
    sv.tables[BaseCombat]->initialized=false;rv.tables[BaseCombat]->decoder=nullptr;
    assert(sa.destroy(&sv)&&ra.destroy(&rv)&&!sv.handle&&!rv.handle);
    assert(!sa.destroy(&sv)&&!ra.destroy(&rv));
}
// Native MSVC x86 vtable fixtures with original slots 63 and 72.
// The object field +0x16c is checked before invoking a DLL callback.
class NativeOwner {
public:
    virtual void slot0(){assert(false);}
    virtual void slot1(){assert(false);}
    virtual void slot2(){assert(false);}
    virtual void slot3(){assert(false);}
    virtual void slot4(){assert(false);}
    virtual void slot5(){assert(false);}
    virtual void slot6(){assert(false);}
    virtual void slot7(){assert(false);}
    virtual void slot8(){assert(false);}
    virtual void slot9(){assert(false);}
    virtual void slot10(){assert(false);}
    virtual void slot11(){assert(false);}
    virtual void slot12(){assert(false);}
    virtual void slot13(){assert(false);}
    virtual void slot14(){assert(false);}
    virtual void slot15(){assert(false);}
    virtual void slot16(){assert(false);}
    virtual void slot17(){assert(false);}
    virtual void slot18(){assert(false);}
    virtual void slot19(){assert(false);}
    virtual void slot20(){assert(false);}
    virtual void slot21(){assert(false);}
    virtual void slot22(){assert(false);}
    virtual void slot23(){assert(false);}
    virtual void slot24(){assert(false);}
    virtual void slot25(){assert(false);}
    virtual void slot26(){assert(false);}
    virtual void slot27(){assert(false);}
    virtual void slot28(){assert(false);}
    virtual void slot29(){assert(false);}
    virtual void slot30(){assert(false);}
    virtual void slot31(){assert(false);}
    virtual void slot32(){assert(false);}
    virtual void slot33(){assert(false);}
    virtual void slot34(){assert(false);}
    virtual void slot35(){assert(false);}
    virtual void slot36(){assert(false);}
    virtual void slot37(){assert(false);}
    virtual void slot38(){assert(false);}
    virtual void slot39(){assert(false);}
    virtual void slot40(){assert(false);}
    virtual void slot41(){assert(false);}
    virtual void slot42(){assert(false);}
    virtual void slot43(){assert(false);}
    virtual void slot44(){assert(false);}
    virtual void slot45(){assert(false);}
    virtual void slot46(){assert(false);}
    virtual void slot47(){assert(false);}
    virtual void slot48(){assert(false);}
    virtual void* animating(){return animating_object;}
    virtual void slot50(){assert(false);}
    virtual void slot51(){assert(false);}
    virtual void slot52(){assert(false);}
    virtual void slot53(){assert(false);}
    virtual void slot54(){assert(false);}
    virtual void slot55(){assert(false);}
    virtual void slot56(){assert(false);}
    virtual void slot57(){assert(false);}
    virtual void slot58(){assert(false);}
    virtual void slot59(){assert(false);}
    virtual void slot60(){assert(false);}
    virtual void slot61(){assert(false);}
    virtual void slot62(){assert(false);}
    virtual void* base(){++base_calls;return resolved;}
    virtual void slot64(){assert(false);}
    virtual void slot65(){assert(false);}
    virtual void slot66(){assert(false);}
    virtual void slot67(){assert(false);}
    virtual void slot68(){assert(false);}
    virtual void slot69(){assert(false);}
    virtual void slot70(){assert(false);}
    virtual void slot71(){assert(false);}
    virtual bool is_player(){++player_calls;return player;}
    virtual void slot73(){assert(false);}
    virtual void slot74(){assert(false);}
    virtual void slot75(){assert(false);}
    virtual void* indirect(){++indirect_calls;return indirect_object;}
    unsigned char pad[0x168]{};
    const void* edict=nullptr;
    void* resolved=nullptr;
    bool player=false;
    unsigned base_calls=0,player_calls=0,indirect_calls=0;
    void* indirect_object=nullptr;
    void* animating_object=nullptr;
};
class NativeEngine {
public:
    virtual void slot0(){assert(false);}
    virtual void slot1(){assert(false);}
    virtual void slot2(){assert(false);}
    virtual void slot3(){assert(false);}
    virtual void slot4(){assert(false);}
    virtual void slot5(){assert(false);}
    virtual void slot6(){assert(false);}
    virtual void slot7(){assert(false);}
    virtual void slot8(){assert(false);}
    virtual void slot9(){assert(false);}
    virtual void slot10(){assert(false);}
    virtual void slot11(){assert(false);}
    virtual void slot12(){assert(false);}
    virtual void slot13(){assert(false);}
    virtual void slot14(){assert(false);}
    virtual void slot15(){assert(false);}
    virtual void slot16(){assert(false);}
    virtual void slot17(){assert(false);}
    virtual int index(const void* edict){
        ++calls;
        assert(edict==expected);
        if(throws)throw 1;
        return result;
    }
    const void* expected=nullptr;
    unsigned calls=0;
    int result=0;
    bool throws=false;
};
inline void check_resolved(const ResolvedSendApi& api){
    s::SendProp dummy;s::SendTable base;assert(base.Construct(&dummy,1,"DT_BaseAnimating"));
    NativeOwner handle,player;
    assert(reinterpret_cast<const unsigned char*>(&player.edict)-reinterpret_cast<const unsigned char*>(&player)==0x16c);
    alignas(4) unsigned char edicts[257*20]{};
    NativeEngine engine;engine.expected=edicts+256*20;engine.result=256;
    handle.resolved=&player;player.player=true;player.edict=engine.expected;
    sin_independent::EntityHandleEntry entries[64]{};
    entries[37].entity=reinterpret_cast<CBaseEntity*>(&handle);
    entries[37].serial=0x12345;
    const std::uint32_t raw=(0x12345u<<12)|37u;
    alignas(4) unsigned char weapon[0x4f4]{};
    std::memcpy(weapon+0x494,&raw,4);
    ResolvedSendBindings b{&base,&engine,entries,64,edicts,256};
    SendView view{};
    auto bad=b;bad.max_clients=257;assert(!api.create(&bad,&view)&&!view.handle);
    bad=b;bad.engine_server=reinterpret_cast<void*>(1);assert(!api.create(&bad,&view));
    bad=b;bad.entry_count=4097;assert(!api.create(&bad,&view));
    assert(api.create(&b,&view));
    auto& local=view.tables[BaseCombat]->props[1];
    auto& active=view.tables[BaseCombat]->props[2];
    auto& owner=view.tables[BaseCombat]->props[6];
    s::Recipients recipients{};
    const auto mask_empty=[&](){for(auto word:recipients.words)assert(!word);};
    const auto invoke=[&](s::SendProp& p){return p.tableProxy(&p,nullptr,weapon,&recipients,1);};
    const auto packed=[&](){s::DVariant v{};owner.scalarProxy(&owner,weapon,weapon+0x494,&v,0,1);return v.words[0];};
    assert(invoke(local)==weapon&&recipients.words[7]==0x80000000u);
    for(unsigned i=0;i<7;++i)assert(!recipients.words[i]);
    assert(handle.base_calls==1&&player.player_calls==1&&engine.calls==1);
    assert(invoke(active)==weapon&&engine.calls==2);
    assert(packed()==((0x345u<<11)|37u)&&engine.calls==2);
    // Serial reuse must null the public handle and both local tables.
    ++entries[37].serial;
    assert(!invoke(local));mask_empty();
    assert(packed()==n::wire_invalid_handle&&handle.base_calls==2);
    --entries[37].serial;
    player.player=false;
    assert(!invoke(active));mask_empty();
    assert(packed()==((0x345u<<11)|37u)&&engine.calls==2);
    player.player=true;handle.resolved=nullptr;
    assert(!invoke(local));mask_empty();
    handle.resolved=&player;
    player.edict=edicts+1;assert(!invoke(local)&&engine.calls==2);mask_empty();
    player.edict=edicts;assert(!invoke(local)&&engine.calls==2);
    player.edict=engine.expected;engine.result=255;
    assert(!invoke(local)&&engine.calls==3);mask_empty();
    engine.result=256;engine.throws=true;
    assert(!invoke(local)&&engine.calls==4);
    engine.throws=false;assert(invoke(local)==weapon&&engine.calls==5);
    entries[37].entity=nullptr;
    assert(!invoke(local)&&packed()==n::wire_invalid_handle&&engine.calls==5);
    assert(api.destroy(&view));
}

class NativeIndirect {
public:
    virtual void slot0(){assert(false);}
    virtual void slot1(){assert(false);}
    virtual void* resolve(int n){assert(n==0);++calls;return target;}
    void* target=nullptr;
    unsigned calls=0;
};
inline void check_inventory_graph(const InventorySendApi& sa,const InventoryRecvApi& ra){
    s::SendProp sdummy;s::SendTable sb;assert(sb.Construct(&sdummy,1,"DT_BaseFlex"));
    RecvProp rdummy;RecvTable rb{&rdummy,1,nullptr,"DT_BaseFlex"};
    alignas(4) unsigned char edicts[9*20]{};
    NativeEngine engine;engine.expected=edicts+7*20;engine.result=7;
    NativeOwner player,npc;player.player=true;player.edict=engine.expected;
    NativeIndirect indirect;indirect.target=&player;npc.indirect_object=&indirect;
    sin_independent::EntityHandleEntry entries[64]{};
    entries[37].entity=reinterpret_cast<CBaseEntity*>(&npc);entries[37].serial=0x12345;
    InventorySendBindings bindings{&sb,&engine,entries,64,edicts,8};
    InventoryRecvBindings rec{&rb};
    InventorySendView sv{};InventoryRecvView rv{};
    assert(sa.create(&bindings,&sv)&&ra.create(&rec,&rv));
    assert(sv.ammo_table->count==32&&rv.ammo_table->count==32&&
        sv.weapons_table->count==48&&rv.weapons_table->count==48);
    assert(sv.combat_local->count==2&&rv.combat_local->count==2&&sv.combat_base->count==3&&rv.combat_base->count==3);
    assert(std::strcmp(sv.combat_base->name,"DT_BaseCombatCharacter")==0&&
        std::strcmp(rv.combat_local->name,"DT_BCCLocalPlayerExclusive")==0);
    assert(sv.combat_base->props[0].table==&sb&&rv.combat_base->props[0].table==&rb);
    assert(sv.combat_base->props[1].table==sv.combat_local&&rv.combat_base->props[1].table==rv.combat_local);
    assert(sv.ammo_property->arrayProp&&sv.ammo_property->arrayProp->offset==0x64c&&
        sv.ammo_property->table==sv.ammo_table&&sv.ammo_property->flags==0x200);
    assert(rv.ammo_property->array_prop==nullptr&&rv.ammo_property->table==rv.ammo_table&&rv.ammo_property->offset==0xc00);
    auto& wp=sv.combat_local->props[1];auto& wr=rv.combat_local->props[1];
    assert(wp.arrayProp&&wp.arrayProp->offset==0x6cc&&wp.table==sv.weapons_table&&
        wr.table==rv.weapons_table&&!wr.array_prop&&wr.offset==0xc80);
    alignas(4) unsigned char server[0x790]{},client[0xd44]{};
    s::Recipients recipients{};
    assert(sv.player_local_proxy(nullptr,nullptr,server,&recipients,8)==server&&recipients.words[0]==128u);
    assert(sv.player_local_proxy(nullptr,nullptr,server,&recipients,0)==server&&recipients.words[0]==0);
    auto& bcc=sv.combat_base->props[1];
    assert(bcc.tableProxy(&bcc,&player,server,&recipients,0)==server&&recipients.words[0]==64u&&engine.calls==1);
    assert(bcc.tableProxy(&bcc,&npc,server,&recipients,0)==server&&recipients.words[0]==64u&&engine.calls==2);
    assert(npc.indirect_calls==1&&indirect.calls==1);
    indirect.target=nullptr;
    assert(bcc.tableProxy(&bcc,&npc,server,&recipients,0)==server&&recipients.words[0]==0&&engine.calls==2);
    assert(bcc.tableProxy(&bcc,nullptr,server,&recipients,0)==server&&recipients.words[0]==0);
    const auto compare_array=[&](s::SendProp& sp,RecvProp& rp,unsigned count,bool handles){
        const auto* input=static_cast<const unsigned char*>(sp.tableProxy(&sp,server,server+sp.offset,&recipients,1));
        void* dest=nullptr;rp.table_proxy(&rp,&dest,client+rp.offset,1);
        assert(input==server+sp.offset&&dest==client+rp.offset);
        for(unsigned i=0;i<count;++i){
            auto& a=sp.table->props[i];auto& b=rp.table->props[i];
            char expected[4]={'0',static_cast<char>('0'+i/10),static_cast<char>('0'+i%10),0};
            assert(std::strcmp(a.name,expected)==0&&std::strcmp(b.name,expected)==0);
            assert(std::strcmp(a.excludeOrParent,sp.name)==0&&std::strcmp(b.parent_array,rp.name)==0);
            assert(a.offset==static_cast<int>(i*4)&&b.offset==a.offset&&a.bits==(handles?21:10)&&a.flags==1);
            const std::uint32_t value=handles?(i==47?((0x12345u<<12)|37u):n::invalid_handle):i*17u;
            std::memcpy(server+sp.offset+a.offset,&value,4);
            s::DVariant v{};a.scalarProxy(&a,server,input+a.offset,&v,0,1);
            RecvData d{&b,v,0,1};b.proxy(&d,dest,static_cast<unsigned char*>(dest)+b.offset);
            std::uint32_t output=0;std::memcpy(&output,client+rp.offset+b.offset,4);
            assert(output==(handles&&i==47?((0x345u<<12)|37u):value));
        }
    };
    compare_array(*sv.ammo_property,*rv.ammo_property,32,false);
    compare_array(wp,wr,48,true);
    // Public active handle and private timer use the same scalar proxies.
    auto& a=sv.combat_base->props[2];auto& b=rv.combat_base->props[2];
    const std::uint32_t raw=(0x12345u<<12)|37u;std::memcpy(server+a.offset,&raw,4);
    s::DVariant v{};a.scalarProxy(&a,server,server+a.offset,&v,0,1);
    assert(v.words[0]==((0x345u<<11)|37u));
    RecvData d{&b,v,0,1};b.proxy(&d,client,client+b.offset);
    std::uint32_t active=0;std::memcpy(&active,client+b.offset,4);assert(active==((0x345u<<12)|37u));
    auto& timer=sv.combat_local->props[0];auto& receiveTimer=rv.combat_local->props[0];
    const float seconds=13.25f;std::memcpy(server+timer.offset,&seconds,4);
    timer.scalarProxy(&timer,server,server+timer.offset,&v,0,1);
    d={&receiveTimer,v,0,1};receiveTimer.proxy(&d,client,client+receiveTimer.offset);
    float result=0;std::memcpy(&result,client+receiveTimer.offset,4);assert(result==seconds);
    sv.weapons_table->written=true;rv.ammo_table->in_main_list=true;
    assert(!sa.destroy(&sv)&&!ra.destroy(&rv));
    sv.weapons_table->written=false;rv.ammo_table->in_main_list=false;
    assert(sa.destroy(&sv)&&ra.destroy(&rv)&&!sv.handle&&!rv.handle);
}

inline void check_anim_graph(const AnimSendApi& sa,const AnimRecvApi& ra,const SendApi& wa,const RecvApi& wr){
    s::SendProp sp[24];s::SendTable sb;assert(sb.Construct(sp,24,"DT_BaseEntity"));
    RecvProp rp[24];RecvTable rb{rp,24,nullptr,"DT_BaseEntity"};
    sin_independent::EntityHandleEntry entries[64]{};
    NativeOwner host;entries[37].entity=reinterpret_cast<CBaseEntity*>(&host);entries[37].serial=0x12345;
    AnimSendBindings bindings{&sb,entries,64};AnimRecvBindings rec{&rb};
    AnimSendView sv{};AnimRecvView rv{};
    sb.count=23;assert(!sa.create(&bindings,&sv)&&!sv.handle);sb.count=24;
    assert(sa.create(&bindings,&sv)&&ra.create(&rec,&rv));
    assert(sv.base_animating->count==18&&rv.base_animating->count==18);
    assert(sv.base_animating->props[0].table==&sb&&rv.base_animating->props[0].table==&rb);
    SendView wsv{};RecvView wrv{};
    SendBindings wb{sv.base_animating,&local,&local,&owner};RecvBindings wbr{rv.base_animating};
    assert(wa.create(&wb,&wsv)&&wr.create(&wbr,&wrv));
    assert(wsv.tables[BaseCombat]->props[0].table==sv.base_animating&&wrv.tables[BaseCombat]->props[0].table==rv.base_animating);
    auto& a=*sv.base_animating;auto& b=*rv.base_animating;
    assert(a.props[6].low==1.0f/64&&a.props[6].multiplier==64&&a.props[9].low==-3.9375f&&a.props[9].multiplier==16);
    assert(a.props[16].flags==5&&a.props[16].bits==21);
    alignas(4) unsigned char server[0x474]{},client[0x788]{};
    const auto pair=[&](s::SendProp& x,RecvProp& y,unsigned width,std::uint32_t value){
        assert(std::strcmp(x.name,y.name)==0);
        std::memcpy(server+x.offset,&value,width);
        std::memset(client+y.offset,0x99,width+1);
        s::DVariant v{};x.scalarProxy(&x,server,server+x.offset,&v,0,1);
        RecvData d{&y,v,0,1};y.proxy(&d,client,client+y.offset);
        const std::uint32_t expected=x.bits==21?((0x345u<<12)|37u):value;
        assert(!std::memcmp(client+y.offset,&expected,width)&&client[y.offset+width]==0x99);
    };
    const int indices[]={0,2,3,4,5,6,7,8,1,9,10,11,12,13,14,15,16,17};
    for(unsigned i=1;i<17;++i){
        if(i==2||i==7||i==10)continue;
        const bool byte=i==11||i==12||i==15;
        pair(a.props[i],b.props[indices[i]],byte?1:4,i==16?((0x12345u<<12)|37u):byte?1u:0x3f000000u);
    }
    const std::uint32_t xyz[3]={0x3f000000u,0x80000000u,0x7fc12345u};
    std::memcpy(server+a.props[2].offset,xyz,12);
    s::DVariant v{};a.props[2].scalarProxy(&a.props[2],server,server+a.props[2].offset,&v,0,1);
    RecvData d{&b.props[3],v,0,1};b.props[3].proxy(&d,client,client+b.props[3].offset);
    assert(!std::memcmp(client+b.props[3].offset,xyz,12));
    const auto array=[&](s::SendProp& x,RecvProp& y,int count){
        assert(x.table->count==count&&y.table->count==count&&x.arrayProp&&!y.array_prop);
        for(int i=0;i<count;++i){
            auto& u=x.table->props[i];auto& w=y.table->props[i];
            assert(std::strcmp(u.name,w.name)==0&&u.offset==i*4&&w.offset==i*4);
            assert(std::strcmp(u.excludeOrParent,x.name)==0&&std::strcmp(w.parent_array,y.name)==0);
            const std::uint32_t value=0x3f000000u+static_cast<unsigned>(i);
            std::memcpy(server+x.offset+u.offset,&value,4);
            u.scalarProxy(&u,server,server+x.offset+u.offset,&v,i,1);
            d={&w,v,i,1};w.proxy(&d,client,client+y.offset+w.offset);
            assert(!std::memcmp(client+y.offset+w.offset,&value,4));
        }
    };
    array(a.props[7],b.props[8],24);array(a.props[10],b.props[10],4);
    pair(sv.server_anim_data->props[0],rv.server_anim_data->props[0],4,0x3f000000u);
    auto& gate=a.props[17];s::Recipients mask{};mask.words[0]=0x42;
    host.animating_object=server;server[0x460]=0;
    assert(gate.tableProxy(&gate,&host,server,&mask,1)==server&&mask.words[0]==0x42);
    server[0x460]=1;assert(!gate.tableProxy(&gate,&host,server,&mask,1)&&mask.words[0]==0x42);
    host.animating_object=nullptr;assert(!gate.tableProxy(&gate,&host,server,&mask,1));
    assert(wa.destroy(&wsv)&&wr.destroy(&wrv));
    sv.pose->written=true;rv.controllers->initialized=true;
    assert(!sa.destroy(&sv)&&!ra.destroy(&rv));
    sv.pose->written=false;rv.controllers->initialized=false;
    assert(sa.destroy(&sv)&&ra.destroy(&rv));
}

inline void check_flex_graph(const FlexSendApi& sa,const FlexRecvApi& ra,const AnimSendApi& asa,
        const AnimRecvApi& ara,const InventorySendApi& isa,const InventoryRecvApi& ira){
    s::SendProp ep[24],op;s::SendTable se,so;
    assert(se.Construct(ep,24,"DT_BaseEntity")&&so.Construct(&op,1,"DT_OverlayVars"));
    RecvProp cep[24],cop;RecvTable ce{cep,24,nullptr,"DT_BaseEntity"},co{&cop,1,nullptr,"DT_OverlayVars"};
    sin_independent::EntityHandleEntry entries[1]{};
    AnimSendBindings ab{&se,entries,1};AnimRecvBindings arb{&ce};
    AnimSendView av{};AnimRecvView arv{};
    assert(asa.create(&ab,&av)&&ara.create(&arb,&arv));
    FlexSendBindings sb{av.base_animating,&so};FlexRecvBindings rb{arv.base_animating,&co};
    FlexSendView sv{};FlexRecvView rv{};
    auto bad=sb;bad.overlay_vars=nullptr;
    assert(!sa.create(&bad,&sv)&&!sv.handle);
    assert(sa.create(&sb,&sv)&&ra.create(&rb,&rv));
    assert(sv.overlay->count==2&&rv.overlay->count==2&&sv.flex->count==7&&rv.flex->count==7);
    assert(sv.overlay->props[0].table==av.base_animating&&rv.overlay->props[0].table==arv.base_animating);
    assert(sv.overlay->props[1].table==&so&&rv.overlay->props[1].table==&co);
    assert(sv.flex->props[0].table==sv.overlay&&rv.flex->props[0].table==rv.overlay);
    // New native inheritance chain connects directly to Phase696 inventory.
    alignas(4) unsigned char edicts[2*20]{};
    NativeEngine engine;engine.expected=edicts+20;engine.result=1;
    InventorySendBindings ib{sv.flex,&engine,entries,1,edicts,1};InventoryRecvBindings irb{rv.flex};
    InventorySendView iv{};InventoryRecvView irv{};
    assert(isa.create(&ib,&iv)&&ira.create(&irb,&irv));
    assert(iv.combat_base->props[0].table==sv.flex&&irv.combat_base->props[0].table==rv.flex);
    alignas(4) unsigned char server[0x5b8]{},client[0xac4]{};
    auto& a=*sv.flex;auto& b=*rv.flex;
    assert(a.props[1].offset==0x4a8&&b.props[1].offset==0x860&&a.props[1].arrayProp&&!b.props[1].array_prop);
    assert(sv.weights->count==64&&rv.weights->count==64&&a.props[1].table==sv.weights&&b.props[1].table==rv.weights);
    for(int i=0;i<64;++i){
        auto& x=sv.weights->props[i];auto& y=rv.weights->props[i];
        assert(x.bits==12&&x.flags==8&&x.high==1.0f-1.0f/4096&&x.multiplier==4096);
        assert(x.offset==i*4&&y.offset==i*4&&std::strcmp(x.name,y.name)==0&&
            std::strcmp(x.excludeOrParent,"m_flexWeight")==0&&std::strcmp(y.parent_array,"m_flexWeight")==0);
        const float value=static_cast<float>(i)/64;
        std::memcpy(server+a.props[1].offset+x.offset,&value,4);
        s::DVariant v{};x.scalarProxy(&x,server,server+a.props[1].offset+x.offset,&v,i,1);
        RecvData d{&y,v,i,1};y.proxy(&d,client,client+b.props[1].offset+y.offset);
        assert(!std::memcmp(client+b.props[1].offset+y.offset,&value,4));
    }
    for(unsigned i=2;i<7;++i){
        auto& x=a.props[i];auto& y=b.props[i];
        assert(std::strcmp(x.name,y.name)==0);
        const std::uint32_t values[3]={i==2?1u:0x3f000000u,0x80000000u,0x7fc12345u};
        const unsigned width=i==3?12:4;
        std::memcpy(server+x.offset,values,width);
        s::DVariant v{};x.scalarProxy(&x,server,server+x.offset,&v,0,1);
        RecvData d{&y,v,0,1};y.proxy(&d,client,client+y.offset);
        assert(!std::memcmp(client+y.offset,values,width));
    }
    assert(isa.destroy(&iv)&&ira.destroy(&irv));
    sv.weights->written=true;rv.overlay->initialized=true;
    assert(!sa.destroy(&sv)&&!ra.destroy(&rv));
    sv.weights->written=false;rv.overlay->initialized=false;
    assert(sa.destroy(&sv)&&ra.destroy(&rv));
    assert(asa.destroy(&av)&&ara.destroy(&arv));
}

}
