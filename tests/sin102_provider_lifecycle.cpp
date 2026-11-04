#include "sin_compat/source_provider_c_api.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

struct MockState {
    std::string name{"step"};
    uint32_t token{77};
    bool omit_terminator{false};
    sin101_context* parent{nullptr};
    sin101_provider_v1* replace_with{nullptr};
};
static int32_t lookup(void* user,const char* name,uint32_t len,int64_t* candidate) {
    auto& s=*static_cast<MockState*>(user);
    if(s.replace_with) {
        auto* replacement=s.replace_with;
        s.replace_with=nullptr;
        assert(sin101_register_provider(s.parent,replacement)==SIN101_OK);
    }
    if(s.name.size()!=len || std::memcmp(name,s.name.data(),len)!=0)return -1;
    *candidate=s.token;return 0;
}
static int32_t valid(void* user,uint32_t token,uint32_t* out) {
    auto& s=*static_cast<MockState*>(user);
    *out=(s.token==token)?1u:0u;return 0;
}
static int32_t reverse(void* user,uint32_t token,char* buffer,uint32_t cap,uint32_t* n) {
    auto& s=*static_cast<MockState*>(user);
    if(token!=s.token || cap<s.name.size()+1)return -1;
    std::memcpy(buffer,s.name.data(),s.name.size());
    if(!s.omit_terminator)buffer[s.name.size()]='\0';
    *n=static_cast<uint32_t>(s.name.size()+1);
    return 0;
}
static sin101_provider_v1 provider(MockState& s) {
    return {sizeof(sin101_provider_v1),SIN101_ABI_VERSION,SIN101_PROVIDER_MOCK,
        0,&s,lookup,valid,reverse};
}
static void check_mapping(sin101_context* c,int id,uint32_t token) {
    uint32_t out=0;
    assert(sin101_old_to_native(c,id,&out)==SIN101_OK && out==token);
    assert(sin101_native_to_old(c,token,&out)==SIN101_OK && out==static_cast<uint32_t>(id));
}
static void check_absent(sin101_context* c,int id,uint32_t native) {
    uint32_t out=0;
    assert(sin101_old_to_native(c,id,&out)==SIN101_NOT_FOUND && out==SIN101_MISSING);
    assert(sin101_native_to_old(c,native,&out)==SIN101_NOT_FOUND && out==SIN101_MISSING);
}
int main(int argc,char** argv) {
    assert(argc==2);
    sin101_context* c=nullptr;
    assert(sin101_create(SIN101_ABI_VERSION,argv[1],&c)==SIN101_OK && c);
    MockState a{},b{};b.token=99;
    a.parent=b.parent=c;
    auto pa=provider(a),pb=provider(b);
    assert(sin101_register_provider(c,&pa)==SIN101_OK);
    // A valid name without the explicit trailing NUL must be rejected.
    a.omit_terminator=true;
    assert(sin101_map_sound(c,70000,100000,"step",4)==SIN101_NAME_MISMATCH);
    uint32_t count_before=999;
    assert(sin101_sound_count(c,&count_before)==SIN101_OK && count_before==0);
    a.omit_terminator=false;
    assert(sin101_map_sound(c,70000,100000,"step",4)==SIN101_OK);
    check_mapping(c,70000,77);
    auto rejected=pb;
    rejected.kind=SIN101_PROVIDER_SOURCEVR_NATIVE;
    assert(sin101_register_provider(c,&rejected)==SIN101_NATIVE_ABI_NOT_VERIFIED);
    check_mapping(c,70000,77);

    bool real_keys=false;
    if(const char* private_path=std::getenv("SIN100_PRIVATE_KEYMAP_BYTES")) {
        std::ifstream file(private_path,std::ios::binary);
        assert(file.good());
        std::vector<uint8_t> bytes(std::istreambuf_iterator<char>{file},{});
        assert(bytes.size()==1024);
        assert(sin101_install_keymap_le(c,bytes.data(),static_cast<uint32_t>(bytes.size()))==SIN101_OK);
        uint32_t code=0;
        assert(sin101_translate_key(c,48,&code)==SIN101_OK && code==1);
        real_keys=true;
    }
    assert(sin101_register_provider(c,&pb)==SIN101_OK);
    check_absent(c,70000,77);
    uint32_t count=999;
    assert(sin101_sound_count(c,&count)==SIN101_OK && count==0);
    if(real_keys) {
        uint32_t code=0;
        assert(sin101_translate_key(c,48,&code)==SIN101_OK && code==1);
        assert(sin101_translate_key(c,8,&code)==SIN101_OK && code==66);
    }
    assert(sin101_map_sound(c,70000,100000,"step",4)==SIN101_OK);
    check_mapping(c,70000,99);
    check_absent(c,70001,77);

    b.replace_with=&pa;
    assert(sin101_map_sound(c,70001,100000,"step",4)==SIN101_PROVIDER_CHANGED);
    check_absent(c,70000,99);
    assert(sin101_sound_count(c,&count)==SIN101_OK && count==0);
    if(real_keys) {
        uint32_t code=0;
        assert(sin101_translate_key(c,49,&code)==SIN101_OK && code==2);
    }
    assert(sin101_map_sound(c,70001,100000,"step",4)==SIN101_OK);
    check_mapping(c,70001,77);

    std::atomic<bool> stop{false};
    std::atomic<bool> error{false};
    std::thread worker([&]{
        while(!stop.load()) {
            uint32_t tok=0;
            auto result=sin101_old_to_native(c,70001,&tok);
            if(result!=SIN101_OK && result!=SIN101_NOT_FOUND) error=true;
            if(sin101_sound_count(c,&tok)!=SIN101_OK) error=true;
        }
    });
    for(int i=0;i<200;++i)
        assert(sin101_register_provider(c,(i%2)?&pa:&pb)==SIN101_OK);
    stop=true;worker.join();
    assert(!error.load());
    assert(sin101_sound_count(c,&count)==SIN101_OK && count==0);
    check_absent(c,70001,77);
    check_absent(c,70000,99);

    assert(sin101_reset(c)==SIN101_OK);
    uint32_t key=1;
    assert(sin101_translate_key(c,48,&key)==SIN101_PROVIDER_NOT_READY && key==0);
    assert(sin101_map_sound(c,70000,100000,"step",4)==SIN101_PROVIDER_NOT_READY);
    sin101_destroy(c);
    std::puts("PASS: Phase102 provider replacement invalidates sound namespace, keyboard survives, epoch guarded");
}
