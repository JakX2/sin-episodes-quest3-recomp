#pragma once
#include "native_weapon_datatables.h"
#include <cassert>
#include <cstring>
#include <initializer_list>
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

struct OverlayFixture {
    alignas(4) unsigned char storage[15*64]{};
    unsigned reserves=0,resizes=0;
    bool fail=false;
};
inline int __cdecl reserve_layers(void* opaque,void* entity,int off,int stride,int count) noexcept {
    auto& f=*static_cast<OverlayFixture*>(opaque);
    ++f.reserves;if(f.fail)return 0;
    auto* address=static_cast<unsigned char*>(entity)+off;
    LayerVector v{};std::memcpy(&v,address,sizeof(v));
    assert(count==15&&(stride==32||stride==64));
    if(v.memory!=f.storage){
        if(v.memory&&v.count)std::memcpy(f.storage,v.memory,static_cast<unsigned>(v.count*stride));
        v.memory=f.storage;
    }
    v.capacity=count;v.elements=v.memory;
    std::memset(v.memory+v.count*stride,0,static_cast<unsigned>((count-v.count)*stride));
    std::memcpy(address,&v,sizeof(v));return 1;
}
inline int __cdecl resize_layers(void* opaque,void* entity,int off,int stride,int count) noexcept {
    auto& f=*static_cast<OverlayFixture*>(opaque);
    ++f.resizes;if(f.fail)return 0;
    assert(off==0x7a4&&stride==32&&count>=0&&count<=15);
    auto* address=static_cast<unsigned char*>(entity)+off;
    LayerVector v{};std::memcpy(&v,address,sizeof(v));
    if(v.capacity<count){if(!reserve_layers(opaque,entity,off,stride,15))return 0;std::memcpy(&v,address,sizeof(v));}
    v.count=count;std::memcpy(address,&v,sizeof(v));return 1;
}
inline void check_overlay_graph(const OverlaySendApi& sa,const OverlayRecvApi& ra,
        const AnimSendApi& asa,const AnimRecvApi& ara,const FlexSendApi& fsa,const FlexRecvApi& fra){
    OverlayFixture sf,cf;
    OverlaySendBindings sb{&sf,&reserve_layers};OverlayRecvBindings rb{&cf,&reserve_layers,&resize_layers};
    OverlaySendView sv{};OverlayRecvView rv{};
    auto bad=rb;bad.resize=nullptr;assert(!ra.create(&bad,&rv)&&!rv.handle);
    assert(sa.create(&sb,&sv)&&ra.create(&rb,&rv));
    assert(sv.overlay_vars->count==1&&rv.overlay_vars->count==1&&sv.vector_table->count==16&&rv.vector_table->count==16);
    assert(sv.layer->count==5&&rv.layer->count==5);
    assert(std::strcmp(sv.vector_table->name,"_ST_m_AnimOverlay_15")==0&&
        std::strcmp(rv.length_table->name,"_LPT_m_AnimOverlay_15")==0);
    assert(sv.overlay_vars->props[0].table==sv.vector_table&&rv.overlay_vars->props[0].table==rv.vector_table);
    s::SendProp seprops[24];s::SendTable se;assert(se.Construct(seprops,24,"DT_BaseEntity"));
    RecvProp ceprops[24];RecvTable ce{ceprops,24,nullptr,"DT_BaseEntity"};
    sin_independent::EntityHandleEntry entries[1]{};
    AnimSendView av{};AnimRecvView arv{};
    AnimSendBindings ab{&se,entries,1};AnimRecvBindings arb{&ce};
    assert(asa.create(&ab,&av)&&ara.create(&arb,&arv));
    FlexSendView fv{};FlexRecvView frv{};
    FlexSendBindings fb{av.base_animating,sv.overlay_vars};FlexRecvBindings frb{arv.base_animating,rv.overlay_vars};
    assert(fsa.create(&fb,&fv)&&fra.create(&frb,&frv));
    assert(fv.overlay->props[1].table==sv.overlay_vars&&frv.overlay->props[1].table==rv.overlay_vars);
    alignas(4) unsigned char server[0x4a8]{},client[0x7b8]{};
    LayerVector sh{sf.storage,15,0,3,sf.storage},ch{};
    std::memcpy(server+0x494,&sh,sizeof(sh));std::memcpy(client+0x7a4,&ch,sizeof(ch));
    sf.storage[8]=0x53;
    auto& sl=sv.vector_table->props[0];auto& cl=rv.vector_table->props[0];
    s::Recipients mask{};mask.words[0]=0x22;
    assert(sl.tableProxy(&sl,server,server,&mask,1)==server&&sf.reserves==1&&sf.storage[8]==0x53&&mask.words[0]==0x22);
    void* dest=nullptr;cl.table_proxy(&cl,&dest,client,1);assert(dest==client&&cf.reserves==1);
    auto& lp=sv.length_table->props[0];auto& lr=rv.length_table->props[0];
    s::DVariant value{};lp.scalarProxy(&lp,server,server,&value,0,1);
    assert(value.words[0]==3);
    RecvData d{&lr,value,0,1};lr.proxy(&d,client,client);
    std::memcpy(&ch,client+0x7a4,sizeof(ch));assert(ch.count==3&&ch.capacity==15&&cf.resizes==1);
    lr.proxy(&d,client,client);assert(cf.resizes==1);
    d.value.words[0]=16;lr.proxy(&d,client,client);assert(cf.resizes==1);
    // Every numbered child resolves its vector element by stored index,
    // including reserved dormant slots beyond the logical count.
    const int so[]={8,12,16,24,52},co[]={0,20,4,8,12};
    for(int i=0;i<15;++i){
        auto& sp=sv.vector_table->props[i+1];auto& rp=rv.vector_table->props[i+1];
        assert(std::strcmp(sp.name,rp.name)==0&&sp.stride==i&&rp.stride==i&&sp.offset==0&&rp.offset==0&&sp.flags==0x200);
        auto* src=static_cast<unsigned char*>(sp.tableProxy(&sp,server,server,&mask,1));
        rp.table_proxy(&rp,&dest,client,1);
        assert(src==sf.storage+i*64&&dest==cf.storage+i*32);
        for(int j=0;j<5;++j){
            auto& a=sv.layer->props[j];auto& b=rv.layer->props[j];
            assert(a.offset==so[j]&&b.offset==co[j]&&std::strcmp(a.name,b.name)==0);
            const std::uint32_t v=(j==0||j==4)?static_cast<unsigned>(i):0x3f000000u+static_cast<unsigned>(i);
            std::memcpy(src+a.offset,&v,4);a.scalarProxy(&a,src,src+a.offset,&value,i,1);
            d={&b,value,i,1};b.proxy(&d,dest,static_cast<unsigned char*>(dest)+b.offset);
            assert(!std::memcmp(static_cast<unsigned char*>(dest)+b.offset,&v,4));
        }
    }
    d={&lr,{},0,1};d.value.words[0]=1;lr.proxy(&d,client,client);
    std::memcpy(&ch,client+0x7a4,sizeof(ch));assert(ch.count==1&&cf.resizes==2);
    sf.fail=true;assert(!sl.tableProxy(&sl,server,server,&mask,1));
    cf.fail=true;cl.table_proxy(&cl,&dest,client,1);assert(!dest);
    sv.vector_table->props[15].stride=15;assert(!sv.vector_table->props[15].tableProxy(&sv.vector_table->props[15],server,server,&mask,1));
    rv.vector_table->props[15].stride=-1;rv.vector_table->props[15].table_proxy(&rv.vector_table->props[15],&dest,client,1);assert(!dest);
    assert(fsa.destroy(&fv)&&fra.destroy(&frv)&&asa.destroy(&av)&&ara.destroy(&arv));
    sv.layer->initialized=true;rv.length_table->decoder=&cf;
    assert(!sa.destroy(&sv)&&!ra.destroy(&rv));sv.layer->initialized=false;rv.length_table->decoder=nullptr;
    assert(sa.destroy(&sv)&&ra.destroy(&rv));
}

struct VelocityFixture {
    unsigned calls=0;
    bool fail=false;
    float before[3]{};
};
inline int __cdecl invalidate_velocity(void* opaque,void* entity,int flags) noexcept {
    auto& f=*static_cast<VelocityFixture*>(opaque);assert(flags==4);
    ++f.calls;std::memcpy(f.before,static_cast<unsigned char*>(entity)+0xec,12);
    return !f.fail;
}
inline void check_player_local_graph(const PlayerLocalSendApi& sa,const PlayerLocalRecvApi& ra,
        const InventorySendApi& isa,const InventoryRecvApi& ira){
    s::SendProp sflexp,slocalp[48];s::SendTable sflex,slocal;
    assert(sflex.Construct(&sflexp,1,"DT_BaseFlex")&&slocal.Construct(slocalp,48,"DT_Local"));
    RecvProp cflexp,clocalp[49];RecvTable cflex{&cflexp,1,nullptr,"DT_BaseFlex"},clocal{clocalp,49,nullptr,"DT_Local"};
    alignas(4) unsigned char edicts[2*20]{};
    NativeEngine engine;engine.expected=edicts+20;engine.result=1;
    NativeOwner ownerObj;sin_independent::EntityHandleEntry entries[64]{};
    entries[37].entity=reinterpret_cast<CBaseEntity*>(&ownerObj);entries[37].serial=0x12345;
    InventorySendBindings ib{&sflex,&engine,entries,64,edicts,1};InventoryRecvBindings irb{&cflex};
    InventorySendView iv{};InventoryRecvView irv{};
    assert(isa.create(&ib,&iv)&&ira.create(&irb,&irv));
    VelocityFixture vf;
    PlayerLocalSendBindings sb{&slocal,iv.ammo_property,entries,64};
    PlayerLocalRecvBindings rb{&clocal,irv.ammo_property,&vf,&invalidate_velocity};
    PlayerLocalSendView sv{};PlayerLocalRecvView rv{};
    auto bad=rb;bad.invalidate_velocity=nullptr;assert(!ra.create(&bad,&rv));
    slocal.count=49;assert(!sa.create(&sb,&sv));slocal.count=48;
    assert(sa.create(&sb,&sv)&&ra.create(&rb,&rv));
    assert(sv.table->count==27&&rv.table->count==27);
    auto* s=sv.table->props;auto* c=rv.table->props;
    assert(s[0].table==&slocal&&c[0].table==&clocal&&s[0].offset==0x794&&c[0].offset==0xd68);
    assert(s[5].table==iv.ammo_table&&c[5].table==irv.ammo_table&&s[5].arrayProp==iv.ammo_property->arrayProp);
    assert(s[17].type==4&&c[17].type==4&&s[17].elements==2&&c[17].elements==2&&
        s[17].stride==4&&c[17].stride==4&&!s[17].arrayProp&&!c[17].array_prop);
    assert(s[4].high==3.984375f&&s[4].multiplier==64&&s[12].bits==20&&s[14].bits==16&&s[12].flags==0x400);
    alignas(4) unsigned char server[0x1300]{},client[0x1180]{};
    const auto pair=[&](int i,unsigned width,std::uint32_t value){
        auto& a=s[i];auto& b=c[i];
        assert(i==16||std::strcmp(a.name,b.name)==0);
        std::memcpy(server+a.offset,&value,width);
        s::DVariant v{};a.scalarProxy(&a,server,server+a.offset,&v,0,1);
        RecvData d{&b,v,0,1};b.proxy(&d,client,client+b.offset);
        const std::uint32_t expect=a.bits==21?((0x345u<<12)|37u):value;
        assert(!std::memcmp(client+b.offset,&expect,width));
    };
    for(int i=1;i<27;++i){
        if(i==5||i==15||i==17||i==19)continue;
        const bool byte=i==6||i==7;
        const bool handle=i==10||i==11||i==16||i==18;
        pair(i,byte?1:4,handle?((0x12345u<<12)|37u):byte?1u:0x3f000000u);
    }
    assert(vf.calls==3&&vf.before[0]==0.5f&&vf.before[1]==0.5f&&vf.before[2]==0);
    pair(12,4,0x3f000000u);assert(vf.calls==3);
    // Signed zero is numerically equal; NaN remains an update.
    float zero=0;std::memcpy(client+0xec,&zero,4);
    RecvData d{&c[12],{},0,1};d.value.words[0]=0x80000000u;
    c[12].proxy(&d,client,client+0xec);assert(vf.calls==3&&!std::memcmp(client+0xec,&zero,4));
    vf.fail=true;d.value.words[0]=0x3f800000u;
    c[12].proxy(&d,client,client+0xec);assert(vf.calls==4&&!std::memcmp(client+0xec,&zero,4));
    vf.fail=false;c[12].proxy(&d,client,client+0xec);assert(vf.calls==5);
    const std::uint32_t xyz[3]={0x3f000000u,0x80000000u,0x7fc12345u};
    for(int i:{15,19}){
        std::memcpy(server+s[i].offset,xyz,12);s::DVariant v{};
        s[i].scalarProxy(&s[i],server,server+s[i].offset,&v,0,1);
        d={&c[i],v,0,1};c[i].proxy(&d,client,client+c[i].offset);
        assert(!std::memcmp(client+c[i].offset,xyz,12));
    }
    sv.table->written=true;rv.table->decoder=&vf;
    assert(!sa.destroy(&sv)&&!ra.destroy(&rv));
    sv.table->written=false;rv.table->decoder=nullptr;
    assert(sa.destroy(&sv)&&ra.destroy(&rv)&&isa.destroy(&iv)&&ira.destroy(&irv));
}


struct FovFixture {
    FovSample sample{0x41200000u,73,90,1};
    unsigned calls=0;
    bool fail=false;
};
inline int __cdecl sample_fov(void* context,FovSample* out) noexcept {
    auto& f=*static_cast<FovFixture*>(context);++f.calls;
    if(f.fail)return 0;
    *out=f.sample;return 1;
}

struct EntityNetworkFixture {
    NetworkClock clock{1234,0.015625f};
    unsigned effects=0,interpolation=0,transforms=0;
    std::uint32_t observed_effects=0;
};
inline int __cdecl sample_clock(void* context,NetworkClock* out) noexcept {
    *out=static_cast<EntityNetworkFixture*>(context)->clock;return 1;
}
inline int __cdecl sample_transform(void* context,const void* entity,int angles,std::uint32_t* out) noexcept {
    ++static_cast<EntityNetworkFixture*>(context)->transforms;
    std::memcpy(out,static_cast<const unsigned char*>(entity)+(angles?0x36c:0x360),12);return 1;
}
inline void __cdecl effects_changed(void* context,void* entity) noexcept {
    auto& f=*static_cast<EntityNetworkFixture*>(context);++f.effects;
    std::memcpy(&f.observed_effects,static_cast<unsigned char*>(entity)+0x78,4);
}
inline void __cdecl interpolation_changed(void* context,void*) noexcept {
    ++static_cast<EntityNetworkFixture*>(context)->interpolation;
}

inline void check_derived_player_graph(const DerivedPlayerSendApi& sa,const DerivedPlayerRecvApi& ra,
        s::SendTable* sb,RecvTable* rb,s::SendTable* se,RecvTable* re,
        const sin_independent::EntityHandleEntry* entries){
    DerivedPlayerSendBindings bs{sb,se,entries,64};DerivedPlayerRecvBindings br{rb,re};
    DerivedPlayerSendView sv{};DerivedPlayerRecvView rv{};
    auto bad=bs;bad.entity=nullptr;assert(!sa.create(&bad,&sv));
    assert(sa.create(&bs,&sv)&&ra.create(&br,&rv));
    assert(sv.player->count==6&&rv.player->count==6&&sv.local->count==13&&rv.local->count==13&&sv.hand->count==1&&rv.hand->count==1);
    auto* s=sv.player->props;auto* c=rv.player->props;
    assert(s[0].table==sb&&c[0].table==rb&&sv.hand->props[0].table==se&&rv.hand->props[0].table==re);
    assert(s[1].table==sv.local&&c[1].table==rv.local&&s[1].offset==0x137c&&c[1].offset==0x11c8&&s[1].flags==0);
    alignas(4) unsigned char server[0x151c]{},client[0x1248]{};
    s::Recipients mask{};mask.words[0]=0xffffffffu;
    auto* sl=static_cast<unsigned char*>(s[1].tableProxy(&s[1],server,server+s[1].offset,&mask,7));
    assert(sl==server+0x137c&&mask.words[0]==64);
    void* dst=nullptr;c[1].table_proxy(&c[1],&dst,client+c[1].offset,7);
    assert(dst==client+0x11c8);auto* cl=static_cast<unsigned char*>(dst);
    const unsigned widths[]={4,4,4,1,1,4,4,4,1,1,4,4,4};
    const int offsets[]={4,8,0xc,0x10,0x11,0x14,0x18,0x1c,0x20,0x21,0x24,0x28,0x2c};
    const std::uint32_t input[]={0x42480000u,0x41c80000u,0x3f400000u,1,1,5,4,2,1,1,0x41200000u,0x41a00000u,(0x12345u<<12)|37u};
    for(unsigned i=0;i<13;++i){
        auto& a=sv.local->props[i];auto& b=rv.local->props[i];
        assert(a.offset==offsets[i]&&b.offset==offsets[i]&&std::strcmp(a.name,b.name)==0);
        std::memcpy(sl+a.offset,&input[i],widths[i]);
        s::DVariant v{};a.scalarProxy(&a,sl,sl+a.offset,&v,0,7);
        RecvData d{&b,v,0,7};b.proxy(&d,cl,cl+b.offset);
        const auto expected=i==12?((0x345u<<12)|37u):input[i];
        assert(!std::memcmp(cl+b.offset,&expected,widths[i]));
    }
    assert(sv.local->props[0].bits==10&&sv.local->props[0].flags==0x11&&sv.local->props[0].low==100.0f/1024);
    assert(sv.local->props[10].flags==4&&sv.local->props[11].flags==4);
    for(unsigned i=2;i<6;++i){
        auto& a=s[i];auto& b=c[i];
        const unsigned width=i==2?1:4;
        const std::uint32_t input_word=i==2?1u:0x3f000000u;
        std::memcpy(server+a.offset,&input_word,width);
        s::DVariant v{};a.scalarProxy(&a,server,server+a.offset,&v,0,7);
        RecvData d{&b,v,0,7};b.proxy(&d,client,client+b.offset);
        assert(std::strcmp(a.name,b.name)==0&&!std::memcmp(client+b.offset,&input_word,width));
    }
    // Owner-only routing is preserved when the local subobject is nested
    // at nonzero original offsets.
    assert(s[1].tableProxy(&s[1],server,sl,&mask,0)==sl&&mask.words[0]==0);
    sv.local->precalc=&mask;rv.hand->initialized=true;assert(!sa.destroy(&sv)&&!ra.destroy(&rv));
    sv.local->precalc=nullptr;rv.hand->initialized=false;assert(sa.destroy(&sv)&&ra.destroy(&rv));
}

inline unsigned class_creator_calls=0;
inline int class_creator_index=0,class_creator_serial=0;
inline int class_creator_object=0;
inline void* __cdecl fixture_class_creator(int index,int serial){
    ++class_creator_calls;class_creator_index=index;class_creator_serial=serial;
    return &class_creator_object;
}
inline void check_class_subset(const ServerClassApi& sa,const ClientClassApi& ra,
        const ServerClassBindings& sb,const ClientClassBindings& cb){
    ServerClassView sv{};ClientClassView cv{};
    auto bad=cb;bad.creators[ClPlayer]=nullptr;assert(!ra.create(&bad,&cv)&&!cv.handle);
    auto sbad=sb;sbad.tables[ClPlayer]=sb.tables[ClBasePlayer];assert(!sa.create(&sbad,&sv)&&!sv.handle);
    auto old=sb.tables[ClPlayer]->props[0].table;
    sb.tables[ClPlayer]->props[0].table=sb.tables[ClEntity];
    assert(!sa.create(&sb,&sv)&&!sv.handle);
    sb.tables[ClPlayer]->props[0].table=old;
    assert(sa.create(&sb,&sv)&&ra.create(&cb,&cv));
    assert(sv.count==ClassSubsetCount&&cv.count==ClassSubsetCount&&
        !sa.create(&sb,&sv)&&!ra.create(&cb,&cv));
    constexpr unsigned sorted[ClassSubsetCount]={ClAnimating,ClOverlay,ClCombatCharacter,ClCombatWeapon,
        ClEntity,ClFlex,ClHLCombatWeapon,ClBasePlayer,ClPlayer,ClMachineGun,ClSelectFire,ClHand,
        ClAssault,ClMagnum,ClScatter};
    auto* sc=sv.head;
    for(unsigned i=0;i<ClassSubsetCount;++i){
        const unsigned j=sorted[i];
        assert(sc==&sv.records[j]&&std::strcmp(sc->name,class_names[j])==0&&
            sc->table==sb.tables[j]&&sc->class_id==0&&sc->instance_baseline==0xffff);
        sc=sc->next;
    }
    assert(!sc);
    auto* cc=cv.head;
    for(unsigned i=ClassSubsetCount;i>0;--i){
        const unsigned j=i-1;
        assert(cc==&cv.records[j]&&std::strcmp(cc->name,class_names[j])==0&&
            cc->table==cb.tables[j]&&cc->class_id==0&&cc->create==cb.creators[j]&&!cc->create_event);
        cc=cc->next;
    }
    assert(!cc);
    class_creator_calls=0;
    assert(cv.records[ClPlayer].create(12,345)==&class_creator_object&&class_creator_calls==1&&
        class_creator_index==12&&class_creator_serial==345);
    // The registry does not invent engine-assigned IDs or free tables.
    sv.records[ClPlayer].instance_baseline=7;cv.records[ClPlayer].class_id=9;
    assert(!sa.destroy(&sv)&&!ra.destroy(&cv));
    sv.records[ClPlayer].instance_baseline=0xffff;cv.records[ClPlayer].class_id=0;
    sb.tables[ClPlayer]->written=true;cb.tables[ClPlayer]->initialized=true;
    assert(!sa.destroy(&sv)&&!ra.destroy(&cv));
    sb.tables[ClPlayer]->written=false;cb.tables[ClPlayer]->initialized=false;
    assert(sa.destroy(&sv)&&ra.destroy(&cv)&&!sv.head&&!cv.head);
}
inline void check_base_player_graph(const BasePlayerSendApi& sa,const BasePlayerRecvApi& ra,
        const AnimSendApi& asa,const AnimRecvApi& ara,const OverlaySendApi& osa,const OverlayRecvApi& ora,
        const FlexSendApi& fsa,const FlexRecvApi& fra,const InventorySendApi& isa,const InventoryRecvApi& ira,
        const PlayerLocalSendApi& lsa,const PlayerLocalRecvApi& lra,
        const LocalDataSendApi& dsa,const LocalDataRecvApi& dra,
        const BaseEntitySendApi& esa,const BaseEntityRecvApi& era,
        const DerivedPlayerSendApi& psa,const DerivedPlayerRecvApi& pra,
        const ResolvedSendApi& wsa,const RecvApi& wra,const ServerClassApi& csa,const ClientClassApi& cra){
    s::SendProp collision_props[8];s::SendTable collision;
    assert(collision.Construct(collision_props,8,"DT_CollisionProperty"));
    RecvProp collision_recv_props[8];RecvTable collision_recv{collision_recv_props,8,nullptr,"DT_CollisionProperty"};
    NativeOwner ownerObj;
    sin_independent::EntityHandleEntry entries[64]{};
    entries[37].entity=reinterpret_cast<CBaseEntity*>(&ownerObj);entries[37].serial=0x12345;
    EntityNetworkFixture ef;
    BaseEntitySendBindings eb{&collision,entries,64,&ef,&sample_clock,&sample_transform};
    BaseEntityRecvBindings erb{&collision_recv,&ef,&sample_clock,&effects_changed,&interpolation_changed};
    BaseEntitySendView ev{};BaseEntityRecvView erv{};assert(esa.create(&eb,&ev)&&era.create(&erb,&erv));
    AnimSendView av{};AnimRecvView arv{};AnimSendBindings ab{ev.entity,entries,64};AnimRecvBindings arb{erv.entity};
    assert(asa.create(&ab,&av)&&ara.create(&arb,&arv));
    OverlayFixture sf,cf;OverlaySendBindings ob{&sf,&reserve_layers};OverlayRecvBindings orb{&cf,&reserve_layers,&resize_layers};
    OverlaySendView ov{};OverlayRecvView orv{};assert(osa.create(&ob,&ov)&&ora.create(&orb,&orv));
    FlexSendView fv{};FlexRecvView frv{};FlexSendBindings fb{av.base_animating,ov.overlay_vars};
    FlexRecvBindings frb{arv.base_animating,orv.overlay_vars};assert(fsa.create(&fb,&fv)&&fra.create(&frb,&frv));
    alignas(4) unsigned char edicts[2*20]{};NativeEngine engine;engine.expected=edicts+20;engine.result=1;
    InventorySendBindings ib{fv.flex,&engine,entries,64,edicts,1};InventoryRecvBindings irb{frv.flex};
    InventorySendView iv{};InventoryRecvView irv{};assert(isa.create(&ib,&iv)&&ira.create(&irb,&irv));
    FovFixture ff;LocalDataSendBindings db{entries,64};LocalDataRecvBindings drb{&ff,&sample_fov};
    LocalDataSendView dv{};LocalDataRecvView drv{};assert(dsa.create(&db,&dv)&&dra.create(&drb,&drv));
    VelocityFixture vf;PlayerLocalSendBindings lb{dv.table,iv.ammo_property,entries,64};
    PlayerLocalRecvBindings lrb{drv.table,irv.ammo_property,&vf,&invalidate_velocity};
    PlayerLocalSendView lv{};PlayerLocalRecvView lrv{};assert(lsa.create(&lb,&lv)&&lra.create(&lrb,&lrv));
    BasePlayerSendBindings pb{iv.combat_base,lv.table,entries,64};
    BasePlayerRecvBindings prb{irv.combat_base,lrv.table};
    BasePlayerSendView pv{};BasePlayerRecvView prv{};
    auto invalid=pb;invalid.private_player=nullptr;assert(!sa.create(&invalid,&pv)&&!pv.handle);
    assert(sa.create(&pb,&pv)&&ra.create(&prb,&prv));
    check_derived_player_graph(psa,pra,pv.player,prv.player,ev.entity,erv.entity,entries);
    DerivedPlayerSendBindings dsb{pv.player,ev.entity,entries,64};
    DerivedPlayerRecvBindings drb2{prv.player,erv.entity};
    DerivedPlayerSendView dsv{};DerivedPlayerRecvView drv2{};
    assert(psa.create(&dsb,&dsv)&&pra.create(&drb2,&drv2));
    ResolvedSendBindings wb{av.base_animating,&engine,entries,64,edicts,1};
    RecvBindings wrb{arv.base_animating};
    SendView wsv{};RecvView wrv{};assert(wsa.create(&wb,&wsv)&&wra.create(&wrb,&wrv));
    ServerClassBindings csb{{wsv.tables[BaseCombat],av.base_animating,fv.overlay,iv.combat_base,
        ev.entity,fv.flex,pv.player,wsv.tables[BaseHL],wsv.tables[MachineGun],wsv.tables[SelectFire],
        dsv.hand,dsv.player,wsv.tables[Magnum],wsv.tables[AssaultRifle],wsv.tables[Scattergun]}};
    ClientClassBindings crb{{wrv.tables[BaseCombat],arv.base_animating,frv.overlay,irv.combat_base,
        erv.entity,frv.flex,prv.player,wrv.tables[BaseHL],wrv.tables[MachineGun],wrv.tables[SelectFire],
        drv2.hand,drv2.player,wrv.tables[Magnum],wrv.tables[AssaultRifle],wrv.tables[Scattergun]},{}};
    for(auto& creator:crb.creators)creator=&fixture_class_creator;
    check_class_subset(csa,cra,csb,crb);
    assert(wsa.destroy(&wsv)&&wra.destroy(&wrv)&&psa.destroy(&dsv)&&pra.destroy(&drv2));
    auto* s=pv.player->props;auto* c=prv.player->props;
    assert(pv.player->count==15&&prv.player->count==15&&pv.state->count==1&&prv.state->count==1);
    assert(s[0].table==iv.combat_base&&c[0].table==irv.combat_base);
    assert(s[14].table==lv.table&&c[1].table==lrv.table&&s[14].flags==0&&c[1].offset==0);
    assert(s[1].table==pv.state&&c[2].table==prv.state&&s[1].offset==0x904&&c[2].offset==0xefc);
    assert(s[12].type==4&&c[13].type==4&&s[12].elements==2&&c[13].elements==2&&
        s[12].stride==4&&c[13].stride==4&&!s[12].arrayProp&&!c[13].array_prop);
    alignas(4) unsigned char server[0x1300]{},client[0x11a0]{};
    s::Recipients mask{};mask.words[0]=0xffffffffu;
    assert(s[14].tableProxy(&s[14],server,server,&mask,256)==server&&mask.words[7]==0x80000000u&&mask.words[0]==0);
    assert(s[14].tableProxy(&s[14],server,server,&mask,0)==server&&mask.words[7]==0);
    void* cp=nullptr;c[1].table_proxy(&c[1],&cp,client,1);assert(cp==client);
    const std::uint32_t raw=(0x12345u<<12)|37u;
    for(int i=2;i<=11;++i){
        auto& a=s[i];auto& b=c[i+1];
        const bool handle=i==2||i==10||i==11;
        assert(i==11||std::strcmp(a.name,b.name)==0);
        const std::uint32_t value=handle?raw:i==8?0xffffffffu:i==7?0x42c80000u:i==5?100u:3u;
        std::memcpy(server+a.offset,&value,4);
        s::DVariant v{};a.scalarProxy(&a,server,server+a.offset,&v,0,1);
        RecvData d{&b,v,0,1};b.proxy(&d,client,client+b.offset);
        const std::uint32_t expected=handle?((0x345u<<12)|37u):i==8?0x1ffu:value;
        assert(!std::memcmp(client+b.offset,&expected,4));
    }
    auto& ds=pv.state->props[0];auto& dc=prv.state->props[0];
    server[s[1].offset+ds.offset]=1;client[c[2].offset+dc.offset+1]=0xa5;
    s::DVariant v{};ds.scalarProxy(&ds,server+s[1].offset,server+s[1].offset+ds.offset,&v,0,1);
    RecvData d{&dc,v,0,1};dc.proxy(&d,client+c[2].offset,client+c[2].offset+dc.offset);
    assert(client[c[2].offset+dc.offset]==1&&client[c[2].offset+dc.offset+1]==0xa5);
    assert(s[13].type==3&&c[14].type==3&&c[14].string_size==18);
    std::memcpy(server+s[13].offset,"Subway",7);
    auto* place=client+c[14].offset;std::memset(place,0xa5,19);
    s[13].scalarProxy(&s[13],server,server+s[13].offset,&v,0,1);
    d={&c[14],v,0,1};c[14].proxy(&d,client,place);
    assert(std::strcmp(reinterpret_cast<const char*>(place),"Subway")==0&&place[7]==0xa5&&place[17]==0&&place[18]==0xa5);
    const char longer[]="abcdefghijklmnopqrstuv";const char* src=longer;
    std::memcpy(&d.value.words[0],&src,sizeof(src));c[14].proxy(&d,client,place);
    assert(std::memcmp(place,longer,17)==0&&place[17]==0&&place[18]==0xa5);
    pv.state->written=true;prv.player->initialized=true;assert(!sa.destroy(&pv)&&!ra.destroy(&prv));
    pv.state->written=false;prv.player->initialized=false;
    assert(sa.destroy(&pv)&&ra.destroy(&prv)&&lsa.destroy(&lv)&&lra.destroy(&lrv)&&
        isa.destroy(&iv)&&ira.destroy(&irv)&&fsa.destroy(&fv)&&fra.destroy(&frv)&&
        osa.destroy(&ov)&&ora.destroy(&orv)&&asa.destroy(&av)&&ara.destroy(&arv)&&dsa.destroy(&dv)&&dra.destroy(&drv)&&esa.destroy(&ev)&&era.destroy(&erv));
}

inline void check_local_data_graph(const LocalDataSendApi& sa,const LocalDataRecvApi& ra){
    NativeOwner entity;
    sin_independent::EntityHandleEntry entries[64]{};
    entries[37].entity=reinterpret_cast<CBaseEntity*>(&entity);entries[37].serial=0x12345;
    FovFixture f;
    LocalDataSendBindings sb{entries,64};LocalDataRecvBindings rb{&f,&sample_fov};
    LocalDataSendView sv{};LocalDataRecvView rv{};
    auto bad=rb;bad.sample_fov=nullptr;assert(!ra.create(&bad,&rv));
    assert(sa.create(&sb,&sv)&&ra.create(&rb,&rv));
    assert(sv.table->count==48&&rv.table->count==49&&sv.area_bits->count==32&&rv.area_bits->count==32);
    auto* sp=sv.table->props;auto* rp=rv.table->props;
    assert(sp[0].offset==4&&rp[0].offset==4&&sp[0].table==sv.area_bits&&rp[0].table==rv.area_bits&&sp[0].arrayProp&&!rp[0].array_prop);
    assert(std::strcmp(rp[12].name,"m_vecClientBaseVelocity")==0&&rp[12].offset==0x58);
    alignas(4) unsigned char server[0x158]{},client[0x190]{};
    for(int i=0;i<32;++i){
        auto& a=sv.area_bits->props[i];auto& b=rv.area_bits->props[i];
        assert(a.offset==i&&b.offset==i&&a.bits==8&&a.flags==1&&std::strcmp(a.name,b.name)==0);
        server[4+i]=static_cast<unsigned char>(i+128);
        s::DVariant v{};a.scalarProxy(&a,server,server+4+i,&v,i,1);
        RecvData d{&b,v,i,1};b.proxy(&d,client,client+4+i);
        assert(client[4+i]==server[4+i]);
    }
    assert(client[3]==0&&client[0x24]==0);
    const int offsets[47][2]={
        {0x24,0x24},
        {0x28,0x28},
        {0x2c,0x2c},
        {0x30,0x38},
        {0x40,0x3c},
        {0x41,0x3d},
        {0x42,0x3e},
        {0x44,0x40},
        {0x48,0x44},
        {0x4c,0x48},
        {0x54,0x50},
        {0x5c,0x64},
        {0x68,0xa0},
        {0x74,0xac},
        {0x75,0xad},
        {0x76,0xae},
        {0x78,0xb0},
        {0x7c,0xb4},
        {0x84,0xbc},
        {0x88,0xc0},
        {0x94,0xcc},
        {0x9c,0xd4},
        {0x9d,0xd5},
        {0xa0,0xd8},
        {0xac,0xe4},
        {0xb0,0xe8},
        {0xb4,0xec},
        {0xb8,0xf0},
        {0xc4,0xfc},
        {0xc5,0xfd},
        {0xc8,0x100},
        {0xd4,0x10c},
        {0xd8,0x110},
        {0xdc,0x114},
        {0xe0,0x118},
        {0xe4,0x11c},
        {0xec,0x124},
        {0xf8,0x130},
        {0x104,0x13c},
        {0x110,0x148},
        {0x11c,0x154},
        {0x128,0x160},
        {0x134,0x16c},
        {0x140,0x178},
        {0x14c,0x184},
        {0x150,0x188},
        {0x154,0x18c},
    };
    const unsigned widths[47]={4,4,4,4,1,1,1,4,4,4,4,12,12,1,1,1,4,1,4,12,4,1,1,12,4,4,4,4,1,1,12,4,4,4,4,4,12,12,12,12,12,12,12,12,4,4,4};
    for(int i=1;i<48;++i){
        auto& a=sp[i];auto& b=rp[i<12?i:i+1];
        assert(a.offset==offsets[i-1][0]&&b.offset==offsets[i-1][1]&&std::strcmp(a.name,b.name)==0);
        if(i==2||i==4)continue;
        const unsigned width=widths[i-1];
        const std::uint32_t value[3]={i==47?((0x12345u<<12)|37u):width==1?0xa5u:0x3f000000u,0x80000000u,0x7fc12345u};
        std::memcpy(server+a.offset,value,width);
        s::DVariant v{};a.scalarProxy(&a,server,server+a.offset,&v,0,1);
        RecvData d{&b,v,0,1};b.proxy(&d,client,client+b.offset);
        const std::uint32_t expected=(0x345u<<12)|37u;
        assert(!std::memcmp(client+b.offset,i==47?&expected:value,width));
    }
    const auto read=[&](unsigned offset){std::uint32_t v=0;std::memcpy(&v,client+offset,4);return v;};
    const auto fov=[&](unsigned v){RecvData d{&rp[2],{},0,1};d.value.words[0]=v;rp[2].proxy(&d,client,nullptr);};
    fov(65);assert(f.calls==1&&read(0x28)==65&&read(0x30)==73&&read(0x34)==f.sample.time_bits);
    fov(65);assert(f.calls==1);
    f.sample.has_displayed_fov=0;f.sample.time_bits=0x41a00000u;fov(0);
    assert(f.calls==2&&read(0x30)==65&&read(0x34)==f.sample.time_bits);
    fov(80);assert(f.calls==3&&read(0x30)==90&&read(0x28)==80);
    f.fail=true;fov(81);assert(f.calls==4&&read(0x28)==80);
    std::uint32_t rate=0x3f800000u;std::memcpy(client+0x2c,&rate,4);
    RecvData d{&rp[4],{},0,1};d.value.words[0]=90;rp[4].proxy(&d,client,nullptr);
    assert(read(0x38)==90&&read(0x2c)==0);
    std::memcpy(client+0x2c,&rate,4);rp[4].proxy(&d,client,nullptr);assert(read(0x2c)==rate);
    sv.area_bits->written=true;rv.table->decoder=&f;
    assert(!sa.destroy(&sv)&&!ra.destroy(&rv));sv.area_bits->written=false;rv.table->decoder=nullptr;
    assert(sa.destroy(&sv)&&ra.destroy(&rv));
}

inline void check_entity_graph(const BaseEntitySendApi& sa,const BaseEntityRecvApi& ra){
    s::SendProp cp[8];s::SendTable coll;assert(coll.Construct(cp,8,"DT_CollisionProperty"));
    RecvProp crp[8];RecvTable cr{crp,8,nullptr,"DT_CollisionProperty"};
    NativeOwner host;sin_independent::EntityHandleEntry entries[64]{};
    entries[37].entity=reinterpret_cast<CBaseEntity*>(&host);entries[37].serial=0x12345;
    EntityNetworkFixture f;
    BaseEntitySendBindings sb{&coll,entries,64,&f,&sample_clock,&sample_transform};
    BaseEntityRecvBindings rb{&cr,&f,&sample_clock,&effects_changed,&interpolation_changed};
    BaseEntitySendView sv{};BaseEntityRecvView rv{};
    coll.count=7;assert(!sa.create(&sb,&sv));coll.count=8;
    assert(sa.create(&sb,&sv)&&ra.create(&rb,&rv));
    assert(sv.entity->count==24&&rv.entity->count==24&&sv.anim_time->count==1&&rv.predictable->count==2);
    auto* s=sv.entity->props;auto* c=rv.entity->props;
    assert(s[4].table==&coll&&c[19].table==&cr&&s[4].offset==0xfc&&c[19].offset==0x16c);
    assert(s[0].table==sv.anim_time&&c[0].table==rv.anim_time&&s[21].table==sv.predictable&&c[21].table==rv.predictable);
    alignas(4) unsigned char server[0x474]{},client[0x474]{};
    s::DVariant v{};
    const auto write=[&](unsigned char* object,int off,std::uint32_t val){std::memcpy(object+off,&val,4);};
    const auto read=[&](unsigned char* object,int off){std::uint32_t val=0;std::memcpy(&val,object+off,4);return val;};
    const auto scalar=[&](int si,int ci,std::uint32_t value,unsigned width=4){
        auto& a=s[si];auto& b=c[ci];
        assert(std::strcmp(a.name,b.name)==0);
        std::memcpy(server+a.offset,&value,width);a.scalarProxy(&a,server,server+a.offset,&v,0,1);
        RecvData d{&b,v,0,1};b.proxy(&d,client,client+b.offset);
    };
    const int map[24]={0,1,2,4,19,7,6,5,8,9,10,11,12,13,14,15,16,17,18,3,20,21,22,23};
    for(int i:{3,5,6,7,8,9,10,11,12,13,14,15,16,20,22,23}){
        const bool byte=i==16||i==22||i==23,handle=i>=13&&i<=15;
        const std::uint32_t input=handle?((0x12345u<<12)|37u):byte?1u:0x3f000000u;
        scalar(i,map[i],input,byte?1:4);
        const std::uint32_t expected=handle?((0x345u<<12)|37u):input;
        assert(!std::memcmp(client+c[map[i]].offset,&expected,byte?1:4));
    }
    assert(f.effects==1&&f.observed_effects==0x3f000000u&&f.interpolation==2);
    scalar(7,5,0x3f000000u);scalar(22,22,1,1);
    assert(f.effects==1&&f.interpolation==2);
    write(server,0x204,3);write(server,0x208,2);write(client,0x148,7);
    for(int i=17;i<=18;++i){
        s[i].scalarProxy(&s[i],server,server,&v,0,1);
        RecvData d{&c[i],v,0,1};c[i].proxy(&d,client,nullptr);
        if(i==17)assert(read(client,0x144)==3&&read(client,0x148)==0);
        else assert(read(client,0x148)==2);
    }
    const float origin[3]={1.25f,-2.5f,3.0f},angles[3]={90.0f,-90.0f,450.0f},wrapped[3]={90,270,90};
    std::memcpy(server+0x360,origin,12);std::memcpy(server+0x36c,angles,12);
    for(int i:{2,19}){
        s[i].scalarProxy(&s[i],server,server+s[i].offset,&v,0,1);
        auto& b=c[map[i]];RecvData d{&b,v,0,1};b.proxy(&d,client,client+b.offset);
        assert(!std::memcmp(client+b.offset,i==2?origin:wrapped,12));
    }
    assert(f.transforms==2);
    auto& at=sv.anim_time->props[0];auto& ct=rv.anim_time->props[0];
    const auto time_case=[&](s::SendProp& a,RecvProp& b,int tick,int expected_wire,int expected_tick){
        const float time=tick*f.clock.interval;
        std::memcpy(server+a.offset,&time,4);a.scalarProxy(&a,server,server+a.offset,&v,0,1);
        assert(v.words[0]==static_cast<unsigned>(expected_wire));
        RecvData d{&b,v,0,1};b.proxy(&d,client,nullptr);
        float got=0;std::memcpy(&got,client+b.offset,4);assert(got==expected_tick*f.clock.interval);
    };
    time_case(s[1],c[1],1234,34,1234);time_case(s[1],c[1],1199,0,1200);
    time_case(at,ct,1199,255,1199);time_case(at,ct,1099,0,1200);
    // Exact +128 relative to current tick maps to -128.
    RecvData d{&c[1],{},0,1};d.value.words[0]=162;c[1].proxy(&d,client,nullptr);
    float got=0;std::memcpy(&got,client+0x6c,4);assert(got==1106*f.clock.interval);
    s::Recipients recipients{};recipients.words[0]=0x80;
    host.animating_object=server;server[0x460]=0;
    assert(s[0].tableProxy(&s[0],&host,server,&recipients,1)==server&&recipients.words[0]==0x80);
    server[0x460]=1;assert(!s[0].tableProxy(&s[0],&host,server,&recipients,1));
    write(server,0x3c,0);assert(!s[21].tableProxy(&s[21],server,server,&recipients,1)&&recipients.words[0]==0x80);
    const std::uint32_t id=0x12340000u|(7u<<1)|1u;write(server,0x3c,id);
    assert(s[21].tableProxy(&s[21],server,server,&recipients,1)==server&&recipients.words[0]==128);
    auto& a=sv.predictable->props[0];auto& b=rv.predictable->props[0];
    a.scalarProxy(&a,server,server+a.offset,&v,0,1);d={&b,v,0,1};b.proxy(&d,client,client+b.offset);
    assert(read(client,0xa4)==id);
    sv.predictable->precalc=&f;rv.entity->initialized=true;
    assert(!sa.destroy(&sv)&&!ra.destroy(&rv));sv.predictable->precalc=nullptr;rv.entity->initialized=false;
    assert(sa.destroy(&sv)&&ra.destroy(&rv));
}

inline void check_aggregate(const AggregateSendApi& sa,const AggregateRecvApi& ra){
    s::SendProp cp[8];s::SendTable coll;assert(coll.Construct(cp,8,"DT_CollisionProperty"));
    RecvProp rp[8];RecvTable rc{rp,8,nullptr,"DT_CollisionProperty"};
    NativeOwner owner;sin_independent::EntityHandleEntry entries[64]{};
    entries[37].entity=reinterpret_cast<CBaseEntity*>(&owner);entries[37].serial=0x12345;
    alignas(4) unsigned char edicts[2*20]{};NativeEngine engine;engine.expected=edicts+20;engine.result=1;
    EntityNetworkFixture ef;OverlayFixture sf,cf;FovFixture ff;VelocityFixture vf;
    AggregateSendBindings sb{{&coll,entries,64,&ef,&sample_clock,&sample_transform},
        {&sf,&reserve_layers},&engine,edicts,1};
    AggregateRecvBindings rb{{&rc,&ef,&sample_clock,&effects_changed,&interpolation_changed},
        {&cf,&reserve_layers,&resize_layers},{&ff,&sample_fov},&vf,&invalidate_velocity,{}};
    for(auto& f:rb.creators)f=&fixture_class_creator;
    AggregateSendView sv{};AggregateRecvView rv{};
    // Fail late (last class creator absent), then construct with the same
    // empty output. The graph was never published or partially returned.
    auto bad=rb;bad.creators[ClScatter]=nullptr;
    assert(!ra.create(&bad,&rv)&&!rv.handle&&!rv.class_head&&!rv.tables[0]);
    auto sbad=sb;sbad.overlay.reserve=nullptr;
    assert(!sa.create(&sbad,&sv)&&!sv.handle&&!sv.class_head&&!sv.tables[0]);
    assert(sa.create(&sb,&sv)&&ra.create(&rb,&rv));
    assert(sv.class_count==15&&rv.class_count==15&&!sa.create(&sb,&sv)&&!ra.create(&rb,&rv));
    for(unsigned i=0;i<aggregate_table_count;++i){
        assert(sv.tables[i]&&rv.tables[i]&&std::strcmp(sv.tables[i]->name,rv.tables[i]->name)==0);
        for(unsigned j=0;j<i;++j)assert(sv.tables[i]!=sv.tables[j]&&rv.tables[i]!=rv.tables[j]);
    }
    assert(sv.class_records[ClPlayer].table==sv.tables[23]&&rv.class_records[ClPlayer].table==rv.tables[23]);
    assert(sv.class_records[ClEntity].table==sv.tables[0]&&rv.class_records[ClEntity].table==rv.tables[0]);
    assert(sv.class_records[ClScatter].table==sv.tables[34]&&rv.class_records[ClScatter].table==rv.tables[34]);
    assert(sv.tables[0]->props[4].table==&coll&&rv.tables[0]->props[19].table==&rc);
    // Traverse from class record into the private SiN player subobject.
    auto& sp=sv.class_records[ClPlayer].table->props[1];
    auto& rp2=rv.class_records[ClPlayer].table->props[1];
    assert(sp.table==sv.tables[24]&&rp2.table==rv.tables[24]);
    alignas(4) unsigned char server[0x151c]{},client[0x1248]{};
    s::Recipients mask{};auto* sd=static_cast<unsigned char*>(sp.tableProxy(&sp,server,server+sp.offset,&mask,1));
    void* cd=nullptr;rp2.table_proxy(&rp2,&cd,client+rp2.offset,1);
    assert(sd==server+sp.offset&&cd==client+rp2.offset&&mask.words[0]==1);
    const std::uint32_t accuracy=0x3f400000u;
    auto& a=sp.table->props[2];auto& b=rp2.table->props[2];
    std::memcpy(sd+a.offset,&accuracy,4);s::DVariant v{};
    a.scalarProxy(&a,sd,sd+a.offset,&v,0,1);
    RecvData d{&b,v,0,1};b.proxy(&d,cd,static_cast<unsigned char*>(cd)+b.offset);
    assert(!std::memcmp(static_cast<unsigned char*>(cd)+b.offset,&accuracy,4));
    // Nested-only array table must block teardown even when all top-level
    // class tables are clear. Refusal leaves both complete graphs intact.
    sv.tables[19]->written=true;rv.tables[19]->decoder=&ff;
    assert(!sa.destroy(&sv)&&!ra.destroy(&rv)&&sv.class_head&&rv.class_head);
    sv.tables[19]->written=false;rv.tables[19]->decoder=nullptr;
    sv.class_records[ClPlayer].instance_baseline=2;rv.class_records[ClPlayer].class_id=2;
    assert(!sa.destroy(&sv)&&!ra.destroy(&rv));
    sv.class_records[ClPlayer].instance_baseline=0xffff;rv.class_records[ClPlayer].class_id=0;
    auto corrupted=sv;corrupted.tables[0]=sv.tables[1];assert(!sa.destroy(&corrupted));
    assert(sa.destroy(&sv)&&ra.destroy(&rv)&&!sv.handle&&!rv.handle&&!sv.tables[34]&&!rv.tables[34]);
    // Borrowed collision data is neither reset nor deleted.
    assert(coll.props==cp&&rc.props==rp);
    assert(sa.create(&sb,&sv)&&ra.create(&rb,&rv));
    assert(sa.destroy(&sv)&&ra.destroy(&rv));
}

struct CreatorFixture {
    const ClientCreatorApi* api=nullptr;
    alignas(4) unsigned char storage[0x1250+8]{};
    unsigned allocs=0,constructs=0,inits=0,cleanups=0,last_slot=0,last_bytes=0;
    int index=0,serial=0;
    unsigned fail=0; // 1 allocation, 2 construction, 3 vtable, 4 init
    bool active=false;
    CreatorCleanup last_stage=CreatorCleanup::Raw;
};
inline bool __fastcall fixture_entity_init(void* object,void*,int index,int serial){
    CreatorFixture* f=nullptr;
    std::memcpy(&f,static_cast<unsigned char*>(object)+0x10,sizeof(f));
    assert(f&&f->active);++f->inits;f->index=index;f->serial=serial;
    std::memcpy(static_cast<unsigned char*>(object)+0x50,&index,4);
    return f->fail!=4;
}
inline void __fastcall fixture_networkable_method(void*,void*){}
inline void* fixture_primary_table[38]{};
inline void* fixture_networkable_table[1]{};
inline void* __cdecl creator_allocate(void* context,unsigned slot,unsigned bytes) noexcept {
    auto& f=*static_cast<CreatorFixture*>(context);
    assert(!f.active);++f.allocs;f.last_slot=slot;f.last_bytes=bytes;
    if(f.fail==1)return nullptr;
    std::memset(f.storage,0xa5,sizeof(f.storage));f.active=true;
    return f.storage+4;
}
inline int __cdecl creator_construct(void* context,unsigned slot,void* object,unsigned bytes) noexcept {
    auto& f=*static_cast<CreatorFixture*>(context);++f.constructs;
    assert(f.active&&object==f.storage+4&&bytes==f.last_bytes&&slot==f.last_slot);
    auto* raw=static_cast<unsigned char*>(object);
    for(unsigned i=0;i<bytes;++i)assert(raw[i]==0);
    if(f.fail==2)return 0;
    fixture_primary_table[37]=reinterpret_cast<void*>(&fixture_entity_init);
    fixture_networkable_table[0]=reinterpret_cast<void*>(&fixture_networkable_method);
    void** primary=f.fail==3?nullptr:fixture_primary_table;
    void** networkable=fixture_networkable_table;
    std::memcpy(raw,&primary,sizeof(primary));std::memcpy(raw+8,&networkable,sizeof(networkable));
    CreatorFixture* fp=&f;std::memcpy(raw+0x10,&fp,sizeof(fp));
    if(slot==ClPlayer){
        assert(f.api->hl2local_defaults(raw,bytes));
        assert(f.api->player_tail_defaults(raw,bytes));
    }else if(slot!=ClHand)assert(f.api->weapon_tail_defaults(raw,bytes));
    return 1;
}
inline void __cdecl creator_cleanup(void* context,unsigned slot,void* object,unsigned bytes,CreatorCleanup stage) noexcept {
    auto& f=*static_cast<CreatorFixture*>(context);
    assert(f.active&&slot==f.last_slot&&bytes==f.last_bytes&&object==f.storage+4);
    ++f.cleanups;f.last_stage=stage;f.active=false;
    assert(f.storage[3]==0xa5&&f.storage[4+bytes]==0xa5);
}
inline void check_client_creator(const ClientCreatorApi& api){
    CreatorFixture f;f.api=&api;
    ClientCreatorBindings b{&f,&creator_allocate,&creator_construct,&creator_cleanup};
    const unsigned slots[]={ClPlayer,ClHand,ClMagnum,ClAssault,ClScatter};
    const unsigned sizes[]={0x1250,0x480,0x830,0x830,0x830};
    const auto read=[&](unsigned offset){std::uint32_t v=0;std::memcpy(&v,f.storage+4+offset,4);return v;};
    for(unsigned i=0;i<5;++i){
        assert(api.object_size(slots[i])==sizes[i]);
        auto* networkable=api.create(&b,slots[i],static_cast<int>(17+i),1234);
        assert(networkable==f.storage+12&&f.active&&f.last_bytes==sizes[i]&&
            f.index==static_cast<int>(17+i)&&f.serial==1234&&read(0x50)==17+i);
        if(i==0){
            assert(read(0x11c8+0xc)==0x3f800000u&&read(0x11c8+0x24)==0x3f800000u&&
                read(0x11c8+0x28)==0x3f800000u&&read(0x11c8+0x2c)==0xffffffffu&&
                read(0x11c8+0x54)==0xffffffffu&&read(0x123c)==0);
        }else if(i>=2){
            assert(read(0x788)==0xffffffffu&&read(0x7ec)==0x42820000u&&
                read(0x7f4)==0x44800000u&&read(0x810)==0xffffu);
        }
        creator_cleanup(&f,slots[i],f.storage+4,sizes[i],CreatorCleanup::InitAttempted);
    }
    assert(f.allocs==5&&f.constructs==5&&f.inits==5&&f.cleanups==5);
    assert(!api.object_size(ClEntity)&&!api.create(&b,ClEntity,0,0)&&f.allocs==5);
    auto bad=b;bad.cleanup=nullptr;assert(!api.create(&bad,ClPlayer,1,2)&&f.allocs==5);
    f.fail=1;assert(!api.create(&b,ClPlayer,1,2)&&!f.active&&f.cleanups==5);
    f.fail=2;assert(!api.create(&b,ClPlayer,1,2)&&!f.active&&f.last_stage==CreatorCleanup::Raw);
    f.fail=3;assert(!api.create(&b,ClPlayer,1,2)&&!f.active&&f.last_stage==CreatorCleanup::Constructed);
    f.fail=4;assert(!api.create(&b,ClPlayer,1,2)&&!f.active&&f.last_stage==CreatorCleanup::InitAttempted);
    assert(f.allocs==9&&f.constructs==8&&f.inits==6&&f.cleanups==8);
    // Defaults must not clobber the vptr, active-device bits, padding,
    // unrelated player tail fields, or the upper half of a 16-bit ID.
    auto* raw=f.storage+4;std::memset(raw,0xa5,0x1250);
    assert(!api.hl2local_defaults(raw,0x124f)&&!api.player_tail_defaults(raw+1,0x1250));
    assert(read(0x11c8+0xc)==0xa5a5a5a5u);
    assert(api.hl2local_defaults(raw,0x1250)&&api.player_tail_defaults(raw,0x1250));
    assert(read(0x11c8)==0xa5a5a5a5u&&read(0x11c8+0x14)==0xa5a5a5a5u&&
        read(0x1238)==0xa5a5a5a5u&&raw[0x11c8+0x12]==0xa5);
    std::memset(raw,0xa5,0x830);
    assert(api.weapon_tail_defaults(raw,0x830)&&read(0x810)==0xa5a5ffffu&&
        raw[0x7fc]==0&&raw[0x7fd]==0xa5);
}

}
