#include "native_server_activity_assignment.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcActivityTranslateApi* __cdecl
SiN_GetServerNpcActivityTranslateApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerSequenceDictionaryApi* __cdecl
SiN_GetServerSequenceDictionaryApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
template<class T> T read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T> void write(void* p,unsigned off,T v) noexcept {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
bool grunt(const ServerNpcActivityAssignmentBindings& b) noexcept {
    return b.translation.actor_class==NpcActivityGrunt||b.translation.actor_class==NpcActivityJetpack;
}
bool valid(void* a,unsigned bytes,const ServerNpcActivityAssignmentBindings* b) noexcept {
    return a&&b&&bytes>=(grunt(*b)?0xfccu:0x90cu);
}
bool clear_cycle(void* a,const ServerSequenceResetBindings& b,bool& changed) noexcept {
    const float cycle=read<float>(a,0x3e8);
    if(!std::isfinite(cycle))return false;
    changed=cycle!=0.f;if(!changed)return true;
    if(read<std::uint8_t>(a,0x1f0))write<std::uint8_t>(a,0x200,1);
    else {
        const auto word=read<std::uint32_t>(a,0x16c);
        if(word){void* p=nullptr;unsigned size=0;
            if(!b.storage||!b.storage(b.context,word,&p,&size)||!p||size<4)return false;
            write(p,0,read<unsigned>(p,0)|1u);
        }
    }
    write(a,0x3e8,0.f);return true;
}
bool movement(void* a,int activity,const ServerNpcActivityAssignmentBindings& b,bool& value) noexcept {
    if(grunt(b)){value=activity>=6&&activity<=14;return true;}
    return b.locomotion&&b.locomotion(b.context,a,activity,&value);
}
bool lower_view(int act) noexcept {
    return act==5||act==0x12||act==0x13||act==0x43||act==0x131||act==0x136||
        act==0x139||act==0x13b||(act>=0x142&&act<=0x147)||act==0x18f;
}
bool view(void* a,unsigned bytes,int activity,const ServerNpcActivityAssignmentBindings& b,ServerNpcActivityAssignmentResult& out) noexcept {
    float v[3]{};
    if(grunt(b)){
        unsigned capabilities=0;
        if(!b.capabilities||!b.capabilities(b.context,a,&capabilities))return false;
        bool lower=false;
        if(capabilities&0x08000000u){
            const auto* translator=SiN_GetServerNpcActivityTranslateApi(1,sizeof(ServerNpcActivityTranslateApi));
            ServerNpcActivityTranslateResult tr{};
            if(!translator||!translator->translate(a,bytes,activity,&b.translation,&tr))return false;
            lower=lower_view(tr.activity);
        }
        v[2]=lower||!read<std::uint8_t>(a,0xc64)?40.f:66.f;
    }else if(!b.view_vector||!b.view_vector(b.context,a,activity,v))return false;
    bool different=false;
    for(unsigned i=0;i<3;++i){
        if(!std::isfinite(v[i])||!std::isfinite(read<float>(a,0x37c+4*i)))return false;
        different|=v[i]!=read<float>(a,0x37c+4*i);
    }
    if(different){
        // 2213e200 calls +280 before the copy. Verified eight Grunt/Jetpack
        // primary vtables use original no-op 22001a50.
        if(!grunt(b)&&(!b.view_dirty||!b.view_dirty(b.context,a)))return false;
        std::memcpy(static_cast<unsigned char*>(a)+0x37c,v,sizeof(v));out.view_changed=1;
    }
    return true;
}
bool assign_impl(void* a,unsigned bytes,const ServerNpcActivityAssignmentInput& in,const ServerNpcActivityAssignmentBindings& b,ServerNpcActivityAssignmentResult& out) noexcept {
    const auto* setter=SiN_GetServerSequenceDictionaryApi(1,sizeof(ServerSequenceDictionaryApi));
    if(!setter||!b.component_notify)return false;
    write(a,0x87c,in.translated);
    if(in.sequence>=0){
        if(read<int>(a,0x3ec)!=in.sequence||!read<std::uint8_t>(a,0x3e1)){
            bool old_move=false,new_move=false;
            if(!movement(a,read<int>(a,0x878),b,old_move))return false;
            if(old_move&&!movement(a,in.requested,b,new_move))return false;
            if(!old_move||!new_move){bool changed=false;if(!clear_cycle(a,b.reset,changed))return false;out.precleared_cycle=changed;}
        }
        if(!setter->set(a,bytes,in.sequence,&b.reset,&b.dictionary,&out.sequence))return false;
        double duration=0;
        if(!b.reset.duration||!b.reset.duration(b.reset.context,a,in.sequence,&duration)||!std::isfinite(duration)||!std::isfinite(static_cast<float>(duration)))return false;
        const auto handle=read<std::uint32_t>(a,0x78c);
        if(handle!=0xffffffffu){
            if(!b.weapon_notify||!b.weapon_notify(b.context,a,handle,in.weapon_activity,static_cast<float>(duration)))return false;
            out.weapon_notified=1; // attempted, resolver may find no live weapon
        }
    }else if(!setter->set(a,bytes,0,&b.reset,&b.dictionary,&out.sequence))return false;
    if(!view(a,bytes,read<int>(a,0x87c),b,out))return false;
    if(read<int>(a,0x878)!=in.requested){
        if(grunt(b))write<std::uint8_t>(a,0xc35,0);
        else if(!b.activity_changed||!b.activity_changed(b.context,a,in.requested))return false;
        out.activity_changed=1;
    }
    write(a,0x878,in.requested);
    const auto component=read<std::uint32_t>(a,0x908);
    if(!component||!b.component_notify(b.context,a,component))return false;
    out.component_notified=1;out.assigned=1;return true;
}
int __cdecl assign(void* a,unsigned bytes,const ServerNpcActivityAssignmentInput* in,const ServerNpcActivityAssignmentBindings* b,ServerNpcActivityAssignmentResult* out) noexcept {
    if(!out)return 0;*out={};out->sequence.registration.failed_index=-1;
    return valid(a,bytes,b)&&in&&assign_impl(a,bytes,*in,*b,*out);
}
int __cdecl request(void* a,unsigned bytes,int requested,const ServerNpcActivityAssignmentBindings* b,ServerNpcActivityAssignmentResult* out) noexcept {
    if(!out)return 0;*out={};out->sequence.registration.failed_index=-1;
    if(!valid(a,bytes,b)||!b->translation.metadata||b->translation.events!=b->dictionary.dictionary)return 0;
    const ServerActivityCompositeView* metadata=nullptr;
    const int available=b->translation.metadata(b->translation.context,a,&metadata);
    if(available<0||(available>0&&!metadata))return 0;
    const int old=read<int>(a,0x878);
    if(!available||old==requested||(requested!=0&&old==2&&read<int>(a,0x880)!=0x9a)){
        out->skipped=1;
    }else{
        const auto* translator=SiN_GetServerNpcActivityTranslateApi(1,sizeof(ServerNpcActivityTranslateApi));
        if(!translator)return 0;
        write(a,0x880,requested);
        if(!translator->select(a,bytes,requested,&b->translation,&out->selected))return 0;
        write(a,0x884,out->selected.sequence);write(a,0x888,out->selected.activity);
        write(a,0x88c,out->selected.translation.initial_weapon);
        const ServerNpcActivityAssignmentInput in{requested,out->selected.sequence,out->selected.activity,out->selected.translation.initial_weapon};
        if(!assign_impl(a,bytes,in,*b,*out))return 0;
    }
    // 22358c30 clears marker after base request even on a skipped request.
    if(grunt(*b))write(a,0xfc8,-1);
    return 1;
}
const ServerNpcActivityAssignmentApi api{1,sizeof(ServerNpcActivityAssignmentApi),&assign,&request};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcActivityAssignmentApi=_SiN_GetServerNpcActivityAssignmentApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcActivityAssignmentApi* __cdecl
SiN_GetServerNpcActivityAssignmentApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ServerNpcActivityAssignmentApi)?&api:nullptr;
}
