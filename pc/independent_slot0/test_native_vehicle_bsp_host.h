#pragma once
#include "server_native_vehicle_map_host.h"
#include <cassert>
#include <vector>
#include <string>
namespace sin_port::map_host::test {
namespace sm=sin_compat::map;
struct VehicleBspShell final:sm::ISourceMapEntityHost,IVehicleMapShells {
    alignas(4) unsigned char vehicle[veh::vehicle_object_bytes]{},radio[veh::music_radio_bytes]{},views[600]{},station[56]{};
    veh::MusicRadioTrack tracks[4]{};
    veh::HologramControl v{vehicle,sizeof(vehicle)},r{radio,sizeof(radio)};
    EntityRef refs[4]{};
    bool live[4]{};
    unsigned next=0,generation=1,ordinary_spawn=0,ordinary_activate=0,vehicle_spawn=0,vehicle_activate=0,
        radio_spawn=0,radio_activate=0,parents=0,opens=0,closes=0,precaches=0,thinks=0,schedules=0,emits=0,destroys=0;
    bool fail_parse=false,fail_remove=false,fail_link=false;
    float now=10;
    std::vector<std::string> keys;
    const veh::VehicleLifecycleApi& life;
    const veh::MusicRadioApi& playback;
    veh::VehicleLifecycleBindings lb{};
    veh::VehicleMapBindings vb{};
    veh::MusicRadioMapBindings mb{};
    VehicleBspShell(const veh::VehicleLifecycleApi& a,const veh::MusicRadioApi& p):life(a),playback(p){
        lb.context=this;lb.config.context=this;vb.context=this;mb.playback.context=this;
        lb.base_spawn=[](void* p,void* v) noexcept -> int{auto& f=*static_cast<VehicleBspShell*>(p);assert(v==f.vehicle&&f.live[3]&&f.word(f.radio,0x398)==0x42480000);++f.vehicle_spawn;return 1;};
        lb.base_activate=[](void* p,void*) noexcept -> int{++static_cast<VehicleBspShell*>(p)->vehicle_activate;return 1;};
        lb.initialize_subsystem=[](void*,void*,unsigned off) noexcept -> int{return off==0x84c;};
        lb.open_config=[](void*,void*,veh::VehicleConfigKind kind,unsigned* present) noexcept -> int{*present=kind==veh::VehicleConfigKind::Views;return 1;};
        lb.close_config=[](void*,void*,veh::VehicleConfigKind kind) noexcept -> int{return kind==veh::VehicleConfigKind::Views;};
        lb.config.entry=[](void*,const char*,unsigned,unsigned* present,veh::VehicleConfigEntry*) noexcept -> int{*present=0;return 1;};
        lb.config.append_view=[](void* p,void* v) noexcept -> int{auto& f=*static_cast<VehicleBspShell*>(p);const auto n=f.word(v,0x808);assert(n<6);f.put(v,0x7fc,f.ptr(f.views));f.put(v,0x808,n+1);return 1;};
        lb.config.attachment_id=[](void*,void*,const char*,int* out) noexcept -> int{*out=1;return 1;};
        lb.append_sound=[](void*,void*) noexcept -> int{return 0;};lb.precache=[](void*,const char*) noexcept -> int{return 0;};
        lb.find_radio=[](void* p,const char* name,void** out) noexcept -> int{auto& f=*static_cast<VehicleBspShell*>(p);assert(!std::strcmp(name,"env_radio"));*out=f.live[3]&&!f.fail_link?f.radio:nullptr;return 1;};
        lb.handle=[](void* p,void* obj,std::uint32_t* out) noexcept -> int{auto& f=*static_cast<VehicleBspShell*>(p);assert(obj==f.radio||obj==f.vehicle);*out=f.handle(obj==f.radio?3:2);return 1;};
        lb.resolve_radio=[](void* p,std::uint32_t h,veh::HologramControl** out) noexcept -> int{auto& f=*static_cast<VehicleBspShell*>(p);*out=f.live[3]&&h==f.handle(3)?&f.r:nullptr;return 1;};
        lb.parent_radio=[](void* p,void* radio,void* v,int attachment) noexcept -> int{auto& f=*static_cast<VehicleBspShell*>(p);assert(radio==f.radio&&v==f.vehicle&&attachment==-1);++f.parents;return 1;};
        lb.initialize_components=[](void*,void*) noexcept -> int{return 1;};
        lb.clock=[](void* p,float* out) noexcept -> int{*out=static_cast<VehicleBspShell*>(p)->now;return 1;};
        lb.health_dirty=[](void*,void*) noexcept -> int{return 1;};
        lb.spawn_tail=[](void*,void*,unsigned flags) noexcept -> int{return flags==0x11;};
        lb.activate_tail=[](void*,void*,unsigned) noexcept -> int{return 0;};
        auto& m=mb;
        m.open_script=[](void* p,std::uint32_t word,void** token) noexcept -> int{auto& f=*static_cast<VehicleBspShell*>(p);assert(word==9&&f.r.busy);++f.opens;*token=&f;return 1;};
        m.entry=[](void*,void*,unsigned i,unsigned* present,const char** station,const char** sound) noexcept -> int{*present=i==0;if(*present){*station="Music";*sound="fixture.track";}return 1;};
        m.close_script=[](void* p,void*) noexcept -> int{++static_cast<VehicleBspShell*>(p)->closes;return 1;};
        m.append_station=[](void* p,void* r) noexcept -> int{auto& f=*static_cast<VehicleBspShell*>(p);assert(!f.word(r,0x3b4));f.put(r,0x3a8,f.ptr(f.station));f.put(r,0x3b4,1);return 1;};
        m.append_track=[](void* p,void*,int station,const veh::MusicRadioTrack* t) noexcept -> int{auto& f=*static_cast<VehicleBspShell*>(p);assert(!station);auto n=f.word(f.station,0x30);assert(n<4);f.tracks[n]=*t;f.put(f.station,0x24,f.ptr(f.tracks));f.put(f.station,0x30,n+1);return 1;};
        m.precache=[](void* p,const char* name) noexcept -> int{auto& f=*static_cast<VehicleBspShell*>(p);assert(!std::strcmp(name,"fixture.track"));++f.precaches;return 1;};
        m.base_spawn=[](void* p,void* radio) noexcept -> int{auto& f=*static_cast<VehicleBspShell*>(p);assert(radio==f.radio&&f.word(radio,0x394)==f.handle(2));++f.radio_spawn;return 1;};
        m.base_activate=[](void* p,void*) noexcept -> int{++static_cast<VehicleBspShell*>(p)->radio_activate;return 1;};
        m.install_think=[](void* p,void*,const char* name) noexcept -> int{auto& f=*static_cast<VehicleBspShell*>(p);assert(!std::strcmp(name,"RadioThink"));++f.thinks;return 1;};
        m.playback.clock=lb.clock;
        m.playback.schedule=[](void* p,void*,float) noexcept -> int{++static_cast<VehicleBspShell*>(p)->schedules;return 1;};
        m.playback.resolve=[](void* p,std::uint32_t h,void** out) noexcept -> int{auto& f=*static_cast<VehicleBspShell*>(p);*out=f.live[2]&&h==f.handle(2)?f.vehicle:f.live[3]&&h==f.handle(3)?f.radio:nullptr;return 1;};
        m.playback.handle=lb.handle;
        m.playback.emit=[](void* p,void* radio,void* speaker,const char* name,float vol,unsigned channel,unsigned level,unsigned flags,float* duration) noexcept -> int{auto& f=*static_cast<VehicleBspShell*>(p);assert(radio==f.radio&&speaker==f.vehicle&&!std::strcmp(name,"fixture.track")&&vol==0.5f&&channel==5&&level==60&&!flags);++f.emits;*duration=2;return 1;};
        m.playback.stop=[](void*,void*,unsigned,const char*) noexcept -> int{return 1;};
        lb.music=mb;
        vb.radio=mb.playback;vb.resolve=mb.playback.resolve;
        vb.radio_control=[](void* p,void* radio,veh::HologramControl** out) noexcept -> int{auto& f=*static_cast<VehicleBspShell*>(p);*out=f.live[3]&&radio==f.radio?&f.r:nullptr;return 1;};
    }
    unsigned handle(unsigned i)const{return (refs[i].generation<<12)|refs[i].index;}
    unsigned ptr(void* p){return static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(p));}
    void put(void* p,unsigned off,unsigned w){std::memcpy(static_cast<unsigned char*>(p)+off,&w,4);}
    unsigned word(void* p,unsigned off){unsigned w=0;std::memcpy(&w,static_cast<unsigned char*>(p)+off,4);return w;}
    bool is_live(EntityRef e)const override{return e.index>=1&&e.index<=4&&live[e.index-1]&&refs[e.index-1]==e;}
    bool create_entity(std::string_view name,EntityRef& e)override{
        if(next>=4)return false;unsigned i=next++;e={i+1,generation};refs[i]=e;live[i]=true;
        if(name=="prop_vehicle_sin"){assert(i==2);return life.defaults(&v)==1;}
        if(name=="env_radio"){assert(i==3);return playback.initialize(&r,8)==1;}
        return true;
    }
    bool key_value(EntityRef e,std::string_view key,std::string_view value)override{
        if(!is_live(e))return false;keys.emplace_back(std::string(key)+"="+std::string(value));return true;
    }
    bool dispatch_spawn(EntityRef e)override{assert(is_live(e)&&e.index<3);++ordinary_spawn;return true;}
    bool activate(EntityRef e)override{assert(is_live(e)&&e.index<3);++ordinary_activate;return true;}
    bool remove_entity(EntityRef e)override{
        if(!is_live(e)||(fail_remove&&e.index==3))return false;
        live[e.index-1]=false;if(e.index>=3)++destroys;return true;
    }
    veh::HologramControl* control(EntityRef e)override{return !is_live(e)?nullptr:e.index==3?&v:e.index==4?&r:nullptr;}
    bool parse_key(EntityRef e,unsigned type,std::string_view value,std::uint32_t& word)override{
        assert(is_live(e));if(fail_parse)return false;
        if(type==6&&value=="0")word=0;
        else if(type==1&&value=="50")word=0x42480000;
        else if(type==2&&value=="fixture.txt")word=9;
        else return false;
        return true;
    }
};
inline void check_vehicle_bsp(const veh::VehicleMapApi& map,const veh::VehicleLifecycleApi& life,const veh::MusicRadioMapApi& music,
    const veh::MusicRadioApi& playback,const std::vector<std::uint8_t>& bsp){
    VehicleBspShell f(life,playback);
    NativeVehicleMapHost host(f,f,map,life,f.vb,f.lb,music,playback,f.mb);
    sm::SourceMapRuntime runtime(host);
    const auto loaded=runtime.load_bsp(bsp.data(),bsp.size());
    assert(loaded.ok()&&loaded.entities_created==4&&loaded.entities_spawned==4&&loaded.entities_activated==4);
    assert(f.ordinary_spawn==2&&f.ordinary_activate==2&&f.vehicle_spawn==1&&f.vehicle_activate==1&&f.radio_spawn==2&&f.radio_activate==1&&f.parents==1);
    assert(f.opens==3&&f.closes==3&&f.precaches==3&&f.thinks==3&&f.schedules==3&&f.word(f.station,0x30)==3&&f.word(f.vehicle,0x808)==6);
    assert(host.attached_count()==2&&!f.vehicle[0x74a]&&f.keys.size()==7);
    assert(f.keys[0]=="targetname=relay"&&f.keys[1]=="OnTrigger=car,EnableUse,1,0,-1"&&f.keys[2]=="targetname=car"&&f.keys[3]=="UseBox=relay,Trigger,,0,-1"&&f.keys[4]=="UseBox=relay,Trigger,,1,-1"&&f.keys[5]=="ViewChanged=relay,Trigger,,0,-1"&&f.keys[6]=="targetname=radio");
    const auto car=runtime.entities()[2],radio=runtime.entities()[3],relay=runtime.entities()[1];
    assert(host.input(relay,"EnableUse",5,1,nullptr)==VehicleMapResult::Unhandled);
    assert(host.input(car,"enableuse",5,1,nullptr)==VehicleMapResult::Applied&&f.vehicle[0x74a]);
    assert(host.input(car,"RadioOn",0,0,nullptr)==VehicleMapResult::Applied); // already positive volume
    assert(host.radio_think(radio)==VehicleMapResult::Applied&&f.emits==1);
    assert(host.input(radio,"NextStation",0,0,nullptr)==VehicleMapResult::Applied&&f.emits==2);
    assert(host.radio_think(car)==VehicleMapResult::Unhandled);
    f.fail_remove=true;assert(runtime.unload().code==sm::SourceMapLoadCode::PartialUnload&&host.attached_count()==1&&f.destroys==1);
    assert(host.input(radio,"TurnOn",0,0,nullptr)==VehicleMapResult::Rejected);
    f.fail_remove=false;assert(runtime.unload().code==sm::SourceMapLoadCode::Unloaded&&!host.attached_count()&&f.destroys==2);
    assert(host.input(car,"EnableUse",5,1,nullptr)==VehicleMapResult::Rejected);
    VehicleBspShell missing(life,playback);NativeVehicleMapHost no_provider(missing);sm::SourceMapRuntime refused(no_provider);
    assert(refused.load_bsp(bsp.data(),bsp.size()).code==sm::SourceMapLoadCode::HostCreateFailed&&!refused.loaded());
    VehicleBspShell bad(life,playback);bad.fail_parse=true;NativeVehicleMapHost badhost(bad,bad,map,life,bad.vb,bad.lb,music,playback,bad.mb);
    sm::SourceMapRuntime rollback(badhost);assert(rollback.load_bsp(bsp.data(),bsp.size()).code==sm::SourceMapLoadCode::HostKeyValueFailed&&!rollback.loaded()&&bad.destroys==1&&!badhost.attached_count());
    VehicleBspShell absent(life,playback);absent.fail_link=true;NativeVehicleMapHost ah(absent,absent,map,life,absent.vb,absent.lb,music,playback,absent.mb);
    sm::SourceMapRuntime ar(ah);assert(ar.load_bsp(bsp.data(),bsp.size()).code==sm::SourceMapLoadCode::HostSpawnFailed&&!ar.loaded()&&absent.destroys==2&&!ah.attached_count());
}
}
