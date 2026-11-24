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
    virtual void slot49(){assert(false);}
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
    unsigned char pad[0x168]{};
    const void* edict=nullptr;
    void* resolved=nullptr;
    bool player=false;
    unsigned base_calls=0,player_calls=0;
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

}
