#ifdef NDEBUG
#undef NDEBUG // MSVC Windows Release tests must not become inert.
#endif
#include "sin_compat/original_client_surface_manifest.hpp"
#include <cassert>
#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
using namespace sin_compat::original_client_surface_manifest;

struct Host {
    std::vector<std::string> events;
    std::vector<std::string> names;
    std::vector<int> next;
    bool parse_ok = true;
    bool zero_allocation = false;
    bool zero_construction = false;
    std::uintptr_t allocate_record(std::uint32_t bytes) {
        assert(bytes == 0x28u);
        events.push_back("allocate");
        return zero_allocation ? 0u : 0x1000u;
    }
    std::uintptr_t construct_record(std::uintptr_t mem,const char* path) {
        assert(mem == 0x1000u && std::string(path) == kManifestPath);
        events.push_back("construct");
        return zero_construction ? 0u : 0x2000u;
    }
    bool parse_manifest(std::uintptr_t kv,std::uintptr_t fs,
                        const char* path,const char* search) {
        assert(kv == 0x2000u && fs == 0x4000u);
        assert(std::string(path) == kManifestPath);
        assert(std::string(search) == "GAME");
        events.push_back("parse"); return parse_ok;
    }
    std::uintptr_t first_key(std::uintptr_t kv) {
        assert(kv == 0x2000u); events.push_back("first");
        return names.empty() ? 0u : 0x3000u;
    }
    std::uintptr_t next_key(std::uintptr_t v) {
        events.push_back("next");
        const auto i = static_cast<std::size_t>(v - 0x3000u);
        assert(i < next.size());
        return next[i] < 0 ? 0u : 0x3000u+static_cast<std::uintptr_t>(next[i]);
    }
    const char* key_name(std::uintptr_t v) {
        events.push_back("name");
        const auto i=static_cast<std::size_t>(v-0x3000u);
        assert(i<names.size());return names[i].c_str();
    }
    bool is_file_type(const char* candidate,const char* expected) {
        events.push_back("compare");
        std::string lhs(candidate),rhs(expected);
        for(char& c:lhs)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        for(char& c:rhs)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return lhs==rhs;
    }
    void load_surface_file(std::uintptr_t surface,std::uintptr_t fs,std::uintptr_t key) {
        assert(surface==0x5000u && fs==0x4000u && key>=0x3000u);
        events.push_back("load");
    }
    void report_load_failure(const char* manifest) {
        assert(std::string(manifest)==kManifestPath);events.push_back("missing");
    }
    void report_unknown_type(const char* manifest,const char* type) {
        assert(std::string(manifest)==kManifestPath);
        events.push_back(std::string("unknown:")+type);
    }
    void destroy_record(std::uintptr_t kv) {assert(kv==0x2000u);events.push_back("destroy");}
};

int main() {
    static_assert(sizeof(NativeKeyValuesLinkLayout32)==32,"x86 KeyValues layout");
    static_assert(offsetof(NativeKeyValuesLinkLayout32,first_child_1c)==28,"first offset");
    assert(kManifestFunctionVA==0x241927A0u && kKeyValuesReleaseVA==0x24166050u);
    KeyNode a{3,nullptr,nullptr}, b{5,nullptr,nullptr}, c{7,nullptr,nullptr}, root{0,nullptr,&a};
    a.next=&b;b.next=&c;
    assert(first_subkey(&root)==&a);
    assert(next_key(&b)==&c);
    assert(find_subkey_by_symbol(&root,7)==&c);
    assert(find_subkey_by_symbol(&root,99)==nullptr);
    c.next=&a;
    bool cycled=false;
    try {(void)find_subkey_by_symbol(&root,99,4);}catch(const std::length_error&){cycled=true;}
    assert(cycled);
    c.next=nullptr;
    std::vector<std::string> forwards;
    assert(forward_alloc_0x24164c60([&](std::uint32_t amount){forwards.push_back("alloc");return amount+1;},40)==41);
    std::vector<std::string> frees;
    release_0x24166050(0, [&](std::uintptr_t){frees.push_back("payload");},[&](std::uintptr_t){frees.push_back("free");});
    assert(frees.empty());
    release_0x24166050(0x10u,[&](std::uintptr_t p){assert(p==0x10u);frees.push_back("payload");},[&](std::uintptr_t p){assert(p==0x10u);frees.push_back("free");});
    assert((frees==std::vector<std::string>{"payload","free"}));
    {
        Host h;h.parse_ok=false;
        const auto stats=load_manifest(h,0x5000u,0x4000u);
        assert(!stats.parsed && stats.visited==0);
        assert((h.events==std::vector<std::string>{"allocate","construct","parse","missing","destroy"}));
    }
    {
        Host h;const auto stats=load_manifest(h,0x5000u,0x4000u);
        assert(stats.parsed && stats.visited==0);
        assert((h.events==std::vector<std::string>{"allocate","construct","parse","first","destroy"}));
    }
    {
        Host h;h.names={"file","misc","FILE","File"};h.next={1,2,3,-1};
        const auto stats=load_manifest(h,0x5000u,0x4000u);
        assert(stats.parsed && stats.visited==4 && stats.file_entries==3 && stats.unknown_entries==1);
        assert(h.events.front()=="allocate" && h.events.back()=="destroy");
        std::size_t loads=0,unknown=0;
        for(const auto& x:h.events){if(x=="load")++loads; if(x=="unknown:misc")++unknown;}
        assert(loads==3 && unknown==1);
    }
    {
        Host h;h.names={"file"};h.next={0};bool exception=false;
        try {(void)load_manifest(h,0x5000u,0x4000u,3);}catch(const std::length_error&){exception=true;}
        assert(exception);
    }
    {
        Host h;h.zero_allocation=true;bool exception=false;
        try {(void)load_manifest(h,0x5000u,0x4000u);}catch(const std::runtime_error&){exception=true;}
        assert(exception && h.events.size()==1);
    }
    {
        Host h;h.zero_construction=true;bool exception=false;
        try {(void)load_manifest(h,0x5000u,0x4000u);}catch(const std::runtime_error&){exception=true;}
        assert(exception && h.events.size()==2);
    }
}