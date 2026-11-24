#pragma once
#include "native_camera_link.h"
#include <cassert>
#include <cstring>
#include <initializer_list>
namespace sin_port::native_weapon_tables::test {
struct CameraLinkFixture {
    alignas(4) unsigned char obj[camera_link_bytes]{};
    HologramControl c{obj,sizeof(obj)};
    int camera=0,target=0,wrong=0;
    unsigned events[32]{},n=0;
    bool camera_live=true,target_live=true,registered=false,fail=false;
    void push(unsigned e){assert(c.busy&&n<32);events[n++]=e;}
    void expect(std::initializer_list<unsigned> e){assert(n==e.size()&&!std::memcmp(events,e.begin(),n*4));n=0;}
    void put(unsigned off,unsigned w){std::memcpy(obj+off,&w,4);}
    unsigned word(unsigned off){unsigned w=0;std::memcpy(&w,obj+off,4);return w;}
};
inline void check_camera_link(const CameraLinkApi& api){
    CameraLinkFixture f;CameraLinkBindings b{};b.context=&f;
    b.base_construct=[](void* p,void* o,unsigned arg) noexcept -> int{auto& f=*static_cast<CameraLinkFixture*>(p);assert(o==f.obj&&arg==1);f.push(1);return 1;};
    b.register_link=[](void* p,void*) noexcept -> int{auto& f=*static_cast<CameraLinkFixture*>(p);assert(!f.registered&&f.word(0x394)==0xffffffffu&&f.word(0x398)==0xffffffffu);f.registered=true;f.push(2);return 1;};
    b.unregister_link=[](void* p,void*) noexcept -> int{auto& f=*static_cast<CameraLinkFixture*>(p);assert(f.registered);f.registered=false;f.push(3);return 1;};
    b.base_destroy=[](void* p,void*) noexcept -> int{auto& f=*static_cast<CameraLinkFixture*>(p);assert(!f.registered);f.push(4);return 1;};
    b.base_resolve_names=[](void* p,void*) noexcept -> int{static_cast<CameraLinkFixture*>(p)->push(5);return 1;};
    b.convert=[](void* p,unsigned type,std::uint32_t word,std::uint32_t* out) noexcept -> int{auto& f=*static_cast<CameraLinkFixture*>(p);assert(type==5&&word==7);f.push(6);*out=1;return 1;};
    b.find_named=[](void* p,std::uint32_t word,void** out) noexcept -> int{auto& f=*static_cast<CameraLinkFixture*>(p);f.push(7);*out=word==1&&f.camera_live?&f.camera:word==2&&f.target_live?&f.target:word==3?&f.wrong:nullptr;return !f.fail;};
    b.is_point_camera=[](void* p,void* o,unsigned* out) noexcept -> int{auto& f=*static_cast<CameraLinkFixture*>(p);f.push(8);*out=o==&f.camera;return 1;};
    b.handle=[](void* p,void* o,std::uint32_t* out) noexcept -> int{auto& f=*static_cast<CameraLinkFixture*>(p);f.push(9);assert(o==&f.camera||o==&f.target);*out=o==&f.camera?0x1001:0x2001;return 1;};
    b.resolve=[](void* p,std::uint32_t word,void** out) noexcept -> int{auto& f=*static_cast<CameraLinkFixture*>(p);f.push(10);*out=word==0x1001&&f.camera_live?&f.camera:word==0x2001&&f.target_live?&f.target:nullptr;return 1;};
    const auto* s=api.schema();assert(s&&s->bytes==0x3a0&&s->count==4&&!std::strcmp(s->classname,"info_camera_link")&&s->fields[0].offset==0x39c&&s->fields[2].offset==0x398);
    assert(api.construct(&f.c,&b)==1);f.expect({1,2});
    assert(api.key(&f.c,"PointCamera",2,1)==1&&api.key(&f.c,"PointCamera",5,2)==0);f.put(0x90,2);
    assert(api.resolve_names(&f.c,&b)==1&&f.word(0x394)==0x1001&&f.word(0x398)==0x2001);f.expect({5,7,8,9,10,7,9});
    assert(api.resolve_names(&f.c,&b)==1);f.expect({5,10,10});
    assert(api.input(&f.c,&b,"SetCamera",2,99)==1&&f.word(0x394)==0x1001&&f.word(0x39c)==1);f.expect({7});
    assert(api.input(&f.c,&b,"SetCamera",2,3)==1&&f.word(0x394)==0xffffffffu&&f.word(0x39c)==1);f.expect({7,8});
    assert(api.input(&f.c,&b,"SetCamera",5,7)==1&&f.word(0x394)==0x1001&&f.word(0x39c)==1);f.expect({6,7,8,9,10});
    f.target_live=false;assert(api.resolve_names(&f.c,&b)==1&&f.word(0x398)==0xffffffffu);f.expect({5,10,10,7});
    f.camera_live=false;assert(api.resolve_names(&f.c,&b)==1&&f.word(0x394)==0x1001);f.expect({5,10,7,7});
    f.fail=true;assert(api.input(&f.c,&b,"SetCamera",2,1)==-1&&f.c.blocked);f.expect({7});
    // Destruction can clean up an entity whose previous callback failed.
    assert(api.destroy(&f.c,&b)==1&&!f.registered);f.expect({3,4});
}
}
