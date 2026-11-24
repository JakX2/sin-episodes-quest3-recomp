#pragma once
#include "server_native_radio_map_host.h"
#include <cassert>
#include <vector>
#include <string>
#include <cstring>
namespace sin_port::map_host::test {
namespace sm=sin_compat::map;
struct RadioBspShell final:sm::ISourceMapEntityHost,IRadioShellState {
    struct Slot {
        alignas(4) unsigned char storage[radio::radio_entity_bytes]{};
        radio::HologramControl control{storage,sizeof(storage)};
        EntityRef ref{};
        bool is_radio=false,live=false;
    };
    Slot slots[4]{};
    unsigned next=0,generation=1,ordinary_spawn=0,ordinary_activate=0,spawn_helpers=0,radio_activate=0,destroys=0;
    unsigned precaches=0,outputs=0;
    bool fail_parse=false,fail_remove=false;
    std::vector<std::string> keys;
    const radio::RadioEntityApi& api;
    radio::RadioEntityBindings b{};
    explicit RadioBspShell(const radio::RadioEntityApi& a):api(a){
        b.message.context=this;
        b.base_construct=[](void*,void*,unsigned bytes) noexcept -> int{return bytes==radio::radio_entity_bytes;};
        b.class_init=[](void*,void*,const char* name) noexcept -> int{return !std::strcmp(name,"env_radio_message");};
        b.message.string=[](void*,std::uint32_t word,const char** out) noexcept -> int{
            static const char* names[]={"","npc_jc","fixture.Radio"};if(word>2)return 0;*out=names[word];return 1;
        };
        b.message.precache=[](void* p,const char*) noexcept -> int{++static_cast<RadioBspShell*>(p)->precaches;return 1;};
        b.spawn_helper=[](void* p,void*,unsigned,unsigned,unsigned) noexcept -> int{++static_cast<RadioBspShell*>(p)->spawn_helpers;return 1;};
        b.base_activate=[](void* p,void*) noexcept -> int{++static_cast<RadioBspShell*>(p)->radio_activate;return 1;};
        b.destroy_outputs=[](void*,void*,unsigned) noexcept -> int{return 1;};
        b.base_destroy=[](void* p,void*) noexcept -> int{++static_cast<RadioBspShell*>(p)->destroys;return 1;};
        b.message.resolve=[](void* p,std::uint32_t h,void** obj,unsigned* bytes) noexcept -> int{
            auto& f=*static_cast<RadioBspShell*>(p);*obj=nullptr;*bytes=0;
            for(auto& s:f.slots)if(s.live&&s.is_radio&&h==((s.ref.generation<<16)|s.ref.index)){*obj=s.storage;*bytes=sizeof(s.storage);break;}
            return 1;
        };
        b.message.handle=[](void* p,void* obj,std::uint32_t* out) noexcept -> int{
            auto& f=*static_cast<RadioBspShell*>(p);for(auto& s:f.slots)if(s.live&&s.storage==obj){*out=(s.ref.generation<<16)|s.ref.index;return 1;}return 0;
        };
        b.message.max_clients=[](void*,int* out) noexcept -> int{*out=1;return 1;};
        b.message.single_player=[](void* p,void** out) noexcept -> int{*out=p;return 1;};
        b.message.output=[](void* p,void*,void*,unsigned off) noexcept -> int{assert(off==0x394);++static_cast<RadioBspShell*>(p)->outputs;return 1;};
        b.message.clear_delivery=[](void*) noexcept -> int{return 1;};
    }
    Slot* get(EntityRef e){return e.index&&e.index<=4&&slots[e.index-1].live&&slots[e.index-1].ref==e?&slots[e.index-1]:nullptr;}
    bool is_live(EntityRef e)const override{return e.index&&e.index<=4&&slots[e.index-1].live&&slots[e.index-1].ref==e;}
    bool create_entity(std::string_view name,EntityRef& e) override {
        if(next>=4)return false;
        auto& s=slots[next];e={++next,generation};s.ref=e;s.live=true;s.is_radio=name=="env_radio_message";
        s.control={s.storage,sizeof(s.storage)};
        return !s.is_radio||api.construct(&s.control,&b)==1;
    }
    bool key_value(EntityRef e,std::string_view k,std::string_view v)override{
        if(!get(e))return false;keys.emplace_back(std::string(k)+"="+std::string(v));return true;
    }
    bool dispatch_spawn(EntityRef e)override{auto* s=get(e);assert(s&&!s->is_radio);++ordinary_spawn;return true;}
    bool activate(EntityRef e)override{auto* s=get(e);assert(s&&!s->is_radio);++ordinary_activate;return true;}
    bool remove_entity(EntityRef e)override{
        auto* s=get(e);if(!s||fail_remove)return false;
        if(s->is_radio&&api.destroy(&s->control,&b)!=1)return false;
        s->live=false;return true;
    }
    radio::HologramControl* radio_control(EntityRef e)override{auto* s=get(e);return s&&s->is_radio?&s->control:nullptr;}
    bool parse_radio_key(EntityRef e,const radio::RadioField& field,std::string_view value,radio::RadioValue& out)override{
        assert(get(e));if(fail_parse)return false;
        keys.emplace_back(std::string(field.name)+"="+std::string(value));
        if(field.offset==0x3cc&&value=="npc_jc")out={2,1};
        else if(field.offset==0x3c8&&value=="fixture.Radio")out={0x13,2};
        else if(field.offset==0x3c4&&value=="1")out={5,1};
        else return false;
        return true;
    }
};
inline void check_radio_bsp(const radio::RadioEntityApi& api,const std::vector<std::uint8_t>& bsp){
    RadioBspShell shell(api);radio::RadioDispatcher dispatcher{};
    NativeRadioMapHost host(shell,api,shell,shell.b,dispatcher);sm::SourceMapRuntime runtime(host);
    const auto loaded=runtime.load_bsp(bsp.data(),bsp.size());
    assert(loaded.ok()&&loaded.entities_created==3&&loaded.entities_spawned==3&&loaded.entities_activated==3);
    assert(shell.ordinary_spawn==2&&shell.ordinary_activate==2&&shell.spawn_helpers==2&&shell.radio_activate==1&&shell.precaches==12);
    assert(host.attached_count()==1&&shell.keys.size()==7);
    assert(shell.keys[0]=="targetname=look_fixture"&&shell.keys[1]=="OnTrigger=radio_fixture,Dispatch,,0,-1"&&
        shell.keys[2]=="targetname=radio_fixture"&&shell.keys[3]=="speaker=npc_jc"&&shell.keys[4]=="message=fixture.Radio"&&shell.keys[5]=="priority=1"&&shell.keys[6]=="OnHeard=look_fixture,Disable,,0,-1");
    const auto radio_ref=runtime.entities()[2];const auto trigger=runtime.entities()[1];radio::RadioValue empty{2,0};
    assert(host.input(trigger,"Dispatch",&empty)==RadioMapInput::Unhandled);
    assert(host.input(radio_ref,"Dispatch",&empty)==RadioMapInput::Applied&&shell.outputs==1&&dispatcher.pending==((radio_ref.generation<<16)|radio_ref.index)&&dispatcher.kind==2);
    assert(host.input(radio_ref,"SetPriority",nullptr)==RadioMapInput::Rejected);
    assert(host.input(radio_ref,"Remove",nullptr)==RadioMapInput::Applied&&dispatcher.pending==0xffffffffu);
    shell.fail_remove=true;assert(runtime.unload().code==sm::SourceMapLoadCode::PartialUnload&&host.attached_count()==1);
    shell.fail_remove=false;assert(runtime.unload().code==sm::SourceMapLoadCode::Unloaded&&host.attached_count()==0&&shell.destroys==1);
    assert(host.input(radio_ref,"Dispatch",&empty)==RadioMapInput::Rejected);
    // Missing provider must not silently instantiate an unimplemented radio.
    RadioBspShell missing(api);NativeRadioMapHost no_radio(missing);sm::SourceMapRuntime refused(no_radio);
    assert(refused.load_bsp(bsp.data(),bsp.size()).code==sm::SourceMapLoadCode::HostCreateFailed&&!refused.loaded()&&missing.next==2);
    // Malformed radio key rolls back the genuine constructed entity once.
    RadioBspShell failed(api);failed.fail_parse=true;NativeRadioMapHost bad(failed,api,failed,failed.b,dispatcher);sm::SourceMapRuntime rollback(bad);
    assert(rollback.load_bsp(bsp.data(),bsp.size()).code==sm::SourceMapLoadCode::HostKeyValueFailed&&!rollback.loaded()&&failed.destroys==1&&bad.attached_count()==0);
    // Deferred engine deletion: explicit notification before slot reuse.
    RadioBspShell deferred(api);NativeRadioMapHost dh(deferred,api,deferred,deferred.b,dispatcher);sm::SourceMapRuntime dr(dh);
    assert(dr.load_bsp(bsp.data(),bsp.size()).ok());const auto dead=dr.entities()[2];
    assert(deferred.remove_entity(dead)&&dh.notify_deleted(dead)&&dh.attached_count()==0);
    assert(dr.unload().code==sm::SourceMapLoadCode::Unloaded&&deferred.destroys==1);
}
}
