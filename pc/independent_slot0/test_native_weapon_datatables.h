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



struct HologramFixture {
    HologramControl* ctrl=nullptr;
    float now=0;
    unsigned events[32]{},count=0,fail=0;
    bool push(unsigned event){
        assert(count<32&&ctrl&&ctrl->busy);
        events[count++]=event;return event!=fail;
    }
};
inline int __cdecl holo_clock(void* context,float* now) noexcept {
    auto& f=*static_cast<HologramFixture*>(context);*now=f.now;return f.push(0x2000);
}
inline int __cdecl holo_dirty(void* context,void* object,unsigned offset) noexcept {
    auto& f=*static_cast<HologramFixture*>(context);assert(object==f.ctrl->object);return f.push(offset);
}
inline int __cdecl holo_audio(void* context,void* object,HologramSound s) noexcept {
    auto& f=*static_cast<HologramFixture*>(context);assert(object==f.ctrl->object);return f.push(0x3000+static_cast<unsigned>(s));
}
inline int __cdecl holo_base(void* context,void* object,HologramBaseCall which) noexcept {
    auto& f=*static_cast<HologramFixture*>(context);assert(object==f.ctrl->object);return f.push(0x4000+static_cast<unsigned>(which));
}
inline void check_hologram_behavior(const HologramServerApi& sa,const HologramClientApi& ca,
        const ExtraSendView& sv,const ExtraRecvView& rv){
    alignas(4) unsigned char server[0x101c]{},client[0xd88]{},second[0xd88]{};
    HologramControl control{server,sizeof(server)};
    HologramFixture f{&control,10.0f};
    HologramServerBindings sb{&f,&holo_clock,&holo_dirty,&holo_audio,&holo_base};
    const auto set=[](unsigned char* p,unsigned off,std::uint32_t val){std::memcpy(p+off,&val,4);};
    const auto read=[](unsigned char* p,unsigned off){std::uint32_t val=0;std::memcpy(&val,p+off,4);return val;};
    const auto timer=[&](){float v=0;std::memcpy(&v,server+0x1018,4);return v;};
    const auto call=[&](HologramCommand cmd,float duration=0.0f){f.count=0;return sa.dispatch(&control,&sb,cmd,duration);};
    const auto expect=[&](std::initializer_list<unsigned> seq){
        assert(f.count==seq.size()&&!std::memcmp(f.events,seq.begin(),seq.size()*sizeof(unsigned)));
    };
    const auto update=[&](){
        auto* st=sv.tables[ExHologram];auto* ct=rv.tables[ExHologram];
        for(unsigned i=1;i<3;++i){
            auto& a=st->props[i];auto& b=ct->props[i];s::DVariant v{};
            a.scalarProxy(&a,server,server+a.offset,&v,0,1);
            RecvData d{&b,v,0,1};b.proxy(&d,client,client+b.offset);
        }
    };
    set(server,0x3b0,1);
    assert(call(HologramCommand::On)==1&&read(server,0x1014)==1&&timer()==12&&read(server,0x3b0)==0);
    expect({0x1014,0x2000,0x1018,0x3b0,0x3000,0x3003});
    update();assert(read(client,0xd7c)==1&&read(client,0xd80)==read(server,0x1018));
    HologramFlicker shared{};unsigned visible=9;
    assert(ca.visibility(client,sizeof(client),10,&shared,&visible)&&visible==0&&read(client,0xd74)==0);
    assert(ca.visibility(client,sizeof(client),10.3f,&shared,&visible)&&visible==1&&read(client,0xd74)==1);
    set(second,0xd7c,1);set(second,0xd80,read(client,0xd80));
    assert(ca.visibility(second,sizeof(second),10.3f,&shared,&visible)&&visible==0&&read(second,0xd74)==1);
    assert(ca.visibility(client,sizeof(client),12,&shared,&visible)&&visible==1&&read(client,0xd74)==2);
    // Large time jump consumes one threshold, not the rest of the table.
    assert(ca.visibility(client,sizeof(client),20,&shared,&visible)&&read(client,0xd74)==3);
    f.now=12;assert(call(HologramCommand::Think)==1&&read(server,0x1014)==1);expect({0x4000,0x2000});
    f.now=12.125f;assert(call(HologramCommand::Think)==1&&read(server,0x1014)==2&&timer()==12);
    expect({0x4000,0x2000,0x1014,0x3002});
    update();assert(ca.visibility(client,sizeof(client),f.now,&shared,&visible)&&visible==1&&read(client,0xd74)==3);
    assert(call(HologramCommand::Activate)==1);expect({0x4001,0x3002});
    f.now=20;assert(call(HologramCommand::Pulse,0.5f)==1&&read(server,0x1014)==4&&timer()==20.5f&&
        read(server,0x3b0)==1&&read(server,0x1c)==13);
    expect({0x1014,0x2000,0x1018,0x3b0,0x1c});
    assert(call(HologramCommand::Pulse,1)==1&&!f.count&&timer()==20.5f);
    f.now=20.5f;assert(call(HologramCommand::Think)==1&&read(server,0x1014)==4);
    f.now=21;assert(call(HologramCommand::Think)==1&&read(server,0x1014)==2&&timer()==21&&
        read(server,0x3b0)==0&&read(server,0x1c)==0);
    expect({0x4000,0x2000,0x1014,0x2000,0x1018,0x3b0,0x1c});
    assert(call(HologramCommand::Off)==1&&read(server,0x1014)==3&&timer()==22);
    expect({0x1014,0x2000,0x1018,0x3003,0x3001});
    update();
    // Shutdown uses a one-second progress scale and preserves cursor.
    assert(ca.visibility(client,sizeof(client),21.1f,&shared,&visible)&&read(client,0xd74)==3);
    assert(ca.visibility(client,sizeof(client),21.5f,&shared,&visible)&&read(client,0xd74)==4);
    f.now=22.01f;assert(call(HologramCommand::Think)==1&&read(server,0x1014)==0&&timer()==22);
    expect({0x4000,0x2000,0x1014});
    update();assert(ca.visibility(client,sizeof(client),f.now,&shared,&visible)&&visible==0);
    assert(call(HologramCommand::ResetAfterSpawn)==1&&timer()==0);expect({0x1018});
    assert(call(HologramCommand::ResetAfterSpawn)==1&&!f.count);
    f.fail=0x3000;assert(call(HologramCommand::On)==-1&&control.blocked&&read(server,0x1014)==1);
    expect({0x1014,0x2000,0x1018,0x3000});
    assert(call(HologramCommand::On)==0&&!f.count&&sa.acknowledge(&control)&&!control.blocked);
    f.fail=0;assert(call(HologramCommand::On)==1);expect({0x2000,0x3000,0x3003});
    set(client,0xd74,0xffffffffu);set(client,0xd7c,3);visible=9;
    assert(!ca.visibility(client,sizeof(client),0,&shared,&visible)&&visible==9);
    set(client,0xd74,19);assert(ca.visibility(client,sizeof(client),100,&shared,&visible)&&read(client,0xd74)==19);
}

struct GruntDeathFixture {
    HologramControl* ctrl=nullptr;
    int ragdoll=0;
    bool no_ragdoll=false;
    unsigned suppressed=0,events[20]{},count=0,fail=0,random_count=0;
    float sample=0.5f;
    GruntHeadEffect effect{};
    GruntHeadPath path=GruntHeadPath::None;
    bool push(unsigned e){assert(ctrl&&ctrl->busy&&count<20);events[count++]=e;return e!=fail;}
};
inline int __cdecl grunt_base(void* context,void* source,int arg,void** out) noexcept {
    auto& f=*static_cast<GruntDeathFixture*>(context);assert(source==f.ctrl->object&&arg==1);
    *out=f.no_ragdoll?nullptr:&f.ragdoll;return f.push(1);
}
inline int __cdecl grunt_suppressed(void* context,unsigned* out) noexcept {
    auto& f=*static_cast<GruntDeathFixture*>(context);*out=f.suppressed;return f.push(2);
}
inline int __cdecl grunt_find(void* context,void* ragdoll,const char* name,int* group) noexcept {
    auto& f=*static_cast<GruntDeathFixture*>(context);assert(ragdoll==&f.ragdoll&&!std::strcmp(name,"heads"));
    *group=4;return f.push(3);
}
inline int __cdecl grunt_count(void* context,void* ragdoll,int group,int* count) noexcept {
    auto& f=*static_cast<GruntDeathFixture*>(context);assert(ragdoll==&f.ragdoll&&group==4);
    *count=3;return f.push(4);
}
inline int __cdecl grunt_set(void* context,void* ragdoll,int group,int value) noexcept {
    auto& f=*static_cast<GruntDeathFixture*>(context);assert(ragdoll==&f.ragdoll&&group==4&&value==2);
    return f.push(5);
}
inline int __cdecl grunt_random_float(void* context,float low,float high,float* out) noexcept {
    auto& f=*static_cast<GruntDeathFixture*>(context);assert(low==0&&high==1);
    *out=f.sample;return f.push(6);
}
inline int __cdecl grunt_random_int(void* context,int low,int high,int* out) noexcept {
    auto& f=*static_cast<GruntDeathFixture*>(context);assert(low==0&&high==32767&&f.random_count<3);
    constexpr int vals[]={0,16384,32767};*out=vals[f.random_count++];return f.push(7);
}
inline int __cdecl grunt_orientation(void* context,void* object,float* xyz) noexcept {
    auto& f=*static_cast<GruntDeathFixture*>(context);assert(object==f.ctrl->object);
    xyz[0]=5;xyz[1]=6;xyz[2]=7;return f.push(8);
}
inline int __cdecl grunt_position(void* context,void* object,float* xyz) noexcept {
    auto& f=*static_cast<GruntDeathFixture*>(context);assert(object==f.ctrl->object);
    xyz[0]=8;xyz[1]=9;xyz[2]=10;return f.push(9);
}
inline int __cdecl grunt_emit(void* context,GruntHeadPath path,const GruntHeadEffect* e) noexcept {
    auto& f=*static_cast<GruntDeathFixture*>(context);f.effect=*e;f.path=path;return f.push(10);
}
inline void check_grunt_death(const GruntDeathApi& api,const ExtraSendView& sv,const ExtraRecvView& rv){
    alignas(4) unsigned char source[0xd88]{},server[0xc54]{};
    HologramControl ctrl{source,sizeof(source)};GruntDeathFixture f{&ctrl};
    GruntDeathBindings b{&f,&grunt_base,&grunt_suppressed,&grunt_find,&grunt_count,&grunt_set,
        &grunt_random_float,&grunt_random_int,&grunt_orientation,&grunt_position,&grunt_emit};
    GruntDeathResult result{};
    const auto run=[&](){f.count=0;f.random_count=0;return api.run(&ctrl,&b,1,&result);};
    const auto expect=[&](std::initializer_list<unsigned> events){
        assert(f.count==events.size()&&!std::memcmp(f.events,events.begin(),events.size()*4));
    };
    assert(run()==1&&result.ragdoll==&f.ragdoll&&result.path==GruntHeadPath::None);expect({1});
    // Use the actual original grunt property callbacks, not a manually
    // assigned client flag or independently invented velocity.
    server[0xc44]=1;const float velocity[]={1,-2,3};
    std::memcpy(server+0xc48,velocity,12);
    for(unsigned i=1;i<3;++i){
        auto& s=sv.tables[ExGrunt]->props[i];auto& c=rv.tables[ExGrunt]->props[i];
        s::DVariant v{};s.scalarProxy(&s,server,server+s.offset,&v,0,1);
        RecvData d{&c,v,0,1};c.proxy(&d,source,source+c.offset);
    }
    f.no_ragdoll=true;assert(run()==1&&!result.ragdoll);expect({1});f.no_ragdoll=false;
    f.suppressed=1;assert(run()==1&&result.path==GruntHeadPath::None);expect({1,2});
    f.suppressed=0;
    assert(run()==1&&result.path==GruntHeadPath::EffectRecord);expect({1,2,3,4,5,6,7,7,7,8,9,10});
    assert(f.effect.source==source&&!std::strcmp(f.effect.model,"models/characters/mobgrunt/decaphead.mdl")&&
        f.effect.entity_index==-1&&f.effect.lifetime==0&&f.effect.orientation[1]==6&&f.effect.position[2]==10);
    assert(!std::memcmp(f.effect.velocity,velocity,12)&&f.effect.angles[0]==-180&&
        f.effect.angles[1]>0&&f.effect.angles[1]<0.01f&&f.effect.angles[2]>179.99f&&f.effect.angles[2]<180.01f);
    f.sample=0.1f;assert(run()==1&&result.path==GruntHeadPath::TimedModel);expect({1,2,3,4,5,6,7,7,7,9,10});
    assert(f.effect.lifetime==4&&f.effect.orientation[1]==0);
    f.fail=10;assert(run()==-1&&ctrl.blocked&&result.ragdoll==&f.ragdoll);
    assert(run()==0&&!f.count&&api.acknowledge(&ctrl)&&!ctrl.blocked);
    f.fail=0;assert(run()==1&&result.path==GruntHeadPath::TimedModel);
}
inline void check_extra_entities(const ExtraSendApi& sa,const ExtraRecvApi& ra,
        const AggregateSendApi& asa,const AggregateRecvApi& ara,AggregateSendView& sb,AggregateRecvView& rb,
        const HologramServerApi& hsa,const HologramClientApi& hra,const GruntDeathApi& gda){
    ExtraSendBindings bs{&sb};ExtraRecvBindings br{&rb,{}};
    ExtraSendView sv{};ExtraRecvView rv{};
    assert(!ra.create(&br,&rv)&&!rv.handle);
    for(auto& creator:br.creators)creator=&fixture_class_creator;
    assert(sa.create(&bs,&sv)&&ra.create(&br,&rv)&&!asa.destroy(&sb)&&!ara.destroy(&rb));
    ExtraSendView other{};assert(!sa.create(&bs,&other)&&!other.handle);
    constexpr unsigned order[]={ExAI,ExBludgeon,ExElite,ExGrunt,ExHologram,ExShockwave,ExChaingun,
        ExGrenade,ExMelee,ExNPCPistol,ExSMG};
    // Server case-insensitive sorting: CQuadShockwave follows CNPC_*.
    auto* sr=sv.head;auto* cr=rv.head;
    for(unsigned i=0;i<ExtraEntityCount;++i){
        assert(sr==&sv.records[order[i]]&&cr==&rv.records[ExtraEntityCount-1-i]);
        sr=sr->next;cr=cr->next;
        assert(sv.tables[i]->count==extra_table_counts[i]&&rv.tables[i]->count==extra_table_counts[i]);
        assert(std::strcmp(sv.tables[i]->name,extra_table_names[i])==0&&
            std::strcmp(rv.tables[i]->name,extra_table_names[i])==0);
    }
    assert(!sr&&!cr);
    assert(sv.tables[ExAI]->props[0].table==sb.class_records[ClCombatCharacter].table);
    assert(rv.tables[ExAI]->props[0].table==rb.class_records[ClCombatCharacter].table);
    assert(sv.tables[ExElite]->props[0].table==sv.tables[ExGrunt]&&rv.tables[ExElite]->props[0].table==rv.tables[ExGrunt]);
    assert(sv.tables[ExHologram]->props[0].table==sv.tables[ExAI]&&rv.tables[ExHologram]->props[0].table==rv.tables[ExAI]);
    assert(sv.tables[ExMelee]->props[0].table==sv.tables[ExBludgeon]&&rv.tables[ExMelee]->props[0].table==rv.tables[ExBludgeon]);
    for(unsigned i:{ExSMG,ExChaingun}){
        assert(sv.tables[i]->props[0].table==sb.class_records[ClSelectFire].table);
        assert(rv.tables[i]->props[0].table==rb.class_records[ClSelectFire].table);
    }
    alignas(4) unsigned char server[0x101c]{},client[0xda8]{};
    for(unsigned i=0;i<ExtraEntityCount;++i){
        auto* a=sv.tables[i];auto* b=rv.tables[i];
        for(int j=1;j<a->count;++j){
            auto& x=a->props[j];auto& y=b->props[j];
            assert(std::strcmp(x.name,y.name)==0);
            const unsigned width=x.type==2?12u:x.bits==1?1u:4u;
            const std::uint32_t payload[3]={width==1?1u:x.type?0x3f800000u:3u,0x80000000u,0x40000000u};
            std::memcpy(server+x.offset,payload,width);
            s::DVariant val{};x.scalarProxy(&x,server,server+x.offset,&val,0,1);
            RecvData d{&y,val,0,1};y.proxy(&d,client,client+y.offset);
            assert(!std::memcmp(client+y.offset,payload,width));
        }
    }
    auto& g=sv.tables[ExGrunt]->props[2];assert(g.type==2&&g.bits==0&&g.flags==4&&g.offset==0xc48);
    assert(rv.tables[ExGrunt]->props[2].offset==0xd78);
    assert(sv.tables[ExHologram]->props[1].bits==32&&rv.tables[ExHologram]->props[1].offset==0xd7c);
    assert(sv.tables[ExShockwave]->props[1].flags==2&&rv.tables[ExShockwave]->props[1].offset==0xd64);
    check_hologram_behavior(hsa,hra,sv,rv);
    check_grunt_death(gda,sv,rv);
    sv.tables[ExAI]->written=true;rv.records[ExElite].class_id=7;
    assert(!sa.destroy(&sv)&&!ra.destroy(&rv));
    sv.tables[ExAI]->written=false;rv.records[ExElite].class_id=0;
    assert(sa.destroy(&sv)&&ra.destroy(&rv)&&!sv.handle&&!rv.handle);
}

struct KeypadFixture {
    void* server=nullptr;
    unsigned events[16]{},count=0,fail=0;
    const char* text="";
    bool missing_event=false;
    int event_object=0;
    KeypadEvent last=KeypadEvent::Correct;
    bool push(unsigned e){assert(count<16);events[count++]=e;return e!=fail;}
};
inline int __cdecl key_dirty(void* context,void* object,unsigned offset) noexcept {
    auto& f=*static_cast<KeypadFixture*>(context);assert(object==f.server&&offset==0x3c8);return f.push(1);
}
inline int __cdecl key_output(void* context,void* object,unsigned offset,void* activator,void* caller,float delay) noexcept {
    auto& f=*static_cast<KeypadFixture*>(context);assert(object==f.server&&activator==object&&caller==object&&delay==0);
    assert(offset==0x3d0||offset==0x3e8);return f.push(offset);
}
inline int __cdecl key_append(void* context,int command) noexcept {
    assert(command==7);return static_cast<KeypadFixture*>(context)->push(2);
}
inline int __cdecl key_read(void* context,char* buffer,unsigned size) noexcept {
    auto& f=*static_cast<KeypadFixture*>(context);assert(size==255);
    std::memcpy(buffer,f.text,std::strlen(f.text)+1);return f.push(3);
}
inline int __cdecl key_create_event(void* context,const char* name,unsigned force,void** event) noexcept {
    auto& f=*static_cast<KeypadFixture*>(context);assert(force==1);
    if(!std::strcmp(name,"keypad_combo_entered"))f.last=KeypadEvent::Correct;
    else {assert(!std::strcmp(name,"bad_keypad_combo_entered"));f.last=KeypadEvent::Incorrect;}
    *event=f.missing_event?nullptr:&f.event_object;return f.push(4);
}
inline int __cdecl key_fire(void* context,void* event,unsigned local_only) noexcept {
    auto& f=*static_cast<KeypadFixture*>(context);assert(event==&f.event_object&&!local_only);return f.push(5);
}
inline int __cdecl key_clear(void* context) noexcept {return static_cast<KeypadFixture*>(context)->push(6);}
inline int __cdecl key_base(void* context,int command) noexcept {
    assert(command==7);return static_cast<KeypadFixture*>(context)->push(7);
}

struct PropKeyFixture {
    void* server=nullptr;
    void* client=nullptr;
    unsigned events[32]{},count=0,fail=0;
    int token=0,index=17,received=17;
    bool missing=false;
    char command[256]{},output[32]{},sound[266]{},display[32]{};
    bool null_output=false;
    bool push(unsigned n){assert(count<32);events[count++]=n;return n!=fail;}
};
inline int __cdecl pk_dirty(void* context,void* obj,unsigned off) noexcept {
    auto& f=*static_cast<PropKeyFixture*>(context);assert(obj==f.server&&(off==0x3c8||off==0x3c9));return f.push(1);
}
inline int __cdecl pk_index(void* context,void* obj,int* index) noexcept {
    auto& f=*static_cast<PropKeyFixture*>(context);assert(obj==f.server||obj==f.client);*index=f.index;return f.push(2);
}
inline int __cdecl pk_event_index(void* context,void* event,int* index) noexcept {
    auto& f=*static_cast<PropKeyFixture*>(context);assert(event==&f.token);*index=f.received;return f.push(3);
}
inline int __cdecl pk_event_command(void* context,void* event,char* text,unsigned size) noexcept {
    auto& f=*static_cast<PropKeyFixture*>(context);assert(event==&f.token&&size==256);
    std::memcpy(text,f.command,std::strlen(f.command)+1);return f.push(4);
}
inline int __cdecl pk_output(void* context,void* obj,unsigned off,const char* text,void* activator,void* caller,float delay) noexcept {
    auto& f=*static_cast<PropKeyFixture*>(context);assert(obj==f.server&&off==0x3ec&&activator==obj&&caller==obj&&delay==0);
    f.null_output=!text;std::memcpy(f.output,text?text:"",text?std::strlen(text)+1:1);return f.push(5);
}
inline int __cdecl pk_sound(void* context,void* obj,const char* sound) noexcept {
    auto& f=*static_cast<PropKeyFixture*>(context);assert(obj==f.server);
    std::memcpy(f.sound,sound,std::strlen(sound)+1);return f.push(6);
}
inline int __cdecl pk_base_event(void* context,void* obj,void* event) noexcept {
    auto& f=*static_cast<PropKeyFixture*>(context);assert(obj==f.server&&event==&f.token);return f.push(7);
}
inline int __cdecl pk_create(void* context,const char* name,unsigned force,void** event) noexcept {
    auto& f=*static_cast<PropKeyFixture*>(context);assert(!std::strcmp(name,"prop_keypad_input")&&force==1);
    *event=f.missing?nullptr:&f.token;return f.push(8);
}
inline int __cdecl pk_set_index(void* context,void* event,const char* key,int value) noexcept {
    auto& f=*static_cast<PropKeyFixture*>(context);assert(event==&f.token&&!std::strcmp(key,"entindex"));
    f.received=value;return f.push(9);
}
inline int __cdecl pk_set_command(void* context,void* event,const char* key,const char* value) noexcept {
    auto& f=*static_cast<PropKeyFixture*>(context);assert(event==&f.token&&!std::strcmp(key,"Command"));
    std::memcpy(f.command,value,std::strlen(value)+1);return f.push(10);
}
inline int __cdecl pk_fire(void* context,void* event,unsigned local) noexcept {
    auto& f=*static_cast<PropKeyFixture*>(context);assert(event==&f.token&&local==0);return f.push(11);
}
inline int __cdecl pk_base_command(void* context,const char*) noexcept {return static_cast<PropKeyFixture*>(context)->push(12);}
inline int __cdecl pk_display(void* context,const char* text) noexcept {
    auto& f=*static_cast<PropKeyFixture*>(context);std::memcpy(f.display,text,std::strlen(text)+1);return f.push(13);
}
inline int __cdecl pk_base_tick(void* context) noexcept {return static_cast<PropKeyFixture*>(context)->push(14);}
inline void check_prop_keypad(const PropKeypadServerApi& sa,const PropKeypadClientApi& ca,const KeypadSendView& sv,const KeypadRecvView& rv){
    alignas(4) unsigned char server[0x41c]{},client[0x538]{};
    HologramControl ctrl{server,sizeof(server)};PropKeyFixture f{server,client};
    PropKeypadServerBindings sb{&f,&pk_dirty,&pk_index,&pk_event_index,&pk_event_command,&pk_output,&pk_sound,&pk_base_event};
    PropKeypadClientBindings cb{&f,&pk_create,&pk_index,&pk_set_index,&pk_set_command,&pk_fire,&pk_base_command,&pk_display,&pk_base_tick};
    const auto expect=[&](std::initializer_list<unsigned> seq){assert(f.count==seq.size()&&!std::memcmp(f.events,seq.begin(),seq.size()*4));f.count=0;};
    const auto transfer=[&](){
        for(unsigned i=1;i<3;++i){
            auto& a=sv.tables[KeyProp]->props[i];auto& b=rv.tables[KeyProp]->props[i];
            s::DVariant v{};a.scalarProxy(&a,server,server+a.offset,&v,0,1);
            RecvData d{&b,v,0,17};b.proxy(&d,client,client+b.offset);
        }
    };
    const auto event=[&](){return sa.event(&ctrl,&sb,"prop_keypad_input",&f.token);};
    assert(sa.set_input(&ctrl,&sb,"12")==1&&!std::strcmp(f.output,"12"));expect({1,5});
    transfer();assert(ca.tick(client,sizeof(client),&cb)==1&&!std::strcmp(f.display,"12"));expect({13,14});
    assert(ca.command(client,sizeof(client),&cb,"3")==1);expect({8,2,9,10,11,12});
    assert(event()==1&&!std::strcmp(f.output,"123")&&!std::strcmp(f.sound,"telephone.3"));expect({2,3,4,1,5,6});
    transfer();assert(ca.tick(client,sizeof(client),&cb)==1&&!std::strcmp(f.display,"123"));expect({13,14});
    f.received=18;assert(event()==1&&!std::strcmp(f.output,"123"));expect({2,3});
    assert(sa.event(&ctrl,&sb,"another_event",&f.token)==1);expect({7});
    assert(sa.enable(&ctrl,&sb,0)==1);expect({1});assert(sa.enable(&ctrl,&sb,0)==1);expect({});
    transfer();assert(ca.command(client,sizeof(client),&cb,"9")==1);expect({12});
    assert(ca.tick(client,sizeof(client),&cb)==1);expect({14});
    f.received=17;std::memcpy(f.command,"45",3);
    assert(event()==1&&!std::strcmp(f.output,"1234")&&!std::strcmp(f.sound,"telephone.45"));expect({2,3,4,1,5,6});
    assert(sa.enable(&ctrl,&sb,1)==1);expect({1});transfer();
    f.missing=true;assert(ca.command(client,sizeof(client),&cb,"5")==1);expect({8,12});f.missing=false;
    assert(ca.command(nullptr,0,&cb,"5")==1);expect({12});assert(ca.tick(nullptr,0,&cb)==1);expect({14});
    char long_text[40];std::memset(long_text,'8',39);long_text[39]=0;
    server[0x3c9+31]=0xa5;
    assert(sa.set_input(&ctrl,&sb,long_text)==1&&std::strlen(f.output)==30&&server[0x3c9+30]==0&&server[0x3c9+31]==0xa5);expect({1,5});
    std::memcpy(f.command,"7",2);assert(event()==1&&std::strlen(f.output)==31&&f.output[30]=='7');expect({2,3,4,1,5,6});
    assert(event()==1&&std::strlen(f.output)==31);expect({2,3,4,1,5,6});
    assert(sa.set_input(&ctrl,&sb,nullptr)==1&&f.null_output&&server[0x3c9]==0);expect({1,5});
    f.command[0]=0;assert(event()==1&&f.null_output&&!std::strcmp(f.sound,"telephone."));expect({2,3,4,1,5,6});
    f.fail=6;assert(event()==-1&&ctrl.blocked);expect({2,3,4,1,5,6});
    assert(sa.set_input(&ctrl,&sb,"0")==0&&sa.acknowledge(&ctrl));expect({});
    f.fail=0;assert(sa.set_input(&ctrl,&sb,"0")==1);expect({1,5});
}
inline void check_keypad(const KeypadSendApi& sa,const KeypadRecvApi& ra,const KeypadServerApi& server_api,
        const KeypadClientApi& client_api,const AggregateSendApi& asa,const AggregateRecvApi& ara,
        AggregateSendView& base_s,AggregateRecvView& base_r,const PropKeypadServerApi& pks,const PropKeypadClientApi& pkc){
    KeypadSendBindings sb{&base_s};KeypadRecvBindings rb{&base_r,{}};
    KeypadSendView sv{};KeypadRecvView rv{};
    assert(!ra.create(&rb,&rv));for(auto& creator:rb.creators)creator=&fixture_class_creator;
    assert(sa.create(&sb,&sv)&&ra.create(&rb,&rv)&&!asa.destroy(&base_s)&&!ara.destroy(&base_r));
    assert(sv.head==&sv.records[KeyCode]&&sv.head->next==&sv.records[KeyProp]&&sv.head->next->next==&sv.records[KeyScreen]);
    assert(rv.head==&rv.records[KeyProp]&&rv.head->next==&rv.records[KeyCode]&&rv.head->next->next==&rv.records[KeyScreen]);
    assert(sv.tables[KeyScreen]->props[0].table==base_s.class_records[ClEntity].table);
    assert(rv.tables[KeyScreen]->props[0].table==base_r.class_records[ClEntity].table);
    for(unsigned i:{KeyCode,KeyProp}){
        assert(sv.tables[i]->props[0].table==sv.tables[KeyScreen]&&rv.tables[i]->props[0].table==rv.tables[KeyScreen]);
        assert(!std::strcmp(sv.tables[i]->name,keypad_tables[i])&&!std::strcmp(rv.tables[i]->name,keypad_tables[i]));
    }
    alignas(4) unsigned char server[0x400]{},client[0x610]{};
    auto transfer=[&](s::SendProp& s,RecvProp& c){
        s::DVariant v{};s.scalarProxy(&s,server,server+s.offset,&v,0,1);
        RecvData d{&c,v,0,1};c.proxy(&d,client,client+c.offset);
    };
    for(int i=1;i<7;++i){
        auto& s=sv.tables[KeyScreen]->props[i];auto& c=rv.tables[KeyScreen]->props[i];
        const std::uint32_t val=i<3?0x41200000u:3u;
        std::memcpy(server+s.offset,&val,4);transfer(s,c);assert(!std::memcmp(client+c.offset,&val,4));
    }
    const char* code="1234";std::memcpy(server+0x3cc,&code,4);
    auto& combo=sv.tables[KeyCode]->props[2];auto& combo_r=rv.tables[KeyCode]->props[2];
    transfer(combo,combo_r);assert(!std::strcmp(reinterpret_cast<char*>(client)+0x511,"1234")&&combo.flags==0x200&&combo_r.string_size==255);
    KeypadFixture f{server};HologramControl ctrl{server,sizeof(server)};
    KeypadServerBindings b{&f,&key_dirty,&key_output};
    KeypadClientBindings cb{&f,&key_append,&key_read,&key_create_event,&key_fire,&key_clear,&key_base};
    const auto expect=[&](std::initializer_list<unsigned> seq){assert(f.count==seq.size()&&!std::memcmp(f.events,seq.begin(),seq.size()*4));f.count=0;};
    const auto run=[&](const char* text){f.text=text;return client_api.command(client,sizeof(client),&cb,7);};
    assert(server_api.enable(&ctrl,&b,0)==1&&server[0x3c8]==1);expect({1});
    assert(server_api.enable(&ctrl,&b,0)==1);expect({});
    transfer(sv.tables[KeyCode]->props[1],rv.tables[KeyCode]->props[1]);
    assert(run("1234")==1);expect({7});
    assert(server_api.event(&ctrl,&b,"keypad_combo_entered")==1);expect({0x3d0});
    assert(server_api.enable(&ctrl,&b,1)==1&&server[0x3c8]==0);expect({1});
    transfer(sv.tables[KeyCode]->props[1],rv.tables[KeyCode]->props[1]);
    assert(run("12")==1);expect({2,3,7});
    assert(run("1234")==1&&f.last==KeypadEvent::Correct);expect({2,3,4,5,6,7});
    assert(run("9999")==1&&f.last==KeypadEvent::Incorrect);expect({2,3,4,5,6,7});
    f.missing_event=true;assert(run("12345")==1);expect({2,3,4,6,7});
    assert(client_api.command(nullptr,0,&cb,7)==1);expect({7});
    assert(server_api.event(&ctrl,&b,"bad_keypad_combo_entered")==1);expect({0x3e8});
    assert(server_api.event(&ctrl,&b,"other_event")==1);expect({});
    f.fail=0x3d0;assert(server_api.event(&ctrl,&b,"keypad_combo_entered")==-1&&ctrl.blocked);expect({0x3d0});
    assert(server_api.enable(&ctrl,&b,0)==0&&server_api.acknowledge(&ctrl));f.fail=0;
    code=nullptr;std::memcpy(server+0x3cc,&code,4);transfer(combo,combo_r);assert(client[0x511]==0);
    std::memset(server+0x3c9,'X',32);std::memset(client+0x511,0xa5,33);
    transfer(sv.tables[KeyProp]->props[2],rv.tables[KeyProp]->props[2]);
    assert(client[0x511+30]=='X'&&client[0x511+31]==0&&client[0x511+32]==0xa5);
    check_prop_keypad(pks,pkc,sv,rv);
    sv.tables[KeyScreen]->written=true;rv.tables[KeyProp]->initialized=true;
    assert(!sa.destroy(&sv)&&!ra.destroy(&rv));sv.tables[KeyScreen]->written=false;rv.tables[KeyProp]->initialized=false;
    assert(sa.destroy(&sv)&&ra.destroy(&rv));
}
inline void check_aggregate(const AggregateSendApi& sa,const AggregateRecvApi& ra,const ExtraSendApi& esa,const ExtraRecvApi& era,const HologramServerApi& hsa,const HologramClientApi& hra,const GruntDeathApi& gda,
        const KeypadSendApi& ksa,const KeypadRecvApi& kra,const KeypadServerApi& ks,const KeypadClientApi& kc,
        const PropKeypadServerApi& pks,const PropKeypadClientApi& pkc){
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
    check_extra_entities(esa,era,sa,ra,sv,rv,hsa,hra,gda);
    check_keypad(ksa,kra,ks,kc,sa,ra,sv,rv,pks,pkc);
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

struct RegistryFixture {
    const ClientRegistryApi* api=nullptr;
    ClientRegistryView* view=nullptr;
    unsigned adds=0,removes=0;
    std::uint32_t last=0;
};
struct RegistryObject {
    void** table=nullptr;
    unsigned char pad[0xb0]{};
    std::uint32_t handle=0xffffffffu;
};
static_assert(offsetof(RegistryObject,handle)==0xb4);
inline void __fastcall fixture_set_handle(void* object,void*,const std::uint32_t* value){
    static_cast<RegistryObject*>(object)->handle=*value;
}
inline void* registry_object_table[2]{nullptr,reinterpret_cast<void*>(&fixture_set_handle)};
inline void __cdecl registry_added(void* context,void* object,std::uint32_t handle) noexcept {
    auto& f=*static_cast<RegistryFixture*>(context);++f.adds;f.last=handle;
    assert(static_cast<RegistryObject*>(object)->handle==handle&&f.api->resolve(f.view,handle)==object);
    assert(!f.api->remove(f.view,handle)&&!f.api->destroy(f.view));
}
inline void __cdecl registry_removed(void* context,void* object,std::uint32_t handle) noexcept {
    auto& f=*static_cast<RegistryFixture*>(context);++f.removes;f.last=handle;
    assert(static_cast<RegistryObject*>(object)->handle==0xffffffffu&&f.api->resolve(f.view,handle)==object);
    assert(!f.api->remove(f.view,handle)&&!f.api->destroy(f.view));
}
inline void check_client_registry(const ClientRegistryApi& api){
    std::uint16_t seeds[client_network_limit]{};seeds[7]=0x7fff;seeds[2047]=5;
    ClientRegistryView view{};RegistryFixture f{&api,&view};
    ClientRegistryBindings b{seeds,&f,&registry_added,&registry_removed};
    seeds[8]=0x8000;assert(!api.create(&b,&view)&&!view.handle);seeds[8]=0;
    assert(api.create(&b,&view)&&!api.create(&b,&view));
    assert(view.entries[7].serial==0x7fff&&view.entries[7].prev==&view.entries[7]);
    ClientRegistryState state{};
    assert(api.state(&view,&state)&&!state.count&&!state.head&&!state.tail);
    RegistryObject a{registry_object_table},bb{registry_object_table},cc{registry_object_table},bad{};
    std::uint32_t ha=0,hb=0,hc=0,unchanged=0xdeadbeefu;
    assert(!api.insert(&view,&a,-1,1,&unchanged)&&!api.insert(&view,&a,2048,1,&unchanged)&&
           !api.insert(&view,&a,7,0x8000,&unchanged)&&!api.insert(&view,&bad,7,1,&unchanged)&&unchanged==0xdeadbeefu);
    assert(api.insert(&view,&a,7,-1,&ha)&&ha==0x07fff007u);
    assert(api.insert(&view,&bb,2047,-1,&hb)&&hb==0x57ffu);
    assert(api.insert(&view,&cc,0,1234,&hc)&&hc==(1234u<<12));
    assert(f.adds==3&&api.state(&view,&state)&&state.count==3&&state.head==&view.entries[7]&&state.tail==&view.entries[0]);
    assert(state.head->next==&view.entries[2047]&&state.tail->prev==&view.entries[2047]);
    assert(!api.insert(&view,&a,8,1,&unchanged)&&!api.insert(&view,&bad,7,1,&unchanged)&&!api.destroy(&view));
    assert(api.resolve(&view,ha)==&a&&api.resolve(&view,ha^0x1000)==nullptr&&
           api.resolve(&view,0xffffffffu)==nullptr&&api.resolve(&view,2048)==nullptr);
    assert(api.remove(&view,hb)&&!api.remove(&view,hb)&&!api.resolve(&view,hb));
    assert(view.entries[2047].serial==6&&view.entries[2047].next==&view.entries[2047]);
    assert(state.head->next==state.tail&&state.tail->prev==state.head);
    assert(api.remove(&view,ha)&&view.entries[7].serial==0);
    assert(api.state(&view,&state)&&state.count==1&&state.head==state.tail&&state.head==&view.entries[0]);
    assert(api.insert(&view,&a,7,-1,&ha)&&ha==7&&api.resolve(&view,ha)==&a);
    assert(api.remove(&view,hc)&&api.remove(&view,ha));
    assert(f.adds==4&&f.removes==4&&api.state(&view,&state)&&!state.count&&!state.head&&!state.tail);
    assert(api.destroy(&view)&&!view.handle&&!view.entries);
}

struct InitFixture {
    const ClientRegistryApi* registry=nullptr;
    ClientRegistryView* view=nullptr;
    alignas(4) unsigned char object[0x1250]{};
    unsigned events[32]{},event_count=0;
    std::uint32_t handle=0;
    unsigned fail=0;
    const ClientInterpMap* map=nullptr;
    ClientInitStage rollback_stage=ClientInitStage::CollisionAttempted;
    void event(unsigned e){assert(event_count<32);events[event_count++]=e;}
};
struct InitWatcher {
    void** table;
    InitFixture* fixture;
    unsigned id;
    void* data=nullptr;
    int kind=0;
    float duration=0;
};
inline InitFixture& init_fixture(void* object){
    InitFixture* f=nullptr;std::memcpy(&f,static_cast<unsigned char*>(object)+0x10,4);return *f;
}
inline void __fastcall init_set_handle(void* object,void*,const std::uint32_t* handle){
    auto& f=init_fixture(object);f.event(*handle==0xffffffffu?90u:1u);
    std::memcpy(static_cast<unsigned char*>(object)+0xb4,handle,4);
}
inline void* __fastcall init_primary(void* secondary,void*){
    return static_cast<unsigned char*>(secondary)-4;
}
inline const ClientInterpMap* __fastcall init_map(void* object,void*){
    auto& f=init_fixture(object);f.event(4);return f.map;
}
inline void __fastcall init_watcher_setup(void* object,void*,void* data,int kind){
    auto& w=*static_cast<InitWatcher*>(object);w.fixture->event(10+w.id);
    w.data=data;w.kind=kind;
}
inline int __fastcall init_watcher_flags(void* object,void*){
    auto& w=*static_cast<InitWatcher*>(object);w.fixture->event(20+w.id);
    return static_cast<int>(w.id);
}
inline void __fastcall init_watcher_duration(void* object,void*,float duration){
    auto& w=*static_cast<InitWatcher*>(object);w.fixture->event(40+w.id);w.duration=duration;
}
inline void* init_primary_table[18]{};
inline void* init_secondary_table[1]{reinterpret_cast<void*>(&init_primary)};
inline void* init_watcher_table[6]{reinterpret_cast<void*>(&init_watcher_setup),
    reinterpret_cast<void*>(&init_watcher_duration),nullptr,nullptr,nullptr,reinterpret_cast<void*>(&init_watcher_flags)};
inline void __cdecl init_added(void* context,void* object,std::uint32_t handle) noexcept {
    auto& f=*static_cast<InitFixture*>(context);f.event(2);f.handle=handle;
    assert(object==f.object&&f.registry->resolve(f.view,handle)==object);
}
inline void __cdecl init_removed(void* context,void* object,std::uint32_t handle) noexcept {
    auto& f=*static_cast<InitFixture*>(context);f.event(91);
    assert(object==f.object&&handle==f.handle&&f.registry->resolve(f.view,handle)==object);
}
inline int __cdecl init_collision(void* context,void* collision) noexcept {
    auto& f=*static_cast<InitFixture*>(context);f.event(3);
    assert(collision==f.object+0x16c&&f.registry->resolve(f.view,f.handle)==f.object);
    assert(!f.registry->remove(f.view,f.handle));
    return f.fail!=1;
}
inline int __cdecl init_duration(void* context,void* object,int flags,float* result) noexcept {
    auto& f=*static_cast<InitFixture*>(context);f.event(30+static_cast<unsigned>(flags));
    assert(object==f.object);
    if(f.fail==2&&flags==2)return 0;
    *result=static_cast<float>(flags)*0.125f;return 1;
}
inline void __cdecl init_rollback(void* context,void* object,ClientInitStage stage) noexcept {
    auto& f=*static_cast<InitFixture*>(context);f.event(80);f.rollback_stage=stage;
    assert(object==f.object&&f.registry->resolve(f.view,f.handle)==object);
}
inline void check_client_init(const ClientRegistryApi& registry,const ClientInitApi& api){
    InitFixture f;ClientRegistryView view{};f.registry=&registry;f.view=&view;
    std::uint16_t seeds[client_network_limit]{};
    ClientRegistryBindings rb{seeds,&f,&init_added,&init_removed};
    assert(registry.create(&rb,&view));
    init_primary_table[1]=reinterpret_cast<void*>(&init_set_handle);
    init_primary_table[17]=reinterpret_cast<void*>(&init_map);
    void** pv=init_primary_table;void** sv=init_secondary_table;
    std::memcpy(f.object,&pv,4);std::memcpy(f.object+4,&sv,4);
    InitFixture* fp=&f;std::memcpy(f.object+0x10,&fp,4);
    int data[2]={123,456};
    InitWatcher first{init_watcher_table,&f,1},second{init_watcher_table,&f,2};
    ClientInterpEntry a{7,&data[0],&first},b{9,&data[1],&second};
    ClientInterpMap base{&b,1,0,1,&b,0,nullptr};
    ClientInterpMap derived{&a,1,0,1,&a,0,&base};f.map=&derived;
    ClientInitBindings ib{&view,&f,&init_collision,&init_duration,&init_rollback};
    std::uint32_t handle=0;
    assert(api.initialize(&ib,ClPlayer,f.object,5,17,&handle)==1&&handle==0x11005);
    const unsigned expected[]={1,2,3,4,11,21,31,41,12,22,32,42};
    assert(f.event_count==12&&!std::memcmp(f.events,expected,sizeof(expected)));
    int index=0;std::memcpy(&index,f.object+0x50,4);
    assert(index==5&&first.data==&data[0]&&second.data==&data[1]&&first.kind==7&&second.kind==9&&
        first.duration==0.125f&&second.duration==0.25f);
    assert(registry.remove(&view,handle)&&f.events[12]==90&&f.events[13]==91);
    f.event_count=0;f.fail=1;handle=0xdeadbeefu;
    assert(!api.initialize(&ib,ClPlayer,f.object,6,18,&handle)&&handle==0xdeadbeefu);
    const unsigned failed[]={1,2,3,80,90,91};
    assert(f.event_count==6&&!std::memcmp(f.events,failed,sizeof(failed))&&
        f.rollback_stage==ClientInitStage::CollisionAttempted&&!registry.resolve(&view,f.handle));
    f.event_count=0;f.fail=2;
    assert(!api.initialize(&ib,ClPlayer,f.object,6,18,&handle)&&f.rollback_stage==ClientInitStage::InterpolationAttempted);
    assert(f.event_count==14&&f.events[10]==32&&f.events[11]==80&&f.events[12]==90&&f.events[13]==91);
    std::memcpy(&index,f.object+0x50,4);assert(index==5); // last successful index remains
    f.fail=0;f.event_count=0;base.parent=&derived;
    assert(!api.initialize(&ib,ClPlayer,f.object,6,18,&handle));
    const unsigned malformed[]={1,2,3,4,80,90,91};
    assert(f.event_count==7&&!std::memcmp(f.events,malformed,sizeof(malformed)));
    base.parent=nullptr;f.event_count=0;f.map=nullptr;
    assert(api.initialize(&ib,ClHand,f.object,7,19,&handle)==1&&f.event_count==4);
    assert(registry.remove(&view,handle)&&registry.destroy(&view));
}

struct SpawnFixture {
    InitFixture init;
    unsigned allocations=0,constructions=0,cleanups=0;
    bool allocated=false;
    CreatorCleanup stage=CreatorCleanup::Raw;
};
inline void* __cdecl spawn_allocate(void* context,unsigned,unsigned bytes) noexcept {
    auto& f=*static_cast<SpawnFixture*>(context);assert(!f.allocated&&bytes<=sizeof(f.init.object));
    ++f.allocations;f.allocated=true;std::memset(f.init.object,0xa5,sizeof(f.init.object));return f.init.object;
}
inline int __cdecl spawn_construct(void* context,unsigned,void* object,unsigned bytes) noexcept {
    auto& f=*static_cast<SpawnFixture*>(context);++f.constructions;
    auto* raw=static_cast<unsigned char*>(object);
    for(unsigned i=0;i<bytes;++i)assert(raw[i]==0);
    init_primary_table[1]=reinterpret_cast<void*>(&init_set_handle);
    init_primary_table[17]=reinterpret_cast<void*>(&init_map);
    void** primary=init_primary_table;void** secondary=init_secondary_table;void** networkable=fixture_networkable_table;
    fixture_networkable_table[0]=reinterpret_cast<void*>(&fixture_networkable_method);
    std::memcpy(raw,&primary,4);std::memcpy(raw+4,&secondary,4);std::memcpy(raw+8,&networkable,4);
    InitFixture* fp=&f.init;std::memcpy(raw+0x10,&fp,4);
    return 1;
}
inline void __cdecl spawn_cleanup(void* context,unsigned,void* object,unsigned,CreatorCleanup stage) noexcept {
    auto& f=*static_cast<SpawnFixture*>(context);assert(f.allocated&&object==f.init.object);
    assert(!f.init.registry->resolve(f.init.view,f.init.handle));
    ++f.cleanups;f.allocated=false;f.stage=stage;
}

inline void check_spawned_network_transfer(const AggregateSendView& sv,const AggregateRecvView& rv,
        const ClientSpawnRecord& record,sin_independent::EntityHandleEntry* entries){
    assert(record.initialized&&record.primary&&record.networkable==static_cast<unsigned char*>(record.primary)+8);
    alignas(4) unsigned char server[0x1520]{};
    auto* client=static_cast<unsigned char*>(record.primary);
    const auto transfer=[&](s::SendProp& a,RecvProp& b,unsigned char* sd,unsigned char* cd,
                            std::uint32_t input,std::uint32_t expected,unsigned width){
        assert(std::strcmp(a.name,b.name)==0&&a.scalarProxy&&b.proxy);
        std::memcpy(sd+a.offset,&input,width);
        s::DVariant v{};a.scalarProxy(&a,sd,sd+a.offset,&v,0,1);
        RecvData d{&b,v,0,1};b.proxy(&d,cd,cd+b.offset);
        assert(!std::memcmp(cd+b.offset,&expected,width));
    };
    const auto* sc=sv.class_records[record.slot].table;
    const auto* cc=rv.class_records[record.slot].table;
    assert(std::strcmp(sc->name,cc->name)==0);
    if(record.slot==ClPlayer){
        auto& sp=sc->props[1];auto& cp=cc->props[1];
        s::Recipients mask{};void* out=nullptr;
        auto* sd=static_cast<unsigned char*>(sp.tableProxy(&sp,server,server+sp.offset,&mask,1));
        cp.table_proxy(&cp,&out,client+cp.offset,1);
        assert(sd==server+0x137c&&out==client+0x11c8&&mask.words[0]==1);
        auto* cd=static_cast<unsigned char*>(out);
        transfer(sp.table->props[2],cp.table->props[2],sd,cd,0x3f400000u,0x3f400000u,4);
        transfer(sp.table->props[4],cp.table->props[4],sd,cd,1,1,1);
        transfer(sp.table->props[10],cp.table->props[10],sd,cd,0x41200000u,0x41200000u,4);
        transfer(sp.table->props[11],cp.table->props[11],sd,cd,0x40a00000u,0x40a00000u,4);
        transfer(sc->props[3],cc->props[3],server,client,0x42000000u,0x42000000u,4);
        transfer(sc->props[5],cc->props[5],server,client,0x437f0000u,0x437f0000u,4);
        auto* sb=sc->props[0].table;auto* cb=cc->props[0].table;
        transfer(sb->props[5],cb->props[6],server,client,75,75,4);
        auto& sl=sb->props[14];auto& cl=cb->props[1];
        assert(sl.tableProxy(&sl,server,server,&mask,1)==server&&mask.words[0]==1);
        cl.table_proxy(&cl,&out,client,1);assert(out==client);
        auto& sam=sl.table->props[5];auto& cam=cl.table->props[5];
        for(unsigned i=0;i<32;++i)
            transfer(sam.table->props[i],cam.table->props[i],server+sam.offset,client+cam.offset,i+10,i+10,4);
        // All callbacks operate on the creator-returned allocation;
        // neither the registry handle nor the entity index is overwritten.
    }else if(record.slot==ClHand){
        assert(sc->props[0].table==sv.class_records[ClEntity].table&&cc->props[0].table==rv.class_records[ClEntity].table);
        transfer(sc->props[0].table->props[9],cc->props[0].table->props[9],server,client,2,2,4);
    }else{
        // Descend the actual class inheritance, not an unrelated global
        // table. All three SiN weapons share BaseCombatWeapon.
        for(unsigned depth=0;depth<4&&std::strcmp(sc->name,"DT_BaseCombatWeapon")!=0;++depth){
            assert(sc->props[0].table&&cc->props[0].table);
            sc=sc->props[0].table;cc=cc->props[0].table;
        }
        assert(std::strcmp(sc->name,"DT_BaseCombatWeapon")==0&&std::strcmp(cc->name,sc->name)==0);
        const std::uint32_t owner=(0x345u<<12)|37u;
        std::memcpy(server+0x494,&owner,4);
        s::Recipients mask{};
        auto& sl=sc->props[1];auto& cl=cc->props[1];
        auto& sa=sc->props[2];auto& ca=cc->props[2];
        assert(sl.tableProxy(&sl,server,server,&mask,1)==server&&mask.words[0]==1);
        assert(sa.tableProxy(&sa,server,server,&mask,1)==server&&mask.words[0]==1);
        void* local=nullptr;void* active=nullptr;
        cl.table_proxy(&cl,&local,client,1);ca.table_proxy(&ca,&active,client,1);
        assert(local==client&&active==client);
        transfer(sl.table->props[0],cl.table->props[0],server,client,7,7,4);
        transfer(sl.table->props[1],cl.table->props[1],server,client,0xffffffffu,0xffffffffu,4);
        transfer(sl.table->props[2],cl.table->props[2],server,client,3,3,4);
        transfer(sa.table->props[0],ca.table->props[0],server,client,0x41200000u,0x41200000u,4);
        transfer(sc->props[5],cc->props[5],server,client,2,2,4);
        transfer(sc->props[6],cc->props[6],server,client,owner,owner,4);
        ++entries[37].serial;
        assert(!sl.tableProxy(&sl,server,server,&mask,1)&&mask.words[0]==0);
        assert(!sa.tableProxy(&sa,server,server,&mask,1)&&mask.words[0]==0);
        transfer(sc->props[6],cc->props[6],server,client,owner,0xffffffffu,4);
        --entries[37].serial;
    }
    std::uint32_t handle=0,index=0;std::memcpy(&handle,client+0xb4,4);std::memcpy(&index,client+0x50,4);
    assert(handle==record.handle&&index==(record.handle&0xfffu));
}
inline void check_spawn_session(const AggregateSendApi& server_api,const AggregateRecvApi& aggregate,const ClientRegistryApi& registry,const ClientSpawnApi& api){
    RecvProp cp[8];RecvTable collision{cp,8,nullptr,"DT_CollisionProperty"};
    EntityNetworkFixture ef;OverlayFixture cf;FovFixture ff;VelocityFixture vf;
    AggregateRecvBindings ab{{&collision,&ef,&sample_clock,&effects_changed,&interpolation_changed},
        {&cf,&reserve_layers,&resize_layers},{&ff,&sample_fov},&vf,&invalidate_velocity,{}};
    for(auto& creator:ab.creators)creator=&fixture_class_creator;
    AggregateRecvView av{};assert(aggregate.create(&ab,&av));
    s::SendProp sp[8];s::SendTable sc;assert(sc.Construct(sp,8,"DT_CollisionProperty"));
    NativeOwner owner,player;owner.resolved=&player;player.player=true;
    sin_independent::EntityHandleEntry entries[64]{};
    entries[37].entity=reinterpret_cast<CBaseEntity*>(&owner);entries[37].serial=0x345;
    alignas(4) unsigned char edicts[2*20]{};NativeEngine engine;engine.expected=edicts+20;engine.result=1;
    player.edict=engine.expected;OverlayFixture sf;
    AggregateSendBindings sb2{{&sc,entries,64,&ef,&sample_clock,&sample_transform},
        {&sf,&reserve_layers},&engine,edicts,1};
    AggregateSendView server_view{};assert(server_api.create(&sb2,&server_view));
    SpawnFixture f;ClientRegistryView rv{};f.init.registry=&registry;f.init.view=&rv;
    std::uint16_t seeds[client_network_limit]{};
    ClientRegistryBindings rb{seeds,&f.init,&init_added,&init_removed};
    assert(registry.create(&rb,&rv));
    ClientSpawnBindings sb{&av,{&f,&spawn_allocate,&spawn_construct,&spawn_cleanup},
        {&rv,&f.init,&init_collision,&init_duration,&init_rollback}};
    ClientSpawnView session{},second{};
    av.tables[19]->initialized=true;assert(!api.bind(&sb,&session));av.tables[19]->initialized=false;
    assert(api.bind(&sb,&session)&&!api.bind(&sb,&second));
    assert(!aggregate.destroy(&av)&&!registry.destroy(&rv));
    constexpr unsigned slots[]={ClPlayer,ClHand,ClMagnum,ClAssault,ClScatter};
    assert(av.class_records[ClEntity].create==&fixture_class_creator);
    ClientSpawnRecord record{};
    for(unsigned i=0;i<5;++i){
        f.init.event_count=0;
        const auto creator=av.class_records[slots[i]].create;
        assert(creator!=&fixture_class_creator&&!creator(-1,1));
        auto* networkable=creator(9,static_cast<int>(10+i));
        assert(networkable==f.init.object+8&&api.record(&session,9,&record)&&
            record.primary==f.init.object&&record.networkable==networkable&&record.slot==slots[i]&&record.initialized==1&&
            record.handle==((10+i)<<12|9u));
        assert(f.init.event_count==4&&registry.resolve(&rv,record.handle)==record.primary);
        check_spawned_network_transfer(server_view,av,record,entries);
        assert(!creator(9,10)&&!api.unbind(&session)&&!api.release(&session,record.handle^0x1000u));
        assert(api.release(&session,record.handle)&&!api.record(&session,9,&record)&&f.stage==CreatorCleanup::InitAttempted);
    }
    assert(f.allocations==5&&f.constructions==5&&f.cleanups==5);
    f.init.event_count=0;f.init.fail=1;
    assert(!av.class_records[ClPlayer].create(9,55)&&!f.allocated&&!api.record(&session,9,&record)&&
        f.stage==CreatorCleanup::Constructed&&f.cleanups==6);
    av.tables[19]->in_main_list=true;assert(!api.unbind(&session));av.tables[19]->in_main_list=false;
    const auto stale=av.class_records[ClPlayer].create;
    assert(api.unbind(&session)&&!session.handle&&!stale(9,55));
    for(unsigned slot:slots)assert(av.class_records[slot].create==&fixture_class_creator);
    assert(aggregate.destroy(&av)&&registry.destroy(&rv)&&server_api.destroy(&server_view));
}

}
