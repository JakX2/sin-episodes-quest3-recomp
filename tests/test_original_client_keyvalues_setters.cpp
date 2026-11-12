#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include "sin_compat/original_client_keyvalues_setters.hpp"
namespace kv = sin_compat::original_client_keyvalues_setters;
int main(){
    kv::TestHost h;
    kv::Node root;
    assert(kv::find_or_create(h,root,"",true)==nullptr);
    assert(kv::find_or_create(h,root,"missing",false)==nullptr);
    assert(root.first_child==nullptr);
    kv::Node* node=kv::find_or_create(h,root,"group/item",true);
    assert(node && node->name=="item" && root.first_child->name=="group");
    assert(root.first_child->first_child==node);
    assert(kv::find_or_create(h,root,"group/item",false)==node);
    assert(kv::find_or_create(h,root,"group/absent",false)==nullptr);
    assert(kv::find_or_create(h,root,"group/other",true)!=nullptr);
    assert(root.first_child->first_child->next_peer!=nullptr);
    assert(h.nodes.size()==3);
    assert(kv::find_or_create(h,root,"group/",true)==nullptr);
    assert(kv::find_or_create(h,root,"/item",false)==nullptr);
    bool threw=false;
    try{(void)kv::find_or_create(h,root,std::string(241,'x'),true);}catch(const std::length_error&){threw=true;}
    assert(threw);

    // +0x20 chained fallback resolves existing content without creating child.
    kv::Node fallback;
    kv::Node* f=kv::find_or_create(h,fallback,"shared",true);
    kv::Node chained;
    chained.chain=&fallback;
    assert(kv::find_or_create(h,chained,"shared",false)==f);
    assert(chained.first_child==nullptr);
    assert(kv::find_or_create(h,chained,"not-here",false)==nullptr);
    assert(kv::find_or_create(h,chained,"new-here",true)!=nullptr);
    assert(chained.first_child && chained.first_child->name=="new-here");

    h.events.clear();
    auto* str=kv::set_string(h,root,"group/item","surface");
    assert(str==node && node->narrow=="surface" && node->type==1);
    assert(h.events.size()==3 && h.events[0]=="free-narrow" && h.events[1]=="free-wide" && h.events[2]=="alloc-narrow");
    h.events.clear();
    assert(kv::set_string(h,root,"group/item",nullptr)==node);
    assert(node->narrow.empty() && node->type==1 && h.events.size()==3);
    h.events.clear();
    assert(kv::set_wide(h,root,"group/item",u"water")==node);
    assert(node->wide==u"water" && node->type==5);
    assert(h.events.size()==3 && h.events[0]=="free-wide" && h.events[1]=="free-narrow" && h.events[2]=="alloc-wide");
    h.events.clear();
    assert(kv::set_wide(h,root,"group/item",nullptr)==node);
    assert(node->wide.empty() && node->type==5);
    assert(kv::set_int(h,root,"group/item",-137)==node);
    assert(node->raw_value==static_cast<std::uint32_t>(-137) && node->type==2);
    assert(kv::set_float(h,root,"group/item",3.25f)==node);
    assert(node->raw_value==0x40500000u && node->type==3);
    assert(kv::set_pointer_bits(h,root,"group/item",0xf0001234u)==node);
    assert(node->raw_value==0xf0001234u && node->type==4);
    threw=false;
    try{(void)kv::set_raw(h,root,"group/item",0,5);}catch(const std::invalid_argument&){threw=true;}
    assert(threw);
    assert(h.nodes.size()==5);
    return 0;
}