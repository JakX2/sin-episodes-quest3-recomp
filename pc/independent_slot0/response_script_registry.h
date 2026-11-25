#pragma once
#include "response_script_parser.h"
#include "response_rule_scoring.h"
#include "response_group_choice.h"
#include <limits>
namespace sin_port::response_script {
// Owned, resolver-backed original-layout records. The 32-bit words are
// opaque handles, NOT process pointers or an authentic CResponseSystem.
struct Registry {
    std::array<unsigned char,0xcc> system{};
    std::vector<std::vector<unsigned char>> blocks;
    std::vector<std::string> strings,rule_names,group_names;
    std::vector<Enum> enums;
    using Bytes=rr::Bytes;
    template<class T> static void put(unsigned char* p,unsigned off,T v){std::memcpy(p+off,&v,sizeof(v));}
    unsigned block(unsigned size){
        if(!size)return 0;
        blocks.emplace_back(size,0);return unsigned(blocks.size());
    }
    unsigned string_word(const std::string& s){
        strings.push_back(s);return unsigned(strings.size());
    }
    unsigned ids(const std::vector<unsigned>& values){
        const auto word=block(unsigned(values.size())*4);
        if(word)for(unsigned i=0;i<values.size();++i)put(blocks[word-1].data(),i*4,values[i]);
        return word;
    }
    bool storage(unsigned word,Bytes& out)const noexcept{
        if(!word||word>blocks.size())return false;
        const auto& b=blocks[word-1];out={b.data(),unsigned(b.size())};return true;
    }
    bool string(unsigned word,const char*& out)const noexcept{
        if(!word||word>strings.size())return false;
        out=strings[word-1].c_str();return true;
    }
    static bool enum_lookup(void* p,const char* key,float* v,bool* found) noexcept{
        const auto& r=*static_cast<const Registry*>(p);*v=0;*found=false;
        for(const auto& e:r.enums)if(rr::ascii_equal(e.name.c_str(),key)){*v=e.value;*found=true;break;}
        return true;
    }
    int group_by_name(const std::string& name)const{
        for(unsigned i=0;i<group_names.size();++i)if(eq(group_names[i],name.c_str()))return int(i);
        return -1;
    }
    bool row(unsigned off,unsigned stride,unsigned payload,unsigned i,Bytes& out)const noexcept{
        if(i>=rr::group_get<unsigned>(system.data(),off+4))return false;
        Bytes b{};if(!storage(rr::group_get<unsigned>(system.data(),off),b)||!b.has(i*stride+0x14+payload))return false;
        out={b.p+i*stride+0x14,payload};return true;
    }
    unsigned table(unsigned off,unsigned count,unsigned stride){
        const auto word=block(count*stride);put(system.data(),off,word);put(system.data(),off+4,count);
        put(system.data(),off+0x10,count);
        if(word)for(unsigned i=0;i<count;++i)put(blocks[word-1].data()+i*stride,0,0xffffffffu);
        return word;
    }
    // Build a complete private snapshot, publish only on success.
    bool build(const Database& db){
        Registry r;
        if(db.criteria.size()>limit||db.groups.size()>limit||db.rules.size()>limit||db.enums.size()>limit)return false;
        r.enums=db.enums;
        const auto cs=r.table(0x60,unsigned(db.criteria.size()),rr::criterion_stride);
        const auto gs=r.table(8,unsigned(db.groups.size()),rr::group_stride);
        const auto rs=r.table(0xb8,unsigned(db.rules.size()),rr::rule_stride);
        for(unsigned i=0;i<db.criteria.size();++i){
            const auto& c=db.criteria[i];
            if(c.children.size()>limit)return false;
            for(auto k:c.children)if(k>=db.criteria.size())return false;
            const auto key=r.string_word(c.key),expr=r.string_word(c.expression),vec=r.ids(c.children);
            auto* p=r.blocks[cs-1].data()+i*rr::criterion_stride+0x14;
            put(p,0,key);put(p,4,expr);put(p,8,c.weight);p[0xc]=c.required;
            std::memcpy(p+0x10,c.matcher.data(),c.matcher.size());
            put(p,0x124,vec);put(p,0x128,unsigned(c.children.size()));put(p,0x130,unsigned(c.children.size()));put(p,0x134,vec);
        }
        for(unsigned i=0;i<db.groups.size();++i){
            const auto& g=db.groups[i];
            if(g.entries.size()>limit||!g.params.soundlevel.empty())return false; // soundlevel resolver not implemented
            r.group_names.push_back(g.name);
            const auto entries=r.block(unsigned(g.entries.size())*rr::response_row_bytes);
            for(unsigned j=0;j<g.entries.size();++j){
                const auto& e=g.entries[j];if(e.type<1||e.type>5)return false;
                const auto value=r.string_word(e.value);
                auto* p=r.blocks[entries-1].data()+j*rr::response_row_bytes;
                put(p,0,e.type);put(p,4,value);put(p,8,e.weight);p[0x10]=e.first;p[0x11]=e.last;
            }
            auto* p=r.blocks[gs-1].data()+i*rr::group_stride+0x14;
            put(p,0,entries);put(p,4,unsigned(g.entries.size()));put(p,0xc,unsigned(g.entries.size()));put(p,0x10,entries);
            put(p,0x14,g.params.flags);put(p,0x18,g.params.odds);put(p,0x1c,g.params.delay);put(p,0x20,g.params.delay_range);
            put(p,0x24,g.params.respeak);put(p,0x28,g.params.respeak_range);
            p[0x30]=g.deplete;put(p,0x34,1u);p[0x3a]=g.sequential;p[0x3b]=g.norepeat;p[0x3c]=1;
        }
        for(unsigned i=0;i<db.rules.size();++i){
            const auto& rule=db.rules[i];
            if(rule.criteria.size()>limit||rule.groups.size()>limit)return false;
            for(auto k:rule.criteria)if(k>=db.criteria.size())return false;
            for(auto k:rule.groups)if(k>=db.groups.size())return false;
            r.rule_names.push_back(rule.name);
            const auto c=r.ids(rule.criteria),g=r.ids(rule.groups);
            auto* node=r.blocks[rs-1].data()+i*rr::rule_stride;
            put(node,0x10,static_cast<std::uint16_t>(i));
            auto* p=node+0x14;
            put(p,0,c);put(p,4,unsigned(rule.criteria.size()));put(p,0xc,unsigned(rule.criteria.size()));put(p,0x10,c);
            put(p,0x14,g);put(p,0x18,unsigned(rule.groups.size()));put(p,0x20,unsigned(rule.groups.size()));put(p,0x24,g);
            p[0x28]=rule.once;p[0x29]=1;
        }
        *this=std::move(r);return true;
    }
};
}
