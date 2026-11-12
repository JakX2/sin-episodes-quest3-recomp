#include "sin_compat/original_client_surface_file.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <limits>
#include <stdexcept>
#include <cstdint>
#include <optional>
#include <utility>
#include <algorithm>
using namespace sin_compat::original_client_surface_file;

struct ValueHost {
    ValueRecord value;
    bool present=true;
    bool conversion_ok=true;
    std::vector<std::string> events;
    ValueRecord* find(const char* key,bool create) {
        assert(std::strcmp(key,"surface-file")==0 && !create);
        events.push_back("find");
        return present?&value:nullptr;
    }
    void assign(ValueRecord& r,const std::string& converted) {
        events.push_back("assign:"+converted);
        r.published_string=0x123456u;
    }
    std::optional<std::string> convert_utf8(std::uint32_t src,std::size_t cap) {
        assert(src==0xBABAu && cap==0x200u);
        events.push_back("utf8");
        if(!conversion_ok)return std::nullopt;
        return std::string("converted");
    }
};

struct FileHost {
    bool open_ok=true;
    std::size_t bytes_size=4;
    std::vector<std::string> events;
    std::string parsed;
    std::uintptr_t open(const char* filename,const char* mode,std::uint32_t flags) {
        assert(std::strcmp(filename,"scripts/testsurface.txt")==0);
        assert(std::strcmp(mode,"rb")==0 && flags==0u);
        events.push_back("open");
        return open_ok?0x3456u:0;
    }
    std::size_t size(std::uintptr_t h) {
        assert(h==0x3456u);events.push_back("size");return bytes_size;
    }
    void read(std::uintptr_t h,char* dst,std::size_t count) {
        assert(h==0x3456u && count==bytes_size);
        events.push_back("read");
        for(std::size_t i=0;i<count;++i)dst[i]="abcd"[i%4];
    }
    void close(std::uintptr_t h) {assert(h==0x3456u);events.push_back("close");}
    void parse_surface(const char* path,const char* data) {
        assert(std::strcmp(path,"scripts/testsurface.txt")==0);
        events.push_back("parse");parsed=data;
    }
    void warning(const char* path,const char* manifest) {
        assert(std::strcmp(path,"scripts/testsurface.txt")==0);
        assert(std::strcmp(manifest,kOriginalManifest)==0);
        events.push_back("warn");
    }
};
struct BridgeHost {
    std::vector<std::string> calls;
    void method4(std::uintptr_t a,std::uintptr_t b) {
        assert(a==0x11u && b==0x22u);calls.push_back("slot4");
    }
    void method8(std::uintptr_t a,std::uintptr_t b) {
        assert(a==0x11u && b==0x22u);calls.push_back("slot8");
    }
    void method12(std::uintptr_t a,std::uintptr_t b) {
        assert(a==0 && b==0);calls.push_back("slot12");
    }
};
int main() {
    static_assert(sizeof(NativeValuePrefix32)==20,"native x86 layout");
    assert(kGetValueStringVA==0x241667C0u && kReadSurfaceFileVA==0x24191EC0u);
    assert(kValueTagTableVA==0x241668A8u);
    ValueHost v;
    v.present=false;
    assert(get_value_string(v,"surface-file",0xDEADu)==0xDEADu);
    v.present=true;
    v.value.tag=0;
    assert(get_value_string(v,"surface-file",0xDEADu)==0xDEADu);
    v.value.tag=6;
    assert(get_value_string(v,"surface-file",0xDEADu)==0xDEADu);
    v.value.tag=1;v.value.published_string=0x2222u;
    assert(get_value_string(v,"surface-file",0xDEADu)==0x2222u);
    assert(v.events.back()=="find");
    v.value.tag=2;v.value.scalar_bits=0xFFFFFFFFu;
    assert(get_value_string(v,"surface-file",0xDEADu)==0x123456u);
    assert(v.events.back()=="assign:-1");
    v.value.tag=4;v.value.scalar_bits=101;
    assert(get_value_string(v,"surface-file",0xDEADu)==0x123456u);
    assert(v.events.back()=="assign:101");
    v.value.tag=3;
    float f=1.25f;std::memcpy(&v.value.scalar_bits,&f,sizeof(f));
    assert(get_value_string(v,"surface-file",0xDEADu)==0x123456u);
    assert(v.events.back()=="assign:1.250000");
    v.value.tag=5;v.value.wide_address=0xBABAu;
    v.conversion_ok=true;
    assert(get_value_string(v,"surface-file",0xDEADu)==0x123456u);
    assert(v.events.back()=="assign:converted");
    v.conversion_ok=false;
    assert(get_value_string(v,"surface-file",0xDEADu)==0xDEADu);
    assert(v.events.back()=="utf8");

    FileHost fh;
    auto r=read_surface_file(fh,"scripts/testsurface.txt");
    assert(r.opened && r.size==4u && fh.parsed=="abcd");
    assert((fh.events==std::vector<std::string>{"open","size","read","close","parse"}));
    FileHost no;
    no.open_ok=false;
    auto miss=read_surface_file(no,"scripts/testsurface.txt");
    assert(!miss.opened && miss.size==0u);
    assert((no.events==std::vector<std::string>{"open","warn"}));
    FileHost empty;empty.bytes_size=0;
    auto e=read_surface_file(empty,"scripts/testsurface.txt");
    assert(e.opened && e.size==0u && empty.parsed.empty());
    FileHost unsafe;unsafe.bytes_size=5;
    bool threw=false;
    try {read_surface_file(unsafe,"scripts/testsurface.txt",4u);}
    catch(const std::length_error&){threw=true;}
    assert(threw);
    assert((unsafe.events==std::vector<std::string>{"open","size","close"}));

    BridgeHost bh;
    dispatch_if_both_method4(bh,0,0x22u);
    dispatch_if_both_method4(bh,0x11u,0);
    dispatch_if_both_method8(bh,0,0x22u);
    dispatch_if_both_method8(bh,0x11u,0);
    assert(bh.calls.empty());
    dispatch_if_both_method4(bh,0x11u,0x22u);
    dispatch_if_both_method8(bh,0x11u,0x22u);
    dispatch_unconditional_method12(bh,0,0);
    assert((bh.calls==std::vector<std::string>{"slot4","slot8","slot12"}));
}