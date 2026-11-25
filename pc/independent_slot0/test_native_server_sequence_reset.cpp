#include "native_server_sequence_reset.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <vector>
#include <cstdio>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char actor[0x46c]{};
    unsigned dirty=0x80,flags=0;double duration=2.,distance=120.;
    std::vector<int> trace;
    template<class T> void put(unsigned off,T v){std::memcpy(actor+off,&v,sizeof(v));}
    template<class T> T get(unsigned off){T v{};std::memcpy(&v,actor+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetServerSequenceResetApi>(GetProcAddress(server,server_sequence_reset_export));
    assert(get&&!GetProcAddress(client,server_sequence_reset_export)&&!get(0,sizeof(ServerSequenceResetApi))&&!get(1,0));
    auto* api=get(1,sizeof(ServerSequenceResetApi));assert(api);
    F f;ServerSequenceResetBindings b{};
    b.context=&f;
    b.duration=[](void* p,void* a,int seq,double* out) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&seq==f.get<int>(0x3ec));f.trace.push_back(1);*out=f.duration;return 1;};
    b.distance=[](void* p,void*,int,double* out) noexcept -> int{auto& f=F::self(p);f.trace.push_back(2);*out=f.distance;return 1;};
    b.flags=[](void* p,void*,int,unsigned* out) noexcept -> int{auto& f=F::self(p);f.trace.push_back(3);*out=f.flags;return 1;};
    b.storage=[](void* p,std::uint32_t word,void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);assert(word==0x1000);*out=&f.dirty;*n=4;return 1;};
    b.register_events=[](void* p,void*,int seq) noexcept -> int{auto& f=F::self(p);assert(seq==f.get<int>(0x3ec)&&!f.get<std::uint8_t>(0x3e0)&&f.get<float>(0x398)==0.f&&f.get<float>(0x3c0)==1.f);f.trace.push_back(4);return 1;};
    b.debug=[](void* p,void*,int,int) noexcept{F::self(p).trace.push_back(5);};
    f.put(0x1f0,std::uint8_t{1});f.put(0x3ec,7);f.put(0x3e8,.75f);f.put(0x3c0,2.f);f.put(0x3e0,std::uint8_t{1});f.put(0x398,.5f);f.put(0x464,7u);f.put(0x468,3u);
    ServerSequenceResetResult out{};
    assert(api->set(f.actor,sizeof(f.actor),7,&b,&out)&&out.reset&&out.cycle_changed&&!out.sequence_changed);
    assert((f.trace==std::vector<int>{1,2,3,4})&&f.get<float>(0x394)==60.f&&f.get<float>(0x3e8)==0.f&&f.get<unsigned>(0x464)==0&&f.get<unsigned>(0x468)==4&&f.get<std::uint8_t>(0x200)==1);
    f.trace.clear();f.put(0x3e1,std::uint8_t{1});f.put(0x3e8,.4f);f.put(0x200,std::uint8_t{0});
    assert(api->set(f.actor,sizeof(f.actor),7,&b,&out)&&!out.reset&&!out.cycle_changed&&!out.sequence_changed&&f.trace.empty()&&!f.get<std::uint8_t>(0x200));
    f.flags=1;f.put(0x1f0,std::uint8_t{0});f.put(0x16c,0x1000u);f.put(0xa9,std::uint8_t{0x10});b.debug_enabled=true;
    assert(api->set(f.actor,sizeof(f.actor),8,&b,&out)&&out.reset&&out.sequence_changed&&!out.cycle_changed&&f.get<float>(0x3e8)==.4f&&f.dirty==0x81);
    assert((f.trace==std::vector<int>{5,1,2,3,4})&&f.get<std::uint8_t>(0x3e1)==1);
    f.trace.clear();f.duration=0.;f.flags=0;f.put(0x3ec,-1);f.put(0x464,7u);
    assert(api->reset(f.actor,sizeof(f.actor),&b,&out)&&out.reset&&f.get<int>(0x3ec)==0&&f.get<float>(0x394)==0.f&&f.get<float>(0x3e8)==.4f&&f.get<unsigned>(0x464)==0);
    assert((f.trace==std::vector<int>{1,3,4}));
    assert(!api->reset(f.actor,0x46b,&b,&out));
    FreeLibrary(server);FreeLibrary(client);
    std::puts("PASS native server sequence assignment, original loop gate and reset parity");
}
