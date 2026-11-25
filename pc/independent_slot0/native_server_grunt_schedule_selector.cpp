#include "native_server_grunt_schedule_selector.h"
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl
SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcBehaviorSelectionApi* __cdecl
SiN_GetServerNpcBehaviorSelectionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerActivityChoiceApi* __cdecl
SiN_GetServerActivityChoiceApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T>T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T>void write(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
struct Call {
    void* a;unsigned bytes;
    const ServerGruntScheduleSelectorBindings& b;
    ServerGruntScheduleSelectorResult& out;
    const ServerNpcConditionApi* cond;
    bool ok=true;
    bool has(int id) noexcept {
        ServerNpcConditionResult cr{};++out.condition_tests;
        if(!ok||!cond->test(a,bytes,id,NpcConditionCurrent,&b.schedule.conditions,&cr)){ok=false;return false;}
        return cr.value!=0;
    }
    bool clear(int id) noexcept {
        ServerNpcConditionResult cr{};++out.condition_clears;
        return cond->store(a,bytes,id,NpcConditionCurrent,false,&b.schedule.conditions,&cr)!=0;
    }
    int chosen(int id) noexcept {if(!ok)return 0;out.schedule=id;return 1;}
    int parent() noexcept {
        if(!ok||!b.parent)return 0;out.parent_called=1;return b.parent(b.context,a,&out.schedule);
    }
};
int __cdecl select(void* a,unsigned bytes,const ServerGruntScheduleSelectorBindings* b,ServerGruntScheduleSelectorResult* out) noexcept {
    if(!out)return 0;*out={};out->schedule=out->flinch_sequence=-1;
    if(!a||bytes<0xdd8||!b||b->entities.count>sin_independent::kEntityHandleCapacity||(b->entities.count&&!b->entities.entries))return 0;
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    const auto* behavior=SiN_GetServerNpcBehaviorSelectionApi(1,sizeof(ServerNpcBehaviorSelectionApi));
    if(!cond||!behavior)return 0;
    Call q{a,bytes,*b,*out,cond};
    if((read<std::uint8_t>(a,0xb6)&2)&&!read<std::uint8_t>(a,0xcb0)){
        write<std::uint8_t>(a,0xcb0,1);return q.chosen(0x6f);
    }
    if(q.has(0x46))return q.chosen(0x68);
    if(q.has(0x4c))return q.chosen(0x73);
    if(q.has(0x48))return q.chosen(0x75);
    if(!q.ok)return 0;
    // 2204e670: condition17 and +988 bit0x40 clear, then +728(1,0).
    if(q.has(0x11)&&!(read<std::uint8_t>(a,0x988)&0x40)){
        const auto* t=b->sequences;
        if(!b->flinch_activity||!t||!t->metadata||!t->activities||!t->events)return 0;
        int activity=0;if(!b->flinch_activity(b->context,a,&activity))return 0;
        const ServerActivityCompositeView* view=nullptr;
        const int status=t->metadata(t->context,a,&view);
        if(status<0||(status>0&&!view))return 0;
        if(status>0){
            const auto* api=SiN_GetServerActivityChoiceApi(1,sizeof(ServerActivityChoiceApi));ServerActivityChoiceResult result{};
            out->flinch_probed=1;
            if(!api||!api->choose(t->activities,t->events,view,activity,read<int>(a,0x3ec),true,&t->random,&result))return 0;
            out->flinch_sequence=result.selected;if(result.selected!=-1)return q.chosen(0x16);
        }
    }
    if(!q.ok)return 0;
    if(read<int>(a,0x850)!=4){
        if(q.has(0x38))return q.chosen(0x42);
        const int state=read<int>(a,0x850);
        if(state!=6&&state!=7){
            if(q.has(0x37))return q.chosen(0x6d);
            if(q.has(0x30)){
                ServerGruntScheduleSound sound{};++out->sound_queried;
                if(!b->sound||!b->sound(b->context,a,0xfff1f,&sound))return 0;
                if(sound.token&&(sound.flags&8)){
                    unsigned cue=0;
                    auto registry=sin_independent::EntityHandleView(b->entities.entries,b->entities.count);
                    void* grenade=registry.Resolve(sound.entity_handle);void* owner=nullptr;
                    if(grenade){
                        // The callback performs original RTTI cast and owner
                        // resolution. Non-grenade or absent owner returns null.
                        if(!b->grenade_owner||!b->grenade_owner(b->context,grenade,&owner))return 0;
                    }
                    if(owner){
                        int relation=0;
                        if(!b->relation||!b->relation(b->context,a,owner,&relation))return 0;
                        if(relation!=3)cue=1;
                        write(a,0xcb8,sound.entity_handle);
                        bool eligible=false;
                        if(!b->kick_eligible||!b->kick_eligible(b->context,a,&eligible))return 0;
                        if(eligible){
                            const auto handle=read<std::uint32_t>(a,0xcb8);
                            // 22031820 assigns target handle or sentinel for stale.
                            write(a,0xcb4,handle);write(a,0x8a8,registry.Resolve(handle)?handle:0xffffffffu);
                            write(a,0xcb8,0xffffffffu);out->kick_transferred=1;return q.chosen(0x73);
                        }
                    }
                    if(!b->cue||!b->cue(b->context,a,cue,0,1))return 0;
                    unsigned yes=0;
                    if(!behavior->schedule_is(a,bytes,0x5e,true,&b->schedule,&yes))return 0;
                    if(!yes&&!behavior->schedule_is(a,bytes,0x5f,true,&b->schedule,&yes))return 0;
                    return q.chosen(yes?0x5f:0x5e);
                }
                if(sound.token&&!(sound.flags&8)&&!q.has(0xa)&&(sound.flags&5)){
                    if(!q.ok||!b->sound_target||!b->sound_target(b->context,a,sound.token))return 0;
                }
            }
        }
        if(!q.ok||!behavior->select(a,bytes,&b->behavior,&out->behavior))return 0;
        if(out->behavior.selected)return q.parent();
    }
    // Original OR, not AND. State4 skips behavior pass, not help-up branch.
    if(q.has(0x4a)||read<std::uint32_t>(a,0xca4))return q.chosen(0x6c);
    if(q.has(0x49)){
        if(!q.clear(0x49)||!q.clear(0x47))return 0;
        const auto token=read<std::uint32_t>(a,0xca8);
        if(token){
            void* helper=nullptr;unsigned extent=0;
            if(!b->mutable_storage||!b->mutable_storage(b->context,token,&helper,&extent)||!helper||extent<0xca8)return 0;
            write(helper,0xca4,0u);write(helper,0x8a8,0xffffffffu);
            write(a,0xca8,0u);out->helper_cleared=1;
        }
        return q.chosen(0x71);
    }
    if(read<int>(a,0x850)!=4&&q.has(0x47)&&!q.has(0x49))return q.chosen(0x6b);
    if(q.has(0x4b))return q.chosen(0x6e);
    if(!q.ok)return 0;
    const int state=read<int>(a,0x850);
    if(state==3){
        bool special=false;int id=0;
        if(!b->combat_special||!b->combat_special(b->context,a,&special))return 0;
        if(special)return q.chosen(0x55);
        if(!b->combat_schedule||!b->combat_schedule(b->context,a,&id))return 0;
        return id?q.chosen(id):q.parent();
    }
    if(state==1&&read<std::uint8_t>(a,0xc65))return q.chosen(0x64);
    if(state==1||state==2){
        if(q.has(0x10)||q.has(0x11)){
            bool yes=false;if(!b->investigate||!b->investigate(b->context,a,&yes))return 0;
            if(yes)return q.chosen(0x1c);
        }
        if(q.has(0x33)&&read<std::uint32_t>(a,0x9c4)){
            bool yes=false;if(!b->squad_gate||!b->squad_gate(b->context,a,4,&yes))return 0;
            if(yes)return q.chosen(0xa);
        }
        if(!read<std::uint32_t>(a,0xdd4)&&(read<std::uint8_t>(a,0xc65)||q.has(0x44)))return q.chosen(0x64);
    }
    return q.parent();
}
const ServerGruntScheduleSelectorApi api{1,sizeof(ServerGruntScheduleSelectorApi),&select};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerGruntScheduleSelectorApi=_SiN_GetServerGruntScheduleSelectorApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerGruntScheduleSelectorApi* __cdecl
SiN_GetServerGruntScheduleSelectorApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerGruntScheduleSelectorApi)?&api:nullptr;
}
