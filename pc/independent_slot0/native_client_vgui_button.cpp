#include "native_client_vgui_button.h"
#include <cstring>
namespace sin_port::native_vgui_button {
namespace {
template<class T>T field(const void* p,unsigned offset) noexcept {
    T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+offset,sizeof(v));return v;
}
int __cdecl command(void* panel,unsigned bytes,const char* name,const Bindings* b,Result* o) noexcept {
    if(!o)return 0;*o={};
    if(!panel||bytes<0xdc||field<unsigned>(panel,0)!=panel_vtable||!name||!b||!b->create_event||
        b->entry_count>sin_independent::kEntityHandleCapacity||(b->entry_count&&!b->entries))return 0;
    void* event=nullptr;o->create_called=1;
    if(!b->create_event(b->context,"button_press",true,&event))return 0;
    auto done=[&](){o->base_noop=o->completed=1;return 1;};
    // 240070c0 base Panel::OnCommand is a RET 4.
    if(!event)return done();
    o->created=1;
    auto fail=[&](){
        if(b->discard_event&&b->discard_event(b->context,event))o->discarded=1;
        return 0;
    };
    const sin_independent::EntityHandleView registry(b->entries,b->entry_count);
    void* screen=registry.Resolve(field<unsigned>(panel,0xd8));
    if(!screen)return fail(); // retail would dereference nullptr+8
    o->resolved=1;unsigned extent=0;
    if(!b->extent||!b->extent(b->context,screen,&extent)||extent<0x54)return fail();o->extent_read=1;
    if(field<unsigned>(screen,0)!=screen_vtable||field<unsigned>(screen,8)!=screen_networkable_vtable)return fail();
    // Original networkable +24 = 240c4860: subobject+48 = owner+50.
    o->index=field<int>(screen,0x50);o->index_native=1;
    if(!b->set_int||!b->set_int(b->context,event,"entindex",o->index))return fail();o->int_set=1;
    if(!b->set_string||!b->set_string(b->context,event,"command",name))return fail();o->string_set=1;
    if(!b->fire_event||!b->fire_event(b->context,event,false))return fail();o->fired=1;
    return done();
}
const Api api{1,sizeof(Api),&command};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetClientVguiButtonApi=_SiN_GetClientVguiButtonApi")
extern "C" __declspec(dllexport) const sin_port::native_vgui_button::Api* __cdecl
SiN_GetClientVguiButtonApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_vgui_button;return version==1&&size==sizeof(Api)?&api:nullptr;
}
