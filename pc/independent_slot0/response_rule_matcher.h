#pragma once
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <cstdio>

namespace sin_port::response_rule {
// Original compiled matcher at criterion +0x10. All multibyte fields
// are PE32 little-endian; no native pointer or compiler packing dependency.
inline constexpr unsigned matcher_bytes=0x114;
inline float read_float(const unsigned char* p,unsigned offset) noexcept {
    float value;std::memcpy(&value,p+offset,4);return value;
}
inline void write_float(unsigned char* p,unsigned offset,float value) noexcept {
    std::memcpy(p+offset,&value,4);
}
struct Enumeration {
    void* context=nullptr;
    // Original enum lookup returns 0 for missing. Distinguish host failure
    // from absent enumeration and consume full bracketed name.
    bool (*lookup)(void*,const char*,float*,bool*) noexcept=nullptr;
};
inline bool ascii_equal(const char* a,const char* b) noexcept {
    auto fold=[](unsigned char ch){return ch>='A'&&ch<='Z'?static_cast<unsigned char>(ch+32):ch;};
    while(*a&&*b){if(fold(*a++)!=fold(*b++))return false;}
    return *a==*b;
}
inline bool terminated(const char* s,unsigned n) noexcept {return s&&std::memchr(s,0,n)!=nullptr;}
inline double numeric(const char* s) noexcept {return std::atof(s);}
inline bool numeric_literal(const char* s) noexcept {
    // 220915c0: nonzero atof OR a string containing only zero characters.
    if(numeric(s)!=0.0)return true;
    for(;*s;++s)if(*s!='0')return false;
    return true;
}
inline bool resolve_enum(const Enumeration& e,const char* name,float& value,bool& found) noexcept {
    value=0;found=false;
    return e.lookup&&e.lookup(e.context,name,&value,&found);
}
// 22099290, evaluate already-compiled matcher. Return false only for
// invalid input/transport; result is the retail match decision.
inline bool match(const unsigned char* m,unsigned n,const char* actual,const Enumeration& e,bool& result) noexcept {
    if(!m||n<matcher_bytes||!actual||!terminated(reinterpret_cast<const char*>(m+0x14),128))return false;
    result=false;if(!m[0])return true;
    float x=static_cast<float>(numeric(actual));
    if(*actual=='['){bool found=false;if(!resolve_enum(e,actual,x,found))return false;if(!found)x=0;}
    // Retail x87 unordered comparisons accept NaN for bounds, but
    // numeric equality is ordered and numeric inequality is unordered-or-not-equal.
    if(m[3]&&!(std::isnan(x)||std::isnan(read_float(m,8))||
               (m[4]?x>=read_float(m,8):x>read_float(m,8))))return true;
    if(m[0xc]&&!(std::isnan(x)||std::isnan(read_float(m,0x10))||
                 (m[0xd]?x<=read_float(m,0x10):x<read_float(m,0x10))))return true;
    if(m[3]||m[0xc]){result=true;return true;}
    const auto* expected=reinterpret_cast<const char*>(m+0x14);
    if(m[1]){
        const double y=numeric(expected);
        result=m[2]?double(x)!=y:(*actual&&double(x)==y);
    }else{
        const bool same=ascii_equal(actual,expected);
        result=m[2]?!same:same;
    }
    return true;
}
// 220990f0 builds matcher from criterion value. Fresh zeroed destination;
// reject token overflow instead of overflowing original 128-byte scratch.
inline bool compile(const char* expression,const Enumeration& e,unsigned char* m,unsigned n) noexcept {
    if(!m||n<matcher_bytes)return false;
    std::memset(m,0,matcher_bytes);
    if(!expression)return true;
    char token[128]{};unsigned used=0;
    bool lower=false,upper=false,inclusive=false,negated=false;
    for(const char* p=expression;;++p){
        const char ch=*p;
        if(ch=='>')lower=true;
        else if(ch=='<')upper=true;
        else if(ch=='=')inclusive=true;
        else if(ch=='!')negated=true;
        else if(ch==','||!ch){
            token[used]=0;
            auto* text=reinterpret_cast<char*>(m+0x14);
            auto* scratch=reinterpret_cast<char*>(m+0x94);
            std::memcpy(scratch,token,used+1);std::memcpy(text,token,used+1);
            if(token[0]=='['){
                float value=0;bool found=false;
                if(!resolve_enum(e,token,value,found))return false;
                if(found){
                    // Original Q_snprintf uses %f.
                    const int written=std::snprintf(text,128,"%f",double(value));
                    if(written<0||written>=128)return false;
                }
            }
            if(lower){m[3]=1;m[4]=inclusive;write_float(m,8,static_cast<float>(numeric(text)));m[1]=1;}
            else if(upper){m[0xc]=1;m[0xd]=inclusive;write_float(m,0x10,static_cast<float>(numeric(text)));m[1]=1;}
            else{m[2]=negated;m[1]=numeric_literal(text);}
            used=0;lower=upper=inclusive=negated=false;
            if(!ch)break;
        }else{
            if(used==127)return false;
            token[used++]=ch;
        }
    }
    m[0]=1;return true;
}
}
