#pragma once
#include "response_rule_matcher.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
namespace sin_port::response_script {
namespace rr=sin_port::response_rule;
inline constexpr unsigned limit=4096;
struct Token {std::string text;unsigned line=0;bool quoted=false;};
inline bool lex(std::string_view input,std::vector<Token>& out){
    if(input.size()>4*1024*1024)return false;
    unsigned line=1;
    for(size_t p=0;p<input.size();){
        const char c=input[p];
        if(c=='\n'){++line;++p;continue;}
        if(static_cast<unsigned char>(c)<=32){if(!c)return false;++p;continue;}
        if(input.substr(p,2)=="//"){while(p<input.size()&&input[p]!='\n')++p;continue;}
        Token t{};t.line=line;
        if(c=='"'){
            t.quoted=true;++p;
            while(p<input.size()&&input[p]!='"'){
                if(input[p]=='\n'||input[p]=='\0'||t.text.size()>=127)return false;
                t.text+=input[p++];
            }
            if(p==input.size())return false;
            ++p;
        }else if(c=='{'||c=='}'){t.text+=c;++p;}
        else{
            while(p<input.size()&&static_cast<unsigned char>(input[p])>32&&input[p]!='{'&&input[p]!='}'&&input[p]!='"'&&input.substr(p,2)!="//"){
                if(t.text.size()>=127)return false;
                t.text+=input[p++];
            }
            if(t.text.empty())return false;
        }
        if(out.size()>=65536)return false;
        out.push_back(std::move(t));
    }
    return true;
}
inline bool eq(const std::string& a,const char* b){return rr::ascii_equal(a.c_str(),b);}
inline bool brace(const Token& t,char c){return !t.quoted&&t.text.size()==1&&t.text[0]==c;}
struct Cursor {
    std::vector<Token> tokens;size_t i=0;
    const Token* peek()const{return i<tokens.size()?&tokens[i]:nullptr;}
    bool is(char c)const{return peek()&&brace(*peek(),c);}
    bool take(std::string& s){if(!peek()||is('{')||is('}'))return false;s=tokens[i++].text;return true;}
    bool eat(char c){if(!is(c))return false;++i;return true;}
    bool same(unsigned line)const{return peek()&&peek()->line==line&&!is('{')&&!is('}');}
};
struct Enum {std::string name;float value=0;};
struct Criterion {
    std::string name,key,expression;float weight=1;bool required=false;
    std::vector<unsigned> children;
    std::array<unsigned char,rr::matcher_bytes> matcher{};
};
struct Entry {unsigned type=0;std::string value;float weight=1;bool first=false,last=false;};
struct Params {unsigned flags=0;int odds=100;float delay=0,delay_range=0,respeak=0,respeak_range=0;std::string soundlevel;};
struct Group {std::string name;Params params;bool deplete=true,sequential=false,norepeat=false;std::vector<Entry> entries;};
struct Rule {std::string name;bool once=false;std::vector<unsigned> criteria,groups;};
struct Database {
    std::vector<Enum> enums;
    std::vector<Criterion> criteria;
    std::vector<Group> groups;
    std::vector<Rule> rules;
    std::vector<std::string> warnings,files;
};
using Include=bool(*)(void*,const std::string&,std::string&);
inline bool enum_lookup(void* p,const char* key,float* v,bool* found) noexcept{
    const auto& db=*static_cast<const Database*>(p);
    *found=false;*v=0;
    for(const auto& e:db.enums)if(rr::ascii_equal(e.name.c_str(),key)){*v=e.value;*found=true;break;}
    return true;
}
template<class T> inline int find(const std::vector<T>& items,const std::string& name){
    for(unsigned i=0;i<items.size();++i)if(eq(items[i].name,name.c_str()))return static_cast<int>(i);
    return -1;
}
inline float number(const std::string& s){return static_cast<float>(std::atof(s.c_str()));}
inline void interval(const std::string& s,float& start,float& range){
    start=number(s);range=0;const auto comma=s.find(',');
    if(comma!=std::string::npos)range=static_cast<float>(std::atof(s.c_str()+comma+1)-start);
}
inline unsigned response_type(const std::string& s){
    if(eq(s,"speak"))return 1;
    if(eq(s,"sentence"))return 2;
    if(eq(s,"scene"))return 3;
    if(eq(s,"response"))return 4;
    if(eq(s,"print"))return 5;
    return 0;
}
struct Parser {
    Database db;Include include=nullptr;void* context=nullptr;unsigned serial=0;
    std::string error;
    bool fail(const char* message){error=message;return false;}
    bool criterion(Cursor& c,std::string name,unsigned line,unsigned& index){
        Criterion a{};a.name=std::move(name);
        bool have=false;
        while(c.peek()&&(c.same(line)||!have)){
            if(c.eat('{')){
                have=true;while(!c.eat('}')){
                    std::string child;if(!c.take(child))return fail("unclosed composite criterion");
                    const int k=find(db.criteria,child);
                    if(k<0)db.warnings.push_back("unknown subcriterion: "+child);
                    else a.children.push_back(unsigned(k));
                }
            }else{
                std::string token;if(!c.take(token))return fail("missing criterion field");
                if(eq(token,"required"))a.required=true;
                else if(eq(token,"weight")){
                    std::string value;if(!c.take(value))return fail("missing criterion weight");a.weight=number(value);
                }else{
                    a.key=std::move(token);if(!c.take(a.expression))return fail("missing criterion value");have=true;
                }
            }
        }
        if(db.criteria.size()>=limit)return fail("criterion limit");
        if(a.children.empty()&&!rr::compile(a.expression.c_str(),{&db,enum_lookup},a.matcher.data(),unsigned(a.matcher.size())))return fail("matcher compile");
        index=unsigned(db.criteria.size());db.criteria.push_back(std::move(a));return true;
    }
    bool option(Cursor& c,const std::string& key,Params& a,Entry* e){
        std::string val;
        if(eq(key,"speakonce"))a.flags|=2;
        else if(eq(key,"noscene"))a.flags|=32;
        else if(eq(key,"defaultdelay")){a.flags|=1;a.delay=2.8f;a.delay_range=3.2f-2.8f;}
        else if(eq(key,"nodelay")){
            // 22099fba/2209c9ae consumes one additional token.
            if(!c.take(val))return fail("missing nodelay operand");
            a.flags|=1;a.delay=a.delay_range=0;
        }else if(eq(key,"delay")||eq(key,"respeakdelay")){
            if(!c.take(val))return fail("missing interval");
            if(eq(key,"delay")){a.flags|=1;interval(val,a.delay,a.delay_range);}
            else{a.flags|=8;interval(val,a.respeak,a.respeak_range);}
        }else if(eq(key,"odds")){
            if(!c.take(val))return fail("missing odds");
            a.flags|=4;a.odds=std::clamp(std::atoi(val.c_str()),0,100);
        }else if(eq(key,"soundlevel")){
            if(!c.take(a.soundlevel))return fail("missing soundlevel");
            a.flags|=16;
        }else if(e&&eq(key,"weight")){
            if(!c.take(val))return fail("missing entry weight");
            e->weight=number(val);
        }else if(e&&eq(key,"displayfirst"))e->first=true;
        else if(e&&eq(key,"displaylast"))e->last=true;
        else db.warnings.push_back("unknown response option: "+key);
        return true;
    }
    bool entry(Cursor& c,std::string type,unsigned line,Group& g){
        Entry e{};e.type=response_type(type);
        if(!e.type)return fail("unknown response type");
        if(!c.take(e.value))return fail("missing response value");
        while(c.same(line)){std::string key;c.take(key);if(!option(c,key,g.params,&e))return false;}
        if(g.entries.size()>=limit)return fail("entry limit");
        g.entries.push_back(std::move(e));return true;
    }
    bool group(Cursor& c,std::string name,unsigned line){
        Group g{};g.name=std::move(name);
        if(c.eat('{')){
            while(!c.eat('}')){
                if(!c.peek())return fail("unclosed response group");
                const unsigned ln=c.peek()->line;
                std::string t;if(!c.take(t))return fail("invalid response group token");
                if(eq(t,"permitrepeats"))g.deplete=false;
                else if(eq(t,"sequential"))g.sequential=true;
                else if(eq(t,"norepeat"))g.norepeat=true;
                else if(!entry(c,std::move(t),ln,g))return false;
            }
        }else while(c.same(line)){
            std::string key;c.take(key);
            if(response_type(key)){if(!entry(c,std::move(key),line,g))return false;}
            else if(!option(c,key,g.params,nullptr))return false;
        }
        if(db.groups.size()>=limit)return fail("group limit");
        db.groups.push_back(std::move(g));return true;
    }
    bool rule(Cursor& c,std::string name){
        Rule r{};r.name=std::move(name);bool discard=false;
        if(!c.eat('{'))return fail("rule expects opening brace");
        while(!c.eat('}')){
            if(!c.peek())return fail("unclosed rule");
            const unsigned line=c.peek()->line;
            std::string key;if(!c.take(key))return fail("invalid rule token");
            if(eq(key,"matchonce"))r.once=true;
            else if(eq(key,"criteria")||eq(key,"criterion")||eq(key,"response")){
                const bool groups=eq(key,"response");
                while(c.same(line)){
                    std::string name2;c.take(name2);
                    const int i=groups?find(db.groups,name2):find(db.criteria,name2);
                    if(i<0){discard=true;db.warnings.push_back("unresolved reference "+name2+" in "+r.name);}
                    else (groups?r.groups:r.criteria).push_back(unsigned(i));
                }
            }else{
                // Unknown rule token is the first match key of an inline criterion.
                --c.i;char suffix[32]{};
                std::snprintf(suffix,sizeof(suffix),"%03u",++serial);
                unsigned idx=0;
                if(!criterion(c,"["+r.name+suffix+"]",line,idx))return false;
                r.criteria.push_back(idx);
            }
        }
        if(!discard){if(db.rules.size()>=limit)return fail("rule limit");db.rules.push_back(std::move(r));}
        return true;
    }
    bool enumeration(Cursor& c,const std::string& name){
        if(!c.eat('{'))return fail("enumeration expects opening brace");
        while(!c.eat('}')){
            std::string key,val;if(!c.take(key)||!c.take(val))return fail("invalid enumeration");
            std::string full="["+name+"::"+key+"]";
            if(find(db.enums,full)>=0)db.warnings.push_back("duplicate enumeration: "+full);
            else{if(db.enums.size()>=limit)return fail("enum limit");db.enums.push_back({std::move(full),number(val)});}
        }
        return true;
    }
    bool file(const std::string& name,std::string_view data,std::vector<std::string>& stack){
        if(stack.size()>=16||std::find(stack.begin(),stack.end(),name)!=stack.end())return fail("include cycle/depth");
        Cursor c{};if(!lex(data,c.tokens))return fail("tokenization failed");
        stack.push_back(name);db.files.push_back(name);
        while(c.peek()){
            const unsigned line=c.peek()->line;
            std::string type,n;if(!c.take(type)||!c.take(n))return fail("missing top-level name");
            if(eq(type,"#include")){
                std::string src;
                if(!include||!include(context,n,src))return fail("include unavailable");
                if(!file(n,src,stack))return false;
            }else if(eq(type,"criterion")||eq(type,"criteria")){unsigned idx=0;if(!criterion(c,std::move(n),line,idx))return false;}
            else if(eq(type,"response")){if(!group(c,std::move(n),line))return false;}
            else if(eq(type,"rule")){if(!rule(c,std::move(n)))return false;}
            else if(eq(type,"enumeration")){if(!enumeration(c,n))return false;}
            else return fail("unknown top-level token");
        }
        stack.pop_back();return true;
    }
    bool parse(const std::string& name,std::string_view text){
        db={};serial=0;error.clear();std::vector<std::string> stack;
        if(file(name,text,stack))return true;
        db={};return false;
    }
};
}
