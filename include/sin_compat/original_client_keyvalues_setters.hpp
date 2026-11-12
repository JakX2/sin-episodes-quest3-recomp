#pragma once
// Phase183: host-neutral branch model for six SHA-verified original 2006
// client.dll KeyValues routines. These are not usable Source engine objects.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace sin_compat::original_client_keyvalues_setters {
inline constexpr std::uint32_t kFindCreateVA = 0x241656E0u;
inline constexpr std::uint32_t kSetStringVA = 0x24165AE0u;
inline constexpr std::uint32_t kSetWideStringVA = 0x24165B60u;
inline constexpr std::uint32_t kSetIntVA = 0x24165BE0u;
inline constexpr std::uint32_t kSetFloatVA = 0x24165C10u;
inline constexpr std::uint32_t kSetPointerVA = 0x24165C40u;

// Original x86 instance fields validated by MOV instruction witnesses.
// This is only a byte-layout prefix, not a fully recovered C++ class.
struct NativeKeyValuesFields32 {
    std::uint32_t name_symbol_00;
    std::uint32_t narrow_04;
    std::uint32_t wide_08;
    std::uint32_t raw_value_0c;
    std::uint32_t type_10;
    std::uint32_t opaque_14;
    std::uint32_t next_peer_18;
    std::uint32_t first_child_1c;
    std::uint32_t chain_20;
    std::uint8_t unknown_24[4];
};
static_assert(sizeof(NativeKeyValuesFields32) == 0x28, "original KeyValues allocation is 0x28 bytes");
static_assert(offsetof(NativeKeyValuesFields32, narrow_04) == 4, "native narrow value offset");
static_assert(offsetof(NativeKeyValuesFields32, wide_08) == 8, "native wide value offset");
static_assert(offsetof(NativeKeyValuesFields32, raw_value_0c) == 12, "native numeric payload offset");
static_assert(offsetof(NativeKeyValuesFields32, type_10) == 16, "native tagged type offset");
static_assert(offsetof(NativeKeyValuesFields32, next_peer_18) == 0x18, "native peer offset");
static_assert(offsetof(NativeKeyValuesFields32, first_child_1c) == 0x1c, "native first child offset");
static_assert(offsetof(NativeKeyValuesFields32, chain_20) == 0x20, "native chained fallback offset");
static_assert(std::is_standard_layout<NativeKeyValuesFields32>::value,"native field-prefix layout");

struct Node {
    std::uint32_t name_symbol = 0;
    std::string name;
    std::string narrow;
    std::u16string wide;
    std::uint32_t raw_value = 0;
    std::uint32_t type = 0;
    Node* next_peer = nullptr;
    Node* first_child = nullptr;
    Node* chain = nullptr;
};

// The real native routine invokes KeyValuesSystem::GetSymbol and a separate
// original allocator/constructor. This fixture host owns allocations and may
// select different symbol case rules; no concrete engine symbol ABI is assumed.
struct TestHost {
    std::vector<std::unique_ptr<Node>> nodes;
    std::vector<std::string> events;
    std::uint32_t symbol(std::string_view name) {
        std::uint32_t h=2166136261u;
        for (unsigned char c: name) h=(h^c)*16777619u;
        return h;
    }
    Node* create(std::string_view name, std::uint32_t sym) {
        auto p=std::make_unique<Node>();
        p->name=std::string(name);
        p->name_symbol=sym;
        Node* raw=p.get();nodes.emplace_back(std::move(p));
        events.emplace_back("construct");
        return raw;
    }
    void free_narrow(Node&) { events.emplace_back("free-narrow"); }
    void free_wide(Node&) { events.emplace_back("free-wide"); }
    void alloc_narrow(Node&, std::size_t) { events.emplace_back("alloc-narrow"); }
    void alloc_wide(Node&, std::size_t) { events.emplace_back("alloc-wide"); }
};

// Original 0x241656E0: nonempty slash-separated path; GetSymbol for the
// current segment; compare child symbol DWORD, then the +0x20 fallback chain;
// when create is nonzero, construct a 0x28-byte node and append at peer tail,
// set parent type +0x10 to zero and recurse on the suffix after '/'. Native
// uses a fixed-size stack buffer and no allocation/cycle checks. The two
// guards here explicitly protect host tests rather than claiming native logic.
template<class Host>
Node* find_or_create(Host& host, Node& parent, std::string_view path,
                     bool create, std::size_t depth = 0) {
    if (path.empty()) return nullptr;
    if (depth >= 64) throw std::length_error("host KeyValues recursion/cycle guard");
    const std::size_t slash=path.find('/');
    std::string_view part=path.substr(0,slash);
    if (part.size() > 240) throw std::length_error("host guard on native fixed-size slash buffer");
    const std::uint32_t sym=host.symbol(part);
    Node* previous=nullptr;
    Node* found=nullptr;
    std::size_t steps=0;
    for(Node* node=parent.first_child;node;node=node->next_peer){
        if (++steps>65536) throw std::length_error("host KeyValues sibling cycle guard");
        previous=node;
        if(node->name_symbol==sym){found=node;break;}
    }
    if(!found && parent.chain){
        found=find_or_create(host,*parent.chain,path,false,depth+1);
    }
    if(!found){
        if(!create) return nullptr;
        found=host.create(part,sym);
        if(!found) throw std::runtime_error("host guard on native null allocated child");
        if(previous) previous->next_peer=found;
        else parent.first_child=found;
        found->next_peer=nullptr;
        parent.type=0;
    }
    if(slash!=std::string_view::npos)
        return find_or_create(host,*found,path.substr(slash+1),create,depth+1);
    return found;
}

// Original 0x24165AE0: find-or-create(true), release prior narrow and wide
// payloads, null wide pointer, use empty literal if incoming char* is null,
// allocate/copy strlen+1 narrow bytes, set type DWORD+0x10 to 1.
template<class Host>
Node* set_string(Host& host, Node& parent,std::string_view key,const char* text){
    Node* node=find_or_create(host,parent,key,true);
    if(!node) return nullptr;
    host.free_narrow(*node);
    host.free_wide(*node);
    node->wide.clear();
    const char* source=text?text:"";
    host.alloc_narrow(*node,std::strlen(source)+1);
    node->narrow=source;
    node->type=1;
    return node;
}

// Original 0x24165B60: same branch class for 2-byte input, use wide empty
// literal on null, allocate 2*(wcslen+1) bytes, set type DWORD to 5. This
// portable model uses char16_t; original Win32 wchar_t is two bytes.
template<class Host>
Node* set_wide(Host& host, Node& parent,std::string_view key,const char16_t* text){
    Node* node=find_or_create(host,parent,key,true);
    if(!node) return nullptr;
    host.free_wide(*node);
    host.free_narrow(*node);
    node->narrow.clear();
    const char16_t* src=text?text:u"";
    std::size_t n=0;while(src[n]) ++n;
    host.alloc_wide(*node,(n+1)*2);
    node->wide.assign(src,n);
    node->type=5;
    return node;
}

// Three separate verified native 33-byte wrappers at 0x24165BE0/5C10/5C40.
// They find-or-create(true), write raw DWORD +0x0C, then tag +0x10 = 2,3,4.
// The float stores unmodified IEEE754 bits, not a numeric decimal conversion.
template<class Host>
Node* set_raw(Host& host, Node& parent,std::string_view key,
              std::uint32_t bits,std::uint32_t tag){
    if(tag!=2 && tag!=3 && tag!=4) throw std::invalid_argument("unsupported original type wrapper");
    Node* node=find_or_create(host,parent,key,true);
    if(node){node->raw_value=bits;node->type=tag;}
    return node;
}
template<class Host>
Node* set_int(Host& h,Node& p,std::string_view key,std::int32_t value){
    return set_raw(h,p,key,static_cast<std::uint32_t>(value),2);
}
template<class Host>
Node* set_float(Host& h,Node& p,std::string_view key,float value){
    static_assert(sizeof(float)==4,"native x86 KeyValues float width");
    std::uint32_t bits=0;std::memcpy(&bits,&value,4);
    return set_raw(h,p,key,bits,3);
}
template<class Host>
Node* set_pointer_bits(Host& h,Node& p,std::string_view key,std::uint32_t value){
    return set_raw(h,p,key,value,4);
}
} // namespace sin_compat::original_client_keyvalues_setters