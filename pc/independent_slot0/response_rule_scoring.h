#pragma once
#include "response_rule_matcher.h"
#include <cstdint>
#include <cstring>
namespace sin_port::response_rule {
inline constexpr unsigned criterion_stride=0x14c, criterion_bytes=0x138;
inline constexpr unsigned rule_stride=0x40,rule_bytes=0x2c;
inline constexpr unsigned row_limit=4096;
struct Bytes {
    const unsigned char* p=nullptr;unsigned n=0;
    bool has(unsigned size) const noexcept{return p&&n>=size;}
    template<class T> T get(unsigned off) const noexcept {T v{};std::memcpy(&v,p+off,sizeof(v));return v;}
};
struct Scoring {
    void* context=nullptr;
    // Indexed payloads (after the 0x14-byte tree-node prefix).
    bool (*criterion)(void*,unsigned,Bytes*) noexcept=nullptr;
    bool (*rule)(void*,unsigned,Bytes*) noexcept=nullptr;
    bool (*storage)(void*,std::uint32_t,Bytes*) noexcept=nullptr;
    bool (*string)(void*,std::uint32_t,const char**) noexcept=nullptr;
    // Return empty string/default 1 for absent criterion, null actual
    // for the retail null GetString path (no required failure).
    bool (*lookup)(void*,const char*,const char**,float*) noexcept=nullptr;
    bool (*draw)(void*,int,int,int*) noexcept=nullptr;
    Enumeration enumeration{};
};
struct Score {double value=0;bool required_failed=false;};
inline bool score_criterion_impl(const Scoring& s,unsigned idx,Score& out,unsigned depth,unsigned& budget) noexcept {
    if(depth>=64||!budget||idx>=row_limit)return false;
    --budget;Bytes row{};
    if(!s.criterion(s.context,idx,&row)||!row.has(criterion_bytes))return false;
    const auto count=row.get<int>(0x130);const auto weight=row.get<float>(8);
    const bool required=row.p[0xc]!=0;
    if(count>0){
        Bytes ids{};if(count>int(row_limit)||!s.storage(s.context,row.get<std::uint32_t>(0x124),&ids)||!ids.has(unsigned(count)*4))return false;
        float sum=0;
        for(int i=0;i<count;++i){
            Score child{};
            if(!score_criterion_impl(s,ids.get<std::uint32_t>(unsigned(i)*4),child,depth+1,budget))return false;
            // 220930e0 ignores the child's required flag; only the
            // composite's own required flag and zero sum matter.
            sum=static_cast<float>(double(sum)+child.value);
        }
        out={double(sum)*weight,required&&sum==0.f};return true;
    }
    const char* key=nullptr,*actual="";float input_weight=1.f;
    if(!s.string(s.context,row.get<std::uint32_t>(0),&key)||!key||
       !s.lookup(s.context,key,&actual,&input_weight))return false;
    if(!actual){out={};return true;}
    bool accepted=false;
    if(!match(row.p+0x10,matcher_bytes,actual,s.enumeration,accepted))return false;
    out={accepted?static_cast<float>(double(input_weight)*weight):0.f,!accepted&&required};
    return true;
}
inline bool score_criterion(const Scoring& s,unsigned idx,Score& out) noexcept {
    if(!s.criterion||!s.storage||!s.string||!s.lookup)return false;
    unsigned budget=65536;return score_criterion_impl(s,idx,out,0,budget);
}
// 220995b0: disabled rules and first required failure both score zero.
inline bool score_rule(const Scoring& s,unsigned idx,float& out) noexcept {
    if(!s.rule||!s.criterion||!s.storage||!s.string||!s.lookup||idx>=row_limit)return false;
    Bytes row{};if(!s.rule(s.context,idx,&row)||!row.has(rule_bytes))return false;
    if(!row.p[0x29]){out=0;return true;}
    const int count=row.get<int>(0xc);float sum=0;
    if(count>0){
        Bytes ids{};
        if(count>int(row_limit)||!s.storage(s.context,row.get<std::uint32_t>(0),&ids)||!ids.has(unsigned(count)*4))return false;
        unsigned budget=65536;
        for(int i=0;i<count;++i){
            Score one{};if(!score_criterion_impl(s,ids.get<std::uint32_t>(unsigned(i)*4),one,0,budget))return false;
            sum=static_cast<float>(double(sum)+one.value);
            if(one.required_failed){out=0;return true;}
        }
    }
    out=sum;return true;
}
// 22099ae0: score all ordinal rules, initial threshold 0.001f,
// replace tie list on greater score, append on equality, one RNG
// call only for multiple tied rules. NaN rejected by first x87 test.
inline bool best_rule(const Scoring& s,unsigned count,int* scratch,unsigned capacity,int& out) noexcept {
    if(count>row_limit||capacity<count||(count&&!scratch))return false;
    float best=0.001f;unsigned ties=0;
    for(unsigned i=0;i<count;++i){
        float score=0;if(!score_rule(s,i,score))return false;
        if(!(score>=best))continue;
        if(score>best){best=score;ties=0;}
        scratch[ties++]=static_cast<int>(i);
    }
    int selected=-1;
    if(ties==1)selected=scratch[0];
    else if(ties>1){
        int draw=0;if(!s.draw||!s.draw(s.context,0,static_cast<int>(ties-1),&draw)||draw<0||unsigned(draw)>=ties)return false;
        selected=scratch[draw];
    }
    out=selected;return true;
}
}
