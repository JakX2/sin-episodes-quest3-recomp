#include "sin_compat/original_cache_header_client_cleanup.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
using namespace sin_compat::lifecycle;
static std::vector<std::string> events;
int main() {
    static_assert(cache_header_bytes==16 && cache_header_magic==2);
    auto make=[](std::uint32_t magic,std::uint32_t format,std::uint32_t key,std::uint32_t count) {
        std::array<std::uint8_t,16> bytes{};
        const std::uint32_t values[4]{magic,format,key,count};
        for(int i=0;i<4;++i)
            for(int j=0;j<4;++j)bytes[static_cast<std::size_t>(4*i+j)]=
                static_cast<std::uint8_t>(values[i]>>(8*j));
        return bytes;
    };
    auto data=make(2,3,0x12345678,4);
    auto valid=scene_header(data.data(),data.size(),0x12345678);
    assert(valid.status==CacheHeaderStatus::HeaderMatches && valid.may_parse_payload);
    assert(!valid.mark_dirty_and_continue && valid.record_count==4);
    assert(scene_header(data.data(),16,0).may_parse_payload);
    assert(scene_header(data.data(),15,0).status==CacheHeaderStatus::InsufficientData);
    assert(scene_header(nullptr,16,0).status==CacheHeaderStatus::InsufficientData);
    assert(model_sounds_header(data.data(),16,0).status==CacheHeaderStatus::WrongFormat);
    assert(scene_header(data.data(),16,99).status==CacheHeaderStatus::ChangedSourceKey);
    data[0]=0;
    assert(scene_header(data.data(),16,0).status==CacheHeaderStatus::WrongMagic);
    auto model=make(2,2,1,0xffffffffu);
    assert(model_sounds_header(model.data(),16,1).record_count==-1);
    assert(model_sounds_header(model.data(),16,1).may_parse_payload);
    int a=1,b=2,c=3;
    void* global=&a;
    release_client_global(global,[&](void* ptr,int flag){
        assert(ptr==&a && flag==1);events.push_back("global");});
    assert(!global && events.size()==1);
    release_client_global(global,[](void*,int){assert(false);});
    ClientPointerArrayState state{{&a,nullptr,&b,&c},4,0,true,7,true};
    cleanup_client_pointer_array(state,[&](void* ptr,int flag){
        assert(flag==1);events.push_back(ptr==&c?"C":ptr==&b?"B":"A");
    },[&](){events.push_back("free");});
    assert((events==std::vector<std::string>{"global","C","B","A","free"}));
    assert(state.active_count==0 && !state.heap_buffer_attached && state.auxiliary_count==0
           && !state.auxiliary_flag);
    events.clear();
    state={{&a,&b},2,-1,true,9,true};
    cleanup_client_pointer_array(state,[&](void* ptr,int flag){
        assert(flag==1);events.push_back(ptr==&b?"B":"A");
    },[](){assert(false);});
    assert((events==std::vector<std::string>{"B","A"}));
    assert(state.heap_buffer_attached && state.auxiliary_count==9 && !state.auxiliary_flag);
}
