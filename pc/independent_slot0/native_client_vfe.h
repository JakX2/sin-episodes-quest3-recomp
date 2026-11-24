#pragma once
#include "native_client_phoneme_emphasis.h"
#include <cstring>
#include <cstddef>
namespace sin_port::native_weapon_tables {
inline constexpr char client_vfe_export[]="SiN_GetClientVfeApi";
struct ClientVfeView {const unsigned char* data;unsigned size;};
using ClientVfeMap=int (__cdecl *)(void*,const char*) noexcept;
// Return 1 setting found, 0 absent, -1 invalid. Caller provides storage for
// resolved rows and controller-name registry. No filesystem/model access.
struct ClientVfeApi {
    std::uint32_t version,size;
    int (__cdecl *resolve)(ClientVfeView,ClientVfeView,int,void*,ClientVfeMap,
        ClientPhonemeContribution*,unsigned,unsigned*) noexcept;
};
using GetClientVfeApi=const ClientVfeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
namespace vfe_detail {
struct Reader {
    ClientVfeView v;
    int settings{},settings_off{},codes{},codes_off{},keys{},names_off{},map_off{};
    bool span(std::int64_t pos,std::int64_t n) const noexcept {
        return pos>=0&&n>=0&&pos<=v.size&&n<=static_cast<std::int64_t>(v.size)-pos;
    }
    int word(std::int64_t pos) const noexcept {
        int x=0;std::memcpy(&x,v.data+pos,4);return x;
    }
    const char* name(std::int64_t pos) const noexcept {
        if(!span(pos,1))return nullptr;
        const auto* s=reinterpret_cast<const char*>(v.data+pos);
        return std::memchr(s,0,v.size-static_cast<unsigned>(pos))?s:nullptr;
    }
    bool init() noexcept {
        if(!v.data||!span(0,108)||std::memcmp(v.data,"EFV",4)!=0||word(4)!=0)return false;
        const int length=word(0x48);
        if(length<108||static_cast<unsigned>(length)>v.size)return false;
        v.size=static_cast<unsigned>(length);
        settings=word(0x4c);settings_off=word(0x50);
        codes=word(0x58);codes_off=word(0x5c);
        keys=word(0x60);names_off=word(0x64);map_off=word(0x68);
        return span(settings_off,24LL*settings)&&span(codes_off,4LL*codes)&&
            span(names_off,4LL*keys)&&span(map_off,4LL*keys);
    }
    std::int64_t setting(int i) const noexcept {return settings_off+24LL*i;}
    const char* setting_name(int i) const noexcept {
        const auto s=setting(i);return name(s+word(s));
    }
    int by_code(int code,int& i) const noexcept {
        if(code<0||code>=codes)return 0;
        i=word(codes_off+4LL*code);
        if(i==-1)return 0;
        return i>=0&&i<settings?1:-1;
    }
    int terminal(int& i) const noexcept {
        for(int steps=0;steps<settings;++steps){
            const auto s=setting(i);
            if(word(s+4)!=1)return 1;
            const int count=word(s+8),selected=word(s+16);
            if(selected<0||selected>=count)return -1;
            const auto off=s+word(s+20);
            if(!span(off,8LL*count))return -1;
            i=word(off+8LL*selected);
            if(i<0||i>=settings)return -1;
        }
        return -1; // cyclic alias
    }
};
inline unsigned char fold(unsigned char c) noexcept {return c>='A'&&c<='Z'?static_cast<unsigned char>(c+32):c;}
inline bool equal(const char* a,const char* b) noexcept {
    for(;*a&&*b;++a,++b)if(fold(*a)!=fold(*b))return false;
    return *a==*b;
}
inline int resolve(ClientVfeView generic,ClientVfeView override_view,int code,void* ctx,ClientVfeMap map,
    ClientPhonemeContribution* out,unsigned capacity,unsigned* count) noexcept {
    if(!count||!map||(!out&&capacity))return -1;
    Reader base{generic},over{override_view};
    if(!generic.data&&!generic.size){*count=0;return 0;}
    if(!base.init())return -1;
    int i=0;const int found=base.by_code(code,i);
    if(found<=0){if(!found)*count=0;return found;}
    if(base.terminal(i)!=1)return -1;
    const auto* wanted=base.setting_name(i);
    if(!wanted)return -1;
    const Reader* source=&base;
    if(override_view.data||override_view.size){
        if(!over.init())return -1;
        for(int j=0;j<over.settings;++j){
            const auto* n=over.setting_name(j);
            if(!n)return -1;
            if(equal(n,wanted)){source=&over;i=j;if(over.terminal(i)!=1)return -1;break;}
        }
    }
    const auto s=source->setting(i);
    const int n=source->word(s+8);
    const auto off=s+source->word(s+20);
    if(n<0||static_cast<unsigned>(n)>capacity||n>4096||!source->span(off,12LL*n))return -1;
    ClientPhonemeContribution tmp[4096];
    for(int j=0;j<n;++j){
        const auto row=off+12LL*j;const int key=source->word(row);
        if(key<0||key>=source->keys)return -1;
        const auto* name=source->name(source->word(source->names_off+4LL*key));
        if(!name)return -1;
        const int global=map(ctx,name);
        float weight=0;std::memcpy(&weight,source->v.data+row+4,4);
        if(global<0||global>=128||!std::isfinite(weight))return -1;
        tmp[j]={static_cast<unsigned>(global),weight};
    }
    if(n)std::memcpy(out,tmp,static_cast<unsigned>(n)*sizeof(*out));
    *count=static_cast<unsigned>(n);return 1;
}
}
}
