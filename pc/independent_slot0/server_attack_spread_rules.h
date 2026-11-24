#pragma once
// Phase678: independent bounded parser for the retail AttackSpreadRules schema.
// Reuses the project lexer, not the proprietary KeyValues object ABI.
#include "client_stats_script_parser.h"
#include <cerrno>
#include <cmath>
#include <cstdlib>

namespace sin_independent::attack_spread {
struct Node {
    std::string name,value;
    bool object=false;
    std::vector<Node> children;
};
inline constexpr std::size_t max_bytes=256*1024;
inline constexpr float fallback=0.13053f;
inline char fold(char c) noexcept {return c>='A'&&c<='Z'?static_cast<char>(c-'A'+'a'):c;}
inline bool equal(std::string_view a,std::string_view b) noexcept {
    if(a.size()!=b.size())return false;
    for(std::size_t i=0;i<a.size();++i)if(fold(a[i])!=fold(b[i]))return false;
    return true;
}
inline const Node* find(const Node& n,std::string_view key) noexcept {
    for(const auto& child:n.children)if(equal(child.name,key))return &child;
    return nullptr;
}
inline const Node* mapped(const Node& n,std::string_view group,std::string_view key) noexcept {
    const auto* table=find(n,group);
    if(!table||!table->object)return nullptr;
    auto* entry=find(*table,key);
    if(!entry)entry=find(*table,"*");
    if(!entry||entry->object)return nullptr;
    return find(n,entry->value);
}
inline bool number(const Node& n,float& value) noexcept {
    if(n.object||n.value.empty())return false;
    char* end=nullptr;
    errno=0;
    const auto v=std::strtof(n.value.c_str(),&end);
    if(end!=n.value.c_str()+n.value.size()||errno==ERANGE||!std::isfinite(v)||v<0.f)return false;
    value=v;return true;
}
using Lexer=sin_independent::stats_scripts::Lexer;
inline bool parse_children(Lexer& lexer,Node& node,unsigned depth,std::size_t& count){
    if(depth>8)return false;
    while(true){
        auto key=lexer.Next();
        if(key.type==Lexer::Type::Close)return true;
        if(key.type!=Lexer::Type::Word||key.text.empty()||++count>8192||find(node,key.text))return false;
        Node child;child.name=std::move(key.text);
        auto value=lexer.Next();
        if(value.type==Lexer::Type::Open){
            child.object=true;
            if(!parse_children(lexer,child,depth+1,count))return false;
        }else if(value.type==Lexer::Type::Word)child.value=std::move(value.text);
        else return false;
        node.children.push_back(std::move(child));
    }
}
inline bool validate(const Node& n,unsigned tier) noexcept {
    constexpr const char* groups[]={"class_names","weapon_class_names","target_class_names"};
    const auto* table=find(n,groups[tier]);
    if(!table||!table->object||table->children.empty())return false;
    for(const auto& entry:table->children){
        if(entry.object||entry.value.empty()||equal(entry.value,groups[tier]))return false;
        const auto* rule=find(n,entry.value);
        if(!rule)return false;
        if(tier==2){float v=0.f;if(!number(*rule,v))return false;}
        else if(!rule->object||!validate(*rule,tier+1))return false;
    }
    return true;
}
class Rules {
public:
    bool load(std::string_view text) {
        if(text.empty()||text.size()>max_bytes||text.find('\0')!=std::string_view::npos)return false;
        Lexer lexer(text);
        auto key=lexer.Next();
        if(key.type!=Lexer::Type::Word||!equal(key.text,"AttackSpreadRules")||
           lexer.Next().type!=Lexer::Type::Open)return false;
        Node next;next.object=true;next.name=std::move(key.text);
        std::size_t count=0;
        if(!parse_children(lexer,next,0,count)||lexer.Next().type!=Lexer::Type::End||
           !validate(next,0))return false;
        root_=std::move(next);
        return true;
    }
    void clear() noexcept {root_=Node{};}
    bool resolve(std::string_view npc,std::string_view weapon,std::string_view target,float& value) const noexcept {
        const auto* a=mapped(root_,"class_names",npc);
        const auto* b=a?mapped(*a,"weapon_class_names",weapon):nullptr;
        const auto* c=b?mapped(*b,"target_class_names",target):nullptr;
        if(c&&number(*c,value))return true;
        value=fallback;return false;
    }
private:
    Node root_;
};
}
