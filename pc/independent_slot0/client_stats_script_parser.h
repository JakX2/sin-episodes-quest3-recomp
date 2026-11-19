#pragma once
// Independently owned parser for SiN Episodes 2006 stats script formats.
// NOT a binary-compatible substitute for the proprietary KeyValues ABI.
#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
namespace sin_independent::stats_scripts {
struct Definition {std::string name,short_text,long_text,percentage_of;};
struct DisplayEntry {
    std::string name,format;
    std::size_t line=0;
    bool divider=false;
};
class Lexer {
public:
    enum class Type {Word,Open,Close,End,Error};
    struct Token {Type type;std::string text;std::size_t line;};
    explicit Lexer(std::string_view src):source_(src){}
    Token Next(){
        while(index_<source_.size()){
            const char c=source_[index_];
            if(c=='\n'){++line_;++index_;continue;}
            if(c==' '||c=='\r'||c=='\t'||c=='\v'||c=='\f'){++index_;continue;}
            if(c=='/'&&index_+1<source_.size()&&source_[index_+1]=='/'){
                index_+=2;
                while(index_<source_.size()&&source_[index_]!='\n')++index_;
                continue;
            }
            break;
        }
        if(index_==source_.size())return {Type::End,{},line_};
        const auto line=line_;
        char c=source_[index_++];
        if(c=='{')return {Type::Open,{},line};
        if(c=='}')return {Type::Close,{},line};
        if(c=='"'){
            std::string s;
            while(index_<source_.size()){
                c=source_[index_++];
                if(c=='"')return {Type::Word,std::move(s),line};
                if(c=='\n'||c=='\r')return {Type::Error,"newline in quoted token",line};
                if(c=='\\'&&index_<source_.size()&&
                   (source_[index_]=='"'||source_[index_]=='\\'))
                    c=source_[index_++];
                s+=c;
                if(s.size()>4096)return {Type::Error,"token too long",line};
            }
            return {Type::Error,"unclosed quote",line};
        }
        if(c=='/'||c==';'||c=='\0')
            return {Type::Error,"invalid unquoted token",line};
        std::string s(1,c);
        while(index_<source_.size()){
            c=source_[index_];
            if(c==' '||c=='\r'||c=='\t'||c=='\n'||c=='{'||c=='}')break;
            if(c=='/'&&index_+1<source_.size()&&source_[index_+1]=='/')break;
            if(c=='"')return {Type::Error,"unexpected quote",line};
            s+=c;++index_;
            if(s.size()>4096)return {Type::Error,"token too long",line};
        }
        return {Type::Word,std::move(s),line};
    }
private:
    std::string_view source_;
    std::size_t index_=0,line_=1;
};
inline bool Fail(std::string& error,const char* reason,std::size_t line){
    error=std::string(reason)+" at line "+std::to_string(line);
    return false;
}
inline const Definition* FindDefinition(const std::vector<Definition>& d,
                                        std::string_view name){
    for(const auto& item:d)if(item.name==name)return &item;
    return nullptr;
}
inline bool ParseDefinitions(std::string_view text,
                             std::vector<Definition>& out,
                             std::string& error){
    error.clear();
    if(text.size()>256*1024)return Fail(error,"definitions file too large",1);
    Lexer lex(text);
    auto tok=lex.Next();
    if(tok.type!=Lexer::Type::Word||tok.text!="StatDefinitions")
        return Fail(error,"missing StatDefinitions root",tok.line);
    tok=lex.Next();
    if(tok.type!=Lexer::Type::Open)
        return Fail(error,"missing root open brace",tok.line);
    std::vector<Definition> pending;
    while(true){
        tok=lex.Next();
        if(tok.type==Lexer::Type::Close)break;
        if(tok.type!=Lexer::Type::Word||tok.text!="stat")
            return Fail(error,"expected stat object",tok.line);
        const auto open=lex.Next();
        if(open.type!=Lexer::Type::Open)
            return Fail(error,"expected stat open brace",open.line);
        Definition d;
        unsigned seen=0;
        while(true){
            tok=lex.Next();
            if(tok.type==Lexer::Type::Close)break;
            if(tok.type!=Lexer::Type::Word)
                return Fail(error,"expected stat property",tok.line);
            const auto value=lex.Next();
            if(value.type!=Lexer::Type::Word)
                return Fail(error,"expected property string",value.line);
            unsigned bit=0;
            if(tok.text=="name"){bit=1;d.name=value.text;}
            else if(tok.text=="short_text"){bit=2;d.short_text=value.text;}
            else if(tok.text=="long_text"){bit=4;d.long_text=value.text;}
            else if(tok.text=="percentage_of"){bit=8;d.percentage_of=value.text;}
            else return Fail(error,"unknown stat property",tok.line);
            if((seen&bit)!=0)return Fail(error,"duplicate stat property",tok.line);
            seen|=bit;
        }
        if((seen&7)!=7||d.name.empty())
            return Fail(error,"missing required stat property",tok.line);
        if(FindDefinition(pending,d.name))
            return Fail(error,"duplicate stat name",tok.line);
        pending.push_back(std::move(d));
        if(pending.size()>256)return Fail(error,"too many stat definitions",tok.line);
    }
    tok=lex.Next();
    if(tok.type!=Lexer::Type::End)
        return Fail(error,"trailing definitions content",tok.line);
    for(const auto& d:pending){
        if(!d.percentage_of.empty()&&!FindDefinition(pending,d.percentage_of))
            return Fail(error,"unresolved percentage_of",1);
    }
    out.swap(pending);
    return true;
}
inline bool ValidFormat(std::string_view v){
    return v=="tab"||v=="title"||v=="value"||v=="percent"||
        v=="percent_of"||v=="time"||v=="dynamic_block"||
        v=="highest_name"||v=="lowest_name"||
        v=="highest_value"||v=="lowest_value";
}
inline bool IsDivider(std::string_view v){
    return v.size()==26&&v.find_first_not_of('=')==std::string_view::npos;
}
inline bool ParseDisplayLayout(std::string_view text,
                               const std::vector<Definition>& definitions,
                               std::vector<DisplayEntry>& out,
                               std::string& error){
    error.clear();
    if(text.size()>256*1024)return Fail(error,"layout file too large",1);
    Lexer lex(text);
    std::vector<DisplayEntry> pending;
    while(true){
        auto key=lex.Next();
        if(key.type==Lexer::Type::End)break;
        if(key.type!=Lexer::Type::Word)
            return Fail(error,"expected layout name",key.line);
        DisplayEntry e;
        e.name=std::move(key.text);
        e.line=key.line;
        if(IsDivider(e.name))e.divider=true;
        else {
            auto value=lex.Next();
            if(value.type!=Lexer::Type::Word||value.line!=e.line)
                return Fail(error,"layout format must be on same line",key.line);
            if(!ValidFormat(value.text))
                return Fail(error,"unrecognized layout format",value.line);
            e.format=std::move(value.text);
            if(e.name.compare(0,5,"STAT_")==0){
                const auto* def=FindDefinition(definitions,e.name);
                if(!def)return Fail(error,"undefined stat referenced by layout",e.line);
                if(e.format=="percent_of"&&def->percentage_of.empty())
                    return Fail(error,"percent_of stat lacks definition reference",e.line);
            }
        }
        pending.push_back(std::move(e));
        if(pending.size()>2048)return Fail(error,"too many display layout items",key.line);
    }
    out.swap(pending);
    return true;
}
} // namespace sin_independent::stats_scripts
