#include "native_server_weapon_post_frame.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerBaseAnimationFrameApi* __cdecl
SiN_GetServerBaseAnimationFrameApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerBaseAnimationEventsApi* __cdecl
SiN_GetServerBaseAnimationEventsApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerActivityChoiceApi* __cdecl
SiN_GetServerActivityChoiceApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerSequenceDictionaryApi* __cdecl
SiN_GetServerSequenceDictionaryApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T> T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
bool registry_ok(const ServerWeaponActivityBindings& b) noexcept {
    return b.count<=sin_independent::kEntityHandleCapacity&&(!b.count||b.entries);
}
void* resolve(const ServerWeaponPostFrameBindings& b,std::uint32_t handle) noexcept {
    return sin_independent::EntityHandleView(b.weapons.entries,b.weapons.count).Resolve(handle);
}
int player_owner(void* weapon,const ServerWeaponPostFrameBindings& b,void** out,bool* player) noexcept {
    *out=nullptr;*player=false;
    void* entity=resolve(b,read<std::uint32_t>(weapon,0x494));
    if(!entity)return 0;
    if(!b.combat_owner)return -1;
    const int status=b.combat_owner(b.context,entity,out);
    if(status<0||(status>0&&!*out))return -1;
    if(!status){*out=nullptr;return 0;}
    if(!b.is_player||!b.is_player(b.context,*out,player))return -1;
    return 1;
}
struct ViewCall {
    const ServerWeaponPostFrameBindings* b;
    ServerWeaponPostFrameResult* result;
    void* weapon;
    bool failed;
};
int __cdecl run(void* weapon,unsigned bytes,void* receiver,float now,const ServerWeaponPostFrameBindings* b,ServerWeaponPostFrameResult* out) noexcept {
    if(!out)return 0;*out={};out->selected=-1;out->reset.registration.failed_index=-1;
    out->events.failed_layer=out->view_events.failed_layer=-2;
    if(!weapon||bytes<0x4c0||!receiver||!b||!std::isfinite(now)||!registry_ok(b->weapons))return 0;
    const auto* frame=SiN_GetServerBaseAnimationFrameApi(1,sizeof(ServerBaseAnimationFrameApi));
    const auto* events=SiN_GetServerBaseAnimationEventsApi(1,sizeof(ServerBaseAnimationEventsApi));
    if(!frame||!events||!frame->run(weapon,bytes,now,&b->frame))return 0;
    out->advanced=1;
    if(read<std::uint8_t>(weapon,0x3e0)&&read<std::uint8_t>(weapon,0x3e1)){
        const auto& a=b->animation;
        if(!a.metadata||!a.activities||!a.events||a.events!=a.dictionary.dictionary)return 0;
        const auto* choice=SiN_GetServerActivityChoiceApi(1,sizeof(ServerActivityChoiceApi));
        const auto* reset=SiN_GetServerSequenceDictionaryApi(1,sizeof(ServerSequenceDictionaryApi));
        const ServerActivityCompositeView* view=nullptr;
        const int status=a.metadata(a.context,weapon,&view);
        if(status<0||(status>0&&!view)||!choice||!reset)return 0;
        if(!status)view=nullptr;
        ServerActivityChoiceResult selected{};out->reselected=1;
        if(!choice->choose(a.activities,a.events,view,read<int>(weapon,0x4bc),read<int>(weapon,0x3ec),true,&a.random,&selected))return 0;
        out->selected=selected.selected;
        if(out->selected!=-1){
            if(!reset->set(weapon,bytes,out->selected,&a.reset,&a.dictionary,&out->reset))return 0;
            out->sequence_set=1;
        }
    }
    if(!events->dispatch(weapon,bytes,receiver,now,&b->events,&out->events))return 0;
    void* owner=nullptr;bool player=false;
    const int status=player_owner(weapon,*b,&owner,&player);
    if(status<0)return 0;
    out->owner_live=status>0;out->player_owner=player;
    if(!player)return 1;
    unsigned owner_bytes=0;
    if(!b->extent||!b->extent(b->context,owner,&owner_bytes)||owner_bytes<0xbe0)return 0;
    void* view=resolve(*b,read<std::uint32_t>(owner,0xbdc));
    if(!view)return 1;
    if(view==weapon)return 0;
    unsigned view_bytes=0;
    if(!b->extent(b->context,view,&view_bytes))return 0;
    out->viewmodel_live=1;
    if(!frame->run(view,view_bytes,now,&b->view_frame))return 0;
    out->viewmodel_advanced=1;
    // Do not reuse the initial owner pointer when forwarding individual
    // viewmodel events. Original 222aaf90 resolves it again for each event.
    ViewCall call{b,out,weapon,false};
    auto eb=b->view_events;eb.context=&call;
    eb.sequence=[](void* p,void* a,int seq,ServerAnimationEventSequence* out) noexcept -> int {
        auto& v=*static_cast<ViewCall*>(p);const auto& e=v.b->view_events;
        return e.sequence?e.sequence(e.context,a,seq,out):-1;};
    eb.cycle_rate=[](void* p,void* a,int seq,double* out) noexcept -> int {
        auto& v=*static_cast<ViewCall*>(p);const auto& e=v.b->view_events;
        return e.cycle_rate&&e.cycle_rate(e.context,a,seq,out);};
    eb.emit=[](void* p,void* receiver,int,const ServerAnimationEvent* e) noexcept {
        auto& v=*static_cast<ViewCall*>(p);if(v.failed)return;
        if(receiver!=v.weapon){v.failed=true;return;}
        void* owner=nullptr;bool player=false;
        const int status=player_owner(v.weapon,*v.b,&owner,&player);
        if(status<0){v.failed=true;return;}
        if(!player)return;
        if(!v.b->weapon_event||!v.b->weapon_event(v.b->context,v.weapon,owner,e)){v.failed=true;return;}
        ++v.result->viewmodel_forwarded;
    };
    return events->dispatch(view,view_bytes,weapon,now,&eb,&out->view_events)&&!call.failed;
}
int __cdecl notify(void* actor,unsigned bytes,float now,const ServerWeaponPostFrameBindings* b,ServerWeaponPostFrameResult* out,unsigned* live) noexcept {
    if(!out||!live)return 0;*out={};out->selected=-1;*live=0;
    if(!actor||bytes<0x790||!b||!registry_ok(b->weapons)||!std::isfinite(now))return 0;
    void* weapon=resolve(*b,read<std::uint32_t>(actor,0x78c));
    if(!weapon)return 1;
    unsigned kind=0,extent=0;
    if(!b->weapons.classify||!b->weapons.classify(b->weapons.context,weapon,&kind)||kind>WeaponActivitySmg||
       !b->extent||!b->extent(b->context,weapon,&extent))return 0;
    *live=1;return run(weapon,extent,actor,now,b,out);
}
const ServerWeaponPostFrameApi api{1,sizeof(ServerWeaponPostFrameApi),&run,&notify};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerWeaponPostFrameApi=_SiN_GetServerWeaponPostFrameApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerWeaponPostFrameApi* __cdecl
SiN_GetServerWeaponPostFrameApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerWeaponPostFrameApi)?&api:nullptr;
}
