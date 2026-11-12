// Native-SiN-derived branch behavior, not a game executable test.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include "sin_compat/original_client_keyvalues_lifetime.hpp"
using namespace sin_compat::original_client_keyvalues_lifetime;

struct Host {
    std::vector<std::string> calls;
    void release_node(Node* node) { calls.push_back("node:" + std::to_string(node->name_symbol)); }
    void release_buffer(std::uintptr_t token) { calls.push_back("buffer:" + std::to_string(token)); }
};

static void test_native_constructor_fields() {
    NativeKeyValues40 n;
    auto* raw=reinterpret_cast<unsigned char*>(&n);
    for(std::size_t i=0;i<sizeof(n);++i) raw[i]=0x7b;
    std::string called;
    auto* returned=construct_native_snapshot(n,"sibling",[&](const char* s){
        assert(n.name_00==0xffffffffu); // original writes sentinel before intern
        assert(n.type_10==0 && n.child_1c==0 && n.next_18==0 && n.fallback_20==0);
        assert(n.text_04==0 && n.wide_08==0 && n.number_0c==0 && n.escape_24==0);
        called=s;return 0x1234u;
    });
    assert(returned==&n && called=="sibling" && n.name_00==0x1234u);
    assert(n.unknown_14==0x7b7b7b7bu);
    for(std::size_t i=0;i<3;++i) assert(n.untouched_25_27[i]==0x7bu);
    assert(kCtorVA==0x24165010u && kDeletingDtorVA==0x24165050u);
}
static void test_child_then_peers_release_order() {
    Node root;root.name_symbol=1;root.text=11;root.wide=12;
    Node a;a.name_symbol=2;a.text=21;a.wide=22;
    Node b;b.name_symbol=3;b.text=31;b.wide=32;
    Node grand;grand.name_symbol=4;grand.text=41;grand.wide=42;
    Node peer;peer.name_symbol=5;peer.text=51;peer.wide=52;
    root.first_child=&a;a.next=&b;a.first_child=&grand;root.next=&peer;
    Host h;destroy_contents(root,h);
    const std::vector<std::string> expected={
        "buffer:41","buffer:42","node:4",
        "buffer:21","buffer:22","node:2",
        "buffer:31","buffer:32","node:3",
        "buffer:51","buffer:52","node:5",
        "buffer:11","buffer:12"};
    assert(h.calls==expected);
    assert(root.first_child==nullptr && root.next==nullptr);
    assert(root.text==0 && root.wide==0);
}
static void test_deleting_flag_0_1_2_3() {
    for(std::uint32_t flag=0;flag<4;++flag){
        Node n; n.name_symbol=7;n.text=10;n.wide=0;
        Host h; assert(scalar_deleting_destructor(n,flag,h)==&n);
        assert(h.calls.size()==((flag&1u)?3u:2u));
        assert(h.calls[0]=="buffer:10" && h.calls[1]=="buffer:0");
        if(flag&1u) assert(h.calls[2]=="node:7");
    }
}
static void test_self_peer_native_stop() {
    Node root;root.name_symbol=8;root.next=&root;
    Host h;destroy_contents(root,h);
    assert(h.calls==std::vector<std::string>({"buffer:0","buffer:0"}));
}
static void test_host_guards() {
    Node a,b; a.first_child=&b;b.first_child=&a;
    Host h;
    bool caught=false;
    try{destroy_contents(a,h);}catch(const std::length_error&){caught=true;}
    assert(caught);
    Node root,child;root.first_child=&child;
    caught=false;
    try{destroy_contents(root,h,1);}catch(const std::length_error&){caught=true;}
    assert(caught);
}
int main(){
    test_native_constructor_fields();
    test_child_then_peers_release_order();
    test_deleting_flag_0_1_2_3();
    test_self_peer_native_stop();
    test_host_guards();
}